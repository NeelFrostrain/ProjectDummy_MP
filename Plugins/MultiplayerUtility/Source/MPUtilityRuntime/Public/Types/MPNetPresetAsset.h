#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "MPNetPresetAsset.generated.h"

/**
 * Data asset defining reusable network replication presets for actors.
 * Apply authoritatively to actors in batches or at spawn time.
 */
UCLASS(BlueprintType)
class MPUTILITYRUNTIME_API UMPNetPresetAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Multiplayer Utility|Preset", meta=(ClampMin="0.1", ClampMax="120.0"))
    float NetUpdateFrequency = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Multiplayer Utility|Preset", meta=(ClampMin="0.1", ClampMax="120.0"))
    float MinNetUpdateFrequency = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Multiplayer Utility|Preset", meta=(ClampMin="0.0"))
    float NetCullDistanceSquared = 225000000.0f; // 15,000 units squared

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Multiplayer Utility|Preset", meta=(ClampMin="0.0", ClampMax="10.0"))
    float NetPriority = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Multiplayer Utility|Preset")
    TEnumAsByte<ENetDormancy> NetDormancy = DORM_DormantAll;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Multiplayer Utility|Preset")
    bool bAlwaysRelevant = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Multiplayer Utility|Preset")
    bool bOnlyRelevantToOwner = false;

    /**
     * Applies this preset's replication configuration to the target actor.
     * Must be called on the server with authority.
     */
    UFUNCTION(BlueprintCallable, Category="Multiplayer Utility|Preset")
    void ApplyToActor(AActor* TargetActor) const;
};

