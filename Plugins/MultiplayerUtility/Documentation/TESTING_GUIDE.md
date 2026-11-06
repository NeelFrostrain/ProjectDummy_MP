# Multiplayer Utility — Testing & Automation Guide

## 1. Running Automation Tests

The plugin contains **37 comprehensive automation test suites** in module `MPUtilityTests`.

### Run via Unreal Editor:
1. Open Unreal Editor.
2. Open **Tools** $\rightarrow$ **Test Automation**.
3. In the Test Automation window, check the box next to **MultiplayerUtility**.
4. Click **Start Tests**.

### Run via Command Line:
```powershell
& "<UE_ROOT>\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "<ProjectRoot>\ProjectDummy_MP.uproject" -ExecCmds="Automation RunTests MultiplayerUtility; Quit" -testexit="Automation Test Queue Empty" -unattended -nopause -nullrhi -nosound -log
```

---

## 2. In-Engine Stress Testing

To test how your server handles thousands of concurrent RPCs and rate limiters:

```cpp
#include "MPNetworkStressTester.h"

FMPNetworkStressTestConfig Config;
Config.ActorCount = 100;
Config.RPCCallsPerSecond = 50;

FMPNetworkStressTestResult Result = UMPNetworkStressTester::RunInEngineStressTest(Config);
UE_LOG(LogNet, Log, TEXT("%s"), *Result.SummaryReport);
```
