#include "ItemDragDropOperation.h"
#include "ItemContainerComponent.h"

void UItemDragDropOperation::DragCancelled_Implementation(const FPointerEvent& PointerEvent)
{
    Super::DragCancelled_Implementation(PointerEvent);

    if (bDropToWorldIfCancelled && SourceContainer && SourceSlot != INDEX_NONE && Count > 0)
    {
        SourceContainer->DropItem(SourceSlot, Count);
    }
}
