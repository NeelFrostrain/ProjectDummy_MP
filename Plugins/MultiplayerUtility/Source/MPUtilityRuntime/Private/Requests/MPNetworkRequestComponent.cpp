// Copyright Epic Games, Inc. All Rights Reserved.

#include "Requests/MPNetworkRequestComponent.h"
#include "Requests/MPNetworkRequestSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "TimerManager.h"

UMPNetworkRequestComponent::UMPNetworkRequestComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UMPNetworkRequestComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UMPNetworkRequestComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Clean up all active timers and pending requests to prevent leaks or stale callbacks
	UWorld* World = GetWorld();
	if (World)
	{
		FTimerManager& TM = World->GetTimerManager();
		for (auto& Pair : PendingClientRequests)
		{
			TM.ClearTimer(Pair.Value.TimeoutTimer);
			if (Pair.Value.Handle.IsValid())
			{
				Pair.Value.Handle->UpdateState(EMPNetworkRequestState::Cancelled);
			}

			FMPNetworkResponse DisconnectResponse;
			DisconnectResponse.RequestId = Pair.Key;
			DisconnectResponse.Result = EMPNetworkRequestResult::Disconnected;
			DisconnectResponse.Message = TEXT("Network Request Component destroyed (World transition or disconnect).");

			if (Pair.Value.NativeCallback)
			{
				Pair.Value.NativeCallback(DisconnectResponse);
			}
			else if (Pair.Value.BPCallback.IsBound())
			{
				Pair.Value.BPCallback.Execute(DisconnectResponse);
			}
		}
	}
	PendingClientRequests.Empty();
	ServerHandlers.Empty();
	NativeServerHandlers.Empty();

	Super::EndPlay(EndPlayReason);
}

bool UMPNetworkRequestComponent::SendRequest(
	FMPNetworkRequest& Request,
	FMPOnNetworkResponseDelegate OnResponse,
	UMPNetworkRequestHandle*& OutHandle)
{
	OutHandle = nullptr;

	if (!Request.RequestId.IsValid())
	{
		Request.RequestId = FGuid::NewGuid();
	}

	if (PendingClientRequests.Contains(Request.RequestId))
	{
		UE_LOG(LogNet, Warning, TEXT("[MPNetworkRequestComponent] Duplicate RequestId %s detected. Aborting dispatch."), *Request.RequestId.ToString());
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	Request.ClientTimestamp = World->GetTimeSeconds();
	Request.State = EMPNetworkRequestState::Pending;

	// Create tracking handle
	UMPNetworkRequestHandle* Handle = NewObject<UMPNetworkRequestHandle>(this);
	Handle->RequestId = Request.RequestId;
	Handle->UpdateState(EMPNetworkRequestState::Dispatched);
	Handle->SetOwningComponent(this);
	OutHandle = Handle;

	FPendingClientRequest Pending;
	Pending.Request = Request;
	Pending.BPCallback = OnResponse;
	Pending.Handle = Handle;

	// Setup timeout timer
	const float Timeout = FMath::Clamp(Request.TimeoutSeconds, 0.5f, 60.0f);
	const FGuid ReqId = Request.RequestId;
	World->GetTimerManager().SetTimer(
		Pending.TimeoutTimer,
		FTimerDelegate::CreateUObject(this, &UMPNetworkRequestComponent::HandleRequestTimeout, ReqId),
		Timeout,
		false
	);

	PendingClientRequests.Add(Request.RequestId, MoveTemp(Pending));

	// Notify subsystem
	if (UMPNetworkRequestSubsystem* Subsystem = World->GetSubsystem<UMPNetworkRequestSubsystem>())
	{
		FMPNetworkResponse InitialResp;
		InitialResp.RequestId = Request.RequestId;
		Subsystem->NotifyRequestLifecycle(Request, InitialResp);
	}

	// Dispatch reliable Server RPC
	Server_ProcessRequest(Request);
	return true;
}

UMPNetworkRequestHandle* UMPNetworkRequestComponent::SendRequestNative(
	FMPNetworkRequest& Request,
	TFunction<void(const FMPNetworkResponse&)> InCallback)
{
	UMPNetworkRequestHandle* OutHandle = nullptr;
	FMPOnNetworkResponseDelegate EmptyDelegate;

	if (SendRequest(Request, EmptyDelegate, OutHandle) && OutHandle)
	{
		if (FPendingClientRequest* Pending = PendingClientRequests.Find(Request.RequestId))
		{
			Pending->NativeCallback = MoveTemp(InCallback);
		}
	}
	return OutHandle;
}

void UMPNetworkRequestComponent::RegisterServerHandler(FName RequestAction, FMPNetworkRequestHandlerDelegate Handler)
{
	if (RequestAction.IsNone())
	{
		return;
	}
	ServerHandlers.Add(RequestAction, Handler);
}

void UMPNetworkRequestComponent::UnregisterServerHandler(FName RequestAction)
{
	ServerHandlers.Remove(RequestAction);
}

bool UMPNetworkRequestComponent::CancelRequest(const FGuid& RequestId)
{
	FPendingClientRequest* Found = PendingClientRequests.Find(RequestId);
	if (!Found)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(Found->TimeoutTimer);
	}

	if (Found->Handle.IsValid())
	{
		Found->Handle->UpdateState(EMPNetworkRequestState::Cancelled);
	}

	FMPNetworkResponse Response;
	Response.RequestId = RequestId;
	Response.Result = EMPNetworkRequestResult::Cancelled;
	Response.Message = TEXT("Request cancelled by client.");

	if (Found->NativeCallback)
	{
		Found->NativeCallback(Response);
	}
	else if (Found->BPCallback.IsBound())
	{
		Found->BPCallback.Execute(Response);
	}

	if (World)
	{
		if (UMPNetworkRequestSubsystem* Subsystem = World->GetSubsystem<UMPNetworkRequestSubsystem>())
		{
			Subsystem->NotifyRequestLifecycle(Found->Request, Response);
		}
	}

	PendingClientRequests.Remove(RequestId);
	return true;
}

bool UMPNetworkRequestComponent::Server_ProcessRequest_Validate(const FMPNetworkRequest& Request)
{
	// 1. Validate request ID
	if (!Request.RequestId.IsValid())
	{
		return false;
	}

	// 2. Bound payload size (max 64KB per request)
	if (Request.Payload.Num() > 64 * 1024)
	{
		return false;
	}

	// 3. Must have an action name or tag
	if (Request.RequestAction.IsNone() && !Request.RequestTag.IsValid())
	{
		return false;
	}

	return true;
}

void UMPNetworkRequestComponent::Server_ProcessRequest_Implementation(const FMPNetworkRequest& Request)
{
	FMPNetworkResponse Response;
	Response.RequestId = Request.RequestId;
	Response.ServerTimestamp = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;

	APlayerController* SenderPC = Cast<APlayerController>(GetOwner());
	if (!SenderPC)
	{
		if (APawn* Pawn = Cast<APawn>(GetOwner()))
		{
			SenderPC = Cast<APlayerController>(Pawn->GetController());
		}
	}

	// Check if a handler is registered for this action
	if (const FMPNetworkRequestHandlerDelegate* Handler = ServerHandlers.Find(Request.RequestAction))
	{
		if (Handler->IsBound())
		{
			Response = Handler->Execute(Request, SenderPC);
			Response.RequestId = Request.RequestId; // Ensure correlation ID matches
		}
		else
		{
			Response.Result = EMPNetworkRequestResult::ServerRejected;
			Response.Message = FString::Printf(TEXT("Server handler for '%s' was unbound."), *Request.RequestAction.ToString());
		}
	}
	else if (const auto* NativeHandler = NativeServerHandlers.Find(Request.RequestAction))
	{
		Response = (*NativeHandler)(Request, SenderPC);
		Response.RequestId = Request.RequestId;
	}
	else
	{
		Response.Result = EMPNetworkRequestResult::ServerRejected;
		Response.Message = FString::Printf(TEXT("No server handler registered for action '%s'."), *Request.RequestAction.ToString());
	}

	// Deliver authoritative response back to calling client
	Client_ReceiveResponse(Response);
}

void UMPNetworkRequestComponent::Client_ReceiveResponse_Implementation(const FMPNetworkResponse& Response)
{
	FPendingClientRequest* Found = PendingClientRequests.Find(Response.RequestId);
	if (!Found)
	{
		// Could be a response to a timed-out or cancelled request
		UE_LOG(LogNet, Log, TEXT("[MPNetworkRequestComponent] Received late response for RequestId %s (already resolved)."), *Response.RequestId.ToString());
		return;
	}

	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(Found->TimeoutTimer);
	}

	if (Found->Handle.IsValid())
	{
		Found->Handle->UpdateState(Response.IsSuccess() ? EMPNetworkRequestState::Completed : EMPNetworkRequestState::Failed);
	}

	// Notify subsystem
	if (World)
	{
		if (UMPNetworkRequestSubsystem* Subsystem = World->GetSubsystem<UMPNetworkRequestSubsystem>())
		{
			Subsystem->NotifyRequestLifecycle(Found->Request, Response);
		}
	}

	// Invoke callback
	if (Found->NativeCallback)
	{
		Found->NativeCallback(Response);
	}
	else if (Found->BPCallback.IsBound())
	{
		Found->BPCallback.Execute(Response);
	}

	PendingClientRequests.Remove(Response.RequestId);
}

void UMPNetworkRequestComponent::HandleRequestTimeout(FGuid RequestId)
{
	FPendingClientRequest* Found = PendingClientRequests.Find(RequestId);
	if (!Found)
	{
		return;
	}

	// Bounded retry check: only allowed for idempotent requests
	if (Found->Request.bIsIdempotent && Found->Request.RetryCount < Found->Request.MaxRetries)
	{
		Found->Request.RetryCount++;
		UE_LOG(LogNet, Log, TEXT("[MPNetworkRequestComponent] Retrying idempotent request %s (Attempt %d/%d)"),
			*RequestId.ToString(), Found->Request.RetryCount, Found->Request.MaxRetries);

		UWorld* World = GetWorld();
		if (World)
		{
			const float Timeout = FMath::Clamp(Found->Request.TimeoutSeconds, 0.5f, 60.0f);
			World->GetTimerManager().SetTimer(
				Found->TimeoutTimer,
				FTimerDelegate::CreateUObject(this, &UMPNetworkRequestComponent::HandleRequestTimeout, RequestId),
				Timeout,
				false
			);
		}

		Server_ProcessRequest(Found->Request);
		return;
	}

	if (Found->Handle.IsValid())
	{
		Found->Handle->UpdateState(EMPNetworkRequestState::TimedOut);
	}

	FMPNetworkResponse Response;
	Response.RequestId = RequestId;
	Response.Result = EMPNetworkRequestResult::Timeout;
	Response.Message = FString::Printf(TEXT("Request '%s' timed out after %.1f seconds."),
		*Found->Request.RequestAction.ToString(), Found->Request.TimeoutSeconds);

	UWorld* World = GetWorld();
	if (World)
	{
		if (UMPNetworkRequestSubsystem* Subsystem = World->GetSubsystem<UMPNetworkRequestSubsystem>())
		{
			Subsystem->NotifyRequestLifecycle(Found->Request, Response);
		}
	}

	if (Found->NativeCallback)
	{
		Found->NativeCallback(Response);
	}
	else if (Found->BPCallback.IsBound())
	{
		Found->BPCallback.Execute(Response);
	}

	PendingClientRequests.Remove(RequestId);
}

