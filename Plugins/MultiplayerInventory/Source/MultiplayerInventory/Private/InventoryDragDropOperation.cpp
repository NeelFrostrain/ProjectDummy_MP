#include "InventoryDragDropOperation.h"
#include "InventoryComponent.h"
#include "InventoryItemData.h"

void UInventoryDragDropOperation::DragCancelled_Implementation(const FPointerEvent& PointerEvent)
{
    Super::DragCancelled_Implementation(PointerEvent);

    if (bDropToWorldOnDropOutside && SourceInventoryComponent.IsValid() && SourceIndex != INDEX_NONE && ItemData && Quantity > 0)
    {
        SourceInventoryComponent->RequestDrop(SourceContainer, SourceIndex, Quantity);
    }
}

