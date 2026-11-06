#include "FlowControl/MPLatentActions.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

class FMPDelayUntilValidAction : public FPendingLatentAction
{
public:
    TWeakObjectPtr<const AActor> TargetActorPtr;
    float RemainingTime;
    EMPDelayReplicationResult& OutResult;
    FName ExecutionFunction;
    int32 OutputLink;
    FWeakObjectPtr CallbackTarget;

    FMPDelayUntilValidAction(const AActor* InTarget, float InTimeout, EMPDelayReplicationResult& InResult, const FLatentActionInfo& LatentInfo)
        : TargetActorPtr(InTarget)
        , RemainingTime(FMath::Max(0.05f, InTimeout))
        , OutResult(InResult)
        , ExecutionFunction(LatentInfo.ExecutionFunction)
        , OutputLink(LatentInfo.Linkage)
        , CallbackTarget(LatentInfo.CallbackTarget)
    {
    }

    virtual void UpdateOperation(FLatentResponse& Response) override
    {
        if (TargetActorPtr.IsValid())
        {
            OutResult = EMPDelayReplicationResult::Success;
            Response.FinishAndTriggerIf(true, ExecutionFunction, OutputLink, CallbackTarget);
            return;
        }

        RemainingTime -= Response.ElapsedTime();
        if (RemainingTime <= 0.0f)
        {
            OutResult = EMPDelayReplicationResult::TimedOut;
            Response.FinishAndTriggerIf(true, ExecutionFunction, OutputLink, CallbackTarget);
            return;
        }
    }
};

void UMPLatentActions::DelayUntilReplicated(
    const UObject* WorldContextObject,
    const AActor* TargetActor,
    float TimeoutSeconds,
    EMPDelayReplicationResult& Result,
    FLatentActionInfo LatentInfo)
{
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!World)
    {
        Result = EMPDelayReplicationResult::TimedOut;
        return;
    }

    FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
    if (LatentActionManager.FindExistingAction<FMPDelayUntilValidAction>(LatentInfo.CallbackTarget, LatentInfo.UUID) == nullptr)
    {
        LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, new FMPDelayUntilValidAction(TargetActor, TimeoutSeconds, Result, LatentInfo));
    }
}

class FMPWaitServerSecondsAction : public FPendingLatentAction
{
public:
    float RemainingTime;
    FName ExecutionFunction;
    int32 OutputLink;
    FWeakObjectPtr CallbackTarget;

    FMPWaitServerSecondsAction(float InDuration, const FLatentActionInfo& LatentInfo)
        : RemainingTime(FMath::Max(0.0f, InDuration))
        , ExecutionFunction(LatentInfo.ExecutionFunction)
        , OutputLink(LatentInfo.Linkage)
        , CallbackTarget(LatentInfo.CallbackTarget)
    {
    }

    virtual void UpdateOperation(FLatentResponse& Response) override
    {
        RemainingTime -= Response.ElapsedTime();
        Response.FinishAndTriggerIf(RemainingTime <= 0.0f, ExecutionFunction, OutputLink, CallbackTarget);
    }
};

void UMPLatentActions::WaitServerSeconds(
    const UObject* WorldContextObject,
    float DurationSeconds,
    FLatentActionInfo LatentInfo)
{
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!World)
    {
        return;
    }

    FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
    if (LatentActionManager.FindExistingAction<FMPWaitServerSecondsAction>(LatentInfo.CallbackTarget, LatentInfo.UUID) == nullptr)
    {
        LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, new FMPWaitServerSecondsAction(DurationSeconds, LatentInfo));
    }
}


