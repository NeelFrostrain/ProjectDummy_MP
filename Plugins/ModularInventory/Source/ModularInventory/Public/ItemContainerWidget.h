#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemContainerWidget.generated.h"

class UItemContainerComponent;
class UItemSlotWidget;
class UPanelWidget;
class UTextBlock;

/**
 * Shows a whole container: it creates one Item Slot widget per slot.
 *
 * Easiest use:
 *   1. Make a Blueprint child. Add a panel named "SlotPanel" (Uniform Grid, Horizontal Box, Wrap Box... anything).
 *   2. Set "Slot Widget Class" and (for a Uniform Grid) "Columns".
 *   3. Leave "Auto Find Container" on and set "Auto Find Container Name" (e.g. Hotbar). Done - add it to the viewport.
 * For chests and other containers call "Set Container" yourself.
 */
UCLASS(Abstract, Blueprintable)
class MODULARINVENTORY_API UItemContainerWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Where the slot widgets are created. Any panel works; a Uniform Grid is filled row by row using Columns. */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> SlotPanel;

    /** Optional. Shows the container's title. */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TitleText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container Widget")
    TSubclassOf<UItemSlotWidget> SlotWidgetClass;

    /** Only used when SlotPanel is a Uniform Grid Panel. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container Widget", meta = (ClampMin = "1"))
    int32 Columns = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container Widget")
    bool bShowSlotNumbers = false;

    /** Look for the local player's container by itself (also works if the pawn spawns later). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container Widget")
    bool bAutoFindContainer = true;

    /** Name of the container to find (see Container Name on the component). Empty = the first one. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container Widget", meta = (EditCondition = "bAutoFindContainer"))
    FName AutoFindContainerName = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "Container Widget")
    TWeakObjectPtr<UItemContainerComponent> Container;

    UPROPERTY(BlueprintReadOnly, Transient, Category = "Container Widget")
    TArray<TObjectPtr<UItemSlotWidget>> SlotWidgets;

    /** Shows this container. Pass null to clear. */
    UFUNCTION(BlueprintCallable, Category = "Container Widget")
    void SetContainer(UItemContainerComponent* NewContainer);

    /** Shift+click on a slot sends the item to this container (e.g. the player's backpack while a chest is open). */
    UFUNCTION(BlueprintCallable, Category = "Container Widget")
    void SetQuickTransferTarget(UItemContainerComponent* Target);

    /**
     * Alternative to the automatic layout: you placed Item Slot widgets by hand in the designer.
     * Pass them in order; slot widget N shows slot N.
     */
    UFUNCTION(BlueprintCallable, Category = "Container Widget")
    void BindExistingSlots(const TArray<UItemSlotWidget*>& Widgets);

    UFUNCTION(BlueprintCallable, Category = "Container Widget")
    void Rebuild();

    UFUNCTION(BlueprintImplementableEvent, Category = "Container Widget")
    void OnContainerSet(UItemContainerComponent* NewContainer);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UFUNCTION()
    void HandleContainerChanged();

private:
    UPROPERTY(Transient)
    TObjectPtr<UItemContainerComponent> QuickTransferTarget;

    bool bManualSlots = false;
    float FindTimer = 0.f;

    void TryAutoFind();
};
