#pragma once

#include "CoreMinimal.h"
#include "MPNetTypes.generated.h"

class APlayerState;
class APlayerController;

/**
 * Reason codes for authoritative network validation checks.
 */
UENUM(BlueprintType)
enum class EMPValidationReason : uint8
{
    Valid UMETA(DisplayName="Valid (Passed All Checks)"),
    Failed_NaN UMETA(DisplayName="Failed (Contains NaN or Infinite Values)"),
    Failed_OutOfRange UMETA(DisplayName="Failed (Value Exceeds Allowed Range)"),
    Failed_SpeedLimit UMETA(DisplayName="Failed (Movement Exceeds Max Speed)"),
    Failed_TeleportLimit UMETA(DisplayName="Failed (Movement Exceeds Teleport Distance)"),
    Failed_RewindWindow UMETA(DisplayName="Failed (Timestamp Outside Max Rewind Window)"),
    Failed_InvalidEnum UMETA(DisplayName="Failed (Enum Value Out of Valid Range)"),
    Failed_BlockedPath UMETA(DisplayName="Failed (Path Blocked by Geometry)"),
    Failed_InvalidName UMETA(DisplayName="Failed (Player Name Violates Sanity Rules)"),
    Failed_InvalidWorld UMETA(DisplayName="Failed (Invalid World Context Object)")
};

/**
 * Result structure returned by authoritative server validation nodes.
 */
USTRUCT(BlueprintType)
struct MPUTILITYRUNTIME_API FMPValidationResult
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Multiplayer Utility|Validation")
    bool bIsValid = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Multiplayer Utility|Validation")
    EMPValidationReason Reason = EMPValidationReason::Valid;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Multiplayer Utility|Validation")
    FString ErrorMessage;

    static FMPValidationResult Success()
    {
        return FMPValidationResult();
    }

    static FMPValidationResult Failure(EMPValidationReason InReason, const FString& InError = FString())
    {
        FMPValidationResult Res;
        Res.bIsValid = false;
        Res.Reason = InReason;
        Res.ErrorMessage = InError;
        return Res;
    }
};

/**
 * Dynamic delegate invoked by flow-control delay and tick scheduling nodes.
 */
DECLARE_DYNAMIC_DELEGATE(FMPTimerCallback);

/**
 * Execution branch for actor local network roles.
 */
UENUM(BlueprintType)
enum class EMPNetRoleBranch : uint8
{
    Authority UMETA(DisplayName="Authority (Server)"),
    Autonomous UMETA(DisplayName="Autonomous Proxy (Local Owner Client)"),
    Simulated UMETA(DisplayName="Simulated Proxy (Remote Client Puppet)")
};

/**
 * Execution branch for world network modes.
 */
UENUM(BlueprintType)
enum class EMPNetModeBranch : uint8
{
    Standalone UMETA(DisplayName="Standalone (Single-Player / Offline)"),
    DedicatedServer UMETA(DisplayName="Dedicated Server"),
    ListenServer UMETA(DisplayName="Listen Server (Host Player)"),
    Client UMETA(DisplayName="Client (Remote Player)")
};

/**
 * Execution branch for local player input possession / driving status.
 */
UENUM(BlueprintType)
enum class EMPLocalControlBranch : uint8
{
    LocallyControlled UMETA(DisplayName="Locally Controlled"),
    RemoteOrServer UMETA(DisplayName="Remote / Server")
};

/**
 * Execution branch for network authority check.
 */
UENUM(BlueprintType)
enum class EMPAuthorityBranch : uint8
{
    Authority UMETA(DisplayName="Authority (Server)"),
    Remote UMETA(DisplayName="Remote (Client)")
};

/**
 * Execution branch for conditional network gates.
 */
UENUM(BlueprintType)
enum class EMPGateExec : uint8
{
    Open UMETA(DisplayName="Open (Allowed)"),
    Closed UMETA(DisplayName="Closed (Blocked)")
};

/**
 * Diagnostic reasons for RPC transmission validation.
 */
UENUM(BlueprintType)
enum class EMPRPCSafetyReason : uint8
{
    Safe UMETA(DisplayName="Safe"),
    NullActor UMETA(DisplayName="Actor is Null or Pending Kill"),
    NotReplicated UMETA(DisplayName="Actor Does Not Replicate"),
    NoNetConnection UMETA(DisplayName="No Owning NetConnection"),
    ActorDormant UMETA(DisplayName="Actor is Dormant (NetDormancy Active)"),
    ServerRPC_NoOwner UMETA(DisplayName="Server RPC Rejected: No Autonomous Owner"),
    ClientRPC_NoOwningConnection UMETA(DisplayName="Client RPC Rejected: No Connection to Target Client"),
    ActorNotRelevant UMETA(DisplayName="Actor Not Relevant to Target Connection")
};


/**
 * Interface implemented by Pawns, Characters, or PlayerStates to expose their alive/dead status
 * to multiplayer query functions without tight coupling to specific game classes.
 */
UINTERFACE(Blueprintable)
class MPUTILITYRUNTIME_API UMPAliveInterface : public UInterface
{
    GENERATED_BODY()
};

class MPUTILITYRUNTIME_API IMPAliveInterface
{
    GENERATED_BODY()

public:
    /**
     * Returns true if the entity is alive, false if dead/downed/eliminated.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Multiplayer Utility|Player")
    bool IsAlive() const;
};


/**
 * Quality rating for a player network connection based on latency, packet loss, and jitter.
 */
UENUM(BlueprintType)
enum class EMPConnectionQuality : uint8
{
    Excellent UMETA(DisplayName="Excellent (<50ms, 0% loss)"),
    Good UMETA(DisplayName="Good (<100ms, <1% loss)"),
    Fair UMETA(DisplayName="Fair (<150ms, <2% loss)"),
    Poor UMETA(DisplayName="Poor (<250ms, <5% loss)"),
    Critical UMETA(DisplayName="Critical (High ping or packet loss)"),
    Unknown UMETA(DisplayName="Unknown (Standalone / Disconnected)")
};

/**
 * Trend direction of player ping over a historical time window.
 */
UENUM(BlueprintType)
enum class EMPPingTrend : uint8
{
    Stable UMETA(DisplayName="Stable"),
    Improving UMETA(DisplayName="Improving (Decreasing ping)"),
    Worsening UMETA(DisplayName="Worsening (Increasing ping)"),
    Unknown UMETA(DisplayName="Unknown (Insufficient samples)")
};

/**
 * Result code for scheduled or delayed multiplayer actions.
 */
UENUM(BlueprintType)
enum class EMPDelayedActionResult : uint8
{
    Scheduled UMETA(DisplayName="Action Scheduled"),
    Executed UMETA(DisplayName="Action Executed"),
    Cancelled UMETA(DisplayName="Action Cancelled"),
    FailedInvalidWorld UMETA(DisplayName="Failed (Invalid World Context)")
};

/**
 * Interface implemented by networked entities to expose their logical player owner.
 * Allows resolving the authoritative PlayerState for spawned pets, vehicles, or projectiles.
 */
UINTERFACE(Blueprintable)
class MPUTILITYRUNTIME_API UMPOwnershipInterface : public UInterface
{
    GENERATED_BODY()
};

class MPUTILITYRUNTIME_API IMPOwnershipInterface
{
    GENERATED_BODY()

public:
    /**
     * Returns the logical owner's PlayerState for this networked entity.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Multiplayer Utility|Ownership")
    APlayerState* GetLogicalOwnerPlayerState() const;
};

/**
 * Interface implemented by actors or components to expose team affiliation.
 */
UINTERFACE(Blueprintable)
class MPUTILITYRUNTIME_API UMPTeamInterface : public UInterface
{
    GENERATED_BODY()
};

class MPUTILITYRUNTIME_API IMPTeamInterface
{
    GENERATED_BODY()

public:
    /**
     * Returns the current team identifier for this entity.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Multiplayer Utility|Team")
    int32 GetTeamId() const;

    /**
     * Sets the team identifier for this entity.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Multiplayer Utility|Team")
    void SetTeamId(int32 NewTeamId);
};

/**
 * Interface for actors that implement custom network relevancy rules.
 */
UINTERFACE(Blueprintable)
class MPUTILITYRUNTIME_API UMPRelevancyInterface : public UInterface
{
    GENERATED_BODY()
};

class MPUTILITYRUNTIME_API IMPRelevancyInterface
{
    GENERATED_BODY()

public:
    /**
     * Evaluates custom network relevancy rule for a target player connection.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Multiplayer Utility|Relevancy")
    bool IsActorRelevantToPlayer(const APlayerController* PC) const;
};

/**
 * Interface allowing networked entities to supply custom debug text to replication HUDs.
 */
UINTERFACE(Blueprintable)
class MPUTILITYRUNTIME_API UMPNetDebugInterface : public UInterface
{
    GENERATED_BODY()
};

class MPUTILITYRUNTIME_API IMPNetDebugInterface
{
    GENERATED_BODY()

public:
    /**
     * Appends diagnostic debug lines describing current network state.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Multiplayer Utility|Diagnostics")
    void GetNetDebugStrings(TArray<FString>& OutDebugStrings) const;
};

