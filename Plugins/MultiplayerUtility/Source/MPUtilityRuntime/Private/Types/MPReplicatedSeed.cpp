#include "Types/MPReplicatedSeed.h"

FMPReplicatedSeed UMPReplicatedSeedLibrary::MakeReplicatedSeed(int32 InSeed)
{
    if (InSeed == 0)
    {
        InSeed = FMath::RandRange(100000, 999999999);
    }
    return FMPReplicatedSeed(InSeed);
}

FRandomStream UMPReplicatedSeedLibrary::SeedToRandomStream(const FMPReplicatedSeed& InSeed)
{
    // Derive initial seed perturbed by sequence step
    const int32 EffectiveSeed = InSeed.BaseSeed ^ (InSeed.SequenceStep * 2654435761);
    return FRandomStream(EffectiveSeed);
}

float UMPReplicatedSeedLibrary::RandomFloatFromSeed(FMPReplicatedSeed& InSeed, float Min, float Max)
{
    FRandomStream Stream = SeedToRandomStream(InSeed);
    InSeed.SequenceStep++;
    return Stream.FRandRange(Min, Max);
}

int32 UMPReplicatedSeedLibrary::RandomIntFromSeed(FMPReplicatedSeed& InSeed, int32 Min, int32 Max)
{
    FRandomStream Stream = SeedToRandomStream(InSeed);
    InSeed.SequenceStep++;
    return Stream.RandRange(Min, Max);
}

FVector UMPReplicatedSeedLibrary::RandomUnitVectorFromSeed(FMPReplicatedSeed& InSeed)
{
    FRandomStream Stream = SeedToRandomStream(InSeed);
    InSeed.SequenceStep++;
    return Stream.VRand();
}

void UMPReplicatedSeedLibrary::ResetReplicatedSeed(FMPReplicatedSeed& InSeed, int32 NewBaseSeed)
{
    if (NewBaseSeed != 0)
    {
        InSeed.BaseSeed = NewBaseSeed;
    }
    InSeed.SequenceStep = 0;
}

