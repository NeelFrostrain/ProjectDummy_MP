// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/DamageType.h"
#include "MPGameplayHelperLibrary.generated.h"

class APlayerController;
class APlayerState;
class APawn;
class AActor;

/**
 * Collection of stateless, reusable multiplayer gameplay helper functions covering:
 * 1. Player and Network Utilities
 * 2. RPC Utilities
 * 3. Replication Utilities
 * 4. Multiplayer Gameplay Utilities
 * 5. Multiplayer Synchronization Utilities
 * 6. Multiplayer Debugging Utilities
 */
UCLASS()
class MPUTILITYRUNTIME_API UMPGameplayHelperLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// =========================================================================
	// 1. PLAYER AND NETWORK UTILITIES
	// =========================================================================

	/**
	 * Returns the local player's PlayerState without requiring manual controller casting.
	 */
	UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Player", meta = (WorldContext = "WorldContextObject", Keywords = "local player state get"))
	static APlayerState* GetLocalPlayerState(const UObject* WorldContextObject);

	/**
	 * Resolves the APlayerState from any actor in its ownership chain (Pawn, Controller, or PlayerState itself).
	 */
	UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Player", meta = (DefaultToSelf = "TargetActor", Keywords = "player state from actor resolve owner"))
	static APlayerState* GetPlayerStateFromActor(const AActor* TargetActor);

	/**
	 * Resolves the APlayerController from any actor in its ownership chain (Pawn, PlayerState, or Controller itself).
	 */
	UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Player", meta = (DefaultToSelf = "TargetActor", Keywords = "player controller from actor resolve owner"))
	static APlayerController* GetPlayerControllerFromActor(const AActor* TargetActor);

	/**
	 * Returns true if this actor is currently being viewed or possessed by the local player's camera / view target.
	 */
	UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Player", meta = (DefaultToSelf = "TargetActor", Keywords = "is player locally viewed camera view target local"))
	static bool IsPlayerLocallyViewed(const AActor* TargetActor);

	/**
	 * Returns the local split-screen player index for this controller (0 for primary, 1+ for split-screen, -1 if remote/dedicated).
	 */
	UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Player", meta = (DefaultToSelf = "PlayerController", Keywords = "player index local split screen id"))
	static int32 GetPlayerControllerIndex(const APlayerController* PlayerController);

	/**
	 * Returns all PlayerControllers that represent remote network clients (excludes local controllers).
	 */
	UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Network", meta = (WorldContext = "WorldContextObject", Keywords = "all remote player controllers dedicated listen server broadcast"))
	static TArray<APlayerController*> GetAllRemotePlayerControllers(const UObject* WorldContextObject);

	/**
	 * Returns true if the player controller has an active network connection and is not pending disconnect.
	 */
	UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Network", meta = (DefaultToSelf = "PlayerController", Keywords = "is player connection active valid connected"))
	static bool IsPlayerConnectionActive(const APlayerController* PlayerController);

	/**
	 * Formats a player's ping cleanly as a display string (e.g. "45 ms" or "-- ms" if uninitialized).
	 */
	UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Player", meta = (DefaultToSelf = "PlayerState", Keywords = "player ping formatted string ms latency"))
	static FString GetPlayerPingFormatted(const APlayerState* PlayerState);

	// =========================================================================
	// 2. RPC UTILITIES
	// =========================================================================

	/**
	 * Checks whether an actor can safely send a Server RPC, explaining any rejection reasons (lack of ownership, no connection, not replicated).
	 */
	UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|RPC", meta = (DefaultToSelf = "TargetActor", Keywords = "can actor send server rpc validation ownership connection"))
	static bool CanActorSendServerRPC(const AActor* TargetActor, FString& OutReason);

	/**
	 * Explains the execution scope of an RPC called on this actor (Server, Owning Client, Multicast, or Local).
	 */
	UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|RPC", meta = (DefaultToSelf = "TargetActor", Keywords = "rpc execution scope where rpc runs server client multicast"))
	static FString GetRPCExecutionScope(const AActor* TargetActor, FString& OutScopeDescription);

	/**
	 * Blueprint rate-limiter gate preventing RPC spamming based on an identifier and cooldown in seconds.
	 * Returns true if allowed, false if rejected due to cooldown.
	 */
	UFUNCTION(BlueprintCallable, Category = "Multiplayer Utility|RPC", meta = (DefaultToSelf = "ContextObject", Keywords = "check rpc rate limit gate cooldown throttle spam"))
	static bool CheckRPCRateLimitGate(UObject* ContextObject, FName RPCIdentifier, float CooldownSeconds, bool& bOutAllowed);

	// =========================================================================
	// 3. REPLICATION UTILITIES
	// =========================================================================

	/**
	 * Safely toggles actor replication and movement replication with server authority validation.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Multiplayer Utility|Replication", meta = (DefaultToSelf = "TargetActor", Keywords = "set actor replication safe server authority"))
	static void SetActorReplicationSafe(AActor* TargetActor, bool bReplicateActor, bool bReplicateMovement);

	/**
	 * Sets actor dormancy safely with authority check (wakes and flushes if bDormant is false).
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Multiplayer Utility|Replication", meta = (DefaultToSelf = "TargetActor", Keywords = "set actor net dormant safe sleep wake flush"))
	static void SetActorNetDormantSafe(AActor* TargetActor, bool bDormant);

	/**
	 * Returns true if the actor is currently dormant on the network.
	 */
	UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Replication", meta = (DefaultToSelf = "TargetActor", Keywords = "is actor dormant sleep"))
	static bool IsActorDormant(const AActor* TargetActor);

	/**
	 * Returns the current net update frequency (Hz) of the actor.
	 */
	UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Replication", meta = (DefaultToSelf = "TargetActor", Keywords = "get actor net update frequency hz rate"))
	static float GetActorNetUpdateFrequency(const AActor* TargetActor);

	/**
	 * Forces a network replication update for an actor and all attached child actors recursively (Server-only).
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Multiplayer Utility|Replication", meta = (DefaultToSelf = "TargetActor", Keywords = "force net update attached actors children recursive"))
	static void ForceNetUpdateWithAttachedActors(AActor* TargetActor);

	/**
	 * Authoritatively tears off an actor from network replication. Severs network connection so clients keep local simulated copies.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Multiplayer Utility|Replication", meta = (DefaultToSelf = "TargetActor", Keywords = "tear off actor safe sever networking local"))
	static void TearOffActorSafe(AActor* TargetActor);

	/**
	 * Returns true if the actor has been torn off from network replication.
	 */
	UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Replication", meta = (DefaultToSelf = "TargetActor", Keywords = "is actor torn off detached"))
	static bool IsActorTornOff(const AActor* TargetActor);

	// =========================================================================
	// 4. MULTIPLAYER GAMEPLAY UTILITIES
	// =========================================================================

	/**
	 * Authoritatively determines if Attacker can damage Victim based on null safety, self-damage, and team rules.
	 */
	UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Gameplay", meta = (Keywords = "can damage actor authoritative team friendly fire"))
	static bool CanDamageActorAuthoritative(const AActor* Attacker, const AActor* Victim, bool bAllowFriendlyFire);

	/**
	 * Server-only point damage applicator with authority validation and logging.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Multiplayer Utility|Gameplay", meta = (Keywords = "apply point damage server only authoritative hit"))
	static bool ApplyPointDamageServerOnly(AActor* DamagedActor, float BaseDamage, const FVector& HitFromDirection, const FHitResult& HitInfo, AController* EventInstigator, AActor* DamageCauser, TSubclassOf<UDamageType> DamageTypeClass);

	/**
	 * Server-only radial damage applicator with authority validation.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Multiplayer Utility|Gameplay", meta = (WorldContext = "WorldContextObject", Keywords = "apply radial damage server only authoritative explosion aoe"))
	static bool ApplyRadialDamageServerOnly(const UObject* WorldContextObject, float BaseDamage, const FVector& Origin, float DamageRadius, TSubclassOf<UDamageType> DamageTypeClass, const TArray<AActor*>& IgnoreActors, AActor* DamageCauser, AController* InstigatedByController, bool bDoFullDamage = false);

	/**
	 * Spawns a replicated actor on the server, assigning ownership and instigator properly before finishing spawn.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Multiplayer Utility|Gameplay", meta = (WorldContext = "WorldContextObject", Keywords = "spawn replicated actor server only deferred complete owner instigator"))
	static AActor* SpawnReplicatedActorServerOnly(const UObject* WorldContextObject, TSubclassOf<AActor> ClassToSpawn, const FTransform& SpawnTransform, ESpawnActorCollisionHandlingMethod CollisionHandling, AActor* Owner = nullptr, APawn* Instigator = nullptr);

	/**
	 * Authoritatively respawns a player's pawn at the given transform, unpossessing/cleaning up the old pawn.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Multiplayer Utility|Gameplay", meta = (Keywords = "respawn player pawn at transform authoritative possess"))
	static APawn* RespawnPlayerPawnAtTransform(APlayerController* PlayerController, TSubclassOf<APawn> PawnClass, const FTransform& SpawnTransform);

	/**
	 * Finds the best unoccupied APlayerStart closest to PreferredLocation (Server-only).
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Multiplayer Utility|Gameplay", meta = (WorldContext = "WorldContextObject", Keywords = "find best player start spawn point preferred location"))
	static AActor* FindBestPlayerStart(const UObject* WorldContextObject, const FVector& PreferredLocation, const FString& IncomingName = TEXT(""));

	/**
	 * Server-authoritative teleport that resets movement velocity and smoothly corrects clients.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Multiplayer Utility|Gameplay", meta = (Keywords = "teleport pawn server only authoritative reset velocity"))
	static bool TeleportPawnServerOnly(APawn* Pawn, const FVector& DestLocation, const FRotator& DestRotation, bool bResetVelocity = true);

	/**
	 * Server-only helper to update player score on the authoritative PlayerState.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Multiplayer Utility|Gameplay", meta = (Keywords = "set player score server only player state"))
	static bool SetPlayerScoreServerOnly(APlayerState* PlayerState, float NewScore);

	// =========================================================================
	// 5. MULTIPLAYER SYNCHRONIZATION UTILITIES
	// =========================================================================

	/**
	 * Returns synchronized server world time (seconds) using GameState GetServerWorldTimeSeconds().
	 */
	UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Synchronization", meta = (WorldContext = "WorldContextObject", Keywords = "get server time synced clock authoritative world time"))
	static float GetServerTimeSynced(const UObject* WorldContextObject);

	/**
	 * Returns the elapsed time in seconds since a server timestamp occurred, clamped >= 0.
	 */
	UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Synchronization", meta = (WorldContext = "WorldContextObject", Keywords = "time since server timestamp elapsed delta"))
	static float GetTimeSinceServerTimestamp(const UObject* WorldContextObject, float ServerTimestamp);

	/**
	 * Extrapolates where an actor is on the server based on its velocity and client half-trip ping latency.
	 */
	UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Synchronization", meta = (Keywords = "calculate latency compensated location ping velocity extrapolation"))
	static FVector CalculateLatencyCompensatedLocation(const FVector& CurrentLocation, const FVector& Velocity, float PingSeconds);

	/**
	 * Checks if EarlierTimestamp happened before LaterTimestamp, safely handling clock wrapping.
	 */
	UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Synchronization", meta = (Keywords = "are timestamps ordered sequence time clock"))
	static bool AreTimestampsOrdered(float EarlierTimestamp, float LaterTimestamp);

	/**
	 * Returns the current server tick rate (frames per second).
	 */
	UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Synchronization", meta = (WorldContext = "WorldContextObject", Keywords = "get server tick rate fps frame rate"))
	static float GetServerTickRate(const UObject* WorldContextObject);

	/**
	 * Returns true once every N ticks based on GFrameCounter, useful for staggered periodic network checks without timers.
	 */
	UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Synchronization", meta = (WorldContext = "WorldContextObject", Keywords = "is server tick cadence frame interval periodic"))
	static bool IsServerTickCadence(const UObject* WorldContextObject, int32 TickInterval = 10);

	// =========================================================================
	// 6. MULTIPLAYER DEBUGGING UTILITIES
	// =========================================================================

	/**
	 * Generates a clean 1-line network diagnostic string for any actor: [Name | Role | NetMode | Owner | Dormancy | Freq].
	 */
	UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|Debug", meta = (DefaultToSelf = "TargetActor", Keywords = "get actor network debug string role mode owner dormancy"))
	static FString GetActorNetworkDebugString(const AActor* TargetActor);

	/**
	 * Draws overhead network role and net mode in the world (development builds only).
	 */
	UFUNCTION(BlueprintCallable, Category = "Multiplayer Utility|Debug", meta = (DefaultToSelf = "TargetActor", Keywords = "draw network role debug string overhead visual"))
	static void DrawNetworkRoleDebugString(const AActor* TargetActor, const FVector& WorldOffset = FVector(0, 0, 100.0f), FColor TextColor = FColor::Green, float Duration = 0.0f);

	/**
	 * Writes a formatted network event to the log with [SERVER] or [CLIENT] prefix.
	 */
	UFUNCTION(BlueprintCallable, Category = "Multiplayer Utility|Debug", meta = (WorldContext = "ContextObject", Keywords = "log network event server client tag"))
	static void LogNetworkEvent(const UObject* ContextObject, const FString& EventName, bool bServerOnly = false);

	/**
	 * Sets Net.PktLag and Net.PktLoss console variables safely for network simulation testing in PIE.
	 */
	UFUNCTION(BlueprintCallable, Category = "Multiplayer Utility|Debug", meta = (Keywords = "simulate network condition cvar lag ping packet loss pie"))
	static void SimulateNetworkConditionCVar(int32 TargetPingMs, float PacketLossPercent);

	/**
	 * Dumps an actor's network replication configuration to LogNet.
	 */
	UFUNCTION(BlueprintCallable, Category = "Multiplayer Utility|Debug", meta = (DefaultToSelf = "TargetActor", Keywords = "dump replication info to log actor details"))
	static void DumpReplicationInfoToLog(const AActor* TargetActor);
};

