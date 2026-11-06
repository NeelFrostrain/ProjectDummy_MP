// Copyright Epic Games, Inc. All Rights Reserved.

#include "Replication/MPReplicationConditions.h"

FString UMPReplicationConditions::DescribeReplicationCondition(ELifetimeCondition Condition)
{
	switch (Condition)
	{
	case COND_None:                 return TEXT("COND_None (Always Replicated)");
	case COND_InitialOnly:          return TEXT("COND_InitialOnly (Spawn/Join Initial Packet Only)");
	case COND_OwnerOnly:            return TEXT("COND_OwnerOnly (Only Replicated to Owning Connection)");
	case COND_SkipOwner:            return TEXT("COND_SkipOwner (Replicated to All Remote Except Owner)");
	case COND_SimulatedOnly:        return TEXT("COND_SimulatedOnly (Only to Simulated Proxies)");
	case COND_AutonomousOnly:       return TEXT("COND_AutonomousOnly (Only to Autonomous Proxy)");
	case COND_SimulatedOrPhysics:   return TEXT("COND_SimulatedOrPhysics (Simulated Proxies or Physics Simulating)");
	case COND_InitialOrOwner:       return TEXT("COND_InitialOrOwner (Initial Packet or Owner)");
	case COND_Custom:               return TEXT("COND_Custom (Controlled Dynamically via DOREPLIFETIME_ACTIVE_OVERRIDE)");
	case COND_ReplayOrOwner:        return TEXT("COND_ReplayOrOwner");
	case COND_ReplayOnly:           return TEXT("COND_ReplayOnly");
	case COND_SimulatedOnlyNoReplay:return TEXT("COND_SimulatedOnlyNoReplay");
	case COND_SimulatedOrPhysicsNoReplay: return TEXT("COND_SimulatedOrPhysicsNoReplay");
	case COND_SkipReplay:           return TEXT("COND_SkipReplay");
	case COND_Dynamic:              return TEXT("COND_Dynamic (Iris Dynamic Condition)");
	case COND_Never:                return TEXT("COND_Never (Never Replicated)");
	default:                        return TEXT("COND_Unknown");
	}
}

void UMPReplicationConditions::SetDynamicPropertyActiveOverride(
	const UObject* TargetObject,
	uint16 RepIndex,
	bool bIsActive)
{
	if (!TargetObject)
	{
		return;
	}

	UE::Net::Private::FNetPropertyConditionManager::Get().SetPropertyActiveOverride(TargetObject, RepIndex, bIsActive);
}
