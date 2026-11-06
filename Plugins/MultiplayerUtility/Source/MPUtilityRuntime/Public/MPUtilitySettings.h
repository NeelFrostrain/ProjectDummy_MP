#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MPUtilitySettings.generated.h"

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Multiplayer Utility"))
class MPUTILITYRUNTIME_API UMPUtilitySettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UMPUtilitySettings();

    virtual FName GetCategoryName() const override { return FName(TEXT("Plugins")); }
    virtual FName GetSectionName() const override { return FName(TEXT("Multiplayer Utility")); }

    // Ping in milliseconds that triggers high-ping warning badges/logs
    UPROPERTY(EditAnywhere, Config, Category="Network Diagnostics", meta=(ClampMin="20.0", ClampMax="1000.0"))
    float HighPingThresholdMs = 150.0f;

    // Critical ping in milliseconds for connection quality categorization
    UPROPERTY(EditAnywhere, Config, Category="Network Diagnostics", meta=(ClampMin="50.0", ClampMax="2000.0"))
    float CriticalPingThresholdMs = 250.0f;

    // Packet loss percentage threshold above which a connection is deemed unstable
    UPROPERTY(EditAnywhere, Config, Category="Network Diagnostics", meta=(ClampMin="1.0", ClampMax="50.0"))
    float UnstablePacketLossThreshold = 3.0f;

    // Enable detailed network logging
    UPROPERTY(EditAnywhere, Config, Category="Network Diagnostics")
    bool bEnableVerboseNetLogging = false;

    // Default tokens refilled per second for RPC rate limiting
    UPROPERTY(EditAnywhere, Config, Category="Security & Rate Limiting", meta=(ClampMin="1.0", ClampMax="200.0"))
    float DefaultRPCRefillRatePerSec = 25.0f;

    // Maximum token bucket capacity for RPC burst handling
    UPROPERTY(EditAnywhere, Config, Category="Security & Rate Limiting", meta=(ClampMin="5", ClampMax="500"))
    int32 DefaultRPCTokenBucketCapacity = 40;

    // When true, MP_VALIDATE_OR_KICK actively kicks offending clients on validation failure. Default is false (log warning only).
    UPROPERTY(EditAnywhere, Config, Category="Security & Rate Limiting")
    bool bKickOnValidationFailure = false;

    // Enable CPU profiler events for MP_NET_SCOPE in non-shipping builds
    UPROPERTY(EditAnywhere, Config, Category="Profiling & Diagnostics")
    bool bEnableCPUScopes = true;

    // Helper singleton getter
    static const UMPUtilitySettings* Get() { return GetDefault<UMPUtilitySettings>(); }

    // Default duration in seconds to preserve actor transforms for lag compensation hit verification
    UPROPERTY(EditAnywhere, Config, Category="Lag Compensation", meta=(ClampMin="0.1", ClampMax="3.0"))
    float LagCompensationBufferSeconds = 1.0f;

    // Near distance for Net LOD (actors closer than this run at MaxUpdateFrequency)
    UPROPERTY(EditAnywhere, Config, Category="Bandwidth & Net LOD", meta=(ClampMin="100.0"))
    float NetLODNearDistance = 1500.0f;

    // Far distance for Net LOD (actors farther than this run at MinUpdateFrequency)
    UPROPERTY(EditAnywhere, Config, Category="Bandwidth & Net LOD", meta=(ClampMin="500.0"))
    float NetLODFarDistance = 6000.0f;

    // Maximum NetUpdateFrequency for close actors
    UPROPERTY(EditAnywhere, Config, Category="Bandwidth & Net LOD", meta=(ClampMin="5.0", ClampMax="120.0"))
    float NetLODMaxFrequency = 30.0f;

    // Minimum NetUpdateFrequency for distant actors
    UPROPERTY(EditAnywhere, Config, Category="Bandwidth & Net LOD", meta=(ClampMin="0.5", ClampMax="10.0"))
    float NetLODMinFrequency = 2.0f;
};

