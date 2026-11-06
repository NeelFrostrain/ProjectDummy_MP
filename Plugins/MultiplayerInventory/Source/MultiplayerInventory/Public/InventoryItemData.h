#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "InventoryItemData.generated.h"

class UTexture2D;
class UStaticMesh;
class AWorldInventoryItem;
class UInventoryItemGameplayAbility;

UCLASS(BlueprintType, Blueprintable)
class MULTIPLAYERINVENTORY_API UInventoryItemData : public UDataAsset
{
    GENERATED_BODY()

public:
    /** Unique identifier for this item type, auto-generated on asset creation. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item")
    FGuid ItemId;

    virtual void PostInitProperties() override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item", meta=(MultiLine=true))
    FText Description;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|UI")
    TObjectPtr<UTexture2D> Icon = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|World")
    TObjectPtr<UStaticMesh> WorldMesh = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|World")
    TSubclassOf<AWorldInventoryItem> WorldActorClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Stack", meta=(ClampMin="1"))
    int32 MaxStackSize = 64;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Tags")
    FGameplayTagContainer ItemTags;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item")
    bool bCanBeDropped = true;

    /** If true, using this item (when OnItemUsed returns true) will automatically consume 1 from its stack. Default is true for consumables/single-use items. Uncheck for reusable items like weapons or permanent tools. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Item")
    bool bConsumeOnUse = true;

    // --- Gameplay Ability ---

    // GAS ability to activate when this item is used. Derive from UInventoryItemGameplayAbility for tag checks and consumption control.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Ability")
    TSubclassOf<UInventoryItemGameplayAbility> AbilityClass;

};
