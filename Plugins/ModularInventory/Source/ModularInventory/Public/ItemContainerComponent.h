#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "ItemTypes.h"
#include "ItemContainerComponent.generated.h"

class UItemDefinition;
class AItemPickup;
class AController;

/** A slot's contents changed. Fires on the server AND on clients. Item is null when the slot became empty. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnItemSlotChanged, int32, SlotIndex, UItemDefinition*, Item, int32, Count);

/** Something in the container changed (fires once per change batch). Handy for "refresh everything". */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnItemContainerChanged);

/** The selected slot changed (hotbar style containers). Fires on server and owning client. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemSelectionChanged, int32, OldSlot, int32, NewSlot);

/** An item was used. SERVER ONLY. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnItemUsed, UItemDefinition*, Item, int32, SlotIndex, AActor*, User);

/**
 * The one component of this plugin. It holds N slots of items.
 *
 * Add as many as you want to any actor: a backpack and a hotbar on the player, a chest, a vendor, an equipment slot.
 * They all behave the same - you configure them in the Details panel.
 *
 * Networking: the server owns the data. Functions marked "Client or Server" can be called from either side;
 * on a client they are sent to the server automatically. Functions marked "Server only" do nothing on clients.
 */
UCLASS(ClassGroup = (Inventory), Blueprintable, meta = (BlueprintSpawnableComponent, DisplayName = "Item Container"))
class MODULARINVENTORY_API UItemContainerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UItemContainerComponent();

    // ------------------------------------------------------------------ Setup (Details panel)

    /** Name used to find this container, e.g. "Backpack", "Hotbar", "Chest". */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container")
    FName ContainerName = FName(TEXT("Inventory"));

    /** Optional title shown by the container widget. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container")
    FText ContainerTitle;

    /** How many slots at start. Change later with Set Num Slots. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container", meta = (ClampMin = "1"))
    int32 NumSlots = 20;

    /** Owner Only = private (backpack/hotbar). Everyone = shared (chest). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Container")
    EItemContainerReplication ReplicationMode = EItemContainerReplication::OwnerOnly;

    /** Items placed here when the game starts (server). Great for chests and starter kits. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Container")
    TArray<FItemStack> StartingItems;

    /** If not empty, only items with at least one of these tags can enter. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container|Rules")
    FGameplayTagContainer AllowedItemTags;

    /** Items with any of these tags can never enter. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container|Rules")
    FGameplayTagContainer BlockedItemTags;

    /** Merge new items into existing stacks. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container|Rules")
    bool bAutoStack = true;

    /** Keep a "selected slot" (hotbar behaviour). Turn on for a hotbar, leave off for everything else. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Container|Rules")
    bool bAllowSelection = false;

    /** Items can be dropped into the world from this container. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container|Rules")
    bool bAllowDropping = true;

    /** Let OTHER players take/put items here when they are close (chests). Leave off for private containers. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container|Access")
    bool bAllowExternalAccess = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container|Access", meta = (EditCondition = "bAllowExternalAccess", ClampMin = "0"))
    float MaxAccessDistance = 350.f;

    /** How far an item can be and still be picked up by this container. 0 = no limit. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container|Pickup", meta = (ClampMin = "0"))
    float MaxPickupDistance = 400.f;

    /** Pickup actor used when dropping, unless the item defines its own. Empty = default Item Pickup. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container|Pickup")
    TSubclassOf<AItemPickup> DropClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container|Pickup")
    float DropDistance = 120.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container|Pickup")
    float DropSpeed = 250.f;

    // ------------------------------------------------------------------ Events

    UPROPERTY(BlueprintAssignable, Category = "Container|Events")
    FOnItemSlotChanged OnSlotChanged;

    UPROPERTY(BlueprintAssignable, Category = "Container|Events")
    FOnItemContainerChanged OnContainerChanged;

    UPROPERTY(BlueprintAssignable, Category = "Container|Events")
    FOnItemSelectionChanged OnSelectionChanged;

    /** Server only. Hook gameplay here (heal the player, spawn a weapon...). */
    UPROPERTY(BlueprintAssignable, Category = "Container|Events")
    FOnItemUsed OnItemUsed;

    // ------------------------------------------------------------------ Queries (safe anywhere)

    UFUNCTION(BlueprintPure, Category = "Container|Query")
    int32 GetNumSlots() const { return GetSlots().Num(); }

    UFUNCTION(BlueprintPure, Category = "Container|Query")
    bool IsValidSlot(int32 SlotIndex) const { return GetSlots().IsValidIndex(SlotIndex); }

    /** Returns an empty slot if the index is invalid. */
    UFUNCTION(BlueprintPure, Category = "Container|Query")
    FItemSlot GetSlot(int32 SlotIndex) const;

    UFUNCTION(BlueprintPure, Category = "Container|Query")
    TArray<FItemSlot> GetAllSlots() const { return GetSlots(); }

    UFUNCTION(BlueprintPure, Category = "Container|Query")
    bool IsSlotEmpty(int32 SlotIndex) const;

    UFUNCTION(BlueprintPure, Category = "Container|Query")
    int32 GetItemCount(UItemDefinition* Item) const;

    UFUNCTION(BlueprintPure, Category = "Container|Query")
    bool HasItem(UItemDefinition* Item, int32 Count = 1) const;

    /** True if the container holds ALL of these (use for recipe costs). */
    UFUNCTION(BlueprintPure, Category = "Container|Query")
    bool HasItems(const TArray<FItemStack>& Stacks) const;

    /** First slot holding Item, or -1. */
    UFUNCTION(BlueprintPure, Category = "Container|Query")
    int32 FindSlotWithItem(UItemDefinition* Item) const;

    /** First empty slot, or -1. */
    UFUNCTION(BlueprintPure, Category = "Container|Query")
    int32 FindEmptySlot() const;

    UFUNCTION(BlueprintPure, Category = "Container|Query")
    int32 GetUsedSlotCount() const;

    UFUNCTION(BlueprintPure, Category = "Container|Query")
    bool IsFull() const { return FindEmptySlot() == INDEX_NONE; }

    /** How many more of this item would fit right now. */
    UFUNCTION(BlueprintPure, Category = "Container|Query")
    int32 GetFreeSpaceFor(UItemDefinition* Item) const;

    UFUNCTION(BlueprintPure, Category = "Container|Query")
    bool CanFit(UItemDefinition* Item, int32 Count = 1) const { return GetFreeSpaceFor(Item) >= Count; }

    /** Override in Blueprint for custom rules (level requirements, weight...). Default checks the tag rules. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Container|Query")
    bool CanAcceptItem(UItemDefinition* Item) const;
    virtual bool CanAcceptItem_Implementation(UItemDefinition* Item) const;

    /** Override in Blueprint for locked chests etc. Who is null when game code on the server asks. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Container|Query")
    bool CanBeAccessedBy(AController* Who) const;
    virtual bool CanBeAccessedBy_Implementation(AController* Who) const;

    // ------------------------------------------------------------------ Server only: add / remove

    /** Server only. Returns how many did NOT fit (0 = everything was added). */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Container|Server")
    int32 AddItem(UItemDefinition* Item, int32 Count = 1);

    /** Server only. Puts items in one specific slot. Returns the leftover. */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Container|Server")
    int32 AddItemToSlot(UItemDefinition* Item, int32 Count, int32 SlotIndex);

    /** Server only. Removes up to Count of Item from anywhere. Returns how many were removed. */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Container|Server")
    int32 RemoveItem(UItemDefinition* Item, int32 Count = 1);

    /** Server only. Removes from one slot. Count 0 = the whole stack. Returns how many were removed. */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Container|Server")
    int32 RemoveFromSlot(int32 SlotIndex, int32 Count = 0);

    /** Server only. Removes ALL of these (recipe costs) or nothing at all. Returns true if everything was removed. */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Container|Server")
    bool RemoveItems(const TArray<FItemStack>& Stacks);

    /** Server only. Empties every slot. */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Container|Server")
    void ClearAll();

    /** Server only. Grow or shrink. Items in removed slots move to free slots or drop in the world. */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Container|Server")
    void SetNumSlots(int32 NewNumSlots);

    // ------------------------------------------------------------------ Client or Server: player actions

    /**
     * Move items to another slot or another container. Works between ANY two containers.
     * ToContainer empty = this container. ToSlot -1 = automatic (stack, then first free slot).
     * Count 0 = whole stack. Dropping on an occupied slot stacks or swaps.
     * Returns true if the request was valid (on a client: if it was sent).
     */
    UFUNCTION(BlueprintCallable, Category = "Container|Actions")
    bool MoveItem(int32 FromSlot, UItemContainerComponent* ToContainer = nullptr, int32 ToSlot = -1, int32 Count = 0);

    /** Drop items from a slot into the world. Count 0 = whole stack. */
    UFUNCTION(BlueprintCallable, Category = "Container|Actions")
    bool DropItem(int32 SlotIndex, int32 Count = 0);

    /** Use the item in a slot (runs its Use Action, fires On Item Used, consumes if set). */
    UFUNCTION(BlueprintCallable, Category = "Container|Actions")
    bool UseItem(int32 SlotIndex);

    /** Merge stacks and sort by name. */
    UFUNCTION(BlueprintCallable, Category = "Container|Actions")
    bool SortItems();

    /** Pick up a world item (Item Pickup actor). Call this from your interact input with the actor you traced. */
    UFUNCTION(BlueprintCallable, Category = "Container|Actions")
    bool PickupActor(AActor* Target);

    /** Pick up the closest world item within Radius. */
    UFUNCTION(BlueprintCallable, Category = "Container|Actions")
    bool PickupNearby(float Radius = 250.f);

    // ------------------------------------------------------------------ Selection (only if Allow Selection is on)

    UFUNCTION(BlueprintCallable, Category = "Container|Selection")
    bool SelectSlot(int32 SlotIndex);

    UFUNCTION(BlueprintCallable, Category = "Container|Selection")
    bool SelectNext();

    UFUNCTION(BlueprintCallable, Category = "Container|Selection")
    bool SelectPrevious();

    UFUNCTION(BlueprintPure, Category = "Container|Selection")
    int32 GetSelectedSlot() const { return SelectedSlot; }

    UFUNCTION(BlueprintPure, Category = "Container|Selection")
    FItemSlot GetSelectedItem() const { return GetSlot(SelectedSlot); }

    UFUNCTION(BlueprintCallable, Category = "Container|Selection")
    bool UseSelectedItem() { return UseItem(SelectedSlot); }

    UFUNCTION(BlueprintCallable, Category = "Container|Selection")
    bool DropSelectedItem(int32 Count = 0) { return DropItem(SelectedSlot, Count); }

    // ------------------------------------------------------------------ Helpers

    /** The controller that owns this container (found through pawn / player state / controller). Null for chests. */
    AController* GetOwningController() const;

    /** The pawn (or actor) that acts for this container: used for distances and drop positions. */
    AActor* GetReferenceActor() const;

protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // Two arrays, one active: they exist so the replication condition can differ per mode.
    UPROPERTY(ReplicatedUsing = OnRep_OwnerSlots)
    TArray<FItemSlot> OwnerSlots;

    UPROPERTY(ReplicatedUsing = OnRep_SharedSlots)
    TArray<FItemSlot> SharedSlots;

    UPROPERTY(ReplicatedUsing = OnRep_SelectedSlot)
    int32 SelectedSlot = 0;

    UPROPERTY(Transient)
    TArray<FItemSlot> ClientCache;

    UFUNCTION()
    void OnRep_OwnerSlots() { HandleSlotsReplicated(); }

    UFUNCTION()
    void OnRep_SharedSlots() { HandleSlotsReplicated(); }

    UFUNCTION()
    void OnRep_SelectedSlot(int32 OldSlot);

    // --- RPCs (the container the local player owns "carries" the request) ---

    UFUNCTION(Server, Reliable)
    void Server_Transfer(UItemContainerComponent* Source, int32 FromSlot, UItemContainerComponent* Dest, int32 ToSlot, int32 Count);

    UFUNCTION(Server, Reliable)
    void Server_Drop(UItemContainerComponent* Target, int32 SlotIndex, int32 Count);

    UFUNCTION(Server, Reliable)
    void Server_Use(UItemContainerComponent* Target, int32 SlotIndex);

    UFUNCTION(Server, Reliable)
    void Server_Sort(UItemContainerComponent* Target);

    UFUNCTION(Server, Reliable)
    void Server_PickupActor(AActor* Target);

    UFUNCTION(Server, Reliable)
    void Server_PickupNearby(float Radius);

    UFUNCTION(Server, Reliable)
    void Server_SelectSlot(int32 SlotIndex);

private:
    const TArray<FItemSlot>& GetSlots() const { return ReplicationMode == EItemContainerReplication::Everyone ? SharedSlots : OwnerSlots; }
    TArray<FItemSlot>& MutableSlots() { return ReplicationMode == EItemContainerReplication::Everyone ? SharedSlots : OwnerSlots; }

    bool HasServerAuthority() const;
    UItemContainerComponent* FindRpcCarrier();

    void InitializeSlots();
    void HandleSlotsReplicated();
    void BroadcastSlot(int32 Index, const FItemSlot& Slot);
    void NotifyChanged(const TArray<int32>& Changed);

    int32 AddInternal(UItemDefinition* Item, int32 Count, TArray<int32>& Changed, int32 ExcludeSlot);
    int32 RemoveFromSlotInternal(int32 SlotIndex, int32 Count, TArray<int32>& Changed);

    static bool TransferInternal(UItemContainerComponent* Src, int32 FromSlot, UItemContainerComponent* Dst, int32 ToSlot, int32 Count, AController* Who);
    bool DropInternal(int32 SlotIndex, int32 Count, AController* Who);
    bool UseInternal(int32 SlotIndex, AController* Who);
    void SortInternal();
    bool PickupInternal(AItemPickup* Pickup);
    bool PickupNearbyInternal(float Radius);
    bool SelectInternal(int32 SlotIndex);
    void SpawnDrop(UItemDefinition* Item, int32 Count);
};
