// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/MPRPCGameplayComponent.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

UMPRPCGameplayComponent::UMPRPCGameplayComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UMPRPCGameplayComponent::SendServerGameplayEvent(FName EventName, FVector Location, AActor* TargetActor)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	if (OwnerActor->HasAuthority())
	{
		// Standalone or Server dispatch
		OnServerEventReceived.Broadcast(EventName, Location, TargetActor);
	}
	else
	{
		Server_SendGameplayEvent(EventName, Location, TargetActor);
	}
}

void UMPRPCGameplayComponent::SendClientNotification(const FText& Message, FLinearColor Color, float Duration)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	Client_ReceiveGameplayNotification(Message, Color, Duration);
}

void UMPRPCGameplayComponent::BroadcastCosmeticCue(FName CueName, FVector Location, FRotator Rotation)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	Multicast_PlayCosmeticCue(CueName, Location, Rotation);
}

void UMPRPCGameplayComponent::BroadcastSoundAtLocation(USoundBase* Sound, FVector Location, float VolumeMultiplier, float PitchMultiplier)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || !Sound)
	{
		return;
	}

	Multicast_PlaySoundAtLocation(Sound, Location, VolumeMultiplier, PitchMultiplier);
}

bool UMPRPCGameplayComponent::Server_SendGameplayEvent_Validate(FName EventName, FVector Location, AActor* TargetActor)
{
	// Validate event name and coordinates are finite
	if (EventName.IsNone() || Location.ContainsNaN())
	{
		return false;
	}
	return true;
}

void UMPRPCGameplayComponent::Server_SendGameplayEvent_Implementation(FName EventName, FVector Location, AActor* TargetActor)
{
	OnServerEventReceived.Broadcast(EventName, Location, TargetActor);
}

void UMPRPCGameplayComponent::Client_ReceiveGameplayNotification_Implementation(const FText& Message, FLinearColor Color, float Duration)
{
	OnClientNotificationReceived.Broadcast(Message, Color, Duration);
}

void UMPRPCGameplayComponent::Multicast_PlayCosmeticCue_Implementation(FName CueName, FVector Location, FRotator Rotation)
{
	OnCosmeticCueReceived.Broadcast(CueName, Location, Rotation);
}

void UMPRPCGameplayComponent::Multicast_PlaySoundAtLocation_Implementation(USoundBase* Sound, FVector Location, float VolumeMultiplier, float PitchMultiplier)
{
	if (Sound && GetWorld())
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), Sound, Location, VolumeMultiplier, PitchMultiplier);
	}
}

