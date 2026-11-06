// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Requests/MPNetworkRequestTypes.h"
#include "MPNetworkRequestSubsystem.generated.h"

class UMPNetworkRequestComponent;

/**
 * World subsystem orchestrating RPC Request / Response operations, lifetime tracking, and telemetry.
 */
UCLASS(DisplayName = "MP Network Request Subsystem")
class MPUTILITYRUNTIME_API UMPNetworkRequestSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Global lifecycle event for telemetry and debugging. */
	UPROPERTY(BlueprintAssignable, Category = "Networking|Requests")
	FMPOnNetworkRequestLifecycleDelegate OnRequestLifecycleEvent;

	/**
	 * Convenience helper: Sends a request via the local PlayerController's UMPNetworkRequestComponent.
	 */
	UFUNCTION(BlueprintCallable, Category = "Networking|Requests", meta = (WorldContext = "WorldContextObject"))
	static bool SendNetworkRequest(
		const UObject* WorldContextObject,
		UPARAM(ref) FMPNetworkRequest& Request,
		FMPOnNetworkResponseDelegate OnResponse,
		UMPNetworkRequestHandle*& OutHandle
	);

	/**
	 * Broadcasts request lifecycle events for telemetry and diagnostics.
	 */
	void NotifyRequestLifecycle(const FMPNetworkRequest& Request, const FMPNetworkResponse& Response);

	/** Returns the total number of requests dispatched during this world's lifetime. */
	UFUNCTION(BlueprintPure, Category = "Networking|Requests")
	int32 GetTotalDispatchedRequests() const { return TotalDispatchedRequests; }

	/** Returns the total number of requests that timed out. */
	UFUNCTION(BlueprintPure, Category = "Networking|Requests")
	int32 GetTotalTimedOutRequests() const { return TotalTimedOutRequests; }

private:
	int32 TotalDispatchedRequests = 0;
	int32 TotalTimedOutRequests = 0;
};

