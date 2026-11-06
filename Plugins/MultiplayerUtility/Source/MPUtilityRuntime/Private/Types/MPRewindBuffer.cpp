#include "Types/MPRewindBuffer.h"

FMPRewindBuffer::FMPRewindBuffer()
    : MaxHistorySeconds(1.0f)
    , MaxEntries(128)
    , HeadIndex(0)
    , ActiveCount(0)
{
    Buffer.SetNum(MaxEntries);
}

FMPRewindBuffer::FMPRewindBuffer(int32 InCapacity, float InMaxHistorySeconds)
    : MaxHistorySeconds(FMath::Max(0.1f, InMaxHistorySeconds))
    , MaxEntries(FMath::Clamp(InCapacity, 8, 2048))
    , HeadIndex(0)
    , ActiveCount(0)
{
    Buffer.SetNum(MaxEntries);
}

void FMPRewindBuffer::RecordSnapshot(double Timestamp, const FTransform& InTransform, const FVector& Velocity)
{
    if (Buffer.Num() != MaxEntries)
    {
        Buffer.SetNum(MaxEntries);
        HeadIndex = 0;
        ActiveCount = 0;
    }

    if (ActiveCount > 0)
    {
        const int32 LatestIdx = (HeadIndex - 1 + MaxEntries) % MaxEntries;
        if (Timestamp <= Buffer[LatestIdx].Timestamp)
        {
            // Drop duplicate or strictly non-monotonic timestamps
            return;
        }
    }

    Buffer[HeadIndex] = FMPTransformSnapshot(Timestamp, InTransform, Velocity);
    HeadIndex = (HeadIndex + 1) % MaxEntries;
    if (ActiveCount < MaxEntries)
    {
        ++ActiveCount;
    }

    // Prune entries older than MaxHistorySeconds relative to the newest snapshot
    const double NewestTime = Timestamp;
    while (ActiveCount > 1)
    {
        const int32 OldestIdx = GetInternalIndex(0);
        if (NewestTime - Buffer[OldestIdx].Timestamp > MaxHistorySeconds)
        {
            --ActiveCount;
        }
        else
        {
            break;
        }
    }
}

int32 FMPRewindBuffer::GetInternalIndex(int32 LogicalIndex) const
{
    check(LogicalIndex >= 0 && LogicalIndex < ActiveCount);
    const int32 StartIdx = (HeadIndex - ActiveCount + MaxEntries) % MaxEntries;
    return (StartIdx + LogicalIndex) % MaxEntries;
}

double FMPRewindBuffer::GetOldestTimestamp() const
{
    if (ActiveCount == 0) return -1.0;
    return Buffer[GetInternalIndex(0)].Timestamp;
}

double FMPRewindBuffer::GetNewestTimestamp() const
{
    if (ActiveCount == 0) return -1.0;
    return Buffer[GetInternalIndex(ActiveCount - 1)].Timestamp;
}

void FMPRewindBuffer::Reset()
{
    HeadIndex = 0;
    ActiveCount = 0;
}

bool FMPRewindBuffer::GetInterpolatedTransform(double TargetTimestamp, FTransform& OutTransform, double MaxExtrapolationSeconds) const
{
    if (ActiveCount == 0)
    {
        return false;
    }

    const double OldestTime = GetOldestTimestamp();
    const double NewestTime = GetNewestTimestamp();

    // Past boundary check
    if (TargetTimestamp < OldestTime)
    {
        OutTransform = Buffer[GetInternalIndex(0)].Transform;
        return true;
    }

    // Future / extrapolation check
    if (TargetTimestamp >= NewestTime)
    {
        const FMPTransformSnapshot& Latest = Buffer[GetInternalIndex(ActiveCount - 1)];
        const double ExtrapolationDt = TargetTimestamp - NewestTime;

        if (ExtrapolationDt <= MaxExtrapolationSeconds)
        {
            OutTransform = Latest.Transform;
            OutTransform.AddToTranslation(Latest.LinearVelocity * static_cast<float>(ExtrapolationDt));
            return true;
        }
        else
        {
            OutTransform = Latest.Transform;
            return false;
        }
    }

    // Binary search between Oldest and Newest
    int32 Low = 0;
    int32 High = ActiveCount - 1;

    while (Low <= High)
    {
        const int32 Mid = Low + (High - Low) / 2;
        const double MidTime = Buffer[GetInternalIndex(Mid)].Timestamp;

        if (MidTime < TargetTimestamp)
        {
            Low = Mid + 1;
        }
        else
        {
            High = Mid - 1;
        }
    }

    // Low is index of first element >= TargetTimestamp
    const int32 IndexAfter = FMath::Clamp(Low, 1, ActiveCount - 1);
    const int32 IndexBefore = IndexAfter - 1;

    const FMPTransformSnapshot& A = Buffer[GetInternalIndex(IndexBefore)];
    const FMPTransformSnapshot& B = Buffer[GetInternalIndex(IndexAfter)];

    const double TimeDelta = B.Timestamp - A.Timestamp;
    if (TimeDelta <= 0.000001)
    {
        OutTransform = B.Transform;
        return true;
    }

    const float Alpha = static_cast<float>(FMath::Clamp((TargetTimestamp - A.Timestamp) / TimeDelta, 0.0, 1.0));

    // Spherical Linear Interpolation for Rotation, Linear Lerp for Translation & Scale
    const FQuat SlerpedQuat = FQuat::Slerp(A.Transform.GetRotation(), B.Transform.GetRotation(), Alpha);
    const FVector LerpedLocation = FMath::Lerp(A.Transform.GetLocation(), B.Transform.GetLocation(), Alpha);
    const FVector LerpedScale = FMath::Lerp(A.Transform.GetScale3D(), B.Transform.GetScale3D(), Alpha);

    OutTransform = FTransform(SlerpedQuat, LerpedLocation, LerpedScale);
    return true;
}

