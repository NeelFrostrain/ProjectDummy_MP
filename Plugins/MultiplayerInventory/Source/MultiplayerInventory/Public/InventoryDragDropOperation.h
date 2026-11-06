#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "InventoryComponent.h"
#include "InventoryDragDropOperation.generated.h"

class UInventoryItemData;

UCLASS(BlueprintType)
class MULTIPLAYERINVENTORY_API UInventoryDragDropOperation : public UDragDropOperation
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|DragDrop")
    EInventoryContainer SourceContainer = EInventoryContainer::Hotbar;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|DragDrop")
    int32 SourceIndex = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|DragDrop")
    TObjectPtr<UInventoryItemData> ItemData = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|DragDrop")
    int32 Quantity = 0;

    UPROPERTY(BlueprintReadWrite, Category="Inventory|DragDrop")
    TWeakObjectPtr<class UInventoryComponent> SourceInventoryComponent = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|DragDrop")
    bool bDropToWorldOnDropOutside = true;

    virtual void DragCancelled_Implementation(const FPointerEvent& PointerEvent) override;
};

