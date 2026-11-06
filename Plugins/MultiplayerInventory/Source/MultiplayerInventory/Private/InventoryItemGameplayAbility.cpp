#include "InventoryItemGameplayAbility.h"
#include "InventoryItemData.h"

UInventoryItemGameplayAbility::UInventoryItemGameplayAbility()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UInventoryItemGameplayAbility::CanUseItem(UInventoryItemData* InItemData) const
{
    if (!InItemData) return false;
    return CanUseItemTags(InItemData->ItemTags);
}

bool UInventoryItemGameplayAbility::CanUseItemTags(const FGameplayTagContainer& ItemTags) const
{
    if (!RequiredItemTags.IsEmpty() && !ItemTags.HasAll(RequiredItemTags))
    {
        return false;
    }

    return BlockedItemTags.IsEmpty() || !ItemTags.HasAny(BlockedItemTags);
}

FGameplayTagContainer UInventoryItemGameplayAbility::GetSourceItemTags() const
{
    if (SourceItemData) return SourceItemData->ItemTags;
    if (ItemData) return ItemData->ItemTags;
    return FGameplayTagContainer::EmptyContainer;
}

bool UInventoryItemGameplayAbility::ConsumeSourceItem(int32 Amount)
{
    if (Amount <= 0) return false;
    bWasConsumedDuringUse = true;
    if (UInventoryComponent* InvComp = SourceInventoryComponent.Get())
    {
        return InvComp->ConsumeSlotItem(SourceContainer, SourceSlotIndex, Amount);
    }
    return false;
}

bool UInventoryItemGameplayAbility::RemoveSourceItem(int32 Amount)
{
    return ConsumeSourceItem(Amount);
}

bool UInventoryItemGameplayAbility::OnItemSelected_Implementation(UInventoryItemData* InItemData, const FGameplayTagContainer& InItemTags)
{
    return true;
}

bool UInventoryItemGameplayAbility::OnItemUnselected_Implementation(UInventoryItemData* InItemData, const FGameplayTagContainer& InItemTags)
{
    return true;
}

bool UInventoryItemGameplayAbility::OnItemUsed_Implementation(UInventoryItemData* InItemData, const FGameplayTagContainer& InItemTags)
{
    return true;
}
