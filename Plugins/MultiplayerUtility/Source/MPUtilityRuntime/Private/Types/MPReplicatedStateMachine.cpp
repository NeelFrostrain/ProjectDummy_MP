// Copyright Epic Games, Inc. All Rights Reserved.

#include "Types/MPReplicatedStateMachine.h"

bool FMPReplicatedStateMachine::TransitionToState(FGameplayTag NewState, double CurrentServerTime, AActor* ContextActor)
{
	if (!NewState.IsValid())
	{
		return false;
	}

	if (CurrentState == NewState)
	{
		return false; // Already in this state
	}

	PreviousState = CurrentState;
	CurrentState = NewState;
	StateEnteredServerTime = CurrentServerTime;
	return true;
}

float FMPReplicatedStateMachine::GetTimeInCurrentState(double CurrentServerTime) const
{
	if (StateEnteredServerTime <= 0.0 || CurrentServerTime < StateEnteredServerTime)
	{
		return 0.0f;
	}
	return static_cast<float>(CurrentServerTime - StateEnteredServerTime);
}

bool FMPReplicatedStateMachine::IsInState(FGameplayTag StateToCheck) const
{
	return CurrentState.MatchesTagExact(StateToCheck);
}

bool FMPReplicatedStateMachine::MatchesState(FGameplayTag ParentStateTag) const
{
	return CurrentState.MatchesTag(ParentStateTag);
}

