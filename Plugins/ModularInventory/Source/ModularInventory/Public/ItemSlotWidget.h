#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemSlotWidget.generated.h"

class UItemContainerComponent;
class UItemDefinition;
class UImage;
class UTextBlock;
class UWidget;

/**
 * Shows ONE slot of ANY container. Make a Blueprint child, design it, and (optionally) add widgets with these exact names:
 *   IconImage (Image), CountText (Text), KeyText (Text), SelectionHighlight (any widget).
 * All four are optional. For fully custom visuals use the "On Slot Refreshed" event instead.
 *
 * Built-in behaviour: drag & drop between any slots, right-click = use, Shift+click = quick move to another container,
 * click = select (if the container allows selection), Ctrl+drag = drag half the stack.
 */
UCLASS(Abstract, Blueprintable)
class MODULARINVENTORY_API UItemSlotWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> IconImage;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> CountText;

    /** Shows the slot number (1, 2, 3...) when Show Slot Number is on. Handy for hotbars. */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> KeyText;

    /** Shown only while this slot is the container's selected slot. */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UWidget> SelectionHighlight;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    bool bShowSlotNumber = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    bool bRightClickToUse = true;

    /** Dropping the item on nothing throws it into the world. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    bool bDropToWorldIfCancelled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    FVector2D DragIconSize = FVector2D(64.f, 64.f);

    /** Shift+click sends the item here (set by the container widget, or call Set Quick Transfer Target). */
    UPROPERTY(BlueprintReadWrite, Category = "Slot")
    TObjectPtr<UItemContainerComponent> QuickTransferTarget = nullptr;

    // --- Read-only state, updated automatically ---

    UPROPERTY(BlueprintReadOnly, Category = "Slot")
    TWeakObjectPtr<UItemContainerComponent> Container;

    UPROPERTY(BlueprintReadOnly, Category = "Slot")
    int32 SlotIndex = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly, Transient, Category = "Slot")
    TObjectPtr<UItemDefinition> Item = nullptr;

    UPROPERTY(BlueprintReadOnly, Transient, Category = "Slot")
    int32 Count = 0;

    UPROPERTY(BlueprintReadOnly, Transient, Category = "Slot")
    bool bSelected = false;

    /** Connects this widget to slot "SlotIndex" of "Container". Call with an empty container to disconnect. */
    UFUNCTION(BlueprintCallable, Category = "Slot")
    void SetSlot(UItemContainerComponent* InContainer, int32 InSlotIndex);

    UFUNCTION(BlueprintCallable, Category = "Slot")
    void RefreshSlot();

    /** Called every time the slot's content or selection changes. Use it for custom visuals. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Slot")
    void OnSlotRefreshed(UItemDefinition* InItem, int32 InCount, bool bInSelected);

protected:
    virtual void NativeDestruct() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
    virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

    UFUNCTION()
    void HandleSlotChanged(int32 ChangedIndex, UItemDefinition* ChangedItem, int32 ChangedCount);

    UFUNCTION()
    void HandleSelectionChanged(int32 OldSlot, int32 NewSlot);
};
