#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ItemLibrary.generated.h"

class UItemContainerComponent;

UCLASS()
class MODULARINVENTORY_API UItemLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Finds an Item Container for a player. Pass a Pawn, Player Controller or Player State - all three are searched,
     * so it works wherever you put the component. Leave Container Name empty to get the first one found.
     */
    UFUNCTION(BlueprintPure, Category = "Item Container", meta = (DefaultToSelf = "Context"))
    static UItemContainerComponent* FindItemContainer(const AActor* Context, FName ContainerName = NAME_None);

    /** All Item Containers on the pawn, controller and player state of this player. */
    UFUNCTION(BlueprintPure, Category = "Item Container", meta = (DefaultToSelf = "Context"))
    static TArray<UItemContainerComponent*> FindItemContainers(const AActor* Context);
};
