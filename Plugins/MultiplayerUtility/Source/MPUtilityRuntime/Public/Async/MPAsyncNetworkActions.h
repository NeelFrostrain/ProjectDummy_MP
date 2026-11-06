// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "Requests/MPNetworkRequestTypes.h"
#include "MPAsyncNetworkActions.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMPAsyncActorDelegate, AActor*, ReplicatedActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMPAsyncPlayerStateDelegate, APlayerState*, PlayerState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMPAsyncPawnDelegate, APawn*, PossessedPawn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMPAsyncTimeoutDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMPAsyncResponseDelegate, const FMPNetworkResponse&, Response);

/**
 * Async Action: Waits until a specific replicated actor is spawned and available on the client.
 */
UCLASS()
class MPUTILITYRUNTIME_API UMPAsyncWaitUntilActorReplicated : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FMPAsyncActorDelegate OnReplicated;

	UPROPERTY(BlueprintAssignable)
	FMPAsyncTimeoutDelegate OnTimedOut;

	/**
	 * Asynchronously polls until an actor of the requested class (and optional tag) replicates to this client.
	 */
	UFUNCTION(BlueprintCallable, Category = "Networking|Async", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"))
	static UMPAsyncWaitUntilActorReplicated* WaitUntilActorReplicated(
		const UObject* WorldContextObject,
		TSubclassOf<AActor> ActorClass,
		FName RequiredTag = NAME_None,
		float TimeoutSeconds = 10.0f
	);

	virtual void Activate() override;

private:
	void PollActor();
	void HandleTimeout();

	TWeakObjectPtr<const UObject> WorldContext;
	TSubclassOf<AActor> TargetClass;
	FName TargetTag = NAME_None;
	float Timeout = 10.0f;
	double StartTime = 0.0;
	FTimerHandle PollTimerHandle;
};

/**
 * Async Action: Waits until PlayerController has its replicated PlayerState assigned.
 */
UCLASS()
class MPUTILITYRUNTIME_API UMPAsyncWaitUntilPlayerState : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FMPAsyncPlayerStateDelegate OnAvailable;

	UPROPERTY(BlueprintAssignable)
	FMPAsyncTimeoutDelegate OnTimedOut;

	/**
	 * Asynchronously waits until PlayerState is valid on the given PlayerController.
	 */
	UFUNCTION(BlueprintCallable, Category = "Networking|Async", meta = (BlueprintInternalUseOnly = "true", DefaultToSelf = "Controller"))
	static UMPAsyncWaitUntilPlayerState* WaitUntilPlayerStateAvailable(
		APlayerController* Controller,
		float TimeoutSeconds = 5.0f
	);

	virtual void Activate() override;

private:
	void PollPlayerState();
	void HandleTimeout();

	TWeakObjectPtr<APlayerController> TargetController;
	float Timeout = 5.0f;
	double StartTime = 0.0;
	FTimerHandle PollTimerHandle;
};

/**
 * Async Action: Waits until PlayerController possesses a Pawn.
 */
UCLASS()
class MPUTILITYRUNTIME_API UMPAsyncWaitUntilPawnPossessed : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FMPAsyncPawnDelegate OnPossessed;

	UPROPERTY(BlueprintAssignable)
	FMPAsyncTimeoutDelegate OnTimedOut;

	/**
	 * Asynchronously waits until the PlayerController possesses a valid Pawn.
	 */
	UFUNCTION(BlueprintCallable, Category = "Networking|Async", meta = (BlueprintInternalUseOnly = "true", DefaultToSelf = "Controller"))
	static UMPAsyncWaitUntilPawnPossessed* WaitUntilPawnPossessed(
		APlayerController* Controller,
		float TimeoutSeconds = 5.0f
	);

	virtual void Activate() override;

private:
	void PollPossession();
	void HandleTimeout();

	TWeakObjectPtr<APlayerController> TargetController;
	float Timeout = 5.0f;
	double StartTime = 0.0;
	FTimerHandle PollTimerHandle;
};

/**
 * Async Action: Dispatches a reliable request to the server and branches on Success, Failed, or TimedOut.
 */
UCLASS()
class MPUTILITYRUNTIME_API UMPAsyncServerRequest : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FMPAsyncResponseDelegate OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FMPAsyncResponseDelegate OnFailed;

	UPROPERTY(BlueprintAssignable)
	FMPAsyncResponseDelegate OnTimedOut;

	/**
	 * Dispatches an asynchronous request to the server and routes response to output exec pins.
	 */
	UFUNCTION(BlueprintCallable, Category = "Networking|Async", meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"))
	static UMPAsyncServerRequest* AsyncSendServerRequest(
		const UObject* WorldContextObject,
		FMPNetworkRequest Request
	);

	virtual void Activate() override;

private:
	UFUNCTION()
	void HandleResponse(const FMPNetworkResponse& Response);

	TWeakObjectPtr<const UObject> WorldContext;
	FMPNetworkRequest PendingRequest;
};

