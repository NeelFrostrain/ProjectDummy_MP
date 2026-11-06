#include "MPUtilityRuntimeModule.h"
#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY(LogMPUtility);

#if STATS
DEFINE_STAT(STAT_MP_RateLimit);
DEFINE_STAT(STAT_MP_Rewind);
DEFINE_STAT(STAT_MP_PingSample);
#endif

// =============================================================================
// CONSOLE VARIABLES (CVars - Batch 7)
// =============================================================================

static TAutoConsoleVariable<int32> CVarMPDebugOverlay(
    TEXT("mp.Debug.Overlay"),
    0,
    TEXT("Enables or disables on-screen multiplayer debug overlays (0=off, 1=on)."),
    ECVF_Default);

static TAutoConsoleVariable<int32> CVarMPDebugLogOnce(
    TEXT("mp.Debug.LogOnce"),
    1,
    TEXT("Enables or disables MP log-once warning throttling (1=enabled, 0=log every occurrence)."),
    ECVF_Default);

static TAutoConsoleVariable<int32> CVarMPRateLimitEnabled(
    TEXT("mp.RateLimit.Enabled"),
    1,
    TEXT("Global toggle for multiplayer rate limiters and spam prevention (1=enabled, 0=disabled)."),
    ECVF_Default);

static TAutoConsoleVariable<int32> CVarMPValidationKickOnFail(
    TEXT("mp.Validation.KickOnFail"),
    0,
    TEXT("If 1, kicks clients failing security validation checks; if 0, only rejects/logs (0=reject only, 1=kick)."),
    ECVF_Default);

static TAutoConsoleVariable<float> CVarMPNetPingSampleInterval(
    TEXT("mp.Net.PingSampleInterval"),
    0.25f,
    TEXT("Interval in seconds between ping history sampling for moving average and trend calculations."),
    ECVF_Default);

#define LOCTEXT_NAMESPACE "FMPUtilityRuntimeModule"

void FMPUtilityRuntimeModule::StartupModule()
{
}

void FMPUtilityRuntimeModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMPUtilityRuntimeModule, MPUtilityRuntime)

