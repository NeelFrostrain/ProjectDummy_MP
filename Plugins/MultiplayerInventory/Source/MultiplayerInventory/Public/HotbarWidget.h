#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryComponent.h"
#include "HotbarWidget.generated.h"

class UInventorySlotWidget;
class UTextBlock;
class UButton;

UCLASS(Abstract, Blueprintable)
class MULTIPLAYERINVENTORY_API UHotbarWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|UI")
    EInventoryContainer Container = EInventoryContainer::Hotbar;

    UPROPERTY(BlueprintReadOnly, Category="Inventory|UI")
    TWeakObjectPtr<UInventoryComponent> InventoryComponent;

    UPROPERTY(Transient, BlueprintReadOnly, Category="Inventory|UI")
    TArray<TObjectPtr<UInventorySlotWidget>> SlotWidgets;

    // Optional direct widget bindings (auto-populated by name or designer BindWidget)
    UPROPERTY(BlueprintReadOnly, Category="Inventory|UI", meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> CapacityText = nullptr;

    UPROPERTY(BlueprintReadOnly, Category="Inventory|UI", meta=(BindWidgetOptional))
    TObjectPtr<UButton> SortButton = nullptr;

    UPROPERTY(BlueprintReadOnly, Category="Inventory|UI", meta=(BindWidgetOptional))
    TObjectPtr<UButton> DropAllButton = nullptr;

    UFUNCTION(BlueprintCallable, Category="Inventory|UI")
    void InitializeWithComponent(UInventoryComponent* InComp);

    UFUNCTION(BlueprintCallable, Category="Inventory|UI")
    void AutoDiscoverChildSlots();

    UFUNCTION(BlueprintCallable, Category="Inventory|UI")
    void AutoBindControls();

    UFUNCTION(BlueprintCallable, Category="Inventory|UI")
    void RegisterSlotWidget(UInventorySlotWidget* SlotWidget, int32 SlotIndex);

    UFUNCTION(BlueprintCallable, Category="Inventory|UI")
    void RefreshAll();

    UFUNCTION(BlueprintCallable, Category="Inventory|UI")
    void SortItems();

    UFUNCTION(BlueprintCallable, Category="Inventory|UI")
    void DropAllItems();

    UFUNCTION(BlueprintPure, Category="Inventory|UI")
    FText GetCapacityText() const;

    UFUNCTION(BlueprintImplementableEvent, Category="Inventory|UI")
    void BP_OnInventoryRefreshed(int32 UsedCount, int32 MaxCount);

    UFUNCTION(BlueprintImplementableEvent, Category="Inventory|UI")
    void BP_OnHotbarSelectionChanged(int32 NewIndex);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UFUNCTION()
    void HandleInventoryChanged();

    UFUNCTION()
    void HandleSelectionChanged(int32 NewIndex);
};
