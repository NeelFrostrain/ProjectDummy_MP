#pragma once

#include "CoreMinimal.h"

/**
 * Shared testing helpers and deterministic simulated time driver.
 */
struct FMPTestClock
{
    double CurrentTime = 0.0;

    void Advance(double DeltaSeconds)
    {
        CurrentTime += FMath::Max(0.0, DeltaSeconds);
    }

    void Reset()
    {
        CurrentTime = 0.0;
    }
};

