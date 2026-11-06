#pragma once

#include "CoreMinimal.h"
#include "MPRateLimiter.generated.h"

/**
 * Deterministic token-bucket rate limiter for RPCs and networked events.
 * Thread-safe for game thread use, requires zero dynamic allocations during runtime.
 */
USTRUCT(BlueprintType)
struct MPUTILITYRUNTIME_API FMPRateLimiter
{
    GENERATED_BODY()

    /** Maximum capacity of burst tokens in bucket */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplayer Utility|Rate Limiter")
    double Capacity = 10.0;

    /** Number of tokens added to bucket per second */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplayer Utility|Rate Limiter")
    double RefillRate = 5.0;

    /** Current available tokens in bucket */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Multiplayer Utility|Rate Limiter")
    double CurrentTokens = 10.0;

    /** Timestamp (seconds) of last refill operation */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Multiplayer Utility|Rate Limiter")
    double LastRefillTime = 0.0;

    FMPRateLimiter();
    FMPRateLimiter(double InCapacity, double InRefillRate, double InInitialTime = 0.0);

    /**
     * Attempts to consume requested tokens at given timestamp.
     * @param Tokens Number of tokens to consume (default 1)
     * @param CurrentTimeSeconds Monotonic or world timestamp in seconds
     * @return True if sufficient tokens existed and were consumed, false otherwise
     */
    bool Consume(double Tokens = 1.0, double CurrentTimeSeconds = 0.0);

    /**
     * Refills tokens up to current time without consuming.
     * @param CurrentTimeSeconds Timestamp in seconds
     */
    void Refill(double CurrentTimeSeconds);

    /**
     * Resets bucket to full capacity at specified timestamp.
     */
    void Reset(double CurrentTimeSeconds = 0.0);

    /**
     * Gets available tokens at current time without modifying state.
     */
    double GetAvailableTokens(double CurrentTimeSeconds) const;

    /**
     * Calculates seconds until specified number of tokens will be available.
     */
    double GetTimeUntilAvailable(double Tokens, double CurrentTimeSeconds) const;
};

