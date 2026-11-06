// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Security/MPSecurityTypes.h"
#include "MPServerSecurityLibrary.generated.h"

/**
 * Server-authoritative security validation function library for anti-cheat and exploit prevention.
 */
UCLASS()
class MPUTILITYRUNTIME_API UMPServerSecurityLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Validates a client damage request on the authoritative server.
	 * Checks damage magnitude limits, attacker/target validity, maximum reach distance, and line-of-sight.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Networking|Security")
	static bool ValidateDamageRequestAuthoritative(
		AActor* Attacker,
		AActor* Target,
		float RequestedDamage,
		float MaxPossibleDamage,
		float MaxAttackDistance,
		bool bRequireLineOfSight,
		FString& OutReason
	);

	/**
	 * Validates client timestamp against server time to prevent speed hacks, desync replay, and future timestamps.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Networking|Security")
	static bool ValidateClientTimestampDrift(
		double ClientTimestamp,
		double ServerTimestamp,
		float MaxAllowedDriftSeconds,
		FString& OutReason
	);

	/**
	 * Validates and records a unique item/currency transaction identifier to prevent duplication exploits.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Networking|Security")
	static bool ValidateTransactionNonDuplicated(
		const FGuid& TransactionId,
		UPARAM(ref) TArray<FGuid>& InOutProcessedTransactionHistory,
		int32 MaxHistoryCapacity = 1000
	);
};

