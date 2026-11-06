#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/EngineTypes.h"
#include "MPComponentNetworkWrapper.generated.h"

class UActorComponent;
class AActor;

UCLASS()
class MPUTILITYRUNTIME_API UMPComponentNetworkWrapper : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Enables or disables replication for an ActorComponent.
     * Machine: Server (Authority only).
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Multiplayer Utility|Component", meta=(DisplayName="Set Component Replicated", Keywords="component replicate network enable"))
    static void SetComponentReplicated(UActorComponent* TargetComponent, bool bReplicates);

    /**
     * Returns true if the target ActorComponent is configured to replicate.
     * Machine: Server / Client / Both
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Component", meta=(DisplayName="Is Component Replicated", Keywords="is component replicated"))
    static bool IsComponentReplicated(const UActorComponent* TargetComponent);

    /**
     * Returns true if the component's owner is actively simulating (SimulatedProxy or Physics).
     * Machine: Server / Client / Both
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Component", meta=(DisplayName="Is Component Net Simulating", Keywords="component net simulating"))
    static bool IsComponentNetSimulating(const UActorComponent* TargetComponent);

    /**
     * Returns the Local Role of the Actor that owns this Component.
     * Machine: Server / Client / Both
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Component", meta=(DisplayName="Get Component Owner Local Role", Keywords="component owner local role"))
    static ENetRole GetComponentOwnerLocalRole(const UActorComponent* TargetComponent);

    /**
     * Returns the Remote Role of the Actor that owns this Component.
     * Machine: Server / Client / Both
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Component", meta=(DisplayName="Get Component Owner Remote Role", Keywords="component owner remote role"))
    static ENetRole GetComponentOwnerRemoteRole(const UActorComponent* TargetComponent);

    /**
     * Checks if the local machine has network authority over the component's owning actor.
     * Machine: Server / Client / Both
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Component", meta=(DisplayName="Has Component Authority", Keywords="has component authority owner server"))
    static bool HasComponentAuthority(const UActorComponent* TargetComponent);

    /**
     * Safely registers a subobject for network replication on an Actor or ActorComponent (UE 5.1+ Registered Subobject list).
     * Runs on: Server (Authority only).
     * 
     * @param Owner Owning Actor or ActorComponent.
     * @param SubObject The UObject subobject to replicate.
     * @param NetCondition Lifetime replication condition (default COND_None).
     * @return True if successfully registered; false if arguments are invalid or lack authority.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Multiplayer Utility|Component", meta=(DisplayName="Register Replicated Subobject Safe", Keywords="register replicated subobject safe component actor"))
    static bool RegisterReplicatedSubobjectSafe(UObject* Owner, UObject* SubObject, ELifetimeCondition NetCondition = COND_None);

    /**
     * Safely unregisters a subobject from network replication on an Actor or ActorComponent.
     * Runs on: Server (Authority only).
     * 
     * @param Owner Owning Actor or ActorComponent.
     * @param SubObject The UObject subobject to unregister.
     * @return True if successfully unregistered; false otherwise.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Multiplayer Utility|Component", meta=(DisplayName="Unregister Replicated Subobject Safe", Keywords="unregister replicated subobject safe component actor"))
    static bool UnregisterReplicatedSubobjectSafe(UObject* Owner, UObject* SubObject);

    /**
     * Checks whether a subobject is currently registered for network replication on an Actor or ActorComponent.
     * Runs on: Both (Server and Client).
     * 
     * @param Owner Owning Actor or ActorComponent.
     * @param SubObject The UObject subobject to inspect.
     * @return True if registered for replication; false otherwise.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Component", meta=(DisplayName="Is Subobject Replicated", Keywords="is subobject replicated registered check"))
    static bool IsSubobjectReplicated(const UObject* Owner, const UObject* SubObject);

    /**
     * Returns all ActorComponents attached to the actor that are configured to replicate across the network.
     * Runs on: Both (Server and Client).
     * 
     * @param TargetActor Actor whose components will be inspected.
     * @return Array of replicating actor components.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Component", meta=(DefaultToSelf="TargetActor", DisplayName="Get Replicated Components Of Actor", Keywords="get replicated components actor list find"))
    static TArray<UActorComponent*> GetReplicatedComponentsOfActor(const AActor* TargetActor);

    /**
     * Sets replication state on a component and recursively applies it to all attached child scene components.
     * Runs on: Server (Authority only).
     * 
     * @param TargetComponent Root component to update.
     * @param bReplicates True to enable replication, false to disable.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Multiplayer Utility|Component", meta=(DefaultToSelf="TargetComponent", DisplayName="Set Component Replicated Recursive", Keywords="set component replicated recursive children hierarchy"))
    static void SetComponentReplicatedRecursive(UActorComponent* TargetComponent, bool bReplicates);

    /**
     * Diagnostic helper that formats and optionally logs all replicated components and subobjects of an actor.
     * Runs on: Both (Server and Client).
     * 
     * @param TargetActor Actor to inspect.
     * @param bPrintToLog If true, outputs summary lines to UE_LOG.
     * @return Formatted strings describing all replicated components and subobjects.
     */
    UFUNCTION(BlueprintCallable, Category="Multiplayer Utility|Component", meta=(DefaultToSelf="TargetActor", DisplayName="Print Replicated Subobjects", Keywords="print replicated subobjects debug components list"))
    static TArray<FString> PrintReplicatedSubobjects(const AActor* TargetActor, bool bPrintToLog = true);
};

