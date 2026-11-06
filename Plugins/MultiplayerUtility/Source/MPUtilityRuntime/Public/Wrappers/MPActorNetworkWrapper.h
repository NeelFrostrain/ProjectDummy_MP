#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/EngineTypes.h"
#include "MPActorNetworkWrapper.generated.h"

class AActor;
class UNetConnection;
class APlayerController;
class APlayerState;

/**
 * ============================================================================
 * CONDITIONAL REPLICATION CHEAT-SHEET (For C++ GetLifetimeReplicatedProps)
 * ============================================================================
 * Use: DOREPLIFETIME_CONDITION(AYourActorClass, YourVariableName, COND_X);
 * 
 * COND_None                -> Replicates to all replicating connections (Default).
 * COND_InitialOnly         -> Replicates once on actor spawn, never updates again (Great for seeds, initial stats).
 * COND_OwnerOnly           -> Replicates ONLY to the connection that owns this actor (Inventory, private stats, quest log).
 * COND_SkipOwner           -> Replicates to everyone EXCEPT the owner (Third-person animations, remote cosmetic effects).
 * COND_SimulatedOnly       -> Replicates only to simulated proxies (Puppets on remote clients).
 * COND_AutonomousOnly      -> Replicates only to the autonomous proxy (Locally controlled client player).
 * COND_SimulatedOrPhysics  -> Replicates to simulated proxies or actors actively simulating physics.
 * COND_InitialOrOwner      -> Replicates initial spawn value to everyone, but subsequent updates ONLY to owner.
 * COND_Custom              -> Evaluated dynamically via PreReplication() and DOREPLIFETIME_ACTIVE_OVERRIDE.
 * COND_ReplayOrOwner       -> Replicates to owner connection or active demo/replay recorder.
 * ============================================================================
 */

UCLASS()
class MPUTILITYRUNTIME_API UMPActorNetworkWrapper : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // =========================================================================
    // NETWORK ROLE & AUTHORITY
    // =========================================================================

    /**
     * Returns the Local Role of this actor on this machine.
     * Machine: Server / Client / Both
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Actor", meta=(DefaultToSelf="TargetActor", DisplayName="Get Local Net Role", Keywords="local role autonomous simulated authority"))
    static ENetRole GetLocalRole(const AActor* TargetActor);

    /**
     * Returns the Remote Role of this actor (how the other side views this actor).
     * Machine: Server / Client / Both
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Actor", meta=(DefaultToSelf="TargetActor", DisplayName="Get Remote Net Role", Keywords="remote role"))
    static ENetRole GetRemoteRole(const AActor* TargetActor);

    // =========================================================================
    // REPLICATION TOGGLES
    // =========================================================================

    /**
     * Enables or disables network replication for an Actor.
     * Machine: Server (Authority only). Calling on client logs a warning.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Multiplayer Utility|Actor", meta=(DefaultToSelf="TargetActor", DisplayName="Set Replicates", Keywords="set replicates replication enable"))
    static void SetReplicates(AActor* TargetActor, bool bReplicates);

    /**
     * Enables or disables movement synchronization for an Actor across the network.
     * Machine: Server (Authority only). Calling on client logs a warning.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Multiplayer Utility|Actor", meta=(DefaultToSelf="TargetActor", DisplayName="Set Replicate Movement", Keywords="replicate movement sync position velocity"))
    static void SetReplicateMovement(AActor* TargetActor, bool bReplicateMovement);

    // =========================================================================
    // OWNERSHIP & CONNECTION
    // =========================================================================

    /**
     * Sets the network Owner of an Actor. Required for an Actor to send Server RPCs.
     * Machine: Server (Authority only).
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Multiplayer Utility|Actor", meta=(DefaultToSelf="TargetActor", DisplayName="Set Net Owner", Keywords="set owner net owner connection rpc"))
    static void SetOwner(AActor* TargetActor, AActor* NewOwner);

    /**
     * Returns the direct Owner of this Actor.
     * Machine: Server / Client / Both
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Actor", meta=(DefaultToSelf="TargetActor", DisplayName="Get Net Owner (Direct)", Keywords="get owner"))
    static AActor* GetOwner(const AActor* TargetActor);

    /**
     * Traverses the owner hierarchy upward until finding the root Actor with a valid NetConnection.
     * Machine: Server / Client / Both
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Actor", meta=(DefaultToSelf="TargetActor", DisplayName="Get Top-Level Net Owner", Keywords="top level net owner connection root"))
    static AActor* GetTopLevelNetOwner(const AActor* TargetActor);

    /**
     * Checks if this Actor currently possesses an active NetConnection to a client or server.
     * Machine: Server / Client / Both
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Actor", meta=(DefaultToSelf="TargetActor", DisplayName="Has Active Net Connection", Keywords="has net connection valid client"))
    static bool HasNetConnection(const AActor* TargetActor);

    // =========================================================================
    // DORMANCY & NETWORK UPDATES
    // =========================================================================

    /**
     * Sets the dormancy state of the actor to optimize server CPU and bandwidth.
     * Machine: Server (Authority only).
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Multiplayer Utility|Actor", meta=(DefaultToSelf="TargetActor", DisplayName="Set Net Dormancy", Keywords="dormancy dormant awake sleep bandwidth"))
    static void SetNetDormancy(AActor* TargetActor, ENetDormancy NewDormancy);

    /**
     * Wakes a dormant actor up so changes replicate on the next network frame.
     * Machine: Server (Authority only).
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Multiplayer Utility|Actor", meta=(DefaultToSelf="TargetActor", DisplayName="Flush Net Dormancy", Keywords="flush dormancy wake update force"))
    static void FlushNetDormancy(AActor* TargetActor);

    /**
     * Forces the server to immediately evaluate replication for this actor on this frame.
     * Machine: Server (Authority only).
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Multiplayer Utility|Actor", meta=(DefaultToSelf="TargetActor", DisplayName="Force Net Update", Keywords="force net update replicate immediately"))
    static void ForceNetUpdate(AActor* TargetActor);

    // =========================================================================
    // UPDATE FREQUENCIES & RELEVANCY
    // =========================================================================

    /**
     * Sets the maximum replication frequency (times per second).
     * Machine: Server (Authority only).
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Multiplayer Utility|Actor", meta=(DefaultToSelf="TargetActor", DisplayName="Set Net Update Frequency", Keywords="net update rate frequency tick rate"))
    static void SetNetUpdateFrequency(AActor* TargetActor, float NewFrequency);

    /**
     * Returns the maximum replication frequency (times per second) configured on this actor.
     * Machine: Server / Client / Both
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Actor", meta=(DefaultToSelf="TargetActor", DisplayName="Get Net Update Frequency", Keywords="net update rate frequency"))
    static float GetNetUpdateFrequency(const AActor* TargetActor);

    /**
     * Sets the minimum replication frequency when actor is stationary or unchanged.
     * Machine: Server (Authority only).
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Multiplayer Utility|Actor", meta=(DefaultToSelf="TargetActor", DisplayName="Set Min Net Update Frequency", Keywords="min net update rate"))
    static void SetMinNetUpdateFrequency(AActor* TargetActor, float NewMinFrequency);

    /**
     * Returns the minimum replication frequency when the actor is stationary or unchanged.
     * Machine: Server / Client / Both
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Actor", meta=(DefaultToSelf="TargetActor", DisplayName="Get Min Net Update Frequency", Keywords="min net update rate"))
    static float GetMinNetUpdateFrequency(const AActor* TargetActor);

    /**
     * Sets actor replication priority relative to other actors.
     * Machine: Server (Authority only).
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Multiplayer Utility|Actor", meta=(DefaultToSelf="TargetActor", DisplayName="Set Net Priority", Keywords="net priority importance bandwidth"))
    static void SetNetPriority(AActor* TargetActor, float NewPriority);

    /**
     * Returns the replication priority multiplier configured on this actor.
     * Machine: Server / Client / Both
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Actor", meta=(DefaultToSelf="TargetActor", DisplayName="Get Net Priority", Keywords="net priority"))
    static float GetNetPriority(const AActor* TargetActor);

    /**
     * Sets the distance in unreal units beyond which this actor is not replicated to clients.
     * Machine: Server (Authority only).
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Multiplayer Utility|Actor", meta=(DefaultToSelf="TargetActor", DisplayName="Set Net Cull Distance", Keywords="net cull distance radius relevance range"))
    static void SetNetCullDistance(AActor* TargetActor, float Distance);

    /**
     * Returns the distance in units beyond which this actor is not replicated to clients.
     * Machine: Server / Client / Both
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Actor", meta=(DefaultToSelf="TargetActor", DisplayName="Get Net Cull Distance", Keywords="net cull distance radius"))
    static float GetNetCullDistance(const AActor* TargetActor);

    /**
     * When true, this actor replicates to all clients regardless of location or culling distance.
     * Machine: Server (Authority only).
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Multiplayer Utility|Actor", meta=(DefaultToSelf="TargetActor", DisplayName="Set Always Relevant", Keywords="always relevant global replicate"))
    static void SetAlwaysRelevant(AActor* TargetActor, bool bAlwaysRelevant);

    /**
     * When true, this actor only replicates to its owning network connection.
     * Machine: Server (Authority only).
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Multiplayer Utility|Actor", meta=(DefaultToSelf="TargetActor", DisplayName="Set Only Relevant To Owner", Keywords="only relevant owner private inventory"))
    static void SetOnlyRelevantToOwner(AActor* TargetActor, bool bOnlyRelevantToOwner);

    /**
     * When true, this actor calculates relevancy based on its Owner's position instead of its own.
     * Machine: Server (Authority only).
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Multiplayer Utility|Actor", meta=(DefaultToSelf="TargetActor", DisplayName="Set Net Use Owner Relevancy", Keywords="owner relevancy attachment weapon"))
    static void SetNetUseOwnerRelevancy(AActor* TargetActor, bool bNetUseOwnerRelevancy);

    /**
     * Checks if this actor was placed in the level editor before the game started (startup actor).
     * Machine: Server / Client / Both
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Actor", meta=(DefaultToSelf="TargetActor", DisplayName="Is Net Startup Actor", Keywords="startup level actor placed static"))
    static bool IsNetStartupActor(const AActor* TargetActor);

    /**
     * Resolves the logical player owner's PlayerState for an actor.
     * First queries IMPOwnershipInterface on the actor; if not implemented, traverses the engine Owner chain.
     * Runs on: Both (Server and Client).
     * 
     * @param TargetActor Actor to query.
     * @return Authoritative APlayerState of the logical player owner, or nullptr.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Actor", meta=(DefaultToSelf="TargetActor", DisplayName="Get Logical Owner Safe", Keywords="get logical owner safe player state ownership chain"))
    static APlayerState* GetLogicalOwnerSafe(const AActor* TargetActor);
};

