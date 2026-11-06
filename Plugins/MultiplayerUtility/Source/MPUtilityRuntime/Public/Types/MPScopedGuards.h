#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "HAL/IConsoleManager.h"
#include "MPUtilityRuntimeModule.h"

// =============================================================================
// SCOPED GUARDS & RAII HELPERS (BATCH 8)
// =============================================================================

/**
 * RAII guard that validates network authority on scope entry and logs if called without authority.
 *
 * Example:
 *   void AWeapon::FireServerRPC()
 *   {
 *       FMPScopedAuthorityCheck AuthGuard(this, TEXT("AWeapon::FireServerRPC"));
 *       if (!AuthGuard.HasAuthority()) return;
 *       // authoritative logic
 *   }
 */
struct MPUTILITYRUNTIME_API FMPScopedAuthorityCheck
{
private:
    bool bHasAuthority = false;

public:
    explicit FMPScopedAuthorityCheck(const UObject* ContextObject, const TCHAR* ScopeName = TEXT("UnknownScope"))
    {
        if (const AActor* Actor = Cast<AActor>(ContextObject))
        {
            bHasAuthority = Actor->HasAuthority();
        }
        else if (const UActorComponent* Comp = Cast<UActorComponent>(ContextObject))
        {
            bHasAuthority = Comp->GetOwner() ? Comp->GetOwner()->HasAuthority() : false;
        }
        else if (const UWorld* World = ContextObject ? ContextObject->GetWorld() : nullptr)
        {
            bHasAuthority = World->GetNetMode() != NM_Client;
        }

        if (!bHasAuthority)
        {
            UE_LOG(LogMPUtility, Warning, TEXT("[FMPScopedAuthorityCheck] '%s' executed on client without authority!"),
                ScopeName ? ScopeName : TEXT("UnknownScope"));
        }
    }

    FORCEINLINE bool HasAuthority() const { return bHasAuthority; }
    FORCEINLINE explicit operator bool() const { return bHasAuthority; }
};

/**
 * RAII timer that tracks the elapsed wall-clock execution time of a C++ scope,
 * logging a warning if the duration exceeds a specified threshold in milliseconds.
 *
 * Example:
 *   {
 *       FMPScopedNetTimer ScopeTimer(TEXT("ReplicationQueryBatch"), 2.0f);
 *       // performance-critical network processing
 *   }
 */
struct MPUTILITYRUNTIME_API FMPScopedNetTimer
{
private:
    const TCHAR* ScopeName;
    double StartTimeSeconds;
    float WarnThresholdMs;

public:
    explicit FMPScopedNetTimer(const TCHAR* InScopeName, float InWarnThresholdMs = 5.0f)
        : ScopeName(InScopeName)
        , StartTimeSeconds(FPlatformTime::Seconds())
        , WarnThresholdMs(InWarnThresholdMs)
    {
    }

    ~FMPScopedNetTimer()
    {
        const double ElapsedMs = (FPlatformTime::Seconds() - StartTimeSeconds) * 1000.0;
        if (ElapsedMs >= WarnThresholdMs)
        {
            UE_LOG(LogMPUtility, Warning, TEXT("[FMPScopedNetTimer] '%s' took %.2f ms (threshold: %.2f ms)"),
                ScopeName ? ScopeName : TEXT("Unknown"), ElapsedMs, WarnThresholdMs);
        }
        else
        {
            UE_LOG(LogMPUtility, VeryVerbose, TEXT("[FMPScopedNetTimer] '%s' took %.3f ms"),
                ScopeName ? ScopeName : TEXT("Unknown"), ElapsedMs);
        }
    }
};

/**
 * Non-shipping development guard tracking active in-flight or re-entrant RPC execution depth.
 *
 * Example:
 *   void ACharacter::ServerInteract_Implementation()
 *   {
 *       FMPScopedRPCCounter RPCCounter(TEXT("ServerInteract"));
 *       if (RPCCounter.GetDepth() > 1) { return; } // prevent re-entrancy
 *   }
 */
struct MPUTILITYRUNTIME_API FMPScopedRPCCounter
{
private:
#if !UE_BUILD_SHIPPING
    FName RPCName;
    int32 ActiveDepth = 0;
    static TMap<FName, int32>& GetActiveCounters()
    {
        static TMap<FName, int32> Counters;
        return Counters;
    }
#endif

public:
    explicit FMPScopedRPCCounter(const TCHAR* InRPCName)
    {
#if !UE_BUILD_SHIPPING
        RPCName = FName(InRPCName);
        int32& Count = GetActiveCounters().FindOrAdd(RPCName);
        Count++;
        ActiveDepth = Count;
#endif
    }

    ~FMPScopedRPCCounter()
    {
#if !UE_BUILD_SHIPPING
        if (int32* Count = GetActiveCounters().Find(RPCName))
        {
            (*Count)--;
            if (*Count <= 0)
            {
                GetActiveCounters().Remove(RPCName);
            }
        }
#endif
    }

    FORCEINLINE int32 GetDepth() const
    {
#if !UE_BUILD_SHIPPING
        return ActiveDepth;
#else
        return 0;
#endif
    }
};

/**
 * RAII guard that wakes a dormant network actor upon scope entry and automatically
 * restores its prior dormancy setting upon scope exit.
 *
 * Example:
 *   {
 *       FMPScopedDormancyWake WakeGuard(LootChestActor);
 *       LootChestActor->UpdateInventory(NewItems);
 *   } // Restores LootChestActor dormancy (e.g. DORM_DormantAll) automatically on exit.
 */
struct MPUTILITYRUNTIME_API FMPScopedDormancyWake
{
private:
    TWeakObjectPtr<AActor> ActorPtr;
    ENetDormancy PreviousDormancy = DORM_Awake;
    bool bWasDormant = false;

public:
    explicit FMPScopedDormancyWake(AActor* InActor)
        : ActorPtr(InActor)
    {
        if (InActor && InActor->HasAuthority())
        {
            PreviousDormancy = InActor->NetDormancy;
            if (PreviousDormancy > DORM_Awake)
            {
                InActor->SetNetDormancy(DORM_Awake);
                InActor->FlushNetDormancy();
                bWasDormant = true;
            }
        }
    }

    ~FMPScopedDormancyWake()
    {
        if (bWasDormant && ActorPtr.IsValid())
        {
            ActorPtr->SetNetDormancy(PreviousDormancy);
        }
    }

    FORCEINLINE bool WasDormant() const { return bWasDormant; }
    FORCEINLINE ENetDormancy GetPreviousDormancy() const { return PreviousDormancy; }
};

/**
 * RAII guard that temporarily overrides an engine or plugin console variable,
 * restoring the prior value when exiting the scope.
 *
 * Example:
 *   {
 *       FMPScopedCVarOverride OverrideGuard(TEXT("mp.RateLimit.Enabled"), 0);
 *       // execute unthrottled operations or tests
 *   } // Original CVar setting restored automatically.
 */
struct MPUTILITYRUNTIME_API FMPScopedCVarOverride
{
private:
    IConsoleVariable* CVar = nullptr;
    FString PreviousValue;
    bool bActive = false;

public:
    explicit FMPScopedCVarOverride(const TCHAR* CVarName, int32 TemporaryValue)
    {
        if (CVarName)
        {
            CVar = IConsoleManager::Get().FindConsoleVariable(CVarName);
            if (CVar)
            {
                PreviousValue = CVar->GetString();
                CVar->Set(TemporaryValue, ECVF_SetByCode);
                bActive = true;
            }
        }
    }

    explicit FMPScopedCVarOverride(const TCHAR* CVarName, float TemporaryValue)
    {
        if (CVarName)
        {
            CVar = IConsoleManager::Get().FindConsoleVariable(CVarName);
            if (CVar)
            {
                PreviousValue = CVar->GetString();
                CVar->Set(TemporaryValue, ECVF_SetByCode);
                bActive = true;
            }
        }
    }

    explicit FMPScopedCVarOverride(const TCHAR* CVarName, const TCHAR* TemporaryValue)
    {
        if (CVarName && TemporaryValue)
        {
            CVar = IConsoleManager::Get().FindConsoleVariable(CVarName);
            if (CVar)
            {
                PreviousValue = CVar->GetString();
                CVar->Set(TemporaryValue, ECVF_SetByCode);
                bActive = true;
            }
        }
    }

    ~FMPScopedCVarOverride()
    {
        if (bActive && CVar)
        {
            CVar->Set(*PreviousValue, ECVF_SetByCode);
        }
    }

    FORCEINLINE bool IsActive() const { return bActive; }
    FORCEINLINE const FString& GetPreviousValue() const { return PreviousValue; }
};

