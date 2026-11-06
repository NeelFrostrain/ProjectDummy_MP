#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "MPTestActors.generated.h"

UCLASS()
class MPUTILITYTESTS_API AMPMockReplicatedActor : public AActor
{
    GENERATED_BODY()

public:
    AMPMockReplicatedActor();

    UPROPERTY(Replicated)
    int32 TestCounter = 0;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};

UCLASS()
class MPUTILITYTESTS_API AMPMockPawn : public APawn
{
    GENERATED_BODY()

public:
    AMPMockPawn();
};

UCLASS()
class MPUTILITYTESTS_API AMPMockPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AMPMockPlayerController();
};

