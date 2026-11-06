#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ItemDefinition.generated.h"

class UTexture2D;
class UStaticMesh;
class AItemPickup;
class UItemContainerComponent;

/**
 * Optional "what happens when this item is used".
 * Make a Blueprint subclass, override "On Use", and put it in the item's Use Action slot.
 * Actions are shared by every use of the item, so keep them stateless.
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class MODULARINVENTORY_API UItemAction : public UObject
{
    GENERATED_BODY()

public:
    /** Runs on the SERVER. Return true if the item was really used (this allows Consume On Use to take effect). */
    UFUNCTION(BlueprintNativeEvent, Category = "Item")
    bool OnUse(UItemContainerComponent* Container, int32 SlotIndex, AActor* User);
    virtual bool OnUse_Implementation(UItemContainerComponent* Container, int32 SlotIndex, AActor* User) { return true; }

    bool ExecuteUse(UItemContainerComponent* Container, int32 SlotIndex, AActor* User);

    virtual UWorld* GetWorld() const override;

private:
    TWeakObjectPtr<UWorld> CachedWorld;
};

/** A kind of item (Apple, Sword, Wood...). Create one Data Asset per item type. */
UCLASS(BlueprintType, Blueprintable)
class MODULARINVENTORY_API UItemDefinition : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (MultiLine = true))
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
    TObjectPtr<UTexture2D> Icon = nullptr;

    /** 1 = does not stack. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1"))
    int32 MaxStackSize = 64;

    /** Free-form tags. Containers can allow/block items by tag (e.g. a slot that only takes Item.Weapon). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
    FGameplayTagContainer ItemTags;

    // --- Using ---

    /** Optional. What happens when the player uses this item. If empty, "use" just fires the container's On Item Used event. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "Item|Use")
    TObjectPtr<UItemAction> UseAction;

    /** Remove 1 from the stack after a successful use (food, potions, ammo...). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Use")
    bool bConsumeOnUse = false;

    // --- In the world ---

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|World")
    bool bCanDrop = true;

    /** Mesh shown when this item lies in the world. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|World")
    TObjectPtr<UStaticMesh> PickupMesh = nullptr;

    /** Optional custom pickup actor (subclass of Item Pickup). Leave empty to use the default. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|World")
    TSubclassOf<AItemPickup> PickupClass;
};
