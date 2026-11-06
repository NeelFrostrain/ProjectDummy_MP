// Copyright Epic Games, Inc. All Rights Reserved.

#include "Optimization/MPNetLODComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "TimerManager.h"

UMPNetLODComponent::UMPNetLODComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false); // Optimization logic is server-only
}

void UMPNetLODComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World)
	{
		const float Interval = FMath::Clamp(EvaluationIntervalSeconds, 0.2f, 10.0f);
		World->GetTimerManager().SetTimer(
			EvalTimerHandle,
			this,
			&UMPNetLODComponent::EvaluateLOD,
			Interval,
			true
		);
	}
}

void UMPNetLODComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EvalTimerHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void UMPNetLODComponent::ForceEvaluateLOD()
{
	EvaluateLOD();
}

void UMPNetLODComponent::EvaluateLOD()
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector OwnerLocation = Owner->GetActorLocation();
	float MinDistanceSq = MAX_FLT;
	bool bFoundPlayer = false;

	// Find distance to closest player
	for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PC = Iterator->Get();
		if (PC && PC->GetPawn())
		{
			const float DistSq = FVector::DistSquared(OwnerLocation, PC->GetPawn()->GetActorLocation());
			if (DistSq < MinDistanceSq)
			{
				MinDistanceSq = DistSq;
				bFoundPlayer = true;
			}
		}
	}

	if (!bFoundPlayer)
	{
		// No players connected; throttle frequency
		Owner->SetNetUpdateFrequency(LODSettings.FarFrequencyHz);
		return;
	}

	const float Distance = FMath::Sqrt(MinDistanceSq);
	const EMPReplicationLODTier NewTier = LODSettings.EvaluateDistance(Distance);

	if (NewTier != CurrentLODTier)
	{
		const EMPReplicationLODTier OldTier = CurrentLODTier;
		CurrentLODTier = NewTier;

		// Apply network update frequency
		const float TargetFrequency = LODSettings.GetFrequencyForTier(NewTier);
		Owner->SetNetUpdateFrequency(TargetFrequency);

		// Handle dormancy transition
		if (NewTier == EMPReplicationLODTier::Dormant)
		{
			Owner->SetNetDormancy(DORM_DormantAll);
		}
		else if (OldTier == EMPReplicationLODTier::Dormant)
		{
			Owner->SetNetDormancy(DORM_Awake);
			Owner->FlushNetDormancy();
		}

		OnLODTierChanged.Broadcast(NewTier, OldTier);
	}
}

