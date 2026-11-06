#include "MPTestActors.h"
#include "Net/UnrealNetwork.h"

AMPMockReplicatedActor::AMPMockReplicatedActor()
{
    bReplicates = true;
}

void AMPMockReplicatedActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AMPMockReplicatedActor, TestCounter);
}

AMPMockPawn::AMPMockPawn()
{
    bReplicates = true;
}

AMPMockPlayerController::AMPMockPlayerController()
{
    bReplicates = true;
}

