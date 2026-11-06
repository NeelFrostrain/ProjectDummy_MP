// Copyright Epic Games, Inc. All Rights Reserved.

#include "Physics/MPPhysicsReplicationHelper.h"
#include "GameFramework/Actor.h"

bool UMPPhysicsReplicationHelper::InitNetworkRagdollSafe(
	USkeletalMeshComponent* MeshComponent,
	FName RagdollCollisionProfile,
	float LinearDamping,
	float AngularDamping)
{
	if (!MeshComponent)
	{
		return false;
	}

	AActor* OwnerActor = MeshComponent->GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return false;
	}

	MeshComponent->SetCollisionProfileName(RagdollCollisionProfile);
	MeshComponent->SetSimulatePhysics(true);
	MeshComponent->SetEnableGravity(true);
	MeshComponent->SetLinearDamping(LinearDamping);
	MeshComponent->SetAngularDamping(AngularDamping);
	MeshComponent->WakeAllRigidBodies();

	// Ensure actor replicates movement for ragdoll root sync
	OwnerActor->SetReplicatingMovement(true);

	return true;
}

bool UMPPhysicsReplicationHelper::PutPhysicsToSleepServerOnly(UPrimitiveComponent* PrimitiveComponent)
{
	if (!PrimitiveComponent)
	{
		return false;
	}

	AActor* Owner = PrimitiveComponent->GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return false;
	}

	PrimitiveComponent->PutAllRigidBodiesToSleep();
	return true;
}

bool UMPPhysicsReplicationHelper::WakePhysicsServerOnly(UPrimitiveComponent* PrimitiveComponent, const FVector& InitialImpulse)
{
	if (!PrimitiveComponent)
	{
		return false;
	}

	AActor* Owner = PrimitiveComponent->GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return false;
	}

	PrimitiveComponent->WakeAllRigidBodies();

	if (!InitialImpulse.IsNearlyZero())
	{
		PrimitiveComponent->AddImpulse(InitialImpulse);
	}

	return true;
}

void UMPPhysicsReplicationHelper::SetNetworkPhysicsDampingSafe(
	UPrimitiveComponent* PrimitiveComponent,
	float LinearDamping,
	float AngularDamping)
{
	if (!PrimitiveComponent)
	{
		return;
	}

	PrimitiveComponent->SetLinearDamping(FMath::Max(0.0f, LinearDamping));
	PrimitiveComponent->SetAngularDamping(FMath::Max(0.0f, AngularDamping));
}

bool UMPPhysicsReplicationHelper::ValidatePhysicsTransformError(
	const UPrimitiveComponent* PrimitiveComponent,
	const FTransform& ProposedTransform,
	float MaxAllowedLocationError,
	float MaxAllowedRotationAngleDegrees,
	float& OutLocationError,
	float& OutAngleError)
{
	OutLocationError = 0.0f;
	OutAngleError = 0.0f;

	if (!PrimitiveComponent)
	{
		return false;
	}

	const FTransform CurrentTransform = PrimitiveComponent->GetComponentTransform();

	OutLocationError = FVector::Dist(CurrentTransform.GetLocation(), ProposedTransform.GetLocation());
	OutAngleError = FMath::RadiansToDegrees(CurrentTransform.GetRotation().AngularDistance(ProposedTransform.GetRotation()));

	return (OutLocationError <= MaxAllowedLocationError) && (OutAngleError <= MaxAllowedRotationAngleDegrees);
}

