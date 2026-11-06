#include "ItemLibrary.h"
#include "ItemContainerComponent.h"

#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

static void AddContainersFrom(const AActor* Actor, TArray<UItemContainerComponent*>& Out)
{
    if (!Actor) { return; }

    TArray<UItemContainerComponent*> Found;
    Actor->GetComponents<UItemContainerComponent>(Found);
    for (UItemContainerComponent* Container : Found)
    {
        Out.AddUnique(Container);
    }
}

TArray<UItemContainerComponent*> UItemLibrary::FindItemContainers(const AActor* Context)
{
    TArray<UItemContainerComponent*> Result;
    if (!Context) { return Result; }

    AddContainersFrom(Context, Result);

    if (const APawn* Pawn = Cast<APawn>(Context))
    {
        AddContainersFrom(Pawn->GetController(), Result);
        AddContainersFrom(Pawn->GetPlayerState(), Result);
    }
    else if (const AController* Controller = Cast<AController>(Context))
    {
        AddContainersFrom(Controller->GetPawn(), Result);
        AddContainersFrom(Controller->GetPlayerState<APlayerState>(), Result);
    }
    else if (const APlayerState* PlayerState = Cast<APlayerState>(Context))
    {
        AddContainersFrom(PlayerState->GetPawn(), Result);
        AddContainersFrom(PlayerState->GetOwner(), Result);
    }
    return Result;
}

UItemContainerComponent* UItemLibrary::FindItemContainer(const AActor* Context, FName ContainerName)
{
    for (UItemContainerComponent* Container : FindItemContainers(Context))
    {
        if (ContainerName.IsNone() || Container->ContainerName == ContainerName)
        {
            return Container;
        }
    }
    return nullptr;
}
