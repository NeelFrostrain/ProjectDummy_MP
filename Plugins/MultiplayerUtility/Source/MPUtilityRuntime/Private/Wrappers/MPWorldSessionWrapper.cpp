#include "Wrappers/MPWorldSessionWrapper.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/Level.h"
#include "Engine/LevelStreaming.h"
#include "Engine/LevelBounds.h"
#include "Engine/NetConnection.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "Misc/PackageName.h"
#include "MPNetMacros.h"

static UWorld* GetSafeWorld(const UObject* WorldContextObject)
{
    return GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
}

void UMPWorldSessionWrapper::SafeServerTravel(const UObject* WorldContextObject, const FString& MapURL, bool bAbsolute, bool bShouldSkipGameNotify)
{
    UWorld* World = GetSafeWorld(WorldContextObject);
    if (!World) return;

    if (World->GetNetMode() == NM_Client)
    {
        MP_LOG_WARN_ONCE(TEXT("SafeServerTravel is Server-only but was called on a Client."));
        return;
    }

    World->ServerTravel(MapURL, bAbsolute, bShouldSkipGameNotify);
}

int32 UMPWorldSessionWrapper::GetConnectedPlayerCount(const UObject* WorldContextObject)
{
    const UWorld* World = GetSafeWorld(WorldContextObject);
    if (!World) return 0;

    if (const AGameStateBase* GS = World->GetGameState<AGameStateBase>())
    {
        return GS->PlayerArray.Num();
    }
    return 0;
}

double UMPWorldSessionWrapper::GetServerWorldTime(const UObject* WorldContextObject)
{
    const UWorld* World = GetSafeWorld(WorldContextObject);
    if (!World) return 0.0;

    if (const AGameStateBase* GS = World->GetGameState<AGameStateBase>())
    {
        return GS->GetServerWorldTimeSeconds();
    }
    return World->GetTimeSeconds();
}

bool UMPWorldSessionWrapper::IsLevelLoadedForAllPlayers(const UObject* WorldContextObject, FName LevelName)
{
    UWorld* World = GetSafeWorld(WorldContextObject);
    if (!World)
    {
        return false;
    }

    if (World->GetNetMode() == NM_Client)
    {
        MP_LOG_WARN_ONCE(TEXT("IsLevelLoadedForAllPlayers is Server-only but was called on a Client."));
        return false;
    }

    if (LevelName.IsNone())
    {
        return false;
    }

    ULevelStreaming* TargetStreamingLevel = nullptr;

    for (ULevelStreaming* StreamingLevel : World->GetStreamingLevels())
    {
        if (!StreamingLevel)
        {
            continue;
        }

        const FString AssetPkg = StreamingLevel->GetWorldAssetPackageFName().ToString();
        const FString ShortName = FPackageName::GetShortName(AssetPkg);
        const FString CleanPkg = UWorld::RemovePIEPrefix(AssetPkg);
        const FString CleanShort = FPackageName::GetShortName(CleanPkg);

        if (StreamingLevel->GetWorldAssetPackageFName() == LevelName ||
            FName(*ShortName) == LevelName ||
            FName(*CleanPkg) == LevelName ||
            FName(*CleanShort) == LevelName)
        {
            TargetStreamingLevel = StreamingLevel;
            break;
        }
    }

    if (!TargetStreamingLevel)
    {
        return false;
    }

    const FName CanonicalPackageFName = TargetStreamingLevel->GetWorldAssetPackageFName();
    const FName CleanPackageFName = FName(*UWorld::RemovePIEPrefix(CanonicalPackageFName.ToString()));

    int32 EvaluatedPlayers = 0;
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        const APlayerController* PC = It->Get();
        if (!PC)
        {
            continue;
        }

        EvaluatedPlayers++;

        if (PC->IsLocalController())
        {
            if (!TargetStreamingLevel->IsLevelLoaded() || !TargetStreamingLevel->IsLevelVisible())
            {
                return false;
            }
        }
        else
        {
            const UNetConnection* NetConn = Cast<UNetConnection>(PC->Player);
            if (!NetConn)
            {
                return false;
            }

            if (!NetConn->ClientVisibleLevelNames.Contains(CanonicalPackageFName) &&
                !NetConn->ClientVisibleLevelNames.Contains(CleanPackageFName))
            {
                return false;
            }
        }
    }

    return EvaluatedPlayers > 0;
}

TArray<APawn*> UMPWorldSessionWrapper::GetPlayersInStreamingLevel(const UObject* WorldContextObject, FName LevelName)
{
    TArray<APawn*> Results;
    const UWorld* World = GetSafeWorld(WorldContextObject);
    if (!World || LevelName.IsNone())
    {
        return Results;
    }

    const ULevelStreaming* TargetStreamingLevel = nullptr;
    for (const ULevelStreaming* StreamingLevel : World->GetStreamingLevels())
    {
        if (!StreamingLevel)
        {
            continue;
        }

        const FString AssetPkg = StreamingLevel->GetWorldAssetPackageFName().ToString();
        const FString ShortName = FPackageName::GetShortName(AssetPkg);
        const FString CleanPkg = UWorld::RemovePIEPrefix(AssetPkg);
        const FString CleanShort = FPackageName::GetShortName(CleanPkg);

        if (StreamingLevel->GetWorldAssetPackageFName() == LevelName ||
            FName(*ShortName) == LevelName ||
            FName(*CleanPkg) == LevelName ||
            FName(*CleanShort) == LevelName)
        {
            TargetStreamingLevel = StreamingLevel;
            break;
        }
    }

    if (!TargetStreamingLevel)
    {
        return Results;
    }

    const ULevel* LoadedLevel = TargetStreamingLevel->GetLoadedLevel();
    if (!LoadedLevel)
    {
        return Results;
    }

    FBox LevelBox(EForceInit::ForceInit);
    if (LoadedLevel->LevelBoundsActor.IsValid())
    {
        LevelBox = LoadedLevel->LevelBoundsActor->GetComponentsBoundingBox();
    }
    else
    {
        LevelBox = ALevelBounds::CalculateLevelBounds(LoadedLevel);
    }

    TSet<APawn*> CandidatePawns;

    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        if (const APlayerController* PC = It->Get())
        {
            if (APawn* P = PC->GetPawn())
            {
                CandidatePawns.Add(P);
            }
        }
    }

    if (const AGameStateBase* GS = World->GetGameState<AGameStateBase>())
    {
        for (const APlayerState* PS : GS->PlayerArray)
        {
            if (PS)
            {
                if (APawn* P = PS->GetPawn())
                {
                    CandidatePawns.Add(P);
                }
            }
        }
    }

    for (APawn* Pawn : CandidatePawns)
    {
        if (!Pawn)
        {
            continue;
        }

        if (Pawn->GetLevel() == LoadedLevel)
        {
            Results.AddUnique(Pawn);
        }
        else if (LevelBox.IsValid && LevelBox.IsInsideOrOn(Pawn->GetActorLocation()))
        {
            Results.AddUnique(Pawn);
        }
    }

    return Results;
}

bool UMPWorldSessionWrapper::IsActorInLoadedLevel(const AActor* Actor)
{
    if (!Actor)
    {
        return false;
    }

    const ULevel* Level = Actor->GetLevel();
    if (!Level)
    {
        return false;
    }

    if (Level->IsPersistentLevel())
    {
        return true;
    }

    return Level->bIsVisible;
}

FString UMPWorldSessionWrapper::GetLevelNameOfActor(const AActor* Actor, bool bCleanShortName)
{
    if (!Actor)
    {
        return FString();
    }

    const ULevel* Level = Actor->GetLevel();
    if (!Level)
    {
        return FString();
    }

    const UPackage* LevelPackage = Level->GetOutermost();
    const FString PackageName = LevelPackage ? LevelPackage->GetName() : Level->GetName();
    const FString CleanPackageName = UWorld::RemovePIEPrefix(PackageName);

    if (bCleanShortName)
    {
        return FPackageName::GetShortName(CleanPackageName);
    }

    return CleanPackageName;
}

bool UMPWorldSessionWrapper::IsSeamlessTraveling(const UObject* WorldContextObject)
{
    const UWorld* World = GetSafeWorld(WorldContextObject);
    if (!World)
    {
        return false;
    }

    return World->IsInSeamlessTravel();
}

FString UMPWorldSessionWrapper::GetCurrentMapNameClean(const UObject* WorldContextObject, bool bCleanShortName)
{
    const UWorld* World = GetSafeWorld(WorldContextObject);
    if (!World)
    {
        return FString();
    }

    FString MapName = World->GetMapName();
    MapName = UWorld::RemovePIEPrefix(MapName);

    if (bCleanShortName)
    {
        return FPackageName::GetShortName(MapName);
    }

    return MapName;
}

TMap<FString, FString> UMPWorldSessionWrapper::GetTravelOptionsFromURL(const FString& InURL)
{
    TMap<FString, FString> OptionsMap;
    if (InURL.IsEmpty())
    {
        return OptionsMap;
    }

    TArray<FString> Tokens;
    InURL.ParseIntoArray(Tokens, TEXT("?"), true);

    const bool bFirstTokenIsMap = !InURL.StartsWith(TEXT("?"));
    for (int32 Index = 0; Index < Tokens.Num(); ++Index)
    {
        const FString& Token = Tokens[Index];
        if (Index == 0 && bFirstTokenIsMap && !Token.Contains(TEXT("=")))
        {
            continue;
        }

        FString Key;
        FString Value;
        if (Token.Split(TEXT("="), &Key, &Value))
        {
            OptionsMap.Add(Key, Value);
        }
        else
        {
            OptionsMap.Add(Token, FString());
        }
    }

    return OptionsMap;
}
