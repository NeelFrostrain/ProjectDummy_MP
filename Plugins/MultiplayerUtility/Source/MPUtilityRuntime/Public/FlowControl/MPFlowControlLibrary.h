#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Types/MPNetTypes.h"
#include "MPFlowControlLibrary.generated.h"

class AActor;
class APawn;
class APlayerController;

UCLASS()
class MPUTILITYRUNTIME_API UMPFlowControlLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Splits execution path by the actor's local network role.
     * Authority: Server logic (Damage, inventory, spawning).
     * Autonomous Proxy: Locally controlled player (HUD, camera shake, input prediction).
     * Simulated Proxy: Remote player puppet (Third-person cosmetic VFX, footsteps).
     */
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Utility|Flow Control", meta = (DefaultToSelf = "TargetActor", ExpandEnumAsExecs = "Branches", DisplayName = "Branch By Net Role", Keywords="network role switch branch authority autonomous simulated"))
    static void BranchByNetRole(const AActor* TargetActor, EMPNetRoleBranch& Branches);

    /**
     * Splits execution path by current world net mode (Dedicated, Listen, Client, Standalone).
     */
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Utility|Flow Control", meta = (WorldContext = "WorldContextObject", ExpandEnumAsExecs = "Branches", DisplayName = "Branch By Net Mode", Keywords="network mode switch branch dedicated listen client standalone"))
    static void BranchByNetMode(const UObject* WorldContextObject, EMPNetModeBranch& Branches);

    /**
     * Splits execution based on whether the Pawn or Controller is driven by local player input on this machine.
     */
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Utility|Flow Control", meta = (DefaultToSelf = "TargetActor", ExpandEnumAsExecs = "Branches", DisplayName = "Branch Is Locally Controlled", Keywords="locally controlled branch autonomous owner"))
    static void BranchIsLocallyControlled(const AActor* TargetActor, EMPLocalControlBranch& Branches);

    /**
     * Splits execution based on whether the actor has network authority (Server vs Client).
     */
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Utility|Flow Control", meta = (DefaultToSelf = "TargetActor", ExpandEnumAsExecs = "Branches", DisplayName = "Branch Has Authority", Keywords="has authority server client branch"))
    static void BranchHasAuthority(const AActor* TargetActor, EMPAuthorityBranch& Branches);

    /**
     * Network gate allowing execution only on specified network environments.
     */
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Utility|Flow Control", meta = (WorldContext = "WorldContextObject", ExpandEnumAsExecs = "Branches", DisplayName = "Network Gate", Keywords="network gate filter allow server client standalone"))
    static void NetworkGate(const UObject* WorldContextObject, bool bAllowServer, bool bAllowClient, bool bAllowStandalone, EMPGateExec& Branches);

    /**
     * Schedules a dynamic delegate callback to run on the very next engine frame.
     * Overwrites any prior pending action sharing the same ActionKey.
     * Runs on: Both (Server and Client).
     * 
     * @param WorldContextObject World context.
     * @param ActionKey Unique key for this scheduled action.
     * @param Callback The Blueprint delegate to execute next tick.
     * @return Result status (Scheduled or FailedInvalidWorld).
     */
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Utility|Flow Control", meta = (WorldContext = "WorldContextObject", DisplayName = "Run On Next Frame", Keywords="run on next frame tick delay defer timer schedule"))
    static EMPDelayedActionResult RunOnNextFrame(const UObject* WorldContextObject, FName ActionKey, FMPTimerCallback Callback);

    /**
     * Schedules a dynamic delegate callback to execute after a specified delay in seconds.
     * Respects server time / world dilation and overwrites any existing pending action with the same key.
     * Runs on: Both (Server and Client).
     * 
     * @param WorldContextObject World context.
     * @param ActionKey Unique key identifying this scheduled action.
     * @param DelaySeconds Delay time in seconds before execution.
     * @param Callback The Blueprint delegate to execute upon delay completion.
     * @return Result status (Scheduled or FailedInvalidWorld).
     */
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Utility|Flow Control", meta = (WorldContext = "WorldContextObject", DisplayName = "Run After Server Seconds", Keywords="run after server seconds delay timer schedule defer"))
    static EMPDelayedActionResult RunAfterServerSeconds(const UObject* WorldContextObject, FName ActionKey, float DelaySeconds, FMPTimerCallback Callback);

    /**
     * Cancels any pending delayed or next-frame action associated with the specified ActionKey.
     * Runs on: Both (Server and Client).
     * 
     * @param WorldContextObject World context.
     * @param ActionKey The key of the pending action to cancel.
     * @return True if a pending action was found and cancelled; false otherwise.
     */
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Utility|Flow Control", meta = (WorldContext = "WorldContextObject", DisplayName = "Cancel Pending By Key", Keywords="cancel pending action key timer abort stop"))
    static bool CancelPendingByKey(const UObject* WorldContextObject, FName ActionKey);

    /**
     * Schedules a dynamic delegate callback to execute after a delay, guaranteed to execute only on the server with authority.
     * Runs on: Server Only.
     * 
     * @param WorldContextObject World context.
     * @param ActionKey Unique key for this scheduled action.
     * @param DelaySeconds Delay time in seconds before execution.
     * @param Callback The delegate to execute on the server.
     * @return Result status.
     */
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Utility|Flow Control", meta = (WorldContext = "WorldContextObject", DisplayName = "Do On Server Only After Delay", Keywords="do on server only after delay schedule timer"))
    static EMPDelayedActionResult DoOnServerOnlyAfterDelay(const UObject* WorldContextObject, FName ActionKey, float DelaySeconds, FMPTimerCallback Callback);

    /**
     * Executes a callback only once per player controller for the given ActionKey.
     * Ensures idempotent actions (e.g., initial intro messages, first-time tutorial hints, one-shot gifts).
     * Runs on: Both (Server and Client).
     * 
     * @param PC Target player controller.
     * @param ActionKey Unique action key.
     * @param Callback Delegate to execute if key has not been processed for this player.
     * @return True if callback was executed; false if already processed previously.
     */
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Utility|Flow Control", meta = (DisplayName = "Call Once Per Player", Keywords="call once per player key filter"))
    static bool CallOncePerPlayer(APlayerController* PC, FName ActionKey, FMPTimerCallback Callback);
};
