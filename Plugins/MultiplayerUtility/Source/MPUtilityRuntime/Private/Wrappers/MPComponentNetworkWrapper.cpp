#include "Wrappers/MPComponentNetworkWrapper.h"
#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "MPNetMacros.h"
#include "Misc/EngineVersionComparison.h"

void UMPComponentNetworkWrapper::SetComponentReplicated(UActorComponent* TargetComponent, bool bReplicates)
{
    if (!TargetComponent) return;

    const AActor* Owner = TargetComponent->GetOwner();
    if (!Owner || !Owner->HasAuthority())
    {
        MP_LOG_WARN_ONCE(TEXT("SetComponentReplicated called on client without authority!"));
        return;
    }

    TargetComponent->SetIsReplicated(bReplicates);
}

bool UMPComponentNetworkWrapper::IsComponentReplicated(const UActorComponent* TargetComponent)
{
    return TargetComponent ? TargetComponent->GetIsReplicated() : false;
}

bool UMPComponentNetworkWrapper::IsComponentNetSimulating(const UActorComponent* TargetComponent)
{
    if (!TargetComponent) return false;
    const AActor* Owner = TargetComponent->GetOwner();
    if (!Owner) return false;

    return Owner->GetLocalRole() == ROLE_SimulatedProxy;
}

ENetRole UMPComponentNetworkWrapper::GetComponentOwnerLocalRole(const UActorComponent* TargetComponent)
{
    if (!TargetComponent) return ROLE_None;
    const AActor* Owner = TargetComponent->GetOwner();
    return Owner ? Owner->GetLocalRole() : ROLE_None;
}

ENetRole UMPComponentNetworkWrapper::GetComponentOwnerRemoteRole(const UActorComponent* TargetComponent)
{
    if (!TargetComponent) return ROLE_None;
    const AActor* Owner = TargetComponent->GetOwner();
    return Owner ? Owner->GetRemoteRole() : ROLE_None;
}

bool UMPComponentNetworkWrapper::HasComponentAuthority(const UActorComponent* TargetComponent)
{
    if (!TargetComponent) return false;
    const AActor* Owner = TargetComponent->GetOwner();
    return Owner ? Owner->HasAuthority() : false;
}

bool UMPComponentNetworkWrapper::RegisterReplicatedSubobjectSafe(UObject* Owner, UObject* SubObject, ELifetimeCondition NetCondition)
{
    if (!Owner || !SubObject)
    {
        return false;
    }

    bool bHasAuthority = false;
    if (const AActor* ActorOwner = Cast<AActor>(Owner))
    {
        bHasAuthority = ActorOwner->HasAuthority();
    }
    else if (const UActorComponent* CompOwner = Cast<UActorComponent>(Owner))
    {
        bHasAuthority = CompOwner->GetOwner() ? CompOwner->GetOwner()->HasAuthority() : false;
    }

    if (!bHasAuthority)
    {
        MP_LOG_WARN_ONCE(TEXT("RegisterReplicatedSubobjectSafe called on client without authority!"));
        return false;
    }

#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1) || ENGINE_MAJOR_VERSION > 5
    if (AActor* ActorOwner = Cast<AActor>(Owner))
    {
        ActorOwner->AddReplicatedSubObject(SubObject, NetCondition);
        return true;
    }
    else if (UActorComponent* CompOwner = Cast<UActorComponent>(Owner))
    {
        CompOwner->AddReplicatedSubObject(SubObject, NetCondition);
        return true;
    }
#else
    UE_LOG(LogTemp, Warning, TEXT("[RegisterReplicatedSubobjectSafe] Registered subobject list requires UE 5.1+."));
#endif

    return false;
}

bool UMPComponentNetworkWrapper::UnregisterReplicatedSubobjectSafe(UObject* Owner, UObject* SubObject)
{
    if (!Owner || !SubObject)
    {
        return false;
    }

    bool bHasAuthority = false;
    if (const AActor* ActorOwner = Cast<AActor>(Owner))
    {
        bHasAuthority = ActorOwner->HasAuthority();
    }
    else if (const UActorComponent* CompOwner = Cast<UActorComponent>(Owner))
    {
        bHasAuthority = CompOwner->GetOwner() ? CompOwner->GetOwner()->HasAuthority() : false;
    }

    if (!bHasAuthority)
    {
        MP_LOG_WARN_ONCE(TEXT("UnregisterReplicatedSubobjectSafe called on client without authority!"));
        return false;
    }

#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1) || ENGINE_MAJOR_VERSION > 5
    if (AActor* ActorOwner = Cast<AActor>(Owner))
    {
        ActorOwner->RemoveReplicatedSubObject(SubObject);
        return true;
    }
    else if (UActorComponent* CompOwner = Cast<UActorComponent>(Owner))
    {
        CompOwner->RemoveReplicatedSubObject(SubObject);
        return true;
    }
#else
    UE_LOG(LogTemp, Warning, TEXT("[UnregisterReplicatedSubobjectSafe] Registered subobject list requires UE 5.1+."));
#endif

    return false;
}

bool UMPComponentNetworkWrapper::IsSubobjectReplicated(const UObject* Owner, const UObject* SubObject)
{
    if (!Owner || !SubObject)
    {
        return false;
    }

#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1) || ENGINE_MAJOR_VERSION > 5
    if (const AActor* ActorOwner = Cast<AActor>(Owner))
    {
        return ActorOwner->IsReplicatedSubObjectRegistered(SubObject);
    }
    else if (const UActorComponent* CompOwner = Cast<UActorComponent>(Owner))
    {
        return CompOwner->IsReplicatedSubObjectRegistered(SubObject);
    }
#endif

    return false;
}

TArray<UActorComponent*> UMPComponentNetworkWrapper::GetReplicatedComponentsOfActor(const AActor* TargetActor)
{
    TArray<UActorComponent*> ReplicatedComps;
    if (!TargetActor)
    {
        return ReplicatedComps;
    }

    for (UActorComponent* Comp : TargetActor->GetComponents())
    {
        if (Comp && Comp->GetIsReplicated())
        {
            ReplicatedComps.Add(Comp);
        }
    }

    return ReplicatedComps;
}

void UMPComponentNetworkWrapper::SetComponentReplicatedRecursive(UActorComponent* TargetComponent, bool bReplicates)
{
    if (!TargetComponent)
    {
        return;
    }

    const AActor* Owner = TargetComponent->GetOwner();
    if (!Owner || !Owner->HasAuthority())
    {
        MP_LOG_WARN_ONCE(TEXT("SetComponentReplicatedRecursive called on client without authority!"));
        return;
    }

    TargetComponent->SetIsReplicated(bReplicates);

    if (USceneComponent* SceneComp = Cast<USceneComponent>(TargetComponent))
    {
        TArray<USceneComponent*> Children;
        SceneComp->GetChildrenComponents(true, Children);
        for (USceneComponent* Child : Children)
        {
            if (Child)
            {
                Child->SetIsReplicated(bReplicates);
            }
        }
    }
}

TArray<FString> UMPComponentNetworkWrapper::PrintReplicatedSubobjects(const AActor* TargetActor, bool bPrintToLog)
{
    TArray<FString> OutStrings;
    if (!TargetActor)
    {
        return OutStrings;
    }

    TArray<UActorComponent*> RepComps = GetReplicatedComponentsOfActor(TargetActor);
    const FString Header = FString::Printf(TEXT("[Replication Diagnostic] Actor '%s' has %d replicated components:"),
        *TargetActor->GetName(), RepComps.Num());
    OutStrings.Add(Header);

    if (bPrintToLog)
    {
        UE_LOG(LogTemp, Log, TEXT("%s"), *Header);
    }

    for (int32 Index = 0; Index < RepComps.Num(); ++Index)
    {
        const UActorComponent* Comp = RepComps[Index];
        const FString Line = FString::Printf(TEXT("  [%d] %s (%s)"),
            Index, *Comp->GetName(), *Comp->GetClass()->GetName());
        OutStrings.Add(Line);

        if (bPrintToLog)
        {
            UE_LOG(LogTemp, Log, TEXT("%s"), *Line);
        }
    }

    return OutStrings;
}

