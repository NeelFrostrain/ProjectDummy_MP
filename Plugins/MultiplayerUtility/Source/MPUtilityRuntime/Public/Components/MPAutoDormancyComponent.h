#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "MPAutoDormancyComponent.generated.h"

/**
 * Automatically transitions an actor into network dormancy after a configurable idle duration.
 * Reduces server CPU evaluation and network bandwidth for static or settled entities.
 * Runs on: Server Only.
 */
UCLASS(ClassGroup=(MultiplayerUtility), meta=(BlueprintSpawnableComponent))
class MPUTILITYRUNTIME_API UMPAutoDormancyComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UMPAutoDormancyComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Multiplayer Utility|Dormancy", meta=(ClampMin="1.0", ClampMax="300.0"))
    float IdleSecondsBeforeDormancy = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Multiplayer Utility|Dormancy")
    TEnumAsByte<ENetDormancy> TargetDormancy = DORM_DormantAll;

    /**
     * Notifies the component of active interaction or movement, resetting the idle countdown.
     */
    UFUNCTION(BlueprintCallable, Category="Multiplayer Utility|Dormancy")
    void NotifyActivity();

    /**
     * Immediately wakes the actor up from dormancy and flushes network state.
     */
    UFUNCTION(BlueprintCallable, Category="Multiplayer Utility|Dormancy")
    void WakeFromDormancy();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void CheckIdleAndSleep();

    FTimerHandle TimerHandle_Dormancy;
    FVector LastRecordedLocation = FVector::ZeroVector;
};

