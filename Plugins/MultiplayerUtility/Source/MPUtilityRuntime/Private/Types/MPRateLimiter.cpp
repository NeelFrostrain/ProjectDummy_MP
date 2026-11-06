#include "Types/MPRateLimiter.h"

FMPRateLimiter::FMPRateLimiter()
    : Capacity(10.0)
    , RefillRate(5.0)
    , CurrentTokens(10.0)
    , LastRefillTime(0.0)
{
}

FMPRateLimiter::FMPRateLimiter(double InCapacity, double InRefillRate, double InInitialTime)
    : Capacity(FMath::Max(1.0, InCapacity))
    , RefillRate(FMath::Max(0.01, InRefillRate))
    , CurrentTokens(Capacity)
    , LastRefillTime(InInitialTime)
{
}

void FMPRateLimiter::Refill(double CurrentTimeSeconds)
{
    if (CurrentTimeSeconds <= LastRefillTime)
    {
        return;
    }

    const double DeltaTime = CurrentTimeSeconds - LastRefillTime;
    const double TokensToAdd = DeltaTime * RefillRate;
    CurrentTokens = FMath::Min(Capacity, CurrentTokens + TokensToAdd);
    LastRefillTime = CurrentTimeSeconds;
}

bool FMPRateLimiter::Consume(double Tokens, double CurrentTimeSeconds)
{
    if (Tokens <= 0.0)
    {
        return true;
    }

    Refill(CurrentTimeSeconds);

    if (CurrentTokens >= Tokens)
    {
        CurrentTokens -= Tokens;
        return true;
    }

    return false;
}

void FMPRateLimiter::Reset(double CurrentTimeSeconds)
{
    CurrentTokens = Capacity;
    LastRefillTime = CurrentTimeSeconds;
}

double FMPRateLimiter::GetAvailableTokens(double CurrentTimeSeconds) const
{
    if (CurrentTimeSeconds <= LastRefillTime)
    {
        return CurrentTokens;
    }

    const double DeltaTime = CurrentTimeSeconds - LastRefillTime;
    const double TokensToAdd = DeltaTime * RefillRate;
    return FMath::Min(Capacity, CurrentTokens + TokensToAdd);
}

double FMPRateLimiter::GetTimeUntilAvailable(double Tokens, double CurrentTimeSeconds) const
{
    const double Available = GetAvailableTokens(CurrentTimeSeconds);
    if (Available >= Tokens)
    {
        return 0.0;
    }

    if (RefillRate <= 0.0001)
    {
        return 999999.0;
    }

    const double Needed = Tokens - Available;
    return Needed / RefillRate;
}

