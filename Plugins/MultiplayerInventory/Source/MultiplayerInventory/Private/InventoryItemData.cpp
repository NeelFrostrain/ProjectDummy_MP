#include "InventoryItemData.h"

void UInventoryItemData::PostInitProperties()
{
    Super::PostInitProperties();

    // Auto-assign a unique GUID when the asset is first created
    if (!ItemId.IsValid())
    {
        ItemId = FGuid::NewGuid();
    }
}

#if WITH_EDITOR
void UInventoryItemData::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    // If the GUID was somehow cleared in the editor, regenerate it
    if (!ItemId.IsValid())
    {
        ItemId = FGuid::NewGuid();
    }
}
#endif

