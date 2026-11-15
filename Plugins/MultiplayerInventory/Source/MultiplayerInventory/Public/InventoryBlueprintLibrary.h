#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "InventoryComponent.h"
#include "InventoryItemData.h"
#include "InventoryBlueprintLibrary.generated.h"

UCLASS()
class MULTIPLAYERINVENTORY_API UInventoryBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Finds UInventoryComponent on an Actor, Pawn, or PlayerController
    UFUNCTION(BlueprintPure, Category="Inventory|Utility")
    static UInventoryComponent* GetPlayerInventoryComponent(const AActor* ContextActor);

    UFUNCTION(BlueprintPure, Category="Inventory|Slot", meta=(DisplayName="Is Slot Empty"))
    static bool IsSlotEmpty(const FInventorySlot& Slot);

    UFUNCTION(BlueprintPure, Category="Inventory|Slot", meta=(DisplayName="Break Inventory Slot", NativeBreakFunc))
    static void BreakInventorySlot(const FInventorySlot& Slot, int32& OutSlotIndex, UInventoryItemData*& OutItemData, int32& OutQuantity, bool& bOutIsEmpty);

    UFUNCTION(BlueprintPure, Category="Inventory|Slot", meta=(DisplayName="Make Inventory Slot", NativeMakeFunc))
    static FInventorySlot MakeInventorySlot(int32 SlotIndex, UInventoryItemData* ItemData, int32 Quantity);

    // Formats a quantity cleanly: e.g. 500 -> "500", 1500 -> "1.5K", 2500000 -> "2.5M"
    UFUNCTION(BlueprintPure, Category="Inventory|Utility")
    static FText FormatItemQuantity(int32 Quantity);

    // Safely gets display name of ItemData (or empty text if null)
    UFUNCTION(BlueprintPure, Category="Inventory|Item")
    static FText GetItemDisplayName(const UInventoryItemData* ItemData);

    // Safely gets description of ItemData
    UFUNCTION(BlueprintPure, Category="Inventory|Item")
    static FText GetItemDescription(const UInventoryItemData* ItemData);

    // Safely gets icon texture of ItemData
    UFUNCTION(BlueprintPure, Category="Inventory|Item")
    static UTexture2D* GetItemIcon(const UInventoryItemData* ItemData);

    // Safely gets max stack size of ItemData
    UFUNCTION(BlueprintPure, Category="Inventory|Item")
    static int32 GetItemMaxStack(const UInventoryItemData* ItemData);

    // Filters array of slots to return only those containing the specified item
    UFUNCTION(BlueprintPure, Category="Inventory|Slot")
    static TArray<FInventorySlot> FilterSlotsByItem(const TArray<FInventorySlot>& Slots, const UInventoryItemData* ItemData);

    // Calculates total count across an array of slots
    UFUNCTION(BlueprintPure, Category="Inventory|Slot")
    static int32 GetTotalQuantityInSlots(const TArray<FInventorySlot>& Slots);

    // Returns array of indices of all empty slots
    UFUNCTION(BlueprintPure, Category="Inventory|Slot")
    static TArray<int32> FindEmptySlotIndices(const TArray<FInventorySlot>& Slots);

    // Sets up an array of slot widgets with an inventory component, assigning their SlotIndex to match their Array Index. Returns the widgets array.
    UFUNCTION(BlueprintCallable, Category="Inventory|UI", meta=(DisplayName="Setup Slot Widgets with Component"))
    static TArray<class UInventorySlotWidget*> SetupSlotWidgets(UInventoryComponent* InventoryComponent, EInventoryContainer Container, const TArray<class UInventorySlotWidget*>& InWidgets);

    // --- Standard Inventory Event Gameplay Tags ---
    UFUNCTION(BlueprintPure, Category="Inventory|Events")
    static FGameplayTag GetEventTag_Selected();

    UFUNCTION(BlueprintPure, Category="Inventory|Events")
    static FGameplayTag GetEventTag_Unselected();

    UFUNCTION(BlueprintPure, Category="Inventory|Events")
    static FGameplayTag GetEventTag_PickedUp();

    UFUNCTION(BlueprintPure, Category="Inventory|Events")
    static FGameplayTag GetEventTag_Dropped();

    UFUNCTION(BlueprintPure, Category="Inventory|Events")
    static FGameplayTag GetEventTag_Added();

    UFUNCTION(BlueprintPure, Category="Inventory|Events")
    static FGameplayTag GetEventTag_Removed();

    UFUNCTION(BlueprintPure, Category="Inventory|Events")
    static FGameplayTag GetEventTag_Used();

    // Backward-compatible tag getters
    UFUNCTION(BlueprintPure, Category="Inventory|Events")
    static FGameplayTag GetEventTag_Equipped();

    UFUNCTION(BlueprintPure, Category="Inventory|Events")
    static FGameplayTag GetEventTag_Unequipped();

    UFUNCTION(BlueprintPure, Category="Inventory|Events")
    static FGameplayTag GetEventTag_Consumed();
};
