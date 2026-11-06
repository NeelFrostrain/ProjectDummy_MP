#pragma once

#include "CoreMinimal.h"
#include "ItemTypes.generated.h"

class UItemDefinition;

/** Who gets to see the contents of a container. */
UENUM(BlueprintType)
enum class EItemContainerReplication : uint8
{
    /** Only the owning player sees the contents. Use for backpack, hotbar, equipment. */
    OwnerOnly UMETA(DisplayName = "Owner Only (private)"),

    /** Everyone who can see the actor sees the contents. Use for chests, shops, loot crates. */
    Everyone  UMETA(DisplayName = "Everyone (shared)")
};

/** "N of this item". Used for starting items, costs, rewards. */
USTRUCT(BlueprintType)
struct MODULARINVENTORY_API FItemStack
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    TObjectPtr<UItemDefinition> Item = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (ClampMin = "1"))
    int32 Count = 1;

    FItemStack() = default;
    FItemStack(UItemDefinition* InItem, int32 InCount) : Item(InItem), Count(InCount) {}

    bool IsEmpty() const { return Item == nullptr || Count <= 0; }
};

/** One slot inside a container. The slot's index is its position in the container. */
USTRUCT(BlueprintType)
struct MODULARINVENTORY_API FItemSlot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Item")
    TObjectPtr<UItemDefinition> Item = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Item")
    int32 Count = 0;

    bool IsEmpty() const { return Item == nullptr || Count <= 0; }

    bool IsSame(const FItemSlot& Other) const
    {
        if (IsEmpty() && Other.IsEmpty()) { return true; }
        return Item == Other.Item && Count == Other.Count;
    }
};
