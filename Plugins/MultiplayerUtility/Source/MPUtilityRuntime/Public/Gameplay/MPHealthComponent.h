// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MPHealthComponent.generated.h"

/**
 * Health and vitality states for server-authoritative damage handling.
 */
UENUM(BlueprintType)
enum class EMPHealthState : uint8
{
	Alive         UMETA(DisplayName = "Alive"),
	Downed        UMETA(DisplayName = "Downed (Awaiting Revive)"),
	Dead          UMETA(DisplayName = "Dead"),
	Invulnerable  UMETA(DisplayName = "Invulnerable")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FMPOnHealthChangedDelegate, float, CurrentHealth, float, MaxHealth, float, Delta, AActor*, DamageCauser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FMPOnShieldChangedDelegate, float, CurrentShield, float, MaxShield, float, Delta, AActor*, DamageCauser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMPOnDeathDelegate, AActor*, Victim, AController*, Killer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMPOnDownedDelegate, AActor*, DownedActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMPOnRevivedDelegate, AActor*, RevivedActor);

/**
 * Server-authoritative health, shield, damage absorption, down, and revive component.
 */
UCLASS(ClassGroup = (Networking), meta = (BlueprintSpawnableComponent, DisplayName = "MP Health Component"))
class MPUTILITYRUNTIME_API UMPHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMPHealthComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --------------------------------------------------------------------------
	// Health & Shield Configuration
	// --------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Health", meta = (ClampMin = "1.0", ClampMax = "100000.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth, BlueprintReadOnly, Category = "Networking|Health")
	float CurrentHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Health", meta = (ClampMin = "0.0", ClampMax = "100000.0"))
	float MaxShield = 50.0f;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentShield, BlueprintReadOnly, Category = "Networking|Health")
	float CurrentShield = 50.0f;

	/** If true, player transitions to Downed state instead of immediate death when health reaches 0. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Health")
	bool bCanBeDowned = false;

	UPROPERTY(ReplicatedUsing = OnRep_HealthState, BlueprintReadOnly, Category = "Networking|Health")
	EMPHealthState HealthState = EMPHealthState::Alive;

	// --------------------------------------------------------------------------
	// Delegates
	// --------------------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "Networking|Health")
	FMPOnHealthChangedDelegate OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Networking|Health")
	FMPOnShieldChangedDelegate OnShieldChanged;

	UPROPERTY(BlueprintAssignable, Category = "Networking|Health")
	FMPOnDeathDelegate OnDeath;

	UPROPERTY(BlueprintAssignable, Category = "Networking|Health")
	FMPOnDownedDelegate OnDowned;

	UPROPERTY(BlueprintAssignable, Category = "Networking|Health")
	FMPOnRevivedDelegate OnRevived;

	// --------------------------------------------------------------------------
	// Server-Authoritative API
	// --------------------------------------------------------------------------

	/**
	 * Applies authoritative damage, absorbing through shield first.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Networking|Health")
	float ApplyDamageServerOnly(float DamageAmount, AActor* DamageCauser = nullptr, AController* InstigatorController = nullptr);

	/**
	 * Applies authoritative healing up to MaxHealth.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Networking|Health")
	float ApplyHealingServerOnly(float HealAmount);

	/**
	 * Revives a downed player with a percentage of MaxHealth.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Networking|Health")
	bool RevivePlayerServerOnly(float HealthPercentage = 0.5f);

	/**
	 * Sets invulnerability state on server.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Networking|Health")
	void SetInvulnerableServerOnly(bool bInvulnerable);

	UFUNCTION(BlueprintPure, Category = "Networking|Health", meta = (DisplayName = "Is Health Alive"))
	bool IsHealthAlive() const { return HealthState == EMPHealthState::Alive || HealthState == EMPHealthState::Invulnerable; }

	// C++ inline convenience alias
	bool IsAlive() const { return IsHealthAlive(); }

	UFUNCTION(BlueprintPure, Category = "Networking|Health")
	bool IsDowned() const { return HealthState == EMPHealthState::Downed; }

	UFUNCTION(BlueprintPure, Category = "Networking|Health")
	bool IsDead() const { return HealthState == EMPHealthState::Dead; }

protected:
	UFUNCTION()
	void OnRep_CurrentHealth(float OldHealth);

	UFUNCTION()
	void OnRep_CurrentShield(float OldShield);

	UFUNCTION()
	void OnRep_HealthState(EMPHealthState OldState);
};
