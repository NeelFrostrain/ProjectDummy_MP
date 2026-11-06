#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/EngineBaseTypes.h"
#include "MPWorldSessionWrapper.generated.h"

class AGameModeBase;
class AGameStateBase;
class APawn;
class AActor;

UCLASS()
class MPUTILITYRUNTIME_API UMPWorldSessionWrapper : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Executes a server-wide seamless or non-seamless level transition.
     * Runs on: Server (Authority only).
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Multiplayer Utility|World", meta=(WorldContext="WorldContextObject", DisplayName="Safe Server Travel", Keywords="server travel map transition seamless"))
    static void SafeServerTravel(const UObject* WorldContextObject, const FString& MapURL, bool bAbsolute = false, bool bShouldSkipGameNotify = false);

    /**
     * Returns the total number of connected players from the replicated GameState.
     * Runs on: Both (Server and Client).
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|World", meta=(WorldContext="WorldContextObject", DisplayName="Get Connected Player Count", Keywords="connected player count num players"))
    static int32 GetConnectedPlayerCount(const UObject* WorldContextObject);

    /**
     * Returns authoritative server world time synchronized via GameState.
     * Runs on: Both (Server and Client).
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|World", meta=(WorldContext="WorldContextObject", DisplayName="Get Authoritative Server Time", Keywords="server time synced clock world time"))
    static double GetServerWorldTime(const UObject* WorldContextObject);

    /**
     * Checks whether a streaming level is loaded and visible for all currently connected player controllers.
     * Runs on: Server (Authority only).
     * 
     * @param WorldContextObject World context.
     * @param LevelName Name of the streaming level (package name or short name).
     * @return True if all connected players have the level loaded and visible; false otherwise.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Multiplayer Utility|World", meta=(WorldContext="WorldContextObject", DisplayName="Is Level Loaded For All Players", Keywords="streaming level loaded all players ready server world"))
    static bool IsLevelLoadedForAllPlayers(const UObject* WorldContextObject, FName LevelName);

    /**
     * Returns all player pawns currently situated inside a streaming level (by outer level ownership or bounding box).
     * Runs on: Both (Server and Client).
     * 
     * @param WorldContextObject World context.
     * @param LevelName Name of the streaming level (package name or short name).
     * @return Array of player pawns located in the specified level.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|World", meta=(WorldContext="WorldContextObject", DisplayName="Get Players In Streaming Level", Keywords="streaming level players pawns inside query world"))
    static TArray<APawn*> GetPlayersInStreamingLevel(const UObject* WorldContextObject, FName LevelName);

    /**
     * Checks if the given actor belongs to a level that is currently loaded and visible in the active world.
     * Runs on: Both (Server and Client).
     * 
     * @param Actor Actor to query.
     * @return True if the actor is in a loaded and visible level; false otherwise.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|World", meta=(DisplayName="Is Actor In Loaded Level", Keywords="actor loaded level visible streaming world"))
    static bool IsActorInLoadedLevel(const AActor* Actor);

    /**
     * Returns the clean level name of an actor's outer level, stripped of PIE and path prefixes.
     * Runs on: Both (Server and Client).
     * 
     * @param Actor Actor to query.
     * @param bCleanShortName If true, removes path and PIE prefix (returns "MapName"); if false, returns full package path without PIE.
     * @return Clean level name string.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|World", meta=(DisplayName="Get Level Name Of Actor", Keywords="actor level name package map clean"))
    static FString GetLevelNameOfActor(const AActor* Actor, bool bCleanShortName = true);

    /**
     * Checks whether the world is currently in the middle of a seamless travel transition.
     * Runs on: Both (Server and Client).
     * 
     * @param WorldContextObject World context.
     * @return True if seamless travel is actively underway.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|World", meta=(WorldContext="WorldContextObject", DisplayName="Is Seamless Traveling", Keywords="seamless travel transitioning map load world"))
    static bool IsSeamlessTraveling(const UObject* WorldContextObject);

    /**
     * Returns the current active world map name, stripped of PIE prefixes (e.g., UEDPIE_X_) and directory paths.
     * Runs on: Both (Server and Client).
     * 
     * @param WorldContextObject World context.
     * @param bCleanShortName If true, strips directory paths returning only the map base name (e.g. "Arena"); if false, retains full path without PIE.
     * @return Clean map name string.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|World", meta=(WorldContext="WorldContextObject", DisplayName="Get Current Map Name Clean", Keywords="current map name level clean pie stripped world"))
    static FString GetCurrentMapNameClean(const UObject* WorldContextObject, bool bCleanShortName = true);

    /**
     * Parses a travel URL or option string into a key-value dictionary of all URL options.
     * Standalone flags (e.g. "?listen") are mapped with an empty string value.
     * Runs on: Both (Server and Client).
     * 
     * @param InURL Travel URL or options string (e.g. "/Game/Maps/Arena?listen?port=7777?Team=2").
     * @return Map of option key-value pairs.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|World", meta=(DisplayName="Get Travel Options From URL", Keywords="travel options url parse parameters query dictionary map"))
    static TMap<FString, FString> GetTravelOptionsFromURL(const FString& InURL);
};
