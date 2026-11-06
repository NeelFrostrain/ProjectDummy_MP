#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "Types/MPNetTypes.h"
#include "MPNetFunctionLibrary.generated.h"

class APawn;
class APlayerController;
class APlayerState;
class UMPNetPresetAsset;
class UActorComponent;

UCLASS()
class MPUTILITYRUNTIME_API UMPNetFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // =========================================================================
    // NETWORK STATE & ROLE QUERIES (PURE)
    // =========================================================================

    /**
     * Returns true ONLY if executing on an actual multiplayer server (Dedicated Server or Listen Server Host).
     * NOTE: Returns FALSE in Standalone (single-player). To check for single-player, use IsStandalone.
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|NetMode", meta=(WorldContext="WorldContextObject", Keywords="is server host authority"))
    static bool IsServer(const UObject* WorldContextObject);

    /**
     * Returns true if networking is active (Dedicated Server, Listen Server, or Client).
     * Returns false if running in Standalone (offline single-player).
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|NetMode", meta=(WorldContext="WorldContextObject", Keywords="is network game multiplayer online offline"))
    static bool IsNetworkGame(const UObject* WorldContextObject);

    /**
     * Returns true if executing in offline single-player (Standalone mode).
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|NetMode", meta=(WorldContext="WorldContextObject", Keywords="is standalone offline singleplayer"))
    static bool IsStandalone(const UObject* WorldContextObject);

    /**
     * Returns true if executing on a headless Dedicated Server.
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|NetMode", meta=(WorldContext="WorldContextObject", Keywords="is dedicated server"))
    static bool IsDedicatedServer(const UObject* WorldContextObject);

    /**
     * Returns true if executing on a Listen Server (Player hosting the match).
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|NetMode", meta=(WorldContext="WorldContextObject", Keywords="is listen server host"))
    static bool IsListenServer(const UObject* WorldContextObject);

    /**
     * Returns true if executing on a remote Client connected to a server.
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|NetMode", meta=(WorldContext="WorldContextObject", Keywords="is client remote"))
    static bool IsClient(const UObject* WorldContextObject);

    /**
     * Returns true if the target actor has local authority (HasAuthority()).
     * Null-safe check returning false on null actor.
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Role", meta=(DefaultToSelf="TargetActor", DisplayName="Has Network Authority (Safe)", Keywords="has authority server is authority"))
    static bool HasAuthority(const AActor* TargetActor);

    /**
     * Returns true if the target actor is an Autonomous Proxy (locally driven client).
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Role", meta=(DefaultToSelf="TargetActor", Keywords="is autonomous proxy local"))
    static bool IsAutonomousProxy(const AActor* TargetActor);

    /**
     * Returns true if the target actor is a Simulated Proxy (remote client puppet).
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Role", meta=(DefaultToSelf="TargetActor", Keywords="is simulated proxy remote"))
    static bool IsSimulatedProxy(const AActor* TargetActor);

    /**
     * Returns true if the Pawn or Controller is locally controlled.
     * Null-safe check accepting APawn, APlayerController, or any owned actor.
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Role", meta=(DefaultToSelf="TargetActor", DisplayName="Is Locally Controlled (Safe)", Keywords="is locally controlled local player"))
    static bool IsLocallyControlled(const AActor* TargetActor);

    /**
     * Returns true if the actor has replication enabled (bReplicates == true).
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Replication", meta=(DefaultToSelf="TargetActor", Keywords="is replicated bReplicates"))
    static bool IsReplicatedActor(const AActor* TargetActor);

    // =========================================================================
    // RPC VALIDATION & OWNERSHIP DEBUGGING
    // =========================================================================

    /**
     * Checks if an RPC can be safely sent without engine drop.
     * Evaluates null check, bReplicates, dormancy, and connection ownership.
     * @param TargetActor Actor sending or receiving the RPC.
     * @param bIsServerRPC True for Server RPC (requires client ownership); False for Client RPC (requires server authority & client connection).
     * @param OutReason Detailed enum indicating the exact failure reason if unsafe.
     * @return True if safe to call, false otherwise.
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|RPC", meta=(DefaultToSelf="TargetActor", DisplayName="Is RPC Safe To Call", Keywords="is rpc safe to call validate ownership connection dormancy"))
    static bool IsRPCSafeToCall(const AActor* TargetActor, bool bIsServerRPC, EMPRPCSafetyReason& OutReason);

    /**
     * Returns a human-readable diagnostic message explaining why an RPC dropped or would drop.
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|RPC", meta=(DefaultToSelf="TargetActor", DisplayName="Explain Why RPC Dropped", Keywords="explain why rpc dropped reason failure text drop"))
    static FString ExplainWhyRPCDropped(const AActor* TargetActor, bool bIsServerRPC);

    /**
     * Validates if an Actor can successfully send a Server RPC without it being silently dropped by the engine.
     * Checks: Actor validity, bReplicates, NetConnection ownership, and dormancy.
     * @param OutFailureReason Explains exactly why the RPC would drop if this returns false.
     * Runs on: Client.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|RPC", meta=(DefaultToSelf="TargetActor", Keywords="can send rpc validate ownership connection"))
    static bool CanSendServerRPC(const AActor* TargetActor, FString& OutFailureReason);

    /**
     * Traces the Owner hierarchy of an Actor up to the PlayerController/Connection.
     * Useful for debugging why an Actor lacks an owning connection for RPCs.
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|RPC", meta=(DefaultToSelf="TargetActor", Keywords="rpc ownership chain owner trace hierarchy"))
    static bool GetRPCOwnershipChain(const AActor* TargetActor, TArray<FString>& OutOwnerNames);

    // =========================================================================
    // CONNECTION & LATENCY QUERIES
    // =========================================================================

    /**
     * Returns accurate player ping in milliseconds.
     * Evaluates high-resolution decompressed ping from PlayerState (GetPingInMilliseconds()),
     * falling back to UNetConnection->AvgLag * 1000.0f.
     * Returns -1.0 if connection or player state is unavailable (e.g. Standalone or unassigned).
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Connection", meta=(DisplayName="Get Player Ping (ms)", Keywords="player ping latency ms rtt"))
    static float GetPlayerPingMs(const APlayerController* PC);

    /**
     * Returns packet loss percentages for a player connection.
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Connection", meta=(Keywords="packet loss in out connection"))
    static bool GetConnectionLossPercent(const APlayerController* PC, float& OutInLossPercent, float& OutOutLossPercent);

    /**
     * Evaluates comprehensive connection quality (Excellent, Good, Fair, Poor, Critical)
     * by combining latency, packet loss percentage, and jitter against project settings thresholds.
     * Returns EMPConnectionQuality::Unknown on null controller or in Standalone mode.
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Connection", meta=(DisplayName="Get Connection Quality", Keywords="connection quality rating ping packet loss health"))
    static EMPConnectionQuality GetConnectionQualityEnum(const APlayerController* PC);

    /**
     * Checks if a player's network connection is unstable due to high packet loss (> UnstablePacketLossThreshold)
     * or critical latency (> CriticalPingThresholdMs).
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Connection", meta=(DisplayName="Is Connection Unstable", Keywords="is connection unstable bad packet loss jitter spike"))
    static bool IsConnectionUnstable(const APlayerController* PC);

    /**
     * Calculates the smoothed average ping in milliseconds over a historical time window.
     * Samples historical ping using a circular buffer window without ticking.
     * Returns -1.0f if insufficient history or invalid controller.
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Connection", meta=(DisplayName="Get Average Ping Over N Seconds", Keywords="average ping moving average smoothed latency window"))
    static float GetAveragePingOverNSeconds(const APlayerController* PC, float WindowSeconds = 5.0f);

    /**
     * Determines whether player latency is Improving, Stable, or Worsening by comparing
     * the recent half of the ping history window against the older half.
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Connection", meta=(DisplayName="Get Ping Trend", Keywords="ping trend improving worsening stable delta direction"))
    static EMPPingTrend GetPingTrend(const APlayerController* PC, float WindowSeconds = 10.0f);

    /**
     * Safely retrieves inbound and outbound packet loss percentages.
     * Returns the maximum loss percentage as return value, or 0.0f on Standalone/null controller.
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Connection", meta=(DisplayName="Get Packet Loss Percent Safe", Keywords="packet loss percent safe in out dropped packets"))
    static float GetPacketLossPercentSafe(const APlayerController* PC, float& OutInLossPercent, float& OutOutLossPercent);

    /**
     * Recommends whether replication frequency, relevancy, or cosmetic RPC bandwidth should be reduced
     * for this player based on their connection quality (returns true if Poor or Critical).
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Connection", meta=(DisplayName="Should Reduce Network Load", Keywords="should reduce network load bandwidth throttle poor connection"))
    static bool ShouldReduceNetworkLoad(const APlayerController* PC);

    /**
     * Estimates current network bandwidth consumption for a player in bytes per second.
     * Aggregates live connection byte rates (InBytesPerSecond + OutBytesPerSecond) with CurrentNetSpeed fallback.
     * Documented as an authoritative runtime approximation.
     * Runs on: Server (Client fallback).
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Connection", meta=(DisplayName="Estimate Bandwidth Per Player", Keywords="estimate bandwidth per player bytes per second rate speed consumption"))
    static float EstimateBandwidthPerPlayer(const APlayerController* PC);

    /**
     * Returns the remote IP address and port of a connected client.
     * SERVER ONLY: Calling on client will safely return a warning message.
     * Runs on: Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Connection", meta=(Keywords="client ip address connection port server only"))
    static FString GetPlayerIPAddress(const APlayerController* PC);

    /**
     * Returns unique network ID string (Steam ID, Epic ID, etc.) for a player.
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Connection", meta=(Keywords="unique net id steam epic player id"))
    static FString GetPlayerUniqueIdString(const APlayerController* PC);

    // =========================================================================
    // VALIDATION & RATE LIMITING
    // =========================================================================

    /**
     * Token-bucket rate limiter to protect Server RPCs against client spam.
     * @param ChannelKey Unique identifier for the rate limit channel (e.g. "Chat", "FireWeapon", "Interact").
     * @param MaxBurst Maximum allowed burst tokens.
     * @param RefillRatePerSec Tokens regenerated per second.
     * @return True if permitted, False if rate limited. Runs on: Server.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Multiplayer Utility|Security", meta=(WorldContext="WorldContextObject", Keywords="rate limit spam throttle token bucket"))
    static bool CheckRPCRateLimit(const UObject* WorldContextObject, FName ChannelKey, int32 MaxBurst = 25, float RefillRatePerSec = 20.0f);

    /**
     * Server validation helper: checks if attacker and victim are within acceptable distance.
     * Includes ping latency compensation buffer to prevent false rejections.
     * Runs on: Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Security", meta=(Keywords="validate distance hit check sanity range"))
    static bool ValidateHitDistance(const AActor* Attacker, const AActor* Victim, float MaxAllowedDistance, float LatencyTolerance = 150.0f);

    /**
     * Server cooldown validation helper.
     * Runs on: Server.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Multiplayer Utility|Security", meta=(WorldContext="WorldContextObject", Keywords="validate cooldown timer"))
    static bool ValidateCooldown(const UObject* WorldContextObject, FName CooldownName, float DurationSeconds, bool bResetOnSuccess = true);

    // =========================================================================
    // SPATIAL & PLAYER QUERIES
    // =========================================================================

    /**
     * Gets all active PlayerControllers in the world.
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Queries", meta=(WorldContext="WorldContextObject", Keywords="all player controllers"))
    static TArray<APlayerController*> GetAllPlayerControllers(const UObject* WorldContextObject);

    /**
     * Gets all active Player Pawns in the world.
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Queries", meta=(WorldContext="WorldContextObject", Keywords="all player pawns characters"))
    static TArray<APawn*> GetAllPlayerPawns(const UObject* WorldContextObject);

    /**
     * Gets all remote player pawns (excludes the local player pawn).
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Queries", meta=(WorldContext="WorldContextObject", Keywords="all remote players other pawns"))
    static TArray<APawn*> GetAllRemotePlayers(const UObject* WorldContextObject);

    /**
     * Safe shortcut to the primary local player controller.
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Queries", meta=(WorldContext="WorldContextObject", Keywords="get local player controller"))
    static APlayerController* GetLocalPlayerController(const UObject* WorldContextObject);

    /**
     * Finds the closest player pawn to an origin location.
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Queries", meta=(WorldContext="WorldContextObject", DisplayName="Get Closest Player Pawn", Keywords="closest player pawn nearest"))
    static APawn* GetClosestPlayerPawn(const UObject* WorldContextObject, const FVector& Origin, float MaxDistance = -1.0f);

    /**
     * Gets all player pawns within a sphere radius.
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Queries", meta=(WorldContext="WorldContextObject", DisplayName="Get Players In Sphere", Keywords="players in sphere radius aoe range"))
    static TArray<APawn*> GetPlayersInSphere(const UObject* WorldContextObject, const FVector& Center, float Radius);

    /**
     * Calculates the distance in Unreal units from an origin location to the nearest player pawn.
     * Returns -1.0f if no player pawns exist within MaxDistance.
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Queries", meta=(WorldContext="WorldContextObject", DisplayName="Get Distance To Nearest Player", Keywords="distance nearest player closest range"))
    static float GetDistanceToNearestPlayer(const UObject* WorldContextObject, const FVector& Origin, float MaxDistance = -1.0f);

    /**
     * Checks if any player pawn is within the specified distance from an origin point.
     * Optimized early-exit query that avoids sorting or allocating arrays.
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Queries", meta=(WorldContext="WorldContextObject", DisplayName="Is Any Player Within Range", Keywords="is any player within range distance proximity"))
    static bool IsAnyPlayerWithinRange(const UObject* WorldContextObject, const FVector& Origin, float Range);

    /**
     * Retrieves all player pawns sorted by their distance from an origin location.
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Queries", meta=(WorldContext="WorldContextObject", DisplayName="Get Players Sorted By Distance", Keywords="players sorted distance order nearest furthest"))
    static TArray<APawn*> GetPlayersSortedByDistance(const UObject* WorldContextObject, const FVector& Origin, bool bAscending = true);

    /**
     * Finds the single player pawn furthest from the specified origin point.
     * Returns nullptr if no players exist within MaxDistance.
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Queries", meta=(WorldContext="WorldContextObject", DisplayName="Get Furthest Player", Keywords="furthest player pawn max distance distant"))
    static APawn* GetFurthestPlayer(const UObject* WorldContextObject, const FVector& Origin, float MaxDistance = -1.0f);

    /**
     * Finds all player pawns located within a directional vision or spell cone.
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Queries", meta=(WorldContext="WorldContextObject", DisplayName="Get Players In Cone", Keywords="players cone angle fov arc directional range"))
    static TArray<APawn*> GetPlayersInCone(const UObject* WorldContextObject, const FVector& ConeOrigin, const FVector& ConeDirection, float HalfAngleDegrees, float Range);

    /**
     * Finds all player pawns within a sphere radius that have an unobstructed line-of-sight to the center.
     * Executes line traces against world collision geometry.
     * Note: Server-authoritative query. Calling on clients logs a warning.
     * Runs on: Server (Client fallback with warning).
     */
    UFUNCTION(BlueprintCallable, Category="Multiplayer Utility|Queries", meta=(WorldContext="WorldContextObject", DisplayName="Get Players In Radius With Line Of Sight", Keywords="players line of sight radius raycast visibility los trace"))
    static TArray<APawn*> GetPlayersInRadiusWithLineOfSight(const UObject* WorldContextObject, const FVector& Center, float Radius, ECollisionChannel TraceChannel = ECC_Visibility, AActor* IgnoreActor = nullptr);

    /**
     * Returns a random active player pawn, with an optional list of pawns to exclude.
     * Returns nullptr if no eligible player pawns are found.
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Queries", meta=(WorldContext="WorldContextObject", DisplayName="Get Random Player", Keywords="random player select choose"))
    static APawn* GetRandomPlayer(const UObject* WorldContextObject, const TArray<APawn*>& ExcludeList);

    /**
     * Filters all player pawns, returning only those that are currently alive.
     * Queries the Blueprint-implementable IMPAliveInterface on the Pawn or its PlayerState.
     * If an actor does not implement IMPAliveInterface, it is assumed alive if valid and possessed.
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Queries", meta=(WorldContext="WorldContextObject", DisplayName="Get Alive Players Only", Keywords="alive players only living active eliminate down"))
    static TArray<APawn*> GetAlivePlayersOnly(const UObject* WorldContextObject);

    // =========================================================================
    // REPLICATION & NET DRIVER HELPERS
    // =========================================================================

    /**
     * Forces immediate replication update of an Actor to all connections.
     * Runs on: Server.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Multiplayer Utility|Replication", meta=(DefaultToSelf="TargetActor", Keywords="force net update replication sync"))
    static void ForceActorNetUpdate(AActor* TargetActor);

    /**
     * Dynamically sets an Actor's NetUpdateFrequency and MinNetUpdateFrequency.
     * Runs on: Server.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Multiplayer Utility|Replication", meta=(DefaultToSelf="TargetActor", Keywords="set net update rate frequency tick lod"))
    static void SetActorNetUpdateRate(AActor* TargetActor, float NetUpdateFrequency, float MinNetUpdateFrequency = 2.0f);

    /**
     * Wakes up a dormant actor so recent property changes replicate immediately.
     * Runs on: Server.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Multiplayer Utility|Replication", meta=(DefaultToSelf="TargetActor", Keywords="flush dormancy wake replicated"))
    static void FlushActorDormancy(AActor* TargetActor);

    /**
     * Toggles actor replication flags (bReplicates and bReplicateMovement).
     * Runs on: Server.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Multiplayer Utility|Replication", meta=(DefaultToSelf="TargetActor", Keywords="set actor replication replicates movement"))
    static void SetActorReplication(AActor* TargetActor, bool bReplicateMovement, bool bReplicates);

    // =========================================================================
    // DEBUGGING & DIAGNOSTICS
    // =========================================================================

    /**
     * Prints a message prefixed with current NetMode [Server] / [Client 1] / [Standalone] with custom color.
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintCallable, Category="Multiplayer Utility|Debug", meta=(WorldContext="WorldContextObject", AdvancedDisplay="TextColor,Duration,bPrintToScreen,bPrintToLog,Key", Keywords="print net string log message debug"))
    static void PrintNetString(const UObject* WorldContextObject, const FString& InString, FLinearColor TextColor = FLinearColor::Green, float Duration = 5.0f, bool bPrintToScreen = true, bool bPrintToLog = true, FName Key = NAME_None);

    /**
     * Displays a live diagnostic on-screen badge showing Actor, NetMode, Role, RemoteRole, Owner, and Ping.
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintCallable, Category="Multiplayer Utility|Debug", meta=(DefaultToSelf="TargetActor", Keywords="print actor net diagnostic badge overlay"))
    static void PrintActorNetDiagnostic(const AActor* TargetActor, float Duration = 5.0f);

    /**
     * Returns the Net Role of an Actor as a readable string.
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Debug", meta=(DefaultToSelf="TargetActor"))
    static FString GetActorNetRoleAsString(const AActor* TargetActor);

    /**
     * Returns the Remote Role of an Actor as a readable string.
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Debug", meta=(DefaultToSelf="TargetActor"))
    static FString GetActorRemoteRoleAsString(const AActor* TargetActor);

    /**
     * Returns the Net Mode as a readable string ("DedicatedServer", "ListenServer", "Client", "Standalone").
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Debug", meta=(WorldContext="WorldContextObject"))
    static FString GetNetModeAsString(const UObject* WorldContextObject);

    // =========================================================================
    // TAG & NAME HELPERS (BATCH 4)
    // =========================================================================

    /**
     * Retrieves all actors matching a specific gameplay tag or actor tag that are valid and optionally replicated.
     * Checks both IGameplayTagAssetInterface (if implemented) and standard Actor Tags as fallback.
     * Runs on: Client & Server.
     * 
     * @param WorldContextObject World context.
     * @param Tag The gameplay tag or actor tag to search for.
     * @param ActorClass Optional actor class filter.
     * @param bOnlyReplicated If true, only returns actors whose bReplicates flag is set.
     * @return Array of matching valid actors.
     */
    UFUNCTION(BlueprintCallable, Category="Multiplayer Utility|Tags", meta=(WorldContext="WorldContextObject", DisplayName="Get Actors With Tag Replicated Safe", Keywords="get actors tag gameplay replicated safe filter query"))
    static TArray<AActor*> GetActorsWithTagReplicatedSafe(const UObject* WorldContextObject, FGameplayTag Tag, TSubclassOf<AActor> ActorClass = nullptr, bool bOnlyReplicated = true);

    /**
     * Safely checks if an actor possesses any of the gameplay tags in the provided container, or matching actor tags.
     * Runs on: Client & Server.
     * 
     * @param Actor Actor to inspect.
     * @param TagContainer Container of tags to check against.
     * @param bExactMatch If true, requires exact tag match; if false, parent tags also match child tags according to GameplayTag hierarchy.
     * @return True if the actor has at least one matching tag; false otherwise.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Tags", meta=(DisplayName="Has Any Tag Safe", Keywords="has any tag safe gameplay tags container check"))
    static bool HasAnyTagSafe(const AActor* Actor, const FGameplayTagContainer& TagContainer, bool bExactMatch = false);

    /**
     * Generates a deterministic network-unique string identifier combining a prefix, a network index (e.g. PlayerId or NetDriver index), and a sequence counter.
     * Runs on: Client & Server.
     * 
     * @param Prefix Descriptive string prefix (e.g., "Player", "Entity", "Item").
     * @param NetIndex Network entity index or PlayerId (e.g., from PlayerState::GetPlayerId()).
     * @param Counter Monotonically increasing sequence or spawn counter.
     * @return Unique formatted name string (e.g., "Player_1_42").
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Names", meta=(DisplayName="Make Net Unique Name", Keywords="make net unique name identifier id key deterministic"))
    static FString MakeNetUniqueName(const FString& Prefix = TEXT("NetObj"), int32 NetIndex = 0, int32 Counter = 0);

    /**
     * Validates a player display name against length constraints and forbidden characters.
     * Ensures display names are safe for networked scoreboards, chat, and database storage.
     * Runs on: Client & Server.
     * 
     * @param InName Name string to validate.
     * @param MinLength Minimum allowed character count (default: 3).
     * @param MaxLength Maximum allowed character count (default: 20).
     * @param bAllowSpaces If true, spaces are allowed between characters.
     * @return True if the name meets all multiplayer safety criteria; false otherwise.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Names", meta=(DisplayName="Is Valid Player Name", Keywords="is valid player name sanitize check characters length scoreboard"))
    static bool IsValidPlayerName(const FString& InName, int32 MinLength = 3, int32 MaxLength = 20, bool bAllowSpaces = false);

    /**
     * Compares two player names for equality, ignoring case differences and leading/trailing whitespace.
     * Runs on: Client & Server.
     * 
     * @param NameA First player name.
     * @param NameB Second player name.
     * @return True if both names are equivalent ignoring case and surrounding whitespace.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Names", meta=(DisplayName="Compare Player Names (Case-Insensitive)", Keywords="compare player names case insensitive equal match"))
    static bool ComparePlayerNamesCaseInsensitive(const FString& NameA, const FString& NameB);

    // =========================================================================
    // CONSOLE VARIABLE (CVAR) HELPERS (BATCH 7)
    // =========================================================================

    /**
     * Safely queries a boolean console variable by name.
     * Runs on: Client & Server.
     * 
     * @param CVarName Console variable name (e.g. "mp.RateLimit.Enabled").
     * @param DefaultValue Fallback value if the CVar is not found.
     * @return Current boolean value of the console variable.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Settings", meta=(DisplayName="Get MP CVar (Bool)", Keywords="get cvar console variable bool"))
    static bool GetMPCVarBool(const FString& CVarName, bool DefaultValue = false);

    /**
     * Safely sets a boolean console variable by name.
     * Runs on: Client & Server.
     * 
     * @param CVarName Console variable name.
     * @param bValue Value to assign.
     */
    UFUNCTION(BlueprintCallable, Category="Multiplayer Utility|Settings", meta=(DisplayName="Set MP CVar (Bool)", Keywords="set cvar console variable bool"))
    static void SetMPCVarBool(const FString& CVarName, bool bValue);

    /**
     * Safely queries an integer console variable by name.
     * Runs on: Client & Server.
     * 
     * @param CVarName Console variable name (e.g. "mp.Debug.Overlay").
     * @param DefaultValue Fallback value if the CVar is not found.
     * @return Current integer value of the console variable.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Settings", meta=(DisplayName="Get MP CVar (Int)", Keywords="get cvar console variable int integer"))
    static int32 GetMPCVarInt(const FString& CVarName, int32 DefaultValue = 0);

    /**
     * Safely sets an integer console variable by name.
     * Runs on: Client & Server.
     * 
     * @param CVarName Console variable name.
     * @param Value Integer value to assign.
     */
    UFUNCTION(BlueprintCallable, Category="Multiplayer Utility|Settings", meta=(DisplayName="Set MP CVar (Int)", Keywords="set cvar console variable int integer"))
    static void SetMPCVarInt(const FString& CVarName, int32 Value);

    /**
     * Safely queries a floating-point console variable by name.
     * Runs on: Client & Server.
     * 
     * @param CVarName Console variable name (e.g. "mp.Net.PingSampleInterval").
     * @param DefaultValue Fallback value if the CVar is not found.
     * @return Current float value of the console variable.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Settings", meta=(DisplayName="Get MP CVar (Float)", Keywords="get cvar console variable float"))
    static float GetMPCVarFloat(const FString& CVarName, float DefaultValue = 0.0f);

    /**
     * Safely sets a floating-point console variable by name.
     * Runs on: Client & Server.
     * 
     * @param CVarName Console variable name.
     * @param Value Float value to assign.
     */
    UFUNCTION(BlueprintCallable, Category="Multiplayer Utility|Settings", meta=(DisplayName="Set MP CVar (Float)", Keywords="set cvar console variable float"))
    static void SetMPCVarFloat(const FString& CVarName, float Value);

    // =========================================================================
    // TEAM & RELEVANCY HELPERS
    // =========================================================================

    /**
     * Checks if two actors belong to the same team via IMPTeamInterface.
     * Runs on: Client & Server.
     * 
     * @param ActorA First actor to check.
     * @param ActorB Second actor to check.
     * @return True if both actors implement IMPTeamInterface and have identical non-negative team IDs.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Team", meta=(DisplayName="Are Teammates", Keywords="are teammates team same friendly ally"))
    static bool AreTeammates(const AActor* ActorA, const AActor* ActorB);

    /**
     * Checks if two actors belong to opposing teams via IMPTeamInterface.
     * Runs on: Client & Server.
     * 
     * @param ActorA First actor to check.
     * @param ActorB Second actor to check.
     * @return True if both actors implement IMPTeamInterface and have differing non-negative team IDs.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Team", meta=(DisplayName="Are Enemies", Keywords="are enemies opposing team hostile"))
    static bool AreEnemies(const AActor* ActorA, const AActor* ActorB);

    /**
     * Finds all actors belonging to a specific team ID.
     * Runs on: Client & Server.
     * 
     * @param WorldContextObject Context object.
     * @param TeamId Target team identifier.
     * @param ActorClass Optional class filter.
     * @return Array of actors that implement IMPTeamInterface with the specified team ID.
     */
    UFUNCTION(BlueprintCallable, Category="Multiplayer Utility|Team", meta=(WorldContext="WorldContextObject", DisplayName="Get Actors By Team", Keywords="get actors by team id filter"))
    static TArray<AActor*> GetActorsByTeam(const UObject* WorldContextObject, int32 TeamId, TSubclassOf<AActor> ActorClass = nullptr);

    /**
     * Safely retrieves the team ID of an actor, falling back to a default value if not implemented.
     * Runs on: Client & Server.
     * 
     * @param Actor Target actor.
     * @param DefaultTeamId Fallback team ID if interface is not implemented.
     * @return Team identifier.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Team", meta=(DisplayName="Get Team Of Actor Safe", Keywords="get team of actor safe id"))
    static int32 GetTeamOfActorSafe(const AActor* Actor, int32 DefaultTeamId = -1);

    /**
     * Checks if an actor is relevant to a specific player controller, respecting IMPRelevancyInterface if implemented.
     * Runs on: Server Only.
     * 
     * @param Actor Target actor to test relevancy for.
     * @param PC Target player controller.
     * @return True if actor is relevant to target player.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Relevancy", meta=(DisplayName="Is Actor Relevant To Player Safe", Keywords="is actor relevant to player safe custom relevancy"))
    static bool IsActorRelevantToPlayerSafe(const AActor* Actor, const APlayerController* PC);

    // =========================================================================
    // AUTHORITATIVE SERVER VALIDATION HELPERS
    // =========================================================================

    /**
     * Validates a client movement transform against speed limits, teleport distance limits, and valid delta time.
     * Runs on: Server Only.
     * 
     * @param Actor Actor being moved.
     * @param NewTransform Proposed transform from client.
     * @param DeltaTime Time elapsed since last update.
     * @param MaxSpeed Maximum allowable velocity (units/sec).
     * @param MaxTeleportDist Maximum allowable instant teleport distance.
     * @param OutResult Output validation result with reason.
     * @return True if transform passes all movement validation checks.
     */
    UFUNCTION(BlueprintCallable, Category="Multiplayer Utility|Validation", meta=(DisplayName="Validate Transform From Client", Keywords="validate transform from client speed teleport movement"))
    static bool ValidateTransformFromClient(const AActor* Actor, const FTransform& NewTransform, float DeltaTime, float MaxSpeed, float MaxTeleportDist, FMPValidationResult& OutResult);

    /**
     * Validates that a client-provided timestamp falls within a sane server rewind window.
     * Runs on: Server Only.
     * 
     * @param WorldContextObject Context object.
     * @param ClientTimestamp Timestamp sent by client.
     * @param MaxRewindWindow Maximum allowable past rewind seconds (e.g. 1.0s).
     * @param OutResult Output validation result.
     * @return True if timestamp is valid and within the acceptable rewind window.
     */
    UFUNCTION(BlueprintCallable, Category="Multiplayer Utility|Validation", meta=(WorldContext="WorldContextObject", DisplayName="Validate Timestamp From Client", Keywords="validate timestamp from client rewind window clamp"))
    static bool ValidateTimestampFromClient(const UObject* WorldContextObject, float ClientTimestamp, float MaxRewindWindow, FMPValidationResult& OutResult);

    /**
     * Validates that a floating-point value is not NaN/Infinite and falls within [MinValue, MaxValue].
     * Runs on: Client & Server.
     * 
     * @param Value Input float.
     * @param MinValue Minimum allowable value.
     * @param MaxValue Maximum allowable value.
     * @param OutResult Output validation result.
     * @return True if value is valid and within range.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Validation", meta=(DisplayName="Validate Float Range", Keywords="validate float range nan inf bounds"))
    static bool ValidateFloatRange(float Value, float MinValue, float MaxValue, FMPValidationResult& OutResult);

    /**
     * Validates that all components of a vector are valid numbers (not NaN or Infinite).
     * Runs on: Client & Server.
     * 
     * @param Vec Input vector.
     * @param OutResult Output validation result.
     * @return True if vector contains only finite real numbers.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Validation", meta=(DisplayName="Validate Vector Not NaN", Keywords="validate vector not nan infinite sane"))
    static bool ValidateVectorNotNaN(const FVector& Vec, FMPValidationResult& OutResult);

    /**
     * Validates that an unsigned 8-bit integer is within valid enumeration bounds.
     * Runs on: Client & Server.
     * 
     * @param Value Input byte/enum.
     * @param MaxValidValue Maximum allowable enum ordinal.
     * @param OutResult Output validation result.
     * @return True if value <= MaxValidValue.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Validation", meta=(DisplayName="Validate Enum Value", Keywords="validate enum value bounds range byte"))
    static bool ValidateEnumValue(uint8 Value, uint8 MaxValidValue, FMPValidationResult& OutResult);

    /**
     * Clamps a 2D or 3D client input vector to a maximum length (e.g., normalizes analog stick input).
     * Runs on: Client & Server.
     * 
     * @param InputVector Raw input vector from client.
     * @param MaxLength Maximum allowable magnitude (default: 1.0).
     * @return Clamped vector.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Validation", meta=(DisplayName="Clamp Client Input Vector", Keywords="clamp client input vector stick analog"))
    static FVector ClampClientInputVector(const FVector& InputVector, float MaxLength = 1.0f);

    /**
     * Checks if a spawn location is free of geometry collisions and optionally on a valid NavMesh.
     * Runs on: Server Only.
     * 
     * @param WorldContextObject Context object.
     * @param Location Proposed spawn location.
     * @param Extent Box extent to test for collisions.
     * @param bCheckCollision If true, performs an overlap check.
     * @return True if location is clear and valid for spawning.
     */
    UFUNCTION(BlueprintCallable, Category="Multiplayer Utility|Validation", meta=(WorldContext="WorldContextObject", DisplayName="Is Valid Spawn Location", Keywords="is valid spawn location collision clear free"))
    static bool IsValidSpawnLocation(const UObject* WorldContextObject, const FVector& Location, const FVector& Extent, bool bCheckCollision = true);

    /**
     * Verifies that line-of-sight between two locations is unobstructed by world geometry.
     * Runs on: Server Only.
     * 
     * @param WorldContextObject Context object.
     * @param Start Start location.
     * @param End End location.
     * @param Channel Collision trace channel.
     * @param IgnoreActor Optional actor to ignore in the trace.
     * @return True if path is completely clear without blocking hits.
     */
    UFUNCTION(BlueprintCallable, Category="Multiplayer Utility|Validation", meta=(WorldContext="WorldContextObject", DisplayName="Is Path Clear Server", Keywords="is path clear server line trace line of sight"))
    static bool IsPathClearServer(const UObject* WorldContextObject, const FVector& Start, const FVector& End, ECollisionChannel Channel = ECC_Visibility, const AActor* IgnoreActor = nullptr);

    /**
     * Sanitizes a player display name, trimming whitespace, enforcing length, and stripping forbidden characters.
     * Runs on: Client & Server.
     * 
     * @param InName Raw player name.
     * @param MinLength Minimum allowed length.
     * @param MaxLength Maximum allowed length.
     * @param OutSanitizedName Cleaned output name string.
     * @param OutResult Validation result.
     * @return True if sanitized name is valid.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Validation", meta=(DisplayName="Sanitize Player Name", Keywords="sanitize player name trim length filter characters"))
    static bool SanitizePlayerName(const FString& InName, int32 MinLength, int32 MaxLength, FString& OutSanitizedName, FMPValidationResult& OutResult);

    // =========================================================================
    // AUTHORITY-AWARE COMPONENT & SPAWN HELPERS
    // =========================================================================

    /**
     * Authoritatively dynamically creates and registers a replicated component on an actor.
     * Runs on: Server Only.
     * 
     * @param Owner Target owner actor.
     * @param ComponentClass Subclass of UActorComponent to create.
     * @param ComponentName Optional unique name for the component.
     * @return Newly created and registered replicated component.
     */
    UFUNCTION(BlueprintCallable, Category="Multiplayer Utility|Spawn", meta=(DisplayName="Add Replicated Component Server Only", Keywords="add replicated component server only spawn register"))
    static UActorComponent* AddReplicatedComponentServerOnly(AActor* Owner, TSubclassOf<UActorComponent> ComponentClass, FName ComponentName = NAME_None);

    /**
     * Safely finds a component on an actor by class and optional Gameplay Tag.
     * Runs on: Client & Server.
     * 
     * @param Actor Target actor.
     * @param ComponentClass Component class filter.
     * @param ComponentTag Optional gameplay tag filter.
     * @return Matching component or nullptr.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Component", meta=(DisplayName="Find Component Safe", Keywords="find component safe tag class"))
    static UActorComponent* FindComponentSafe(const AActor* Actor, TSubclassOf<UActorComponent> ComponentClass, FGameplayTag ComponentTag);

    /**
     * Defers actor spawning authoritatively so caller can configure replicated properties before FinishSpawning.
     * Runs on: Server Only.
     * 
     * @param WorldContextObject Context object.
     * @param ActorClass Actor class to spawn.
     * @param SpawnTransform Initial transform.
     * @param Owner Owning actor.
     * @param Instigator Instigating pawn.
     * @param CollisionMethod Collision handling method.
     * @return Unfinished spawned actor pointer.
     */
    UFUNCTION(BlueprintCallable, Category="Multiplayer Utility|Spawn", meta=(WorldContext="WorldContextObject", DisplayName="Spawn Actor Deferred Server Only", Keywords="spawn actor deferred server only initialize finish"))
    static AActor* SpawnActorDeferredServerOnly(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform, AActor* Owner = nullptr, APawn* Instigator = nullptr, ESpawnActorCollisionHandlingMethod CollisionMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

    /**
     * Completes spawning of a deferred actor on the server.
     * Runs on: Server Only.
     * 
     * @param Actor Deferred actor to finalize.
     * @param SpawnTransform Final transform.
     * @return Finalized spawned actor.
     */
    UFUNCTION(BlueprintCallable, Category="Multiplayer Utility|Spawn", meta=(DisplayName="Finish Spawning Actor Server Only", Keywords="finish spawning actor server only deferred complete"))
    static AActor* FinishSpawningActorServerOnly(AActor* Actor, const FTransform& SpawnTransform);

    /**
     * Spawns a player pawn authoritatively at an unoccupied APlayerStart.
     * Runs on: Server Only.
     * 
     * @param WorldContextObject Context object.
     * @param PawnClass Pawn class to instantiate.
     * @param Controller Optional controller to possess newly spawned pawn.
     * @return Spawned pawn or nullptr if no valid start found.
     */
    UFUNCTION(BlueprintCallable, Category="Multiplayer Utility|Spawn", meta=(WorldContext="WorldContextObject", DisplayName="Spawn At Free Player Start Safe", Keywords="spawn at free player start safe pawn possess"))
    static APawn* SpawnAtFreePlayerStartSafe(const UObject* WorldContextObject, TSubclassOf<AActor> PawnClass, AController* Controller = nullptr);

    /**
     * Authoritatively teleports an actor to destination and optionally clears its velocity to prevent desyncs.
     * Runs on: Server Only.
     * 
     * @param TargetActor Actor to teleport.
     * @param DestLocation Target location.
     * @param DestRotation Target rotation.
     * @param bStopVelocity If true, zeroes out linear and angular momentum.
     * @return True if teleport succeeded.
     */
    UFUNCTION(BlueprintCallable, Category="Multiplayer Utility|Movement", meta=(DefaultToSelf="TargetActor", DisplayName="Teleport Safe Server", Keywords="teleport safe server stop velocity transform"))
    static bool TeleportSafeServer(AActor* TargetActor, const FVector& DestLocation, const FRotator& DestRotation, bool bStopVelocity = true);

    /**
     * Safely retrieves the owning PlayerState for an actor by traversing pawn possession, owner chain, and IMPOwnershipInterface.
     * Runs on: Client & Server.
     * 
     * @param TargetActor Target actor.
     * @return Owning PlayerState or nullptr.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Player", meta=(DefaultToSelf="TargetActor", DisplayName="Get Owning Player State Safe", Keywords="get owning player state safe owner chain interface"))
    static APlayerState* GetOwningPlayerStateSafe(const AActor* TargetActor);

    /**
     * Safely retrieves the owning Pawn for an actor by traversing owner hierarchies.
     * Runs on: Client & Server.
     * 
     * @param TargetActor Target actor.
     * @return Owning Pawn or nullptr.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Player", meta=(DefaultToSelf="TargetActor", DisplayName="Get Owning Pawn Safe", Keywords="get owning pawn safe instigator possess"))
    static APawn* GetOwningPawnSafe(const AActor* TargetActor);

    /**
     * Applies a network replication preset data asset to the target actor on the server.
     * Runs on: Server Only.
     * 
     * @param TargetActor Actor to configure.
     * @param Preset Reusable preset data asset.
     */
    UFUNCTION(BlueprintCallable, Category="Multiplayer Utility|Preset", meta=(DefaultToSelf="TargetActor", DisplayName="Apply Net Preset To Actor", Keywords="apply net preset to actor data asset"))
    static void ApplyNetPresetToActor(AActor* TargetActor, const UMPNetPresetAsset* Preset);
};

