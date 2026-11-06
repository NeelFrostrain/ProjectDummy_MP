#include "Types/MPTimeMath.h"

void UMPTimeMath::CalculateNTPTimeOffset(
    double T1, double T2, double T3, double T4,
    double& OutRTT, double& OutClockOffset)
{
    // RTT = (T4 - T1) - (T3 - T2)
    OutRTT = FMath::Max(0.0, (T4 - T1) - (T3 - T2));

    // Clock Offset = ((T2 - T1) + (T3 - T4)) / 2
    OutClockOffset = ((T2 - T1) + (T3 - T4)) * 0.5;
}

float UMPTimeMath::SmoothPingEMA(float CurrentSmoothedPing, float NewSampleMs, float Alpha)
{
    const float ClampedAlpha = FMath::Clamp(Alpha, 0.01f, 1.0f);
    if (CurrentSmoothedPing <= 0.0f)
    {
        return NewSampleMs;
    }
    return (ClampedAlpha * NewSampleMs) + ((1.0f - ClampedAlpha) * CurrentSmoothedPing);
}

float UMPTimeMath::CalculateRFC3550Jitter(float PreviousPingMs, float CurrentPingMs, float CurrentJitter)
{
    // RFC 3550 standard algorithm: J = J + (|D| - J) / 16
    const float Diff = FMath::Abs(CurrentPingMs - PreviousPingMs);
    return CurrentJitter + (Diff - CurrentJitter) * (1.0f / 16.0f);
}

double UMPTimeMath::ComputeRewindTargetTime(double CurrentServerTime, double ClientPingSeconds, double InterpolationDelaySeconds)
{
    const double ClampedPing = FMath::Max(0.0, ClientPingSeconds);
    const double ClampedInterp = FMath::Max(0.0, InterpolationDelaySeconds);
    return CurrentServerTime - (ClampedPing + ClampedInterp);
}

bool UMPTimeMath::IsTimestampInValidWindow(double Timestamp, double CurrentServerTime, double MaxHistorySeconds, double MaxFutureToleranceSeconds)
{
    if (Timestamp > CurrentServerTime + FMath::Max(0.0, MaxFutureToleranceSeconds))
    {
        // In the future beyond tolerance threshold
        return false;
    }

    if (Timestamp < CurrentServerTime - FMath::Max(0.0, MaxHistorySeconds))
    {
        // Expired beyond rewind history limit
        return false;
    }

    return true;
}

bool UMPTimeMath::IsSequenceNewer16(int32 SequenceA, int32 SequenceB)
{
    const uint16 A = static_cast<uint16>(SequenceA & 0xFFFF);
    const uint16 B = static_cast<uint16>(SequenceB & 0xFFFF);

    return (A != B) && static_cast<uint16>(A - B) < 0x8000;
}

bool UMPTimeMath::IsTimestampNewer(float TimestampA, float TimestampB, float HalfRange)
{
    const float Diff = TimestampA - TimestampB;
    if (Diff > 0.0f)
    {
        return Diff < HalfRange;
    }
    return Diff < -HalfRange;
}

int32 UMPTimeMath::SequenceDistance16(int32 SequenceA, int32 SequenceB)
{
    const int16 Diff = static_cast<int16>(static_cast<uint16>(SequenceA) - static_cast<uint16>(SequenceB));
    return static_cast<int32>(Diff);
}

int64 UMPTimeMath::SequenceDistance32(int64 SequenceA, int64 SequenceB)
{
    const int32 Diff = static_cast<int32>(static_cast<uint32>(SequenceA) - static_cast<uint32>(SequenceB));
    return static_cast<int64>(Diff);
}

float UMPTimeMath::ClampDeltaTimeForNetwork(float DeltaTime, float MinDelta, float MaxDelta)
{
    if (DeltaTime <= 0.0f)
    {
        return MinDelta;
    }
    return FMath::Clamp(DeltaTime, MinDelta, MaxDelta);
}

float UMPTimeMath::LerpNetValue(float Current, float Target, float DeltaTime, float InterpolationSpeed)
{
    if (DeltaTime <= 0.0f || InterpolationSpeed <= 0.0f)
    {
        return Target;
    }
    const float Alpha = FMath::Clamp(DeltaTime * InterpolationSpeed, 0.0f, 1.0f);
    return FMath::Lerp(Current, Target, Alpha);
}

float UMPTimeMath::ExponentialSmoothFloat(float Current, float Target, float DeltaTime, float SmoothingSpeed)
{
    if (DeltaTime <= 0.0f || SmoothingSpeed <= 0.0f)
    {
        return Target;
    }
    const float Decay = FMath::Exp(-SmoothingSpeed * DeltaTime);
    return Target + (Current - Target) * Decay;
}

FVector UMPTimeMath::ExponentialSmoothVector(const FVector& Current, const FVector& Target, float DeltaTime, float SmoothingSpeed)
{
    if (DeltaTime <= 0.0f || SmoothingSpeed <= 0.0f)
    {
        return Target;
    }
    const float Decay = FMath::Exp(-SmoothingSpeed * DeltaTime);
    return Target + (Current - Target) * Decay;
}

