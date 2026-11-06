// Copyright Epic Games, Inc. All Rights Reserved.

#include "Security/MPServerSecurityLibrary.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

bool UMPServerSecurityLibrary::ValidateDamageRequestAuthoritative(
	AActor* Attacker,
	AActor* Target,
	float RequestedDamage,
	float MaxPossibleDamage,
	float MaxAttackDistance,
	bool bRequireLineOfSight,
	FString& OutReason)
{
	OutReason.Empty();

	if (!Attacker || !Target)
	{
		OutReason = TEXT("Attacker or Target actor is null.");
		return false;
	}

	// 1. Damage bounds check
	if (RequestedDamage <= 0.0f)
	{
		OutReason = TEXT("Requested damage must be positive.");
		return false;
	}

	if (RequestedDamage > MaxPossibleDamage)
	{
		OutReason = FString::Printf(TEXT("Damage %.1f exceeds weapon/ability maximum %.1f."), RequestedDamage, MaxPossibleDamage);
		return false;
	}

	// 2. Distance check
	const float Distance = FVector::Dist(Attacker->GetActorLocation(), Target->GetActorLocation());
	if (Distance > MaxAttackDistance + 100.0f) // 100 unit tolerance for network lag
	{
		OutReason = FString::Printf(TEXT("Distance %.1f exceeds attack range %.1f."), Distance, MaxAttackDistance);
		return false;
	}

	// 3. Optional Line-of-sight check
	if (bRequireLineOfSight)
	{
		UWorld* World = Attacker->GetWorld();
		if (World)
		{
			FHitResult Hit;
			FCollisionQueryParams Params(SCENE_QUERY_STAT(SecurityDamageLOS), true);
			Params.AddIgnoredActor(Attacker);
			Params.AddIgnoredActor(Target);

			const bool bBlocked = World->LineTraceSingleByChannel(
				Hit,
				Attacker->GetActorLocation(),
				Target->GetActorLocation(),
				ECC_Visibility,
				Params
			);

			if (bBlocked && Hit.GetActor() != Target)
			{
				OutReason = FString::Printf(TEXT("Attack line of sight blocked by %s."), *GetNameSafe(Hit.GetActor()));
				return false;
			}
		}
	}

	return true;
}

bool UMPServerSecurityLibrary::ValidateClientTimestampDrift(
	double ClientTimestamp,
	double ServerTimestamp,
	float MaxAllowedDriftSeconds,
	FString& OutReason)
{
	OutReason.Empty();

	if (ClientTimestamp <= 0.0)
	{
		OutReason = TEXT("Client timestamp must be positive.");
		return false;
	}

	const double Difference = ClientTimestamp - ServerTimestamp;

	// Check if timestamp is too far in the future
	if (Difference > static_cast<double>(MaxAllowedDriftSeconds))
	{
		OutReason = FString::Printf(TEXT("Client timestamp is %.2fs in the future (Max allowed: %.2fs)."),
			Difference, MaxAllowedDriftSeconds);
		return false;
	}

	// Check if timestamp is unreasonably old (replay or lag switch)
	const double MaxPastDrift = static_cast<double>(MaxAllowedDriftSeconds) * 2.0;
	if (-Difference > MaxPastDrift)
	{
		OutReason = FString::Printf(TEXT("Client timestamp is %.2fs in the past (Max allowed: %.2fs)."),
			-Difference, MaxPastDrift);
		return false;
	}

	return true;
}

bool UMPServerSecurityLibrary::ValidateTransactionNonDuplicated(
	const FGuid& TransactionId,
	TArray<FGuid>& InOutProcessedTransactionHistory,
	int32 MaxHistoryCapacity)
{
	if (!TransactionId.IsValid())
	{
		return false;
	}

	if (InOutProcessedTransactionHistory.Contains(TransactionId))
	{
		// Duplication attempt detected!
		return false;
	}

	// Prune oldest if at capacity
	if (InOutProcessedTransactionHistory.Num() >= MaxHistoryCapacity)
	{
		InOutProcessedTransactionHistory.RemoveAt(0);
	}

	InOutProcessedTransactionHistory.Add(TransactionId);
	return true;
}

