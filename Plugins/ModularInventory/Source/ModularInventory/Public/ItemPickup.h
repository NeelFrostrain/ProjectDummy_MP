#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemPickup.generated.h"

class UItemDefinition;
class UStaticMeshComponent;
class USphereComponent;
class UPrimitiveComponent;

/**
 * An item lying in the world. Put one in your level and set Item + Count, or let containers spawn them when items are dropped.
 * To pick one up, call "Pickup Actor" on a container with it (from a trace, an overlap, an interact key...).
 * Or tick Auto Pickup and walking over it is enough.
 */
UCLASS(Blueprintable)
class MODULARINVENTORY_API AItemPickup : public AActor
{
    GENERATED_BODY()

public:
    AItemPickup();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
    TObjectPtr<USphereComponent> AutoPickupSphere;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_Item, Category = "Pickup")
    TObjectPtr<UItemDefinition> Item;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Pickup", meta = (ClampMin = "1"))
    int32 Count = 1;

    /** Walk over it to pick it up. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Auto")
    bool bAutoPickup = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Auto", meta = (EditCondition = "bAutoPickup", ClampMin = "1"))
    float AutoPickupRadius = 120.f;

    /** Which container receives auto-picked items. None = the first one found. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Auto", meta = (EditCondition = "bAutoPickup"))
    FName AutoPickupContainerName = NAME_None;

    /** Let physics move it (needs simple collision on the mesh). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
    bool bSimulatePhysics = true;

    /** Override in Blueprint to refuse some pickers (e.g. only a certain team). */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pickup")
    bool CanBePickedUpBy(AActor* Picker) const;
    virtual bool CanBePickedUpBy_Implementation(AActor* Picker) const { return true; }

    /** Server only. Removes Amount from this pickup (it destroys itself at 0). Returns how many were taken. */
    UFUNCTION(BlueprintCallable, Category = "Pickup")
    int32 Take(int32 Amount);

    /** Gives the pickup a push (velocity change). */
    UFUNCTION(BlueprintCallable, Category = "Pickup")
    void Launch(FVector Velocity);

    /** Server only. Spawns an item into the world. */
    UFUNCTION(BlueprintCallable, Category = "Pickup", meta = (WorldContext = "WorldContextObject"))
    static AItemPickup* SpawnPickup(UObject* WorldContextObject, UItemDefinition* Item, int32 Count, FVector Location, FVector Velocity);

    static AItemPickup* SpawnInternal(UWorld* World, UItemDefinition* Item, int32 Count, FVector Location, FVector Velocity,
                                      bool bAllowAutoPickup, TSubclassOf<AItemPickup> ClassOverride);

protected:
    virtual void BeginPlay() override;
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION()
    void OnRep_Item();

    UFUNCTION()
    void OnSphereOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                         int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    void RefreshVisuals();
};
