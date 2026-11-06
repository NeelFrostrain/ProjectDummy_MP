#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MPReplicatedTimer.generated.h"

/**
 * Replicated countdown timer using authoritative server timestamp and duration.
 * Clients compute exact remaining and elapsed time locally from synchronized server world time.
 * Supports pause, resume, and custom NetSerialize to save network bandwidth.
 */
USTRUCT(BlueprintType)
struct MPUTILITYRUNTIME_API FMPReplicatedTimer
{
    GENERATED_BODY()

    /** Synchronized server world timestamp when timer started */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplayer Utility|Timer")
    double ServerStartTime = 0.0;

    /** Total duration in seconds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplayer Utility|Timer")
    float Duration = 0.0f;

    /** Remaining time preserved when paused */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplayer Utility|Timer")
    float RemainingTimeAtPause = 0.0f;

    /** State flags packed into single byte */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplayer Utility|Timer")
    uint8 bIsRunning : 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplayer Utility|Timer")
    uint8 bIsPaused : 1;

    FMPReplicatedTimer();

    bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FMPReplicatedTimer> : public TStructOpsTypeTraitsBase2<FMPReplicatedTimer>
{
    enum
    {
        WithNetSerializer = true,
    };
};

/**
 * Replicated cooldown tracker for abilities, weapons, interactions, or inventory items.
 * Transmits server expiration timestamp, allowing clients to independently compute remaining cooldown.
 */
USTRUCT(BlueprintType)
struct MPUTILITYRUNTIME_API FMPReplicatedCooldown
{
    GENERATED_BODY()

    /** Authoritative server timestamp when cooldown expires */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplayer Utility|Cooldown")
    double ServerEndTime = 0.0;

    /** Total duration of cooldown in seconds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplayer Utility|Cooldown")
    float TotalDuration = 0.0f;

    FMPReplicatedCooldown();

    bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FMPReplicatedCooldown> : public TStructOpsTypeTraitsBase2<FMPReplicatedCooldown>
{
    enum
    {
        WithNetSerializer = true,
    };
};

/**
 * Blueprint function library for interacting with FMPReplicatedTimer and FMPReplicatedCooldown.
 */
UCLASS()
class MPUTILITYRUNTIME_API UMPReplicatedTimerLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // =========================================================================
    // REPLICATED TIMER NODES
    // =========================================================================

    /**
     * Starts or restarts a replicated timer with specified duration.
     * Machine: Server (Authority only).
     */
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Utility|Timer", meta = (WorldContext = "WorldContextObject", DisplayName = "Start Replicated Timer", Keywords = "timer start countdown clock"))
    static void StartTimer(UPARAM(ref) FMPReplicatedTimer& Timer, float Duration, const UObject* WorldContextObject);

    /**
     * Pauses a running timer, preserving remaining time.
     * Machine: Server (Authority only).
     */
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Utility|Timer", meta = (WorldContext = "WorldContextObject", DisplayName = "Pause Replicated Timer", Keywords = "timer pause stop freeze"))
    static void PauseTimer(UPARAM(ref) FMPReplicatedTimer& Timer, const UObject* WorldContextObject);

    /**
     * Resumes a paused timer with its preserved remaining duration.
     * Machine: Server (Authority only).
     */
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Utility|Timer", meta = (WorldContext = "WorldContextObject", DisplayName = "Resume Replicated Timer", Keywords = "timer resume continue unpause"))
    static void ResumeTimer(UPARAM(ref) FMPReplicatedTimer& Timer, const UObject* WorldContextObject);

    /**
     * Resets and stops the timer.
     * Machine: Server (Authority only).
     */
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Utility|Timer", meta = (DisplayName = "Reset Replicated Timer", Keywords = "timer reset clear stop"))
    static void ResetTimer(UPARAM(ref) FMPReplicatedTimer& Timer);

    /**
     * Computes seconds remaining on the timer using current synchronized server time.
     * Machine: Server / Client / Both.
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Timer", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Remaining Time", Keywords = "timer remaining seconds time left"))
    static float GetRemainingTime(const FMPReplicatedTimer& Timer, const UObject* WorldContextObject);

    /**
     * Computes seconds elapsed since timer started.
     * Machine: Server / Client / Both.
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Timer", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Elapsed Time", Keywords = "timer elapsed seconds time passed"))
    static float GetElapsedTime(const FMPReplicatedTimer& Timer, const UObject* WorldContextObject);

    /**
     * Returns timer completion progress normalized between 0.0 (just started) and 1.0 (finished).
     * Machine: Server / Client / Both.
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Timer", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Timer Progress (0-1)", Keywords = "timer progress percent normalized ratio"))
    static float GetTimerProgress(const FMPReplicatedTimer& Timer, const UObject* WorldContextObject);

    /**
     * Checks if the timer is actively running (not paused and not finished).
     * Machine: Server / Client / Both.
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Timer", meta = (WorldContext = "WorldContextObject", DisplayName = "Is Timer Running", Keywords = "timer is running active"))
    static bool IsTimerRunning(const FMPReplicatedTimer& Timer, const UObject* WorldContextObject);

    /**
     * Checks if the timer is currently paused.
     * Machine: Server / Client / Both.
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Timer", meta = (DisplayName = "Is Timer Paused", Keywords = "timer is paused frozen"))
    static bool IsTimerPaused(const FMPReplicatedTimer& Timer);

    /**
     * Returns true if duration has elapsed or timer is reset.
     * Machine: Server / Client / Both.
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Timer", meta = (WorldContext = "WorldContextObject", DisplayName = "Is Timer Finished", Keywords = "timer is finished completed expired"))
    static bool IsTimerFinished(const FMPReplicatedTimer& Timer, const UObject* WorldContextObject);

    // =========================================================================
    // REPLICATED COOLDOWN NODES
    // =========================================================================

    /**
     * Starts a cooldown with specified duration.
     * Machine: Server (Authority only).
     */
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Utility|Cooldown", meta = (WorldContext = "WorldContextObject", DisplayName = "Start Replicated Cooldown", Keywords = "cooldown start trigger ability item"))
    static void StartCooldown(UPARAM(ref) FMPReplicatedCooldown& Cooldown, float Duration, const UObject* WorldContextObject);

    /**
     * Checks whether the cooldown is currently active.
     * Machine: Server / Client / Both.
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Cooldown", meta = (WorldContext = "WorldContextObject", DisplayName = "Is On Cooldown", Keywords = "cooldown is active ready"))
    static bool IsOnCooldown(const FMPReplicatedCooldown& Cooldown, const UObject* WorldContextObject);

    /**
     * Gets seconds remaining until cooldown finishes (returns 0.0 if ready).
     * Machine: Server / Client / Both.
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Cooldown", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Remaining Cooldown", Keywords = "cooldown remaining seconds time left"))
    static float GetRemainingCooldown(const FMPReplicatedCooldown& Cooldown, const UObject* WorldContextObject);

    /**
     * Returns cooldown completion progress normalized between 0.0 (just started) and 1.0 (ready).
     * Machine: Server / Client / Both.
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Cooldown", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Cooldown Progress (0-1)", Keywords = "cooldown progress percent normalized"))
    static float GetCooldownProgress(const FMPReplicatedCooldown& Cooldown, const UObject* WorldContextObject);

    /**
     * Clears and resets cooldown immediately.
     * Machine: Server (Authority only).
     */
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Utility|Cooldown", meta = (DisplayName = "Reset Replicated Cooldown", Keywords = "cooldown reset clear ready"))
    static void ResetCooldown(UPARAM(ref) FMPReplicatedCooldown& Cooldown);
};

