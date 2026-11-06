// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "MPPlayerLifecycleLibrary.generated.h"

/**
 * Reusable server-authoritative player lifecycle, possession, spawning, and ownership utilities.
 */
UCLASS()
class MPUTILITYRUNTIME_API UMPPlayerLifecycleLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Safely possesses a Pawn on the authoritative server.
	 * Verifies server authority, unpossesses previous controller if any, and handles pawn restart.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Networking|PlayerLifecycle", meta = (DefaultToSelf = "Controller"))
	static bool SafePossessPawnServerOnly(APlayerController* Controller, APawn* NewPawn);

	/**
	 * Safely unpossesses a Pawn on the authoritative server without leaving dangling pointers.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Networking|PlayerLifecycle", meta = (DefaultToSelf = "Controller"))
	static bool SafeUnpossessPawnServerOnly(APlayerController* Controller);

	/**
	 * Server-side player pawn spawning and immediate possession at a designated transform.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Networking|PlayerLifecycle", meta = (DefaultToSelf = "Controller"))
	static APawn* SpawnAndPossessPlayerPawnServerOnly(
		APlayerController* Controller,
		TSubclassOf<APawn> PawnClass,
		const FTransform& SpawnTransform,
		ESpawnActorCollisionHandlingMethod CollisionHandling = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn
	);

	/**
	 * Transfers network ownership of an Actor to a new PlayerController safely on the server.
	 * Automatically updates NetOwner and verifies connection validity.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Networking|PlayerLifecycle", meta = (DefaultToSelf = "TargetActor"))
	static bool TransferActorOwnershipSafe(AActor* TargetActor, APlayerController* NewOwnerPC);

	/**
	 * Transitions a PlayerController into or out of Spectator mode on the authoritative server.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Networking|PlayerLifecycle", meta = (DefaultToSelf = "Controller"))
	static bool SetPlayerSpectatorStateServerOnly(APlayerController* Controller, bool bEnableSpectator);

	/**
	 * Respawns a player's pawn at the designated PlayerStart or transform, destroying or unpossessing the old pawn.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Networking|PlayerLifecycle", meta = (DefaultToSelf = "Controller"))
	static APawn* RespawnPlayerPawnServerOnly(
		APlayerController* Controller,
		TSubclassOf<APawn> PawnClass,
		const FTransform& RespawnTransform,
		bool bDestroyOldPawn = true
	);

	/**
	 * Destroys and cleans up a player pawn safely without crashing client proxies.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Networking|PlayerLifecycle", meta = (DefaultToSelf = "PawnToClean"))
	static bool CleanupPlayerPawnSafe(APawn* PawnToClean, float DelayBeforeDestroy = 0.0f);
};

