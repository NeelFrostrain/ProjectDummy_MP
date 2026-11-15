#include "InventoryComponent.h"
#include "InventoryItemData.h"
#include "InventoryItemGameplayAbility.h"
#include "WorldInventoryItem.h"
#include "InventorySlotWidget.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "CollisionQueryParams.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayAbilitySpec.h"
#include "NativeGameplayTags.h"

namespace InventoryItemEventTags
{
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Selected, "Inventory.Event.Selected", "Fired when an item is selected in the hotbar");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Unselected, "Inventory.Event.Unselected", "Fired when an item is unselected from the hotbar");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Used, "Inventory.Event.Used", "Fired when an item is used/executed");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(PickedUp, "Inventory.Event.PickedUp", "Fired when an item is picked up from the world");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dropped, "Inventory.Event.Dropped", "Fired when an item is dropped into the world");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Added, "Inventory.Event.Added", "Fired when an item is added to a container");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Removed, "Inventory.Event.Removed", "Fired when an item is removed from a container");

    // Aliases for backward compatibility
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Equipped, "Inventory.Event.Equipped", "Alias for Selected");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Unequipped, "Inventory.Event.Unequipped", "Alias for Unselected");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Consumed, "Inventory.Event.Consumed", "Alias for Used/Removed");
}

void FInventoryArray::PostReplicatedAdd(const TArrayView<int32>&, int32) { if (OwnerComponent) OwnerComponent->NotifyInventoryChanged(); }
void FInventoryArray::PostReplicatedChange(const TArrayView<int32>&, int32) { if (OwnerComponent) OwnerComponent->NotifyInventoryChanged(); }
void FInventoryArray::PostReplicatedReceive(const FFastArraySerializer::FPostReplicatedReceiveParameters&) { if (OwnerComponent) OwnerComponent->NotifyInventoryChanged(); }

static UInventoryItemGameplayAbility* FindOrGrantItemAbility(UAbilitySystemComponent* AbilitySystem, UInventoryItemData* ItemData, FGameplayAbilitySpecHandle& OutHandle)
{
    AActor* AbilityOwner = AbilitySystem ? AbilitySystem->GetOwner() : nullptr;
    if (!AbilityOwner || !AbilityOwner->HasAuthority() || !ItemData || !ItemData->AbilityClass) return nullptr;

    for (const FGameplayAbilitySpec& ExistingSpec : AbilitySystem->GetActivatableAbilities())
    {
        if (ExistingSpec.Ability && ExistingSpec.Ability->GetClass() == ItemData->AbilityClass.Get() &&
            ExistingSpec.SourceObject.Get() == ItemData)
        {
            OutHandle = ExistingSpec.Handle;
            break;
        }
    }

    if (!OutHandle.IsValid())
    {
        const FGameplayAbilitySpec NewSpec(ItemData->AbilityClass, 1, INDEX_NONE, ItemData);
        OutHandle = AbilitySystem->GiveAbility(NewSpec);
    }

    FGameplayAbilitySpec* Spec = AbilitySystem->FindAbilitySpecFromHandle(OutHandle);
    return Spec ? Cast<UInventoryItemGameplayAbility>(Spec->GetPrimaryInstance()) : nullptr;
}

UInventoryComponent::UInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
    InventorySlots.OwnerComponent = this;
    HotbarSlots.OwnerComponent = this;
}

void UInventoryComponent::BeginPlay()
{
    Super::BeginPlay();
    InventorySlots.OwnerComponent = this;
    HotbarSlots.OwnerComponent = this;
    if (GetOwner() && GetOwner()->HasAuthority())
    {
        InitializeArray(InventorySlots, InventorySlotCount);
        InitializeArray(HotbarSlots, HotbarSlotCount);
        UpdateActiveHotbarAbility();
    }
}

void UInventoryComponent::InitializeArray(FInventoryArray& Array, int32 Count)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || Array.Slots.Num() > 0) return;
    for (int32 i = 0; i < Count; ++i)
    {
        FInventorySlot& Slot = Array.Slots.AddDefaulted_GetRef();
        Slot.SlotIndex = i;
        Slot.ItemData = nullptr;
        Slot.Quantity = 0;
        Array.MarkItemDirty(Slot);
    }
}

FInventoryArray& UInventoryComponent::GetArray(EInventoryContainer Container) { return Container == EInventoryContainer::Hotbar ? HotbarSlots : InventorySlots; }
const FInventoryArray& UInventoryComponent::GetArray(EInventoryContainer Container) const { return Container == EInventoryContainer::Hotbar ? HotbarSlots : InventorySlots; }

FInventorySlot* UInventoryComponent::FindSlot(EInventoryContainer Container, int32 Index)
{
    FInventoryArray& Array = GetArray(Container);
    return Array.Slots.IsValidIndex(Index) ? &Array.Slots[Index] : nullptr;
}

const FInventorySlot* UInventoryComponent::FindSlot(EInventoryContainer Container, int32 Index) const
{
    const FInventoryArray& Array = GetArray(Container);
    return Array.Slots.IsValidIndex(Index) ? &Array.Slots[Index] : nullptr;
}

bool UInventoryComponent::IsValidSlot(EInventoryContainer Container, int32 Index) const
{
    const FInventoryArray& Array = GetArray(Container);
    if (Array.Slots.Num() > 0)
    {
        return Array.Slots.IsValidIndex(Index);
    }
    return Index >= 0 && Index < (Container == EInventoryContainer::Hotbar ? HotbarSlotCount : InventorySlotCount);
}

bool UInventoryComponent::GetSlot(EInventoryContainer Container, int32 Index, UInventoryItemData*& OutItemData, int32& OutQuantity) const
{
    OutItemData = nullptr;
    OutQuantity = 0;
    const FInventorySlot* Slot = FindSlot(Container, Index);
    if (!Slot) return false;
    OutItemData = Slot->ItemData;
    OutQuantity = Slot->Quantity;
    return true;
}

bool UInventoryComponent::GetSlotStruct(EInventoryContainer Container, int32 Index, FInventorySlot& OutSlot) const
{
    const FInventorySlot* Slot = FindSlot(Container, Index);
    if (!Slot) return false;
    OutSlot = *Slot;
    return true;
}

TArray<FInventorySlot> UInventoryComponent::GetAllSlots(EInventoryContainer Container) const
{
    return GetArray(Container).Slots;
}

int32 UInventoryComponent::GetSlotCount(EInventoryContainer Container) const
{
    const FInventoryArray& Array = GetArray(Container);
    if (Array.Slots.Num() > 0)
    {
        return Array.Slots.Num();
    }
    return (Container == EInventoryContainer::Hotbar) ? HotbarSlotCount : InventorySlotCount;
}

void UInventoryComponent::SetSlotCount(EInventoryContainer Container, int32 NewCount)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || NewCount <= 0) return;

    if (Container == EInventoryContainer::Hotbar)
    {
        HotbarSlotCount = NewCount;
    }
    else
    {
        InventorySlotCount = NewCount;
    }

    FInventoryArray& Array = GetArray(Container);
    const int32 CurrentNum = Array.Slots.Num();

    if (NewCount > CurrentNum)
    {
        for (int32 i = CurrentNum; i < NewCount; ++i)
        {
            FInventorySlot& Slot = Array.Slots.AddDefaulted_GetRef();
            Slot.SlotIndex = i;
            Slot.ItemData = nullptr;
            Slot.Quantity = 0;
            Array.MarkItemDirty(Slot);
        }
    }
    else if (NewCount < CurrentNum)
    {
        for (int32 i = CurrentNum - 1; i >= NewCount; --i)
        {
            FInventorySlot& Slot = Array.Slots[i];
            if (Slot.ItemData && Slot.Quantity > 0)
            {
                SpawnDroppedItem(Slot.ItemData, Slot.Quantity);
            }
            Array.Slots.RemoveAt(i);
        }
        Array.MarkArrayDirty();
    }

    if (Container == EInventoryContainer::Hotbar && SelectedHotbarIndex >= NewCount)
    {
        SelectedHotbarIndex = FMath::Clamp(SelectedHotbarIndex, 0, NewCount - 1);
        OnRep_SelectedHotbarIndex();
    }

    NotifyInventoryChanged();
}

TArray<UInventorySlotWidget*> UInventoryComponent::SetupHotbarSlotWidgets(const TArray<UInventorySlotWidget*>& InWidgets)
{
    return SetupSlotWidgets(EInventoryContainer::Hotbar, InWidgets);
}

TArray<UInventorySlotWidget*> UInventoryComponent::SetupSlotWidgets(EInventoryContainer Container, const TArray<UInventorySlotWidget*>& InWidgets)
{
    if (InWidgets.IsEmpty()) return InWidgets;

    // If on authority and slot count doesn't match the widget array size, auto-sync slot count!
    if (GetOwner() && GetOwner()->HasAuthority())
    {
        if (GetSlotCount(Container) != InWidgets.Num())
        {
            SetSlotCount(Container, InWidgets.Num());
        }
    }

    for (int32 i = 0; i < InWidgets.Num(); ++i)
    {
        if (UInventorySlotWidget* SlotWidget = InWidgets[i])
        {
            SlotWidget->SetupSlot(this, Container, i);
        }
    }

    return InWidgets;
}

int32 UInventoryComponent::GetUsedSlotCount(EInventoryContainer Container) const
{
    int32 Count = 0;
    for (const FInventorySlot& Slot : GetArray(Container).Slots)
    {
        if (Slot.ItemData && Slot.Quantity > 0) ++Count;
    }
    return Count;
}

bool UInventoryComponent::GetSelectedHotbarItem(UInventoryItemData*& OutItemData, int32& OutQuantity) const
{
    return GetSlot(EInventoryContainer::Hotbar, SelectedHotbarIndex, OutItemData, OutQuantity);
}

bool UInventoryComponent::GetSelectedHotbarSlot(FInventorySlot& OutSlot) const
{
    return GetSlotStruct(EInventoryContainer::Hotbar, SelectedHotbarIndex, OutSlot);
}

bool UInventoryComponent::IsHotbarSelectedSlotEmpty() const
{
    const FInventorySlot* Slot = FindSlot(EInventoryContainer::Hotbar, SelectedHotbarIndex);
    return !Slot || !Slot->ItemData || Slot->Quantity <= 0;
}

bool UInventoryComponent::HasItem(UInventoryItemData* ItemData, int32 RequiredQuantity) const
{
    return GetTotalItemCount(ItemData) >= RequiredQuantity;
}

int32 UInventoryComponent::GetTotalItemCount(UInventoryItemData* ItemData) const
{
    return GetItemCountInContainer(EInventoryContainer::Hotbar, ItemData) + GetItemCountInContainer(EInventoryContainer::Inventory, ItemData);
}

int32 UInventoryComponent::GetItemCountInContainer(EInventoryContainer Container, UInventoryItemData* ItemData) const
{
    if (!ItemData) return 0;
    int32 Count = 0;
    for (const FInventorySlot& Slot : GetArray(Container).Slots)
    {
        if (Slot.ItemData == ItemData && Slot.Quantity > 0)
        {
            Count += Slot.Quantity;
        }
    }
    return Count;
}

int32 UInventoryComponent::FindFirstSlotWithItem(EInventoryContainer Container, UInventoryItemData* ItemData) const
{
    if (!ItemData) return INDEX_NONE;
    const FInventoryArray& Array = GetArray(Container);
    for (int32 i = 0; i < Array.Slots.Num(); ++i)
    {
        if (Array.Slots[i].ItemData == ItemData && Array.Slots[i].Quantity > 0)
        {
            return i;
        }
    }
    return INDEX_NONE;
}

int32 UInventoryComponent::FindFirstEmptySlot(EInventoryContainer Container) const
{
    const FInventoryArray& Array = GetArray(Container);
    for (int32 i = 0; i < Array.Slots.Num(); ++i)
    {
        if (!Array.Slots[i].ItemData || Array.Slots[i].Quantity <= 0)
        {
            return i;
        }
    }
    return INDEX_NONE;
}

bool UInventoryComponent::HasEmptySlot(EInventoryContainer Container) const
{
    return FindFirstEmptySlot(Container) != INDEX_NONE;
}

bool UInventoryComponent::IsSlotEmpty(EInventoryContainer Container, int32 SlotIndex) const
{
    const FInventorySlot* Slot = FindSlot(Container, SlotIndex);
    return !Slot || !Slot->ItemData || Slot->Quantity <= 0;
}

bool UInventoryComponent::CanAddItem(UInventoryItemData* ItemData, int32 Quantity, EInventoryContainer PreferredContainer) const
{
    if (!ItemData || Quantity <= 0) return false;
    const int32 FreePreferred = GetFreeSpaceForItem(ItemData, PreferredContainer);
    if (FreePreferred >= Quantity) return true;
    const EInventoryContainer Other = (PreferredContainer == EInventoryContainer::Hotbar) ? EInventoryContainer::Inventory : EInventoryContainer::Hotbar;
    const int32 FreeOther = GetFreeSpaceForItem(ItemData, Other);
    return (FreePreferred + FreeOther) >= Quantity;
}

int32 UInventoryComponent::GetFreeSpaceForItem(UInventoryItemData* ItemData, EInventoryContainer Container) const
{
    if (!ItemData) return 0;
    const int32 MaxStack = FMath::Max(1, ItemData->MaxStackSize);
    int32 FreeSpace = 0;
    for (const FInventorySlot& Slot : GetArray(Container).Slots)
    {
        if (!Slot.ItemData || Slot.Quantity <= 0)
        {
            FreeSpace += MaxStack;
        }
        else if (Slot.ItemData == ItemData && Slot.Quantity < MaxStack)
        {
            FreeSpace += (MaxStack - Slot.Quantity);
        }
    }
    return FreeSpace;
}

float UInventoryComponent::GetContainerFillRatio(EInventoryContainer Container) const
{
    const int32 Total = GetSlotCount(Container);
    if (Total <= 0) return 0.f;
    return (float)GetUsedSlotCount(Container) / (float)Total;
}

int32 UInventoryComponent::SelectNextHotbar()
{
    const int32 Count = GetSlotCount(EInventoryContainer::Hotbar);
    if (Count <= 0) return SelectedHotbarIndex;
    for (int32 Step = 1; Step <= Count; ++Step)
    {
        const int32 Next = (SelectedHotbarIndex + Step) % Count;
        const FInventorySlot* Slot = FindSlot(EInventoryContainer::Hotbar, Next);
        if (!Slot || !Slot->ItemData || CanEquipItem(Slot->ItemData))
        {
            if (RequestSelectHotbar(Next)) return Next;
        }
    }
    return SelectedHotbarIndex;
}

int32 UInventoryComponent::SelectPreviousHotbar()
{
    const int32 Count = GetSlotCount(EInventoryContainer::Hotbar);
    if (Count <= 0) return SelectedHotbarIndex;
    for (int32 Step = 1; Step <= Count; ++Step)
    {
        const int32 Prev = (SelectedHotbarIndex - Step + Count) % Count;
        const FInventorySlot* Slot = FindSlot(EInventoryContainer::Hotbar, Prev);
        if (!Slot || !Slot->ItemData || CanEquipItem(Slot->ItemData))
        {
            if (RequestSelectHotbar(Prev)) return Prev;
        }
    }
    return SelectedHotbarIndex;
}

bool UInventoryComponent::SelectHotbarSlot(int32 Index)
{
    return RequestSelectHotbar(Index);
}

bool UInventoryComponent::SelectHotbarItem(int32 Index)
{
    return RequestSelectHotbar(Index);
}

void UInventoryComponent::NotifyInventoryChanged()
{
    if (GetOwner() && GetOwner()->HasAuthority())
    {
        UpdateActiveHotbarAbility();
    }
    OnInventoryChanged.Broadcast();
}

void UInventoryComponent::BroadcastItemEvent(
    FGameplayTag EventTag,
    UInventoryItemData* InItemData,
    int32 InQuantity,
    EInventoryContainer InContainer,
    int32 InSlotIndex,
    AActor* InRelatedActor)
{
    const FGameplayTagContainer Tags = InItemData ? InItemData->ItemTags : FGameplayTagContainer::EmptyContainer;

    // 1. Unified dispatcher (all events)
    OnItemEvent.Broadcast(EventTag, InItemData, InQuantity, InContainer, InSlotIndex, Tags, InRelatedActor);

    // 2. Specific dispatchers with identical parameters
    if (EventTag == InventoryItemEventTags::Selected || EventTag == InventoryItemEventTags::Equipped)
    {
        OnItemSelected.Broadcast(InItemData, InQuantity, InContainer, InSlotIndex, Tags, InRelatedActor);
    }
    else if (EventTag == InventoryItemEventTags::Unselected || EventTag == InventoryItemEventTags::Unequipped)
    {
        OnItemUnselected.Broadcast(InItemData, InQuantity, InContainer, InSlotIndex, Tags, InRelatedActor);
    }
    else if (EventTag == InventoryItemEventTags::Used)
    {
        OnItemUsed.Broadcast(InItemData, InQuantity, InContainer, InSlotIndex, Tags, InRelatedActor);
    }
    else if (EventTag == InventoryItemEventTags::PickedUp)
    {
        OnItemPickedUp.Broadcast(InItemData, InQuantity, InContainer, InSlotIndex, Tags, InRelatedActor);
        OnItemAdded.Broadcast(InItemData, InQuantity, InContainer, InSlotIndex, Tags, InRelatedActor);
    }
    else if (EventTag == InventoryItemEventTags::Dropped)
    {
        OnItemDropped.Broadcast(InItemData, InQuantity, InContainer, InSlotIndex, Tags, InRelatedActor);
    }
    else if (EventTag == InventoryItemEventTags::Added)
    {
        OnItemAdded.Broadcast(InItemData, InQuantity, InContainer, InSlotIndex, Tags, InRelatedActor);
    }
    else if (EventTag == InventoryItemEventTags::Removed)
    {
        OnItemRemoved.Broadcast(InItemData, InQuantity, InContainer, InSlotIndex, Tags, InRelatedActor);
    }
}

void UInventoryComponent::OnRep_SelectedHotbarIndex()
{
    OnHotbarSelectionChanged.Broadcast(SelectedHotbarIndex);
    NotifyInventoryChanged();

    // On simulated/autonomous clients, broadcast selected/unselected events locally for cosmetic attachment / UI
    if (GetOwner() && !GetOwner()->HasAuthority())
    {
        const FInventorySlot* Slot = FindSlot(EInventoryContainer::Hotbar, SelectedHotbarIndex);
        UInventoryItemData* NewItemData = (Slot && Slot->Quantity > 0) ? Slot->ItemData.Get() : nullptr;

        if (LastSelectedItemData && (LastSelectedItemData != NewItemData || LastSelectedSlotIndex != SelectedHotbarIndex))
        {
            BroadcastItemEvent(InventoryItemEventTags::Unselected, LastSelectedItemData, 1, EInventoryContainer::Hotbar, LastSelectedSlotIndex, GetOwner());
            LastSelectedItemData = nullptr;
            LastSelectedSlotIndex = INDEX_NONE;
        }

        if (NewItemData && (LastSelectedItemData != NewItemData || LastSelectedSlotIndex != SelectedHotbarIndex))
        {
            LastSelectedItemData = NewItemData;
            LastSelectedSlotIndex = SelectedHotbarIndex;
            BroadcastItemEvent(InventoryItemEventTags::Selected, NewItemData, 1, EInventoryContainer::Hotbar, SelectedHotbarIndex, GetOwner());
        }
    }
}

// --- Ability System Component Resolution ---

UAbilitySystemComponent* UInventoryComponent::GetOwnerAbilitySystemComponent() const
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor) return nullptr;

    if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(OwnerActor))
    {
        if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
        {
            return ASC;
        }
    }

    if (UAbilitySystemComponent* ASC = OwnerActor->FindComponentByClass<UAbilitySystemComponent>())
    {
        return ASC;
    }

    if (APawn* Pawn = Cast<APawn>(OwnerActor))
    {
        if (APlayerState* PS = Pawn->GetPlayerState())
        {
            if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PS))
            {
                if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
                {
                    return ASC;
                }
            }
            if (UAbilitySystemComponent* ASC = PS->FindComponentByClass<UAbilitySystemComponent>())
            {
                return ASC;
            }
        }
    }

    return nullptr;
}

// --- Query Functions for Item Usage ---

bool UInventoryComponent::CanSelectItem(UInventoryItemData* ItemData) const
{
    if (!ItemData) return false;
    if (!ItemData->AbilityClass) return true;

    const UInventoryItemGameplayAbility* AbilityCDO = ItemData->AbilityClass->GetDefaultObject<UInventoryItemGameplayAbility>();
    return AbilityCDO && AbilityCDO->CanUseItemTags(ItemData->ItemTags);
}

bool UInventoryComponent::CanUseItem(UInventoryItemData* ItemData) const
{
    return ItemData && ItemData->AbilityClass && CanSelectItem(ItemData);
}

bool UInventoryComponent::CanUseSelectedHotbarItem() const
{
    const FInventorySlot* Slot = FindSlot(EInventoryContainer::Hotbar, SelectedHotbarIndex);
    if (!Slot || !Slot->ItemData || Slot->Quantity <= 0) return false;
    return CanUseItem(Slot->ItemData);
}

bool UInventoryComponent::CanUseSlotItem(EInventoryContainer Container, int32 SlotIndex) const
{
    const FInventorySlot* Slot = FindSlot(Container, SlotIndex);
    if (!Slot || !Slot->ItemData || Slot->Quantity <= 0) return false;
    return CanUseItem(Slot->ItemData);
}

// --- Hotbar Ability Activation & Deactivation ---

void UInventoryComponent::UpdateActiveHotbarAbility()
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;

    UAbilitySystemComponent* OwnerASC = GetOwnerAbilitySystemComponent();

    const FInventorySlot* Slot = FindSlot(EInventoryContainer::Hotbar, SelectedHotbarIndex);
    UInventoryItemData* NewItemData = (Slot && Slot->Quantity > 0) ? Slot->ItemData.Get() : nullptr;

    // 1. Check if unselect is needed for previous item
    if (LastSelectedItemData && (LastSelectedItemData != NewItemData || LastSelectedSlotIndex != SelectedHotbarIndex))
    {
        UInventoryItemData* OldItem = LastSelectedItemData;
        const int32 OldSlot = LastSelectedSlotIndex;
        const FGameplayTagContainer OldTags = OldItem->ItemTags;
        LastSelectedItemData = nullptr;
        LastSelectedSlotIndex = INDEX_NONE;

        if (ActiveSelectedAbilityHandle.IsValid())
        {
            if (OwnerASC)
            {
                if (FGameplayAbilitySpec* CurrentSpec = OwnerASC->FindAbilitySpecFromHandle(ActiveSelectedAbilityHandle))
                {
                    if (UInventoryItemGameplayAbility* PrevAbility = Cast<UInventoryItemGameplayAbility>(CurrentSpec->GetPrimaryInstance()))
                    {
                        PrevAbility->OnItemUnselected(OldItem, OldTags);
                    }
                }
                OwnerASC->CancelAbilityHandle(ActiveSelectedAbilityHandle);
            }
            ActiveSelectedAbilityHandle = FGameplayAbilitySpecHandle();
        }

        BroadcastItemEvent(InventoryItemEventTags::Unselected, OldItem, 1, EInventoryContainer::Hotbar, OldSlot, GetOwner());
    }

    // 2. If new slot has an item that can be selected
    if (NewItemData && CanSelectItem(NewItemData))
    {
        if (LastSelectedItemData != NewItemData || LastSelectedSlotIndex != SelectedHotbarIndex)
        {
            LastSelectedItemData = NewItemData;
            LastSelectedSlotIndex = SelectedHotbarIndex;

            if (NewItemData->AbilityClass && OwnerASC)
            {
                FGameplayAbilitySpecHandle NewHandle;
                if (UInventoryItemGameplayAbility* NewAbility = FindOrGrantItemAbility(OwnerASC, NewItemData, NewHandle))
                {
                    NewAbility->SourceInventoryComponent = this;
                    NewAbility->SourceItemData = NewItemData;
                    NewAbility->ItemData = NewItemData;
                    NewAbility->SourceContainer = EInventoryContainer::Hotbar;
                    NewAbility->SourceSlotIndex = SelectedHotbarIndex;

                    ActiveSelectedAbilityHandle = NewHandle;

                    // Activate ability upon selecting item
                    OwnerASC->TryActivateAbility(NewHandle);
                    const bool bSelectSuccess = NewAbility->OnItemSelected(NewItemData, NewItemData->ItemTags);
                    if (bSelectSuccess)
                    {
                        BroadcastItemEvent(InventoryItemEventTags::Selected, NewItemData, 1, EInventoryContainer::Hotbar, SelectedHotbarIndex, GetOwner());
                    }
                }
            }
            else
            {
                BroadcastItemEvent(InventoryItemEventTags::Selected, NewItemData, 1, EInventoryContainer::Hotbar, SelectedHotbarIndex, GetOwner());
            }
        }
        else if (ActiveSelectedAbilityHandle.IsValid() && OwnerASC)
        {
            // Same item already selected, ensure ability is active
            if (FGameplayAbilitySpec* Spec = OwnerASC->FindAbilitySpecFromHandle(ActiveSelectedAbilityHandle))
            {
                if (!Spec->IsActive())
                {
                    OwnerASC->TryActivateAbility(ActiveSelectedAbilityHandle);
                }
            }
        }
    }
}

// --- Item Execution ---

bool UInventoryComponent::ExecuteItemEffects(UInventoryItemData* ItemData, EInventoryContainer Container, int32 SlotIndex)
{
    if (!ItemData || !GetOwner() || !GetOwner()->HasAuthority()) return false;
    FInventorySlot* Slot = FindSlot(Container, SlotIndex);
    if (!Slot || Slot->ItemData != ItemData || Slot->Quantity <= 0 || !CanUseItem(ItemData)) return false;

    UAbilitySystemComponent* OwnerASC = GetOwnerAbilitySystemComponent();
    if (!OwnerASC)
    {
        if (ItemData->bConsumeOnUse)
        {
            ConsumeSlotItem(Container, SlotIndex, 1);
            BroadcastItemEvent(InventoryItemEventTags::Used, ItemData, 1, Container, SlotIndex, GetOwner());
            return true;
        }
        UE_LOG(LogTemp, Warning, TEXT("InventoryComponent: Cannot use item '%s' because its owner has no AbilitySystemComponent."), *ItemData->DisplayName.ToString());
        return false;
    }

    if (!ItemData->AbilityClass)
    {
        if (ItemData->bConsumeOnUse)
        {
            ConsumeSlotItem(Container, SlotIndex, 1);
            BroadcastItemEvent(InventoryItemEventTags::Used, ItemData, 1, Container, SlotIndex, GetOwner());
            return true;
        }
        return false;
    }

    FGameplayAbilitySpecHandle AbilityHandle;
    UInventoryItemGameplayAbility* Ability = FindOrGrantItemAbility(OwnerASC, ItemData, AbilityHandle);
    if (!Ability || !AbilityHandle.IsValid()) return false;

    Ability->SourceInventoryComponent = this;
    Ability->SourceItemData = ItemData;
    Ability->ItemData = ItemData;
    Ability->SourceContainer = Container;
    Ability->SourceSlotIndex = SlotIndex;
    Ability->bWasConsumedDuringUse = false;

    FGameplayAbilitySpec* Spec = OwnerASC->FindAbilitySpecFromHandle(AbilityHandle);
    if (!Spec || !Spec->IsActive())
    {
        OwnerASC->TryActivateAbility(AbilityHandle);
    }

    const bool bSuccess = Ability->OnItemUsed(ItemData, ItemData->ItemTags);
    if (bSuccess)
    {
        BroadcastItemEvent(InventoryItemEventTags::Used, ItemData, 1, Container, SlotIndex, GetOwner());

        // Automatically consume if item or ability specifies bConsumeOnUse (unless ability already consumed it)
        const bool bShouldConsume = !Ability->bWasConsumedDuringUse && ((ItemData && ItemData->bConsumeOnUse) || Ability->bConsumeOnUse);
        if (bShouldConsume)
        {
            ConsumeSlotItem(Container, SlotIndex, 1);
        }
        return true;
    }
    return false;
}

void UInventoryComponent::NotifyItemSelected(UInventoryItemData* ItemData)
{
    UpdateActiveHotbarAbility();
}

bool UInventoryComponent::ConsumeSlotItem(EInventoryContainer Container, int32 SlotIndex, int32 Amount)
{
    if (!GetOwner() || Amount <= 0 || !IsValidSlot(Container, SlotIndex)) return false;
    const FInventorySlot* Slot = FindSlot(Container, SlotIndex);
    if (!Slot || !Slot->ItemData || Slot->Quantity <= 0) return false;

    if (!GetOwner()->HasAuthority())
    {
        Server_ConsumeSlotItem(Container, SlotIndex, Amount);
        return true;
    }

    FInventorySlot* MutableSlot = FindSlot(Container, SlotIndex);
    if (!MutableSlot || !MutableSlot->ItemData || MutableSlot->Quantity <= 0) return false;

    UInventoryItemData* ItemData = MutableSlot->ItemData;
    const int32 DeductAmount = FMath::Min(Amount, MutableSlot->Quantity);
    MutableSlot->Quantity -= DeductAmount;
    const int32 RemainingQuantity = MutableSlot->Quantity;

    if (MutableSlot->Quantity <= 0)
    {
        MutableSlot->Quantity = 0;
        MutableSlot->ItemData = nullptr;
    }
    GetArray(Container).MarkItemDirty(*MutableSlot);
    NotifyInventoryChanged();

    AActor* OwnerActor = GetOwner();
    BroadcastItemEvent(InventoryItemEventTags::Removed, ItemData, DeductAmount, Container, SlotIndex, OwnerActor);
    return true;
}

bool UInventoryComponent::ConsumeSelectedHotbarItem(int32 Amount)
{
    return ConsumeSlotItem(EInventoryContainer::Hotbar, SelectedHotbarIndex, Amount);
}

bool UInventoryComponent::RemoveSlotItem(EInventoryContainer Container, int32 SlotIndex, int32 Amount)
{
    return ConsumeSlotItem(Container, SlotIndex, Amount);
}

bool UInventoryComponent::RemoveSelectedHotbarItem(int32 Amount)
{
    return ConsumeSlotItem(EInventoryContainer::Hotbar, SelectedHotbarIndex, Amount);
}

void UInventoryComponent::Server_ConsumeSlotItem_Implementation(EInventoryContainer Container, int32 SlotIndex, int32 Amount)
{
    ConsumeSlotItem(Container, SlotIndex, Amount);
}

void UInventoryComponent::Server_ConsumeSelectedHotbarItem_Implementation(int32 Amount)
{
    ConsumeSelectedHotbarItem(Amount);
}

bool UInventoryComponent::ExecuteSelectedHotbarItem()
{
    if (GetOwner() && GetOwner()->HasAuthority())
    {
        const FInventorySlot* Slot = FindSlot(EInventoryContainer::Hotbar, SelectedHotbarIndex);
        if (!Slot || !Slot->ItemData || Slot->Quantity <= 0) return false;
        return ExecuteItemEffects(Slot->ItemData, EInventoryContainer::Hotbar, SelectedHotbarIndex);
    }
    return RequestExecuteSelectedHotbarItem();
}

bool UInventoryComponent::UseSelectedHotbarItem()
{
    return ExecuteSelectedHotbarItem();
}

bool UInventoryComponent::RequestExecuteSelectedHotbarItem()
{
    const FInventorySlot* Slot = FindSlot(EInventoryContainer::Hotbar, SelectedHotbarIndex);
    if (!GetOwner() || !Slot || !Slot->ItemData || Slot->Quantity <= 0 || !CanUseItem(Slot->ItemData)) return false;
    if (GetOwner()->HasAuthority())
    {
        Server_UseSelectedHotbarItem_Implementation();
    }
    else
    {
        Server_UseSelectedHotbarItem();
    }
    return true;
}

bool UInventoryComponent::RequestUseSelectedHotbarItem()
{
    return RequestExecuteSelectedHotbarItem();
}

void UInventoryComponent::Server_UseSelectedHotbarItem_Implementation()
{
    const FInventorySlot* Slot = FindSlot(EInventoryContainer::Hotbar, SelectedHotbarIndex);
    if (!Slot || !Slot->ItemData || Slot->Quantity <= 0) return;
    ExecuteItemEffects(Slot->ItemData, EInventoryContainer::Hotbar, SelectedHotbarIndex);
}

bool UInventoryComponent::ExecuteSlotItem(EInventoryContainer Container, int32 SlotIndex)
{
    if (GetOwner() && GetOwner()->HasAuthority())
    {
        const FInventorySlot* Slot = FindSlot(Container, SlotIndex);
        if (!Slot || !Slot->ItemData || Slot->Quantity <= 0) return false;
        return ExecuteItemEffects(Slot->ItemData, Container, SlotIndex);
    }
    return RequestExecuteSlotItem(Container, SlotIndex);
}

bool UInventoryComponent::UseSlotItem(EInventoryContainer Container, int32 SlotIndex)
{
    return ExecuteSlotItem(Container, SlotIndex);
}

bool UInventoryComponent::RequestExecuteSlotItem(EInventoryContainer Container, int32 SlotIndex)
{
    const FInventorySlot* Slot = FindSlot(Container, SlotIndex);
    if (!GetOwner() || !Slot || !Slot->ItemData || Slot->Quantity <= 0 || !CanUseItem(Slot->ItemData)) return false;
    if (GetOwner()->HasAuthority())
    {
        Server_UseSlotItem_Implementation(Container, SlotIndex);
    }
    else
    {
        Server_UseSlotItem(Container, SlotIndex);
    }
    return true;
}

bool UInventoryComponent::RequestUseSlotItem(EInventoryContainer Container, int32 SlotIndex)
{
    return RequestExecuteSlotItem(Container, SlotIndex);
}

void UInventoryComponent::Server_UseSlotItem_Implementation(EInventoryContainer Container, int32 SlotIndex)
{
    const FInventorySlot* Slot = FindSlot(Container, SlotIndex);
    if (!Slot || !Slot->ItemData || Slot->Quantity <= 0) return;
    ExecuteItemEffects(Slot->ItemData, Container, SlotIndex);
}

bool UInventoryComponent::DropSelectedHotbarItem(int32 Amount)
{
    return RequestDrop(EInventoryContainer::Hotbar, SelectedHotbarIndex, Amount);
}

bool UInventoryComponent::DropSlotItem(EInventoryContainer Container, int32 SlotIndex, int32 Amount)
{
    return RequestDrop(Container, SlotIndex, Amount);
}

bool UInventoryComponent::RequestDropSelected(int32 Amount)
{
    return RequestDrop(EInventoryContainer::Hotbar, SelectedHotbarIndex, Amount);
}

bool UInventoryComponent::TossSelectedItem(float ForwardForce, int32 Amount)
{
    const FInventorySlot* Slot = FindSlot(EInventoryContainer::Hotbar, SelectedHotbarIndex);
    if (!GetOwner() || Amount <= 0 || !Slot || !Slot->ItemData || Slot->Quantity <= 0 || !Slot->ItemData->bCanBeDropped) return false;
    Server_TossSelectedItem(ForwardForce, Amount);
    return true;
}

void UInventoryComponent::Server_TossSelectedItem_Implementation(float ForwardForce, int32 Amount)
{
    if (Amount <= 0) return;
    FInventorySlot* Slot = FindSlot(EInventoryContainer::Hotbar, SelectedHotbarIndex);
    if (!Slot || !Slot->ItemData || Slot->Quantity <= 0 || !Slot->ItemData->bCanBeDropped) return;

    const int32 TossAmount = FMath::Min(Amount, Slot->Quantity);
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor) return;

    FVector StartLoc = OwnerActor->GetActorLocation() + OwnerActor->GetActorForwardVector() * 80.f + FVector(0,0,30.f);
    FVector ForwardDir = OwnerActor->GetActorForwardVector();

    if (APawn* Pawn = Cast<APawn>(OwnerActor))
    {
        FRotator ViewRot;
        Pawn->GetActorEyesViewPoint(StartLoc, ViewRot);
        ForwardDir = ViewRot.Vector();
        StartLoc += ForwardDir * 60.f;
    }

    const FVector Velocity = (ForwardDir * ForwardForce) + FVector(0, 0, 150.f);
    UInventoryItemData* TossedItem = Slot->ItemData;
    const int32 TossedSlotIndex = SelectedHotbarIndex;
    if (AWorldInventoryItem* Spawned = SpawnItemInWorld(TossedItem, TossAmount, StartLoc, ForwardDir.Rotation(), Velocity))
    {
        Slot->Quantity -= TossAmount;
        if (Slot->Quantity <= 0)
        {
            Slot->Quantity = 0;
            Slot->ItemData = nullptr;
        }
        HotbarSlots.MarkItemDirty(*Slot);
        NotifyInventoryChanged();
        BroadcastItemEvent(InventoryItemEventTags::Dropped, TossedItem, TossAmount, EInventoryContainer::Hotbar, TossedSlotIndex, Spawned);
        BroadcastItemEvent(InventoryItemEventTags::Removed, TossedItem, TossAmount, EInventoryContainer::Hotbar, TossedSlotIndex, GetOwner());
    }
}

bool UInventoryComponent::QuickTransferSlot(EInventoryContainer FromContainer, int32 FromIndex)
{
    const FInventorySlot* FromSlot = FindSlot(FromContainer, FromIndex);
    if (!GetOwner() || !FromSlot || !FromSlot->ItemData || FromSlot->Quantity <= 0) return false;
    const EInventoryContainer TargetContainer = (FromContainer == EInventoryContainer::Hotbar) ? EInventoryContainer::Inventory : EInventoryContainer::Hotbar;
    if (GetFreeSpaceForItem(FromSlot->ItemData, TargetContainer) <= 0) return false;
    Server_QuickTransferSlot(FromContainer, FromIndex);
    return true;
}

void UInventoryComponent::Server_QuickTransferSlot_Implementation(EInventoryContainer FromContainer, int32 FromIndex)
{
    FInventorySlot* FromSlot = FindSlot(FromContainer, FromIndex);
    if (!FromSlot || !FromSlot->ItemData || FromSlot->Quantity <= 0) return;

    const EInventoryContainer TargetContainer = (FromContainer == EInventoryContainer::Hotbar) ? EInventoryContainer::Inventory : EInventoryContainer::Hotbar;
    const int32 Remaining = AddItemToContainer(TargetContainer, FromSlot->ItemData, FromSlot->Quantity);
    FromSlot->Quantity = Remaining;
    if (Remaining <= 0)
    {
        FromSlot->ItemData = nullptr;
        FromSlot->Quantity = 0;
    }
    GetArray(FromContainer).MarkItemDirty(*FromSlot);
    NotifyInventoryChanged();
}

bool UInventoryComponent::SwapSlots(EInventoryContainer ContainerA, int32 IndexA, EInventoryContainer ContainerB, int32 IndexB)
{
    return RequestMove(ContainerA, IndexA, ContainerB, IndexB);
}

bool UInventoryComponent::ClearSlot(EInventoryContainer Container, int32 Index)
{
    if (!GetOwner() || !IsValidSlot(Container, Index)) return false;
    const FInventorySlot* Slot = FindSlot(Container, Index);
    if (!Slot || (!Slot->ItemData && Slot->Quantity <= 0)) return false;

    if (!GetOwner()->HasAuthority())
    {
        Server_ClearSlot(Container, Index);
        return true;
    }

    FInventorySlot* MutableSlot = FindSlot(Container, Index);
    if (MutableSlot)
    {
        UInventoryItemData* OldItem = MutableSlot->ItemData;
        const int32 OldQuantity = MutableSlot->Quantity;
        MutableSlot->ItemData = nullptr;
        MutableSlot->Quantity = 0;
        GetArray(Container).MarkItemDirty(*MutableSlot);
        NotifyInventoryChanged();
        if (OldItem && OldQuantity > 0)
        {
            BroadcastItemEvent(InventoryItemEventTags::Removed, OldItem, OldQuantity, Container, Index, GetOwner());
        }
        return true;
    }
    return false;
}

bool UInventoryComponent::ClearSelectedHotbarSlot()
{
    return ClearSlot(EInventoryContainer::Hotbar, SelectedHotbarIndex);
}

void UInventoryComponent::Server_ClearSlot_Implementation(EInventoryContainer Container, int32 Index)
{
    ClearSlot(Container, Index);
}

bool UInventoryComponent::ClearContainer(EInventoryContainer Container)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() ||
        (Container != EInventoryContainer::Inventory && Container != EInventoryContainer::Hotbar)) return false;
    FInventoryArray& Array = GetArray(Container);
    for (FInventorySlot& Slot : Array.Slots)
    {
        Slot.ItemData = nullptr;
        Slot.Quantity = 0;
        Array.MarkItemDirty(Slot);
    }
    NotifyInventoryChanged();
    return true;
}

bool UInventoryComponent::CanCraftRecipe(const TArray<UInventoryItemData*>& Ingredients, const TArray<int32>& Quantities) const
{
    if (Ingredients.Num() != Quantities.Num() || Ingredients.IsEmpty()) return false;
    for (int32 i = 0; i < Ingredients.Num(); ++i)
    {
        if (!Ingredients[i] || Quantities[i] <= 0) continue;
        if (GetTotalItemCount(Ingredients[i]) < Quantities[i])
        {
            return false;
        }
    }
    return true;
}

bool UInventoryComponent::RequestCraft(const TArray<UInventoryItemData*>& Ingredients, const TArray<int32>& Quantities, UInventoryItemData* ResultItem, int32 ResultQuantity)
{
    if (!GetOwner() || !ResultItem || ResultQuantity <= 0 ||
        !CanCraftRecipe(Ingredients, Quantities) || !CanAddItem(ResultItem, ResultQuantity)) return false;
    Server_RequestCraft(Ingredients, Quantities, ResultItem, ResultQuantity);
    return true;
}

void UInventoryComponent::Server_RequestCraft_Implementation(const TArray<UInventoryItemData*>& Ingredients, const TArray<int32>& Quantities, UInventoryItemData* ResultItem, int32 ResultQuantity)
{
    if (!ResultItem || ResultQuantity <= 0 || !CanCraftRecipe(Ingredients, Quantities)) return;
    if (!CanAddItem(ResultItem, ResultQuantity)) return;

    for (int32 i = 0; i < Ingredients.Num(); ++i)
    {
        if (Ingredients[i] && Quantities[i] > 0)
        {
            RemoveItem(Ingredients[i], Quantities[i]);
        }
    }

    AddItem(ResultItem, ResultQuantity);
}

AWorldInventoryItem* UInventoryComponent::SpawnItemInWorld(UInventoryItemData* ItemData, int32 Quantity, FVector Location, FRotator Rotation, FVector Velocity)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || !ItemData || Quantity <= 0) return nullptr;
    UWorld* World = GetWorld();
    if (!World) return nullptr;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    TSubclassOf<AWorldInventoryItem> ClassToSpawn = ItemData->WorldActorClass ? ItemData->WorldActorClass : TSubclassOf<AWorldInventoryItem>(AWorldInventoryItem::StaticClass());

    AWorldInventoryItem* Spawned = World->SpawnActor<AWorldInventoryItem>(ClassToSpawn, Location, Rotation, Params);
    if (Spawned)
    {
        Spawned->InitializeDrop(ItemData, Quantity);
        if (!Velocity.IsNearlyZero())
        {
            Spawned->LaunchItem(Velocity);
        }
    }
    return Spawned;
}

int32 UInventoryComponent::AddItemToContainer(EInventoryContainer Container, UInventoryItemData* ItemData, int32 Quantity)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || !ItemData || Quantity <= 0) return Quantity;
    int32 Remaining = Quantity;
    const int32 MaxStack = FMath::Max(1, ItemData->MaxStackSize);
    FInventoryArray& Array = GetArray(Container);

    for (FInventorySlot& Slot : Array.Slots)
    {
        if (Slot.ItemData == ItemData && Slot.Quantity < MaxStack)
        {
            const int32 Added = FMath::Min(Remaining, MaxStack - Slot.Quantity);
            Slot.Quantity += Added;
            Remaining -= Added;
            Array.MarkItemDirty(Slot);
            BroadcastItemEvent(InventoryItemEventTags::Added, ItemData, Added, Container, Slot.SlotIndex, GetOwner());
            if (Remaining == 0) { NotifyInventoryChanged(); return 0; }
        }
    }

    for (FInventorySlot& Slot : Array.Slots)
    {
        if (!Slot.ItemData || Slot.Quantity <= 0)
        {
            const int32 Added = FMath::Min(Remaining, MaxStack);
            Slot.ItemData = ItemData;
            Slot.Quantity = Added;
            Remaining -= Added;
            Array.MarkItemDirty(Slot);
            BroadcastItemEvent(InventoryItemEventTags::Added, ItemData, Added, Container, Slot.SlotIndex, GetOwner());
            if (Remaining == 0) break;
        }
    }

    NotifyInventoryChanged();
    return Remaining;
}

int32 UInventoryComponent::AddItem(UInventoryItemData* ItemData, int32 Quantity, EInventoryContainer PreferredContainer)
{
    int32 Remaining = AddItemToContainer(PreferredContainer, ItemData, Quantity);
    if (Remaining > 0)
    {
        const EInventoryContainer Other = (PreferredContainer == EInventoryContainer::Hotbar) ? EInventoryContainer::Inventory : EInventoryContainer::Hotbar;
        Remaining = AddItemToContainer(Other, ItemData, Remaining);
    }
    return Remaining;
}

int32 UInventoryComponent::RemoveItem(UInventoryItemData* ItemData, int32 Quantity)
{
    if (!GetOwner() || !ItemData || Quantity <= 0) return 0;
    if (!GetOwner()->HasAuthority())
    {
        Server_RemoveItem(ItemData, Quantity);
        return FMath::Min(Quantity, GetTotalItemCount(ItemData));
    }

    int32 Removed = 0;
    EInventoryContainer Containers[] = { EInventoryContainer::Hotbar, EInventoryContainer::Inventory };
    for (EInventoryContainer C : Containers)
    {
        FInventoryArray& Array = GetArray(C);
        for (int32 i = Array.Slots.Num() - 1; i >= 0; --i)
        {
            FInventorySlot& Slot = Array.Slots[i];
            if (Slot.ItemData == ItemData && Slot.Quantity > 0)
            {
                const int32 Take = FMath::Min(Quantity - Removed, Slot.Quantity);
                Slot.Quantity -= Take;
                Removed += Take;
                const int32 SlotIdx = Slot.SlotIndex;
                if (Slot.Quantity == 0) Slot.ItemData = nullptr;
                Array.MarkItemDirty(Slot);
                BroadcastItemEvent(InventoryItemEventTags::Removed, ItemData, Take, C, SlotIdx, GetOwner());
                if (Removed >= Quantity) { NotifyInventoryChanged(); return Removed; }
            }
        }
    }
    if (Removed > 0) NotifyInventoryChanged();
    return Removed;
}

void UInventoryComponent::Server_RemoveItem_Implementation(UInventoryItemData* ItemData, int32 Quantity)
{
    RemoveItem(ItemData, Quantity);
}

// --- Pickup Implementation ---

bool UInventoryComponent::TryPickupActor(AActor* TargetActor)
{
    if (!TargetActor) return false;

    AWorldInventoryItem* WorldItem = Cast<AWorldInventoryItem>(TargetActor);
    if (!WorldItem)
    {
        TArray<AActor*> Attached;
        TargetActor->GetAttachedActors(Attached);
        for (AActor* Att : Attached)
        {
            if (AWorldInventoryItem* Found = Cast<AWorldInventoryItem>(Att))
            {
                WorldItem = Found;
                break;
            }
        }
    }

    if (!WorldItem)
    {
        UE_LOG(LogTemp, Warning, TEXT("InventoryComponent::TryPickupActor: Actor '%s' is not an AWorldInventoryItem!"), *GetNameSafe(TargetActor));
        return false;
    }

    if (!WorldItem->ItemData || WorldItem->Quantity <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("InventoryComponent::TryPickupActor: Item '%s' has no ItemData or Quantity <= 0!"), *GetNameSafe(WorldItem));
        return false;
    }

    const AActor* OwnerActor = GetOwner();
    if (!OwnerActor) return false;

    const float DistSq = FVector::DistSquared(OwnerActor->GetActorLocation(), WorldItem->GetActorLocation());
    if (DistSq > FMath::Square(MaxPickupDistance))
    {
        UE_LOG(LogTemp, Warning, TEXT("InventoryComponent::TryPickupActor: '%s' is too far (%.1f > %.1f)!"), *GetNameSafe(WorldItem), FMath::Sqrt(DistSq), MaxPickupDistance);
        return false;
    }

    if (OwnerActor->HasAuthority())
    {
        Server_TryPickupActor_Implementation(WorldItem);
    }
    else
    {
        Server_TryPickupActor(WorldItem);
    }
    return true;
}

void UInventoryComponent::Server_TryPickupActor_Implementation(AActor* TargetActor)
{
    if (!TargetActor) return;

    AWorldInventoryItem* WorldItem = Cast<AWorldInventoryItem>(TargetActor);
    if (!WorldItem)
    {
        TArray<AActor*> Attached;
        TargetActor->GetAttachedActors(Attached);
        for (AActor* Att : Attached)
        {
            if (AWorldInventoryItem* Found = Cast<AWorldInventoryItem>(Att))
            {
                WorldItem = Found;
                break;
            }
        }
    }

    if (!WorldItem || !WorldItem->ItemData || WorldItem->Quantity <= 0) return;

    const AActor* OwnerActor = GetOwner();
    if (!OwnerActor) return;

    const float DistSq = FVector::DistSquared(OwnerActor->GetActorLocation(), WorldItem->GetActorLocation());
    if (DistSq > FMath::Square(MaxPickupDistance + 150.f))
    {
        UE_LOG(LogTemp, Warning, TEXT("InventoryComponent::Server_TryPickupActor: '%s' is too far (%.1f > %.1f)!"), *GetNameSafe(WorldItem), FMath::Sqrt(DistSq), MaxPickupDistance);
        return;
    }

    const int32 Before = WorldItem->Quantity;
    const int32 Remaining = AddItem(WorldItem->ItemData, Before, EInventoryContainer::Hotbar);
    const int32 Taken = Before - Remaining;
    if (Taken > 0)
    {
        WorldItem->TakeQuantity(Taken);
        BroadcastItemEvent(InventoryItemEventTags::PickedUp, WorldItem->ItemData, Taken, EInventoryContainer::Hotbar, INDEX_NONE, WorldItem);
    }
}

bool UInventoryComponent::TryPickupInFront(float Distance, float SphereRadius)
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor) return false;
    UWorld* World = GetWorld();
    if (!World) return false;

    FVector StartLoc = OwnerActor->GetActorLocation();
    FVector ForwardDir = OwnerActor->GetActorForwardVector();

    if (APawn* Pawn = Cast<APawn>(OwnerActor))
    {
        FRotator ViewRot;
        Pawn->GetActorEyesViewPoint(StartLoc, ViewRot);
        ForwardDir = ViewRot.Vector();
    }

    const FVector EndLoc = StartLoc + (ForwardDir * Distance);

    FCollisionQueryParams QueryParams(TEXT("PickupTrace"), false, OwnerActor);
    TArray<FHitResult> HitResults;

    const FCollisionShape SphereShape = FCollisionShape::MakeSphere(SphereRadius);
    World->SweepMultiByChannel(HitResults, StartLoc, EndLoc, FQuat::Identity, ECC_Visibility, SphereShape, QueryParams);

    if (HitResults.IsEmpty())
    {
        World->LineTraceMultiByChannel(HitResults, StartLoc, EndLoc, ECC_Visibility, QueryParams);
    }

    for (const FHitResult& Hit : HitResults)
    {
        if (AActor* HitActor = Hit.GetActor())
        {
            if (Cast<AWorldInventoryItem>(HitActor))
            {
                return TryPickupActor(HitActor);
            }
        }
    }

    return false;
}

bool UInventoryComponent::TryPickupNearby(float Radius)
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor) return false;
    UWorld* World = GetWorld();
    if (!World) return false;

    TArray<FOverlapResult> Overlaps;
    FCollisionQueryParams Params(TEXT("NearbyPickup"), false, OwnerActor);
    const FCollisionShape Sphere = FCollisionShape::MakeSphere(Radius);

    World->OverlapMultiByChannel(Overlaps, OwnerActor->GetActorLocation(), FQuat::Identity, ECC_WorldDynamic, Sphere, Params);

    AWorldInventoryItem* ClosestItem = nullptr;
    float ClosestDistSq = MAX_FLT;

    for (const FOverlapResult& Overlap : Overlaps)
    {
        if (AWorldInventoryItem* Item = Cast<AWorldInventoryItem>(Overlap.GetActor()))
        {
            if (Item->ItemData && Item->Quantity > 0)
            {
                const float DistSq = FVector::DistSquared(OwnerActor->GetActorLocation(), Item->GetActorLocation());
                if (DistSq < ClosestDistSq)
                {
                    ClosestDistSq = DistSq;
                    ClosestItem = Item;
                }
            }
        }
    }

    if (ClosestItem)
    {
        return TryPickupActor(ClosestItem);
    }

    return false;
}

bool UInventoryComponent::RequestPickup(AWorldInventoryItem* WorldItem)
{
    return TryPickupActor(WorldItem);
}

void UInventoryComponent::Server_RequestPickup_Implementation(AWorldInventoryItem* WorldItem)
{
    Server_TryPickupActor_Implementation(WorldItem);
}

bool UInventoryComponent::MoveItems(EInventoryContainer FromContainer, int32 FromIndex, EInventoryContainer ToContainer, int32 ToIndex)
{
    if (!IsValidSlot(FromContainer, FromIndex) || !IsValidSlot(ToContainer, ToIndex)) return false;
    FInventorySlot* From = FindSlot(FromContainer, FromIndex);
    FInventorySlot* To = FindSlot(ToContainer, ToIndex);
    if (!From || !To || !From->ItemData || From->Quantity <= 0 || From == To) return false;

    if (To->ItemData == From->ItemData && To->Quantity < FMath::Max(1, To->ItemData->MaxStackSize))
    {
        const int32 Amount = FMath::Min(From->Quantity, To->ItemData->MaxStackSize - To->Quantity);
        To->Quantity += Amount;
        From->Quantity -= Amount;
        if (From->Quantity <= 0) { From->Quantity = 0; From->ItemData = nullptr; }
    }
    else
    {
        Swap(From->ItemData, To->ItemData);
        Swap(From->Quantity, To->Quantity);
    }

    GetArray(FromContainer).MarkItemDirty(*From);
    GetArray(ToContainer).MarkItemDirty(*To);
    NotifyInventoryChanged();
    return true;
}

bool UInventoryComponent::RequestMove(EInventoryContainer A, int32 AI, EInventoryContainer B, int32 BI)
{
    const FInventorySlot* From = FindSlot(A, AI);
    const FInventorySlot* To = FindSlot(B, BI);
    if (!GetOwner() || !IsValidSlot(A, AI) || !IsValidSlot(B, BI) ||
        !From || !To || From == To || !From->ItemData || From->Quantity <= 0) return false;
    Server_RequestMove(A, AI, B, BI);
    return true;
}
void UInventoryComponent::Server_RequestMove_Implementation(EInventoryContainer A, int32 AI, EInventoryContainer B, int32 BI) { MoveItems(A, AI, B, BI); }

AWorldInventoryItem* UInventoryComponent::SpawnDroppedItem(UInventoryItemData* ItemData, int32 Quantity)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || !ItemData || Quantity <= 0 || !ItemData->bCanBeDropped) return nullptr;
    UWorld* World = GetWorld();
    if (!World) return nullptr;

    const FVector Location = GetOwner()->GetActorLocation() + GetOwner()->GetActorForwardVector() * 120.f + FVector(0,0,40.f);
    FActorSpawnParameters Params;
    Params.Owner = nullptr;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    TSubclassOf<AWorldInventoryItem> ClassToSpawn = ItemData->WorldActorClass ? ItemData->WorldActorClass : TSubclassOf<AWorldInventoryItem>(AWorldInventoryItem::StaticClass());
    AWorldInventoryItem* Spawned = World->SpawnActor<AWorldInventoryItem>(ClassToSpawn, Location, GetOwner()->GetActorRotation(), Params);
    if (!Spawned) return nullptr;
    Spawned->InitializeDrop(ItemData, Quantity);
    return Spawned;
}

bool UInventoryComponent::RequestDrop(EInventoryContainer C, int32 I, int32 Amount)
{
    const FInventorySlot* Slot = FindSlot(C, I);
    if (!GetOwner() || !IsValidSlot(C, I) || Amount <= 0 ||
        !Slot || !Slot->ItemData || Slot->Quantity <= 0 || !Slot->ItemData->bCanBeDropped) return false;
    Server_RequestDrop(C, I, Amount);
    return true;
}
void UInventoryComponent::Server_RequestDrop_Implementation(EInventoryContainer C, int32 I, int32 Amount)
{
    if (!IsValidSlot(C, I) || Amount <= 0) return;
    FInventorySlot* Slot = FindSlot(C, I);
    if (!Slot || !Slot->ItemData || !Slot->ItemData->bCanBeDropped) return;
    const int32 DropAmount = FMath::Min(Amount, Slot->Quantity);
    UInventoryItemData* DroppedItem = Slot->ItemData;
    AWorldInventoryItem* Spawned = SpawnDroppedItem(DroppedItem, DropAmount);
    if (!Spawned) return;
    Slot->Quantity -= DropAmount;
    if (Slot->Quantity <= 0) { Slot->Quantity = 0; Slot->ItemData = nullptr; }
    GetArray(C).MarkItemDirty(*Slot);
    NotifyInventoryChanged();
    BroadcastItemEvent(InventoryItemEventTags::Dropped, DroppedItem, DropAmount, C, I, Spawned);
    BroadcastItemEvent(InventoryItemEventTags::Removed, DroppedItem, DropAmount, C, I, GetOwner());
}

bool UInventoryComponent::RequestDropAll(EInventoryContainer Container)
{
    if (!GetOwner() ||
        (Container != EInventoryContainer::Inventory && Container != EInventoryContainer::Hotbar)) return false;
    for (const FInventorySlot& Slot : GetArray(Container).Slots)
    {
        if (Slot.ItemData && Slot.Quantity > 0 && Slot.ItemData->bCanBeDropped)
        {
            Server_RequestDropAll(Container);
            return true;
        }
    }
    return false;
}

void UInventoryComponent::Server_RequestDropAll_Implementation(EInventoryContainer Container)
{
    FInventoryArray& Array = GetArray(Container);
    for (FInventorySlot& Slot : Array.Slots)
    {
        if (Slot.ItemData && Slot.Quantity > 0 && Slot.ItemData->bCanBeDropped)
        {
            UInventoryItemData* OldItem = Slot.ItemData;
            const int32 OldQty = Slot.Quantity;
            const int32 OldSlotIdx = Slot.SlotIndex;
            if (AWorldInventoryItem* Spawned = SpawnDroppedItem(OldItem, OldQty))
            {
                Slot.ItemData = nullptr;
                Slot.Quantity = 0;
                Array.MarkItemDirty(Slot);
                BroadcastItemEvent(InventoryItemEventTags::Dropped, OldItem, OldQty, Container, OldSlotIdx, Spawned);
                BroadcastItemEvent(InventoryItemEventTags::Removed, OldItem, OldQty, Container, OldSlotIdx, GetOwner());
            }
        }
    }
    NotifyInventoryChanged();
}

bool UInventoryComponent::RequestSort(EInventoryContainer Container)
{
    if (!GetOwner() ||
        (Container != EInventoryContainer::Inventory && Container != EInventoryContainer::Hotbar)) return false;
    Server_RequestSort(Container);
    return true;
}

void UInventoryComponent::Server_RequestSort_Implementation(EInventoryContainer Container)
{
    FInventoryArray& Array = GetArray(Container);

    TMap<UInventoryItemData*, int32> Totals;
    for (const FInventorySlot& Slot : Array.Slots)
    {
        if (Slot.ItemData && Slot.Quantity > 0)
        {
            Totals.FindOrAdd(Slot.ItemData) += Slot.Quantity;
        }
    }

    TArray<UInventoryItemData*> Keys;
    Totals.GetKeys(Keys);
    Keys.Sort([](const UInventoryItemData& A, const UInventoryItemData& B) {
        return A.DisplayName.ToString() < B.DisplayName.ToString();
    });

    for (FInventorySlot& Slot : Array.Slots)
    {
        Slot.ItemData = nullptr;
        Slot.Quantity = 0;
    }

    int32 SlotIdx = 0;
    for (UInventoryItemData* ItemData : Keys)
    {
        int32 Remaining = Totals[ItemData];
        const int32 MaxStack = FMath::Max(1, ItemData->MaxStackSize);
        while (Remaining > 0 && SlotIdx < Array.Slots.Num())
        {
            const int32 Put = FMath::Min(Remaining, MaxStack);
            Array.Slots[SlotIdx].ItemData = ItemData;
            Array.Slots[SlotIdx].Quantity = Put;
            Array.MarkItemDirty(Array.Slots[SlotIdx]);
            Remaining -= Put;
            SlotIdx++;
        }
    }

    for (int32 i = SlotIdx; i < Array.Slots.Num(); ++i)
    {
        Array.MarkItemDirty(Array.Slots[i]);
    }

    NotifyInventoryChanged();
}

bool UInventoryComponent::RequestSplit(EInventoryContainer Container, int32 FromIndex, int32 ToIndex, int32 Amount)
{
    const FInventorySlot* From = FindSlot(Container, FromIndex);
    const FInventorySlot* To = FindSlot(Container, ToIndex);
    if (!GetOwner() || !IsValidSlot(Container, FromIndex) || !IsValidSlot(Container, ToIndex) ||
        Amount <= 0 || FromIndex == ToIndex || !From || !To ||
        !From->ItemData || From->Quantity <= Amount || (To->ItemData && To->Quantity > 0)) return false;
    Server_RequestSplit(Container, FromIndex, ToIndex, Amount);
    return true;
}

void UInventoryComponent::Server_RequestSplit_Implementation(EInventoryContainer Container, int32 FromIndex, int32 ToIndex, int32 Amount)
{
    if (!IsValidSlot(Container, FromIndex) || !IsValidSlot(Container, ToIndex) || Amount <= 0 || FromIndex == ToIndex) return;
    FInventorySlot* From = FindSlot(Container, FromIndex);
    FInventorySlot* To = FindSlot(Container, ToIndex);
    if (!From || !To || !From->ItemData || From->Quantity <= Amount) return;

    if (!To->ItemData || To->Quantity <= 0)
    {
        To->ItemData = From->ItemData;
        To->Quantity = Amount;
        From->Quantity -= Amount;
        GetArray(Container).MarkItemDirty(*From);
        GetArray(Container).MarkItemDirty(*To);
        NotifyInventoryChanged();
    }
}

bool UInventoryComponent::RequestSelectHotbar(int32 Index)
{
    const int32 Count = GetSlotCount(EInventoryContainer::Hotbar);
    if (!GetOwner() || Index < 0 || Index >= Count) return false;
    const FInventorySlot* Slot = FindSlot(EInventoryContainer::Hotbar, Index);
    if (Slot && Slot->ItemData && !CanSelectItem(Slot->ItemData)) return false;
    Server_RequestSelectHotbar(Index);
    return true;
}

void UInventoryComponent::Server_RequestSelectHotbar_Implementation(int32 Index)
{
    const int32 Count = GetSlotCount(EInventoryContainer::Hotbar);
    if (Index >= 0 && Index < Count)
    {
        const FInventorySlot* Slot = FindSlot(EInventoryContainer::Hotbar, Index);
        if (Slot && Slot->ItemData && !CanSelectItem(Slot->ItemData)) return;
        if (SelectedHotbarIndex == Index) return;
        SelectedHotbarIndex = Index;
        OnRep_SelectedHotbarIndex();
        UpdateActiveHotbarAbility();
    }
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UInventoryComponent, InventorySlots);
    DOREPLIFETIME(UInventoryComponent, HotbarSlots);
    DOREPLIFETIME(UInventoryComponent, SelectedHotbarIndex);
}
