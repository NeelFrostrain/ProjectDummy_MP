#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/LatentActionManager.h"
#include "LatentActions.h"
#include "MPLatentActions.generated.h"

UENUM(BlueprintType)
enum class EMPDelayReplicationResult : uint8
{
    Success UMETA(DisplayName="Replicated (Valid)"),
    TimedOut UMETA(DisplayName="Timed Out")
};

class AActor;

UCLASS()
class MPUTILITYRUNTIME_API UMPLatentActions : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Latent node that pauses execution until the target actor reference becomes valid
     * (replicated from server to client) or until TimeoutSeconds elapses.
     * Automatically defaults TargetActor to 'Self' in Actor blueprints.
     */
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Utility|Flow Control", meta = (Latent, LatentInfo = "LatentInfo", WorldContext = "WorldContextObject", DefaultToSelf = "TargetActor", ExpandEnumAsExecs = "Result", DisplayName = "Delay Until Replicated"))
    static void DelayUntilReplicated(
        const UObject* WorldContextObject,
        const AActor* TargetActor,
        float TimeoutSeconds,
        EMPDelayReplicationResult& Result,
        FLatentActionInfo LatentInfo);

    /**
     * Latent node that delays execution for a duration based on synchronized world server time.
     * Runs on: Both (Server and Client).
     */
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Utility|Flow Control", meta = (Latent, LatentInfo = "LatentInfo", WorldContext = "WorldContextObject", DisplayName = "Wait Server Seconds", Keywords="wait server seconds delay synchronized timer"))
    static void WaitServerSeconds(
        const UObject* WorldContextObject,
        float DurationSeconds,
        FLatentActionInfo LatentInfo);
};

