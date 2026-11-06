// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "MPFastArrayTypes.generated.h"

/**
 * Base struct for items replicated via the Multiplayer Utility Fast Array framework.
 * Extends Unreal's FFastArraySerializerItem with tracking metadata and unique item ID.
 */
USTRUCT(BlueprintType)
struct MPUTILITYRUNTIME_API FMPFastArrayItemBase : public FFastArraySerializerItem
{
	GENERATED_BODY()

	/** Unique identifier for this item within the array. */
	UPROPERTY(BlueprintReadOnly, Category = "Networking|FastArray")
	FGuid ItemGuid = FGuid();

	/** Authoritative timestamp when item was added/modified. */
	UPROPERTY(BlueprintReadOnly, Category = "Networking|FastArray")
	double LastModifiedServerTime = 0.0;

	FMPFastArrayItemBase()
		: ItemGuid(FGuid::NewGuid())
		, LastModifiedServerTime(0.0)
	{
	}

	/** Called on the client when this item is received via delta replication. */
	void PostReplicatedAdd(const struct FMPFastArrayBase& InArraySerializer) {}
	void PreReplicatedRemove(const struct FMPFastArrayBase& InArraySerializer) {}
	void PostReplicatedChange(const struct FMPFastArrayBase& InArraySerializer) {}
};

/**
 * Reusable Fast Array container base struct.
 * Provides optimized delta-serialization, late-join sync, and index tracking.
 */
USTRUCT(BlueprintType)
struct MPUTILITYRUNTIME_API FMPFastArrayBase : public FFastArraySerializer
{
	GENERATED_BODY()

	FMPFastArrayBase()
		: OwningActor(nullptr)
	{
	}

	void SetOwningActor(AActor* InOwner)
	{
		OwningActor = InOwner;
	}

	AActor* GetOwningActor() const { return OwningActor.Get(); }

protected:
	UPROPERTY()
	TWeakObjectPtr<AActor> OwningActor;
};

/**
 * Generic string key-value item for replicated fast dictionaries.
 */
USTRUCT(BlueprintType)
struct MPUTILITYRUNTIME_API FMPReplicatedKeyValuePair : public FMPFastArrayItemBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|FastArray")
	FName Key = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|FastArray")
	FString Value;

	FMPReplicatedKeyValuePair() = default;

	FMPReplicatedKeyValuePair(FName InKey, const FString& InValue)
		: Key(InKey)
		, Value(InValue)
	{
	}
};

/**
 * Fast Array Serializer for key-value pair lists.
 */
USTRUCT(BlueprintType)
struct MPUTILITYRUNTIME_API FMPReplicatedKeyValueArray : public FMPFastArrayBase
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Networking|FastArray")
	TArray<FMPReplicatedKeyValuePair> Items;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FMPReplicatedKeyValuePair, FMPReplicatedKeyValueArray>(Items, DeltaParms, *this);
	}

	/** Adds or updates a key-value entry. Returns true if modified. */
	bool SetValue(FName Key, const FString& Value, double ServerTime = 0.0);

	/** Removes an entry by key. Returns true if found and removed. */
	bool RemoveKey(FName Key);

	/** Finds value by key. */
	bool GetValue(FName Key, FString& OutValue) const;

	/** Checks if key exists. */
	bool ContainsKey(FName Key) const;

	/** Empties the array and marks dirty. */
	void Clear();

	/** Total element count. */
	int32 Num() const { return Items.Num(); }
};

template<>
struct TStructOpsTypeTraits<FMPReplicatedKeyValueArray> : public TStructOpsTypeTraitsBase2<FMPReplicatedKeyValueArray>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};
