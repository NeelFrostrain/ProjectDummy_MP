#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryComponent.h"
#include "InventorySlotWidget.generated.h"

class UInventoryItemData;
class UImage;
class UTextBlock;
class UBorder;

UCLASS(Abstract, Blueprintable)
class MULTIPLAYERINVENTORY_API UInventorySlotWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Slot", meta=(ExposeOnSpawn="true"))
    EInventoryContainer Container = EInventoryContainer::Hotbar;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Slot", meta=(ExposeOnSpawn="true"))
    int32 SlotIndex = INDEX_NONE;

    UPROPERTY(Transient, BlueprintReadOnly, Category="Inventory|Slot")
    bool bIsSelected = false;

    UPROPERTY(Transient, BlueprintReadOnly, Category="Inventory|Slot")
    TObjectPtr<UInventoryItemData> CurrentItemData = nullptr;

    UPROPERTY(Transient, BlueprintReadOnly, Category="Inventory|Slot")
    int32 CurrentQuantity = 0;

    UPROPERTY(BlueprintReadOnly, Category="Inventory|Slot")
    TWeakObjectPtr<UInventoryComponent> InventoryComponent;

    // Optional direct widget bindings (auto-populated by name or designer BindWidget)
    UPROPERTY(BlueprintReadOnly, Category="Inventory|Slot", meta=(BindWidgetOptional))
    TObjectPtr<UImage> ItemIcon = nullptr;

    UPROPERTY(BlueprintReadOnly, Category="Inventory|Slot", meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> QuantityText = nullptr;

    UPROPERTY(BlueprintReadOnly, Category="Inventory|Slot", meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> KeyNumberText = nullptr;

    UPROPERTY(BlueprintReadOnly, Category="Inventory|Slot", meta=(BindWidgetOptional))
    TObjectPtr<UBorder> HighlightBorder = nullptr;

    UFUNCTION(BlueprintCallable, Category="Inventory|Slot")
    void SetupSlot(UInventoryComponent* InComp, EInventoryContainer InContainer, int32 InIndex);

    UFUNCTION(BlueprintCallable, Category="Inventory|Slot")
    void RefreshSlot();

    UFUNCTION(BlueprintCallable, Category="Inventory|Slot")
    void SelectThisSlot();

    UFUNCTION(BlueprintCallable, Category="Inventory|Slot")
    void DropSlotItem(int32 Amount = 1);

    UFUNCTION(BlueprintCallable, Category="Inventory|Slot")
    bool RemoveItem(int32 Amount = 1);

    UFUNCTION(BlueprintCallable, Category="Inventory|Slot")
    bool ClearSlot();

    UFUNCTION(BlueprintCallable, Category="Inventory|Slot")
    void AutoFindWidgets();

    UFUNCTION(BlueprintImplementableEvent, Category="Inventory|Slot")
    void BP_OnSlotUpdated(UInventoryItemData* ItemData, int32 Quantity, bool bSelected);

protected:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
    virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
};
