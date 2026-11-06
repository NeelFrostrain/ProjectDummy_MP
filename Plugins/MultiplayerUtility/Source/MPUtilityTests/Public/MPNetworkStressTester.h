// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MPNetworkStressTester.generated.h"

/**
 * Configuration parameters for network stress testing.
 */
USTRUCT(BlueprintType)
struct MPUTILITYTESTS_API FMPNetworkStressTestConfig
{
	GENERATED_BODY()

	/** Number of mock actors to simulate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Testing")
	int32 ActorCount = 50;

	/** Target RPC dispatch rate per second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Testing")
	int32 RPCCallsPerSecond = 100;

	/** Duration of stress simulation in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Testing")
	float DurationSeconds = 2.0f;

	/** Simulated latency in milliseconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Testing")
	float SimulatedLatencyMs = 80.0f;

	/** Simulated packet loss percentage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Networking|Testing")
	float SimulatedPacketLossPercent = 2.0f;

	FMPNetworkStressTestConfig() = default;
};

/**
 * Metrics recorded during a stress test run.
 */
USTRUCT(BlueprintType)
struct MPUTILITYTESTS_API FMPNetworkStressTestResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Networking|Testing")
	int32 TotalAttemptedRPCs = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Networking|Testing")
	int32 SuccessfulRPCs = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Networking|Testing")
	int32 DroppedOrRateLimitedRPCs = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Networking|Testing")
	double ExecutionDurationSeconds = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Networking|Testing")
	double PeakThroughputRPCsPerSec = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Networking|Testing")
	bool bPassedConsistencyCheck = false;

	UPROPERTY(BlueprintReadOnly, Category = "Networking|Testing")
	FString SummaryReport;

	FMPNetworkStressTestResult() = default;
};

/**
 * Reusable stress testing benchmark runner.
 */
UCLASS()
class MPUTILITYTESTS_API UMPNetworkStressTester : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Executes an in-memory network stress simulation testing token-bucket rate limits,
	 * request tracking buffers, and state consistency under high call frequencies.
	 */
	UFUNCTION(BlueprintCallable, Category = "Networking|Testing")
	static FMPNetworkStressTestResult RunInEngineStressTest(const FMPNetworkStressTestConfig& Config);
};

