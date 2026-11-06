#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "InventoryComponent.h"
#include "InventoryItemGameplayAbility.generated.h"

class UInventoryItemData;

UCLASS(Abstract, Blueprintable)
class MULTIPLAYERINVENTORY_API UInventoryItemGameplayAbility : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UInventoryItemGameplayAbility();

    /** Default or active ItemData Data Asset associated with this ability */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory|Item")
    TObjectPtr<UInventoryItemData> ItemData = nullptr;

    /** If true, executing this ability will automatically consume 1 from the item stack when OnItemUsed returns true (unless already consumed via ConsumeSourceItem). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Inventory|Item")
    bool bConsumeOnUse = false;

    /** Tracks whether this item was already consumed during OnItemUsed execution to avoid double-consumption. */
    UPROPERTY(Transient, BlueprintReadOnly, Category="Inventory|Context")
    bool bWasConsumedDuringUse = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory|Item Tags")
    FGameplayTagContainer RequiredItemTags;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory|Item Tags")
    FGameplayTagContainer BlockedItemTags;

    // --- Activation Context (automatically set when this ability is triggered from inventory) ---

    UPROPERTY(BlueprintReadOnly, Category="Inventory|Context")
    TWeakObjectPtr<UInventoryComponent> SourceInventoryComponent;

    UPROPERTY(BlueprintReadOnly, Category="Inventory|Context")
    TObjectPtr<UInventoryItemData> SourceItemData;

    UPROPERTY(BlueprintReadOnly, Category="Inventory|Context")
    EInventoryContainer SourceContainer = EInventoryContainer::Hotbar;

    UPROPERTY(BlueprintReadOnly, Category="Inventory|Context")
    int32 SourceSlotIndex = INDEX_NONE;

    // --- Blueprint Helper Functions ---

    UFUNCTION(BlueprintPure, Category="Inventory|Item")
    bool CanUseItem(UInventoryItemData* InItemData) const;

    UFUNCTION(BlueprintPure, Category="Inventory|Item Tags")
    bool CanUseItemTags(const FGameplayTagContainer& ItemTags) const;

    /** Returns the tags of the item that activated this ability */
    UFUNCTION(BlueprintPure, Category="Inventory|Context")
    FGameplayTagContainer GetSourceItemTags() const;

    /** Returns the ItemData that activated this ability */
    UFUNCTION(BlueprintPure, Category="Inventory|Context")
    UInventoryItemData* GetSourceItemData() const { return SourceItemData ? SourceItemData : ItemData; }

    /** Returns the inventory component that owns this item */
    UFUNCTION(BlueprintPure, Category="Inventory|Context")
    UInventoryComponent* GetSourceInventoryComponent() const { return SourceInventoryComponent.Get(); }

    /** Consumes/decrements the source item from the inventory. Call this in your ability Blueprint when you want to consume the item (e.g. after throw, drink, or use). */
    UFUNCTION(BlueprintCallable, Category="Inventory|Action")
    bool ConsumeSourceItem(int32 Amount = 1);

    /** Manually removes/decrements the source item from the inventory (alias for ConsumeSourceItem). */
    UFUNCTION(BlueprintCallable, Category="Inventory|Action")
    bool RemoveSourceItem(int32 Amount = 1);

    // --- Events (Return bool: return true if successful, false if failed/cancelled) ---

    /** Called when this item is selected/equipped in the hotbar. Return true if equip was successful. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Inventory|Events")
    bool OnItemSelected(UInventoryItemData* InItemData, const FGameplayTagContainer& ItemTags);
    virtual bool OnItemSelected_Implementation(UInventoryItemData* InItemData, const FGameplayTagContainer& ItemTags);

    /** Called when this item is unselected/unequipped from the hotbar. Return true if unequip was successful. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Inventory|Events")
    bool OnItemUnselected(UInventoryItemData* InItemData, const FGameplayTagContainer& ItemTags);
    virtual bool OnItemUnselected_Implementation(UInventoryItemData* InItemData, const FGameplayTagContainer& ItemTags);

    /** Called when this item is used/executed (e.g. attack, consume, throw). Return true if use was successful. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Inventory|Events")
    bool OnItemUsed(UInventoryItemData* InItemData, const FGameplayTagContainer& ItemTags);
    virtual bool OnItemUsed_Implementation(UInventoryItemData* InItemData, const FGameplayTagContainer& ItemTags);
};
