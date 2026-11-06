#include "ItemContainerWidget.h"
#include "ItemContainerComponent.h"
#include "ItemLibrary.h"
#include "ItemSlotWidget.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"

void UItemContainerWidget::NativeConstruct()
{
    Super::NativeConstruct();
    TryAutoFind();
}

void UItemContainerWidget::NativeDestruct()
{
    SetContainer(nullptr);
    Super::NativeDestruct();
}

void UItemContainerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // The pawn (and its container) may not exist yet, or may be replaced after respawn: keep looking.
    if (bAutoFindContainer && !Container.IsValid())
    {
        FindTimer += InDeltaTime;
        if (FindTimer >= 0.25f)
        {
            FindTimer = 0.f;
            TryAutoFind();
        }
    }
}

void UItemContainerWidget::TryAutoFind()
{
    if (!bAutoFindContainer || Container.IsValid()) { return; }

    if (UItemContainerComponent* Found = UItemLibrary::FindItemContainer(GetOwningPlayer(), AutoFindContainerName))
    {
        SetContainer(Found);
    }
}

void UItemContainerWidget::SetContainer(UItemContainerComponent* NewContainer)
{
    if (UItemContainerComponent* Old = Container.Get())
    {
        Old->OnContainerChanged.RemoveDynamic(this, &UItemContainerWidget::HandleContainerChanged);
    }

    Container = NewContainer;

    if (NewContainer)
    {
        NewContainer->OnContainerChanged.AddUniqueDynamic(this, &UItemContainerWidget::HandleContainerChanged);
    }

    if (TitleText)
    {
        TitleText->SetText(NewContainer ? NewContainer->ContainerTitle : FText::GetEmpty());
    }

    Rebuild();
    OnContainerSet(NewContainer);
}

void UItemContainerWidget::SetQuickTransferTarget(UItemContainerComponent* Target)
{
    QuickTransferTarget = Target;
    for (UItemSlotWidget* SlotWidget : SlotWidgets)
    {
        if (SlotWidget) { SlotWidget->QuickTransferTarget = Target; }
    }
}

void UItemContainerWidget::BindExistingSlots(const TArray<UItemSlotWidget*>& Widgets)
{
    // Remove anything we generated before.
    if (!bManualSlots)
    {
        for (UItemSlotWidget* SlotWidget : SlotWidgets)
        {
            if (SlotWidget)
            {
                SlotWidget->SetSlot(nullptr, INDEX_NONE);
                SlotWidget->RemoveFromParent();
            }
        }
    }

    bManualSlots = true;
    SlotWidgets.Reset();
    for (UItemSlotWidget* SlotWidget : Widgets)
    {
        SlotWidgets.Add(SlotWidget);
    }
    Rebuild();
}

void UItemContainerWidget::Rebuild()
{
    UItemContainerComponent* C = Container.Get();

    if (bManualSlots)
    {
        for (int32 i = 0; i < SlotWidgets.Num(); ++i)
        {
            if (UItemSlotWidget* SlotWidget = SlotWidgets[i])
            {
                SlotWidget->bShowSlotNumber = bShowSlotNumbers;
                SlotWidget->QuickTransferTarget = QuickTransferTarget;
                SlotWidget->SetSlot(C, C ? i : INDEX_NONE);
            }
        }
        return;
    }

    // Remove old generated slot widgets.
    for (UItemSlotWidget* SlotWidget : SlotWidgets)
    {
        if (SlotWidget)
        {
            SlotWidget->SetSlot(nullptr, INDEX_NONE);
            SlotWidget->RemoveFromParent();
        }
    }
    SlotWidgets.Reset();

    if (!C || !SlotPanel || !SlotWidgetClass) { return; }

    UUniformGridPanel* Grid = Cast<UUniformGridPanel>(SlotPanel);
    const int32 Cols = FMath::Max(1, Columns);

    const int32 Num = C->GetNumSlots();
    for (int32 i = 0; i < Num; ++i)
    {
        UItemSlotWidget* SlotWidget = CreateWidget<UItemSlotWidget>(this, SlotWidgetClass);
        if (!SlotWidget) { continue; }

        SlotWidget->bShowSlotNumber = bShowSlotNumbers;
        SlotWidget->QuickTransferTarget = QuickTransferTarget;
        SlotWidget->SetSlot(C, i);

        if (Grid) { Grid->AddChildToUniformGrid(SlotWidget, i / Cols, i % Cols); }
        else      { SlotPanel->AddChild(SlotWidget); }

        SlotWidgets.Add(SlotWidget);
    }
}

void UItemContainerWidget::HandleContainerChanged()
{
    // Only the slot COUNT matters here: each slot widget updates itself.
    const UItemContainerComponent* C = Container.Get();
    if (!bManualSlots && C && C->GetNumSlots() != SlotWidgets.Num())
    {
        Rebuild();
    }
}
