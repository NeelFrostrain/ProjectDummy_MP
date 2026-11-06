#include "Wrappers/MPPawnControllerWrapper.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Engine/NetConnection.h"
#include "MPNetFunctionLibrary.h"

bool UMPPawnControllerWrapper::IsPlayerControlled(const APawn* TargetPawn)
{
    return TargetPawn ? TargetPawn->IsPlayerControlled() : false;
}

bool UMPPawnControllerWrapper::IsBotControlled(const APawn* TargetPawn)
{
    return TargetPawn ? TargetPawn->IsBotControlled() : false;
}

APlayerState* UMPPawnControllerWrapper::GetPawnPlayerState(const APawn* TargetPawn)
{
    return TargetPawn ? TargetPawn->GetPlayerState() : nullptr;
}

bool UMPPawnControllerWrapper::SafePossess(APlayerController* Controller, APawn* TargetPawn)
{
    if (!Controller || !TargetPawn)
    {
        return false;
    }

    if (!Controller->HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("[MPPawnControllerWrapper::SafePossess] Called on client! Possess requires Server authority."));
        return false;
    }

    Controller->Possess(TargetPawn);
    return Controller->GetPawn() == TargetPawn;
}

void UMPPawnControllerWrapper::SafeUnpossess(APlayerController* Controller)
{
    if (!Controller) return;

    if (!Controller->HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("[MPPawnControllerWrapper::SafeUnpossess] Called on client! Unpossess requires Server authority."));
        return;
    }

    Controller->UnPossess();
}

bool UMPPawnControllerWrapper::IsLocalPlayerController(const APlayerController* Controller)
{
    return Controller ? Controller->IsLocalPlayerController() : false;
}

float UMPPawnControllerWrapper::GetPlayerPingSeconds(const APlayerController* Controller)
{
    const float PingMs = UMPNetFunctionLibrary::GetPlayerPingMs(Controller);
    return (PingMs >= 0.0f) ? (PingMs * 0.001f) : -1.0f;
}

void UMPPawnControllerWrapper::SafeClientTravel(APlayerController* Controller, const FString& URL, ETravelType TravelType)
{
    if (!Controller || URL.IsEmpty()) return;
    Controller->ClientTravel(URL, TravelType);
}
