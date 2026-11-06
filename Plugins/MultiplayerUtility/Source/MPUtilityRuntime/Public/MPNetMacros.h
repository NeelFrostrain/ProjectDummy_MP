#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "Misc/EngineVersionComparison.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "MPUtilitySettings.h"
#include "MPUtilityRuntimeModule.h"

// =============================================================================
// A. NET MODE ENFORCEMENT GUARDS
// =============================================================================

#define MP_SERVER_ONLY(WorldContext) \
    do { \
        const UWorld* _MP_W = (WorldContext) ? (WorldContext)->GetWorld() : nullptr; \
        if (!_MP_W || _MP_W->GetNetMode() == NM_Client) { \
            UE_LOG(LogTemp, Warning, TEXT("[MPNetMacros] '%s' is Server-Only but was invoked on Client!"), *FString(__FUNCTION__)); \
            return; \
        } \
    } while (0)

#define MP_SERVER_ONLY_RET(WorldContext, Value) \
    do { \
        const UWorld* _MP_W = (WorldContext) ? (WorldContext)->GetWorld() : nullptr; \
        if (!_MP_W || _MP_W->GetNetMode() == NM_Client) { \
            UE_LOG(LogTemp, Warning, TEXT("[MPNetMacros] '%s' is Server-Only but was invoked on Client!"), *FString(__FUNCTION__)); \
            return (Value); \
        } \
    } while (0)

#define MP_CLIENT_ONLY(WorldContext) \
    do { \
        const UWorld* _MP_W = (WorldContext) ? (WorldContext)->GetWorld() : nullptr; \
        if (!_MP_W || _MP_W->GetNetMode() == NM_DedicatedServer) { \
            UE_LOG(LogTemp, Warning, TEXT("[MPNetMacros] '%s' is Client-Only but was invoked on Dedicated Server!"), *FString(__FUNCTION__)); \
            return; \
        } \
    } while (0)

#define MP_CLIENT_ONLY_RET(WorldContext, Value) \
    do { \
        const UWorld* _MP_W = (WorldContext) ? (WorldContext)->GetWorld() : nullptr; \
        if (!_MP_W || _MP_W->GetNetMode() == NM_DedicatedServer) { \
            UE_LOG(LogTemp, Warning, TEXT("[MPNetMacros] '%s' is Client-Only but was invoked on Dedicated Server!"), *FString(__FUNCTION__)); \
            return (Value); \
        } \
    } while (0)

#define MP_LISTEN_ONLY(WorldContext) \
    do { \
        const UWorld* _MP_W = (WorldContext) ? (WorldContext)->GetWorld() : nullptr; \
        if (!_MP_W || _MP_W->GetNetMode() != NM_ListenServer) { \
            return; \
        } \
    } while (0)

#define MP_LISTEN_ONLY_RET(WorldContext, Value) \
    do { \
        const UWorld* _MP_W = (WorldContext) ? (WorldContext)->GetWorld() : nullptr; \
        if (!_MP_W || _MP_W->GetNetMode() != NM_ListenServer) { \
            return (Value); \
        } \
    } while (0)

#define MP_DEDICATED_ONLY(WorldContext) \
    do { \
        const UWorld* _MP_W = (WorldContext) ? (WorldContext)->GetWorld() : nullptr; \
        if (!_MP_W || _MP_W->GetNetMode() != NM_DedicatedServer) { \
            return; \
        } \
    } while (0)

#define MP_DEDICATED_ONLY_RET(WorldContext, Value) \
    do { \
        const UWorld* _MP_W = (WorldContext) ? (WorldContext)->GetWorld() : nullptr; \
        if (!_MP_W || _MP_W->GetNetMode() != NM_DedicatedServer) { \
            return (Value); \
        } \
    } while (0)

#define MP_RETURN_IF_DEDICATED(WorldContext) \
    do { \
        const UWorld* _MP_W = (WorldContext) ? (WorldContext)->GetWorld() : nullptr; \
        if (_MP_W && _MP_W->GetNetMode() == NM_DedicatedServer) { return; } \
    } while (0)

#define MP_RETURN_IF_DEDICATED_RET(WorldContext, ReturnValue) \
    do { \
        const UWorld* _MP_W = (WorldContext) ? (WorldContext)->GetWorld() : nullptr; \
        if (_MP_W && _MP_W->GetNetMode() == NM_DedicatedServer) { return (ReturnValue); } \
    } while (0)

#define MP_RETURN_IF_STANDALONE(WorldContext) \
    do { \
        const UWorld* _MP_W = (WorldContext) ? (WorldContext)->GetWorld() : nullptr; \
        if (!_MP_W || _MP_W->GetNetMode() == NM_Standalone) { return; } \
    } while (0)

#define MP_RETURN_IF_STANDALONE_RET(WorldContext, ReturnValue) \
    do { \
        const UWorld* _MP_W = (WorldContext) ? (WorldContext)->GetWorld() : nullptr; \
        if (!_MP_W || _MP_W->GetNetMode() == NM_Standalone) { return (ReturnValue); } \
    } while (0)

// =============================================================================
// B. BLOCK-STYLE SCOPED EXECUTION MACROS (1-ITERATION SAFE, NO DANGLING-ELSE)
// =============================================================================

#define MP_ON_SERVER(WorldContext) \
    for (bool _MP_Once_ = false; !_MP_Once_ && (WorldContext) && (WorldContext)->GetWorld() && ((WorldContext)->GetWorld()->GetNetMode() != NM_Client); _MP_Once_ = true)

#define MP_ON_CLIENT(WorldContext) \
    for (bool _MP_Once_ = false; !_MP_Once_ && (WorldContext) && (WorldContext)->GetWorld() && ((WorldContext)->GetWorld()->GetNetMode() != NM_DedicatedServer); _MP_Once_ = true)

#define MP_ON_LISTEN(WorldContext) \
    for (bool _MP_Once_ = false; !_MP_Once_ && (WorldContext) && (WorldContext)->GetWorld() && ((WorldContext)->GetWorld()->GetNetMode() == NM_ListenServer); _MP_Once_ = true)

// =============================================================================
// C. ACTOR AUTHORITY & LOCALITY GUARDS
// =============================================================================

#define MP_CHECK_AUTHORITY(Actor) \
    do { \
        if (!(Actor) || !(Actor)->HasAuthority()) { \
            return; \
        } \
    } while (0)

#define MP_CHECK_AUTHORITY_RET(Actor, ReturnValue) \
    do { \
        if (!(Actor) || !(Actor)->HasAuthority()) { \
            return (ReturnValue); \
        } \
    } while (0)

#define MP_CHECK_LOCAL(Actor) \
    do { \
        const APawn* _MP_P = Cast<const APawn>(Actor); \
        const APlayerController* _MP_PC = Cast<const APlayerController>(Actor); \
        const bool _MP_bLoc = (_MP_P && _MP_P->IsLocallyControlled()) || (_MP_PC && _MP_PC->IsLocalPlayerController()); \
        if (!_MP_bLoc) { \
            return; \
        } \
    } while (0)

#define MP_CHECK_LOCAL_RET(Actor, ReturnValue) \
    do { \
        const APawn* _MP_P = Cast<const APawn>(Actor); \
        const APlayerController* _MP_PC = Cast<const APlayerController>(Actor); \
        const bool _MP_bLoc = (_MP_P && _MP_P->IsLocallyControlled()) || (_MP_PC && _MP_PC->IsLocalPlayerController()); \
        if (!_MP_bLoc) { \
            return (ReturnValue); \
        } \
    } while (0)

#define MP_CHECK_VALID(Ptr, Reason) \
    do { \
        if (!(Ptr)) { \
            UE_LOG(LogTemp, Warning, TEXT("[MPNetMacros] '%s' check failed: %s"), *FString(__FUNCTION__), TEXT(Reason)); \
            return; \
        } \
    } while (0)

#define MP_CHECK_VALID_RET(Ptr, ReturnValue, Reason) \
    do { \
        if (!(Ptr)) { \
            UE_LOG(LogTemp, Warning, TEXT("[MPNetMacros] '%s' check failed: %s"), *FString(__FUNCTION__), TEXT(Reason)); \
            return (ReturnValue); \
        } \
    } while (0)

#define MP_CHECK_OWNER(Actor) \
    do { \
        if (!(Actor) || !(Actor)->GetOwner()) { \
            return; \
        } \
    } while (0)

#define MP_CHECK_OWNER_RET(Actor, ReturnValue) \
    do { \
        if (!(Actor) || !(Actor)->GetOwner()) { \
            return (ReturnValue); \
        } \
    } while (0)

#define MP_CHECK_CONNECTION(Actor) \
    do { \
        if (!(Actor) || !(Actor)->GetNetConnection()) { \
            UE_LOG(LogTemp, Warning, TEXT("[MPNetMacros] '%s' lacks NetConnection!"), *GetNameSafe(Actor)); \
            return; \
        } \
    } while (0)

#define MP_CHECK_CONNECTION_RET(Actor, ReturnValue) \
    do { \
        if (!(Actor) || !(Actor)->GetNetConnection()) { \
            UE_LOG(LogTemp, Warning, TEXT("[MPNetMacros] '%s' lacks NetConnection!"), *GetNameSafe(Actor)); \
            return (ReturnValue); \
        } \
    } while (0)

#define MP_CHECK_REPLICATED(Actor) \
    do { \
        if (!(Actor) || !(Actor)->GetIsReplicated()) { \
            return; \
        } \
    } while (0)

#define MP_CHECK_REPLICATED_RET(Actor, ReturnValue) \
    do { \
        if (!(Actor) || !(Actor)->GetIsReplicated()) { \
            return (ReturnValue); \
        } \
    } while (0)

// =============================================================================
// D. ITERATOR HELPER (FOR EACH PLAYER CONTROLLER)
// =============================================================================

#define MP_FOR_EACH_PLAYER(WorldContext, PCVar) \
    if (const UWorld* _MP_LoopWorld_ = (WorldContext) ? (WorldContext)->GetWorld() : nullptr) \
        for (FConstPlayerControllerIterator _MP_It_ = _MP_LoopWorld_->GetPlayerControllerIterator(); _MP_It_; ++_MP_It_) \
            if (APlayerController* PCVar = _MP_It_->Get())

// =============================================================================
// E. RPC VALIDATION & KICK MACROS
// =============================================================================

#define MP_RPC_VALIDATE(Condition, Reason) \
    do { \
        if (!(Condition)) { \
            UE_LOG(LogTemp, Warning, TEXT("[MP_RPC_VALIDATE] Client RPC rejected in '%s': %s"), *FString(__FUNCTION__), TEXT(Reason)); \
            return false; \
        } \
    } while (0)

#define MP_VALIDATE_OR_KICK(Controller, Condition, Reason) \
    do { \
        if (!(Condition)) { \
            UE_LOG(LogTemp, Warning, TEXT("[MP_VALIDATE_OR_KICK] Validation failed for '%s': %s"), *GetNameSafe(Controller), TEXT(Reason)); \
            if (const UMPUtilitySettings* _MP_Settings = UMPUtilitySettings::Get()) { \
                if (_MP_Settings->bKickOnValidationFailure && (Controller)) { \
                    (Controller)->ClientWasKicked(FText::FromString(Reason)); \
                } \
            } \
            return; \
        } \
    } while (0)

#define MP_VALIDATE_OR_KICK_RET(Controller, Condition, Reason, ReturnValue) \
    do { \
        if (!(Condition)) { \
            UE_LOG(LogTemp, Warning, TEXT("[MP_VALIDATE_OR_KICK] Validation failed for '%s': %s"), *GetNameSafe(Controller), TEXT(Reason)); \
            if (const UMPUtilitySettings* _MP_Settings = UMPUtilitySettings::Get()) { \
                if (_MP_Settings->bKickOnValidationFailure && (Controller)) { \
                    (Controller)->ClientWasKicked(FText::FromString(Reason)); \
                } \
            } \
            return (ReturnValue); \
        } \
    } while (0)

// =============================================================================
// F. LOGGING & SCREEN DIAGNOSTICS
// =============================================================================

/**
 * Standard utility logging macro directing output to LogMPUtility while honoring UMPUtilitySettings verbose filtering.
 */
#define MP_LOG(Verbosity, Format, ...) \
    do { \
        const UMPUtilitySettings* _MPSettings_ = UMPUtilitySettings::Get(); \
        if (_MPSettings_ && !_MPSettings_->bEnableVerboseNetLogging && (ELogVerbosity::Verbosity > ELogVerbosity::Log)) { \
            break; \
        } \
        UE_LOG(LogMPUtility, Verbosity, Format, ##__VA_ARGS__); \
    } while (0)

#define MP_LOG_SERVER(Format, ...) \
    UE_LOG(LogMPUtility, Log, TEXT("[Server][%s] ") Format, *FString(__FUNCTION__), ##__VA_ARGS__)

#define MP_LOG_CLIENT(Format, ...) \
    UE_LOG(LogMPUtility, Log, TEXT("[Client][%s] ") Format, *FString(__FUNCTION__), ##__VA_ARGS__)

#define MP_LOG_NET(WorldContext, Format, ...) \
    do { \
        const UWorld* _MP_W = (WorldContext) ? (WorldContext)->GetWorld() : nullptr; \
        const TCHAR* _MP_Pfx = _MP_W ? \
            (_MP_W->GetNetMode() == NM_Client ? TEXT("[Client]") : \
             _MP_W->GetNetMode() == NM_DedicatedServer ? TEXT("[DedicatedServer]") : \
             _MP_W->GetNetMode() == NM_ListenServer ? TEXT("[ListenServer]") : TEXT("[Standalone]")) : TEXT("[Unknown]"); \
        UE_LOG(LogMPUtility, Log, TEXT("%s[%s] ") Format, _MP_Pfx, *FString(__FUNCTION__), ##__VA_ARGS__); \
    } while (0)

#define MP_LOG_WARN_ONCE(Format, ...) \
    do { \
        static bool _MP_bLoggedOnce = false; \
        static const auto* _CVarLogOnce_ = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("mp.Debug.LogOnce")); \
        const bool _bLogOnceEnabled_ = !_CVarLogOnce_ || _CVarLogOnce_->GetValueOnAnyThread() != 0; \
        if (!_MP_bLoggedOnce || !_bLogOnceEnabled_) { \
            _MP_bLoggedOnce = true; \
            UE_LOG(LogMPUtility, Warning, TEXT("[ONCE][%s] ") Format, *FString(__FUNCTION__), ##__VA_ARGS__); \
        } \
    } while (0)

#define MP_LOG_ONCE_PER_ACTOR(Actor, Format, ...) \
    do { \
        if (Actor) { \
            static TSet<uint32> _MP_LoggedActorIDs; \
            const uint32 _MP_ActorUID = (Actor)->GetUniqueID(); \
            if (!_MP_LoggedActorIDs.Contains(_MP_ActorUID)) { \
                _MP_LoggedActorIDs.Add(_MP_ActorUID); \
                UE_LOG(LogTemp, Warning, TEXT("[ONCE_ACTOR][%s][%s] ") Format, *(Actor)->GetName(), *FString(__FUNCTION__), ##__VA_ARGS__); \
            } \
        } \
    } while (0)

#define MP_LOG_EVERY_N_SEC(Sec, Format, ...) \
    do { \
        static double _MP_LastLogTime_ = -999999.0; \
        const double _MP_Now_ = FPlatformTime::Seconds(); \
        if (_MP_Now_ - _MP_LastLogTime_ >= (Sec)) { \
            _MP_LastLogTime_ = _MP_Now_; \
            UE_LOG(LogTemp, Warning, TEXT("[RATE_LIMITED][%s] ") Format, *FString(__FUNCTION__), ##__VA_ARGS__); \
        } \
    } while (0)

#define MP_SCREEN_MSG(Key, Time, Color, Format, ...) \
    do { \
        if (GEngine) { \
            GEngine->AddOnScreenDebugMessage(Key, Time, Color, FString::Printf(Format, ##__VA_ARGS__)); \
        } \
    } while (0)

// =============================================================================
// G. BUILD CONFIGURATION & PROFILING
// =============================================================================

#if WITH_EDITOR
    #define MP_ONLY_IN_EDITOR(Code) do { Code; } while (0)
#else
    #define MP_ONLY_IN_EDITOR(Code) do { } while (0)
#endif

#if !UE_BUILD_SHIPPING
    #define MP_ONLY_IN_DEV(Code) do { Code; } while (0)
    #define MP_NET_SCOPE(Name) TRACE_CPUPROFILER_EVENT_SCOPE_STR(Name)
    #define MP_COUNT_RPC(Name) do { static int32 _MP_RPC_Count = 0; ++_MP_RPC_Count; } while (0)
#else
    #define MP_ONLY_IN_DEV(Code) do { } while (0)
    #define MP_NET_SCOPE(Name) do { } while (0)
    #define MP_COUNT_RPC(Name) do { } while (0)
#endif

#include "Types/MPScopedGuards.h"

#define MP_SCOPED_NET_TIMER(Name) \
    FMPScopedNetTimer _MP_Timer(TEXT(Name))

// =============================================================================
// H. VALUE REPLICATION & CHANGE DETECTION HELPERS
// =============================================================================

namespace UE::MPNet
{
    FORCEINLINE bool IsChangedEnough(float Current, float NewValue, float Threshold)
    {
        return FMath::Abs(NewValue - Current) >= Threshold;
    }

    FORCEINLINE bool IsChangedEnough(double Current, double NewValue, double Threshold)
    {
        return FMath::Abs(NewValue - Current) >= Threshold;
    }

    FORCEINLINE bool IsChangedEnough(const FVector& Current, const FVector& NewValue, float Threshold)
    {
        return FVector::DistSquared(Current, NewValue) >= FMath::Square(static_cast<double>(Threshold));
    }

    FORCEINLINE bool IsChangedEnough(const FRotator& Current, const FRotator& NewValue, float Threshold)
    {
        return !Current.Equals(NewValue, Threshold);
    }
}

#define MP_MARK_DIRTY(Actor, VarName) \
    MARK_PROPERTY_DIRTY_FROM_NAME(Actor->GetClass(), VarName, Actor)

#define MP_REPLICATE_IF_CHANGED_ENOUGH(Actor, Var, NewValue, Threshold) \
    do { \
        if (UE::MPNet::IsChangedEnough((Var), (NewValue), (Threshold))) { \
            (Var) = (NewValue); \
            MP_MARK_DIRTY(Actor, Var); \
        } \
    } while (0)

#define MP_REP_NOTIFY_IF_CHANGED(Actor, Var, NewValue, OnRepFunc) \
    do { \
        if ((Var) != (NewValue)) { \
            (Var) = (NewValue); \
            MP_MARK_DIRTY(Actor, Var); \
            (Actor)->OnRepFunc(); \
        } \
    } while (0)

#define MP_CALL_ONREP_ON_SERVER(Actor, OnRepFunc) \
    do { \
        if ((Actor) && (Actor)->HasAuthority()) { \
            (Actor)->OnRepFunc(); \
        } \
    } while (0)

// =============================================================================
// I. LIFETIME REPLICATION SHORTCUTS (FOR GetLifetimeReplicatedProps)
// =============================================================================

#define MP_DOREP_PROPERTY(Class, Var) \
    DOREPLIFETIME(Class, Var)

#define MP_DOREP_OWNER_ONLY(Class, Var) \
    DOREPLIFETIME_CONDITION(Class, Var, COND_OwnerOnly)

#define MP_DOREP_SKIP_OWNER(Class, Var) \
    DOREPLIFETIME_CONDITION(Class, Var, COND_SkipOwner)

#define MP_DOREP_INITIAL_ONLY(Class, Var) \
    DOREPLIFETIME_CONDITION(Class, Var, COND_InitialOnly)

#define MP_DOREP_SIMULATED_ONLY(Class, Var) \
    DOREPLIFETIME_CONDITION(Class, Var, COND_SimulatedOnly)

// =============================================================================
// J. NETMODE SWITCH EXPRESSION MACRO
// =============================================================================

#define MP_NETMODE_SWITCH(WorldContext, StandaloneExpr, DedicatedExpr, ListenExpr, ClientExpr) \
    ([&]() { \
        const UWorld* _MP_W_ = (WorldContext) ? (WorldContext)->GetWorld() : nullptr; \
        if (!_MP_W_) return (StandaloneExpr); \
        switch (_MP_W_->GetNetMode()) { \
            case NM_DedicatedServer: return (DedicatedExpr); \
            case NM_ListenServer: return (ListenExpr); \
            case NM_Client: return (ClientExpr); \
            default: return (StandaloneExpr); \
        } \
    }())

// =============================================================================
// K. STATIC TYPE CHECKING & DEV ASSERTIONS
// =============================================================================

#define MP_STATIC_ASSERT_REPLICATED_TYPE(InType) \
    static_assert(!TIsPointer<InType>::Value || TIsDerivedFrom<typename TRemovePointer<InType>::Type, UObject>::Value, \
        "Raw C++ pointers cannot be replicated in Unreal Engine! Only UObject* pointers or value types with net serialization are supported.")

#if !UE_BUILD_SHIPPING
    #define MP_ASSERT_AUTHORITY(Actor) \
        checkf((Actor) && (Actor)->HasAuthority(), TEXT("Actor '%s' must have authority in '%s'!"), *GetNameSafe(Actor), *FString(__FUNCTION__))

    #define MP_ENSURE_NET_MODE(WorldContext, ExpectedMode) \
        ensureMsgf(((WorldContext) && (WorldContext)->GetWorld() && (WorldContext)->GetWorld()->GetNetMode() == (ExpectedMode)), \
            TEXT("NetMode mismatch in '%s'!"), *FString(__FUNCTION__))
#else
    #define MP_ASSERT_AUTHORITY(Actor) do { } while (0)
    #define MP_ENSURE_NET_MODE(WorldContext, ExpectedMode) do { } while (0)
#endif

// =============================================================================
// L. ADVANCED MULTIPLAYER MACROS (BATCH 9)
// =============================================================================

/**
 * Ensures the target object has an accessible UWorld pointer; logs a warning and returns if null.
 *
 * Example:
 *   void AWeapon::Fire()
 *   {
 *       MP_ENSURE_WORLD(this);
 *       // safe to use GetWorld()
 *   }
 */
#define MP_ENSURE_WORLD(Obj) \
    do { \
        const UObject* _MP_Obj_ = (Obj); \
        const UWorld* _MP_W_ = _MP_Obj_ ? _MP_Obj_->GetWorld() : nullptr; \
        if (!_MP_W_) { \
            UE_LOG(LogMPUtility, Warning, TEXT("[MP_ENSURE_WORLD] '%s' has null World in '%s'!"), \
                *GetNameSafe(_MP_Obj_), *FString(__FUNCTION__)); \
            return; \
        } \
    } while (0)

#define MP_ENSURE_WORLD_RET(Obj, RetVal) \
    do { \
        const UObject* _MP_Obj_ = (Obj); \
        const UWorld* _MP_W_ = _MP_Obj_ ? _MP_Obj_->GetWorld() : nullptr; \
        if (!_MP_W_) { \
            UE_LOG(LogMPUtility, Warning, TEXT("[MP_ENSURE_WORLD] '%s' has null World in '%s'!"), \
                *GetNameSafe(_MP_Obj_), *FString(__FUNCTION__)); \
            return (RetVal); \
        } \
    } while (0)

/**
 * Ensures execution occurs exclusively on an authoritative server; logs warning and returns otherwise.
 *
 * Example:
 *   bool AInventoryManager::GrantLoot()
 *   {
 *       MP_ENSURE_SERVER_OR_RETURN_RET(this, false);
 *       return true;
 *   }
 */
#define MP_ENSURE_SERVER_OR_RETURN(Obj) \
    do { \
        const UObject* _MP_Obj_ = (Obj); \
        const UWorld* _MP_W_ = _MP_Obj_ ? _MP_Obj_->GetWorld() : nullptr; \
        if (!_MP_W_ || _MP_W_->GetNetMode() == NM_Client) { \
            UE_LOG(LogMPUtility, Warning, TEXT("[MP_ENSURE_SERVER] Server authority required in '%s'!"), *FString(__FUNCTION__)); \
            return; \
        } \
    } while (0)

#define MP_ENSURE_SERVER_OR_RETURN_RET(Obj, RetVal) \
    do { \
        const UObject* _MP_Obj_ = (Obj); \
        const UWorld* _MP_W_ = _MP_Obj_ ? _MP_Obj_->GetWorld() : nullptr; \
        if (!_MP_W_ || _MP_W_->GetNetMode() == NM_Client) { \
            UE_LOG(LogMPUtility, Warning, TEXT("[MP_ENSURE_SERVER] Server authority required in '%s'!"), *FString(__FUNCTION__)); \
            return (RetVal); \
        } \
    } while (0)

/**
 * Resolves the AGameStateBase from WorldContext into Var; returns immediately if null.
 *
 * Example:
 *   void AController::EvaluateGameState()
 *   {
 *       MP_GET_GAMESTATE_OR_RETURN(GameState, this);
 *       // GameState is valid AGameStateBase*
 *   }
 */
#define MP_GET_GAMESTATE_OR_RETURN(Var, WorldContext) \
    AGameStateBase* Var = nullptr; \
    do { \
        const UObject* _MP_Ctx_ = (WorldContext); \
        const UWorld* _MP_W_ = _MP_Ctx_ ? _MP_Ctx_->GetWorld() : nullptr; \
        Var = _MP_W_ ? _MP_W_->GetGameState<AGameStateBase>() : nullptr; \
        if (!Var) { \
            UE_LOG(LogMPUtility, Warning, TEXT("[MP_GET_GAMESTATE] GameState not found in '%s'!"), *FString(__FUNCTION__)); \
            return; \
        } \
    } while (0)

#define MP_GET_GAMESTATE_OR_RETURN_RET(Var, WorldContext, RetVal) \
    AGameStateBase* Var = nullptr; \
    do { \
        const UObject* _MP_Ctx_ = (WorldContext); \
        const UWorld* _MP_W_ = _MP_Ctx_ ? _MP_Ctx_->GetWorld() : nullptr; \
        Var = _MP_W_ ? _MP_W_->GetGameState<AGameStateBase>() : nullptr; \
        if (!Var) { \
            UE_LOG(LogMPUtility, Warning, TEXT("[MP_GET_GAMESTATE] GameState not found in '%s'!"), *FString(__FUNCTION__)); \
            return (RetVal); \
        } \
    } while (0)

/**
 * Resolves APlayerState from a Pawn into Var; returns immediately if null.
 *
 * Example:
 *   void ACharacter::AwardScore()
 *   {
 *       MP_GET_PLAYERSTATE_OR_RETURN(PlayerState, this);
 *       // PlayerState is valid APlayerState*
 *   }
 */
#define MP_GET_PLAYERSTATE_OR_RETURN(Var, Pawn) \
    APlayerState* Var = nullptr; \
    do { \
        const APawn* _MP_P_ = (Pawn); \
        Var = _MP_P_ ? _MP_P_->GetPlayerState() : nullptr; \
        if (!Var) { \
            UE_LOG(LogMPUtility, Warning, TEXT("[MP_GET_PLAYERSTATE] PlayerState not found on '%s' in '%s'!"), \
                *GetNameSafe(_MP_P_), *FString(__FUNCTION__)); \
            return; \
        } \
    } while (0)

#define MP_GET_PLAYERSTATE_OR_RETURN_RET(Var, Pawn, RetVal) \
    APlayerState* Var = nullptr; \
    do { \
        const APawn* _MP_P_ = (Pawn); \
        Var = _MP_P_ ? _MP_P_->GetPlayerState() : nullptr; \
        if (!Var) { \
            UE_LOG(LogMPUtility, Warning, TEXT("[MP_GET_PLAYERSTATE] PlayerState not found on '%s' in '%s'!"), \
                *GetNameSafe(_MP_P_), *FString(__FUNCTION__)); \
            return (RetVal); \
        } \
    } while (0)

/**
 * Asserts the current world net mode in non-shipping builds.
 *
 * Example:
 *   void AClientUI::DisplayMenu()
 *   {
 *       MP_ASSERT_NET_MODE(this, NM_Client);
 *   }
 */
#if !UE_BUILD_SHIPPING
    #define MP_ASSERT_NET_MODE(WorldContext, Mode) \
        do { \
            const UObject* _MP_Ctx_ = (WorldContext); \
            const UWorld* _MP_W_ = _MP_Ctx_ ? _MP_Ctx_->GetWorld() : nullptr; \
            checkf(_MP_W_ && _MP_W_->GetNetMode() == (Mode), \
                TEXT("[MP_ASSERT_NET_MODE] Expected NetMode %d, got %d in '%s'"), \
                static_cast<int32>(Mode), _MP_W_ ? static_cast<int32>(_MP_W_->GetNetMode()) : -1, *FString(__FUNCTION__)); \
        } while (0)
#else
    #define MP_ASSERT_NET_MODE(WorldContext, Mode) do { } while (0)
#endif

/**
 * Marks a function or symbol as deprecated since a specific engine/plugin version.
 *
 * Example:
 *   MP_DEPRECATED_SINCE(5.6, "Use UMPNetFunctionLibrary::GetPlayerPingMs instead.")
 *   void OldFunction();
 */
#define MP_DEPRECATED_SINCE(Version, Msg) UE_DEPRECATED(Version, Msg)

/**
 * Compile-time check for minimum engine version.
 *
 * Example:
 *   #if MP_ENGINE_VERSION_AT_LEAST(5, 4)
 *       // use UE 5.4+ Iris networking feature
 *   #endif
 */
#define MP_ENGINE_VERSION_AT_LEAST(Major, Minor) \
    ((ENGINE_MAJOR_VERSION > (Major)) || (ENGINE_MAJOR_VERSION == (Major) && ENGINE_MINOR_VERSION >= (Minor)))

/**
 * Validates authority and consumes an RPC rate limit token at the top of a Server RPC.
 *
 * Example:
 *   void AWeapon::ServerFire_Implementation()
 *   {
 *       MP_SERVER_RPC_GUARD(FireRateLimiter);
 *       // process authoritative fire
 *   }
 */
#define MP_SERVER_RPC_GUARD(RateLimiterRef) \
    do { \
        if (!HasAuthority()) { \
            UE_LOG(LogMPUtility, Warning, TEXT("[MP_SERVER_RPC_GUARD] Server RPC rejected: lack authority in '%s'!"), *FString(__FUNCTION__)); \
            return; \
        } \
        const auto* _CVarRateLimit_ = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("mp.RateLimit.Enabled")); \
        const bool _bRateLimitActive_ = !_CVarRateLimit_ || _CVarRateLimit_->GetValueOnAnyThread() != 0; \
        if (_bRateLimitActive_ && !(RateLimiterRef).ConsumeToken(FPlatformTime::Seconds(), 1.0)) { \
            UE_LOG(LogMPUtility, Warning, TEXT("[MP_SERVER_RPC_GUARD] Server RPC dropped: rate limit exceeded in '%s'!"), *FString(__FUNCTION__)); \
            return; \
        } \
    } while (0)

#define MP_SERVER_RPC_GUARD_RET(RateLimiterRef, RetVal) \
    do { \
        if (!HasAuthority()) { \
            UE_LOG(LogMPUtility, Warning, TEXT("[MP_SERVER_RPC_GUARD] Server RPC rejected: lack authority in '%s'!"), *FString(__FUNCTION__)); \
            return (RetVal); \
        } \
        const auto* _CVarRateLimit_ = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("mp.RateLimit.Enabled")); \
        const bool _bRateLimitActive_ = !_CVarRateLimit_ || _CVarRateLimit_->GetValueOnAnyThread() != 0; \
        if (_bRateLimitActive_ && !(RateLimiterRef).ConsumeToken(FPlatformTime::Seconds(), 1.0)) { \
            UE_LOG(LogMPUtility, Warning, TEXT("[MP_SERVER_RPC_GUARD] Server RPC dropped: rate limit exceeded in '%s'!"), *FString(__FUNCTION__)); \
            return (RetVal); \
        } \
    } while (0)

/**
 * Expression macro evaluating to ServerExpression on dedicated/listen server, or ClientExpression on clients/standalone.
 *
 * Example:
 *   float TickRate = MP_BRANCH_NETMODE(this, 60.0f, 30.0f);
 */
#define MP_BRANCH_NETMODE(WorldContext, ServerExpression, ClientExpression) \
    ([&]() { \
        const UObject* _MP_Ctx_ = (WorldContext); \
        const UWorld* _MP_W_ = _MP_Ctx_ ? _MP_Ctx_->GetWorld() : nullptr; \
        const bool _bIsServer_ = _MP_W_ && (_MP_W_->GetNetMode() == NM_DedicatedServer || _MP_W_->GetNetMode() == NM_ListenServer); \
        return _bIsServer_ ? (ServerExpression) : (ClientExpression); \
    }())

/**
 * Branch-prediction optimized return if condition is unexpectedly met.
 *
 * Example:
 *   void ACharacter::TickSmoothing(float DeltaTime)
 *   {
 *       MP_UNLIKELY_RETURN_IF(DeltaTime <= 0.0f);
 *       // process smoothing
 *   }
 */
#define MP_UNLIKELY_RETURN_IF(Condition) \
    do { \
        if (UNLIKELY(Condition)) { \
            return; \
        } \
    } while (0)

#define MP_UNLIKELY_RETURN_IF_RET(Condition, RetVal) \
    do { \
        if (UNLIKELY(Condition)) { \
            return (RetVal); \
        } \
    } while (0)

// =============================================================================
// SECTION M: ADDITIONAL WORKFLOW MACROS
// =============================================================================

/**
 * Returns immediately from function if World context is null or invalid.
 *
 * Example:
 *   MP_RETURN_IF_NO_WORLD(this);
 */
#define MP_RETURN_IF_NO_WORLD(Obj) \
    do { \
        const UObject* _MP_Obj_ = (Obj); \
        if (!_MP_Obj_ || !_MP_Obj_->GetWorld()) { \
            return; \
        } \
    } while (0)

#define MP_RETURN_IF_NO_WORLD_RET(Obj, RetVal) \
    do { \
        const UObject* _MP_Obj_ = (Obj); \
        if (!_MP_Obj_ || !_MP_Obj_->GetWorld()) { \
            return (RetVal); \
        } \
    } while (0)

/**
 * Validates condition, logging warning and returning RetVal if condition is false.
 *
 * Example:
 *   MP_VALIDATE_RET(TargetActor != nullptr, false, "TargetActor is null");
 */
#define MP_VALIDATE_RET(Condition, RetVal, Reason) \
    do { \
        if (!(Condition)) { \
            UE_LOG(LogMPUtility, Warning, TEXT("[MP_VALIDATE] Check failed in '%s': %s"), *FString(__FUNCTION__), TEXT(Reason)); \
            return (RetVal); \
        } \
    } while (0)

// =============================================================================
// SECTION N: MULTIPLAYER GAMEPLAY CONVENIENCE MACROS
// =============================================================================

/**
 * Ensures calling actor is an Autonomous Proxy (locally owned client simulation).
 * Returns if actor is null or role is not ROLE_AutonomousProxy.
 */
#define MP_CHECK_AUTONOMOUS(Actor) \
    do { \
        if (!(Actor) || (Actor)->GetLocalRole() != ROLE_AutonomousProxy) { \
            return; \
        } \
    } while (0)

#define MP_CHECK_AUTONOMOUS_RET(Actor, ReturnValue) \
    do { \
        if (!(Actor) || (Actor)->GetLocalRole() != ROLE_AutonomousProxy) { \
            return (ReturnValue); \
        } \
    } while (0)

/**
 * Ensures calling actor is a Simulated Proxy (remote client proxy).
 */
#define MP_CHECK_SIMULATED(Actor) \
    do { \
        if (!(Actor) || (Actor)->GetLocalRole() != ROLE_SimulatedProxy) { \
            return; \
        } \
    } while (0)

#define MP_CHECK_SIMULATED_RET(Actor, ReturnValue) \
    do { \
        if (!(Actor) || (Actor)->GetLocalRole() != ROLE_SimulatedProxy) { \
            return (ReturnValue); \
        } \
    } while (0)

/**
 * Validates that an actor has not been torn off from network replication.
 */
#define MP_CHECK_NOT_TORN_OFF(Actor) \
    do { \
        if (!(Actor) || (Actor)->GetTearOff()) { \
            return; \
        } \
    } while (0)

#define MP_CHECK_NOT_TORN_OFF_RET(Actor, ReturnValue) \
    do { \
        if (!(Actor) || (Actor)->GetTearOff()) { \
            return (ReturnValue); \
        } \
    } while (0)

/**
 * Compares old and new values in an OnRep notification function and skips logic if unchanged.
 */
#define MP_REP_NOTIFY_CHANGED(OldValue, NewValue) \
    if ((OldValue) != (NewValue))



