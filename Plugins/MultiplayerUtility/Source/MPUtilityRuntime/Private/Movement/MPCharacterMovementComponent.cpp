// Copyright Epic Games, Inc. All Rights Reserved.

#include "Movement/MPCharacterMovementComponent.h"
#include "GameFramework/Character.h"

// --------------------------------------------------------------------------
// FSavedMove_MPCharacter Implementation
// --------------------------------------------------------------------------

FSavedMove_MPCharacter::FSavedMove_MPCharacter()
	: bWantsToSprint(0)
	, bWantsToDash(0)
	, bWantsToSlide(0)
	, bIsClimbing(0)
{
}

void FSavedMove_MPCharacter::Clear()
{
	Super::Clear();
	bWantsToSprint = 0;
	bWantsToDash = 0;
	bWantsToSlide = 0;
	bIsClimbing = 0;
}

uint8 FSavedMove_MPCharacter::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();

	if (bWantsToSprint)
	{
		Result |= FLAG_Custom_0;
	}
	if (bWantsToDash)
	{
		Result |= FLAG_Custom_1;
	}
	if (bWantsToSlide)
	{
		Result |= FLAG_Custom_2;
	}
	if (bIsClimbing)
	{
		Result |= FLAG_Custom_3;
	}

	return Result;
}

bool FSavedMove_MPCharacter::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const
{
	const FSavedMove_MPCharacter* NewMPMove = static_cast<const FSavedMove_MPCharacter*>(NewMove.Get());
	if (!NewMPMove)
	{
		return false;
	}

	// Never combine an impulse dash move with another move
	if (bWantsToDash || NewMPMove->bWantsToDash)
	{
		return false;
	}

	if (bWantsToSprint != NewMPMove->bWantsToSprint ||
	    bWantsToSlide != NewMPMove->bWantsToSlide ||
	    bIsClimbing != NewMPMove->bIsClimbing)
	{
		return false;
	}

	return Super::CanCombineWith(NewMove, Character, MaxDelta);
}

void FSavedMove_MPCharacter::SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);

	if (const UMPCharacterMovementComponent* MPMove = Cast<UMPCharacterMovementComponent>(Character->GetCharacterMovement()))
	{
		bWantsToSprint = MPMove->bWantsToSprint;
		bWantsToDash = MPMove->bWantsToDash;
		bWantsToSlide = MPMove->bWantsToSlide;
		bIsClimbing = MPMove->bIsClimbing;
	}
}

void FSavedMove_MPCharacter::PrepMoveFor(ACharacter* Character)
{
	Super::PrepMoveFor(Character);

	if (UMPCharacterMovementComponent* MPMove = Cast<UMPCharacterMovementComponent>(Character->GetCharacterMovement()))
	{
		MPMove->bWantsToSprint = bWantsToSprint;
		MPMove->bWantsToDash = bWantsToDash;
		MPMove->bWantsToSlide = bWantsToSlide;
		MPMove->bIsClimbing = bIsClimbing;
	}
}

// --------------------------------------------------------------------------
// FNetworkPredictionData_Client_MPCharacter Implementation
// --------------------------------------------------------------------------

FNetworkPredictionData_Client_MPCharacter::FNetworkPredictionData_Client_MPCharacter(const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{
}

FSavedMovePtr FNetworkPredictionData_Client_MPCharacter::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_MPCharacter());
}

// --------------------------------------------------------------------------
// UMPCharacterMovementComponent Implementation
// --------------------------------------------------------------------------

UMPCharacterMovementComponent::UMPCharacterMovementComponent()
	: bWantsToSprint(0)
	, bWantsToDash(0)
	, bWantsToSlide(0)
	, bIsClimbing(0)
	, ClientCorrectionCount(0)
{
}

FNetworkPredictionData_Client* UMPCharacterMovementComponent::GetPredictionData_Client() const
{
	if (!ClientPredictionData)
	{
		UMPCharacterMovementComponent* MutableThis = const_cast<UMPCharacterMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_MPCharacter(*this);
	}
	return ClientPredictionData;
}

void UMPCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	bWantsToSprint = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
	bWantsToDash   = (Flags & FSavedMove_Character::FLAG_Custom_1) != 0;
	bWantsToSlide  = (Flags & FSavedMove_Character::FLAG_Custom_2) != 0;
	bIsClimbing    = (Flags & FSavedMove_Character::FLAG_Custom_3) != 0;
}

float UMPCharacterMovementComponent::GetMaxSpeed() const
{
	if (bIsClimbing)
	{
		return ClimbSpeed;
	}

	float BaseSpeed = Super::GetMaxSpeed();

	if (bWantsToSlide && IsWalking())
	{
		BaseSpeed *= SlideSpeedMultiplier;
	}
	else if (bWantsToSprint && IsWalking())
	{
		BaseSpeed *= SprintSpeedMultiplier;
	}

	return BaseSpeed;
}

void UMPCharacterMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	// Handle instantaneous dash impulse trigger
	if (bWantsToDash)
	{
		if (CharacterOwner)
		{
			FVector DashDirection = CharacterOwner->GetActorForwardVector();
			if (Acceleration.SizeSquared() > 0.0f)
			{
				DashDirection = Acceleration.GetSafeNormal();
			}
			Velocity += DashDirection * DashImpulse;
		}
		// Clear dash after applying impulse for this move
		bWantsToDash = 0;
	}

	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
}

void UMPCharacterMovementComponent::SetSprinting(bool bNewSprint)
{
	bWantsToSprint = bNewSprint ? 1 : 0;
}

void UMPCharacterMovementComponent::TriggerDash()
{
	bWantsToDash = 1;
}

void UMPCharacterMovementComponent::SetSliding(bool bNewSlide)
{
	bWantsToSlide = bNewSlide ? 1 : 0;
}

void UMPCharacterMovementComponent::SetClimbing(bool bNewClimb)
{
	bIsClimbing = bNewClimb ? 1 : 0;
	if (bIsClimbing)
	{
		SetMovementMode(MOVE_Flying); // Use flying mode for climbing interpolation
	}
	else if (MovementMode == MOVE_Flying)
	{
		SetMovementMode(MOVE_Falling);
	}
}

void UMPCharacterMovementComponent::ApplySmoothingProfile(EMPNetworkSmoothingProfile Profile)
{
	switch (Profile)
	{
	case EMPNetworkSmoothingProfile::TightFastPaced:
		NetworkSmoothingMode = ENetworkSmoothingMode::Exponential;
		NetworkMaxSmoothUpdateDistance = 92.0f;
		NetworkNoSmoothUpdateDistance = 184.0f;
		break;

	case EMPNetworkSmoothingProfile::CinematicSmooth:
		NetworkSmoothingMode = ENetworkSmoothingMode::Linear;
		NetworkMaxSmoothUpdateDistance = 256.0f;
		NetworkNoSmoothUpdateDistance = 512.0f;
		break;

	case EMPNetworkSmoothingProfile::HighLatencyConservative:
		NetworkSmoothingMode = ENetworkSmoothingMode::Exponential;
		NetworkMaxSmoothUpdateDistance = 384.0f;
		NetworkNoSmoothUpdateDistance = 768.0f;
		break;

	case EMPNetworkSmoothingProfile::Default:
	default:
		NetworkSmoothingMode = ENetworkSmoothingMode::Exponential;
		NetworkMaxSmoothUpdateDistance = 192.0f;
		NetworkNoSmoothUpdateDistance = 384.0f;
		break;
	}
}

bool UMPCharacterMovementComponent::ValidateMovementStep(
	const FVector& PreviousLocation,
	const FVector& ProposedLocation,
	float DeltaSeconds,
	float AllowedMaxSpeed,
	float ToleranceMargin,
	float& OutDistanceError)
{
	OutDistanceError = 0.0f;

	if (DeltaSeconds <= 0.0f || AllowedMaxSpeed <= 0.0f)
	{
		return false;
	}

	const float Distance = FVector::Dist(PreviousLocation, ProposedLocation);
	const float MaxExpectedDistance = (AllowedMaxSpeed * DeltaSeconds) + ToleranceMargin;

	if (Distance > MaxExpectedDistance)
	{
		OutDistanceError = Distance - MaxExpectedDistance;
		return false;
	}

	return true;
}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
void UMPCharacterMovementComponent::OnClientCorrectionReceived(
	FNetworkPredictionData_Client_Character& ClientData,
	float TimeStamp,
	FVector NewLocation,
	FVector NewVelocity,
	UPrimitiveComponent* NewBase,
	FName NewBaseBoneName,
	bool bHasBase,
	bool bBaseRelativePosition,
	uint8 ServerMovementMode,
	FVector ServerGravityDirection)
{
	ClientCorrectionCount++;
	Super::OnClientCorrectionReceived(ClientData, TimeStamp, NewLocation, NewVelocity, NewBase, NewBaseBoneName, bHasBase, bBaseRelativePosition, ServerMovementMode, ServerGravityDirection);
}
#else
void UMPCharacterMovementComponent::OnClientCorrectionReceived(
	FNetworkPredictionData_Client_Character& ClientData,
	float TimeStamp,
	FVector NewLocation,
	FVector NewVelocity,
	UPrimitiveComponent* NewBase,
	FName NewBaseBoneName,
	bool bHasBase,
	bool bBaseRelativePosition,
	uint8 ServerMovementMode)
{
	ClientCorrectionCount++;
	Super::OnClientCorrectionReceived(ClientData, TimeStamp, NewLocation, NewVelocity, NewBase, NewBaseBoneName, bHasBase, bBaseRelativePosition, ServerMovementMode);
}
#endif
