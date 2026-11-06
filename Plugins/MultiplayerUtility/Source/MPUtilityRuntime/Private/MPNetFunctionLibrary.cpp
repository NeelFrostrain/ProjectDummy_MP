#include "MPNetFunctionLibrary.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/NetConnection.h"
#include "Engine/NetDriver.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Types/MPNetPresetAsset.h"
#include "Kismet/GameplayStatics.h"
#include "MPUtilitySettings.h"
#include "GameplayTagAssetInterface.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"

#include "Types/MPRateLimiter.h"

// Token bucket rate limiters and cooldown storage
static TMap<FName, FMPRateLimiter> GRPCRateLimiters;
static TMap<FName, double> GCooldownTimers;

// =========================================================================
// NETWORK STATE & ROLE QUERIES
// =========================================================================

bool UMPNetFunctionLibrary::IsServer(const UObject* WorldContextObject)
{
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!World) return false;
    return World->GetNetMode() == NM_DedicatedServer || World->GetNetMode() == NM_ListenServer;
}

bool UMPNetFunctionLibrary::IsNetworkGame(const UObject* WorldContextObject)
{
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!World) return false;
    return World->GetNetMode() != NM_Standalone;
}

bool UMPNetFunctionLibrary::IsStandalone(const UObject* WorldContextObject)
{
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!World) return true;
    return World->GetNetMode() == NM_Standalone;
}

bool UMPNetFunctionLibrary::IsDedicatedServer(const UObject* WorldContextObject)
{
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    return World && World->GetNetMode() == NM_DedicatedServer;
}

bool UMPNetFunctionLibrary::IsListenServer(const UObject* WorldContextObject)
{
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    return World && World->GetNetMode() == NM_ListenServer;
}

bool UMPNetFunctionLibrary::IsClient(const UObject* WorldContextObject)
{
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    return World && World->GetNetMode() == NM_Client;
}

bool UMPNetFunctionLibrary::HasAuthority(const AActor* TargetActor)
{
    return TargetActor && TargetActor->HasAuthority();
}

bool UMPNetFunctionLibrary::IsAutonomousProxy(const AActor* TargetActor)
{
    return TargetActor && TargetActor->GetLocalRole() == ROLE_AutonomousProxy;
}

bool UMPNetFunctionLibrary::IsSimulatedProxy(const AActor* TargetActor)
{
    return TargetActor && TargetActor->GetLocalRole() == ROLE_SimulatedProxy;
}

bool UMPNetFunctionLibrary::IsLocallyControlled(const AActor* TargetActor)
{
    if (!TargetActor) return false;
    if (const APawn* Pawn = Cast<APawn>(TargetActor))
    {
        return Pawn->IsLocallyControlled();
    }
    if (const APlayerController* PC = Cast<APlayerController>(TargetActor))
    {
        return PC->IsLocalController();
    }
    return false;
}

bool UMPNetFunctionLibrary::IsReplicatedActor(const AActor* TargetActor)
{
    return TargetActor && TargetActor->GetIsReplicated();
}

// =========================================================================
// RPC VALIDATION & OWNERSHIP
// =========================================================================

bool UMPNetFunctionLibrary::IsRPCSafeToCall(const AActor* TargetActor, bool bIsServerRPC, EMPRPCSafetyReason& OutReason)
{
    if (!TargetActor || !IsValid(TargetActor))
    {
        OutReason = EMPRPCSafetyReason::NullActor;
        return false;
    }

    if (!TargetActor->GetIsReplicated())
    {
        OutReason = EMPRPCSafetyReason::NotReplicated;
        return false;
    }

    if (TargetActor->NetDormancy == DORM_DormantAll || TargetActor->NetDormancy == DORM_DormantPartial)
    {
        OutReason = EMPRPCSafetyReason::ActorDormant;
        return false;
    }

    if (bIsServerRPC)
    {
        const UNetConnection* Conn = TargetActor->GetNetConnection();
        if (!Conn)
        {
            OutReason = EMPRPCSafetyReason::ServerRPC_NoOwner;
            return false;
        }

        if (!TargetActor->HasAuthority() && TargetActor->GetLocalRole() != ROLE_AutonomousProxy)
        {
            const AActor* Owner = TargetActor->GetOwner();
            const APawn* OwnerPawn = Cast<const APawn>(Owner);
            const APlayerController* OwnerPC = Cast<const APlayerController>(Owner);
            const bool bLocallyOwned = (OwnerPawn && OwnerPawn->IsLocallyControlled()) || (OwnerPC && OwnerPC->IsLocalPlayerController());
            if (!bLocallyOwned)
            {
                OutReason = EMPRPCSafetyReason::ServerRPC_NoOwner;
                return false;
            }
        }
    }
    else
    {
        if (!TargetActor->HasAuthority())
        {
            OutReason = EMPRPCSafetyReason::ClientRPC_NoOwningConnection;
            return false;
        }

        const UNetConnection* Conn = TargetActor->GetNetConnection();
        if (!Conn)
        {
            OutReason = EMPRPCSafetyReason::ClientRPC_NoOwningConnection;
            return false;
        }
    }

    OutReason = EMPRPCSafetyReason::Safe;
    return true;
}

FString UMPNetFunctionLibrary::ExplainWhyRPCDropped(const AActor* TargetActor, bool bIsServerRPC)
{
    EMPRPCSafetyReason Reason = EMPRPCSafetyReason::Safe;
    if (IsRPCSafeToCall(TargetActor, bIsServerRPC, Reason))
    {
        return TEXT("RPC is safe to transmit.");
    }

    switch (Reason)
    {
    case EMPRPCSafetyReason::NullActor:
        return TEXT("RPC Dropped: Target actor is null or pending kill.");
    case EMPRPCSafetyReason::NotReplicated:
        return FString::Printf(TEXT("RPC Dropped: Actor '%s' has bReplicates=false. RPCs only replicate on replicated actors."), *GetNameSafe(TargetActor));
    case EMPRPCSafetyReason::ActorDormant:
        return FString::Printf(TEXT("RPC Dropped: Actor '%s' is dormant (NetDormancy active). Call FlushNetDormancy() before invoking RPCs."), *GetNameSafe(TargetActor));
    case EMPRPCSafetyReason::ServerRPC_NoOwner:
        return FString::Printf(TEXT("Server RPC Dropped: Actor '%s' lacks an autonomous owning connection. Clients can only send Server RPCs on actors they own (e.g. possessed Pawn or PlayerController)."), *GetNameSafe(TargetActor));
    case EMPRPCSafetyReason::ClientRPC_NoOwningConnection:
        return FString::Printf(TEXT("Client RPC Dropped: Actor '%s' has no NetConnection to target client. Server cannot send Client RPC to an actor without a client owner."), *GetNameSafe(TargetActor));
    case EMPRPCSafetyReason::NoNetConnection:
        return FString::Printf(TEXT("RPC Dropped: Actor '%s' has no valid NetConnection."), *GetNameSafe(TargetActor));
    case EMPRPCSafetyReason::ActorNotRelevant:
        return FString::Printf(TEXT("RPC Dropped: Actor '%s' is not net relevant to target connection."), *GetNameSafe(TargetActor));
    default:
        return TEXT("RPC Dropped: Unknown network precondition failure.");
    }
}

bool UMPNetFunctionLibrary::CanSendServerRPC(const AActor* TargetActor, FString& OutFailureReason)
{
    if (!TargetActor)
    {
        OutFailureReason = TEXT("TargetActor is NULL");
        return false;
    }
    if (!TargetActor->GetIsReplicated())
    {
        OutFailureReason = FString::Printf(TEXT("Actor '%s' has bReplicates=false"), *TargetActor->GetName());
        return false;
    }
    if (TargetActor->HasAuthority())
    {
        OutFailureReason = TEXT("Actor is already Authority on Server (Server RPC not needed; execute function locally)");
        return false;
    }
    if (!TargetActor->GetNetConnection())
    {
        OutFailureReason = FString::Printf(TEXT("Actor '%s' lacks an owning NetConnection. Actor's Owner chain must lead to a PlayerController possessed by this client."), *TargetActor->GetName());
        return false;
    }
    if (TargetActor->NetDormancy == DORM_DormantAll || TargetActor->NetDormancy == DORM_DormantPartial)
    {
        OutFailureReason = FString::Printf(TEXT("Actor '%s' is dormant (Call FlushNetDormancy first)"), *TargetActor->GetName());
        return false;
    }

    OutFailureReason = TEXT("OK");
    return true;
}

bool UMPNetFunctionLibrary::GetRPCOwnershipChain(const AActor* TargetActor, TArray<FString>& OutOwnerNames)
{
    OutOwnerNames.Empty();
    if (!TargetActor) return false;

    const AActor* Current = TargetActor;
    bool bHasConnection = false;

    while (Current)
    {
        const FString RoleStr = GetActorNetRoleAsString(Current);
        const bool bConn = Current->GetNetConnection() != nullptr;
        if (bConn) bHasConnection = true;

        OutOwnerNames.Add(FString::Printf(TEXT("%s (Class: %s, Role: %s, HasNetConnection: %s)"),
            *Current->GetName(), *Current->GetClass()->GetName(), *RoleStr, bConn ? TEXT("YES") : TEXT("NO")));

        Current = Current->GetOwner();
    }

    return bHasConnection;
}

// =========================================================================
// CONNECTION & LATENCY QUERIES
// =========================================================================

float UMPNetFunctionLibrary::GetPlayerPingMs(const APlayerController* PC)
{
    if (!PC) return -1.0f;

    // 1. Try PlayerState decompressed ping
    if (const APlayerState* PS = PC->GetPlayerState<APlayerState>())
    {
        const float PSPing = PS->GetPingInMilliseconds();
        if (PSPing > 0.0f)
        {
            return PSPing;
        }
    }

    // 2. Fallback: query NetConnection AvgLag
    if (const UNetConnection* Conn = PC->GetNetConnection())
    {
        return Conn->AvgLag * 1000.0f;
    }

    return -1.0f;
}

bool UMPNetFunctionLibrary::GetConnectionLossPercent(const APlayerController* PC, float& OutInLossPercent, float& OutOutLossPercent)
{
    OutInLossPercent = 0.0f;
    OutOutLossPercent = 0.0f;
    if (!PC) return false;

    if (const UNetConnection* Conn = PC->GetNetConnection())
    {
        OutInLossPercent = Conn->GetInLossPercentage().GetAvgLossPercentage() * 100.0f;
        OutOutLossPercent = Conn->GetOutLossPercentage().GetAvgLossPercentage() * 100.0f;
        return true;
    }
    return false;
}

struct FMPPingSample
{
    double Timestamp = 0.0;
    float PingMs = 0.0f;
};

struct FMPPlayerPingHistory
{
    TArray<FMPPingSample> Samples;
    static constexpr int32 MaxSamples = 64;

    void AddSample(double Time, float Ping)
    {
        if (Ping < 0.0f) return;
        if (Samples.Num() >= MaxSamples)
        {
            Samples.RemoveAt(0);
        }
        Samples.Add({ Time, Ping });
    }

    void Trim(double Cutoff)
    {
        Samples.RemoveAll([Cutoff](const FMPPingSample& S) { return S.Timestamp < Cutoff; });
    }
};

static TMap<uint32, FMPPlayerPingHistory> GPlayerPingHistories;

float UMPNetFunctionLibrary::GetPacketLossPercentSafe(const APlayerController* PC, float& OutInLossPercent, float& OutOutLossPercent)
{
    OutInLossPercent = 0.0f;
    OutOutLossPercent = 0.0f;
    if (!PC) return 0.0f;

    GetConnectionLossPercent(PC, OutInLossPercent, OutOutLossPercent);
    return FMath::Max(OutInLossPercent, OutOutLossPercent);
}

EMPConnectionQuality UMPNetFunctionLibrary::GetConnectionQualityEnum(const APlayerController* PC)
{
    if (!PC) return EMPConnectionQuality::Unknown;

    const float Ping = GetPlayerPingMs(PC);
    if (Ping < 0.0f) return EMPConnectionQuality::Unknown;

    float InLoss = 0.0f, OutLoss = 0.0f;
    const float MaxLoss = GetPacketLossPercentSafe(PC, InLoss, OutLoss);

    const UMPUtilitySettings* Settings = UMPUtilitySettings::Get();
    const float HighPing = Settings ? Settings->HighPingThresholdMs : 150.0f;
    const float CriticalPing = Settings ? Settings->CriticalPingThresholdMs : 250.0f;
    const float UnstableLoss = Settings ? Settings->UnstablePacketLossThreshold : 3.0f;

    if (Ping >= CriticalPing || MaxLoss >= (UnstableLoss * 2.0f))
    {
        return EMPConnectionQuality::Critical;
    }
    if (Ping >= HighPing || MaxLoss >= UnstableLoss)
    {
        return EMPConnectionQuality::Poor;
    }
    if (Ping >= 100.0f || MaxLoss >= 1.0f)
    {
        return EMPConnectionQuality::Fair;
    }
    if (Ping >= 50.0f || MaxLoss > 0.0f)
    {
        return EMPConnectionQuality::Good;
    }

    return EMPConnectionQuality::Excellent;
}

bool UMPNetFunctionLibrary::IsConnectionUnstable(const APlayerController* PC)
{
    const EMPConnectionQuality Quality = GetConnectionQualityEnum(PC);
    return Quality == EMPConnectionQuality::Poor || Quality == EMPConnectionQuality::Critical;
}

float UMPNetFunctionLibrary::GetAveragePingOverNSeconds(const APlayerController* PC, float WindowSeconds)
{
    if (!PC) return -1.0f;

    const float CurrentPing = GetPlayerPingMs(PC);
    if (CurrentPing < 0.0f) return -1.0f;

    const double Now = FPlatformTime::Seconds();
    const uint32 PCId = PC->GetUniqueID();
    FMPPlayerPingHistory& History = GPlayerPingHistories.FindOrAdd(PCId);

    History.AddSample(Now, CurrentPing);

    const double Cutoff = Now - FMath::Max(0.5f, WindowSeconds);
    History.Trim(Cutoff);

    if (History.Samples.Num() == 0)
    {
        return CurrentPing;
    }

    float Sum = 0.0f;
    for (const FMPPingSample& S : History.Samples)
    {
        Sum += S.PingMs;
    }
    return Sum / static_cast<float>(History.Samples.Num());
}

EMPPingTrend UMPNetFunctionLibrary::GetPingTrend(const APlayerController* PC, float WindowSeconds)
{
    if (!PC) return EMPPingTrend::Unknown;

    const float CurrentPing = GetPlayerPingMs(PC);
    if (CurrentPing < 0.0f) return EMPPingTrend::Unknown;

    const double Now = FPlatformTime::Seconds();
    const uint32 PCId = PC->GetUniqueID();
    FMPPlayerPingHistory& History = GPlayerPingHistories.FindOrAdd(PCId);
    History.AddSample(Now, CurrentPing);

    const double Window = FMath::Max(1.0f, WindowSeconds);
    History.Trim(Now - Window);

    if (History.Samples.Num() < 4)
    {
        return EMPPingTrend::Stable;
    }

    const int32 Mid = History.Samples.Num() / 2;
    float OlderSum = 0.0f;
    for (int32 i = 0; i < Mid; ++i)
    {
        OlderSum += History.Samples[i].PingMs;
    }
    const float OlderAvg = OlderSum / Mid;

    float RecentSum = 0.0f;
    for (int32 i = Mid; i < History.Samples.Num(); ++i)
    {
        RecentSum += History.Samples[i].PingMs;
    }
    const float RecentAvg = RecentSum / (History.Samples.Num() - Mid);

    const float Delta = RecentAvg - OlderAvg;
    if (Delta <= -5.0f)
    {
        return EMPPingTrend::Improving;
    }
    else if (Delta >= 5.0f)
    {
        return EMPPingTrend::Worsening;
    }

    return EMPPingTrend::Stable;
}

bool UMPNetFunctionLibrary::ShouldReduceNetworkLoad(const APlayerController* PC)
{
    return IsConnectionUnstable(PC);
}

float UMPNetFunctionLibrary::EstimateBandwidthPerPlayer(const APlayerController* PC)
{
    if (!PC) return 0.0f;

    if (const UNetConnection* Conn = PC->GetNetConnection())
    {
        const float LiveBytesPerSec = Conn->InBytesPerSecond + Conn->OutBytesPerSecond;
        if (LiveBytesPerSec > 0.0f)
        {
            return LiveBytesPerSec;
        }

        if (Conn->CurrentNetSpeed > 0)
        {
            return static_cast<float>(Conn->CurrentNetSpeed);
        }
    }

    return 15000.0f;
}

FString UMPNetFunctionLibrary::GetPlayerIPAddress(const APlayerController* PC)
{
    if (!PC) return TEXT("Invalid");

    if (!PC->HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("[MPNetFunctionLibrary::GetPlayerIPAddress] Called on client! Querying remote client IP is only supported on Server."));
        return TEXT("ClientSideQueryNotSupported");
    }

    if (UNetConnection* Conn = PC->GetNetConnection())
    {
        return Conn->LowLevelGetRemoteAddress(true);
    }
    return TEXT("LocalHost");
}

FString UMPNetFunctionLibrary::GetPlayerUniqueIdString(const APlayerController* PC)
{
    if (!PC) return TEXT("None");
    if (const APlayerState* PS = PC->GetPlayerState<APlayerState>())
    {
        return PS->GetUniqueId().ToString();
    }
    return TEXT("None");
}

// =========================================================================
// VALIDATION & RATE LIMITING
// =========================================================================

bool UMPNetFunctionLibrary::CheckRPCRateLimit(const UObject* WorldContextObject, FName ChannelKey, int32 MaxBurst, float RefillRatePerSec)
{
    const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!World || ChannelKey.IsNone()) return true;

    const double Now = FPlatformTime::Seconds();
    FMPRateLimiter* Limiter = GRPCRateLimiters.Find(ChannelKey);
    if (!Limiter)
    {
        Limiter = &GRPCRateLimiters.Add(ChannelKey, FMPRateLimiter(static_cast<double>(MaxBurst), static_cast<double>(RefillRatePerSec), Now));
    }

    return Limiter->Consume(1.0, Now);
}

bool UMPNetFunctionLibrary::ValidateHitDistance(const AActor* Attacker, const AActor* Victim, float MaxAllowedDistance, float LatencyTolerance)
{
    if (!Attacker || !Victim) return false;

    float PingBuffer = LatencyTolerance;
    if (const APawn* Pawn = Cast<APawn>(Attacker))
    {
        if (const APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
        {
            const float Ping = GetPlayerPingMs(PC);
            if (Ping > 0.0f)
            {
                // Dynamic speed allowance: average pawn sprint 600 units/sec * (Ping / 1000)
                PingBuffer += (600.0f * (Ping / 1000.0f));
            }
        }
    }

    const float AllowedTotal = MaxAllowedDistance + PingBuffer;
    const float ActualDistSq = FVector::DistSquared(Attacker->GetActorLocation(), Victim->GetActorLocation());
    return ActualDistSq <= FMath::Square(AllowedTotal);
}

bool UMPNetFunctionLibrary::ValidateCooldown(const UObject* WorldContextObject, FName CooldownName, float DurationSeconds, bool bResetOnSuccess)
{
    if (CooldownName.IsNone() || DurationSeconds <= 0.0f) return true;

    const double Now = FPlatformTime::Seconds();
    double* LastTime = GCooldownTimers.Find(CooldownName);

    if (!LastTime || (Now - *LastTime) >= (double)DurationSeconds)
    {
        if (bResetOnSuccess)
        {
            GCooldownTimers.Add(CooldownName, Now);
        }
        return true;
    }

    return false;
}

// =========================================================================
// SPATIAL & PLAYER QUERIES
// =========================================================================

TArray<APlayerController*> UMPNetFunctionLibrary::GetAllPlayerControllers(const UObject* WorldContextObject)
{
    TArray<APlayerController*> Result;
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!World) return Result;

    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        if (APlayerController* PC = It->Get())
        {
            Result.Add(PC);
        }
    }
    return Result;
}

TArray<APawn*> UMPNetFunctionLibrary::GetAllPlayerPawns(const UObject* WorldContextObject)
{
    TArray<APawn*> Result;
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!World) return Result;

    TSet<APawn*> UniquePawns;

    // 1. Check PlayerControllers (authoritative server & local client)
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        if (const APlayerController* PC = It->Get())
        {
            if (APawn* Pawn = PC->GetPawn())
            {
                UniquePawns.Add(Pawn);
            }
        }
    }

    // 2. Check GameState PlayerArray (essential on remote clients where only local controller exists)
    if (const AGameStateBase* GS = World->GetGameState<AGameStateBase>())
    {
        for (const APlayerState* PS : GS->PlayerArray)
        {
            if (PS && PS->GetPawn())
            {
                UniquePawns.Add(PS->GetPawn());
            }
        }
    }

    Result = UniquePawns.Array();
    return Result;
}

TArray<APawn*> UMPNetFunctionLibrary::GetAllRemotePlayers(const UObject* WorldContextObject)
{
    TArray<APawn*> Result;
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!World) return Result;

    const APlayerController* LocalPC = World->GetFirstPlayerController();
    const APawn* LocalPawn = LocalPC ? LocalPC->GetPawn() : nullptr;

    for (APawn* Pawn : GetAllPlayerPawns(WorldContextObject))
    {
        if (Pawn && Pawn != LocalPawn)
        {
            Result.Add(Pawn);
        }
    }
    return Result;
}

APlayerController* UMPNetFunctionLibrary::GetLocalPlayerController(const UObject* WorldContextObject)
{
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    return World ? World->GetFirstPlayerController() : nullptr;
}

APawn* UMPNetFunctionLibrary::GetClosestPlayerPawn(const UObject* WorldContextObject, const FVector& Origin, float MaxDistance)
{
    APawn* Closest = nullptr;
    float ClosestDistSq = (MaxDistance > 0.0f) ? FMath::Square(MaxDistance) : MAX_FLT;

    for (APawn* Pawn : GetAllPlayerPawns(WorldContextObject))
    {
        if (Pawn)
        {
            const float DistSq = FVector::DistSquared(Origin, Pawn->GetActorLocation());
            if (DistSq < ClosestDistSq)
            {
                ClosestDistSq = DistSq;
                Closest = Pawn;
            }
        }
    }
    return Closest;
}

TArray<APawn*> UMPNetFunctionLibrary::GetPlayersInSphere(const UObject* WorldContextObject, const FVector& Center, float Radius)
{
    TArray<APawn*> Result;
    const float RadiusSq = FMath::Square(Radius);

    for (APawn* Pawn : GetAllPlayerPawns(WorldContextObject))
    {
        if (Pawn && FVector::DistSquared(Center, Pawn->GetActorLocation()) <= RadiusSq)
        {
            Result.Add(Pawn);
        }
    }
    return Result;
}

float UMPNetFunctionLibrary::GetDistanceToNearestPlayer(const UObject* WorldContextObject, const FVector& Origin, float MaxDistance)
{
    float ClosestDistSq = (MaxDistance > 0.0f) ? FMath::Square(MaxDistance) : MAX_FLT;
    bool bFound = false;

    for (const APawn* Pawn : GetAllPlayerPawns(WorldContextObject))
    {
        if (Pawn)
        {
            const float DistSq = FVector::DistSquared(Origin, Pawn->GetActorLocation());
            if (DistSq < ClosestDistSq)
            {
                ClosestDistSq = DistSq;
                bFound = true;
            }
        }
    }

    return bFound ? FMath::Sqrt(ClosestDistSq) : -1.0f;
}

bool UMPNetFunctionLibrary::IsAnyPlayerWithinRange(const UObject* WorldContextObject, const FVector& Origin, float Range)
{
    if (Range <= 0.0f) return false;
    const float RangeSq = FMath::Square(Range);

    for (const APawn* Pawn : GetAllPlayerPawns(WorldContextObject))
    {
        if (Pawn && FVector::DistSquared(Origin, Pawn->GetActorLocation()) <= RangeSq)
        {
            return true;
        }
    }

    return false;
}

TArray<APawn*> UMPNetFunctionLibrary::GetPlayersSortedByDistance(const UObject* WorldContextObject, const FVector& Origin, bool bAscending)
{
    TArray<APawn*> Pawns = GetAllPlayerPawns(WorldContextObject);

    Pawns.Sort([Origin, bAscending](const APawn& A, const APawn& B)
    {
        const float DistA = FVector::DistSquared(Origin, A.GetActorLocation());
        const float DistB = FVector::DistSquared(Origin, B.GetActorLocation());
        return bAscending ? (DistA < DistB) : (DistA > DistB);
    });

    return Pawns;
}

APawn* UMPNetFunctionLibrary::GetFurthestPlayer(const UObject* WorldContextObject, const FVector& Origin, float MaxDistance)
{
    APawn* Furthest = nullptr;
    float MaxDistSq = -1.0f;
    const float CapDistSq = (MaxDistance > 0.0f) ? FMath::Square(MaxDistance) : MAX_FLT;

    for (APawn* Pawn : GetAllPlayerPawns(WorldContextObject))
    {
        if (Pawn)
        {
            const float DistSq = FVector::DistSquared(Origin, Pawn->GetActorLocation());
            if (DistSq <= CapDistSq && DistSq > MaxDistSq)
            {
                MaxDistSq = DistSq;
                Furthest = Pawn;
            }
        }
    }

    return Furthest;
}

TArray<APawn*> UMPNetFunctionLibrary::GetPlayersInCone(const UObject* WorldContextObject, const FVector& ConeOrigin, const FVector& ConeDirection, float HalfAngleDegrees, float Range)
{
    TArray<APawn*> Result;
    if (Range <= 0.0f || ConeDirection.IsNearlyZero()) return Result;

    const FVector NormDir = ConeDirection.GetSafeNormal();
    const float MinDot = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(HalfAngleDegrees, 0.0f, 180.0f)));
    const float RangeSq = FMath::Square(Range);

    for (APawn* Pawn : GetAllPlayerPawns(WorldContextObject))
    {
        if (!Pawn) continue;

        const FVector ToPawn = Pawn->GetActorLocation() - ConeOrigin;
        const float DistSq = ToPawn.SizeSquared();

        if (DistSq <= RangeSq)
        {
            if (DistSq <= KINDA_SMALL_NUMBER)
            {
                Result.Add(Pawn);
                continue;
            }

            const FVector DirToPawn = ToPawn / FMath::Sqrt(DistSq);
            if (FVector::DotProduct(NormDir, DirToPawn) >= MinDot)
            {
                Result.Add(Pawn);
            }
        }
    }

    return Result;
}

TArray<APawn*> UMPNetFunctionLibrary::GetPlayersInRadiusWithLineOfSight(const UObject* WorldContextObject, const FVector& Center, float Radius, ECollisionChannel TraceChannel, AActor* IgnoreActor)
{
    TArray<APawn*> Result;
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!World || Radius <= 0.0f) return Result;

    if (World->GetNetMode() == NM_Client)
    {
        static bool bLoggedClientLosWarning = false;
        if (!bLoggedClientLosWarning)
        {
            bLoggedClientLosWarning = true;
            UE_LOG(LogTemp, Warning, TEXT("[GetPlayersInRadiusWithLineOfSight] Called on Client! Performing local raycast; authoritative results require Server authority."));
        }
    }

    const float RadiusSq = FMath::Square(Radius);
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MPPlayersInRadiusLOS), false);
    if (IgnoreActor)
    {
        QueryParams.AddIgnoredActor(IgnoreActor);
    }

    for (APawn* Pawn : GetAllPlayerPawns(WorldContextObject))
    {
        if (!Pawn || Pawn == IgnoreActor) continue;

        const FVector PawnLoc = Pawn->GetActorLocation();
        if (FVector::DistSquared(Center, PawnLoc) <= RadiusSq)
        {
            FHitResult HitResult;
            const bool bHit = World->LineTraceSingleByChannel(HitResult, Center, PawnLoc, TraceChannel, QueryParams);

            if (!bHit || HitResult.GetActor() == Pawn)
            {
                Result.Add(Pawn);
            }
        }
    }

    return Result;
}

APawn* UMPNetFunctionLibrary::GetRandomPlayer(const UObject* WorldContextObject, const TArray<APawn*>& ExcludeList)
{
    TArray<APawn*> Eligible = GetAllPlayerPawns(WorldContextObject);
    if (ExcludeList.Num() > 0)
    {
        Eligible.RemoveAll([&ExcludeList](APawn* Pawn)
        {
            return !Pawn || ExcludeList.Contains(Pawn);
        });
    }

    if (Eligible.Num() == 0) return nullptr;
    const int32 Index = FMath::RandRange(0, Eligible.Num() - 1);
    return Eligible[Index];
}

TArray<APawn*> UMPNetFunctionLibrary::GetAlivePlayersOnly(const UObject* WorldContextObject)
{
    TArray<APawn*> Result;

    for (APawn* Pawn : GetAllPlayerPawns(WorldContextObject))
    {
        if (!IsValid(Pawn)) continue;

        bool bIsAlive = true;

        if (Pawn->Implements<UMPAliveInterface>())
        {
            bIsAlive = IMPAliveInterface::Execute_IsAlive(Pawn);
        }
        else if (const APlayerState* PS = Pawn->GetPlayerState())
        {
            if (PS->Implements<UMPAliveInterface>())
            {
                bIsAlive = IMPAliveInterface::Execute_IsAlive(const_cast<APlayerState*>(PS));
            }
        }

        if (bIsAlive)
        {
            Result.Add(Pawn);
        }
    }

    return Result;
}

// =========================================================================
// REPLICATION & NET DRIVER HELPERS
// =========================================================================

void UMPNetFunctionLibrary::ForceActorNetUpdate(AActor* TargetActor)
{
    if (TargetActor && TargetActor->HasAuthority())
    {
        TargetActor->ForceNetUpdate();
    }
}

void UMPNetFunctionLibrary::SetActorNetUpdateRate(AActor* TargetActor, float NetUpdateFrequency, float MinNetUpdateFrequency)
{
    if (TargetActor && TargetActor->HasAuthority())
    {
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 5
        TargetActor->SetNetUpdateFrequency(FMath::Max(0.1f, NetUpdateFrequency));
        TargetActor->SetMinNetUpdateFrequency(FMath::Clamp(MinNetUpdateFrequency, 0.1f, TargetActor->GetNetUpdateFrequency()));
#else
        TargetActor->NetUpdateFrequency = FMath::Max(0.1f, NetUpdateFrequency);
        TargetActor->MinNetUpdateFrequency = FMath::Clamp(MinNetUpdateFrequency, 0.1f, TargetActor->NetUpdateFrequency);
#endif
    }
}

void UMPNetFunctionLibrary::FlushActorDormancy(AActor* TargetActor)
{
    if (TargetActor && TargetActor->HasAuthority())
    {
        TargetActor->FlushNetDormancy();
    }
}

void UMPNetFunctionLibrary::SetActorReplication(AActor* TargetActor, bool bReplicateMovement, bool bReplicates)
{
    if (TargetActor && TargetActor->HasAuthority())
    {
        TargetActor->SetReplicates(bReplicates);
        TargetActor->SetReplicateMovement(bReplicateMovement);
    }
}

// =========================================================================
// DEBUGGING & DIAGNOSTICS
// =========================================================================

void UMPNetFunctionLibrary::PrintNetString(const UObject* WorldContextObject, const FString& InString, FLinearColor TextColor, float Duration, bool bPrintToScreen, bool bPrintToLog, FName Key)
{
    const FString Prefix = FString::Printf(TEXT("[%s] "), *GetNetModeAsString(WorldContextObject));
    const FString FinalString = Prefix + InString;

    if (bPrintToScreen && GEngine)
    {
        const uint64 KeyHash = (Key != NAME_None) ? GetTypeHash(Key) : INDEX_NONE;
        GEngine->AddOnScreenDebugMessage(KeyHash, Duration, TextColor.ToFColor(true), FinalString);
    }
    if (bPrintToLog)
    {
        UE_LOG(LogTemp, Log, TEXT("%s"), *FinalString);
    }
}

void UMPNetFunctionLibrary::PrintActorNetDiagnostic(const AActor* TargetActor, float Duration)
{
    if (!TargetActor) return;
    const FString ActorName = TargetActor->GetName();
    const FString NetMode = GetNetModeAsString(TargetActor);
    const FString Role = GetActorNetRoleAsString(TargetActor);
    const FString RemoteRole = GetActorRemoteRoleAsString(TargetActor);
    const bool bLocal = IsLocallyControlled(TargetActor);

    float Ping = -1.0f;
    if (const APawn* Pawn = Cast<APawn>(TargetActor))
    {
        if (const APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
        {
            Ping = GetPlayerPingMs(PC);
        }
    }

    const FString Diagnostic = FString::Printf(
        TEXT("[%s] Actor: %s | Role: %s | RemoteRole: %s | LocalControl: %s | Ping: %.1fms"),
        *NetMode, *ActorName, *Role, *RemoteRole, bLocal ? TEXT("YES") : TEXT("NO"), Ping
    );

    if (GEngine)
    {
        const FColor Color = (Role == TEXT("Authority")) ? FColor::Green : FColor::Cyan;
        GEngine->AddOnScreenDebugMessage(INDEX_NONE, Duration, Color, Diagnostic);
    }
    UE_LOG(LogTemp, Log, TEXT("%s"), *Diagnostic);
}

FString UMPNetFunctionLibrary::GetActorNetRoleAsString(const AActor* TargetActor)
{
    if (!TargetActor) return TEXT("None");
    switch (TargetActor->GetLocalRole())
    {
    case ROLE_Authority: return TEXT("Authority");
    case ROLE_AutonomousProxy: return TEXT("AutonomousProxy");
    case ROLE_SimulatedProxy: return TEXT("SimulatedProxy");
    default: return TEXT("None");
    }
}

FString UMPNetFunctionLibrary::GetActorRemoteRoleAsString(const AActor* TargetActor)
{
    if (!TargetActor) return TEXT("None");
    switch (TargetActor->GetRemoteRole())
    {
    case ROLE_Authority: return TEXT("Authority");
    case ROLE_AutonomousProxy: return TEXT("AutonomousProxy");
    case ROLE_SimulatedProxy: return TEXT("SimulatedProxy");
    default: return TEXT("None");
    }
}

FString UMPNetFunctionLibrary::GetNetModeAsString(const UObject* WorldContextObject)
{
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!World) return TEXT("Unknown");
    switch (World->GetNetMode())
    {
    case NM_Standalone: return TEXT("Standalone");
    case NM_DedicatedServer: return TEXT("DedicatedServer");
    case NM_ListenServer: return TEXT("ListenServer");
    case NM_Client: return TEXT("Client");
    default: return TEXT("Unknown");
    }
}

// =========================================================================
// TAG & NAME HELPERS (BATCH 4)
// =========================================================================

TArray<AActor*> UMPNetFunctionLibrary::GetActorsWithTagReplicatedSafe(const UObject* WorldContextObject, FGameplayTag Tag, TSubclassOf<AActor> ActorClass, bool bOnlyReplicated)
{
    TArray<AActor*> OutActors;
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!World || !Tag.IsValid())
    {
        return OutActors;
    }

    UClass* SearchClass = ActorClass ? ActorClass.Get() : AActor::StaticClass();
    const FName TagFName = Tag.GetTagName();

    for (TActorIterator<AActor> It(World, SearchClass); It; ++It)
    {
        AActor* Actor = *It;
        if (!IsValid(Actor))
        {
            continue;
        }

        if (bOnlyReplicated && !Actor->GetIsReplicated())
        {
            continue;
        }

        if (const IGameplayTagAssetInterface* TagInterface = Cast<IGameplayTagAssetInterface>(Actor))
        {
            if (TagInterface->HasMatchingGameplayTag(Tag))
            {
                OutActors.Add(Actor);
                continue;
            }
        }

        if (Actor->ActorHasTag(TagFName))
        {
            OutActors.Add(Actor);
        }
    }

    return OutActors;
}

bool UMPNetFunctionLibrary::HasAnyTagSafe(const AActor* Actor, const FGameplayTagContainer& TagContainer, bool bExactMatch)
{
    if (!IsValid(Actor) || TagContainer.IsEmpty())
    {
        return false;
    }

    if (const IGameplayTagAssetInterface* TagInterface = Cast<IGameplayTagAssetInterface>(Actor))
    {
        if (!bExactMatch)
        {
            if (TagInterface->HasAnyMatchingGameplayTags(TagContainer))
            {
                return true;
            }
        }
        else
        {
            FGameplayTagContainer OwnedTags;
            TagInterface->GetOwnedGameplayTags(OwnedTags);
            if (OwnedTags.HasAnyExact(TagContainer))
            {
                return true;
            }
        }
    }

    for (const FGameplayTag& Tag : TagContainer)
    {
        if (Actor->ActorHasTag(Tag.GetTagName()))
        {
            return true;
        }
    }

    return false;
}

FString UMPNetFunctionLibrary::MakeNetUniqueName(const FString& Prefix, int32 NetIndex, int32 Counter)
{
    const FString CleanPrefix = Prefix.IsEmpty() ? TEXT("NetObj") : Prefix;
    return FString::Printf(TEXT("%s_%d_%d"), *CleanPrefix, NetIndex, Counter);
}

bool UMPNetFunctionLibrary::IsValidPlayerName(const FString& InName, int32 MinLength, int32 MaxLength, bool bAllowSpaces)
{
    const FString Trimmed = InName.TrimStartAndEnd();
    if (Trimmed.Len() < MinLength || Trimmed.Len() > MaxLength)
    {
        return false;
    }

    for (int32 i = 0; i < Trimmed.Len(); ++i)
    {
        const TCHAR C = Trimmed[i];
        if (FChar::IsAlnum(C) || C == TEXT('_') || C == TEXT('-'))
        {
            continue;
        }
        if (bAllowSpaces && C == TEXT(' '))
        {
            continue;
        }
        return false;
    }

    return true;
}

bool UMPNetFunctionLibrary::ComparePlayerNamesCaseInsensitive(const FString& NameA, const FString& NameB)
{
    return NameA.TrimStartAndEnd().Equals(NameB.TrimStartAndEnd(), ESearchCase::IgnoreCase);
}

// =========================================================================
// CONSOLE VARIABLE (CVAR) HELPERS (BATCH 7)
// =========================================================================

bool UMPNetFunctionLibrary::GetMPCVarBool(const FString& CVarName, bool DefaultValue)
{
    if (CVarName.IsEmpty())
    {
        return DefaultValue;
    }
    IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(*CVarName);
    return CVar ? CVar->GetBool() : DefaultValue;
}

void UMPNetFunctionLibrary::SetMPCVarBool(const FString& CVarName, bool bValue)
{
    if (CVarName.IsEmpty())
    {
        return;
    }
    IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(*CVarName);
    if (CVar)
    {
        CVar->Set(bValue ? 1 : 0, ECVF_SetByCode);
    }
}

int32 UMPNetFunctionLibrary::GetMPCVarInt(const FString& CVarName, int32 DefaultValue)
{
    if (CVarName.IsEmpty())
    {
        return DefaultValue;
    }
    IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(*CVarName);
    return CVar ? CVar->GetInt() : DefaultValue;
}

void UMPNetFunctionLibrary::SetMPCVarInt(const FString& CVarName, int32 Value)
{
    if (CVarName.IsEmpty())
    {
        return;
    }
    IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(*CVarName);
    if (CVar)
    {
        CVar->Set(Value, ECVF_SetByCode);
    }
}

float UMPNetFunctionLibrary::GetMPCVarFloat(const FString& CVarName, float DefaultValue)
{
    if (CVarName.IsEmpty())
    {
        return DefaultValue;
    }
    IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(*CVarName);
    return CVar ? CVar->GetFloat() : DefaultValue;
}

void UMPNetFunctionLibrary::SetMPCVarFloat(const FString& CVarName, float Value)
{
    if (CVarName.IsEmpty())
    {
        return;
    }
    IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(*CVarName);
    if (CVar)
    {
        CVar->Set(Value, ECVF_SetByCode);
    }
}

// =========================================================================
// TEAM & RELEVANCY HELPERS
// =========================================================================

bool UMPNetFunctionLibrary::AreTeammates(const AActor* ActorA, const AActor* ActorB)
{
    if (!ActorA || !ActorB) return false;
    const int32 TeamA = GetTeamOfActorSafe(ActorA, -1);
    const int32 TeamB = GetTeamOfActorSafe(ActorB, -2);
    return TeamA >= 0 && TeamA == TeamB;
}

bool UMPNetFunctionLibrary::AreEnemies(const AActor* ActorA, const AActor* ActorB)
{
    if (!ActorA || !ActorB) return false;
    const int32 TeamA = GetTeamOfActorSafe(ActorA, -1);
    const int32 TeamB = GetTeamOfActorSafe(ActorB, -2);
    return TeamA >= 0 && TeamB >= 0 && TeamA != TeamB;
}

TArray<AActor*> UMPNetFunctionLibrary::GetActorsByTeam(const UObject* WorldContextObject, int32 TeamId, TSubclassOf<AActor> ActorClass)
{
    TArray<AActor*> Result;
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!World || TeamId < 0) return Result;

    UClass* FilterClass = ActorClass ? ActorClass.Get() : AActor::StaticClass();
    for (TActorIterator<AActor> It(World, FilterClass); It; ++It)
    {
        AActor* Actor = *It;
        if (IsValid(Actor) && GetTeamOfActorSafe(Actor, -1) == TeamId)
        {
            Result.Add(Actor);
        }
    }
    return Result;
}

int32 UMPNetFunctionLibrary::GetTeamOfActorSafe(const AActor* Actor, int32 DefaultTeamId)
{
    if (!Actor) return DefaultTeamId;

    if (Actor->Implements<UMPTeamInterface>())
    {
        return IMPTeamInterface::Execute_GetTeamId(Actor);
    }

    if (const APawn* Pawn = Cast<APawn>(Actor))
    {
        if (const APlayerState* PS = Pawn->GetPlayerState())
        {
            if (PS->Implements<UMPTeamInterface>())
            {
                return IMPTeamInterface::Execute_GetTeamId(PS);
            }
        }
        if (const AController* Controller = Pawn->GetController())
        {
            if (Controller->Implements<UMPTeamInterface>())
            {
                return IMPTeamInterface::Execute_GetTeamId(Controller);
            }
        }
    }

    return DefaultTeamId;
}

bool UMPNetFunctionLibrary::IsActorRelevantToPlayerSafe(const AActor* Actor, const APlayerController* PC)
{
    if (!Actor || !PC) return false;

    if (Actor->Implements<UMPRelevancyInterface>())
    {
        return IMPRelevancyInterface::Execute_IsActorRelevantToPlayer(Actor, PC);
    }

    return Actor->IsNetRelevantFor(PC->GetViewTarget(), PC, Actor->GetActorLocation());
}

// =========================================================================
// AUTHORITATIVE SERVER VALIDATION HELPERS
// =========================================================================

bool UMPNetFunctionLibrary::ValidateTransformFromClient(const AActor* Actor, const FTransform& NewTransform, float DeltaTime, float MaxSpeed, float MaxTeleportDist, FMPValidationResult& OutResult)
{
    if (!Actor)
    {
        OutResult = FMPValidationResult::Failure(EMPValidationReason::Failed_InvalidWorld, TEXT("Actor is null."));
        return false;
    }

    if (NewTransform.ContainsNaN())
    {
        OutResult = FMPValidationResult::Failure(EMPValidationReason::Failed_NaN, TEXT("Transform contains NaN or Infinite values."));
        return false;
    }

    const FVector CurrentLoc = Actor->GetActorLocation();
    const FVector ProposedLoc = NewTransform.GetLocation();
    const float Dist = FVector::Dist(CurrentLoc, ProposedLoc);

    if (MaxTeleportDist > 0.0f && Dist > MaxTeleportDist)
    {
        OutResult = FMPValidationResult::Failure(EMPValidationReason::Failed_TeleportLimit, FString::Printf(TEXT("Distance %.1f exceeds max teleport distance %.1f"), Dist, MaxTeleportDist));
        return false;
    }

    if (DeltaTime > 0.0001f && MaxSpeed > 0.0f)
    {
        const float Speed = Dist / DeltaTime;
        if (Speed > MaxSpeed * 1.25f) // 25% tolerance for network jitter
        {
            OutResult = FMPValidationResult::Failure(EMPValidationReason::Failed_SpeedLimit, FString::Printf(TEXT("Calculated speed %.1f exceeds max speed %.1f"), Speed, MaxSpeed));
            return false;
        }
    }

    OutResult = FMPValidationResult::Success();
    return true;
}

bool UMPNetFunctionLibrary::ValidateTimestampFromClient(const UObject* WorldContextObject, float ClientTimestamp, float MaxRewindWindow, FMPValidationResult& OutResult)
{
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!World)
    {
        OutResult = FMPValidationResult::Failure(EMPValidationReason::Failed_InvalidWorld, TEXT("Invalid World Context"));
        return false;
    }

    const float ServerTime = (float)World->GetTimeSeconds();

    if (ClientTimestamp > ServerTime + 0.15f) // 150ms future tolerance for clock drift
    {
        OutResult = FMPValidationResult::Failure(EMPValidationReason::Failed_OutOfRange, TEXT("Timestamp is in the future."));
        return false;
    }

    if (MaxRewindWindow > 0.0f && ClientTimestamp < (ServerTime - MaxRewindWindow))
    {
        OutResult = FMPValidationResult::Failure(EMPValidationReason::Failed_RewindWindow, TEXT("Timestamp is outside acceptable rewind window."));
        return false;
    }

    OutResult = FMPValidationResult::Success();
    return true;
}

bool UMPNetFunctionLibrary::ValidateFloatRange(float Value, float MinValue, float MaxValue, FMPValidationResult& OutResult)
{
    if (FMath::IsNaN(Value) || !FMath::IsFinite(Value))
    {
        OutResult = FMPValidationResult::Failure(EMPValidationReason::Failed_NaN, TEXT("Value is NaN or Infinite."));
        return false;
    }

    if (Value < MinValue || Value > MaxValue)
    {
        OutResult = FMPValidationResult::Failure(EMPValidationReason::Failed_OutOfRange, FString::Printf(TEXT("Value %.2f is outside [%.2f, %.2f]"), Value, MinValue, MaxValue));
        return false;
    }

    OutResult = FMPValidationResult::Success();
    return true;
}

bool UMPNetFunctionLibrary::ValidateVectorNotNaN(const FVector& Vec, FMPValidationResult& OutResult)
{
    if (Vec.ContainsNaN())
    {
        OutResult = FMPValidationResult::Failure(EMPValidationReason::Failed_NaN, TEXT("Vector contains NaN or Infinite components."));
        return false;
    }

    OutResult = FMPValidationResult::Success();
    return true;
}

bool UMPNetFunctionLibrary::ValidateEnumValue(uint8 Value, uint8 MaxValidValue, FMPValidationResult& OutResult)
{
    if (Value > MaxValidValue)
    {
        OutResult = FMPValidationResult::Failure(EMPValidationReason::Failed_InvalidEnum, FString::Printf(TEXT("Enum ordinal %u exceeds max %u"), Value, MaxValidValue));
        return false;
    }

    OutResult = FMPValidationResult::Success();
    return true;
}

FVector UMPNetFunctionLibrary::ClampClientInputVector(const FVector& InputVector, float MaxLength)
{
    if (InputVector.ContainsNaN())
    {
        return FVector::ZeroVector;
    }
    return InputVector.GetClampedToMaxSize(FMath::Max(0.0f, MaxLength));
}

bool UMPNetFunctionLibrary::IsValidSpawnLocation(const UObject* WorldContextObject, const FVector& Location, const FVector& Extent, bool bCheckCollision)
{
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!World) return false;

    if (bCheckCollision)
    {
        FCollisionQueryParams Params;
        const bool bOverlap = World->OverlapBlockingTestByChannel(
            Location,
            FQuat::Identity,
            ECC_WorldStatic,
            FCollisionShape::MakeBox(Extent),
            Params
        );
        if (bOverlap) return false;
    }

    return true;
}

bool UMPNetFunctionLibrary::IsPathClearServer(const UObject* WorldContextObject, const FVector& Start, const FVector& End, ECollisionChannel Channel, const AActor* IgnoreActor)
{
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!World) return false;

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(IsPathClearServer), false);
    if (IgnoreActor)
    {
        Params.AddIgnoredActor(IgnoreActor);
    }

    const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, Channel, Params);
    return !bHit;
}

bool UMPNetFunctionLibrary::SanitizePlayerName(const FString& InName, int32 MinLength, int32 MaxLength, FString& OutSanitizedName, FMPValidationResult& OutResult)
{
    OutSanitizedName = InName.TrimStartAndEnd();

    // Strip unprintable / control characters
    FString Filtered;
    for (int32 i = 0; i < OutSanitizedName.Len(); ++i)
    {
        TCHAR C = OutSanitizedName[i];
        if (FChar::IsPrint(C) && C != TEXT('\t') && C != TEXT('\r') && C != TEXT('\n'))
        {
            Filtered.AppendChar(C);
        }
    }
    OutSanitizedName = Filtered;

    if (OutSanitizedName.Len() < MinLength)
    {
        OutResult = FMPValidationResult::Failure(EMPValidationReason::Failed_InvalidName, FString::Printf(TEXT("Name is too short (%d < %d)"), OutSanitizedName.Len(), MinLength));
        return false;
    }

    if (OutSanitizedName.Len() > MaxLength)
    {
        OutSanitizedName = OutSanitizedName.Left(MaxLength);
    }

    OutResult = FMPValidationResult::Success();
    return true;
}

// =========================================================================
// AUTHORITY-AWARE COMPONENT & SPAWN HELPERS
// =========================================================================

UActorComponent* UMPNetFunctionLibrary::AddReplicatedComponentServerOnly(AActor* Owner, TSubclassOf<UActorComponent> ComponentClass, FName ComponentName)
{
    if (!Owner || !Owner->HasAuthority() || !ComponentClass)
    {
        return nullptr;
    }

    FName ActualName = (ComponentName != NAME_None) ? ComponentName : MakeUniqueObjectName(Owner, ComponentClass);
    UActorComponent* NewComp = NewObject<UActorComponent>(Owner, ComponentClass, ActualName);
    if (NewComp)
    {
        NewComp->SetIsReplicated(true);
        NewComp->RegisterComponent();
    }
    return NewComp;
}

UActorComponent* UMPNetFunctionLibrary::FindComponentSafe(const AActor* Actor, TSubclassOf<UActorComponent> ComponentClass, FGameplayTag ComponentTag)
{
    if (!Actor || !ComponentClass) return nullptr;

    if (ComponentTag.IsValid())
    {
        TArray<UActorComponent*> Comps;
        Actor->GetComponents(ComponentClass, Comps);
        for (UActorComponent* Comp : Comps)
        {
            if (Comp && Comp->ComponentHasTag(ComponentTag.GetTagName()))
            {
                return Comp;
            }
        }
        return nullptr;
    }

    return Actor->FindComponentByClass(ComponentClass);
}

AActor* UMPNetFunctionLibrary::SpawnActorDeferredServerOnly(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform, AActor* Owner, APawn* Instigator, ESpawnActorCollisionHandlingMethod CollisionMethod)
{
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!World || !ActorClass || World->GetNetMode() == NM_Client)
    {
        return nullptr;
    }

    return World->SpawnActorDeferred<AActor>(ActorClass, SpawnTransform, Owner, Instigator, CollisionMethod);
}

AActor* UMPNetFunctionLibrary::FinishSpawningActorServerOnly(AActor* Actor, const FTransform& SpawnTransform)
{
    if (!Actor || !Actor->HasAuthority())
    {
        return nullptr;
    }

    return UGameplayStatics::FinishSpawningActor(Actor, SpawnTransform);
}

APawn* UMPNetFunctionLibrary::SpawnAtFreePlayerStartSafe(const UObject* WorldContextObject, TSubclassOf<AActor> PawnClass, AController* Controller)
{
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!World || !PawnClass || World->GetNetMode() == NM_Client)
    {
        return nullptr;
    }

    APlayerStart* ChosenStart = nullptr;
    for (TActorIterator<APlayerStart> It(World); It; ++It)
    {
        APlayerStart* Start = *It;
        if (IsValid(Start))
        {
            // Verify start position is free
            const bool bBlocked = World->OverlapBlockingTestByChannel(
                Start->GetActorLocation(),
                FQuat::Identity,
                ECC_Pawn,
                FCollisionShape::MakeCapsule(40.0f, 90.0f)
            );
            if (!bBlocked)
            {
                ChosenStart = Start;
                break;
            }
        }
    }

    const FTransform SpawnTransform = ChosenStart ? ChosenStart->GetActorTransform() : FTransform::Identity;
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    APawn* NewPawn = World->SpawnActor<APawn>(PawnClass, SpawnTransform, Params);

    if (NewPawn && Controller && Controller->HasAuthority())
    {
        Controller->Possess(NewPawn);
    }

    return NewPawn;
}

bool UMPNetFunctionLibrary::TeleportSafeServer(AActor* TargetActor, const FVector& DestLocation, const FRotator& DestRotation, bool bStopVelocity)
{
    if (!TargetActor || !TargetActor->HasAuthority())
    {
        return false;
    }

    const bool bSuccess = TargetActor->TeleportTo(DestLocation, DestRotation);
    if (bSuccess && bStopVelocity)
    {
        if (ACharacter* Char = Cast<ACharacter>(TargetActor))
        {
            if (UCharacterMovementComponent* CMC = Char->GetCharacterMovement())
            {
                CMC->Velocity = FVector::ZeroVector;
            }
        }
        else if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(TargetActor->GetRootComponent()))
        {
            if (Prim->IsSimulatingPhysics())
            {
                Prim->SetPhysicsLinearVelocity(FVector::ZeroVector);
                Prim->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
            }
        }
    }
    return bSuccess;
}

APlayerState* UMPNetFunctionLibrary::GetOwningPlayerStateSafe(const AActor* TargetActor)
{
    if (!TargetActor) return nullptr;

    if (TargetActor->Implements<UMPOwnershipInterface>())
    {
        if (APlayerState* PS = IMPOwnershipInterface::Execute_GetLogicalOwnerPlayerState(TargetActor))
        {
            return PS;
        }
    }

    if (const APawn* Pawn = Cast<APawn>(TargetActor))
    {
        if (APlayerState* PS = Pawn->GetPlayerState())
        {
            return PS;
        }
    }

    if (const AController* Controller = Cast<AController>(TargetActor))
    {
        return Controller->PlayerState;
    }

    const AActor* Owner = TargetActor->GetOwner();
    int32 Depth = 0;
    while (Owner && Depth < 8)
    {
        if (Owner->Implements<UMPOwnershipInterface>())
        {
            if (APlayerState* PS = IMPOwnershipInterface::Execute_GetLogicalOwnerPlayerState(Owner))
            {
                return PS;
            }
        }
        if (const APawn* Pawn = Cast<APawn>(Owner))
        {
            if (APlayerState* PS = Pawn->GetPlayerState())
            {
                return PS;
            }
        }
        if (const AController* Controller = Cast<AController>(Owner))
        {
            return Controller->PlayerState;
        }
        Owner = Owner->GetOwner();
        Depth++;
    }

    return nullptr;
}

APawn* UMPNetFunctionLibrary::GetOwningPawnSafe(const AActor* TargetActor)
{
    if (!TargetActor) return nullptr;

    if (const APawn* Pawn = Cast<APawn>(TargetActor))
    {
        return const_cast<APawn*>(Pawn);
    }

    if (const AController* Controller = Cast<AController>(TargetActor))
    {
        return Controller->GetPawn();
    }

    const AActor* Owner = TargetActor->GetOwner();
    int32 Depth = 0;
    while (Owner && Depth < 8)
    {
        if (const APawn* Pawn = Cast<APawn>(Owner))
        {
            return const_cast<APawn*>(Pawn);
        }
        if (const AController* Controller = Cast<AController>(Owner))
        {
            return Controller->GetPawn();
        }
        Owner = Owner->GetOwner();
        Depth++;
    }

    return TargetActor->GetInstigator();
}

void UMPNetFunctionLibrary::ApplyNetPresetToActor(AActor* TargetActor, const UMPNetPresetAsset* Preset)
{
    if (Preset && TargetActor)
    {
        Preset->ApplyToActor(TargetActor);
    }
}


