#include "Wrappers/MPActorNetworkWrapper.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/NetConnection.h"
#include "Misc/EngineVersionComparison.h"
#include "MPNetMacros.h"
#include "Types/MPNetTypes.h"

ENetRole UMPActorNetworkWrapper::GetLocalRole(const AActor* TargetActor)
{
    return TargetActor ? TargetActor->GetLocalRole() : ROLE_None;
}

ENetRole UMPActorNetworkWrapper::GetRemoteRole(const AActor* TargetActor)
{
    return TargetActor ? TargetActor->GetRemoteRole() : ROLE_None;
}

void UMPActorNetworkWrapper::SetReplicates(AActor* TargetActor, bool bReplicates)
{
    MP_CHECK_AUTHORITY(TargetActor);
    TargetActor->SetReplicates(bReplicates);
}

void UMPActorNetworkWrapper::SetReplicateMovement(AActor* TargetActor, bool bReplicateMovement)
{
    MP_CHECK_AUTHORITY(TargetActor);
    TargetActor->SetReplicateMovement(bReplicateMovement);
}

void UMPActorNetworkWrapper::SetOwner(AActor* TargetActor, AActor* NewOwner)
{
    MP_CHECK_AUTHORITY(TargetActor);
    TargetActor->SetOwner(NewOwner);
}

AActor* UMPActorNetworkWrapper::GetOwner(const AActor* TargetActor)
{
    return TargetActor ? TargetActor->GetOwner() : nullptr;
}

AActor* UMPActorNetworkWrapper::GetTopLevelNetOwner(const AActor* TargetActor)
{
    if (!TargetActor) return nullptr;

    const AActor* Current = TargetActor;
    while (Current)
    {
        if (Current->GetNetConnection() != nullptr)
        {
            return const_cast<AActor*>(Current);
        }
        Current = Current->GetOwner();
    }
    return nullptr;
}

bool UMPActorNetworkWrapper::HasNetConnection(const AActor* TargetActor)
{
    return TargetActor ? (TargetActor->GetNetConnection() != nullptr) : false;
}

void UMPActorNetworkWrapper::SetNetDormancy(AActor* TargetActor, ENetDormancy NewDormancy)
{
    MP_CHECK_AUTHORITY(TargetActor);
    TargetActor->SetNetDormancy(NewDormancy);
}

void UMPActorNetworkWrapper::FlushNetDormancy(AActor* TargetActor)
{
    MP_CHECK_AUTHORITY(TargetActor);
    TargetActor->FlushNetDormancy();
}

void UMPActorNetworkWrapper::ForceNetUpdate(AActor* TargetActor)
{
    MP_CHECK_AUTHORITY(TargetActor);
    TargetActor->ForceNetUpdate();
}

void UMPActorNetworkWrapper::SetNetUpdateFrequency(AActor* TargetActor, float NewFrequency)
{
    MP_CHECK_AUTHORITY(TargetActor);
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 5
    TargetActor->SetNetUpdateFrequency(FMath::Max(0.01f, NewFrequency));
#else
    TargetActor->NetUpdateFrequency = FMath::Max(0.01f, NewFrequency);
#endif
}

float UMPActorNetworkWrapper::GetNetUpdateFrequency(const AActor* TargetActor)
{
    if (!TargetActor) return 0.0f;
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 5
    return TargetActor->GetNetUpdateFrequency();
#else
    return TargetActor->NetUpdateFrequency;
#endif
}

void UMPActorNetworkWrapper::SetMinNetUpdateFrequency(AActor* TargetActor, float NewMinFrequency)
{
    MP_CHECK_AUTHORITY(TargetActor);
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 5
    TargetActor->SetMinNetUpdateFrequency(FMath::Clamp(NewMinFrequency, 0.01f, TargetActor->GetNetUpdateFrequency()));
#else
    TargetActor->MinNetUpdateFrequency = FMath::Clamp(NewMinFrequency, 0.01f, TargetActor->NetUpdateFrequency);
#endif
}

float UMPActorNetworkWrapper::GetMinNetUpdateFrequency(const AActor* TargetActor)
{
    if (!TargetActor) return 0.0f;
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 5
    return TargetActor->GetMinNetUpdateFrequency();
#else
    return TargetActor->MinNetUpdateFrequency;
#endif
}

void UMPActorNetworkWrapper::SetNetPriority(AActor* TargetActor, float NewPriority)
{
    MP_CHECK_AUTHORITY(TargetActor);
    TargetActor->NetPriority = FMath::Max(0.01f, NewPriority);
}

float UMPActorNetworkWrapper::GetNetPriority(const AActor* TargetActor)
{
    return TargetActor ? TargetActor->NetPriority : 0.0f;
}

void UMPActorNetworkWrapper::SetNetCullDistance(AActor* TargetActor, float Distance)
{
    MP_CHECK_AUTHORITY(TargetActor);
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 5
    TargetActor->SetNetCullDistanceSquared(FMath::Square(FMath::Max(0.0f, Distance)));
#else
    TargetActor->NetCullDistanceSquared = FMath::Square(FMath::Max(0.0f, Distance));
#endif
}

float UMPActorNetworkWrapper::GetNetCullDistance(const AActor* TargetActor)
{
    if (!TargetActor) return 0.0f;
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 5
    return FMath::Sqrt(TargetActor->GetNetCullDistanceSquared());
#else
    return FMath::Sqrt(TargetActor->NetCullDistanceSquared);
#endif
}

void UMPActorNetworkWrapper::SetAlwaysRelevant(AActor* TargetActor, bool bAlwaysRelevant)
{
    MP_CHECK_AUTHORITY(TargetActor);
    TargetActor->bAlwaysRelevant = bAlwaysRelevant;
}

void UMPActorNetworkWrapper::SetOnlyRelevantToOwner(AActor* TargetActor, bool bOnlyRelevantToOwner)
{
    MP_CHECK_AUTHORITY(TargetActor);
    TargetActor->bOnlyRelevantToOwner = bOnlyRelevantToOwner;
}

void UMPActorNetworkWrapper::SetNetUseOwnerRelevancy(AActor* TargetActor, bool bNetUseOwnerRelevancy)
{
    MP_CHECK_AUTHORITY(TargetActor);
    TargetActor->bNetUseOwnerRelevancy = bNetUseOwnerRelevancy;
}

bool UMPActorNetworkWrapper::IsNetStartupActor(const AActor* TargetActor)
{
    return TargetActor ? TargetActor->IsNetStartupActor() : false;
}

APlayerState* UMPActorNetworkWrapper::GetLogicalOwnerSafe(const AActor* TargetActor)
{
    if (!TargetActor)
    {
        return nullptr;
    }

    if (TargetActor->GetClass()->ImplementsInterface(UMPOwnershipInterface::StaticClass()))
    {
        if (APlayerState* CustomPS = IMPOwnershipInterface::Execute_GetLogicalOwnerPlayerState(TargetActor))
        {
            return CustomPS;
        }
    }

    const AActor* Curr = TargetActor;
    TSet<const AActor*> Visited;
    while (Curr && !Visited.Contains(Curr))
    {
        Visited.Add(Curr);

        if (const APlayerController* PC = Cast<APlayerController>(Curr))
        {
            return PC->PlayerState;
        }
        if (const APawn* Pawn = Cast<APawn>(Curr))
        {
            if (Pawn->GetPlayerState())
            {
                return Pawn->GetPlayerState();
            }
        }
        if (const APlayerState* PS = Cast<APlayerState>(Curr))
        {
            return const_cast<APlayerState*>(PS);
        }

        Curr = Curr->GetOwner();
    }

    return nullptr;
}
