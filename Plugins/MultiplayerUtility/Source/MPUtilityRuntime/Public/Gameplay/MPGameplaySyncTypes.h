// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MPGameplaySyncTypes.generated.h"

/**
 * Replicated synchronization timer for gameplay mechanics (bomb plant, zone capture, ability channels, round timers).
 * Authoritative on server, evaluated statelessly on clients using synchronized world time.
 */
USTRUCT(BlueprintType)
struct MPUTILITYRUNTIME_API FMPReplicatedSyncTimer
{
	GENERATED_BODY()

	/** Authoritative server world timestamp when the timer was started (seconds). */
	UPROPERTY(BlueprintReadOnly, Category = "Multiplayer Utility|Synchronization")
	float StartServerTime = 0.0f;

	/** Total planned duration of the timer in seconds. */
	UPROPERTY(BlueprintReadOnly, Category = "Multiplayer Utility|Synchronization")
	float Duration = 0.0f;

	/** Whether the timer is currently running. */
	UPROPERTY(BlueprintReadOnly, Category = "Multiplayer Utility|Synchronization")
	bool bIsActive = false;

	/** Authoritative elapsed time recorded if paused. */
	UPROPERTY(BlueprintReadOnly, Category = "Multiplayer Utility|Synchronization")
	float PausedElapsedTime = 0.0f;

	/** Whether the timer is currently paused. */
	UPROPERTY(BlueprintReadOnly, Category = "Multiplayer Utility|Synchronization")
	bool bIsPaused = false;

	FMPReplicatedSyncTimer() = default;

	/** Starts the timer with a given duration and server start time (Server-only). */
	void Start(float InDuration, float CurrentServerTime);

	/** Stops and resets the timer (Server-only). */
	void Stop();

	/** Pauses the timer (Server-only). */
	void Pause(float CurrentServerTime);

	/** Resumes the paused timer (Server-only). */
	void Resume(float CurrentServerTime);

	/** Returns remaining seconds until expiration, clamped >= 0. */
	float GetRemainingTime(float CurrentServerTime) const;

	/** Returns elapsed seconds since timer started, clamped [0, Duration]. */
	float GetElapsedTime(float CurrentServerTime) const;

	/** Returns normalized progress from 0.0 (just started) to 1.0 (finished). */
	float GetNormalizedProgress(float CurrentServerTime) const;

	/** Returns true if the timer was started and has reached or exceeded its duration. */
	bool IsFinished(float CurrentServerTime) const;

	/** Returns true if the timer is actively running and not paused. */
	bool IsRunning() const { return bIsActive && !bIsPaused; }
};

