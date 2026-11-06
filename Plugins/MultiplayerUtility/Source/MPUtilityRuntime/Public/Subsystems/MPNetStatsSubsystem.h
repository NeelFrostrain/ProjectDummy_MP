#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MPNetStatsSubsystem.generated.h"

/**
 * Lightweight diagnostic world subsystem tracking multiplayer network metrics
 * (RPC call volumes by name, spawned/destroyed actor counts) in development builds.
 */
UCLASS()
class MPUTILITYRUNTIME_API UMPNetStatsSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /**
     * Records a single RPC invocation under the given name.
     * Runs on: Both (Server and Client).
     */
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Utility|Stats", meta=(DisplayName="Record RPC Call", Keywords="record rpc call count metric stats"))
    void RecordRPCCall(FName RPCName);

    /**
     * Returns the total number of calls recorded for a specific RPC.
     * Runs on: Both (Server and Client).
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Stats", meta=(DisplayName="Get RPC Call Count", Keywords="get rpc call count metric stats"))
    int32 GetRPCCallCount(FName RPCName) const;

    /**
     * Increments the total spawned network actor counter.
     * Runs on: Both (Server and Client).
     */
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Utility|Stats", meta=(DisplayName="Record Actor Spawned", Keywords="record actor spawn metric stats"))
    void RecordActorSpawned();

    /**
     * Increments the total destroyed network actor counter.
     * Runs on: Both (Server and Client).
     */
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Utility|Stats", meta=(DisplayName="Record Actor Destroyed", Keywords="record actor destroy metric stats"))
    void RecordActorDestroyed();

    /**
     * Returns the total counts of spawned and destroyed actors recorded in this session.
     * Runs on: Both (Server and Client).
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Stats", meta=(DisplayName="Get Network Actor Stats", Keywords="get network stats actors spawned destroyed"))
    void GetNetworkStats(int32& OutTotalSpawned, int32& OutTotalDestroyed) const;

    /**
     * Resets all RPC counters and actor spawn/destroy counters to zero.
     * Runs on: Both (Server and Client).
     */
    UFUNCTION(BlueprintCallable, Category = "Multiplayer Utility|Stats", meta=(DisplayName="Reset Network Stats", Keywords="reset network stats clear counters"))
    void ResetNetworkStats();

private:
    UPROPERTY()
    TMap<FName, int32> RPCCounters;

    UPROPERTY()
    int32 TotalActorsSpawned = 0;

    UPROPERTY()
    int32 TotalActorsDestroyed = 0;
};

