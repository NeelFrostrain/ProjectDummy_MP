// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/MPGameplaySyncTypes.h"
#include "Math/UnrealMathUtility.h"

void FMPReplicatedSyncTimer::Start(float InDuration, float CurrentServerTime)
{
	Duration = FMath::Max(0.0f, InDuration);
	StartServerTime = CurrentServerTime;
	PausedElapsedTime = 0.0f;
	bIsActive = (Duration > 0.0f);
	bIsPaused = false;
}

void FMPReplicatedSyncTimer::Stop()
{
	bIsActive = false;
	bIsPaused = false;
	StartServerTime = 0.0f;
	Duration = 0.0f;
	PausedElapsedTime = 0.0f;
}

void FMPReplicatedSyncTimer::Pause(float CurrentServerTime)
{
	if (bIsActive && !bIsPaused)
	{
		PausedElapsedTime = FMath::Clamp(CurrentServerTime - StartServerTime, 0.0f, Duration);
		bIsPaused = true;
	}
}

void FMPReplicatedSyncTimer::Resume(float CurrentServerTime)
{
	if (bIsActive && bIsPaused)
	{
		StartServerTime = CurrentServerTime - PausedElapsedTime;
		bIsPaused = false;
	}
}

float FMPReplicatedSyncTimer::GetRemainingTime(float CurrentServerTime) const
{
	if (!bIsActive || Duration <= 0.0f)
	{
		return 0.0f;
	}

	if (bIsPaused)
	{
		return FMath::Max(0.0f, Duration - PausedElapsedTime);
	}

	const float Elapsed = CurrentServerTime - StartServerTime;
	return FMath::Max(0.0f, Duration - Elapsed);
}

float FMPReplicatedSyncTimer::GetElapsedTime(float CurrentServerTime) const
{
	if (!bIsActive || Duration <= 0.0f)
	{
		return 0.0f;
	}

	if (bIsPaused)
	{
		return PausedElapsedTime;
	}

	return FMath::Clamp(CurrentServerTime - StartServerTime, 0.0f, Duration);
}

float FMPReplicatedSyncTimer::GetNormalizedProgress(float CurrentServerTime) const
{
	if (!bIsActive || Duration <= 0.0f)
	{
		return 1.0f;
	}

	return FMath::Clamp(GetElapsedTime(CurrentServerTime) / Duration, 0.0f, 1.0f);
}

bool FMPReplicatedSyncTimer::IsFinished(float CurrentServerTime) const
{
	if (!bIsActive)
	{
		return false;
	}

	return GetRemainingTime(CurrentServerTime) <= 0.0f;
}

