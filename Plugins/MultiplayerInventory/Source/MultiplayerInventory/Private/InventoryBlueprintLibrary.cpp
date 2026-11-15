#include "InventoryBlueprintLibrary.h"
#include "InventorySlotWidget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

UInventoryComponent* UInventoryBlueprintLibrary::GetPlayerInventoryComponent(const AActor* ContextActor)
{
    if (!ContextActor) return nullptr;

    if (UInventoryComponent* Comp = ContextActor->FindComponentByClass<UInventoryComponent>())
    {
        return Comp;
    }

    if (const APlayerController* PC = Cast<APlayerController>(ContextActor))
    {
        if (APawn* Pawn = PC->GetPawn())
        {
            if (UInventoryComponent* Comp = Pawn->FindComponentByClass<UInventoryComponent>())
            {
                return Comp;
            }
        }
    }
    else if (const APawn* Pawn = Cast<APawn>(ContextActor))
    {
        if (APlayerState* PS = Pawn->GetPlayerState())
        {
            if (UInventoryComponent* Comp = PS->FindComponentByClass<UInventoryComponent>())
            {
                return Comp;
            }
        }
    }

    return nullptr;
}

bool UInventoryBlueprintLibrary::IsSlotEmpty(const FInventorySlot& Slot)
{
    return !Slot.ItemData || Slot.Quantity <= 0;
}

void UInventoryBlueprintLibrary::BreakInventorySlot(const FInventorySlot& Slot, int32& OutSlotIndex, UInventoryItemData*& OutItemData, int32& OutQuantity, bool& bOutIsEmpty)
{
    OutSlotIndex = Slot.SlotIndex;
    OutItemData = Slot.ItemData;
    OutQuantity = Slot.Quantity;
    bOutIsEmpty = !Slot.ItemData || Slot.Quantity <= 0;
}

FInventorySlot UInventoryBlueprintLibrary::MakeInventorySlot(int32 SlotIndex, UInventoryItemData* ItemData, int32 Quantity)
{
    FInventorySlot Slot;
    Slot.SlotIndex = SlotIndex;
    Slot.ItemData = ItemData;
    Slot.Quantity = Quantity;
    return Slot;
}

FText UInventoryBlueprintLibrary::FormatItemQuantity(int32 Quantity)
{
    if (Quantity >= 1000000)
    {
        const float Value = Quantity / 1000000.f;
        return FText::Format(FText::FromString(TEXT("{0}M")), FText::AsNumber(FMath::RoundToFloat(Value * 10.f) / 10.f));
    }
    if (Quantity >= 1000)
    {
        const float Value = Quantity / 1000.f;
        return FText::Format(FText::FromString(TEXT("{0}K")), FText::AsNumber(FMath::RoundToFloat(Value * 10.f) / 10.f));
    }
    return FText::AsNumber(Quantity);
}

FText UInventoryBlueprintLibrary::GetItemDisplayName(const UInventoryItemData* ItemData)
{
    return ItemData ? ItemData->DisplayName : FText::GetEmpty();
}

FText UInventoryBlueprintLibrary::GetItemDescription(const UInventoryItemData* ItemData)
{
    return ItemData ? ItemData->Description : FText::GetEmpty();
}

UTexture2D* UInventoryBlueprintLibrary::GetItemIcon(const UInventoryItemData* ItemData)
{
    return ItemData ? ItemData->Icon : nullptr;
}

int32 UInventoryBlueprintLibrary::GetItemMaxStack(const UInventoryItemData* ItemData)
{
    return ItemData ? ItemData->MaxStackSize : 0;
}

TArray<FInventorySlot> UInventoryBlueprintLibrary::FilterSlotsByItem(const TArray<FInventorySlot>& Slots, const UInventoryItemData* ItemData)
{
    TArray<FInventorySlot> Result;
    for (const FInventorySlot& Slot : Slots)
    {
        if (Slot.ItemData == ItemData && Slot.Quantity > 0)
        {
            Result.Add(Slot);
        }
    }
    return Result;
}

int32 UInventoryBlueprintLibrary::GetTotalQuantityInSlots(const TArray<FInventorySlot>& Slots)
{
    int32 Total = 0;
    for (const FInventorySlot& Slot : Slots)
    {
        if (Slot.ItemData && Slot.Quantity > 0)
        {
            Total += Slot.Quantity;
        }
    }
    return Total;
}

TArray<int32> UInventoryBlueprintLibrary::FindEmptySlotIndices(const TArray<FInventorySlot>& Slots)
{
    TArray<int32> Indices;
    for (int32 i = 0; i < Slots.Num(); ++i)
    {
        if (!Slots[i].ItemData || Slots[i].Quantity <= 0)
        {
            Indices.Add(i);
        }
    }
    return Indices;
}

TArray<UInventorySlotWidget*> UInventoryBlueprintLibrary::SetupSlotWidgets(UInventoryComponent* InventoryComponent, EInventoryContainer Container, const TArray<UInventorySlotWidget*>& InWidgets)
{
    if (InventoryComponent)
    {
        return InventoryComponent->SetupSlotWidgets(Container, InWidgets);
    }
    return InWidgets;
}

FGameplayTag UInventoryBlueprintLibrary::GetEventTag_Selected() { return InventoryItemEventTags::Selected.GetTag(); }
FGameplayTag UInventoryBlueprintLibrary::GetEventTag_Unselected() { return InventoryItemEventTags::Unselected.GetTag(); }
FGameplayTag UInventoryBlueprintLibrary::GetEventTag_PickedUp() { return InventoryItemEventTags::PickedUp.GetTag(); }
FGameplayTag UInventoryBlueprintLibrary::GetEventTag_Dropped() { return InventoryItemEventTags::Dropped.GetTag(); }
FGameplayTag UInventoryBlueprintLibrary::GetEventTag_Added() { return InventoryItemEventTags::Added.GetTag(); }
FGameplayTag UInventoryBlueprintLibrary::GetEventTag_Removed() { return InventoryItemEventTags::Removed.GetTag(); }
FGameplayTag UInventoryBlueprintLibrary::GetEventTag_Used() { return InventoryItemEventTags::Used.GetTag(); }
FGameplayTag UInventoryBlueprintLibrary::GetEventTag_Equipped() { return InventoryItemEventTags::Equipped.GetTag(); }
FGameplayTag UInventoryBlueprintLibrary::GetEventTag_Unequipped() { return InventoryItemEventTags::Unequipped.GetTag(); }
FGameplayTag UInventoryBlueprintLibrary::GetEventTag_Consumed() { return InventoryItemEventTags::Consumed.GetTag(); }
