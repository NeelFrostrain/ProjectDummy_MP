// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MPRPCGameplayComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMPGameplayEventReceived, FName, EventName, FVector, Location, AActor*, TargetActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMPGameplayNotificationReceived, const FText&, Message, FLinearColor, Color, float, Duration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMPCosmeticCueReceived, FName, CueName, FVector, Location, FRotator, Rotation);

/**
 * Reusable network RPC component enabling actors and controllers to dispatch
 * server-authoritative events, client-directed notifications, and multicast cosmetic cues.
 */
UCLASS(ClassGroup = (MultiplayerUtility), meta = (BlueprintSpawnableComponent))
class MPUTILITYRUNTIME_API UMPRPCGameplayComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMPRPCGameplayComponent();

	/** Fired on the server when a client dispatches a validated gameplay event. */
	UPROPERTY(BlueprintAssignable, Category = "Multiplayer Utility|RPC")
	FOnMPGameplayEventReceived OnServerEventReceived;

	/** Fired on the client when receiving a server notification. */
	UPROPERTY(BlueprintAssignable, Category = "Multiplayer Utility|RPC")
	FOnMPGameplayNotificationReceived OnClientNotificationReceived;

	/** Fired on all clients when receiving a multicast cosmetic cue. */
	UPROPERTY(BlueprintAssignable, Category = "Multiplayer Utility|RPC")
	FOnMPCosmeticCueReceived OnCosmeticCueReceived;

	/** Dispatches a gameplay event to the server via Server RPC (Client-to-Server). */
	UFUNCTION(BlueprintCallable, Category = "Multiplayer Utility|RPC")
	void SendServerGameplayEvent(FName EventName, FVector Location, AActor* TargetActor);

	/** Sends a private text notification to this actor's owning client (Server-to-Client). */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Multiplayer Utility|RPC")
	void SendClientNotification(const FText& Message, FLinearColor Color = FLinearColor::Yellow, float Duration = 3.0f);

	/** Broadcasts an unreliable cosmetic cue to all replicated clients (Server-to-All). */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Multiplayer Utility|RPC")
	void BroadcastCosmeticCue(FName CueName, FVector Location, FRotator Rotation = FRotator::ZeroRotator);

	/** Plays a sound across all clients at location without replicating actor state. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Multiplayer Utility|RPC")
	void BroadcastSoundAtLocation(USoundBase* Sound, FVector Location, float VolumeMultiplier = 1.0f, float PitchMultiplier = 1.0f);

protected:
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendGameplayEvent(FName EventName, FVector Location, AActor* TargetActor);

	UFUNCTION(Client, Reliable)
	void Client_ReceiveGameplayNotification(const FText& Message, FLinearColor Color, float Duration);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayCosmeticCue(FName CueName, FVector Location, FRotator Rotation);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlaySoundAtLocation(USoundBase* Sound, FVector Location, float VolumeMultiplier, float PitchMultiplier);
};

