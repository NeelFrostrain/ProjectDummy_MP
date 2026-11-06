#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Stats/Stats.h"

MPUTILITYRUNTIME_API DECLARE_LOG_CATEGORY_EXTERN(LogMPUtility, Log, All);

#if STATS
DECLARE_STATS_GROUP(TEXT("MultiplayerUtility"), STATGROUP_MPUtility, STATCAT_Advanced);
DECLARE_CYCLE_STAT_EXTERN(TEXT("MP RateLimit Evaluation"), STAT_MP_RateLimit, STATGROUP_MPUtility, MPUTILITYRUNTIME_API);
DECLARE_CYCLE_STAT_EXTERN(TEXT("MP Rewind Interpolation"), STAT_MP_Rewind, STATGROUP_MPUtility, MPUTILITYRUNTIME_API);
DECLARE_CYCLE_STAT_EXTERN(TEXT("MP Ping Sampling"), STAT_MP_PingSample, STATGROUP_MPUtility, MPUTILITYRUNTIME_API);
#endif

class FMPUtilityRuntimeModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};

