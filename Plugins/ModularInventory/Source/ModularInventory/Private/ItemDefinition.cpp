#include "ItemDefinition.h"
#include "ItemContainerComponent.h"

UWorld* UItemAction::GetWorld() const
{
    return CachedWorld.Get();
}

bool UItemAction::ExecuteUse(UItemContainerComponent* Container, int32 SlotIndex, AActor* User)
{
    CachedWorld = Container ? Container->GetWorld() : nullptr;
    return OnUse(Container, SlotIndex, User);
}
