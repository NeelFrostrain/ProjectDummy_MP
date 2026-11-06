// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/MPHealthComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"

UMPHealthComponent::UMPHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UMPHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UMPHealthComponent, CurrentHealth);
	DOREPLIFETIME(UMPHealthComponent, CurrentShield);
	DOREPLIFETIME(UMPHealthComponent, HealthState);
}

float UMPHealthComponent::ApplyDamageServerOnly(float DamageAmount, AActor* DamageCauser, AController* InstigatorController)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	if (HealthState == EMPHealthState::Invulnerable || HealthState == EMPHealthState::Dead)
	{
		return 0.0f;
	}

	float RemainingDamage = DamageAmount;

	// 1. Absorb with shield first
	if (CurrentShield > 0.0f)
	{
		const float ShieldDamage = FMath::Min(CurrentShield, RemainingDamage);
		const float OldShield = CurrentShield;
		CurrentShield -= ShieldDamage;
		RemainingDamage -= ShieldDamage;
		OnShieldChanged.Broadcast(CurrentShield, MaxShield, -ShieldDamage, DamageCauser);
	}

	// 2. Absorb remaining damage with health
	if (RemainingDamage > 0.0f)
	{
		const float HealthDamage = FMath::Min(CurrentHealth, RemainingDamage);
		const float OldHealth = CurrentHealth;
		CurrentHealth -= HealthDamage;
		OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, -HealthDamage, DamageCauser);

		if (CurrentHealth <= 0.0f)
		{
			if (bCanBeDowned && HealthState == EMPHealthState::Alive)
			{
				HealthState = EMPHealthState::Downed;
				OnDowned.Broadcast(Owner);
			}
			else
			{
				HealthState = EMPHealthState::Dead;
				OnDeath.Broadcast(Owner, InstigatorController);
			}
		}
	}

	return DamageAmount;
}

float UMPHealthComponent::ApplyHealingServerOnly(float HealAmount)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || HealAmount <= 0.0f || HealthState == EMPHealthState::Dead)
	{
		return 0.0f;
	}

	const float OldHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth + HealAmount, 0.0f, MaxHealth);
	const float ActualHealed = CurrentHealth - OldHealth;

	if (ActualHealed > 0.0f)
	{
		OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, ActualHealed, nullptr);
	}

	return ActualHealed;
}

bool UMPHealthComponent::RevivePlayerServerOnly(float HealthPercentage)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || HealthState != EMPHealthState::Downed)
	{
		return false;
	}

	const float Percentage = FMath::Clamp(HealthPercentage, 0.1f, 1.0f);
	CurrentHealth = MaxHealth * Percentage;
	HealthState = EMPHealthState::Alive;

	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, CurrentHealth, nullptr);
	OnRevived.Broadcast(Owner);
	return true;
}

void UMPHealthComponent::SetInvulnerableServerOnly(bool bInvulnerable)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	if (bInvulnerable)
	{
		HealthState = EMPHealthState::Invulnerable;
	}
	else if (HealthState == EMPHealthState::Invulnerable)
	{
		HealthState = EMPHealthState::Alive;
	}
}

void UMPHealthComponent::OnRep_CurrentHealth(float OldHealth)
{
	const float Delta = CurrentHealth - OldHealth;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, Delta, nullptr);
}

void UMPHealthComponent::OnRep_CurrentShield(float OldShield)
{
	const float Delta = CurrentShield - OldShield;
	OnShieldChanged.Broadcast(CurrentShield, MaxShield, Delta, nullptr);
}

void UMPHealthComponent::OnRep_HealthState(EMPHealthState OldState)
{
	AActor* Owner = GetOwner();
	if (HealthState == EMPHealthState::Downed)
	{
		OnDowned.Broadcast(Owner);
	}
	else if (HealthState == EMPHealthState::Dead)
	{
		OnDeath.Broadcast(Owner, nullptr);
	}
	else if (OldState == EMPHealthState::Downed && HealthState == EMPHealthState::Alive)
	{
		OnRevived.Broadcast(Owner);
	}
}

