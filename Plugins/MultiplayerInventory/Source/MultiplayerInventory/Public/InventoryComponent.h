#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "InventoryComponent.generated.h"

namespace InventoryItemEventTags
{
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Selected);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Unselected);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Used);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(PickedUp);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dropped);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Added);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Removed);

    // Aliases for backward compatibility
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Equipped);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Unequipped);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Consumed);
}

class UInventoryItemData;
class AWorldInventoryItem;
class UAbilitySystemComponent;

UENUM(BlueprintType)
enum class EInventoryContainer : uint8
{
    Inventory UMETA(DisplayName="Inventory"),
    Hotbar UMETA(DisplayName="Hotbar")
};

USTRUCT(BlueprintType)
struct MULTIPLAYERINVENTORY_API FInventorySlot : public FFastArraySerializerItem
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
    int32 SlotIndex = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
    TObjectPtr<UInventoryItemData> ItemData = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
    int32 Quantity = 0;
};

USTRUCT(BlueprintType)
struct MULTIPLAYERINVENTORY_API FInventoryArray : public FFastArraySerializer
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Inventory")
    TArray<FInventorySlot> Slots;

    UPROPERTY(NotReplicated)
    TObjectPtr<class UInventoryComponent> OwnerComponent = nullptr;

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
    {
        return FFastArraySerializer::FastArrayDeltaSerialize<FInventorySlot, FInventoryArray>(Slots, DeltaParms, *this);
    }

    void PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize);
    void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize);
    void PostReplicatedReceive(const FFastArraySerializer::FPostReplicatedReceiveParameters& Parameters);
};

template<> struct TStructOpsTypeTraits<FInventoryArray> : public TStructOpsTypeTraitsBase2<FInventoryArray>
{
    enum { WithNetDeltaSerializer = true };
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHotbarSelectionChanged, int32, NewIndex);

/** Delegate signature with standard item action parameters: (ItemData, Quantity, Container, SlotIndex, ItemTags, RelatedActor) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(
    FOnInventoryItemActionSignature,
    UInventoryItemData*, ItemData,
    int32, Quantity,
    EInventoryContainer, Container,
    int32, SlotIndex,
    const FGameplayTagContainer&, ItemTags,
    AActor*, RelatedActor
);

/** Unified dynamic multicast delegate for all inventory item events */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_SevenParams(
    FOnInventoryItemEventSignature,
    FGameplayTag, EventTag,
    UInventoryItemData*, ItemData,
    int32, Quantity,
    EInventoryContainer, Container,
    int32, SlotIndex,
    const FGameplayTagContainer&, ItemTags,
    AActor*, RelatedActor
);

UCLASS(ClassGroup=(Inventory), meta=(BlueprintSpawnableComponent))
class MULTIPLAYERINVENTORY_API UInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UInventoryComponent();

    /** Number of slots in the main inventory container (editable in Blueprint/Details) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Setup", meta=(ClampMin="1"))
    int32 InventorySlotCount = 28;

    /** Number of slots in the hotbar container (editable in Blueprint/Details, defaults to 6) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Setup", meta=(ClampMin="1"))
    int32 HotbarSlotCount = 6;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
    float MaxPickupDistance = 600.f;

    UPROPERTY(BlueprintAssignable, Category="Inventory|Events")
    FOnInventoryChanged OnInventoryChanged;

    UPROPERTY(BlueprintAssignable, Category="Inventory|Events")
    FOnHotbarSelectionChanged OnHotbarSelectionChanged;

    /** Fired when an item is selected in the hotbar */
    UPROPERTY(BlueprintAssignable, Category="Inventory|Events")
    FOnInventoryItemActionSignature OnItemSelected;

    /** Fired when an item is unselected from the hotbar */
    UPROPERTY(BlueprintAssignable, Category="Inventory|Events")
    FOnInventoryItemActionSignature OnItemUnselected;

    /** Fired when an item is used / executed */
    UPROPERTY(BlueprintAssignable, Category="Inventory|Events")
    FOnInventoryItemActionSignature OnItemUsed;

    /** Fired when an item is picked up from the world */
    UPROPERTY(BlueprintAssignable, Category="Inventory|Events")
    FOnInventoryItemActionSignature OnItemPickedUp;

    /** Fired when an item is dropped into the world */
    UPROPERTY(BlueprintAssignable, Category="Inventory|Events")
    FOnInventoryItemActionSignature OnItemDropped;

    /** Fired when an item is added to an inventory container */
    UPROPERTY(BlueprintAssignable, Category="Inventory|Events")
    FOnInventoryItemActionSignature OnItemAdded;

    /** Fired when an item is removed from an inventory container */
    UPROPERTY(BlueprintAssignable, Category="Inventory|Events")
    FOnInventoryItemActionSignature OnItemRemoved;

    /** Unified event dispatcher (fires for all item actions). */
    UPROPERTY(BlueprintAssignable, Category="Inventory|Events")
    FOnInventoryItemEventSignature OnItemEvent;

    /** Resizes container slots (Server authority only). */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Inventory|Setup")
    void SetSlotCount(EInventoryContainer Container, int32 NewCount);

    /** Convenience helper: Sets up an array of slot widgets, assigning their SlotIndex to match their Array Index. Returns the array. */
    UFUNCTION(BlueprintCallable, Category="Inventory|UI", meta=(DisplayName="Setup Hotbar Slot Widgets"))
    TArray<UInventorySlotWidget*> SetupHotbarSlotWidgets(const TArray<UInventorySlotWidget*>& InWidgets);

    /** Convenience helper: Sets up an array of slot widgets for any container, assigning their SlotIndex to match their Array Index. Returns the array. */
    UFUNCTION(BlueprintCallable, Category="Inventory|UI", meta=(DisplayName="Setup Slot Widgets"))
    TArray<UInventorySlotWidget*> SetupSlotWidgets(EInventoryContainer Container, const TArray<UInventorySlotWidget*>& InWidgets);

    /** Helper function to broadcast OnItemEvent with automatic tag resolution */
    void BroadcastItemEvent(
        FGameplayTag EventTag,
        UInventoryItemData* InItemData,
        int32 InQuantity,
        EInventoryContainer InContainer,
        int32 InSlotIndex,
        AActor* InRelatedActor = nullptr
    );

    UPROPERTY(Replicated, BlueprintReadOnly, Category="Inventory")
    FInventoryArray InventorySlots;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="Inventory")
    FInventoryArray HotbarSlots;

    UPROPERTY(ReplicatedUsing=OnRep_SelectedHotbarIndex, BlueprintReadOnly, Category="Inventory")
    int32 SelectedHotbarIndex = 0;

    // --- Query Functions ---

    UFUNCTION(BlueprintPure, Category="Inventory")
    int32 GetSlotCount(EInventoryContainer Container) const;

    UFUNCTION(BlueprintCallable, Category="Inventory")
    bool GetSlot(EInventoryContainer Container, int32 Index, UInventoryItemData*& OutItemData, int32& OutQuantity) const;

    UFUNCTION(BlueprintPure, Category="Inventory")
    bool GetSlotStruct(EInventoryContainer Container, int32 Index, FInventorySlot& OutSlot) const;

    UFUNCTION(BlueprintPure, Category="Inventory")
    TArray<FInventorySlot> GetAllSlots(EInventoryContainer Container) const;

    UFUNCTION(BlueprintCallable, Category="Inventory")
    int32 GetUsedSlotCount(EInventoryContainer Container) const;

    UFUNCTION(BlueprintPure, Category="Inventory")
    int32 GetSelectedHotbarIndex() const { return SelectedHotbarIndex; }

    UFUNCTION(BlueprintCallable, Category="Inventory")
    bool GetSelectedHotbarItem(UInventoryItemData*& OutItemData, int32& OutQuantity) const;

    UFUNCTION(BlueprintPure, Category="Inventory|Query")
    bool GetSelectedHotbarSlot(FInventorySlot& OutSlot) const;

    UFUNCTION(BlueprintPure, Category="Inventory|Query")
    bool IsHotbarSelectedSlotEmpty() const;

    // Checks if player owns at least RequiredQuantity of this item anywhere in their inventory/hotbar
    UFUNCTION(BlueprintPure, Category="Inventory|Query")
    bool HasItem(UInventoryItemData* ItemData, int32 RequiredQuantity = 1) const;

    // Total count of this item across BOTH containers (Hotbar + Inventory)
    UFUNCTION(BlueprintPure, Category="Inventory|Query")
    int32 GetTotalItemCount(UInventoryItemData* ItemData) const;

    // Count of this item in a specific container
    UFUNCTION(BlueprintPure, Category="Inventory|Query")
    int32 GetItemCountInContainer(EInventoryContainer Container, UInventoryItemData* ItemData) const;

    // First slot containing the item, or INDEX_NONE (-1)
    UFUNCTION(BlueprintPure, Category="Inventory|Query")
    int32 FindFirstSlotWithItem(EInventoryContainer Container, UInventoryItemData* ItemData) const;

    // First empty slot index, or INDEX_NONE (-1) if full
    UFUNCTION(BlueprintPure, Category="Inventory|Query")
    int32 FindFirstEmptySlot(EInventoryContainer Container) const;

    // Returns true if there is at least one empty slot in the container
    UFUNCTION(BlueprintPure, Category="Inventory|Query")
    bool HasEmptySlot(EInventoryContainer Container) const;

    // Checks if a specific slot is empty
    UFUNCTION(BlueprintPure, Category="Inventory|Query")
    bool IsSlotEmpty(EInventoryContainer Container, int32 SlotIndex) const;

    // Checks if Quantity of ItemData can fit into the inventory without overflowing
    UFUNCTION(BlueprintPure, Category="Inventory|Query")
    bool CanAddItem(UInventoryItemData* ItemData, int32 Quantity, EInventoryContainer PreferredContainer = EInventoryContainer::Hotbar) const;

    // Calculates remaining capacity for this item in a container
    UFUNCTION(BlueprintPure, Category="Inventory|Query")
    int32 GetFreeSpaceForItem(UInventoryItemData* ItemData, EInventoryContainer Container) const;

    // Percentage of container slots filled (0.0 = completely empty, 1.0 = completely full)
    UFUNCTION(BlueprintPure, Category="Inventory|Query")
    float GetContainerFillRatio(EInventoryContainer Container) const;

    // --- Plug & Play Pickup Functions ---

    // 1-Node Pickup: Pass ANY Actor hit by trace, overlap, or interact event
    UFUNCTION(BlueprintCallable, Category="Inventory")
    bool TryPickupActor(AActor* TargetActor);

    // 1-Node Front Sweep: Trace forward from camera/eyes to automatically find & pick up items
    UFUNCTION(BlueprintCallable, Category="Inventory")
    bool TryPickupInFront(float Distance = 500.f, float SphereRadius = 40.f);

    // 1-Node Nearby Pickup: Pick up closest item within radius
    UFUNCTION(BlueprintCallable, Category="Inventory")
    bool TryPickupNearby(float Radius = 300.f);

    // Backwards-compatible pickup node
    UFUNCTION(BlueprintCallable, Category="Inventory")
    bool RequestPickup(AWorldInventoryItem* WorldItem);

    // --- Hotbar & Item Use / Consumption Actions ---

    // Returns whether the item can be selected in the hotbar.
    UFUNCTION(BlueprintPure, Category="Inventory|Query", meta=(DisplayName="Can Select Item"))
    bool CanSelectItem(UInventoryItemData* ItemData) const;

    // Backward-compatible alias for CanSelectItem
    UFUNCTION(BlueprintPure, Category="Inventory|Query")
    bool CanEquipItem(UInventoryItemData* ItemData) const { return CanSelectItem(ItemData); }

    // Validates whether the given item data can currently be used by the owner
    UFUNCTION(BlueprintPure, Category="Inventory|Query")
    bool CanUseItem(UInventoryItemData* ItemData) const;

    // Checks if the currently selected hotbar item can be used
    UFUNCTION(BlueprintPure, Category="Inventory|Query")
    bool CanUseSelectedHotbarItem() const;

    // Checks if an item in the given container slot can be used
    UFUNCTION(BlueprintPure, Category="Inventory|Query")
    bool CanUseSlotItem(EInventoryContainer Container, int32 SlotIndex) const;

    // Executes the selected hotbar item (attacks, fires, drinks, throws, etc.).
    UFUNCTION(BlueprintCallable, Category="Inventory|Hotbar", meta=(DisplayName="Execute Selected Hotbar Item"))
    bool ExecuteSelectedHotbarItem();

    // Uses/executes the selected hotbar item (alias for ExecuteSelectedHotbarItem).
    UFUNCTION(BlueprintCallable, Category="Inventory|Hotbar")
    bool UseSelectedHotbarItem();

    // Executes an item from any container and slot index.
    UFUNCTION(BlueprintCallable, Category="Inventory|Action", meta=(DisplayName="Execute Slot Item"))
    bool ExecuteSlotItem(EInventoryContainer Container, int32 SlotIndex);

    // Uses an item from any container and slot index (alias for ExecuteSlotItem).
    UFUNCTION(BlueprintCallable, Category="Inventory|Action")
    bool UseSlotItem(EInventoryContainer Container, int32 SlotIndex);

    // Consumes/decrements an item from a specific slot. Callable on Client or Server. Returns true if request dispatched or consumed.
    UFUNCTION(BlueprintCallable, Category="Inventory|Action")
    bool ConsumeSlotItem(EInventoryContainer Container, int32 SlotIndex, int32 Amount = 1);

    // Consumes/decrements the currently selected hotbar item. Callable on Client or Server. Returns true if request dispatched or consumed.
    UFUNCTION(BlueprintCallable, Category="Inventory|Hotbar")
    bool ConsumeSelectedHotbarItem(int32 Amount = 1);

    // Manually removes an item from a slot (alias for ConsumeSlotItem). Callable on Client or Server.
    UFUNCTION(BlueprintCallable, Category="Inventory|Action", meta=(DisplayName="Remove Slot Item"))
    bool RemoveSlotItem(EInventoryContainer Container, int32 SlotIndex, int32 Amount = 1);

    // Manually removes an item from the selected hotbar slot (alias for ConsumeSelectedHotbarItem). Callable on Client or Server.
    UFUNCTION(BlueprintCallable, Category="Inventory|Hotbar", meta=(DisplayName="Remove Selected Hotbar Item"))
    bool RemoveSelectedHotbarItem(int32 Amount = 1);

    // Selects hotbar slot (0 to 6), activating its ability and deactivating the previously selected item's ability.
    UFUNCTION(BlueprintCallable, Category="Inventory|Hotbar")
    bool SelectHotbarSlot(int32 Index);

    // Selects hotbar slot (0 to 6), activating its ability and deactivating the previously selected item's ability.
    UFUNCTION(BlueprintCallable, Category="Inventory|Hotbar", meta=(DisplayName="Select Hotbar Item"))
    bool SelectHotbarItem(int32 Index);

    // Advances to next hotbar slot. Returns the newly selected index.
    UFUNCTION(BlueprintCallable, Category="Inventory|Hotbar")
    int32 SelectNextHotbar();

    // Backtracks to previous hotbar slot. Returns the newly selected index.
    UFUNCTION(BlueprintCallable, Category="Inventory|Hotbar")
    int32 SelectPreviousHotbar();

    // Requests equipping an item by selecting its hotbar slot. Returns true if the index is valid.
    UFUNCTION(BlueprintCallable, Category="Inventory|Hotbar")
    bool RequestSelectHotbar(int32 Index);

    // Drops an amount from the selected hotbar item. Returns true if item was droppable and request dispatched.
    UFUNCTION(BlueprintCallable, Category="Inventory")
    bool DropSelectedHotbarItem(int32 Amount = 1);

    // Drops an amount from any slot. Returns true if item was droppable and request dispatched.
    UFUNCTION(BlueprintCallable, Category="Inventory")
    bool DropSlotItem(EInventoryContainer Container, int32 SlotIndex, int32 Amount = 1);

    // Requests executing selected hotbar item. Returns true if valid item was selected.
    UFUNCTION(BlueprintCallable, Category="Inventory|Hotbar", meta=(DisplayName="Request Execute Selected Hotbar Item"))
    bool RequestExecuteSelectedHotbarItem();

    // Requests using selected hotbar item (alias for RequestExecuteSelectedHotbarItem).
    UFUNCTION(BlueprintCallable, Category="Inventory|Hotbar")
    bool RequestUseSelectedHotbarItem();

    // Requests executing item from specific container and slot. Returns true if slot had valid item.
    UFUNCTION(BlueprintCallable, Category="Inventory|Action", meta=(DisplayName="Request Execute Slot Item"))
    bool RequestExecuteSlotItem(EInventoryContainer Container, int32 SlotIndex);

    // Requests using item from specific container and slot. Returns true if slot had valid item.
    UFUNCTION(BlueprintCallable, Category="Inventory|Action")
    bool RequestUseSlotItem(EInventoryContainer Container, int32 SlotIndex);

    // Requests dropping an amount from selected hotbar item. Returns true if droppable item was present.
    UFUNCTION(BlueprintCallable, Category="Inventory")
    bool RequestDropSelected(int32 Amount = 1);

    // Tosses the selected hotbar item forward with physics impulse. Returns true if an item was tossed.
    UFUNCTION(BlueprintCallable, Category="Inventory|Action")
    bool TossSelectedItem(float ForwardForce = 500.f, int32 Amount = 1);

    // --- Quick Transfer & Slot Manipulation ---

    // Quick-transfers item between Hotbar <-> Inventory (Shift-click transfer). Returns true if transfer initiated.
    UFUNCTION(BlueprintCallable, Category="Inventory|Action")
    bool QuickTransferSlot(EInventoryContainer FromContainer, int32 FromIndex);

    // Swaps two slots. Returns true if request initiated.
    UFUNCTION(BlueprintCallable, Category="Inventory|Action")
    bool SwapSlots(EInventoryContainer ContainerA, int32 IndexA, EInventoryContainer ContainerB, int32 IndexB);

    // Requests moving/stacking items between slots. Returns true if slots were valid.
    UFUNCTION(BlueprintCallable, Category="Inventory")
    bool RequestMove(EInventoryContainer FromContainer, int32 FromIndex, EInventoryContainer ToContainer, int32 ToIndex);

    // Requests dropping an item from a slot into the world. Returns true if slot had droppable item.
    UFUNCTION(BlueprintCallable, Category="Inventory")
    bool RequestDrop(EInventoryContainer Container, int32 SlotIndex, int32 Amount);

    // Requests dropping all items in a container into the world. Returns true if container had items.
    UFUNCTION(BlueprintCallable, Category="Inventory")
    bool RequestDropAll(EInventoryContainer Container);

    // Requests sorting all items in a container. Returns true if container had items.
    UFUNCTION(BlueprintCallable, Category="Inventory")
    bool RequestSort(EInventoryContainer Container);

    // Requests splitting a stack into a target empty slot. Returns true if split request initiated.
    UFUNCTION(BlueprintCallable, Category="Inventory")
    bool RequestSplit(EInventoryContainer Container, int32 FromIndex, int32 ToIndex, int32 Amount);

    // Clears a specific slot completely. Callable on Client or Server. Returns true if slot was cleared or request dispatched.
    UFUNCTION(BlueprintCallable, Category="Inventory|Action")
    bool ClearSlot(EInventoryContainer Container, int32 Index);

    // Clears the active selected hotbar slot. Callable on Client or Server.
    UFUNCTION(BlueprintCallable, Category="Inventory|Hotbar")
    bool ClearSelectedHotbarSlot();

    // Clears all slots in a container (Server only). Returns true if any slot was cleared.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Inventory|Admin")
    bool ClearContainer(EInventoryContainer Container);

    // --- Crafting & Recipes ---

    // Checks if the player owns all required ingredients in sufficient quantities
    UFUNCTION(BlueprintPure, Category="Inventory|Crafting")
    bool CanCraftRecipe(const TArray<UInventoryItemData*>& Ingredients, const TArray<int32>& Quantities) const;

    // Requests crafting: verifies ingredients, removes them, and grants result item. Returns true if ingredients are satisfied and request initiated.
    UFUNCTION(BlueprintCallable, Category="Inventory|Crafting")
    bool RequestCraft(const TArray<UInventoryItemData*>& Ingredients, const TArray<int32>& Quantities, UInventoryItemData* ResultItem, int32 ResultQuantity = 1);

    // Spawns a world item actor with optional launch velocity
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Inventory|World")
    AWorldInventoryItem* SpawnItemInWorld(UInventoryItemData* ItemData, int32 Quantity, FVector Location, FRotator Rotation, FVector Velocity = FVector::ZeroVector);

    // --- Authority Add/Remove ---

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Inventory")
    int32 AddItem(UInventoryItemData* ItemData, int32 Quantity, EInventoryContainer PreferredContainer = EInventoryContainer::Hotbar);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Inventory")
    int32 AddItemToContainer(EInventoryContainer Container, UInventoryItemData* ItemData, int32 Quantity);

    // Removes Quantity of ItemData across containers. Callable on Client or Server.
    UFUNCTION(BlueprintCallable, Category="Inventory")
    int32 RemoveItem(UInventoryItemData* ItemData, int32 Quantity);

    // Helper to find Owner's Ability System Component (checks Owner Pawn, Character, and PlayerState)
    UFUNCTION(BlueprintPure, Category="Inventory|Ability")
    UAbilitySystemComponent* GetOwnerAbilitySystemComponent() const;

    void NotifyInventoryChanged();

protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION()
    void OnRep_SelectedHotbarIndex();

    UPROPERTY()
    FGameplayAbilitySpecHandle ActiveSelectedAbilityHandle;

    UPROPERTY(Transient)
    TObjectPtr<UInventoryItemData> LastSelectedItemData = nullptr;

    UPROPERTY(Transient)
    int32 LastSelectedSlotIndex = INDEX_NONE;

    void UpdateActiveHotbarAbility();

    bool ExecuteItemEffects(UInventoryItemData* ItemData, EInventoryContainer Container, int32 SlotIndex);
    void NotifyItemSelected(UInventoryItemData* ItemData);

    UFUNCTION(Server, Reliable)
    void Server_TryPickupActor(AActor* TargetActor);

    UFUNCTION(Server, Reliable)
    void Server_RequestPickup(AWorldInventoryItem* WorldItem);

    UFUNCTION(Server, Reliable)
    void Server_UseSelectedHotbarItem();

    UFUNCTION(Server, Reliable)
    void Server_UseSlotItem(EInventoryContainer Container, int32 SlotIndex);

    UFUNCTION(Server, Reliable)
    void Server_RequestMove(EInventoryContainer FromContainer, int32 FromIndex, EInventoryContainer ToContainer, int32 ToIndex);

    UFUNCTION(Server, Reliable)
    void Server_RequestDrop(EInventoryContainer Container, int32 SlotIndex, int32 Amount);

    UFUNCTION(Server, Reliable)
    void Server_RequestDropAll(EInventoryContainer Container);

    UFUNCTION(Server, Reliable)
    void Server_RequestSort(EInventoryContainer Container);

    UFUNCTION(Server, Reliable)
    void Server_RequestSplit(EInventoryContainer Container, int32 FromIndex, int32 ToIndex, int32 Amount);

    UFUNCTION(Server, Reliable)
    void Server_RequestSelectHotbar(int32 Index);

    UFUNCTION(Server, Reliable)
    void Server_QuickTransferSlot(EInventoryContainer FromContainer, int32 FromIndex);

    UFUNCTION(Server, Reliable)
    void Server_RequestCraft(const TArray<UInventoryItemData*>& Ingredients, const TArray<int32>& Quantities, UInventoryItemData* ResultItem, int32 ResultQuantity);

    UFUNCTION(Server, Reliable)
    void Server_TossSelectedItem(float ForwardForce, int32 Amount);

    UFUNCTION(Server, Reliable)
    void Server_ConsumeSlotItem(EInventoryContainer Container, int32 SlotIndex, int32 Amount);

    UFUNCTION(Server, Reliable)
    void Server_ConsumeSelectedHotbarItem(int32 Amount);

    UFUNCTION(Server, Reliable)
    void Server_ClearSlot(EInventoryContainer Container, int32 Index);

    UFUNCTION(Server, Reliable)
    void Server_RemoveItem(UInventoryItemData* ItemData, int32 Quantity);

private:
    FInventoryArray& GetArray(EInventoryContainer Container);
    const FInventoryArray& GetArray(EInventoryContainer Container) const;
    FInventorySlot* FindSlot(EInventoryContainer Container, int32 Index);
    const FInventorySlot* FindSlot(EInventoryContainer Container, int32 Index) const;
    void InitializeArray(FInventoryArray& Array, int32 Count);
    bool IsValidSlot(EInventoryContainer Container, int32 Index) const;
    bool MoveItems(EInventoryContainer FromContainer, int32 FromIndex, EInventoryContainer ToContainer, int32 ToIndex);
    AWorldInventoryItem* SpawnDroppedItem(UInventoryItemData* ItemData, int32 Quantity);
};
