#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MPNetDebugTextComponent.generated.h"

/**
 * Diagnostic component that periodically compiles replication debug information for this actor.
 * Stripped in shipping builds; uses fixed-interval world timers instead of Tick.
 */
UCLASS(ClassGroup=(MultiplayerUtility), meta=(BlueprintSpawnableComponent))
class MPUTILITYRUNTIME_API UMPNetDebugTextComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UMPNetDebugTextComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Multiplayer Utility|Debug", meta=(ClampMin="0.1", ClampMax="10.0"))
    float RefreshIntervalSeconds = 0.5f;

    UPROPERTY(BlueprintReadOnly, Category="Multiplayer Utility|Debug")
    FString CachedDebugText;

    UFUNCTION(BlueprintCallable, Category="Multiplayer Utility|Debug")
    void ForceRefreshDebugText();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    FTimerHandle TimerHandle_Refresh;
};

