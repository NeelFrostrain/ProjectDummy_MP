#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/EngineBaseTypes.h"
#include "MPPawnControllerWrapper.generated.h"

class APawn;
class APlayerController;
class APlayerState;
class AActor;

UCLASS()
class MPUTILITYRUNTIME_API UMPPawnControllerWrapper : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // =========================================================================
    // PAWN CONTROL QUERIES
    // =========================================================================

    /**
     * Checks if pawn is controlled by a human player (PlayerController).
     * Machine: Server / Client / Both
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Pawn", meta=(DisplayName="Is Player Controlled", Keywords="player controlled human"))
    static bool IsPlayerControlled(const APawn* TargetPawn);

    /**
     * Checks if pawn is controlled by an AI controller.
     * Machine: Server / Client / Both
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Pawn", meta=(DisplayName="Is Bot Controlled", Keywords="bot ai controlled npc"))
    static bool IsBotControlled(const APawn* TargetPawn);

    /**
     * Safely retrieves the replicated PlayerState for this pawn.
     * Machine: Server / Client / Both
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Pawn", meta=(DisplayName="Get Pawn Player State", Keywords="player state pawn"))
    static APlayerState* GetPawnPlayerState(const APawn* TargetPawn);

    // =========================================================================
    // POSSESSION & CONTROLLER ACTIONS
    // =========================================================================

    /**
     * Safely possesses a Pawn with authority validation.
     * Machine: Server (Authority only).
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Multiplayer Utility|Controller", meta=(DisplayName="Safe Possess", Keywords="possess pawn controller spawn"))
    static bool SafePossess(APlayerController* Controller, APawn* TargetPawn);

    /**
     * Safely unpossesses the current Pawn with authority validation.
     * Machine: Server (Authority only).
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Multiplayer Utility|Controller", meta=(DisplayName="Safe Unpossess", Keywords="unpossess pawn controller detach"))
    static void SafeUnpossess(APlayerController* Controller);

    /**
     * Checks if the PlayerController is local to this client instance.
     * Machine: Server / Client / Both
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Controller", meta=(DisplayName="Is Local Controller", Keywords="local controller is local player"))
    static bool IsLocalPlayerController(const APlayerController* Controller);

    // =========================================================================
    // LATENCY & NETWORK STATS
    // =========================================================================

    /**
     * Retrieves round-trip ping in seconds (useful for lag compensation calculations).
     * Derives value from UMPNetFunctionLibrary::GetPlayerPingMs.
     * Returns -1.0 if unavailable.
     * Machine: Server / Client / Both
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Controller", meta=(DisplayName="Get Player Ping (Seconds)", Keywords="ping rtt latency lag seconds"))
    static float GetPlayerPingSeconds(const APlayerController* Controller);

    /**
     * Safely commands the client player to travel to a new server or map.
     * Machine: Server or Local Client.
     */
    UFUNCTION(BlueprintCallable, Category="Multiplayer Utility|Controller", meta=(DisplayName="Safe Client Travel", Keywords="client travel disconnect reconnect join"))
    static void SafeClientTravel(APlayerController* Controller, const FString& URL, ETravelType TravelType = TRAVEL_Relative);
};
