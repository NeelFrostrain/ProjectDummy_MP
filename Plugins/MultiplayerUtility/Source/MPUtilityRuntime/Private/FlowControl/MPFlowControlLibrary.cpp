#include "FlowControl/MPFlowControlLibrary.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "TimerManager.h"

static TMap<FName, FTimerHandle> GPendingDelayedActions;

void UMPFlowControlLibrary::BranchByNetRole(const AActor* TargetActor, EMPNetRoleBranch& Branches)
{
    if (!TargetActor)
    {
        Branches = EMPNetRoleBranch::Simulated;
        return;
    }

    switch (TargetActor->GetLocalRole())
    {
    case ROLE_Authority:
        Branches = EMPNetRoleBranch::Authority;
        break;
    case ROLE_AutonomousProxy:
        Branches = EMPNetRoleBranch::Autonomous;
        break;
    default:
        Branches = EMPNetRoleBranch::Simulated;
        break;
    }
}

void UMPFlowControlLibrary::BranchByNetMode(const UObject* WorldContextObject, EMPNetModeBranch& Branches)
{
    const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!World)
    {
        Branches = EMPNetModeBranch::Standalone;
        return;
    }

    switch (World->GetNetMode())
    {
    case NM_DedicatedServer:
        Branches = EMPNetModeBranch::DedicatedServer;
        break;
    case NM_ListenServer:
        Branches = EMPNetModeBranch::ListenServer;
        break;
    case NM_Client:
        Branches = EMPNetModeBranch::Client;
        break;
    default:
        Branches = EMPNetModeBranch::Standalone;
        break;
    }
}

void UMPFlowControlLibrary::BranchIsLocallyControlled(const AActor* TargetActor, EMPLocalControlBranch& Branches)
{
    if (!TargetActor)
    {
        Branches = EMPLocalControlBranch::RemoteOrServer;
        return;
    }

    if (const APawn* Pawn = Cast<const APawn>(TargetActor))
    {
        Branches = Pawn->IsLocallyControlled() ? EMPLocalControlBranch::LocallyControlled : EMPLocalControlBranch::RemoteOrServer;
        return;
    }

    if (const APlayerController* PC = Cast<const APlayerController>(TargetActor))
    {
        Branches = PC->IsLocalPlayerController() ? EMPLocalControlBranch::LocallyControlled : EMPLocalControlBranch::RemoteOrServer;
        return;
    }

    // Fallback: check owner
    const AActor* Owner = TargetActor->GetOwner();
    const APawn* OwnerPawn = Cast<const APawn>(Owner);
    const APlayerController* OwnerPC = Cast<const APlayerController>(Owner);
    const bool bLocallyOwned = (OwnerPawn && OwnerPawn->IsLocallyControlled()) || (OwnerPC && OwnerPC->IsLocalPlayerController());
    Branches = bLocallyOwned ? EMPLocalControlBranch::LocallyControlled : EMPLocalControlBranch::RemoteOrServer;
}

void UMPFlowControlLibrary::BranchHasAuthority(const AActor* TargetActor, EMPAuthorityBranch& Branches)
{
    if (TargetActor && TargetActor->HasAuthority())
    {
        Branches = EMPAuthorityBranch::Authority;
    }
    else
    {
        Branches = EMPAuthorityBranch::Remote;
    }
}

void UMPFlowControlLibrary::NetworkGate(const UObject* WorldContextObject, bool bAllowServer, bool bAllowClient, bool bAllowStandalone, EMPGateExec& Branches)
{
    const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!World)
    {
        Branches = bAllowStandalone ? EMPGateExec::Open : EMPGateExec::Closed;
        return;
    }

    const ENetMode Mode = World->GetNetMode();
    const bool bIsServer = (Mode == NM_DedicatedServer || Mode == NM_ListenServer);
    const bool bIsClient = (Mode == NM_Client);
    const bool bIsStandalone = (Mode == NM_Standalone);

    if ((bIsServer && bAllowServer) || (bIsClient && bAllowClient) || (bIsStandalone && bAllowStandalone))
    {
        Branches = EMPGateExec::Open;
    }
    else
    {
        Branches = EMPGateExec::Closed;
    }
}

EMPDelayedActionResult UMPFlowControlLibrary::RunOnNextFrame(const UObject* WorldContextObject, FName ActionKey, FMPTimerCallback Callback)
{
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!World)
    {
        return EMPDelayedActionResult::FailedInvalidWorld;
    }

    if (!ActionKey.IsNone())
    {
        if (FTimerHandle* Existing = GPendingDelayedActions.Find(ActionKey))
        {
            World->GetTimerManager().ClearTimer(*Existing);
            GPendingDelayedActions.Remove(ActionKey);
        }
    }

    FTimerHandle Handle = World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([ActionKey, Callback]() {
        if (!ActionKey.IsNone())
        {
            GPendingDelayedActions.Remove(ActionKey);
        }
        Callback.ExecuteIfBound();
    }));

    if (!ActionKey.IsNone())
    {
        GPendingDelayedActions.Add(ActionKey, Handle);
    }

    return EMPDelayedActionResult::Scheduled;
}

EMPDelayedActionResult UMPFlowControlLibrary::RunAfterServerSeconds(const UObject* WorldContextObject, FName ActionKey, float DelaySeconds, FMPTimerCallback Callback)
{
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!World)
    {
        return EMPDelayedActionResult::FailedInvalidWorld;
    }

    if (!ActionKey.IsNone())
    {
        if (FTimerHandle* Existing = GPendingDelayedActions.Find(ActionKey))
        {
            World->GetTimerManager().ClearTimer(*Existing);
            GPendingDelayedActions.Remove(ActionKey);
        }
    }

    const float ClampedDelay = FMath::Max(0.0001f, DelaySeconds);
    FTimerHandle Handle;
    World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda([ActionKey, Callback]() {
        if (!ActionKey.IsNone())
        {
            GPendingDelayedActions.Remove(ActionKey);
        }
        Callback.ExecuteIfBound();
    }), ClampedDelay, false);

    if (!ActionKey.IsNone())
    {
        GPendingDelayedActions.Add(ActionKey, Handle);
    }

    return EMPDelayedActionResult::Scheduled;
}

bool UMPFlowControlLibrary::CancelPendingByKey(const UObject* WorldContextObject, FName ActionKey)
{
    if (ActionKey.IsNone())
    {
        return false;
    }

    FTimerHandle* Existing = GPendingDelayedActions.Find(ActionKey);
    if (!Existing)
    {
        return false;
    }

    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (World)
    {
        World->GetTimerManager().ClearTimer(*Existing);
    }

    GPendingDelayedActions.Remove(ActionKey);
    return true;
}

static TMap<TWeakObjectPtr<APlayerController>, TSet<FName>> GPlayerExecutedActions;

EMPDelayedActionResult UMPFlowControlLibrary::DoOnServerOnlyAfterDelay(const UObject* WorldContextObject, FName ActionKey, float DelaySeconds, FMPTimerCallback Callback)
{
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!World)
    {
        return EMPDelayedActionResult::FailedInvalidWorld;
    }

    if (World->GetNetMode() == NM_Client)
    {
        return EMPDelayedActionResult::Cancelled;
    }

    return RunAfterServerSeconds(WorldContextObject, ActionKey, DelaySeconds, Callback);
}

bool UMPFlowControlLibrary::CallOncePerPlayer(APlayerController* PC, FName ActionKey, FMPTimerCallback Callback)
{
    if (!PC || ActionKey.IsNone())
    {
        return false;
    }

    TSet<FName>& ExecutedKeys = GPlayerExecutedActions.FindOrAdd(PC);
    if (ExecutedKeys.Contains(ActionKey))
    {
        return false;
    }

    ExecutedKeys.Add(ActionKey);
    Callback.ExecuteIfBound();
    return true;
}

