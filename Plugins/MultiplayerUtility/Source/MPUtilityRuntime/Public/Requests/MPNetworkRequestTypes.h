// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "GameplayTagContainer.h"
#include "MPNetworkRequestTypes.generated.h"

/**
 * State of an active network RPC request.
 */
UENUM(BlueprintType)
enum class EMPNetworkRequestState : uint8
{
	Uninitialized UMETA(DisplayName = "Uninitialized"),
	Pending       UMETA(DisplayName = "Pending Dispatch"),
	Dispatched    UMETA(DisplayName = "Dispatched (Awaiting Response)"),
	Completed     UMETA(DisplayName = "Completed Successfully"),
	Failed        UMETA(DisplayName = "Failed (Server Rejected)"),
	TimedOut      UMETA(DisplayName = "Timed Out"),
	Cancelled     UMETA(DisplayName = "Cancelled by Caller")
};

/**
 * Final execution result of a network RPC request.
 */
UENUM(BlueprintType)
enum class EMPNetworkRequestResult : uint8
{
	Success           UMETA(DisplayName = "Success"),
	ServerRejected    UMETA(DisplayName = "Server Rejected"),
	Timeout           UMETA(DisplayName = "Timed Out"),
	Cancelled         UMETA(DisplayName = "Cancelled"),
	InvalidTarget     UMETA(DisplayName = "Invalid Target / Controller"),
	NotAuthorized     UMETA(DisplayName = "Not Authorized (Missing Authority or Ownership)"),
	PayloadCorrupted  UMETA(DisplayName = "Payload Corrupted / Invalid"),
	Disconnected      UMETA(DisplayName = "Network Disconnected"),
	Unknown           UMETA(DisplayName = "Unknown Error")
};

/**
 * Replicated network request payload structure.
 */
USTRUCT(BlueprintType)
struct MPUTILITYRUNTIME_API FMPNetworkRequest
{
	GENERATED_BODY()

	/** Globally unique request identifier to correlate requests with responses. */
	UPROPERTY(BlueprintReadOnly, Category = "Networking|Requests")
	FGuid RequestId = FGuid();

	/** Semantic action or type name for the request. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Requests")
	FName RequestAction;

	/** Optional gameplay tag categorization for request routing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Requests")
	FGameplayTag RequestTag;

	/** Serialized payload data buffer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Requests")
	TArray<uint8> Payload;

	/** Client timestamp when the request was initiated (seconds). */
	UPROPERTY(BlueprintReadOnly, Category = "Networking|Requests")
	double ClientTimestamp = 0.0;

	/** Timeout duration in seconds before considering the request failed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Requests", meta = (ClampMin = "0.5", ClampMax = "60.0"))
	float TimeoutSeconds = 5.0f;

	/** Maximum allowed automatic retries (only allowed if bIsIdempotent is true). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Requests", meta = (ClampMin = "0", ClampMax = "5"))
	int32 MaxRetries = 0;

	/** Current retry attempt count. */
	UPROPERTY(BlueprintReadOnly, Category = "Networking|Requests")
	int32 RetryCount = 0;

	/** Whether this request is idempotent and safe to retry on timeout without side-effects. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Requests")
	bool bIsIdempotent = false;

	/** Current lifecycle state of the request. */
	UPROPERTY(BlueprintReadOnly, Category = "Networking|Requests")
	EMPNetworkRequestState State = EMPNetworkRequestState::Uninitialized;

	FMPNetworkRequest()
		: RequestId(FGuid::NewGuid())
		, RequestAction(NAME_None)
		, ClientTimestamp(0.0)
		, TimeoutSeconds(5.0f)
		, MaxRetries(0)
		, RetryCount(0)
		, bIsIdempotent(false)
		, State(EMPNetworkRequestState::Uninitialized)
	{
	}
};

/**
 * Network response payload structure returned by the server.
 */
USTRUCT(BlueprintType)
struct MPUTILITYRUNTIME_API FMPNetworkResponse
{
	GENERATED_BODY()

	/** Matching request identifier. */
	UPROPERTY(BlueprintReadOnly, Category = "Networking|Requests")
	FGuid RequestId;

	/** Result status of the request execution. */
	UPROPERTY(BlueprintReadOnly, Category = "Networking|Requests")
	EMPNetworkRequestResult Result = EMPNetworkRequestResult::Unknown;

	/** Descriptive failure or debug message from the server. */
	UPROPERTY(BlueprintReadOnly, Category = "Networking|Requests")
	FString Message;

	/** Serialized response payload data buffer. */
	UPROPERTY(BlueprintReadOnly, Category = "Networking|Requests")
	TArray<uint8> Payload;

	/** Authoritative server timestamp when response was generated. */
	UPROPERTY(BlueprintReadOnly, Category = "Networking|Requests")
	double ServerTimestamp = 0.0;

	FMPNetworkResponse()
		: RequestId()
		, Result(EMPNetworkRequestResult::Unknown)
		, ServerTimestamp(0.0)
	{
	}

	bool IsSuccess() const { return Result == EMPNetworkRequestResult::Success; }
};

/** Delegate fired when a response is received for an active request. */
DECLARE_DYNAMIC_DELEGATE_OneParam(FMPOnNetworkResponseDelegate, const FMPNetworkResponse&, Response);

/** Multicast delegate for global request monitoring and telemetry. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMPOnNetworkRequestLifecycleDelegate, const FMPNetworkRequest&, Request, const FMPNetworkResponse&, Response);

/**
 * Blueprint-accessible handle for tracking, polling, and cancelling an in-flight network request.
 */
UCLASS(BlueprintType)
class MPUTILITYRUNTIME_API UMPNetworkRequestHandle : public UObject
{
	GENERATED_BODY()

public:
	/** Request identifier being tracked. */
	UPROPERTY(BlueprintReadOnly, Category = "Networking|Requests")
	FGuid RequestId;

	/** Current state of the request. */
	UFUNCTION(BlueprintPure, Category = "Networking|Requests")
	EMPNetworkRequestState GetState() const { return CachedState; }

	/** Returns true if the request has resolved (completed, failed, timed out, or cancelled). */
	UFUNCTION(BlueprintPure, Category = "Networking|Requests")
	bool IsDone() const
	{
		return CachedState == EMPNetworkRequestState::Completed ||
		       CachedState == EMPNetworkRequestState::Failed ||
		       CachedState == EMPNetworkRequestState::TimedOut ||
		       CachedState == EMPNetworkRequestState::Cancelled;
	}

	/** Cancels the pending request if it is still in flight. */
	UFUNCTION(BlueprintCallable, Category = "Networking|Requests")
	bool Cancel();

	void UpdateState(EMPNetworkRequestState NewState) { CachedState = NewState; }
	void SetOwningComponent(class UMPNetworkRequestComponent* InComp);

private:
	UPROPERTY()
	EMPNetworkRequestState CachedState = EMPNetworkRequestState::Pending;

	UPROPERTY()
	TWeakObjectPtr<class UMPNetworkRequestComponent> OwningComponent;
};
