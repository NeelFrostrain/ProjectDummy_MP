// Copyright Epic Games, Inc. All Rights Reserved.

#include "Requests/MPNetworkRequestSubsystem.h"
#include "Requests/MPNetworkRequestComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

void UMPNetworkRequestSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	TotalDispatchedRequests = 0;
	TotalTimedOutRequests = 0;
}

void UMPNetworkRequestSubsystem::Deinitialize()
{
	TotalDispatchedRequests = 0;
	TotalTimedOutRequests = 0;
	Super::Deinitialize();
}

bool UMPNetworkRequestSubsystem::SendNetworkRequest(
	const UObject* WorldContextObject,
	FMPNetworkRequest& Request,
	FMPOnNetworkResponseDelegate OnResponse,
	UMPNetworkRequestHandle*& OutHandle)
{
	OutHandle = nullptr;

	if (!WorldContextObject)
	{
		return false;
	}

	const UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return false;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC)
	{
		return false;
	}

	UMPNetworkRequestComponent* Comp = PC->FindComponentByClass<UMPNetworkRequestComponent>();
	if (!Comp)
	{
		// Auto-create component on player controller if not present
		Comp = NewObject<UMPNetworkRequestComponent>(PC, TEXT("Auto_MPNetworkRequestComponent"));
		Comp->RegisterComponent();
	}

	return Comp->SendRequest(Request, OnResponse, OutHandle);
}

void UMPNetworkRequestSubsystem::NotifyRequestLifecycle(const FMPNetworkRequest& Request, const FMPNetworkResponse& Response)
{
	if (Response.Result == EMPNetworkRequestResult::Unknown)
	{
		TotalDispatchedRequests++;
	}
	else if (Response.Result == EMPNetworkRequestResult::Timeout)
	{
		TotalTimedOutRequests++;
	}

	if (OnRequestLifecycleEvent.IsBound())
	{
		OnRequestLifecycleEvent.Broadcast(Request, Response);
	}
}

