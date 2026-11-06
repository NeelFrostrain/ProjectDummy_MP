#include "ItemSlotWidget.h"
#include "ItemContainerComponent.h"
#include "ItemDefinition.h"
#include "ItemDragDropOperation.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"
#include "InputCoreTypes.h"

void UItemSlotWidget::NativeDestruct()
{
    SetSlot(nullptr, INDEX_NONE);
    Super::NativeDestruct();
}

void UItemSlotWidget::SetSlot(UItemContainerComponent* InContainer, int32 InSlotIndex)
{
    if (UItemContainerComponent* Old = Container.Get())
    {
        Old->OnSlotChanged.RemoveDynamic(this, &UItemSlotWidget::HandleSlotChanged);
        Old->OnSelectionChanged.RemoveDynamic(this, &UItemSlotWidget::HandleSelectionChanged);
    }

    Container = InContainer;
    SlotIndex = InSlotIndex;

    if (InContainer)
    {
        InContainer->OnSlotChanged.AddUniqueDynamic(this, &UItemSlotWidget::HandleSlotChanged);
        InContainer->OnSelectionChanged.AddUniqueDynamic(this, &UItemSlotWidget::HandleSelectionChanged);
    }

    RefreshSlot();
}

void UItemSlotWidget::HandleSlotChanged(int32 ChangedIndex, UItemDefinition* ChangedItem, int32 ChangedCount)
{
    if (ChangedIndex == SlotIndex)
    {
        RefreshSlot();
    }
}

void UItemSlotWidget::HandleSelectionChanged(int32 OldSlot, int32 NewSlot)
{
    if (OldSlot == SlotIndex || NewSlot == SlotIndex)
    {
        RefreshSlot();
    }
}

void UItemSlotWidget::RefreshSlot()
{
    Item = nullptr;
    Count = 0;
    bSelected = false;

    if (const UItemContainerComponent* C = Container.Get())
    {
        const FItemSlot Slot = C->GetSlot(SlotIndex);
        if (!Slot.IsEmpty())
        {
            Item = Slot.Item;
            Count = Slot.Count;
        }
        bSelected = C->bAllowSelection && C->GetSelectedSlot() == SlotIndex;
    }

    if (IconImage)
    {
        if (Item && Item->Icon)
        {
            IconImage->SetBrushFromTexture(Item->Icon);
            IconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
        else
        {
            IconImage->SetVisibility(ESlateVisibility::Hidden);
        }
    }

    if (CountText)
    {
        if (Item && Count > 1)
        {
            CountText->SetText(FText::AsNumber(Count));
            CountText->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
        else
        {
            CountText->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    if (KeyText)
    {
        if (bShowSlotNumber && SlotIndex >= 0)
        {
            KeyText->SetText(FText::AsNumber(SlotIndex + 1));
            KeyText->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
        else
        {
            KeyText->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    if (SelectionHighlight)
    {
        SelectionHighlight->SetVisibility(bSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
    }

    OnSlotRefreshed(Item, Count, bSelected);
}

FReply UItemSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    UItemContainerComponent* C = Container.Get();
    if (!C)
    {
        return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
    }

    const FKey Button = InMouseEvent.GetEffectingButton();

    if (Button == EKeys::LeftMouseButton)
    {
        if (InMouseEvent.IsShiftDown() && QuickTransferTarget && Item)
        {
            C->MoveItem(SlotIndex, QuickTransferTarget, -1, 0);
            return FReply::Handled();
        }

        if (C->bAllowSelection)
        {
            C->SelectSlot(SlotIndex);
        }

        if (Item)
        {
            return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
        }
        return FReply::Handled();
    }

    if (Button == EKeys::RightMouseButton && bRightClickToUse && Item)
    {
        C->UseItem(SlotIndex);
        return FReply::Handled();
    }

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UItemSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
    UItemContainerComponent* C = Container.Get();
    if (!C || !Item || Count <= 0) { return; }

    UItemDragDropOperation* Operation = NewObject<UItemDragDropOperation>(GetTransientPackage());
    Operation->SourceContainer = C;
    Operation->SourceSlot = SlotIndex;
    Operation->Item = Item;
    Operation->Count = InMouseEvent.IsControlDown() ? FMath::Max(1, (Count + 1) / 2) : Count;
    Operation->bDropToWorldIfCancelled = bDropToWorldIfCancelled;
    Operation->Pivot = EDragPivot::CenterCenter;

    if (Item->Icon)
    {
        UImage* Visual = NewObject<UImage>(this);
        Visual->SetBrushFromTexture(Item->Icon);
        Visual->SetDesiredSizeOverride(DragIconSize);
        Visual->SetVisibility(ESlateVisibility::HitTestInvisible); // must not block the drop target underneath
        Operation->DefaultDragVisual = Visual;
    }

    OutOperation = Operation;
}

bool UItemSlotWidget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    if (Cast<UItemDragDropOperation>(InOperation))
    {
        return true;
    }
    return Super::NativeOnDragOver(InGeometry, InDragDropEvent, InOperation);
}

bool UItemSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    UItemDragDropOperation* Operation = Cast<UItemDragDropOperation>(InOperation);
    UItemContainerComponent* C = Container.Get();
    if (!Operation || !C || !Operation->SourceContainer)
    {
        return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
    }

    if (Operation->SourceContainer == C && Operation->SourceSlot == SlotIndex)
    {
        return true;
    }

    Operation->SourceContainer->MoveItem(Operation->SourceSlot, C, SlotIndex, Operation->Count);
    return true;
}
