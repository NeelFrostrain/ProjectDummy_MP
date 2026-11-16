#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorldInventoryItem.generated.h"

class UInventoryItemData;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class MULTIPLAYERINVENTORY_API AWorldInventoryItem : public AActor
{
    GENERATED_BODY()

public:
    AWorldInventoryItem();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item")
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY(ReplicatedUsing = OnRep_ItemData, EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (ExposeOnSpawn = "true"))
    TObjectPtr<UInventoryItemData> ItemData;

    UPROPERTY(ReplicatedUsing=OnRep_Quantity, EditAnywhere, BlueprintReadWrite, Category="Item", meta=(ClampMin="1"))
    int32 Quantity = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item", meta=(DeprecatedProperty, DeprecationMessage="Actor is no longer destroyed on pickup. It is hidden and collision disabled instead."))
    bool bDestroyOnEmpty = false;

    UFUNCTION(BlueprintCallable, Category="Inventory")
    void InitializeDrop(UInventoryItemData* InItemData, int32 InQuantity);

    // Server-only. Returns the amount actually removed from this world stack.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Inventory")
    int32 TakeQuantity(int32 RequestedAmount);

    // Applies physical launch impulse to the item
    UFUNCTION(BlueprintCallable, Category="Inventory|Physics")
    void LaunchItem(FVector Velocity);

protected:
    virtual void BeginPlay() override;
    virtual void OnConstruction(const FTransform& Transform) override;

    UFUNCTION()
    void OnRep_ItemData();

    UFUNCTION()
    void OnRep_Quantity();

    void RefreshVisuals();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
