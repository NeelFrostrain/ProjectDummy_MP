#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Math/RandomStream.h"
#include "MPReplicatedSeed.generated.h"

/**
 * Replicated random seed ensuring synchronized, deterministic procedural generation
 * across server and all connected clients without transmitting arrays of random values.
 */
USTRUCT(BlueprintType)
struct MPUTILITYRUNTIME_API FMPReplicatedSeed
{
    GENERATED_BODY()

    /** Primary deterministic seed value */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplayer Utility|Seed")
    int32 BaseSeed = 0;

    /** Monotonic sequence counter to guarantee identical progression on all peers */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplayer Utility|Seed")
    int32 SequenceStep = 0;

    FMPReplicatedSeed() = default;
    explicit FMPReplicatedSeed(int32 InSeed) : BaseSeed(InSeed), SequenceStep(0) {}

    bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
    {
        Ar << BaseSeed;
        Ar << SequenceStep;
        bOutSuccess = true;
        return true;
    }
};

template<>
struct TStructOpsTypeTraits<FMPReplicatedSeed> : public TStructOpsTypeTraitsBase2<FMPReplicatedSeed>
{
    enum
    {
        WithNetSerializer = true,
    };
};

/**
 * Blueprint library for deterministic random generation from an FMPReplicatedSeed.
 */
UCLASS()
class MPUTILITYRUNTIME_API UMPReplicatedSeedLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Initializes a replicated seed. If InSeed is 0, generates a non-zero cryptographically strong seed on server.
     * Machine: Server / Client / Both.
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Seed", meta = (DisplayName = "Make Replicated Seed", Keywords = "random seed deterministic procedural make"))
    static FMPReplicatedSeed MakeReplicatedSeed(int32 InSeed = 0);

    /**
     * Converts a replicated seed to an Unreal FRandomStream.
     * Machine: Server / Client / Both.
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Seed", meta = (DisplayName = "Seed To Random Stream", Keywords = "random stream seed"))
    static FRandomStream SeedToRandomStream(const FMPReplicatedSeed& InSeed);

    /**
     * Generates a deterministic float in range [Min, Max] and increments sequence step.
     * Machine: Server / Client / Both.
     */
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Utility|Seed", meta = (DisplayName = "Random Float From Seed", Keywords = "random float seed range"))
    static float RandomFloatFromSeed(UPARAM(ref) FMPReplicatedSeed& InSeed, float Min = 0.0f, float Max = 1.0f);

    /**
     * Generates a deterministic integer in range [Min, Max] and increments sequence step.
     * Machine: Server / Client / Both.
     */
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Utility|Seed", meta = (DisplayName = "Random Integer From Seed", Keywords = "random int seed range"))
    static int32 RandomIntFromSeed(UPARAM(ref) FMPReplicatedSeed& InSeed, int32 Min = 0, int32 Max = 100);

    /**
     * Generates a deterministic normalized unit vector (sphere direction) and increments sequence step.
     * Machine: Server / Client / Both.
     */
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Utility|Seed", meta = (DisplayName = "Random Unit Vector From Seed", Keywords = "random vector unit direction seed"))
    static FVector RandomUnitVectorFromSeed(UPARAM(ref) FMPReplicatedSeed& InSeed);

    /**
     * Resets sequence step to 0 with optional new base seed.
     * Machine: Server (Authority only).
     */
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Utility|Seed", meta = (DisplayName = "Reset Replicated Seed", Keywords = "seed reset clear"))
    static void ResetReplicatedSeed(UPARAM(ref) FMPReplicatedSeed& InSeed, int32 NewBaseSeed = 0);
};

