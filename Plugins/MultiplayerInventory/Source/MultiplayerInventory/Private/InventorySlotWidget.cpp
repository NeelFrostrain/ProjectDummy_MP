#include "InventorySlotWidget.h"
#include "InventoryDragDropOperation.h"
#include "InventoryItemData.h"
#include "GameFramework/Pawn.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Blueprint/WidgetTree.h"

void UInventorySlotWidget::NativeConstruct()
{
    Super::NativeConstruct();
    AutoFindWidgets();
    RefreshSlot();
}

void UInventorySlotWidget::AutoFindWidgets()
{
    if (!WidgetTree) return;

    TArray<UWidget*> AllWidgets;
    WidgetTree->GetAllWidgets(AllWidgets);

    for (UWidget* W : AllWidgets)
    {
        if (!W) continue;
        const FString Name = W->GetName().ToLower();

        if (!ItemIcon)
        {
            if (UImage* Img = Cast<UImage>(W))
            {
                if (Name.Contains(TEXT("icon")) || Name.Contains(TEXT("item")) || Name.Contains(TEXT("pic")) || Name.Contains(TEXT("thumb")))
                {
                    ItemIcon = Img;
                }
            }
        }

        if (!QuantityText)
        {
            if (UTextBlock* Txt = Cast<UTextBlock>(W))
            {
                if (Name.Contains(TEXT("quantity")) || Name.Contains(TEXT("qty")) || Name.Contains(TEXT("count")) || Name.Contains(TEXT("amount")))
                {
                    QuantityText = Txt;
                }
            }
        }

        if (!KeyNumberText)
        {
            if (UTextBlock* Txt = Cast<UTextBlock>(W))
            {
                if (Name.Contains(TEXT("key")) || Name.Contains(TEXT("num")) || Name.Contains(TEXT("index")) || Name.Contains(TEXT("slotnum")))
                {
                    KeyNumberText = Txt;
                }
            }
        }

        if (!HighlightBorder)
        {
            if (UBorder* Border = Cast<UBorder>(W))
            {
                if (Name.Contains(TEXT("highlight")) || Name.Contains(TEXT("select")) || Name.Contains(TEXT("active")))
                {
                    HighlightBorder = Border;
                }
            }
        }
    }

    // Fallback: If ItemIcon is still null, take any image that is not explicitly a background or frame
    if (!ItemIcon)
    {
        for (UWidget* W : AllWidgets)
        {
            if (UImage* Img = Cast<UImage>(W))
            {
                const FString Name = Img->GetName().ToLower();
                if (!Name.Contains(TEXT("bg")) && !Name.Contains(TEXT("background")) && !Name.Contains(TEXT("frame")) && !Name.Contains(TEXT("border")))
                {
                    ItemIcon = Img;
                    break;
                }
            }
        }
    }
}

void UInventorySlotWidget::SetupSlot(UInventoryComponent* InComp, EInventoryContainer InContainer, int32 InIndex)
{
    InventoryComponent = InComp;
    Container = InContainer;
    SlotIndex = InIndex;
    AutoFindWidgets();
    RefreshSlot();
}

void UInventorySlotWidget::RefreshSlot()
{
    if (!ItemIcon || !QuantityText || !KeyNumberText || !HighlightBorder)
    {
        AutoFindWidgets();
    }

    if (InventoryComponent.IsValid())
    {
        UInventoryItemData* NewData = nullptr;
        int32 NewQty = 0;
        InventoryComponent->GetSlot(Container, SlotIndex, NewData, NewQty);

        CurrentItemData = NewData;
        CurrentQuantity = NewQty;
        bIsSelected = (Container == EInventoryContainer::Hotbar && InventoryComponent->GetSelectedHotbarIndex() == SlotIndex);
    }
    else
    {
        CurrentItemData = nullptr;
        CurrentQuantity = 0;
        bIsSelected = false;
    }

    // Auto-update Item Icon
    if (ItemIcon)
    {
        if (CurrentItemData && CurrentQuantity > 0 && CurrentItemData->Icon)
        {
            ItemIcon->SetBrushFromTexture(CurrentItemData->Icon, true);
            ItemIcon->SetVisibility(ESlateVisibility::Visible);
        }
        else
        {
            ItemIcon->SetVisibility(ESlateVisibility::Hidden);
        }
    }

    // Auto-update Quantity Text
    if (QuantityText)
    {
        if (CurrentItemData && CurrentQuantity > 1)
        {
            QuantityText->SetText(FText::AsNumber(CurrentQuantity));
            QuantityText->SetVisibility(ESlateVisibility::Visible);
        }
        else
        {
            QuantityText->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    // Auto-update Key Number Text (1-based display for hotbar, e.g. Slot 0 shows "1")
    if (KeyNumberText)
    {
        if (Container == EInventoryContainer::Hotbar && SlotIndex >= 0)
        {
            KeyNumberText->SetText(FText::AsNumber(SlotIndex + 1));
            KeyNumberText->SetVisibility(ESlateVisibility::Visible);
        }
        else
        {
            KeyNumberText->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    // Auto-update Highlight Border
    if (HighlightBorder)
    {
        HighlightBorder->SetVisibility(bIsSelected ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    }

    BP_OnSlotUpdated(CurrentItemData, CurrentQuantity, bIsSelected);
}

void UInventorySlotWidget::SelectThisSlot()
{
    if (InventoryComponent.IsValid() && Container == EInventoryContainer::Hotbar)
    {
        InventoryComponent->RequestSelectHotbar(SlotIndex);
    }
}

void UInventorySlotWidget::DropSlotItem(int32 Amount)
{
    if (InventoryComponent.IsValid())
    {
        InventoryComponent->RequestDrop(Container, SlotIndex, Amount);
    }
}

bool UInventorySlotWidget::RemoveItem(int32 Amount)
{
    if (InventoryComponent.IsValid())
    {
        return InventoryComponent->RemoveSlotItem(Container, SlotIndex, Amount);
    }
    return false;
}

bool UInventorySlotWidget::ClearSlot()
{
    if (InventoryComponent.IsValid())
    {
        return InventoryComponent->ClearSlot(Container, SlotIndex);
    }
    return false;
}

FReply UInventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        if (Container == EInventoryContainer::Hotbar)
        {
            SelectThisSlot();
        }

        if (CurrentItemData && CurrentQuantity > 0)
        {
            return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
        }
        return FReply::Handled();
    }
    else if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        if (CurrentItemData && CurrentQuantity > 0 && InventoryComponent.IsValid())
        {
            if (InMouseEvent.IsShiftDown() && CurrentQuantity > 1)
            {
                const int32 SplitAmount = CurrentQuantity / 2;
                const int32 MaxSlots = InventoryComponent->GetSlotCount(Container);
                for (int32 i = 0; i < MaxSlots; ++i)
                {
                    UInventoryItemData* OtherData = nullptr;
                    int32 OtherQty = 0;
                    if (InventoryComponent->GetSlot(Container, i, OtherData, OtherQty) && (!OtherData || OtherQty == 0))
                    {
                        InventoryComponent->RequestSplit(Container, SlotIndex, i, SplitAmount);
                        break;
                    }
                }
            }
            else
            {
                InventoryComponent->UseSlotItem(Container, SlotIndex);
            }
        }
        return FReply::Handled();
    }

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UInventorySlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
    if (!CurrentItemData || CurrentQuantity <= 0) return;

    UInventoryDragDropOperation* DragOp = NewObject<UInventoryDragDropOperation>(GetTransientPackage());
    if (!DragOp) return;

    DragOp->SourceContainer = Container;
    DragOp->SourceIndex = SlotIndex;
    DragOp->ItemData = CurrentItemData;
    DragOp->Quantity = CurrentQuantity;
    DragOp->SourceInventoryComponent = InventoryComponent.Get();

    // Create a lightweight visual for dragging that is HitTestInvisible so it does not intercept drop events on target slots!
    if (CurrentItemData->Icon)
    {
        UImage* DragVisual = NewObject<UImage>(this);
        if (DragVisual)
        {
            DragVisual->SetBrushFromTexture(CurrentItemData->Icon, true);
            DragVisual->SetDesiredSizeOverride(FVector2D(64.f, 64.f));
            DragVisual->SetVisibility(ESlateVisibility::HitTestInvisible);
            DragOp->DefaultDragVisual = DragVisual;
        }
    }

    DragOp->Pivot = EDragPivot::CenterCenter;
    OutOperation = DragOp;
}

bool UInventorySlotWidget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    if (Cast<UInventoryDragDropOperation>(InOperation))
    {
        return true;
    }
    return Super::NativeOnDragOver(InGeometry, InDragDropEvent, InOperation);
}

bool UInventorySlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    UInventoryDragDropOperation* DragOp = Cast<UInventoryDragDropOperation>(InOperation);
    if (!DragOp)
    {
        return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
    }

    // Dropped on the same slot - do nothing
    if (DragOp->SourceContainer == Container && DragOp->SourceIndex == SlotIndex)
    {
        return true;
    }

    UInventoryComponent* Comp = InventoryComponent.Get();
    if (!Comp && GetOwningPlayerPawn())
    {
        Comp = GetOwningPlayerPawn()->FindComponentByClass<UInventoryComponent>();
        if (Comp)
        {
            InventoryComponent = Comp;
        }
    }

    if (Comp)
    {
        Comp->RequestMove(DragOp->SourceContainer, DragOp->SourceIndex, Container, SlotIndex);
        return true;
    }

    return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
}
