#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "ItemDragDropOperation.generated.h"

class UItemContainerComponent;
class UItemDefinition;

/** Created by Item Slot widgets when you drag a slot. */
UCLASS(BlueprintType)
class MODULARINVENTORY_API UItemDragDropOperation : public UDragDropOperation
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "Drag Drop")
    TObjectPtr<UItemContainerComponent> SourceContainer = nullptr;

    UPROPERTY(BlueprintReadWrite, Category = "Drag Drop")
    int32 SourceSlot = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite, Category = "Drag Drop")
    TObjectPtr<UItemDefinition> Item = nullptr;

    /** How many are being dragged (Ctrl while dragging = half the stack). */
    UPROPERTY(BlueprintReadWrite, Category = "Drag Drop")
    int32 Count = 0;

    /** If the drag ends on nothing, drop the item into the world. */
    UPROPERTY(BlueprintReadWrite, Category = "Drag Drop")
    bool bDropToWorldIfCancelled = true;

    virtual void DragCancelled_Implementation(const FPointerEvent& PointerEvent) override;
};
