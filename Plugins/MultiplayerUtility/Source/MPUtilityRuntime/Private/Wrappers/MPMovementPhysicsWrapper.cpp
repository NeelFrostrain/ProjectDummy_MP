#include "Wrappers/MPMovementPhysicsWrapper.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/PrimitiveComponent.h"
#include "MPNetMacros.h"

void UMPMovementPhysicsWrapper::SetSimulatePhysicsServerOnly(UPrimitiveComponent* TargetComponent, bool bSimulate)
{
    if (!TargetComponent) return;

    const AActor* Owner = TargetComponent->GetOwner();
    if (!Owner || !Owner->HasAuthority())
    {
        MP_LOG_WARN_ONCE(TEXT("SetSimulatePhysicsServerOnly called on client without authority! Operation ignored to prevent physics desync."));
        return;
    }

    TargetComponent->SetSimulatePhysics(bSimulate);
}

void UMPMovementPhysicsWrapper::ApplyImpulseServerOnly(UPrimitiveComponent* TargetComponent, const FVector& Impulse, FName BoneName, bool bVelChange)
{
    if (!TargetComponent) return;

    const AActor* Owner = TargetComponent->GetOwner();
    if (!Owner || !Owner->HasAuthority())
    {
        MP_LOG_WARN_ONCE(TEXT("ApplyImpulseServerOnly called on client without authority! Operation ignored."));
        return;
    }

    TargetComponent->AddImpulse(Impulse, BoneName, bVelChange);
}

FVector UMPMovementPhysicsWrapper::GetReplicatedVelocitySafe(const AActor* TargetActor)
{
    if (!TargetActor) return FVector::ZeroVector;

    const FVector Velocity = TargetActor->GetVelocity();
    if (!Velocity.IsNearlyZero())
    {
        return Velocity;
    }

    // Fallback: check primitive component physics linear velocity
    if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(const_cast<AActor*>(TargetActor)->GetRootComponent()))
    {
        if (PrimComp->IsSimulatingPhysics())
        {
            return PrimComp->GetPhysicsLinearVelocity();
        }
    }

    return FVector::ZeroVector;
}

bool UMPMovementPhysicsWrapper::IsPhysicsReplicated(const AActor* TargetActor)
{
    if (!TargetActor || !TargetActor->GetIsReplicated())
    {
        return false;
    }

    const UPrimitiveComponent* RootPrim = Cast<const UPrimitiveComponent>(TargetActor->GetRootComponent());
    return RootPrim && RootPrim->IsSimulatingPhysics();
}

FTransform UMPMovementPhysicsWrapper::SnapOrSmoothTransform(const FTransform& CurrentTransform, const FTransform& TargetTransform, float SnapDistanceThreshold, float LerpAlpha)
{
    const float Dist = FVector::Dist(CurrentTransform.GetLocation(), TargetTransform.GetLocation());
    if (Dist >= FMath::Max(1.0f, SnapDistanceThreshold))
    {
        // Positional difference exceeds tolerance; snap immediately
        return TargetTransform;
    }

    const float Alpha = FMath::Clamp(LerpAlpha, 0.01f, 1.0f);
    const FVector SmoothedLoc = FMath::Lerp(CurrentTransform.GetLocation(), TargetTransform.GetLocation(), Alpha);
    const FQuat SmoothedRot = FQuat::Slerp(CurrentTransform.GetRotation(), TargetTransform.GetRotation(), Alpha);
    const FVector SmoothedScale = FMath::Lerp(CurrentTransform.GetScale3D(), TargetTransform.GetScale3D(), Alpha);

    return FTransform(SmoothedRot, SmoothedLoc, SmoothedScale);
}

float UMPMovementPhysicsWrapper::GetMovementErrorDistance(const FVector& ClientPosition, const FVector& ServerPosition)
{
    return FVector::Dist(ClientPosition, ServerPosition);
}

bool UMPMovementPhysicsWrapper::IsCharacterMovingAuthoritatively(const ACharacter* TargetCharacter)
{
    if (!TargetCharacter || !TargetCharacter->HasAuthority())
    {
        return false;
    }

    return TargetCharacter->GetVelocity().SizeSquared() > 1.0f;
}

ENetworkSmoothingMode UMPMovementPhysicsWrapper::GetNetworkSmoothingMode(const ACharacter* TargetCharacter)
{
    if (!TargetCharacter) return ENetworkSmoothingMode::Disabled;

    if (const UCharacterMovementComponent* MoveComp = TargetCharacter->GetCharacterMovement())
    {
        return MoveComp->NetworkSmoothingMode;
    }

    return ENetworkSmoothingMode::Disabled;
}

void UMPMovementPhysicsWrapper::SetNetworkSmoothingModeSafe(ACharacter* TargetCharacter, ENetworkSmoothingMode NewMode)
{
    if (!TargetCharacter) return;

    if (UCharacterMovementComponent* MoveComp = TargetCharacter->GetCharacterMovement())
    {
        MoveComp->NetworkSmoothingMode = NewMode;
    }
}
