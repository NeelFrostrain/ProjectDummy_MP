#pragma once

#include "CoreMinimal.h"
#include "MPRewindBuffer.generated.h"

/**
 * A single recorded state snapshot in history.
 */
USTRUCT(BlueprintType)
struct MPUTILITYRUNTIME_API FMPTransformSnapshot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplayer Utility|Rewind")
    double Timestamp = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplayer Utility|Rewind")
    FTransform Transform = FTransform::Identity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplayer Utility|Rewind")
    FVector LinearVelocity = FVector::ZeroVector;

    FMPTransformSnapshot() = default;
    FMPTransformSnapshot(double InTime, const FTransform& InTransform, const FVector& InVelocity = FVector::ZeroVector)
        : Timestamp(InTime), Transform(InTransform), LinearVelocity(InVelocity) {}
};

/**
 * Pre-allocated circular rewind buffer for server-side hit registration,
 * lag compensation, and client-side prediction smoothing.
 * Eliminates heap allocation during gameplay.
 */
USTRUCT(BlueprintType)
struct MPUTILITYRUNTIME_API FMPRewindBuffer
{
    GENERATED_BODY()

    /** Maximum historical time window retained in seconds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplayer Utility|Rewind")
    float MaxHistorySeconds = 1.0f;

    /** Fixed circular capacity */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplayer Utility|Rewind")
    int32 MaxEntries = 128;

private:
    UPROPERTY()
    TArray<FMPTransformSnapshot> Buffer;

    int32 HeadIndex = 0;
    int32 ActiveCount = 0;

public:
    FMPRewindBuffer();
    FMPRewindBuffer(int32 InCapacity, float InMaxHistorySeconds = 1.0f);

    /** Records a new snapshot at the current time */
    void RecordSnapshot(double Timestamp, const FTransform& InTransform, const FVector& Velocity = FVector::ZeroVector);

    /**
     * Samples the historical transform at TargetTimestamp.
     * Uses spherical linear interpolation (SLERP) for rotation and linear interpolation for translation.
     * Supports small linear extrapolation if TargetTimestamp is slightly in the future.
     */
    bool GetInterpolatedTransform(double TargetTimestamp, FTransform& OutTransform, double MaxExtrapolationSeconds = 0.05) const;

    /** Resets and clears the buffer history */
    void Reset();

    /** Number of recorded snapshots currently retained */
    int32 GetCount() const { return ActiveCount; }

    /** Oldest recorded timestamp, or -1 if empty */
    double GetOldestTimestamp() const;

    /** Newest recorded timestamp, or -1 if empty */
    double GetNewestTimestamp() const;

private:
    int32 GetInternalIndex(int32 LogicalIndex) const;
};

