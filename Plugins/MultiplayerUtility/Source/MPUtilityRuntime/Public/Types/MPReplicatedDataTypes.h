#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "MPReplicatedDataTypes.generated.h"

/**
 * Authoritatively replicated counter with custom NetSerialize.
 * Server increments/decrements; clients read synchronized value.
 */
USTRUCT(BlueprintType)
struct MPUTILITYRUNTIME_API FMPReplicatedCounter
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Multiplayer Utility|Replication")
    int32 CounterValue = 0;

    int32 Increment(int32 Amount = 1)
    {
        CounterValue += Amount;
        return CounterValue;
    }

    int32 Decrement(int32 Amount = 1)
    {
        CounterValue -= Amount;
        return CounterValue;
    }

    int32 GetValue() const
    {
        return CounterValue;
    }

    void Reset()
    {
        CounterValue = 0;
    }

    bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
    {
        Ar << CounterValue;
        bOutSuccess = true;
        return true;
    }
};

template<>
struct TStructOpsTypeTraits<FMPReplicatedCounter> : public TStructOpsTypeTraitsBase2<FMPReplicatedCounter>
{
    enum
    {
        WithNetSerializer = true
    };
};

/**
 * Authoritatively replicated bitmask flag set.
 * Allows packing up to 32 boolean game states into a single replicated integer.
 */
USTRUCT(BlueprintType)
struct MPUTILITYRUNTIME_API FMPReplicatedFlagSet
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Multiplayer Utility|Replication")
    int32 Flags = 0;

    void SetFlag(int32 BitIndex, bool bValue = true)
    {
        if (BitIndex >= 0 && BitIndex < 32)
        {
            if (bValue)
            {
                Flags |= (1 << BitIndex);
            }
            else
            {
                Flags &= ~(1 << BitIndex);
            }
        }
    }

    void ClearFlag(int32 BitIndex)
    {
        SetFlag(BitIndex, false);
    }

    bool HasFlag(int32 BitIndex) const
    {
        if (BitIndex >= 0 && BitIndex < 32)
        {
            return (Flags & (1 << BitIndex)) != 0;
        }
        return false;
    }

    void ToggleFlag(int32 BitIndex)
    {
        if (BitIndex >= 0 && BitIndex < 32)
        {
            Flags ^= (1 << BitIndex);
        }
    }

    bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
    {
        Ar << Flags;
        bOutSuccess = true;
        return true;
    }
};

template<>
struct TStructOpsTypeTraits<FMPReplicatedFlagSet> : public TStructOpsTypeTraitsBase2<FMPReplicatedFlagSet>
{
    enum
    {
        WithNetSerializer = true
    };
};

/**
 * Replicated fixed-size ring buffer for debugging and HUD event logs.
 */
USTRUCT(BlueprintType)
struct MPUTILITYRUNTIME_API FMPReplicatedLogRing
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Multiplayer Utility|Replication")
    TArray<FString> Messages;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Multiplayer Utility|Replication")
    int32 MaxMessages = 8;

    void AddMessage(const FString& InMsg)
    {
        if (Messages.Num() >= MaxMessages && Messages.Num() > 0)
        {
            Messages.RemoveAt(0);
        }
        Messages.Add(InMsg);
    }

    bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
    {
        Ar << Messages;
        Ar << MaxMessages;
        bOutSuccess = true;
        return true;
    }
};

template<>
struct TStructOpsTypeTraits<FMPReplicatedLogRing> : public TStructOpsTypeTraitsBase2<FMPReplicatedLogRing>
{
    enum
    {
        WithNetSerializer = true
    };
};

struct FMPReplicatedKeyValue;

/**
 * Individual key-value entry for FFastArraySerializer.
 */
USTRUCT(BlueprintType)
struct MPUTILITYRUNTIME_API FMPReplicatedKeyValueItem : public FFastArraySerializerItem
{
    GENERATED_BODY()

    UPROPERTY()
    FString Key;

    UPROPERTY()
    FString StringValue;

    UPROPERTY()
    float FloatValue = 0.0f;

    UPROPERTY()
    int32 IntValue = 0;
};

/**
 * Replicated string-to-value map using Unreal Engine's FFastArraySerializer.
 * Efficiently replicates only changed elements; sends full current state to late joiners.
 */
USTRUCT(BlueprintType)
struct MPUTILITYRUNTIME_API FMPReplicatedKeyValue : public FFastArraySerializer
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FMPReplicatedKeyValueItem> Items;

    void SetString(const FString& Key, const FString& Val);
    FString GetString(const FString& Key, const FString& DefaultVal = TEXT("")) const;

    void SetFloat(const FString& Key, float Val);
    float GetFloat(const FString& Key, float DefaultVal = 0.0f) const;

    void SetInt(const FString& Key, int32 Val);
    int32 GetInt(const FString& Key, int32 DefaultVal = 0) const;

    bool Remove(const FString& Key);
    void Empty();
    int32 Num() const { return Items.Num(); }

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
    {
        return FFastArraySerializer::FastArrayDeltaSerialize<FMPReplicatedKeyValueItem, FMPReplicatedKeyValue>(Items, DeltaParms, *this);
    }
};

template<>
struct TStructOpsTypeTraits<FMPReplicatedKeyValue> : public TStructOpsTypeTraitsBase2<FMPReplicatedKeyValue>
{
    enum
    {
        WithNetDeltaSerializer = true
    };
};

