#include "Types/MPNetPresetAsset.h"
#include "GameFramework/Actor.h"
#include "MPUtilityRuntimeModule.h"

void UMPNetPresetAsset::ApplyToActor(AActor* TargetActor) const
{
    if (!IsValid(TargetActor))
    {
        return;
    }

    if (!TargetActor->HasAuthority())
    {
        UE_LOG(LogMPUtility, Warning, TEXT("[UMPNetPresetAsset::ApplyToActor] Called without authority on actor '%s'!"), *TargetActor->GetName());
        return;
    }

    TargetActor->SetReplicates(true);
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
    TargetActor->SetNetUpdateFrequency(NetUpdateFrequency);
    TargetActor->SetMinNetUpdateFrequency(MinNetUpdateFrequency);
    TargetActor->SetNetCullDistanceSquared(NetCullDistanceSquared);
#else
    TargetActor->NetUpdateFrequency = NetUpdateFrequency;
    TargetActor->MinNetUpdateFrequency = MinNetUpdateFrequency;
    TargetActor->NetCullDistanceSquared = NetCullDistanceSquared;
#endif
    TargetActor->NetPriority = NetPriority;
    TargetActor->SetNetDormancy(NetDormancy);
    TargetActor->bAlwaysRelevant = bAlwaysRelevant;
    TargetActor->bOnlyRelevantToOwner = bOnlyRelevantToOwner;
}
