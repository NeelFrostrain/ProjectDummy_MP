#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MPUrlParser.generated.h"

/**
 * Pure helper library for constructing, inspecting, and parsing Unreal Engine travel URLs
 * (e.g., "/Game/Maps/Arena?listen?port=7777?Team=2?PlayerName=Hero").
 */
UCLASS()
class MPUTILITYRUNTIME_API UMPUrlParser : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Extracts a string parameter value by key from a URL or options string.
     * @param InURL Full URL or option string (e.g., "Map?Key=Val")
     * @param Key The option key to search for (case-insensitive)
     * @param OutValue The parsed value if found
     * @return True if key was found
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|URL Parser")
    static bool GetURLOption(const FString& InURL, const FString& Key, FString& OutValue);

    /**
     * Parses an integer option from URL with safe default fallback.
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|URL Parser")
    static int32 GetURLOptionInt(const FString& InURL, const FString& Key, int32 DefaultValue = 0);

    /**
     * Parses a float option from URL with safe default fallback.
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|URL Parser")
    static float GetURLOptionFloat(const FString& InURL, const FString& Key, float DefaultValue = 0.0f);

    /**
     * Parses a boolean option from URL (supports 1/0, true/false, yes/no, or key presence).
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|URL Parser")
    static bool GetURLOptionBool(const FString& InURL, const FString& Key, bool DefaultValue = false);

    /**
     * Checks if an option key exists in URL (e.g., "?listen").
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|URL Parser")
    static bool HasURLOption(const FString& InURL, const FString& Key);

    /**
     * Extracts the base map name or IP address from the beginning of URL before any '?' or ':'.
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|URL Parser")
    static FString GetURLMapName(const FString& InURL);

    /**
     * Adds or replaces a key-value parameter in an existing URL string.
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|URL Parser")
    static FString SetURLOption(const FString& InURL, const FString& Key, const FString& Value);

    /**
     * Removes a key and its associated value from URL string.
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|URL Parser")
    static FString RemoveURLOption(const FString& InURL, const FString& Key);

    /**
     * Constructs a full travel URL string from map name, options dictionary, listen flag, and port.
     */
    UFUNCTION(BlueprintPure, Category = "Multiplayer Utility|URL Parser")
    static FString BuildTravelURL(const FString& MapName, const TMap<FString, FString>& Options, bool bListen = false, int32 Port = 0);
};

