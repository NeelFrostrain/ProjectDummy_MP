// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MPReplicatedCountdown.generated.h"

/**
 * Replicated authoritative countdown timer.
 * Synchronizes round timers, bomb defusal, revive timers, and match clocks
 * without drifting across varying client latencies.
 */
USTRUCT(BlueprintType)
struct MPUTILITYRUNTIME_API FMPReplicatedCountdownTimer
{
	GENERATED_BODY()

	/** Authoritative server time in seconds when timer started. */
	UPROPERTY(BlueprintReadOnly, Category = "Networking|Timer")
	double ServerStartTime = 0.0;

	/** Total timer duration in seconds. */
	UPROPERTY(BlueprintReadOnly, Category = "Networking|Timer")
	float Duration = 0.0f;

	/** Whether the timer is currently paused. */
	UPROPERTY(BlueprintReadOnly, Category = "Networking|Timer")
	bool bIsPaused = false;

	/** Remaining time frozen when paused. */
	UPROPERTY(BlueprintReadOnly, Category = "Networking|Timer")
	float PausedRemainingTime = 0.0f;

	FMPReplicatedCountdownTimer() = default;

	/** Starts or restarts the timer on the server. */
	void Start(float InDuration, double CurrentServerTime);

	/** Pauses the timer on the server. */
	void Pause(double CurrentServerTime);

	/** Resumes a paused timer on the server. */
	void Resume(double CurrentServerTime);

	/** Stops and resets the timer. */
	void Reset();

	/** Returns remaining time in seconds (clamped to 0.0). */
	float GetRemainingTime(double CurrentServerTime) const;

	/** Returns elapsed time in seconds. */
	float GetElapsedTime(double CurrentServerTime) const;

	/** Returns normalized progress from 0.0 (just started) to 1.0 (finished). */
	float GetNormalizedProgress(double CurrentServerTime) const;

	/** Returns true if the timer has reached zero. */
	bool IsFinished(double CurrentServerTime) const;

	/** Returns true if the timer is actively counting down. */
	bool IsActive() const;
};

