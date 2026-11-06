// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Runtime/Launch/Resources/Version.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MPCharacterMovementComponent.generated.h"

/**
 * Smoothing profile presets for network character proxy interpolation.
 */
UENUM(BlueprintType)
enum class EMPNetworkSmoothingProfile : uint8
{
	Default                   UMETA(DisplayName = "Default Exponential"),
	TightFastPaced            UMETA(DisplayName = "Tight / Competitive (Snappy)"),
	CinematicSmooth           UMETA(DisplayName = "Cinematic / Soft"),
	HighLatencyConservative   UMETA(DisplayName = "High Latency Conservative")
};

/**
 * Custom saved move storing compressed movement inputs for client prediction and server replay.
 */
class MPUTILITYRUNTIME_API FSavedMove_MPCharacter : public FSavedMove_Character
{
public:
	typedef FSavedMove_Character Super;

	uint8 bWantsToSprint : 1;
	uint8 bWantsToDash   : 1;
	uint8 bWantsToSlide  : 1;
	uint8 bIsClimbing    : 1;

	FSavedMove_MPCharacter();
	virtual ~FSavedMove_MPCharacter() override = default;

	virtual void Clear() override;
	virtual uint8 GetCompressedFlags() const override;
	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const override;
	virtual void SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData) override;
	virtual void PrepMoveFor(ACharacter* Character) override;
};

/**
 * Prediction data container for client-side character movement.
 */
class MPUTILITYRUNTIME_API FNetworkPredictionData_Client_MPCharacter : public FNetworkPredictionData_Client_Character
{
public:
	typedef FNetworkPredictionData_Client_Character Super;

	FNetworkPredictionData_Client_MPCharacter(const UCharacterMovementComponent& ClientMovement);
	virtual FSavedMovePtr AllocateNewMove() override;
};

/**
 * Production-ready networked character movement component with full FSavedMove prediction,
 * sprint, dash, slide, and climb synchronization, smoothing profiles, and server validation.
 */
UCLASS(ClassGroup = (Movement), meta = (BlueprintSpawnableComponent, DisplayName = "MP Character Movement Component"))
class MPUTILITYRUNTIME_API UMPCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UMPCharacterMovementComponent();

	virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual float GetMaxSpeed() const override;
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;

	// --------------------------------------------------------------------------
	// Custom Movement Capabilities & Multipliers
	// --------------------------------------------------------------------------

	/** Sprint speed multiplier applied to MaxWalkSpeed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Movement", meta = (ClampMin = "1.0", ClampMax = "4.0"))
	float SprintSpeedMultiplier = 1.6f;

	/** Slide speed multiplier applied when sliding. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Movement", meta = (ClampMin = "1.0", ClampMax = "3.0"))
	float SlideSpeedMultiplier = 1.3f;

	/** Climbing movement speed in world units per second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Movement", meta = (ClampMin = "50.0", ClampMax = "1000.0"))
	float ClimbSpeed = 250.0f;

	/** Dash impulse magnitude. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Movement", meta = (ClampMin = "100.0", ClampMax = "5000.0"))
	float DashImpulse = 1200.0f;

	// --------------------------------------------------------------------------
	// Blueprint State Controls (Called on Autonomous Proxy Client & Server)
	// --------------------------------------------------------------------------

	/** Activates or deactivates sprinting (automatically predicted and synchronized). */
	UFUNCTION(BlueprintCallable, Category = "Networking|Movement")
	void SetSprinting(bool bNewSprint);

	/** Activates a forward dash impulse (predicted and synchronized). */
	UFUNCTION(BlueprintCallable, Category = "Networking|Movement")
	void TriggerDash();

	/** Activates or deactivates sliding. */
	UFUNCTION(BlueprintCallable, Category = "Networking|Movement")
	void SetSliding(bool bNewSlide);

	/** Activates or deactivates climbing state. */
	UFUNCTION(BlueprintCallable, Category = "Networking|Movement")
	void SetClimbing(bool bNewClimb);

	UFUNCTION(BlueprintPure, Category = "Networking|Movement")
	bool IsSprinting() const { return bWantsToSprint; }

	UFUNCTION(BlueprintPure, Category = "Networking|Movement")
	bool IsSliding() const { return bWantsToSlide; }

	UFUNCTION(BlueprintPure, Category = "Networking|Movement")
	bool IsClimbing() const { return bIsClimbing; }

	// --------------------------------------------------------------------------
	// Network Smoothing Profiles
	// --------------------------------------------------------------------------

	/** Applies a predefined smoothing profile to simulated proxy interpolation. */
	UFUNCTION(BlueprintCallable, Category = "Networking|Movement")
	void ApplySmoothingProfile(EMPNetworkSmoothingProfile Profile);

	// --------------------------------------------------------------------------
	// Diagnostics & Server Validation
	// --------------------------------------------------------------------------

	/**
	 * Server-side helper to validate client position delta against expected max speed and delta time.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Networking|Movement")
	static bool ValidateMovementStep(
		const FVector& PreviousLocation,
		const FVector& ProposedLocation,
		float DeltaSeconds,
		float AllowedMaxSpeed,
		float ToleranceMargin,
		float& OutDistanceError
	);

	/** Number of client movement corrections received from server. */
	UFUNCTION(BlueprintPure, Category = "Networking|Movement")
	int32 GetClientCorrectionCount() const { return ClientCorrectionCount; }

protected:
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
	virtual void OnClientCorrectionReceived(class FNetworkPredictionData_Client_Character& ClientData, float TimeStamp, FVector NewLocation, FVector NewVelocity, UPrimitiveComponent* NewBase, FName NewBaseBoneName, bool bHasBase, bool bBaseRelativePosition, uint8 ServerMovementMode, FVector ServerGravityDirection) override;
#else
	virtual void OnClientCorrectionReceived(class FNetworkPredictionData_Client_Character& ClientData, float TimeStamp, FVector NewLocation, FVector NewVelocity, UPrimitiveComponent* NewBase, FName NewBaseBoneName, bool bHasBase, bool bBaseRelativePosition, uint8 ServerMovementMode) override;
#endif

	uint8 bWantsToSprint : 1;
	uint8 bWantsToDash   : 1;
	uint8 bWantsToSlide  : 1;
	uint8 bIsClimbing    : 1;

private:
	friend class FSavedMove_MPCharacter;

	int32 ClientCorrectionCount = 0;
};
