# Multiplayer Utility — Replication & Fast Array Guide

## 1. Fast Array Serializer Framework

Standard `TArray<T>` replication resends the **entire array** whenever any single element changes, generating unacceptable bandwidth spikes for inventories, quest logs, and buff lists.

`FFastArraySerializer` replicates only **modified elements (delta serialization)** and tracks adds/removes cleanly.

### Pre-built Reusable Structures:
1. `FMPReplicatedKeyValueArray`: Fast string/name dictionary for dynamic replicated metadata.

> [!NOTE]
> Dedicated gameplay inventory arrays and components are fully encapsulated inside the independent [`MultiplayerInventory`](../../MultiplayerInventory) plugin. `MultiplayerUtility` provides the fast array serializer primitives and dynamic replication helpers.

### Defining a Custom Fast Array:
```cpp
#include "Replication/MPFastArrayTypes.h"
#include "MyFastArray.generated.h"

USTRUCT(BlueprintType)
struct FMyCustomItem : public FMPFastArrayItemBase
{
    GENERATED_BODY()

    UPROPERTY()
    int32 Score = 0;
};

USTRUCT(BlueprintType)
struct FMyCustomArray : public FMPFastArrayBase
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FMyCustomItem> Items;

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
    {
        return FFastArraySerializer::FastArrayDeltaSerialize<FMyCustomItem, FMyCustomArray>(Items, DeltaParms, *this);
    }
};

template<>
struct TStructOpsTypeTraits<FMyCustomArray> : public TStructOpsTypeTraitsBase2<FMyCustomArray>
{
    enum { WithNetDeltaSerializer = true };
};
```

---

## 2. Dynamic Replication Conditions

To toggle property transmission dynamically (e.g. only replicating sensitive data to certain players or during certain gameplay states):

```cpp
#include "Replication/MPReplicationConditions.h"

// In Actor GetLifetimeReplicatedProps:
void AMyActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION(AMyActor, SecretHealth, COND_Custom);
}

// When you want to dynamically toggle it:
MP_OVERRIDE_REPLICATED_PROPERTY_ACTIVE(AMyActor, SecretHealth, bShouldReplicate);
```
