#include "Types/MPReplicatedTimer.h"
#include "Wrappers/MPWorldSessionWrapper.h"

FMPReplicatedTimer::FMPReplicatedTimer()
    : ServerStartTime(0.0)
    , Duration(0.0f)
    , RemainingTimeAtPause(0.0f)
    , bIsRunning(false)
    , bIsPaused(false)
{
}

bool FMPReplicatedTimer::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
    Ar << ServerStartTime;
    Ar << Duration;
    Ar << RemainingTimeAtPause;

    uint8 PackedFlags = 0;
    if (Ar.IsSaving())
    {
        PackedFlags = (bIsRunning ? 1 : 0) | (bIsPaused ? 2 : 0);
    }

    Ar << PackedFlags;

    if (Ar.IsLoading())
    {
        bIsRunning = (PackedFlags & 1) != 0;
        bIsPaused = (PackedFlags & 2) != 0;
    }

    bOutSuccess = true;
    return true;
}

FMPReplicatedCooldown::FMPReplicatedCooldown()
    : ServerEndTime(0.0)
    , TotalDuration(0.0f)
{
}

bool FMPReplicatedCooldown::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
    Ar << ServerEndTime;
    Ar << TotalDuration;
    bOutSuccess = true;
    return true;
}

// =============================================================================
// LIBRARY IMPLEMENTATION
// =============================================================================

void UMPReplicatedTimerLibrary::StartTimer(FMPReplicatedTimer& Timer, float Duration, const UObject* WorldContextObject)
{
    const double CurrentTime = UMPWorldSessionWrapper::GetServerWorldTime(WorldContextObject);
    Timer.ServerStartTime = CurrentTime;
    Timer.Duration = FMath::Max(0.01f, Duration);
    Timer.RemainingTimeAtPause = 0.0f;
    Timer.bIsRunning = true;
    Timer.bIsPaused = false;
}

void UMPReplicatedTimerLibrary::PauseTimer(FMPReplicatedTimer& Timer, const UObject* WorldContextObject)
{
    if (!Timer.bIsRunning || Timer.bIsPaused)
    {
        return;
    }

    const float Remaining = GetRemainingTime(Timer, WorldContextObject);
    Timer.RemainingTimeAtPause = Remaining;
    Timer.bIsPaused = true;
}

void UMPReplicatedTimerLibrary::ResumeTimer(FMPReplicatedTimer& Timer, const UObject* WorldContextObject)
{
    if (!Timer.bIsRunning || !Timer.bIsPaused)
    {
        return;
    }

    const double CurrentTime = UMPWorldSessionWrapper::GetServerWorldTime(WorldContextObject);
    Timer.Duration = Timer.RemainingTimeAtPause;
    Timer.ServerStartTime = CurrentTime;
    Timer.RemainingTimeAtPause = 0.0f;
    Timer.bIsPaused = false;
}

void UMPReplicatedTimerLibrary::ResetTimer(FMPReplicatedTimer& Timer)
{
    Timer.ServerStartTime = 0.0;
    Timer.Duration = 0.0f;
    Timer.RemainingTimeAtPause = 0.0f;
    Timer.bIsRunning = false;
    Timer.bIsPaused = false;
}

float UMPReplicatedTimerLibrary::GetRemainingTime(const FMPReplicatedTimer& Timer, const UObject* WorldContextObject)
{
    if (!Timer.bIsRunning)
    {
        return 0.0f;
    }

    if (Timer.bIsPaused)
    {
        return Timer.RemainingTimeAtPause;
    }

    const double CurrentTime = UMPWorldSessionWrapper::GetServerWorldTime(WorldContextObject);
    const double Elapsed = CurrentTime - Timer.ServerStartTime;
    const double Remaining = static_cast<double>(Timer.Duration) - Elapsed;

    return FMath::Max(0.0f, static_cast<float>(Remaining));
}

float UMPReplicatedTimerLibrary::GetElapsedTime(const FMPReplicatedTimer& Timer, const UObject* WorldContextObject)
{
    if (!Timer.bIsRunning)
    {
        return 0.0f;
    }

    if (Timer.bIsPaused)
    {
        return FMath::Max(0.0f, Timer.Duration - Timer.RemainingTimeAtPause);
    }

    const double CurrentTime = UMPWorldSessionWrapper::GetServerWorldTime(WorldContextObject);
    const double Elapsed = CurrentTime - Timer.ServerStartTime;

    return FMath::Clamp(static_cast<float>(Elapsed), 0.0f, Timer.Duration);
}

float UMPReplicatedTimerLibrary::GetTimerProgress(const FMPReplicatedTimer& Timer, const UObject* WorldContextObject)
{
    if (!Timer.bIsRunning || Timer.Duration <= 0.0001f)
    {
        return 1.0f;
    }

    const float Elapsed = GetElapsedTime(Timer, WorldContextObject);
    return FMath::Clamp(Elapsed / Timer.Duration, 0.0f, 1.0f);
}

bool UMPReplicatedTimerLibrary::IsTimerRunning(const FMPReplicatedTimer& Timer, const UObject* WorldContextObject)
{
    if (!Timer.bIsRunning || Timer.bIsPaused)
    {
        return false;
    }

    return GetRemainingTime(Timer, WorldContextObject) > 0.0f;
}

bool UMPReplicatedTimerLibrary::IsTimerPaused(const FMPReplicatedTimer& Timer)
{
    return Timer.bIsRunning && Timer.bIsPaused;
}

bool UMPReplicatedTimerLibrary::IsTimerFinished(const FMPReplicatedTimer& Timer, const UObject* WorldContextObject)
{
    if (!Timer.bIsRunning)
    {
        return true;
    }

    if (Timer.bIsPaused)
    {
        return Timer.RemainingTimeAtPause <= 0.0f;
    }

    return GetRemainingTime(Timer, WorldContextObject) <= 0.0f;
}

void UMPReplicatedTimerLibrary::StartCooldown(FMPReplicatedCooldown& Cooldown, float Duration, const UObject* WorldContextObject)
{
    const double CurrentTime = UMPWorldSessionWrapper::GetServerWorldTime(WorldContextObject);
    Cooldown.TotalDuration = FMath::Max(0.01f, Duration);
    Cooldown.ServerEndTime = CurrentTime + static_cast<double>(Cooldown.TotalDuration);
}

bool UMPReplicatedTimerLibrary::IsOnCooldown(const FMPReplicatedCooldown& Cooldown, const UObject* WorldContextObject)
{
    const double CurrentTime = UMPWorldSessionWrapper::GetServerWorldTime(WorldContextObject);
    return CurrentTime < Cooldown.ServerEndTime;
}

float UMPReplicatedTimerLibrary::GetRemainingCooldown(const FMPReplicatedCooldown& Cooldown, const UObject* WorldContextObject)
{
    const double CurrentTime = UMPWorldSessionWrapper::GetServerWorldTime(WorldContextObject);
    const double Remaining = Cooldown.ServerEndTime - CurrentTime;
    return FMath::Max(0.0f, static_cast<float>(Remaining));
}

float UMPReplicatedTimerLibrary::GetCooldownProgress(const FMPReplicatedCooldown& Cooldown, const UObject* WorldContextObject)
{
    if (Cooldown.TotalDuration <= 0.0001f)
    {
        return 1.0f;
    }

    const float Remaining = GetRemainingCooldown(Cooldown, WorldContextObject);
    const float Elapsed = Cooldown.TotalDuration - Remaining;
    return FMath::Clamp(Elapsed / Cooldown.TotalDuration, 0.0f, 1.0f);
}

void UMPReplicatedTimerLibrary::ResetCooldown(FMPReplicatedCooldown& Cooldown)
{
    Cooldown.ServerEndTime = 0.0;
    Cooldown.TotalDuration = 0.0f;
}

