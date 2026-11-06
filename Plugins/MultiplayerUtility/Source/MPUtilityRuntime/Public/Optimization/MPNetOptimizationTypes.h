// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MPNetOptimizationTypes.generated.h"

/**
 * Replication Level of Detail (LOD) tier based on spatial proximity to players.
 */
UENUM(BlueprintType)
enum class EMPReplicationLODTier : uint8
{
	NearHighPriority   UMETA(DisplayName = "Near (High Update Frequency)"),
	MidStandard        UMETA(DisplayName = "Mid (Standard Update Frequency)"),
	FarThrottled       UMETA(DisplayName = "Far (Throttled Update Frequency)"),
	Dormant            UMETA(DisplayName = "Dormant (Network Dormancy Enabled)")
};

/**
 * Distance thresholds and update frequencies for dynamic actor replication LOD.
 */
USTRUCT(BlueprintType)
struct MPUTILITYRUNTIME_API FMPReplicationLODSettings
{
	GENERATED_BODY()

	/** Distance threshold in world units for near high-frequency replication tier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Optimization", meta = (ClampMin = "100.0", ClampMax = "10000.0"))
	float NearDistance = 1500.0f;

	/** Update frequency in Hz for near tier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Optimization", meta = (ClampMin = "1.0", ClampMax = "120.0"))
	float NearFrequencyHz = 60.0f;

	/** Distance threshold for mid standard-frequency tier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Optimization", meta = (ClampMin = "500.0", ClampMax = "30000.0"))
	float MidDistance = 4500.0f;

	/** Update frequency in Hz for mid tier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Optimization", meta = (ClampMin = "1.0", ClampMax = "60.0"))
	float MidFrequencyHz = 20.0f;

	/** Distance threshold for far throttled tier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Optimization", meta = (ClampMin = "1000.0", ClampMax = "100000.0"))
	float FarDistance = 9000.0f;

	/** Update frequency in Hz for far tier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Optimization", meta = (ClampMin = "0.5", ClampMax = "30.0"))
	float FarFrequencyHz = 4.0f;

	/** Distance at which actor should automatically go dormant if supported. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Optimization", meta = (ClampMin = "2000.0", ClampMax = "200000.0"))
	float AutoDormancyDistance = 15000.0f;

	/** Whether auto-dormancy transition is enabled at the dormancy distance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Optimization")
	bool bEnableDormancyAtDistance = true;

	FMPReplicationLODSettings() = default;

	/** Computes the appropriate replication LOD tier for a given distance to the nearest player. */
	EMPReplicationLODTier EvaluateDistance(float DistanceToNearestPlayer) const
	{
		if (bEnableDormancyAtDistance && DistanceToNearestPlayer >= AutoDormancyDistance)
		{
			return EMPReplicationLODTier::Dormant;
		}
		if (DistanceToNearestPlayer > MidDistance)
		{
			return EMPReplicationLODTier::FarThrottled;
		}
		if (DistanceToNearestPlayer > NearDistance)
		{
			return EMPReplicationLODTier::MidStandard;
		}
		return EMPReplicationLODTier::NearHighPriority;
	}

	/** Returns target update frequency in Hz for the evaluated LOD tier. */
	float GetFrequencyForTier(EMPReplicationLODTier Tier) const
	{
		switch (Tier)
		{
		case EMPReplicationLODTier::NearHighPriority: return NearFrequencyHz;
		case EMPReplicationLODTier::MidStandard:      return MidFrequencyHz;
		case EMPReplicationLODTier::FarThrottled:     return FarFrequencyHz;
		case EMPReplicationLODTier::Dormant:          return 0.5f;
		default:                                      return 20.0f;
		}
	}
};

