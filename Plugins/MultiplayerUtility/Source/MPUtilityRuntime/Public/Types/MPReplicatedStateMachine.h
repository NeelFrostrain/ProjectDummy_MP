// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MPReplicatedStateMachine.generated.h"

DECLARE_DYNAMIC_DELEGATE_RetVal_ThreeParams(bool, FMPStateTransitionGuardDelegate, FGameplayTag, FromState, FGameplayTag, ToState, AActor*, ContextActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FMPOnStateChangedDelegate, FGameplayTag, NewState, FGameplayTag, PreviousState, double, ServerTimestamp);

/**
 * Server-authoritative, network-synchronized gameplay state machine structure.
 * Replicates current state, previous state, and transition timestamp.
 */
USTRUCT(BlueprintType)
struct MPUTILITYRUNTIME_API FMPReplicatedStateMachine
{
	GENERATED_BODY()

	/** Current active gameplay state tag. */
	UPROPERTY(BlueprintReadOnly, Category = "Networking|StateMachine")
	FGameplayTag CurrentState;

	/** State prior to the most recent transition. */
	UPROPERTY(BlueprintReadOnly, Category = "Networking|StateMachine")
	FGameplayTag PreviousState;

	/** Authoritative server time in seconds when current state was entered. */
	UPROPERTY(BlueprintReadOnly, Category = "Networking|StateMachine")
	double StateEnteredServerTime = 0.0;

	FMPReplicatedStateMachine() = default;

	/**
	 * Server call: Requests a transition to a new state.
	 * Returns true if the transition was permitted and executed.
	 */
	bool TransitionToState(FGameplayTag NewState, double CurrentServerTime, AActor* ContextActor = nullptr);

	/** Returns duration in seconds since current state was entered. */
	float GetTimeInCurrentState(double CurrentServerTime) const;

	/** Checks if currently in a specific state. */
	bool IsInState(FGameplayTag StateToCheck) const;

	/** Checks if currently in a child state of the given parent tag. */
	bool MatchesState(FGameplayTag ParentStateTag) const;
};

