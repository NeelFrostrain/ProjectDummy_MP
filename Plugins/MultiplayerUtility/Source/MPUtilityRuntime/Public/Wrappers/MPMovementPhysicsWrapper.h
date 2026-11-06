#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MPMovementPhysicsWrapper.generated.h"

class AActor;
class ACharacter;
class UPrimitiveComponent;

/**
 * ============================================================================
 * UNREAL ENGINE CHARACTER MOVEMENT & PHYSICS REPLICATION ARCHITECTURE NOTES:
 * ============================================================================
 * 1. CLIENT PREDICTION & FSavedMove:
 *    UCharacterMovementComponent uses client-side prediction and server replay.
 *    Clients autonomously simulate movement, store unacknowledged inputs in FSavedMove_Character,
 *    and send timestamped moves to the server via ServerMove().
 *    If custom variables (e.g., sprint modes, jetpack fuel, custom movement modes) affect
 *    velocity or acceleration, they MUST be serialized inside FSavedMove_Character and
 *    replicated via compressed movement flags; otherwise, server desyncs and correction snaps will occur.
 *
 * 2. PHYSICS SIMULATION ON CHARACTERS:
 *    Enabling SetSimulatePhysics on a character disables UCharacterMovementComponent prediction.
 *    Physics replication runs through rigid-body replication (FRepMovement), which is lossy
 *    and lacks rewind/replay capability.
 * ============================================================================
 */
UCLASS()
class MPUTILITYRUNTIME_API UMPMovementPhysicsWrapper : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Enables or disables physics simulation on a primitive component.
     * Enforces Server Authority; calling on client logs a warning to prevent desyncs.
     * Runs on: Server.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Multiplayer Utility|Physics", meta=(DisplayName="Set Simulate Physics Server Only", Keywords="set simulate physics server authority rigid body"))
    static void SetSimulatePhysicsServerOnly(UPrimitiveComponent* TargetComponent, bool bSimulate);

    /**
     * Applies a linear impulse to a physics component on the server.
     * Enforces Server Authority so all replicating clients observe the physics change.
     * Runs on: Server.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Multiplayer Utility|Physics", meta=(DefaultToSelf="TargetComponent", DisplayName="Apply Impulse Server Only", Keywords="apply impulse force physics server authority"))
    static void ApplyImpulseServerOnly(UPrimitiveComponent* TargetComponent, const FVector& Impulse, FName BoneName = NAME_None, bool bVelChange = false);

    /**
     * Safely retrieves the replicated velocity of an actor.
     * Queries movement component, physics linear velocity, or ReplicatedMovement data with null-safety.
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Movement", meta=(DefaultToSelf="TargetActor", DisplayName="Get Replicated Velocity Safe", Keywords="get replicated velocity safe speed vector"))
    static FVector GetReplicatedVelocitySafe(const AActor* TargetActor);

    /**
     * Returns true if the actor has movement replication enabled and its root component is actively simulating physics.
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Physics", meta=(DefaultToSelf="TargetActor", DisplayName="Is Physics Replicated", Keywords="is physics replicated simulate movement"))
    static bool IsPhysicsReplicated(const AActor* TargetActor);

    /**
     * Correction helper: if distance between Current and Target exceeds SnapDistanceThreshold,
     * immediately snaps to TargetTransform to avoid rubberbanding; otherwise smoothly interpolates.
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Movement", meta=(DisplayName="Snap Or Smooth Transform", Keywords="snap smooth transform interpolate error correction threshold"))
    static FTransform SnapOrSmoothTransform(const FTransform& CurrentTransform, const FTransform& TargetTransform, float SnapDistanceThreshold = 150.0f, float LerpAlpha = 0.25f);

    /**
     * Calculates the positional error distance in Unreal units between client predicted position and server authority position.
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Movement", meta=(DisplayName="Get Movement Error Distance", Keywords="movement error distance delta client server difference"))
    static float GetMovementErrorDistance(const FVector& ClientPosition, const FVector& ServerPosition);

    /**
     * Returns true if the character has server authority and is actively moving (Velocity > 1.0 unit/sec).
     * Runs on: Server (Client returns false).
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Movement", meta=(DisplayName="Is Character Moving Authoritatively", Keywords="is character moving authority server moving velocity"))
    static bool IsCharacterMovingAuthoritatively(const ACharacter* TargetCharacter);

    /**
     * Safely retrieves the NetworkSmoothingMode configured on the CharacterMovementComponent.
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintPure, Category="Multiplayer Utility|Movement", meta=(DisplayName="Get Network Smoothing Mode", Keywords="get network smoothing mode character movement"))
    static ENetworkSmoothingMode GetNetworkSmoothingMode(const ACharacter* TargetCharacter);

    /**
     * Configures the NetworkSmoothingMode on the CharacterMovementComponent.
     * Safe on both server and client (clients use this to toggle visual prediction smoothing).
     * Runs on: Client & Server.
     */
    UFUNCTION(BlueprintCallable, Category="Multiplayer Utility|Movement", meta=(DisplayName="Set Network Smoothing Mode Safe", Keywords="set network smoothing mode safe character movement linear exponential"))
    static void SetNetworkSmoothingModeSafe(ACharacter* TargetCharacter, ENetworkSmoothingMode NewMode);
};

