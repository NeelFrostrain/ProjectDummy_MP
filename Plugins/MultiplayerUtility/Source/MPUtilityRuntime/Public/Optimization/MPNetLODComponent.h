// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Optimization/MPNetOptimizationTypes.h"
#include "MPNetLODComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMPOnReplicationLODTierChangedDelegate, EMPReplicationLODTier, NewTier, EMPReplicationLODTier, OldTier);

/**
 * Server-only component that dynamically scales NetUpdateFrequency and Dormancy
 * based on the actor's distance to the nearest connected player pawn.
 */
UCLASS(ClassGroup = (Networking), meta = (BlueprintSpawnableComponent, DisplayName = "MP Net LOD Component"))
class MPUTILITYRUNTIME_API UMPNetLODComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMPNetLODComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** LOD distance thresholds and frequency tiers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Optimization")
	FMPReplicationLODSettings LODSettings;

	/** Evaluation period in seconds between spatial distance checks (avoids per-frame polling). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Optimization", meta = (ClampMin = "0.2", ClampMax = "10.0"))
	float EvaluationIntervalSeconds = 1.0f;

	/** Current evaluated replication tier. */
	UPROPERTY(BlueprintReadOnly, Category = "Networking|Optimization")
	EMPReplicationLODTier CurrentLODTier = EMPReplicationLODTier::NearHighPriority;

	/** Fired when the actor transitions between replication LOD tiers. */
	UPROPERTY(BlueprintAssignable, Category = "Networking|Optimization")
	FMPOnReplicationLODTierChangedDelegate OnLODTierChanged;

	/**
	 * Manually forces an immediate LOD re-evaluation.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Networking|Optimization")
	void ForceEvaluateLOD();

private:
	void EvaluateLOD();

	FTimerHandle EvalTimerHandle;
};

