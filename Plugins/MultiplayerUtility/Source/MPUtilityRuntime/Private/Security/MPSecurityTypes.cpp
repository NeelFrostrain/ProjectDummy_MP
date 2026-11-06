// Copyright Epic Games, Inc. All Rights Reserved.

#include "Security/MPSecurityTypes.h"

bool FMPFireRateValidator::ValidateShot(double CurrentServerTime, FString& OutReason)
{
	OutReason.Empty();

	// 1. Minimum interval check (accounting for 15ms clock jitter)
	const double JitterTolerance = 0.015;
	if (CurrentServerTime - LastShotServerTime < (MinShotInterval - JitterTolerance))
	{
		OutReason = FString::Printf(TEXT("Shot interval %.3fs was below minimum %.3fs."),
			CurrentServerTime - LastShotServerTime, MinShotInterval);
		return false;
	}

	// 2. Prune old timestamps outside sliding burst window
	RecentShotTimes.RemoveAll([CurrentServerTime, this](double Timestamp)
	{
		return (CurrentServerTime - Timestamp) > BurstWindowSeconds;
	});

	// 3. Check burst count
	if (RecentShotTimes.Num() >= MaxBurstCount)
	{
		OutReason = FString::Printf(TEXT("Burst limit exceeded: %d shots in %.2fs (Max: %d)."),
			RecentShotTimes.Num() + 1, BurstWindowSeconds, MaxBurstCount);
		return false;
	}

	RecentShotTimes.Add(CurrentServerTime);
	LastShotServerTime = CurrentServerTime;
	return true;
}

bool FMPMovementExploitDetector::ValidateStep(
	const FVector& NewLocation,
	double CurrentServerTime,
	bool& bOutTeleportDetected,
	float& OutSpeedError)
{
	bOutTeleportDetected = false;
	OutSpeedError = 0.0f;

	if (LastVerifiedTimestamp <= 0.0)
	{
		LastVerifiedLocation = NewLocation;
		LastVerifiedTimestamp = CurrentServerTime;
		return true;
	}

	const double DeltaTime = CurrentServerTime - LastVerifiedTimestamp;
	if (DeltaTime <= 0.0001)
	{
		return true; // Zero delta step
	}

	const float Distance = FVector::Dist(LastVerifiedLocation, NewLocation);

	// Teleport check
	if (Distance > TeleportThresholdDistance)
	{
		bOutTeleportDetected = true;
		return false;
	}

	// Speed check
	const float MaxExpectedDistance = (MaxAllowedSpeed * static_cast<float>(DeltaTime)) + ToleranceMargin;
	if (Distance > MaxExpectedDistance)
	{
		OutSpeedError = Distance - MaxExpectedDistance;
		return false;
	}

	LastVerifiedLocation = NewLocation;
	LastVerifiedTimestamp = CurrentServerTime;
	return true;
}

