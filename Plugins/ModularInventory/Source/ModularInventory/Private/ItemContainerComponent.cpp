#include "ItemContainerComponent.h"
#include "ItemDefinition.h"
#include "ItemPickup.h"
#include "ItemLibrary.h"
#include "ModularInventory.h"

#include "Algo/Sort.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

// ====================================================================================================
//  Setup / replication
// ====================================================================================================

UItemContainerComponent::UItemContainerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UItemContainerComponent::BeginPlay()
{
    Super::BeginPlay();

    if (HasServerAuthority())
    {
        InitializeSlots();
    }
}

void UItemContainerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION(UItemContainerComponent, OwnerSlots, COND_OwnerOnly);
    DOREPLIFETIME(UItemContainerComponent, SharedSlots);
    DOREPLIFETIME(UItemContainerComponent, SelectedSlot);
}

bool UItemContainerComponent::HasServerAuthority() const
{
    const AActor* Owner = GetOwner();
    return Owner && Owner->HasAuthority();
}

void UItemContainerComponent::InitializeSlots()
{
    MutableSlots().SetNum(FMath::Max(1, NumSlots));

    TArray<int32> Changed;
    for (const FItemStack& Stack : StartingItems)
    {
        if (Stack.IsEmpty()) { continue; }

        const int32 Left = AddInternal(Stack.Item, Stack.Count, Changed, INDEX_NONE);
        if (Left > 0)
        {
            UE_LOG(LogModularInventory, Warning, TEXT("%s: starting item %s: %d did not fit (full or blocked by tag rules)."),
                *GetNameSafe(this), *GetNameSafe(Stack.Item), Left);
        }
    }
    NotifyChanged(Changed);
}

void UItemContainerComponent::HandleSlotsReplicated()
{
    const TArray<FItemSlot>& Current = GetSlots();
    const int32 Max = FMath::Max(Current.Num(), ClientCache.Num());

    bool bAny = Current.Num() != ClientCache.Num();
    for (int32 i = 0; i < Max; ++i)
    {
        const FItemSlot Now = Current.IsValidIndex(i) ? Current[i] : FItemSlot();
        const FItemSlot Before = ClientCache.IsValidIndex(i) ? ClientCache[i] : FItemSlot();
        if (!Now.IsSame(Before))
        {
            bAny = true;
            BroadcastSlot(i, Now);
        }
    }

    ClientCache = Current;

    if (bAny)
    {
        OnContainerChanged.Broadcast();
    }
}

void UItemContainerComponent::OnRep_SelectedSlot(int32 OldSlot)
{
    OnSelectionChanged.Broadcast(OldSlot, SelectedSlot);
}

void UItemContainerComponent::BroadcastSlot(int32 Index, const FItemSlot& Slot)
{
    OnSlotChanged.Broadcast(Index, Slot.IsEmpty() ? nullptr : Slot.Item.Get(), Slot.IsEmpty() ? 0 : Slot.Count);
}

void UItemContainerComponent::NotifyChanged(const TArray<int32>& Changed)
{
    if (Changed.Num() == 0) { return; }

    const TArray<FItemSlot>& Slots = GetSlots();
    for (const int32 Index : Changed)
    {
        if (Slots.IsValidIndex(Index))
        {
            BroadcastSlot(Index, Slots[Index]);
        }
    }
    OnContainerChanged.Broadcast();
}

// ====================================================================================================
//  Owner helpers
// ====================================================================================================

AController* UItemContainerComponent::GetOwningController() const
{
    AActor* Actor = GetOwner();
    while (Actor)
    {
        if (AController* Controller = Cast<AController>(Actor)) { return Controller; }
        if (APawn* Pawn = Cast<APawn>(Actor)) { return Pawn->GetController(); }
        if (APlayerState* PS = Cast<APlayerState>(Actor)) { return Cast<AController>(PS->GetOwner()); }
        Actor = Actor->GetOwner();
    }
    return nullptr;
}

AActor* UItemContainerComponent::GetReferenceActor() const
{
    if (AController* Controller = GetOwningController())
    {
        if (APawn* Pawn = Controller->GetPawn()) { return Pawn; }
    }
    return GetOwner();
}

UItemContainerComponent* UItemContainerComponent::FindRpcCarrier()
{
    const AActor* Owner = GetOwner();
    if (Owner && Owner->HasLocalNetOwner())
    {
        return this;
    }

    // This container (e.g. a chest) is not ours: use any container of the local player to send the request.
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            for (UItemContainerComponent* Candidate : UItemLibrary::FindItemContainers(PC))
            {
                if (Candidate && Candidate->GetOwner() && Candidate->GetOwner()->HasLocalNetOwner())
                {
                    return Candidate;
                }
            }
        }
    }

    UE_LOG(LogModularInventory, Warning, TEXT("%s: cannot send request. The local player needs to own at least one Item Container."), *GetNameSafe(this));
    return nullptr;
}

// ====================================================================================================
//  Rules
// ====================================================================================================

bool UItemContainerComponent::CanAcceptItem_Implementation(UItemDefinition* Item) const
{
    if (!Item) { return false; }
    if (BlockedItemTags.Num() > 0 && Item->ItemTags.HasAny(BlockedItemTags)) { return false; }
    if (AllowedItemTags.Num() > 0 && !Item->ItemTags.HasAny(AllowedItemTags)) { return false; }
    return true;
}

bool UItemContainerComponent::CanBeAccessedBy_Implementation(AController* Who) const
{
    if (!Who) { return true; } // trusted server code

    if (GetOwningController() == Who) { return true; }
    if (!bAllowExternalAccess) { return false; }

    const APawn* Pawn = Who->GetPawn();
    const AActor* Owner = GetOwner();
    if (!Pawn || !Owner) { return false; }

    return FVector::DistSquared(Pawn->GetActorLocation(), Owner->GetActorLocation()) <= FMath::Square(MaxAccessDistance);
}

// ====================================================================================================
//  Queries
// ====================================================================================================

FItemSlot UItemContainerComponent::GetSlot(int32 SlotIndex) const
{
    const TArray<FItemSlot>& Slots = GetSlots();
    return Slots.IsValidIndex(SlotIndex) ? Slots[SlotIndex] : FItemSlot();
}

bool UItemContainerComponent::IsSlotEmpty(int32 SlotIndex) const
{
    const TArray<FItemSlot>& Slots = GetSlots();
    return !Slots.IsValidIndex(SlotIndex) || Slots[SlotIndex].IsEmpty();
}

int32 UItemContainerComponent::GetItemCount(UItemDefinition* Item) const
{
    if (!Item) { return 0; }

    int32 Total = 0;
    for (const FItemSlot& Slot : GetSlots())
    {
        if (Slot.Item == Item) { Total += Slot.Count; }
    }
    return Total;
}

bool UItemContainerComponent::HasItem(UItemDefinition* Item, int32 Count) const
{
    return Item && GetItemCount(Item) >= FMath::Max(1, Count);
}

bool UItemContainerComponent::HasItems(const TArray<FItemStack>& Stacks) const
{
    // Sum duplicates first so {Wood x2, Wood x3} needs 5 Wood.
    TMap<UItemDefinition*, int32> Needed;
    for (const FItemStack& Stack : Stacks)
    {
        if (!Stack.IsEmpty()) { Needed.FindOrAdd(Stack.Item) += Stack.Count; }
    }
    for (const TPair<UItemDefinition*, int32>& Pair : Needed)
    {
        if (GetItemCount(Pair.Key) < Pair.Value) { return false; }
    }
    return true;
}

int32 UItemContainerComponent::FindSlotWithItem(UItemDefinition* Item) const
{
    if (!Item) { return INDEX_NONE; }

    const TArray<FItemSlot>& Slots = GetSlots();
    for (int32 i = 0; i < Slots.Num(); ++i)
    {
        if (Slots[i].Item == Item && Slots[i].Count > 0) { return i; }
    }
    return INDEX_NONE;
}

int32 UItemContainerComponent::FindEmptySlot() const
{
    const TArray<FItemSlot>& Slots = GetSlots();
    for (int32 i = 0; i < Slots.Num(); ++i)
    {
        if (Slots[i].IsEmpty()) { return i; }
    }
    return INDEX_NONE;
}

int32 UItemContainerComponent::GetUsedSlotCount() const
{
    int32 Used = 0;
    for (const FItemSlot& Slot : GetSlots())
    {
        if (!Slot.IsEmpty()) { ++Used; }
    }
    return Used;
}

int32 UItemContainerComponent::GetFreeSpaceFor(UItemDefinition* Item) const
{
    if (!Item || !CanAcceptItem(Item)) { return 0; }

    const int32 Max = FMath::Max(1, Item->MaxStackSize);
    int32 Space = 0;
    for (const FItemSlot& Slot : GetSlots())
    {
        if (Slot.IsEmpty())
        {
            Space += Max;
        }
        else if (bAutoStack && Slot.Item == Item && Slot.Count < Max)
        {
            Space += Max - Slot.Count;
        }
    }
    return Space;
}

// ====================================================================================================
//  Server only: add / remove
// ====================================================================================================

int32 UItemContainerComponent::AddInternal(UItemDefinition* Item, int32 Count, TArray<int32>& Changed, int32 ExcludeSlot)
{
    if (!Item || Count <= 0) { return FMath::Max(Count, 0); }
    if (!CanAcceptItem(Item)) { return Count; }

    TArray<FItemSlot>& Slots = MutableSlots();
    const int32 Max = FMath::Max(1, Item->MaxStackSize);
    int32 Left = Count;

    if (bAutoStack)
    {
        for (int32 i = 0; i < Slots.Num() && Left > 0; ++i)
        {
            if (i == ExcludeSlot) { continue; }

            FItemSlot& Slot = Slots[i];
            if (Slot.Item == Item && Slot.Count > 0 && Slot.Count < Max)
            {
                const int32 Add = FMath::Min(Left, Max - Slot.Count);
                Slot.Count += Add;
                Left -= Add;
                Changed.AddUnique(i);
            }
        }
    }

    for (int32 i = 0; i < Slots.Num() && Left > 0; ++i)
    {
        if (i == ExcludeSlot) { continue; }

        FItemSlot& Slot = Slots[i];
        if (Slot.IsEmpty())
        {
            const int32 Add = FMath::Min(Left, Max);
            Slot.Item = Item;
            Slot.Count = Add;
            Left -= Add;
            Changed.AddUnique(i);
        }
    }
    return Left;
}

int32 UItemContainerComponent::RemoveFromSlotInternal(int32 SlotIndex, int32 Count, TArray<int32>& Changed)
{
    TArray<FItemSlot>& Slots = MutableSlots();
    if (!Slots.IsValidIndex(SlotIndex) || Slots[SlotIndex].IsEmpty()) { return 0; }

    FItemSlot& Slot = Slots[SlotIndex];
    const int32 Amount = Count <= 0 ? Slot.Count : FMath::Min(Count, Slot.Count);

    Slot.Count -= Amount;
    if (Slot.Count <= 0) { Slot = FItemSlot(); }

    Changed.AddUnique(SlotIndex);
    return Amount;
}

int32 UItemContainerComponent::AddItem(UItemDefinition* Item, int32 Count)
{
    if (!HasServerAuthority())
    {
        UE_LOG(LogModularInventory, Warning, TEXT("AddItem is server only."));
        return FMath::Max(Count, 0);
    }

    TArray<int32> Changed;
    const int32 Left = AddInternal(Item, Count, Changed, INDEX_NONE);
    NotifyChanged(Changed);
    return Left;
}

int32 UItemContainerComponent::AddItemToSlot(UItemDefinition* Item, int32 Count, int32 SlotIndex)
{
    if (!HasServerAuthority())
    {
        UE_LOG(LogModularInventory, Warning, TEXT("AddItemToSlot is server only."));
        return FMath::Max(Count, 0);
    }
    if (!Item || Count <= 0 || !IsValidSlot(SlotIndex) || !CanAcceptItem(Item)) { return FMath::Max(Count, 0); }

    FItemSlot& Slot = MutableSlots()[SlotIndex];
    const int32 Max = FMath::Max(1, Item->MaxStackSize);

    int32 Added = 0;
    if (Slot.IsEmpty())
    {
        Added = FMath::Min(Count, Max);
        Slot.Item = Item;
        Slot.Count = Added;
    }
    else if (Slot.Item == Item)
    {
        Added = FMath::Min(Count, Max - Slot.Count);
        if (Added > 0) { Slot.Count += Added; }
    }

    if (Added > 0)
    {
        NotifyChanged({ SlotIndex });
    }
    return Count - FMath::Max(Added, 0);
}

int32 UItemContainerComponent::RemoveItem(UItemDefinition* Item, int32 Count)
{
    if (!HasServerAuthority())
    {
        UE_LOG(LogModularInventory, Warning, TEXT("RemoveItem is server only."));
        return 0;
    }
    if (!Item || Count <= 0) { return 0; }

    TArray<int32> Changed;
    int32 Left = Count;
    const int32 NumSlotsNow = GetNumSlots();
    for (int32 i = 0; i < NumSlotsNow && Left > 0; ++i)
    {
        if (GetSlots()[i].Item == Item)
        {
            Left -= RemoveFromSlotInternal(i, Left, Changed);
        }
    }
    NotifyChanged(Changed);
    return Count - Left;
}

int32 UItemContainerComponent::RemoveFromSlot(int32 SlotIndex, int32 Count)
{
    if (!HasServerAuthority())
    {
        UE_LOG(LogModularInventory, Warning, TEXT("RemoveFromSlot is server only."));
        return 0;
    }

    TArray<int32> Changed;
    const int32 Removed = RemoveFromSlotInternal(SlotIndex, Count, Changed);
    NotifyChanged(Changed);
    return Removed;
}

bool UItemContainerComponent::RemoveItems(const TArray<FItemStack>& Stacks)
{
    if (!HasServerAuthority())
    {
        UE_LOG(LogModularInventory, Warning, TEXT("RemoveItems is server only."));
        return false;
    }
    if (!HasItems(Stacks)) { return false; }

    for (const FItemStack& Stack : Stacks)
    {
        if (!Stack.IsEmpty()) { RemoveItem(Stack.Item, Stack.Count); }
    }
    return true;
}

void UItemContainerComponent::ClearAll()
{
    if (!HasServerAuthority()) { return; }

    TArray<int32> Changed;
    const int32 Num = GetNumSlots();
    for (int32 i = 0; i < Num; ++i)
    {
        RemoveFromSlotInternal(i, 0, Changed);
    }
    NotifyChanged(Changed);
}

void UItemContainerComponent::SetNumSlots(int32 NewNumSlots)
{
    if (!HasServerAuthority()) { return; }

    NewNumSlots = FMath::Max(1, NewNumSlots);
    TArray<FItemSlot>& Slots = MutableSlots();
    if (NewNumSlots == Slots.Num()) { return; }

    TArray<FItemSlot> Spill;
    for (int32 i = NewNumSlots; i < Slots.Num(); ++i)
    {
        if (!Slots[i].IsEmpty()) { Spill.Add(Slots[i]); }
    }

    Slots.SetNum(NewNumSlots);
    NumSlots = NewNumSlots;

    TArray<int32> Changed;
    for (const FItemSlot& Stack : Spill)
    {
        const int32 Left = AddInternal(Stack.Item, Stack.Count, Changed, INDEX_NONE);
        if (Left > 0) { SpawnDrop(Stack.Item, Left); }
    }

    // Tell listeners about every slot, because the size changed.
    for (int32 i = 0; i < Slots.Num(); ++i) { Changed.AddUnique(i); }
    NotifyChanged(Changed);
}

// ====================================================================================================
//  Moving items (works between any two containers)
// ====================================================================================================

bool UItemContainerComponent::MoveItem(int32 FromSlot, UItemContainerComponent* ToContainer, int32 ToSlot, int32 Count)
{
    if (!ToContainer) { ToContainer = this; }
    if (IsSlotEmpty(FromSlot)) { return false; }

    if (HasServerAuthority())
    {
        return TransferInternal(this, FromSlot, ToContainer, ToSlot, Count, nullptr);
    }

    if (UItemContainerComponent* Carrier = FindRpcCarrier())
    {
        Carrier->Server_Transfer(this, FromSlot, ToContainer, ToSlot, Count);
        return true;
    }
    return false;
}

void UItemContainerComponent::Server_Transfer_Implementation(UItemContainerComponent* Source, int32 FromSlot, UItemContainerComponent* Dest, int32 ToSlot, int32 Count)
{
    AController* Who = GetOwningController();
    if (!Who) { return; }

    TransferInternal(Source, FromSlot, Dest, ToSlot, Count, Who);
}

bool UItemContainerComponent::TransferInternal(UItemContainerComponent* Src, int32 FromSlot, UItemContainerComponent* Dst, int32 ToSlot, int32 Count, AController* Who)
{
    if (!Src || !Dst) { return false; }
    if (Who && (!Src->CanBeAccessedBy(Who) || !Dst->CanBeAccessedBy(Who))) { return false; }
    if (!Src->IsValidSlot(FromSlot)) { return false; }
    if (Src == Dst && FromSlot == ToSlot) { return false; }

    FItemSlot& From = Src->MutableSlots()[FromSlot];
    if (From.IsEmpty()) { return false; }

    UItemDefinition* Item = From.Item;
    const int32 Have = From.Count;
    const int32 Want = Count <= 0 ? Have : FMath::Min(Count, Have);

    TArray<int32> SrcChanged;
    TArray<int32> DstChanged;

    // Takes "Moved" from the source slot.
    auto TakeFromSource = [&](int32 Moved)
    {
        From.Count -= Moved;
        if (From.Count <= 0) { From = FItemSlot(); }
        SrcChanged.AddUnique(FromSlot);
    };

    if (ToSlot == INDEX_NONE)
    {
        // Automatic: stack onto existing stacks, then use free slots.
        const int32 Left = Dst->AddInternal(Item, Want, DstChanged, Dst == Src ? FromSlot : INDEX_NONE);
        const int32 Moved = Want - Left;
        if (Moved <= 0) { return false; }
        TakeFromSource(Moved);
    }
    else
    {
        if (!Dst->IsValidSlot(ToSlot) || !Dst->CanAcceptItem(Item)) { return false; }

        FItemSlot& To = Dst->MutableSlots()[ToSlot];
        const int32 Max = FMath::Max(1, Item->MaxStackSize);

        if (To.IsEmpty())
        {
            const int32 Moved = FMath::Min(Want, Max);
            To.Item = Item;
            To.Count = Moved;
            DstChanged.AddUnique(ToSlot);
            TakeFromSource(Moved);
        }
        else if (To.Item == Item)
        {
            const int32 Moved = FMath::Min(Want, Max - To.Count);
            if (Moved <= 0) { return false; }
            To.Count += Moved;
            DstChanged.AddUnique(ToSlot);
            TakeFromSource(Moved);
        }
        else
        {
            // Different item: swap, but only whole stacks and only if both sides accept what they get.
            if (Want < Have) { return false; }
            if (!Src->CanAcceptItem(To.Item)) { return false; }

            Swap(From, To);
            SrcChanged.AddUnique(FromSlot);
            DstChanged.AddUnique(ToSlot);
        }
    }

    if (Src == Dst)
    {
        for (const int32 Index : DstChanged) { SrcChanged.AddUnique(Index); }
        Src->NotifyChanged(SrcChanged);
    }
    else
    {
        Src->NotifyChanged(SrcChanged);
        Dst->NotifyChanged(DstChanged);
    }
    return true;
}

// ====================================================================================================
//  Dropping
// ====================================================================================================

bool UItemContainerComponent::DropItem(int32 SlotIndex, int32 Count)
{
    if (!bAllowDropping || IsSlotEmpty(SlotIndex)) { return false; }

    if (HasServerAuthority())
    {
        return DropInternal(SlotIndex, Count, nullptr);
    }

    if (UItemContainerComponent* Carrier = FindRpcCarrier())
    {
        Carrier->Server_Drop(this, SlotIndex, Count);
        return true;
    }
    return false;
}

void UItemContainerComponent::Server_Drop_Implementation(UItemContainerComponent* Target, int32 SlotIndex, int32 Count)
{
    AController* Who = GetOwningController();
    if (!Who || !Target) { return; }

    Target->DropInternal(SlotIndex, Count, Who);
}

bool UItemContainerComponent::DropInternal(int32 SlotIndex, int32 Count, AController* Who)
{
    if (!bAllowDropping || !IsValidSlot(SlotIndex)) { return false; }
    if (Who && !CanBeAccessedBy(Who)) { return false; }

    const FItemSlot& Slot = GetSlots()[SlotIndex];
    if (Slot.IsEmpty() || !Slot.Item->bCanDrop) { return false; }

    UItemDefinition* Item = Slot.Item;
    const int32 Amount = Count <= 0 ? Slot.Count : FMath::Min(Count, Slot.Count);

    TArray<int32> Changed;
    const int32 Removed = RemoveFromSlotInternal(SlotIndex, Amount, Changed);
    NotifyChanged(Changed);

    SpawnDrop(Item, Removed);
    return true;
}

void UItemContainerComponent::SpawnDrop(UItemDefinition* Item, int32 Count)
{
    AActor* Reference = GetReferenceActor();
    if (!Item || Count <= 0 || !Reference) { return; }

    const FVector Forward = Reference->GetActorForwardVector();
    const FVector Location = Reference->GetActorLocation() + Forward * DropDistance + FVector(0.f, 0.f, 30.f);

    AItemPickup* Spawned = AItemPickup::SpawnInternal(GetWorld(), Item, Count, Location, Forward * DropSpeed, /*bAllowAutoPickup*/ false, DropClass);
    if (!Spawned)
    {
        // Could not spawn: give the items back rather than losing them.
        TArray<int32> Changed;
        AddInternal(Item, Count, Changed, INDEX_NONE);
        NotifyChanged(Changed);
    }
}

// ====================================================================================================
//  Using
// ====================================================================================================

bool UItemContainerComponent::UseItem(int32 SlotIndex)
{
    if (IsSlotEmpty(SlotIndex)) { return false; }

    if (HasServerAuthority())
    {
        return UseInternal(SlotIndex, nullptr);
    }

    if (UItemContainerComponent* Carrier = FindRpcCarrier())
    {
        Carrier->Server_Use(this, SlotIndex);
        return true;
    }
    return false;
}

void UItemContainerComponent::Server_Use_Implementation(UItemContainerComponent* Target, int32 SlotIndex)
{
    AController* Who = GetOwningController();
    if (!Who || !Target) { return; }

    Target->UseInternal(SlotIndex, Who);
}

bool UItemContainerComponent::UseInternal(int32 SlotIndex, AController* Who)
{
    if (!IsValidSlot(SlotIndex)) { return false; }
    if (Who && !CanBeAccessedBy(Who)) { return false; }

    const FItemSlot& Slot = GetSlots()[SlotIndex];
    if (Slot.IsEmpty()) { return false; }

    UItemDefinition* Item = Slot.Item;
    AActor* User = (Who && Who->GetPawn()) ? static_cast<AActor*>(Who->GetPawn()) : GetReferenceActor();

    if (Item->UseAction && !Item->UseAction->ExecuteUse(this, SlotIndex, User))
    {
        return false;
    }

    OnItemUsed.Broadcast(Item, SlotIndex, User);

    // The action might have changed the slot. Only consume if it still holds the same item.
    if (Item->bConsumeOnUse && IsValidSlot(SlotIndex) && GetSlots()[SlotIndex].Item == Item)
    {
        TArray<int32> Changed;
        RemoveFromSlotInternal(SlotIndex, 1, Changed);
        NotifyChanged(Changed);
    }
    return true;
}

// ====================================================================================================
//  Sorting
// ====================================================================================================

bool UItemContainerComponent::SortItems()
{
    if (HasServerAuthority())
    {
        SortInternal();
        return true;
    }

    if (UItemContainerComponent* Carrier = FindRpcCarrier())
    {
        Carrier->Server_Sort(this);
        return true;
    }
    return false;
}

void UItemContainerComponent::Server_Sort_Implementation(UItemContainerComponent* Target)
{
    AController* Who = GetOwningController();
    if (!Who || !Target || !Target->CanBeAccessedBy(Who)) { return; }

    Target->SortInternal();
}

void UItemContainerComponent::SortInternal()
{
    TArray<FItemSlot>& Slots = MutableSlots();
    const TArray<FItemSlot> Before = Slots;

    TMap<UItemDefinition*, int32> Totals;
    TArray<UItemDefinition*> Order;
    for (const FItemSlot& Slot : Before)
    {
        if (Slot.IsEmpty()) { continue; }

        if (int32* Found = Totals.Find(Slot.Item)) { *Found += Slot.Count; }
        else { Totals.Add(Slot.Item, Slot.Count); Order.Add(Slot.Item); }
    }

    Algo::Sort(Order, [](const UItemDefinition* A, const UItemDefinition* B)
    {
        const int32 ByName = A->DisplayName.CompareTo(B->DisplayName);
        return ByName != 0 ? ByName < 0 : A->GetName().Compare(B->GetName()) < 0;
    });

    for (FItemSlot& Slot : Slots) { Slot = FItemSlot(); }

    int32 Next = 0;
    for (UItemDefinition* Item : Order)
    {
        int32 Left = Totals[Item];
        const int32 Max = FMath::Max(1, Item->MaxStackSize);
        while (Left > 0 && Next < Slots.Num())
        {
            const int32 Put = FMath::Min(Left, Max);
            Slots[Next].Item = Item;
            Slots[Next].Count = Put;
            Left -= Put;
            ++Next;
        }
    }

    TArray<int32> Changed;
    for (int32 i = 0; i < Slots.Num(); ++i)
    {
        if (!Slots[i].IsSame(Before[i])) { Changed.Add(i); }
    }
    NotifyChanged(Changed);
}

// ====================================================================================================
//  Picking up world items
// ====================================================================================================

bool UItemContainerComponent::PickupActor(AActor* Target)
{
    AItemPickup* Pickup = Cast<AItemPickup>(Target);
    if (!Pickup) { return false; }

    if (HasServerAuthority())
    {
        return PickupInternal(Pickup);
    }

    const AActor* Owner = GetOwner();
    if (!Owner || !Owner->HasLocalNetOwner()) { return false; }

    Server_PickupActor(Pickup);
    return true;
}

bool UItemContainerComponent::PickupNearby(float Radius)
{
    if (HasServerAuthority())
    {
        return PickupNearbyInternal(Radius);
    }

    const AActor* Owner = GetOwner();
    if (!Owner || !Owner->HasLocalNetOwner()) { return false; }

    Server_PickupNearby(Radius);
    return true;
}

void UItemContainerComponent::Server_PickupActor_Implementation(AActor* Target)
{
    if (AItemPickup* Pickup = Cast<AItemPickup>(Target))
    {
        PickupInternal(Pickup);
    }
}

void UItemContainerComponent::Server_PickupNearby_Implementation(float Radius)
{
    PickupNearbyInternal(Radius);
}

bool UItemContainerComponent::PickupInternal(AItemPickup* Pickup)
{
    if (!IsValid(Pickup) || !Pickup->Item || Pickup->Count <= 0) { return false; }

    AActor* Reference = GetReferenceActor();
    if (MaxPickupDistance > 0.f && Reference &&
        FVector::DistSquared(Reference->GetActorLocation(), Pickup->GetActorLocation()) > FMath::Square(MaxPickupDistance))
    {
        return false;
    }
    if (!Pickup->CanBePickedUpBy(Reference)) { return false; }

    TArray<int32> Changed;
    const int32 Left = AddInternal(Pickup->Item, Pickup->Count, Changed, INDEX_NONE);
    const int32 Taken = Pickup->Count - Left;
    NotifyChanged(Changed);

    if (Taken > 0)
    {
        Pickup->Take(Taken);
    }
    return Taken > 0;
}

bool UItemContainerComponent::PickupNearbyInternal(float Radius)
{
    AActor* Reference = GetReferenceActor();
    UWorld* World = GetWorld();
    if (!Reference || !World) { return false; }

    if (MaxPickupDistance > 0.f) { Radius = FMath::Min(Radius, MaxPickupDistance); }

    AItemPickup* Best = nullptr;
    float BestDistSq = FMath::Square(Radius);
    for (TActorIterator<AItemPickup> It(World); It; ++It)
    {
        AItemPickup* Candidate = *It;
        if (!IsValid(Candidate) || !Candidate->Item) { continue; }

        const float DistSq = FVector::DistSquared(Reference->GetActorLocation(), Candidate->GetActorLocation());
        if (DistSq <= BestDistSq)
        {
            BestDistSq = DistSq;
            Best = Candidate;
        }
    }
    return Best && PickupInternal(Best);
}

// ====================================================================================================
//  Selection
// ====================================================================================================

bool UItemContainerComponent::SelectSlot(int32 SlotIndex)
{
    if (!bAllowSelection || !IsValidSlot(SlotIndex)) { return false; }

    if (HasServerAuthority())
    {
        return SelectInternal(SlotIndex);
    }

    const AActor* Owner = GetOwner();
    if (!Owner || !Owner->HasLocalNetOwner()) { return false; }

    Server_SelectSlot(SlotIndex);
    return true;
}

void UItemContainerComponent::Server_SelectSlot_Implementation(int32 SlotIndex)
{
    if (bAllowSelection)
    {
        SelectInternal(SlotIndex);
    }
}

bool UItemContainerComponent::SelectInternal(int32 SlotIndex)
{
    if (!IsValidSlot(SlotIndex)) { return false; }
    if (SlotIndex == SelectedSlot) { return true; }

    const int32 Old = SelectedSlot;
    SelectedSlot = SlotIndex;
    OnSelectionChanged.Broadcast(Old, SelectedSlot);
    return true;
}

bool UItemContainerComponent::SelectNext()
{
    const int32 Num = GetNumSlots();
    return Num > 0 && SelectSlot((SelectedSlot + 1) % Num);
}

bool UItemContainerComponent::SelectPrevious()
{
    const int32 Num = GetNumSlots();
    return Num > 0 && SelectSlot((SelectedSlot - 1 + Num) % Num);
}
