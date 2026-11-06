#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MPTimeMath.generated.h"

/**
 * Pure mathematical utilities for network time synchronization,
 * RTT estimation, jitter calculation, and lag compensation offsets.
 */
UCLASS()
class MPUTILITYRUNTIME_API UMPTimeMath : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Calculates Network Time Protocol (NTP) round-trip time and clock offset from 4 timestamps.
     * @param T1 Client transmit timestamp
     * @param T2 Server receive timestamp
     * @param T3 Server transmit timestamp
     * @param T4 Client receive timestamp
     * @param OutRTT Calculated round-trip time in seconds
     * @param OutClockOffset Clock offset to add to client time to produce estimated server time
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Time Math")
    static void CalculateNTPTimeOffset(
        double T1, double T2, double T3, double T4,
        double& OutRTT, double& OutClockOffset);

    /**
     * Exponential Moving Average (EMA) ping smoothing filter.
     * @param CurrentSmoothedPing Current smoothed ping value in milliseconds
     * @param NewSampleMs Raw ping sample in milliseconds
     * @param Alpha Smoothing weight [0.01 - 1.0], default 0.15
     * @return Updated smoothed ping in milliseconds
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Time Math")
    static float SmoothPingEMA(float CurrentSmoothedPing, float NewSampleMs, float Alpha = 0.15f);

    /**
     * RFC 3550 compliant packet jitter calculation.
     * @param PreviousPingMs Prior ping sample in milliseconds
     * @param CurrentPingMs Current ping sample in milliseconds
     * @param CurrentJitter Current running jitter estimate in milliseconds
     * @return Updated jitter in milliseconds
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Time Math")
    static float CalculateRFC3550Jitter(float PreviousPingMs, float CurrentPingMs, float CurrentJitter);

    /**
     * Computes the server-side rewind target time for client projectile/hitscan validation.
     * @param CurrentServerTime Current World Time on Server
     * @param ClientPingSeconds One-way ping or RTT / 2.0 in seconds
     * @param InterpolationDelaySeconds Client interpolation delay (default 0.1s / 100ms)
     * @return Target historical timestamp to rewind actors to
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Time Math")
    static double ComputeRewindTargetTime(double CurrentServerTime, double ClientPingSeconds, double InterpolationDelaySeconds = 0.1);

    /**
     * Validates whether a client timestamp falls within an acceptable historical window.
     * Rejects packets from too far in the past or impossibly far in the future.
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Time Math")
    static bool IsTimestampInValidWindow(double Timestamp, double CurrentServerTime, double MaxHistorySeconds = 1.0, double MaxFutureToleranceSeconds = 0.1);

    /**
     * Wraparound-safe comparison for 16-bit sequence numbers.
     * Returns true if SequenceA is newer than SequenceB.
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Time Math", meta=(DisplayName="Is Sequence Newer (16-bit)", Keywords="sequence newer 16 bit wraparound order"))
    static bool IsSequenceNewer16(int32 SequenceA, int32 SequenceB);

    /**
     * Wraparound-safe comparison for circular network timestamps.
     * Runs on: Both (Server and Client).
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Time Math", meta=(DisplayName="Is Timestamp Newer", Keywords="timestamp newer wraparound safe circular time"))
    static bool IsTimestampNewer(float TimestampA, float TimestampB, float HalfRange = 1800.0f);

    /**
     * Computes the signed distance from SequenceB to SequenceA in 16-bit sequence space [-32768, 32767].
     * Runs on: Both (Server and Client).
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Time Math", meta=(DisplayName="Sequence Distance (16-bit)", Keywords="sequence distance 16 delta diff wraparound"))
    static int32 SequenceDistance16(int32 SequenceA, int32 SequenceB);

    /**
     * Computes the signed distance from SequenceB to SequenceA in 32-bit sequence space.
     * Runs on: Both (Server and Client).
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Time Math", meta=(DisplayName="Sequence Distance (32-bit)", Keywords="sequence distance 32 delta diff wraparound"))
    static int64 SequenceDistance32(int64 SequenceA, int64 SequenceB);

    /**
     * Clamps DeltaTime to prevent physics explosions and extrapolation overshoot during network hitches.
     * Runs on: Both (Server and Client).
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Time Math", meta=(DisplayName="Clamp Delta Time For Network", Keywords="clamp delta time network hitch spike"))
    static float ClampDeltaTimeForNetwork(float DeltaTime, float MinDelta = 0.001f, float MaxDelta = 0.2f);

    /**
     * Rate-limited linear interpolation for smoothly updating networked properties over time.
     * Runs on: Both (Server and Client).
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Time Math", meta=(DisplayName="Lerp Net Value", Keywords="lerp net value rate limited interpolation smooth"))
    static float LerpNetValue(float Current, float Target, float DeltaTime, float InterpolationSpeed);

    /**
     * Frame-rate-independent exponential smoothing filter for float values: Target + (Current - Target) * exp(-Speed * DeltaTime).
     * Runs on: Both (Server and Client).
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Time Math", meta=(DisplayName="Exponential Smooth Float", Keywords="exponential smooth float interpolation framerate independent"))
    static float ExponentialSmoothFloat(float Current, float Target, float DeltaTime, float SmoothingSpeed);

    /**
     * Frame-rate-independent exponential smoothing filter for 3D vectors.
     * Runs on: Both (Server and Client).
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Time Math", meta=(DisplayName="Exponential Smooth Vector", Keywords="exponential smooth vector interpolation framerate independent"))
    static FVector ExponentialSmoothVector(const FVector& Current, const FVector& Target, float DeltaTime, float SmoothingSpeed);
};

