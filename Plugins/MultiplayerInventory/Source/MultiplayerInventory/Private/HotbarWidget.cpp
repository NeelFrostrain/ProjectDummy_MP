#include "HotbarWidget.h"
#include "InventorySlotWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Blueprint/WidgetTree.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

void UHotbarWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (!InventoryComponent.IsValid())
    {
        APawn* Pawn = GetOwningPlayerPawn();
        if (!Pawn)
        {
            Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
        }
        if (Pawn)
        {
            if (UInventoryComponent* Comp = Pawn->FindComponentByClass<UInventoryComponent>())
            {
                InitializeWithComponent(Comp);
                return;
            }
        }
    }

    AutoDiscoverChildSlots();
    AutoBindControls();
    RefreshAll();
}

void UHotbarWidget::NativeDestruct()
{
    if (InventoryComponent.IsValid())
    {
        InventoryComponent->OnInventoryChanged.RemoveAll(this);
        InventoryComponent->OnHotbarSelectionChanged.RemoveAll(this);
    }
    InventoryComponent = nullptr;
    SlotWidgets.Empty();
    Super::NativeDestruct();
}

void UHotbarWidget::AutoDiscoverChildSlots()
{
    if (!WidgetTree) return;

    TArray<UWidget*> AllWidgets;
    WidgetTree->GetAllWidgets(AllWidgets);

    TArray<UInventorySlotWidget*> FoundSlots;

    // Prefer slots placed inside a container panel (HorizontalBox, GridPanel, etc.)
    for (UWidget* W : AllWidgets)
    {
        if (UPanelWidget* Panel = Cast<UPanelWidget>(W))
        {
            TArray<UInventorySlotWidget*> PanelSlots;
            for (int32 i = 0; i < Panel->GetChildrenCount(); ++i)
            {
                if (UInventorySlotWidget* SlotWidget = Cast<UInventorySlotWidget>(Panel->GetChildAt(i)))
                {
                    PanelSlots.Add(SlotWidget);
                }
            }
            if (PanelSlots.Num() > FoundSlots.Num())
            {
                FoundSlots = MoveTemp(PanelSlots);
            }
        }
    }

    // Fallback: collect all slot widgets in widget tree if no panel held them
    if (FoundSlots.IsEmpty())
    {
        for (UWidget* W : AllWidgets)
        {
            if (UInventorySlotWidget* SlotWidget = Cast<UInventorySlotWidget>(W))
            {
                FoundSlots.Add(SlotWidget);
            }
        }

        FoundSlots.Sort([](const UInventorySlotWidget& A, const UInventorySlotWidget& B) {
            return A.GetName() < B.GetName();
        });
    }

    if (!FoundSlots.IsEmpty())
    {
        SlotWidgets.Empty();
        for (int32 i = 0; i < FoundSlots.Num(); ++i)
        {
            UInventorySlotWidget* SlotWidget = FoundSlots[i];
            SlotWidget->Container = Container;
            SlotWidget->SlotIndex = i;
            SlotWidgets.Add(SlotWidget);

            if (InventoryComponent.IsValid())
            {
                SlotWidget->SetupSlot(InventoryComponent.Get(), Container, i);
            }
        }
    }
}

void UHotbarWidget::AutoBindControls()
{
    if (!WidgetTree) return;

    TArray<UWidget*> AllWidgets;
    WidgetTree->GetAllWidgets(AllWidgets);

    for (UWidget* W : AllWidgets)
    {
        if (!W) continue;
        const FString Name = W->GetName().ToLower();

        if (!CapacityText)
        {
            if (UTextBlock* Txt = Cast<UTextBlock>(W))
            {
                if (Name.Contains(TEXT("capacity")) || Name.Contains(TEXT("count")))
                {
                    CapacityText = Txt;
                }
            }
        }

        if (!SortButton)
        {
            if (UButton* Btn = Cast<UButton>(W))
            {
                if (Name.Contains(TEXT("sort")))
                {
                    SortButton = Btn;
                    SortButton->OnClicked.RemoveDynamic(this, &UHotbarWidget::SortItems);
                    SortButton->OnClicked.AddDynamic(this, &UHotbarWidget::SortItems);
                }
            }
        }

        if (!DropAllButton)
        {
            if (UButton* Btn = Cast<UButton>(W))
            {
                if (Name.Contains(TEXT("drop")))
                {
                    DropAllButton = Btn;
                    DropAllButton->OnClicked.RemoveDynamic(this, &UHotbarWidget::DropAllItems);
                    DropAllButton->OnClicked.AddDynamic(this, &UHotbarWidget::DropAllItems);
                }
            }
        }
    }
}

void UHotbarWidget::InitializeWithComponent(UInventoryComponent* InComp)
{
    if (InventoryComponent.IsValid())
    {
        InventoryComponent->OnInventoryChanged.RemoveDynamic(this, &UHotbarWidget::HandleInventoryChanged);
        InventoryComponent->OnHotbarSelectionChanged.RemoveDynamic(this, &UHotbarWidget::HandleSelectionChanged);
    }

    InventoryComponent = InComp;

    if (InventoryComponent.IsValid())
    {
        InventoryComponent->OnInventoryChanged.AddDynamic(this, &UHotbarWidget::HandleInventoryChanged);
        InventoryComponent->OnHotbarSelectionChanged.AddDynamic(this, &UHotbarWidget::HandleSelectionChanged);

        if (SlotWidgets.IsEmpty())
        {
            AutoDiscoverChildSlots();
        }
        else
        {
            for (int32 i = 0; i < SlotWidgets.Num(); ++i)
            {
                if (SlotWidgets[i])
                {
                    SlotWidgets[i]->SetupSlot(InventoryComponent.Get(), Container, i);
                }
            }
        }

        AutoBindControls();
        RefreshAll();
    }
}

void UHotbarWidget::RegisterSlotWidget(UInventorySlotWidget* SlotWidget, int32 SlotIndex)
{
    if (!SlotWidget || SlotIndex < 0) return;

    if (SlotWidgets.Num() <= SlotIndex)
    {
        SlotWidgets.SetNum(SlotIndex + 1);
    }
    SlotWidgets[SlotIndex] = SlotWidget;
    SlotWidget->Container = Container;
    SlotWidget->SlotIndex = SlotIndex;

    if (InventoryComponent.IsValid())
    {
        SlotWidget->SetupSlot(InventoryComponent.Get(), Container, SlotIndex);
    }
}

void UHotbarWidget::RefreshAll()
{
    if (!InventoryComponent.IsValid()) return;

    for (int32 i = 0; i < SlotWidgets.Num(); ++i)
    {
        if (SlotWidgets[i])
        {
            SlotWidgets[i]->RefreshSlot();
        }
    }

    if (CapacityText)
    {
        CapacityText->SetText(GetCapacityText());
    }

    const int32 Used = InventoryComponent->GetUsedSlotCount(Container);
    const int32 Max = InventoryComponent->GetSlotCount(Container);
    BP_OnInventoryRefreshed(Used, Max);
}

void UHotbarWidget::HandleInventoryChanged()
{
    RefreshAll();
}

void UHotbarWidget::HandleSelectionChanged(int32 NewIndex)
{
    if (Container == EInventoryContainer::Hotbar)
    {
        for (int32 i = 0; i < SlotWidgets.Num(); ++i)
        {
            if (SlotWidgets[i])
            {
                SlotWidgets[i]->RefreshSlot();
            }
        }
    }
    BP_OnHotbarSelectionChanged(NewIndex);
}

void UHotbarWidget::SortItems()
{
    if (InventoryComponent.IsValid())
    {
        InventoryComponent->RequestSort(Container);
    }
}

void UHotbarWidget::DropAllItems()
{
    if (InventoryComponent.IsValid())
    {
        InventoryComponent->RequestDropAll(Container);
    }
}

FText UHotbarWidget::GetCapacityText() const
{
    if (!InventoryComponent.IsValid()) return FText::FromString(TEXT("0 / 0"));
    const int32 Used = InventoryComponent->GetUsedSlotCount(Container);
    const int32 Max = InventoryComponent->GetSlotCount(Container);
    return FText::Format(FText::FromString(TEXT("{0} / {1}")), FText::AsNumber(Used), FText::AsNumber(Max));
}
