#include "Components/MPNetDebugTextComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Types/MPNetTypes.h"

UMPNetDebugTextComponent::UMPNetDebugTextComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    bAutoActivate = true;
}

void UMPNetDebugTextComponent::BeginPlay()
{
    Super::BeginPlay();

#if !UE_BUILD_SHIPPING
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            TimerHandle_Refresh,
            this,
            &UMPNetDebugTextComponent::ForceRefreshDebugText,
            FMath::Max(0.1f, RefreshIntervalSeconds),
            true
        );
        ForceRefreshDebugText();
    }
#endif
}

void UMPNetDebugTextComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
#if !UE_BUILD_SHIPPING
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(TimerHandle_Refresh);
    }
#endif
    Super::EndPlay(EndPlayReason);
}

void UMPNetDebugTextComponent::ForceRefreshDebugText()
{
#if !UE_BUILD_SHIPPING
    const AActor* Owner = GetOwner();
    if (!Owner)
    {
        CachedDebugText = TEXT("Owner: None");
        return;
    }

    const UWorld* World = Owner->GetWorld();
    const ENetMode NetMode = World ? World->GetNetMode() : NM_Standalone;
    const ENetRole LocalRole = Owner->GetLocalRole();
    const ENetRole RemoteRole = Owner->GetRemoteRole();

    const TCHAR* NetModeStr = TEXT("Unknown");
    switch (NetMode)
    {
        case NM_Standalone:      NetModeStr = TEXT("Standalone"); break;
        case NM_DedicatedServer: NetModeStr = TEXT("DedicatedServer"); break;
        case NM_ListenServer:    NetModeStr = TEXT("ListenServer"); break;
        case NM_Client:          NetModeStr = TEXT("Client"); break;
        default: break;
    }

    const TCHAR* LocalRoleStr = TEXT("None");
    switch (LocalRole)
    {
        case ROLE_Authority:       LocalRoleStr = TEXT("Authority"); break;
        case ROLE_AutonomousProxy: LocalRoleStr = TEXT("Autonomous"); break;
        case ROLE_SimulatedProxy:  LocalRoleStr = TEXT("Simulated"); break;
        default: break;
    }

    const AActor* NetOwner = Owner->GetNetOwner();
    const FString OwnerName = NetOwner ? NetOwner->GetName() : TEXT("NoNetOwner");

    CachedDebugText = FString::Printf(TEXT("[%s] LocalRole: %s | NetOwner: %s | Dormant: %d"),
        NetModeStr, LocalRoleStr, *OwnerName, (int32)Owner->NetDormancy);

    if (Owner->Implements<UMPNetDebugInterface>())
    {
        TArray<FString> ExtraLines;
        IMPNetDebugInterface::Execute_GetNetDebugStrings(Owner, ExtraLines);
        for (const FString& Line : ExtraLines)
        {
            CachedDebugText += TEXT("\n") + Line;
        }
    }
#endif
}

