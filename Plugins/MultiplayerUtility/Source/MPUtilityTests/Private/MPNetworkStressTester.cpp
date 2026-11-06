// Copyright Epic Games, Inc. All Rights Reserved.

#include "MPNetworkStressTester.h"
#include "Types/MPRateLimiter.h"
#include "Types/MPRewindBuffer.h"
#include "HAL/PlatformTime.h"

FMPNetworkStressTestResult UMPNetworkStressTester::RunInEngineStressTest(const FMPNetworkStressTestConfig& Config)
{
	FMPNetworkStressTestResult Result;

	const double StartTime = FPlatformTime::Seconds();

	// Test a high-frequency token bucket rate limiter under simulated load
	// Capacity 50, refill 20 tokens/sec
	FMPRateLimiter Limiter(50.0, 20.0, 0.0);

	// Test a rewind buffer storing 128 snapshots
	FMPRewindBuffer TransformHistory(128, 2.0f);

	const int32 TotalIterations = FMath::Clamp(Config.ActorCount * Config.RPCCallsPerSecond, 10, 50000);
	double SimulatedTime = 0.0;
	const double TimeStep = 0.005; // 5ms step

	for (int32 i = 0; i < TotalIterations; ++i)
	{
		Result.TotalAttemptedRPCs++;
		SimulatedTime += TimeStep;

		// 1. Consume token
		if (Limiter.Consume(1.0, SimulatedTime))
		{
			Result.SuccessfulRPCs++;

			// 2. Push transform snapshot
			FTransform Snapshot(FRotator(0.0f, (float)i, 0.0f), FVector((float)i, 0.0f, 0.0f));
			TransformHistory.RecordSnapshot(SimulatedTime, Snapshot);
		}
		else
		{
			Result.DroppedOrRateLimitedRPCs++;
		}
	}

	const double EndTime = FPlatformTime::Seconds();
	Result.ExecutionDurationSeconds = EndTime - StartTime;

	if (Result.ExecutionDurationSeconds > 0.0)
	{
		Result.PeakThroughputRPCsPerSec = (double)Result.TotalAttemptedRPCs / Result.ExecutionDurationSeconds;
	}

	// Consistency verification: total attempted must equal successful + dropped
	Result.bPassedConsistencyCheck = (Result.TotalAttemptedRPCs == Result.SuccessfulRPCs + Result.DroppedOrRateLimitedRPCs) &&
	                                 (Result.SuccessfulRPCs > 0) && (Result.DroppedOrRateLimitedRPCs > 0);

	Result.SummaryReport = FString::Printf(
		TEXT("Stress Test Completed in %.4fs | Total Attempted: %d | Passed: %d | Throttled: %d | Throughput: %.0f ops/sec | Consistency: %s"),
		Result.ExecutionDurationSeconds,
		Result.TotalAttemptedRPCs,
		Result.SuccessfulRPCs,
		Result.DroppedOrRateLimitedRPCs,
		Result.PeakThroughputRPCsPerSec,
		Result.bPassedConsistencyCheck ? TEXT("PASSED") : TEXT("FAILED")
	);

	return Result;
}
