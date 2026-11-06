// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MPSecurityTypes.generated.h"

/**
 * Severity level of an authoritative security or exploit check violation.
 */
UENUM(BlueprintType)
enum class EMPSecurityViolationSeverity : uint8
{
	Info          UMETA(DisplayName = "Info / Telemetry"),
	Warning       UMETA(DisplayName = "Warning (Potential Network Jitter)"),
	Suspicious    UMETA(DisplayName = "Suspicious (Repeated Anomalies)"),
	Severe        UMETA(DisplayName = "Severe (Clear Rule Breach)"),
	Critical      UMETA(DisplayName = "Critical (Immediate Mitigation Required)")
};

/**
 * Categorization of security violation types.
 */
UENUM(BlueprintType)
enum class EMPSecurityViolationType : uint8
{
	SpeedHack             UMETA(DisplayName = "Speed Hack (Excessive Velocity)"),
	TeleportExploit       UMETA(DisplayName = "Teleport Exploit (Unregistered Relocation)"),
	FireRateExceeded      UMETA(DisplayName = "Fire Rate Exceeded (Trigger Automation)"),
	DamageExceeded        UMETA(DisplayName = "Damage Exceeded (Impossibly High Damage)"),
	InvalidTimestamp      UMETA(DisplayName = "Invalid Timestamp (Future or Replay)"),
	RPCFlood              UMETA(DisplayName = "RPC Flood (Denial of Service Spike)"),
	DuplicateTransaction  UMETA(DisplayName = "Duplicate Transaction (Inventory Dupe Attempt)"),
	InvalidInventoryAction UMETA(DisplayName = "Invalid Inventory Action"),
	Custom                UMETA(DisplayName = "Custom Violation")
};

/**
 * Security audit record structure for logging and moderation.
 */
USTRUCT(BlueprintType)
struct MPUTILITYRUNTIME_API FMPSecurityViolation
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Networking|Security")
	FString PlayerUniqueId;

	UPROPERTY(BlueprintReadOnly, Category = "Networking|Security")
	EMPSecurityViolationType ViolationType = EMPSecurityViolationType::Custom;

	UPROPERTY(BlueprintReadOnly, Category = "Networking|Security")
	EMPSecurityViolationSeverity Severity = EMPSecurityViolationSeverity::Warning;

	UPROPERTY(BlueprintReadOnly, Category = "Networking|Security")
	FString Details;

	UPROPERTY(BlueprintReadOnly, Category = "Networking|Security")
	double ServerTimestamp = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Networking|Security")
	int32 AccumulativeCount = 1;

	FMPSecurityViolation() = default;
};

/**
 * Helper to validate weapon and action fire rates using sliding window timestamps.
 */
USTRUCT(BlueprintType)
struct MPUTILITYRUNTIME_API FMPFireRateValidator
{
	GENERATED_BODY()

	/** Minimum time allowed between consecutive triggers in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Security")
	float MinShotInterval = 0.1f; // 10 shots/sec max by default

	/** Maximum allowed burst count within sliding burst window. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Security")
	int32 MaxBurstCount = 5;

	/** Sliding burst window in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Security")
	float BurstWindowSeconds = 0.45f;

	UPROPERTY()
	double LastShotServerTime = -100.0;

	TArray<double> RecentShotTimes;

	FMPFireRateValidator() = default;

	/**
	 * Validates whether a new shot/action is permitted at the specified server timestamp.
	 * Returns true if valid, false if fire rate is violated.
	 */
	bool ValidateShot(double CurrentServerTime, FString& OutReason);

	void Reset()
	{
		LastShotServerTime = -100.0;
		RecentShotTimes.Empty();
	}
};

/**
 * Movement exploit detector tracking position deltas, speed hacks, and unregistered teleports.
 */
USTRUCT(BlueprintType)
struct MPUTILITYRUNTIME_API FMPMovementExploitDetector
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Networking|Security")
	FVector LastVerifiedLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Networking|Security")
	double LastVerifiedTimestamp = 0.0;

	/** Maximum expected speed in units/sec. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Security")
	float MaxAllowedSpeed = 1000.0f;

	/** Tolerance margin added for network latency and desync in world units. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Security")
	float ToleranceMargin = 150.0f;

	/** Distance jump threshold that triggers teleport flag. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Security")
	float TeleportThresholdDistance = 2500.0f;

	FMPMovementExploitDetector() = default;

	/**
	 * Updates and validates an authoritative movement step.
	 * Returns true if clean, false if an exploit anomaly was detected.
	 */
	bool ValidateStep(
		const FVector& NewLocation,
		double CurrentServerTime,
		bool& bOutTeleportDetected,
		float& OutSpeedError
	);

	void Reset(const FVector& InitialLocation, double InitialTimestamp)
	{
		LastVerifiedLocation = InitialLocation;
		LastVerifiedTimestamp = InitialTimestamp;
	}
};

