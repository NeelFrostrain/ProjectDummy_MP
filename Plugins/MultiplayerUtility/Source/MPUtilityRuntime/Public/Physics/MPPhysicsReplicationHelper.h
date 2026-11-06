// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "MPPhysicsReplicationHelper.generated.h"

/**
 * Reusable server-authoritative physics synchronization, ragdoll networking, and damping helpers.
 */
UCLASS()
class MPUTILITYRUNTIME_API UMPPhysicsReplicationHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Safely transitions a Skeletal Mesh into authoritative networked ragdoll physics.
	 * Configures collision profiles, enables physics simulation, and sets damping to prevent network jitter.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Networking|Physics")
	static bool InitNetworkRagdollSafe(
		USkeletalMeshComponent* MeshComponent,
		FName RagdollCollisionProfile = TEXT("Ragdoll"),
		float LinearDamping = 0.8f,
		float AngularDamping = 1.0f
	);

	/**
	 * Safely puts a simulated physics component to sleep on the server and broadcasts dormancy update.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Networking|Physics")
	static bool PutPhysicsToSleepServerOnly(UPrimitiveComponent* PrimitiveComponent);

	/**
	 * Safely wakes up a simulated physics component on the server with optional initial impulse.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Networking|Physics")
	static bool WakePhysicsServerOnly(UPrimitiveComponent* PrimitiveComponent, const FVector& InitialImpulse = FVector::ZeroVector);

	/**
	 * Configures network damping on a simulated primitive to stabilize remote client proxy interpolation.
	 */
	UFUNCTION(BlueprintCallable, Category = "Networking|Physics")
	static void SetNetworkPhysicsDampingSafe(UPrimitiveComponent* PrimitiveComponent, float LinearDamping, float AngularDamping);

	/**
	 * Validates whether a proposed physics transform is within acceptable error margins of current server physics.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Networking|Physics")
	static bool ValidatePhysicsTransformError(
		const UPrimitiveComponent* PrimitiveComponent,
		const FTransform& ProposedTransform,
		float MaxAllowedLocationError,
		float MaxAllowedRotationAngleDegrees,
		float& OutLocationError,
		float& OutAngleError
	);
};

