// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PropertyConditions/PropertyConditions.h"
#include "MPReplicationConditions.generated.h"

/**
 * Standard Push Model macro wrapper.
 * Safely marks property dirty when Push Model is enabled, and does nothing when compiled without push model.
 */
#if WITH_PUSH_MODEL
	#define MP_MARK_PROPERTY_DIRTY(OwnerObject, PropertyName) \
		MARK_PROPERTY_DIRTY_FROM_NAME(OwnerObject->GetClass(), PropertyName, OwnerObject)
#else
	#define MP_MARK_PROPERTY_DIRTY(OwnerObject, PropertyName) do {} while(0)
#endif

/**
 * Convenience macro to dynamically toggle replication of a property inside an Actor or Component.
 * Must be called in a scope where 'this' points to the replicated UObject.
 */
#define MP_OVERRIDE_REPLICATED_PROPERTY_ACTIVE(ClassName, PropertyName, bIsActive) \
	DOREPLIFETIME_ACTIVE_OVERRIDE_FAST(ClassName, PropertyName, bIsActive)

/**
 * Reusable helper library for replication conditions, custom lifetime overrides, and RepNotify dispatch.
 */
UCLASS()
class MPUTILITYRUNTIME_API UMPReplicationConditions : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Returns human-readable description of an ELifetimeCondition enum value.
	 */
	UFUNCTION(BlueprintPure, Category = "Networking|Replication")
	static FString DescribeReplicationCondition(ELifetimeCondition Condition);

	/**
	 * Compares two byte buffers to determine if an uncompressed state change occurred (prevents redundant RepNotify calls).
	 */
	static bool HasStateChanged(const void* OldData, const void* NewData, int32 DataSize)
	{
		if (!OldData || !NewData || DataSize <= 0)
		{
			return false;
		}
		return FMemory::Memcmp(OldData, NewData, DataSize) != 0;
	}

	/**
	 * Helper to safely enable or disable a replicated property's transmission dynamically by RepIndex.
	 */
	static void SetDynamicPropertyActiveOverride(
		const UObject* TargetObject,
		uint16 RepIndex,
		bool bIsActive
	);
};
