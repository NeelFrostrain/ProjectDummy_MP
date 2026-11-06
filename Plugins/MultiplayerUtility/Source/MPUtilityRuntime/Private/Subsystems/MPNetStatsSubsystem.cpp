#include "Subsystems/MPNetStatsSubsystem.h"

void UMPNetStatsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ResetNetworkStats();
}

void UMPNetStatsSubsystem::Deinitialize()
{
    ResetNetworkStats();
    Super::Deinitialize();
}

void UMPNetStatsSubsystem::RecordRPCCall(FName RPCName)
{
    if (!RPCName.IsNone())
    {
        int32& Count = RPCCounters.FindOrAdd(RPCName);
        Count++;
    }
}

int32 UMPNetStatsSubsystem::GetRPCCallCount(FName RPCName) const
{
    if (RPCName.IsNone())
    {
        return 0;
    }
    const int32* Found = RPCCounters.Find(RPCName);
    return Found ? *Found : 0;
}

void UMPNetStatsSubsystem::RecordActorSpawned()
{
    TotalActorsSpawned++;
}

void UMPNetStatsSubsystem::RecordActorDestroyed()
{
    TotalActorsDestroyed++;
}

void UMPNetStatsSubsystem::GetNetworkStats(int32& OutTotalSpawned, int32& OutTotalDestroyed) const
{
    OutTotalSpawned = TotalActorsSpawned;
    OutTotalDestroyed = TotalActorsDestroyed;
}

void UMPNetStatsSubsystem::ResetNetworkStats()
{
    RPCCounters.Empty();
    TotalActorsSpawned = 0;
    TotalActorsDestroyed = 0;
}

