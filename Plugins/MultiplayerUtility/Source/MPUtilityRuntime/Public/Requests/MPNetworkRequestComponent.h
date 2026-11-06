// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Requests/MPNetworkRequestTypes.h"
#include "MPNetworkRequestComponent.generated.h"

class UMPNetworkRequestSubsystem;

DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(FMPNetworkResponse, FMPNetworkRequestHandlerDelegate, const FMPNetworkRequest&, Request, APlayerController*, Sender);

/**
 * Actor Component that facilitates bidirectional, reliable request/response RPC communication.
 * Attach this component to your PlayerController or Pawn to enable RPC Request/Response capability.
 */
UCLASS(ClassGroup = (Networking), meta = (BlueprintSpawnableComponent, DisplayName = "MP Network Request Component"))
class MPUTILITYRUNTIME_API UMPNetworkRequestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMPNetworkRequestComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 * Dispatches an RPC request from client to server.
	 */
	UFUNCTION(BlueprintCallable, Category = "Networking|Requests")
	bool SendRequest(UPARAM(ref) FMPNetworkRequest& Request, FMPOnNetworkResponseDelegate OnResponse, UMPNetworkRequestHandle*& OutHandle);

	/**
	 * C++ Native dispatch helper with TFunction callback.
	 */
	UMPNetworkRequestHandle* SendRequestNative(FMPNetworkRequest& Request, TFunction<void(const FMPNetworkResponse&)> InCallback);

	/**
	 * Registers a server-side handler for a specific RequestAction name.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Networking|Requests")
	void RegisterServerHandler(FName RequestAction, FMPNetworkRequestHandlerDelegate Handler);

	/**
	 * Unregisters a server-side handler.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Networking|Requests")
	void UnregisterServerHandler(FName RequestAction);

	/**
	 * Cancels a pending request by ID.
	 */
	UFUNCTION(BlueprintCallable, Category = "Networking|Requests")
	bool CancelRequest(const FGuid& RequestId);

protected:
	/** Server RPC: Handles client request on authoritative server. */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_ProcessRequest(const FMPNetworkRequest& Request);

	/** Client RPC: Delivers authoritative response back to the requesting client. */
	UFUNCTION(Client, Reliable)
	void Client_ReceiveResponse(const FMPNetworkResponse& Response);

private:
	friend class UMPNetworkRequestSubsystem;

	/** Dispatches timeout check for a specific request. */
	void HandleRequestTimeout(FGuid RequestId);

	struct FPendingClientRequest
	{
		FMPNetworkRequest Request;
		FMPOnNetworkResponseDelegate BPCallback;
		TFunction<void(const FMPNetworkResponse&)> NativeCallback;
		TWeakObjectPtr<UMPNetworkRequestHandle> Handle;
		FTimerHandle TimeoutTimer;
	};

	TMap<FGuid, FPendingClientRequest> PendingClientRequests;
	TMap<FName, FMPNetworkRequestHandlerDelegate> ServerHandlers;

	/** C++ native server handlers */
	TMap<FName, TFunction<FMPNetworkResponse(const FMPNetworkRequest&, APlayerController*)>> NativeServerHandlers;
};

