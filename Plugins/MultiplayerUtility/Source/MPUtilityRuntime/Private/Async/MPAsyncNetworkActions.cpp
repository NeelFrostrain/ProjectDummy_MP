// Copyright Epic Games, Inc. All Rights Reserved.

#include "Async/MPAsyncNetworkActions.h"
#include "Requests/MPNetworkRequestSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"

// --------------------------------------------------------------------------
// UMPAsyncWaitUntilActorReplicated
// --------------------------------------------------------------------------

UMPAsyncWaitUntilActorReplicated* UMPAsyncWaitUntilActorReplicated::WaitUntilActorReplicated(
	const UObject* WorldContextObject,
	TSubclassOf<AActor> ActorClass,
	FName RequiredTag,
	float TimeoutSeconds)
{
	UMPAsyncWaitUntilActorReplicated* Action = NewObject<UMPAsyncWaitUntilActorReplicated>();
	Action->WorldContext = WorldContextObject;
	Action->TargetClass = ActorClass;
	Action->TargetTag = RequiredTag;
	Action->Timeout = FMath::Max(0.5f, TimeoutSeconds);
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UMPAsyncWaitUntilActorReplicated::Activate()
{
	if (!WorldContext.IsValid() || !*TargetClass)
	{
		OnTimedOut.Broadcast();
		SetReadyToDestroy();
		return;
	}

	UWorld* World = WorldContext->GetWorld();
	if (!World)
	{
		OnTimedOut.Broadcast();
		SetReadyToDestroy();
		return;
	}

	StartTime = World->GetTimeSeconds();

	// Immediate check
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(World, TargetClass, FoundActors);
	for (AActor* A : FoundActors)
	{
		if (TargetTag.IsNone() || A->ActorHasTag(TargetTag))
		{
			OnReplicated.Broadcast(A);
			SetReadyToDestroy();
			return;
		}
	}

	// Schedule lightweight 50ms polling timer
	World->GetTimerManager().SetTimer(
		PollTimerHandle,
		this,
		&UMPAsyncWaitUntilActorReplicated::PollActor,
		0.05f,
		true
	);
}

void UMPAsyncWaitUntilActorReplicated::PollActor()
{
	UWorld* World = WorldContext.IsValid() ? WorldContext->GetWorld() : nullptr;
	if (!World)
	{
		SetReadyToDestroy();
		return;
	}

	// Check timeout
	if (World->GetTimeSeconds() - StartTime >= Timeout)
	{
		World->GetTimerManager().ClearTimer(PollTimerHandle);
		OnTimedOut.Broadcast();
		SetReadyToDestroy();
		return;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(World, TargetClass, FoundActors);
	for (AActor* A : FoundActors)
	{
		if (TargetTag.IsNone() || A->ActorHasTag(TargetTag))
		{
			World->GetTimerManager().ClearTimer(PollTimerHandle);
			OnReplicated.Broadcast(A);
			SetReadyToDestroy();
			return;
		}
	}
}

// --------------------------------------------------------------------------
// UMPAsyncWaitUntilPlayerState
// --------------------------------------------------------------------------

UMPAsyncWaitUntilPlayerState* UMPAsyncWaitUntilPlayerState::WaitUntilPlayerStateAvailable(
	APlayerController* Controller,
	float TimeoutSeconds)
{
	UMPAsyncWaitUntilPlayerState* Action = NewObject<UMPAsyncWaitUntilPlayerState>();
	Action->TargetController = Controller;
	Action->Timeout = FMath::Max(0.5f, TimeoutSeconds);
	Action->RegisterWithGameInstance(Controller);
	return Action;
}

void UMPAsyncWaitUntilPlayerState::Activate()
{
	if (!TargetController.IsValid())
	{
		OnTimedOut.Broadcast();
		SetReadyToDestroy();
		return;
	}

	// Immediate check
	if (TargetController->PlayerState)
	{
		OnAvailable.Broadcast(TargetController->PlayerState);
		SetReadyToDestroy();
		return;
	}

	UWorld* World = TargetController->GetWorld();
	if (!World)
	{
		OnTimedOut.Broadcast();
		SetReadyToDestroy();
		return;
	}

	StartTime = World->GetTimeSeconds();
	World->GetTimerManager().SetTimer(
		PollTimerHandle,
		this,
		&UMPAsyncWaitUntilPlayerState::PollPlayerState,
		0.05f,
		true
	);
}

void UMPAsyncWaitUntilPlayerState::PollPlayerState()
{
	if (!TargetController.IsValid())
	{
		SetReadyToDestroy();
		return;
	}

	UWorld* World = TargetController->GetWorld();
	if (!World)
	{
		SetReadyToDestroy();
		return;
	}

	if (TargetController->PlayerState)
	{
		World->GetTimerManager().ClearTimer(PollTimerHandle);
		OnAvailable.Broadcast(TargetController->PlayerState);
		SetReadyToDestroy();
		return;
	}

	if (World->GetTimeSeconds() - StartTime >= Timeout)
	{
		World->GetTimerManager().ClearTimer(PollTimerHandle);
		OnTimedOut.Broadcast();
		SetReadyToDestroy();
	}
}

// --------------------------------------------------------------------------
// UMPAsyncWaitUntilPawnPossessed
// --------------------------------------------------------------------------

UMPAsyncWaitUntilPawnPossessed* UMPAsyncWaitUntilPawnPossessed::WaitUntilPawnPossessed(
	APlayerController* Controller,
	float TimeoutSeconds)
{
	UMPAsyncWaitUntilPawnPossessed* Action = NewObject<UMPAsyncWaitUntilPawnPossessed>();
	Action->TargetController = Controller;
	Action->Timeout = FMath::Max(0.5f, TimeoutSeconds);
	Action->RegisterWithGameInstance(Controller);
	return Action;
}

void UMPAsyncWaitUntilPawnPossessed::Activate()
{
	if (!TargetController.IsValid())
	{
		OnTimedOut.Broadcast();
		SetReadyToDestroy();
		return;
	}

	// Immediate check
	if (TargetController->GetPawn())
	{
		OnPossessed.Broadcast(TargetController->GetPawn());
		SetReadyToDestroy();
		return;
	}

	UWorld* World = TargetController->GetWorld();
	if (!World)
	{
		OnTimedOut.Broadcast();
		SetReadyToDestroy();
		return;
	}

	StartTime = World->GetTimeSeconds();
	World->GetTimerManager().SetTimer(
		PollTimerHandle,
		this,
		&UMPAsyncWaitUntilPawnPossessed::PollPossession,
		0.05f,
		true
	);
}

void UMPAsyncWaitUntilPawnPossessed::PollPossession()
{
	if (!TargetController.IsValid())
	{
		SetReadyToDestroy();
		return;
	}

	UWorld* World = TargetController->GetWorld();
	if (!World)
	{
		SetReadyToDestroy();
		return;
	}

	if (TargetController->GetPawn())
	{
		World->GetTimerManager().ClearTimer(PollTimerHandle);
		OnPossessed.Broadcast(TargetController->GetPawn());
		SetReadyToDestroy();
		return;
	}

	if (World->GetTimeSeconds() - StartTime >= Timeout)
	{
		World->GetTimerManager().ClearTimer(PollTimerHandle);
		OnTimedOut.Broadcast();
		SetReadyToDestroy();
	}
}

// --------------------------------------------------------------------------
// UMPAsyncServerRequest
// --------------------------------------------------------------------------

UMPAsyncServerRequest* UMPAsyncServerRequest::AsyncSendServerRequest(
	const UObject* WorldContextObject,
	FMPNetworkRequest Request)
{
	UMPAsyncServerRequest* Action = NewObject<UMPAsyncServerRequest>();
	Action->WorldContext = WorldContextObject;
	Action->PendingRequest = Request;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UMPAsyncServerRequest::Activate()
{
	if (!WorldContext.IsValid())
	{
		FMPNetworkResponse Resp;
		Resp.Result = EMPNetworkRequestResult::InvalidTarget;
		Resp.Message = TEXT("Invalid WorldContext.");
		OnFailed.Broadcast(Resp);
		SetReadyToDestroy();
		return;
	}

	FMPOnNetworkResponseDelegate ResponseDelegate;
	ResponseDelegate.BindDynamic(this, &UMPAsyncServerRequest::HandleResponse);

	UMPNetworkRequestHandle* Handle = nullptr;
	bool bSent = UMPNetworkRequestSubsystem::SendNetworkRequest(
		WorldContext.Get(),
		PendingRequest,
		ResponseDelegate,
		Handle
	);

	if (!bSent)
	{
		FMPNetworkResponse Resp;
		Resp.Result = EMPNetworkRequestResult::InvalidTarget;
		Resp.Message = TEXT("Failed to dispatch request via PlayerController.");
		OnFailed.Broadcast(Resp);
		SetReadyToDestroy();
	}
}

void UMPAsyncServerRequest::HandleResponse(const FMPNetworkResponse& Response)
{
	if (Response.Result == EMPNetworkRequestResult::Success)
	{
		OnSuccess.Broadcast(Response);
	}
	else if (Response.Result == EMPNetworkRequestResult::Timeout)
	{
		OnTimedOut.Broadcast(Response);
	}
	else
	{
		OnFailed.Broadcast(Response);
	}

	SetReadyToDestroy();
}

