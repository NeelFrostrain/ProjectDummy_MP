// Copyright Epic Games, Inc. All Rights Reserved.

#include "Types/MPReplicatedCountdown.h"

void FMPReplicatedCountdownTimer::Start(float InDuration, double CurrentServerTime)
{
	Duration = FMath::Max(0.0f, InDuration);
	ServerStartTime = CurrentServerTime;
	bIsPaused = false;
	PausedRemainingTime = Duration;
}

void FMPReplicatedCountdownTimer::Pause(double CurrentServerTime)
{
	if (!bIsPaused && IsActive())
	{
		PausedRemainingTime = GetRemainingTime(CurrentServerTime);
		bIsPaused = true;
	}
}

void FMPReplicatedCountdownTimer::Resume(double CurrentServerTime)
{
	if (bIsPaused)
	{
		Duration = PausedRemainingTime;
		ServerStartTime = CurrentServerTime;
		bIsPaused = false;
	}
}

void FMPReplicatedCountdownTimer::Reset()
{
	ServerStartTime = 0.0;
	Duration = 0.0f;
	bIsPaused = false;
	PausedRemainingTime = 0.0f;
}

float FMPReplicatedCountdownTimer::GetRemainingTime(double CurrentServerTime) const
{
	if (!IsActive())
	{
		return 0.0f;
	}

	if (bIsPaused)
	{
		return PausedRemainingTime;
	}

	const double Elapsed = CurrentServerTime - ServerStartTime;
	return FMath::Max(0.0f, Duration - static_cast<float>(Elapsed));
}

float FMPReplicatedCountdownTimer::GetElapsedTime(double CurrentServerTime) const
{
	if (!IsActive())
	{
		return 0.0f;
	}

	return FMath::Clamp(Duration - GetRemainingTime(CurrentServerTime), 0.0f, Duration);
}

float FMPReplicatedCountdownTimer::GetNormalizedProgress(double CurrentServerTime) const
{
	if (Duration <= 0.0f || !IsActive())
	{
		return 1.0f;
	}

	return FMath::Clamp(GetElapsedTime(CurrentServerTime) / Duration, 0.0f, 1.0f);
}

bool FMPReplicatedCountdownTimer::IsFinished(double CurrentServerTime) const
{
	if (!IsActive())
	{
		return true;
	}
	return GetRemainingTime(CurrentServerTime) <= 0.0f;
}

bool FMPReplicatedCountdownTimer::IsActive() const
{
	return ServerStartTime > 0.0 && Duration > 0.0f;
}

