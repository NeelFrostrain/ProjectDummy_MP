# Multiplayer Utility Plugin

A production-grade networking library and utility toolkit for **Unreal Engine 5.3, 5.4, 5.5, and 5.6+** (compatible via `ENGINE_MINOR_VERSION` preprocessor guards).

The plugin wraps Unreal Engine's core replication, networking, and RPC systems into safe Blueprint nodes, execution flow-control splitters, latent synchronization gates, deterministic lag-compensation ring buffers, token-bucket RPC rate limiters, network time math, subobject replication helpers, team affiliation interfaces, authoritative server validation, RAII scoped guards, and automated unit test suites.

---

## 📑 Table of Contents
1. [Architecture Overview](#-architecture-overview)
2. [Installation & Setup](#-installation--setup)
3. [Complete API Reference Tables](#-complete-api-reference-tables)
   - [1. Network State & Role Queries](#1-network-state--role-queries)
   - [2. Actor Replication & Lifecycle Control](#2-actor-replication--lifecycle-control)
   - [3. Subobject & Component Replication](#3-subobject--component-replication)
   - [4. Movement & Physics Replication](#4-movement--physics-replication)
   - [5. Connection Quality & Network Diagnostics](#5-connection-quality--network-diagnostics)
   - [6. Player & Spatial Queries](#6-player--spatial-queries)
   - [7. Team Affiliation & Relevancy](#7-team-affiliation--relevancy)
   - [8. Authoritative Server Validation](#8-authoritative-server-validation)
   - [9. Component & Spawn Helpers](#9-component--spawn-helpers)
   - [10. Level Streaming & World Helpers](#10-level-streaming--world-helpers)
   - [11. Network Math, Quantization & Bitwise Helpers](#11-network-math-quantization--bitwise-helpers)
   - [12. Flow-Control Execution & Latent Nodes](#12-flow-control-execution--latent-nodes)
   - [13. Replicated Helper Structs](#13-replicated-helper-structs)
   - [14. Network Components & Presets](#14-network-components--presets)
4. [Settings Reference (`UMPUtilitySettings`)](#-settings-reference-umputilitysettings)
5. [Console Variables (CVars) Reference](#-console-variables-cvars-reference)
6. [Complete Macro Reference (`MPNetMacros.h`)](#-complete-macro-reference-mpnetmacrosh)
7. [Pure C++ Data Structures & RAII Scoped Guards](#-pure-c-data-structures--raii-scoped-guards)
8. [Production Gameplay Examples](#-production-gameplay-examples)
   - [Example 1: Dynamic Replication Frequency from Connection Quality](#example-1-dynamic-replication-frequency-from-connection-quality)
   - [Example 2: Client Transform Correction (`SnapOrSmoothTransform`)](#example-2-client-transform-correction-snadorsmoothtransform)
   - [Example 3: Server RPC Guard & Lag Compensated Weapon Fire](#example-3-server-rpc-guard--lag-compensated-weapon-fire)
   - [Example 4: Scoped Dormancy Wake on Inventory Modification](#example-4-scoped-dormancy-wake-on-inventory-modification)
   - [Example 5: Authoritative Server Movement Validation with Client Correction](#example-5-authoritative-server-movement-validation-with-client-correction)
9. [Common Multiplayer Mistakes & Solutions](#-common-multiplayer-mistakes--solutions)
10. [Known Limitations & Technical Boundaries](#-known-limitations--technical-boundaries)
11. [Engine Version Notes & Compatibility](#-engine-version-notes--compatibility)
12. [Automated QA & Unit Testing Guide](#-automated-qa--unit-testing-guide)
13. [Changelog & Version History](#-changelog--version-history)

---

## 📁 Architecture Overview

```
Plugins/MultiplayerUtility/
├── MultiplayerUtility.uplugin
├── README.md
├── Config/
│   └── BaseMultiplayerUtility.ini         (CoreRedirects and settings defaults)
├── Documentation/                         (Architecture, Blueprint, Replication, and API reference guides)
├── Scripts/
│   ├── place_nodes_in_blueprint.py        (Automated 27-category Blueprint Graph Generator & Clipboard exporter)
│   └── scan_duplicates.py                 (Anti-duplicate validation tool for CI/CD)
└── Source/
    ├── MPUtilityRuntime/                  (PreDefault - Core Gameplay, Wrappers, Subsystems & Math)
    │   ├── Async/
    │   │   └── MPAsyncNetworkActions.h/.cpp (Latent-like async proxies: WaitUntilPlayerState, WaitUntilActorReplicated)
    │   ├── Components/
    │   │   ├── MPNetDebugTextComponent.h/.cpp (Diagnostic floating text component, timer-driven, dev only)
    │   │   └── MPAutoDormancyComponent.h/.cpp (Automatic idle-to-dormancy sleep component)
    │   ├── FlowControl/
    │   │   ├── MPFlowControlLibrary.h/.cpp (Branch By Role/Mode/Authority with DefaultToSelf, Gate, Deferrals)
    │   │   └── MPLatentActions.h/.cpp      (Delay Until Replicated with DefaultToSelf, Wait Server Seconds)
    │   ├── Gameplay/
    │   │   ├── MPGameplayHelperLibrary.h/.cpp (Player/RPC/Dormancy helpers with DefaultToSelf)
    │   │   ├── MPHealthComponent.h/.cpp       (Server-authoritative health, shield, downed, revive)
    │   │   └── MPRPCGameplayComponent.h/.cpp  (Gameplay RPC broadcast & cosmetic dispatch)
    │   ├── Movement/
    │   │   └── MPCharacterMovementComponent.h/.cpp (Client prediction pipeline, sprint/dash/slide/climb)
    │   ├── Optimization/
    │   │   └── MPNetLODComponent.h/.cpp       (Dynamic spatial net frequency and LOD throttling)
    │   ├── Physics/
    │   │   └── MPPhysicsReplicationHelper.h/.cpp (Ragdoll handover, damping, transform validation)
    │   ├── Player/
    │   │   └── MPPlayerLifecycleLibrary.h/.cpp (Safe possession, respawning, spectator, cleanup)
    │   ├── Replication/
    │   │   ├── MPFastArrayTypes.h             (FFastArraySerializer reusable base templates)
    │   │   └── MPReplicationConditions.h/.cpp (Dynamic runtime replication condition overrides)
    │   ├── Requests/
    │   │   ├── MPNetworkRequestSubsystem.h/.cpp (WorldSubsystem client-server request tracking)
    │   │   ├── MPNetworkRequestComponent.h/.cpp (Bidirectional reliable request/response correlation)
    │   │   └── MPNetworkRequestTypes.h/.cpp     (Request, Response, GUID correlation definitions)
    │   ├── Security/
    │   │   └── MPServerSecurityLibrary.h/.cpp (Damage verification, timestamp drift, anti-exploit)
    │   ├── Subsystems/
    │   │   └── MPNetStatsSubsystem.h/.cpp  (UWorldSubsystem tracking RPC calls and network entity counts)
    │   ├── Types/
    │   │   ├── MPNetTypes.h               (Unified networking enums, team interface, relevancy interface)
    │   │   ├── MPNetPresetAsset.h/.cpp    (Reusable replication profile data asset)
    │   │   ├── MPPackedTypes.h/.cpp       (Packed floats, 8-bit/16-bit flags, quantized vector/rotator wrappers)
    │   │   ├── MPQuantization.h/.cpp      (Tolerance checks, bit manipulation, angle wrapping, vector packing)
    │   │   ├── MPRateLimiter.h/.cpp       (Deterministic token-bucket RPC limiter struct)
    │   │   ├── MPReplicatedCountdown.h/.cpp (Replicated countdown and match timers)
    │   │   ├── MPReplicatedDataTypes.h/.cpp (Counter, FlagSet, FastArray KeyValue, LogRing structs)
    │   │   ├── MPReplicatedSeed.h/.cpp    (Replicated seed struct with deterministic FRandomStream)
    │   │   ├── MPReplicatedStateMachine.h/.cpp (Network-synchronized gameplay tag state machines)
    │   │   ├── MPReplicatedTimer.h/.cpp   (Replicated timer and cooldown with client time evaluation)
    │   │   ├── MPRewindBuffer.h/.cpp      (Circular rewind history buffer with Quaternion SLERP & extrapolation)
    │   │   ├── MPScopedGuards.h           (RAII guards: Authority, NetTimer, RPCCounter, DormancyWake, CVarOverride)
    │   │   ├── MPTimeMath.h/.cpp          (NTP RTT & Clock Offset, EMA ping filter, RFC 3550 jitter, delta clamps)
    │   │   └── MPUrlParser.h/.cpp         (Travel URL option parsing and string building)
    │   ├── Wrappers/
    │   │   ├── MPActorNetworkWrapper.h/.cpp (Replication toggles, Dormancy, Cull distance, Update frequency, Owner)
    │   │   ├── MPComponentNetworkWrapper.h/.cpp (Subobject registration list, recursive replication, component roles)
    │   │   ├── MPMovementPhysicsWrapper.h/.cpp (Server physics impulse, replicated velocity, snap-or-smooth)
    │   │   ├── MPPawnControllerWrapper.h/.cpp (Safe possess, Ping ms/sec, input control queries)
    │   │   └── MPWorldSessionWrapper.h/.cpp (Level streaming status, seamless travel queries, map names)
    │   ├── MPNetFunctionLibrary.h/.cpp    (Spatial queries, connection quality, validation, team helpers, spawn helpers)
    │   ├── MPNetMacros.h                  (Authority guards, RPC validation macros, Section L & M advanced macros)
    │   ├── MPUtilityRuntimeModule.h/.cpp  (Module startup, CVar registrations, log category, stat groups)
    │   └── MPUtilitySettings.h/.cpp       (DeveloperSettings for ping thresholds, buffer sizes, logging flags)
    │
    ├── MPUtilityEditor/                   (Editor - Replication Auditing & Graph Generation)
    │   ├── Subsystems/
    │   │   └── MPEditorScannerSubsystem.h/.cpp (Audit replication, PIE emulation, generate Showcase Blueprint)
    │   └── MPUtilityEditorModule.h/.cpp   (Editor module lifecycle)
    │
    └── MPUtilityTests/                    (Default/Editor - Automated Testing Suite)
        ├── MPTestActors.h/.cpp            (Mock replicated actors and controllers for headless execution)
        ├── MPTestUtilities.h              (Deterministic test clocks and net mode overrides)
        ├── MPNetworkStressTester.h/.cpp   (Network load simulation and throughput benchmarking)
        ├── MPUtilityTestsModule.h/.cpp    (Test module interface)
        └── Specs/
            └── MPUtilityAutomationSpecs.cpp (36 automation test suites covering all runtime features)
```

---

## 🛠️ Installation & Setup

1. Copy the `MultiplayerUtility` folder into your project's `Plugins/` directory:
   ```
   <YourProject>/
   ├── Plugins/
   │   └── MultiplayerUtility/
   └── <YourProject>.uproject
   ```
2. In `<YourProject>.uproject`, ensure the plugin is enabled:
   ```json
   {
     "Name": "MultiplayerUtility",
     "Enabled": true
   }
   ```
3. In your game module's `<YourModule>.Build.cs`, add `MPUtilityRuntime` to `PublicDependencyModuleNames`:
   ```csharp
   PublicDependencyModuleNames.AddRange(new string[]
   {
       "Core",
       "CoreUObject",
       "Engine",
       "GameplayTags",
       "NetCore",
       "MPUtilityRuntime"
   });
   ```
4. Rebuild your project using Visual Studio, JetBrains Rider, or Unreal Editor Live Coding.

---

## 📖 Complete API Reference Tables

Every row below has been verified against the underlying C++ header declarations (`Source/MPUtilityRuntime/Public/`).

### 1. Network State & Role Queries
*Header: `MPNetFunctionLibrary.h` & `Wrappers/MPPawnControllerWrapper.h`*

| C++ Function | DisplayName | Category | Return Type | Parameters | Runs On | File:Line |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `IsServer` | `IsServer` | `Multiplayer Utility\|NetMode` | `bool` | `const UObject* WorldContextObject` | Client & Server | `MPNetFunctionLibrary.h:28` |
| `IsNetworkGame` | `IsNetworkGame` | `Multiplayer Utility\|NetMode` | `bool` | `const UObject* WorldContextObject` | Client & Server | `MPNetFunctionLibrary.h:35` |
| `IsStandalone` | `IsStandalone` | `Multiplayer Utility\|NetMode` | `bool` | `const UObject* WorldContextObject` | Client & Server | `MPNetFunctionLibrary.h:42` |
| `IsDedicatedServer` | `IsDedicatedServer` | `Multiplayer Utility\|NetMode` | `bool` | `const UObject* WorldContextObject` | Client & Server | `MPNetFunctionLibrary.h:49` |
| `IsListenServer` | `IsListenServer` | `Multiplayer Utility\|NetMode` | `bool` | `const UObject* WorldContextObject` | Client & Server | `MPNetFunctionLibrary.h:56` |
| `IsClient` | `IsClient` | `Multiplayer Utility\|NetMode` | `bool` | `const UObject* WorldContextObject` | Client & Server | `MPNetFunctionLibrary.h:63` |
| `HasAuthority` | `Has Network Authority (Safe)` | `Multiplayer Utility\|Role` | `bool` | `const AActor* TargetActor` | Client & Server | `MPNetFunctionLibrary.h:75` |
| `IsAutonomousProxy` | `IsAutonomousProxy` | `Multiplayer Utility\|Role` | `bool` | `const AActor* TargetActor` | Client & Server | `MPNetFunctionLibrary.h:82` |
| `IsSimulatedProxy` | `IsSimulatedProxy` | `Multiplayer Utility\|Role` | `bool` | `const AActor* TargetActor` | Client & Server | `MPNetFunctionLibrary.h:89` |
| `IsLocallyControlled` | `Is Locally Controlled (Safe)` | `Multiplayer Utility\|Role` | `bool` | `const AActor* TargetActor` | Client & Server | `MPNetFunctionLibrary.h:97` |
| `IsPlayerControlled` | `Is Player Controlled` | `Multiplayer Utility\|Pawn` | `bool` | `const APawn* TargetPawn` | Client & Server | `MPPawnControllerWrapper.h:22` |
| `IsBotControlled` | `Is Bot Controlled` | `Multiplayer Utility\|Pawn` | `bool` | `const APawn* TargetPawn` | Client & Server | `MPPawnControllerWrapper.h:28` |
| `IsLocalPlayerController` | `Is Local Controller` | `Multiplayer Utility\|Controller` | `bool` | `const APlayerController* Controller` | Client & Server | `MPPawnControllerWrapper.h:47` |

> [!NOTE]
> **Engine Node Duplication Resolution**:
> Unreal Engine provides native `AActor::HasAuthority` and `APawn::IsLocallyControlled`. The plugin's library nodes are explicitly named **`Has Network Authority (Safe)`** and **`Is Locally Controlled (Safe)`** (under `Multiplayer Utility|Role`). They accept a general `const AActor*`, gracefully handle null pointers, and support both Pawns and Controllers without casting errors.
> If upgrading from earlier versions that used `MPActorNetworkWrapper::HasAuthority` or `MPPawnControllerWrapper::IsLocallyControlled`, CoreRedirects in `Config/BaseMultiplayerUtility.ini` automatically redirect existing graphs.

---

### 2. Actor Replication & Lifecycle Control
*Headers: `Wrappers/MPActorNetworkWrapper.h` & `MPNetFunctionLibrary.h`*

| C++ Function | DisplayName | Category | Return Type | Parameters | Runs On | File:Line |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `IsRPCSafeToCall` | `Is RPC Safe To Call` | `Multiplayer Utility\|RPC` | `bool` | `const AActor* TargetActor, bool bIsServerRPC, EMPRPCSafetyReason& OutReason` | Client & Server | `MPNetFunctionLibrary.h:118` |
| `ExplainWhyRPCDropped` | `Explain Why RPC Dropped` | `Multiplayer Utility\|RPC` | `FString` | `const AActor* TargetActor, bool bIsServerRPC` | Client & Server | `MPNetFunctionLibrary.h:129` |
| `CanSendServerRPC` | `CanSendServerRPC` | `Multiplayer Utility\|RPC` | `bool` | `const AActor* TargetActor, FString& OutFailureReason` | Client Only | `MPNetFunctionLibrary.h:139` |
| `GetRPCOwnershipChain` | `GetRPCOwnershipChain` | `Multiplayer Utility\|RPC` | `bool` | `const AActor* TargetActor, TArray<FString>& OutOwnerNames` | Client & Server | `MPNetFunctionLibrary.h:149` |
| `SetReplicates` | `Set Replicates` | `Multiplayer Utility\|Actor` | `void` | `AActor* TargetActor, bool bReplicates` | Server Only | `MPActorNetworkWrapper.h:35` |
| `SetReplicateMovement` | `Set Replicate Movement` | `Multiplayer Utility\|Actor` | `void` | `AActor* TargetActor, bool bReplicateMovement` | Server Only | `MPActorNetworkWrapper.h:42` |
| `SetNetOwner` | `Set Net Owner` | `Multiplayer Utility\|Actor` | `void` | `AActor* TargetActor, AActor* NewOwner` | Server Only | `MPActorNetworkWrapper.h:49` |
| `GetTopLevelNetOwner` | `Get Top-Level Net Owner` | `Multiplayer Utility\|Actor` | `AActor*` | `const AActor* TargetActor` | Client & Server | `MPActorNetworkWrapper.h:56` |
| `GetLogicalOwnerSafe` | `Get Logical Owner Safe` | `Multiplayer Utility\|Actor` | `AActor*` | `const AActor* TargetActor` | Client & Server | `MPActorNetworkWrapper.h:64` |
| `SetNetDormancy` | `Set Net Dormancy` | `Multiplayer Utility\|Actor` | `void` | `AActor* TargetActor, ENetDormancy NewDormancy` | Server Only | `MPActorNetworkWrapper.h:71` |
| `FlushNetDormancy` | `Flush Net Dormancy` | `Multiplayer Utility\|Actor` | `void` | `AActor* TargetActor` | Server Only | `MPActorNetworkWrapper.h:78` |
| `ForceNetUpdate` | `Force Net Update` | `Multiplayer Utility\|Actor` | `void` | `AActor* TargetActor` | Server Only | `MPActorNetworkWrapper.h:85` |
| `SetNetUpdateFrequency` | `Set Net Update Frequency` | `Multiplayer Utility\|Actor` | `void` | `AActor* TargetActor, float NewFrequency` | Server Only | `MPActorNetworkWrapper.h:92` |
| `SetMinNetUpdateFrequency` | `Set Min Net Update Frequency` | `Multiplayer Utility\|Actor` | `void` | `AActor* TargetActor, float NewMinFrequency` | Server Only | `MPActorNetworkWrapper.h:99` |
| `SetNetCullDistance` | `Set Net Cull Distance` | `Multiplayer Utility\|Actor` | `void` | `AActor* TargetActor, float NewCullDistance` | Server Only | `MPActorNetworkWrapper.h:106` |
| `SetAlwaysRelevant` | `Set Always Relevant` | `Multiplayer Utility\|Actor` | `void` | `AActor* TargetActor, bool bAlwaysRelevant` | Server Only | `MPActorNetworkWrapper.h:113` |
| `SetOnlyRelevantToOwner` | `Set Only Relevant To Owner` | `Multiplayer Utility\|Actor` | `void` | `AActor* TargetActor, bool bOnlyRelevantToOwner` | Server Only | `MPActorNetworkWrapper.h:120` |

---

### 3. Subobject & Component Replication
*Header: `Wrappers/MPComponentNetworkWrapper.h`*

| C++ Function | DisplayName | Category | Return Type | Parameters | Runs On | File:Line |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `SetComponentReplicated` | `Set Component Replicated` | `Multiplayer Utility\|Component` | `void` | `UActorComponent* TargetComponent, bool bReplicates` | Server Only | `MPComponentNetworkWrapper.h:29` |
| `IsComponentReplicated` | `Is Component Replicated` | `Multiplayer Utility\|Component` | `bool` | `const UActorComponent* TargetComponent` | Client & Server | `MPComponentNetworkWrapper.h:36` |
| `IsComponentNetSimulating` | `Is Component Net Simulating` | `Multiplayer Utility\|Component` | `bool` | `const UActorComponent* TargetComponent` | Client & Server | `MPComponentNetworkWrapper.h:43` |
| `HasComponentAuthority` | `Has Component Authority` | `Multiplayer Utility\|Component` | `bool` | `const UActorComponent* TargetComponent` | Client & Server | `MPComponentNetworkWrapper.h:50` |
| `RegisterReplicatedSubobjectSafe` | `Register Replicated Subobject Safe` | `Multiplayer Utility\|Component` | `bool` | `UObject* Owner, UObject* SubObject, ELifetimeCondition NetCondition = COND_None` | Server Only | `MPComponentNetworkWrapper.h:69` |
| `UnregisterReplicatedSubobjectSafe`| `Unregister Replicated Subobject Safe` | `Multiplayer Utility\|Component` | `bool` | `UObject* Owner, UObject* SubObject` | Server Only | `MPComponentNetworkWrapper.h:80` |
| `IsSubobjectReplicated` | `Is Subobject Replicated` | `Multiplayer Utility\|Component` | `bool` | `const UObject* Owner, const UObject* SubObject` | Client & Server | `MPComponentNetworkWrapper.h:91` |
| `GetReplicatedComponentsOfActor` | `Get Replicated Components Of Actor` | `Multiplayer Utility\|Component` | `TArray<UActorComponent*>` | `const AActor* TargetActor` | Client & Server | `MPComponentNetworkWrapper.h:101` |
| `SetComponentReplicatedRecursive` | `Set Component Replicated Recursive` | `Multiplayer Utility\|Component` | `void` | `UActorComponent* TargetComponent, bool bReplicates` | Server Only | `MPComponentNetworkWrapper.h:111` |
| `PrintReplicatedSubobjects` | `Print Replicated Subobjects` | `Multiplayer Utility\|Component` | `TArray<FString>` | `const AActor* TargetActor, bool bPrintToLog = true` | Client & Server | `MPComponentNetworkWrapper.h:122` |

---

### 4. Movement & Physics Replication
*Header: `Wrappers/MPMovementPhysicsWrapper.h`*

| C++ Function | DisplayName | Category | Return Type | Parameters | Runs On | File:Line |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `SetSimulatePhysicsServerOnly` | `Set Simulate Physics Server Only` | `Multiplayer Utility\|Physics` | `void` | `UPrimitiveComponent* TargetComponent, bool bSimulate` | Server Only | `MPMovementPhysicsWrapper.h:42` |
| `ApplyImpulseServerOnly` | `Apply Impulse Server Only` | `Multiplayer Utility\|Physics` | `void` | `UPrimitiveComponent* TargetComponent, const FVector& Impulse, FName BoneName = NAME_None, bool bVelChange = false` | Server Only | `MPMovementPhysicsWrapper.h:50` |
| `GetReplicatedVelocitySafe` | `Get Replicated Velocity Safe` | `Multiplayer Utility\|Movement` | `FVector` | `const AActor* TargetActor` | Client & Server | `MPMovementPhysicsWrapper.h:58` |
| `IsPhysicsReplicated` | `Is Physics Replicated` | `Multiplayer Utility\|Physics` | `bool` | `const AActor* TargetActor` | Client & Server | `MPMovementPhysicsWrapper.h:65` |
| `SnapOrSmoothTransform` | `Snap Or Smooth Transform` | `Multiplayer Utility\|Movement` | `FTransform` | `const FTransform& CurrentTransform, const FTransform& TargetTransform, float SnapDistanceThreshold = 150.0f, float LerpAlpha = 0.25f` | Client & Server | `MPMovementPhysicsWrapper.h:73` |
| `GetMovementErrorDistance` | `Get Movement Error Distance` | `Multiplayer Utility\|Movement` | `float` | `const FVector& ClientPosition, const FVector& ServerPosition` | Client & Server | `MPMovementPhysicsWrapper.h:80` |
| `IsCharacterMovingAuthoritatively` | `Is Character Moving Authoritatively` | `Multiplayer Utility\|Movement` | `bool` | `const ACharacter* TargetCharacter` | Server (Client returns false) | `MPMovementPhysicsWrapper.h:87` |
| `GetNetworkSmoothingMode` | `Get Network Smoothing Mode` | `Multiplayer Utility\|Movement` | `ENetworkSmoothingMode` | `const ACharacter* TargetCharacter` | Client & Server | `MPMovementPhysicsWrapper.h:94` |
| `SetNetworkSmoothingModeSafe` | `Set Network Smoothing Mode Safe` | `Multiplayer Utility\|Movement` | `void` | `ACharacter* TargetCharacter, ENetworkSmoothingMode NewMode` | Client & Server | `MPMovementPhysicsWrapper.h:102` |

---

### 5. Connection Quality & Network Diagnostics
*Headers: `MPNetFunctionLibrary.h` & `Wrappers/MPPawnControllerWrapper.h`*

| C++ Function | DisplayName | Category | Return Type | Parameters | Runs On | File:Line |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `GetConnectionQualityEnum` | `Get Connection Quality` | `Multiplayer Utility\|Connection` | `EMPConnectionQuality` | `const APlayerController* PC` | Client & Server | `MPNetFunctionLibrary.h:172` |
| `IsConnectionUnstable` | `Is Connection Unstable` | `Multiplayer Utility\|Connection` | `bool` | `const APlayerController* PC` | Client & Server | `MPNetFunctionLibrary.h:180` |
| `GetAveragePingOverNSeconds` | `Get Average Ping Over N Seconds` | `Multiplayer Utility\|Connection` | `float` | `const APlayerController* PC, float WindowSeconds = 5.0f` | Client & Server | `MPNetFunctionLibrary.h:189` |
| `GetPingTrend` | `Get Ping Trend` | `Multiplayer Utility\|Connection` | `EMPPingTrend` | `const APlayerController* PC, float WindowSeconds = 10.0f` | Client & Server | `MPNetFunctionLibrary.h:197` |
| `GetPacketLossPercentSafe` | `Get Packet Loss Percent Safe` | `Multiplayer Utility\|Connection` | `float` | `const APlayerController* PC, float& OutInLossPercent, float& OutOutLossPercent` | Client & Server | `MPNetFunctionLibrary.h:205` |
| `ShouldReduceNetworkLoad` | `Should Reduce Network Load` | `Multiplayer Utility\|Connection` | `bool` | `const APlayerController* PC` | Client & Server | `MPNetFunctionLibrary.h:213` |
| `EstimateBandwidthPerPlayer` | `Estimate Bandwidth Per Player` | `Multiplayer Utility\|Connection` | `float` | `const APlayerController* PC` | Server (Client fallback) | `MPNetFunctionLibrary.h:222` |
| `GetPlayerIPAddress` | `GetPlayerIPAddress` | `Multiplayer Utility\|Connection` | `FString` | `const APlayerController* PC` | Server Only | `MPNetFunctionLibrary.h:229` |
| `GetPlayerUniqueIdString` | `GetPlayerUniqueIdString` | `Multiplayer Utility\|Connection` | `FString` | `const APlayerController* PC` | Client & Server | `MPNetFunctionLibrary.h:236` |
| `GetPlayerPingMs` | `Get Player Ping (ms)` | `Multiplayer Utility\|Connection` | `float` | `const APlayerController* PC` | Client & Server | `MPNetFunctionLibrary.h:157` |
| `GetPlayerPingSeconds` | `Get Player Ping (Seconds)` | `Multiplayer Utility\|Controller` | `float` | `const APlayerController* Controller` | Client & Server | `MPPawnControllerWrapper.h:53` |

---

### 6. Player & Spatial Queries
*Header: `MPNetFunctionLibrary.h`*

| C++ Function | DisplayName | Category | Return Type | Parameters | Runs On | File:Line |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `GetClosestPlayerPawn` | `Get Closest Player Pawn` | `Multiplayer Utility\|Spatial` | `APawn*` | `const UObject* WorldContextObject, const FVector& Origin, float MaxRadius = 0.0f` | Client & Server | `MPNetFunctionLibrary.h:276` |
| `GetPlayersInSphere` | `Get Players In Sphere` | `Multiplayer Utility\|Spatial` | `TArray<APawn*>` | `const UObject* WorldContextObject, const FVector& Center, float Radius` | Client & Server | `MPNetFunctionLibrary.h:286` |
| `GetAllPlayerPawns` | `Get All Player Pawns` | `Multiplayer Utility\|Spatial` | `TArray<APawn*>` | `const UObject* WorldContextObject` | Client & Server | `MPNetFunctionLibrary.h:293` |
| `GetAllPlayerControllers` | `Get All Player Controllers` | `Multiplayer Utility\|Spatial` | `TArray<APlayerController*>` | `const UObject* WorldContextObject` | Client & Server | `MPNetFunctionLibrary.h:300` |
| `GetAllRemotePlayers` | `Get All Remote Players` | `Multiplayer Utility\|Spatial` | `TArray<APawn*>` | `const UObject* WorldContextObject` | Client & Server | `MPNetFunctionLibrary.h:307` |
| `GetDistanceToNearestPlayer` | `Get Distance To Nearest Player` | `Multiplayer Utility\|Spatial` | `float` | `const UObject* WorldContextObject, const FVector& Origin, float MaxDistance = -1.0f` | Client & Server | `MPNetFunctionLibrary.h:320` |
| `IsAnyPlayerWithinRange` | `Is Any Player Within Range` | `Multiplayer Utility\|Spatial` | `bool` | `const UObject* WorldContextObject, const FVector& Origin, float Range` | Client & Server | `MPNetFunctionLibrary.h:328` |
| `GetPlayersSortedByDistance` | `Get Players Sorted By Distance` | `Multiplayer Utility\|Spatial` | `TArray<APawn*>` | `const UObject* WorldContextObject, const FVector& Origin, bool bAscending = true` | Client & Server | `MPNetFunctionLibrary.h:335` |
| `GetFurthestPlayer` | `Get Furthest Player` | `Multiplayer Utility\|Spatial` | `APawn*` | `const UObject* WorldContextObject, const FVector& Origin, float MaxDistance = -1.0f` | Client & Server | `MPNetFunctionLibrary.h:343` |
| `GetPlayersInCone` | `Get Players In Cone` | `Multiplayer Utility\|Spatial` | `TArray<APawn*>` | `const UObject* WorldContextObject, const FVector& ConeOrigin, const FVector& ConeDirection, float HalfAngleDegrees, float Range` | Client & Server | `MPNetFunctionLibrary.h:350` |
| `GetPlayersInRadiusWithLineOfSight` | `Get Players In Radius With Line Of Sight` | `Multiplayer Utility\|Spatial` | `TArray<APawn*>` | `const UObject* WorldContextObject, const FVector& Center, float Radius, ECollisionChannel TraceChannel = ECC_Visibility, AActor* IgnoreActor = nullptr` | Server Only | `MPNetFunctionLibrary.h:359` |
| `GetRandomPlayer` | `Get Random Player` | `Multiplayer Utility\|Spatial` | `APawn*` | `const UObject* WorldContextObject, const TArray<APawn*>& ExcludeList` | Client & Server | `MPNetFunctionLibrary.h:367` |
| `GetAlivePlayersOnly` | `Get Alive Players Only` | `Multiplayer Utility\|Player` | `TArray<APawn*>` | `const UObject* WorldContextObject` | Client & Server | `MPNetFunctionLibrary.h:376` |
| `GetActorsWithTagReplicatedSafe` | `Get Actors With Tag Replicated Safe` | `Multiplayer Utility\|Names` | `TArray<AActor*>` | `const UObject* WorldContextObject, FGameplayTag Tag, TSubclassOf<AActor> ActorClass = nullptr, bool bOnlyReplicated = true` | Client & Server | `MPNetFunctionLibrary.h:465` |
| `HasAnyTagSafe` | `Has Any Tag Safe` | `Multiplayer Utility\|Names` | `bool` | `const AActor* Actor, const FGameplayTagContainer& TagContainer, bool bExactMatch = false` | Client & Server | `MPNetFunctionLibrary.h:477` |
| `MakeNetUniqueName` | `Make Net Unique Name` | `Multiplayer Utility\|Names` | `FString` | `const FString& Prefix = TEXT("NetObj"), int32 NetIndex = 0, int32 Counter = 0` | Client & Server | `MPNetFunctionLibrary.h:489` |
| `IsValidPlayerName` | `Is Valid Player Name` | `Multiplayer Utility\|Names` | `bool` | `const FString& InName, int32 MinLength = 3, int32 MaxLength = 20, bool bAllowSpaces = false` | Client & Server | `MPNetFunctionLibrary.h:503` |
| `ComparePlayerNamesCaseInsensitive` | `Compare Player Names (Case-Insensitive)` | `Multiplayer Utility\|Names` | `bool` | `const FString& NameA, const FString& NameB` | Client & Server | `MPNetFunctionLibrary.h:514` |

---

### 7. Team Affiliation & Relevancy
*Header: `MPNetFunctionLibrary.h` & `Types/MPNetTypes.h`*

| C++ Function | DisplayName | Category | Return Type | Parameters | Runs On | File:Line |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `AreTeammates` | `Are Teammates` | `Multiplayer Utility\|Team` | `bool` | `const AActor* ActorA, const AActor* ActorB` | Client & Server | `MPNetFunctionLibrary.h:595` |
| `AreEnemies` | `Are Enemies` | `Multiplayer Utility\|Team` | `bool` | `const AActor* ActorA, const AActor* ActorB` | Client & Server | `MPNetFunctionLibrary.h:606` |
| `GetActorsByTeam` | `Get Actors By Team` | `Multiplayer Utility\|Team` | `TArray<AActor*>` | `const UObject* WorldContextObject, int32 TeamId, TSubclassOf<AActor> ActorClass = nullptr` | Client & Server | `MPNetFunctionLibrary.h:617` |
| `GetTeamOfActorSafe` | `Get Team Of Actor Safe` | `Multiplayer Utility\|Team` | `int32` | `const AActor* Actor, int32 DefaultTeamId = -1` | Client & Server | `MPNetFunctionLibrary.h:628` |
| `IsActorRelevantToPlayerSafe` | `Is Actor Relevant To Player Safe` | `Multiplayer Utility\|Relevancy` | `bool` | `const AActor* Actor, const APlayerController* PC` | Server Only | `MPNetFunctionLibrary.h:639` |

---

### 8. Authoritative Server Validation
*Header: `MPNetFunctionLibrary.h`*

| C++ Function | DisplayName | Category | Return Type | Parameters | Runs On | File:Line |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `ValidateTransformFromClient` | `Validate Transform From Client` | `Multiplayer Utility\|Validation` | `bool` | `const AActor* Actor, const FTransform& NewTransform, float DeltaTime, float MaxSpeed, float MaxTeleportDist, FMPValidationResult& OutResult` | Server Only | `MPNetFunctionLibrary.h:662` |
| `ValidateTimestampFromClient` | `Validate Timestamp From Client` | `Multiplayer Utility\|Validation` | `bool` | `const UObject* WorldContextObject, float ClientTimestamp, float MaxRewindWindow, FMPValidationResult& OutResult` | Server Only | `MPNetFunctionLibrary.h:674` |
| `ValidateFloatRange` | `Validate Float Range` | `Multiplayer Utility\|Validation` | `bool` | `float Value, float MinValue, float MaxValue, FMPValidationResult& OutResult` | Client & Server | `MPNetFunctionLibrary.h:687` |
| `ValidateVectorNotNaN` | `Validate Vector Not NaN` | `Multiplayer Utility\|Validation` | `bool` | `const FVector& Vec, FMPValidationResult& OutResult` | Client & Server | `MPNetFunctionLibrary.h:698` |
| `ValidateEnumValue` | `Validate Enum Value` | `Multiplayer Utility\|Validation` | `bool` | `uint8 Value, uint8 MaxValidValue, FMPValidationResult& OutResult` | Client & Server | `MPNetFunctionLibrary.h:709` |
| `ClampClientInputVector` | `Clamp Client Input Vector` | `Multiplayer Utility\|Validation` | `FVector` | `const FVector& InputVector, float MaxLength = 1.0f` | Client & Server | `MPNetFunctionLibrary.h:719` |
| `IsValidSpawnLocation` | `Is Valid Spawn Location` | `Multiplayer Utility\|Validation` | `bool` | `const UObject* WorldContextObject, const FVector& Location, const FVector& Extent, bool bCheckCollision = true` | Server Only | `MPNetFunctionLibrary.h:731` |
| `IsPathClearServer` | `Is Path Clear Server` | `Multiplayer Utility\|Validation` | `bool` | `const UObject* WorldContextObject, const FVector& Start, const FVector& End, ECollisionChannel Channel = ECC_Visibility, const AActor* IgnoreActor = nullptr` | Server Only | `MPNetFunctionLibrary.h:744` |
| `SanitizePlayerName` | `Sanitize Player Name` | `Multiplayer Utility\|Validation` | `bool` | `const FString& InName, int32 MinLength, int32 MaxLength, FString& OutSanitizedName, FMPValidationResult& OutResult` | Client & Server | `MPNetFunctionLibrary.h:757` |

---

### 9. Component & Spawn Helpers
*Header: `MPNetFunctionLibrary.h`*

| C++ Function | DisplayName | Category | Return Type | Parameters | Runs On | File:Line |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `AddReplicatedComponentServerOnly` | `Add Replicated Component Server Only` | `Multiplayer Utility\|Spawn` | `UActorComponent*` | `AActor* Owner, TSubclassOf<UActorComponent> ComponentClass, FName ComponentName = NAME_None` | Server Only | `MPNetFunctionLibrary.h:772` |
| `FindComponentSafe` | `Find Component Safe` | `Multiplayer Utility\|Component` | `UActorComponent*` | `const AActor* Actor, TSubclassOf<UActorComponent> ComponentClass, FGameplayTag ComponentTag` | Client & Server | `MPNetFunctionLibrary.h:783` |
| `SpawnActorDeferredServerOnly` | `Spawn Actor Deferred Server Only` | `Multiplayer Utility\|Spawn` | `AActor*` | `const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform, AActor* Owner = nullptr, APawn* Instigator = nullptr, ESpawnActorCollisionHandlingMethod CollisionMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn` | Server Only | `MPNetFunctionLibrary.h:797` |
| `FinishSpawningActorServerOnly` | `Finish Spawning Actor Server Only` | `Multiplayer Utility\|Spawn` | `AActor*` | `AActor* Actor, const FTransform& SpawnTransform` | Server Only | `MPNetFunctionLibrary.h:807` |
| `SpawnAtFreePlayerStartSafe` | `Spawn At Free Player Start Safe` | `Multiplayer Utility\|Spawn` | `APawn*` | `const UObject* WorldContextObject, TSubclassOf<AActor> PawnClass, AController* Controller = nullptr` | Server Only | `MPNetFunctionLibrary.h:818` |
| `TeleportSafeServer` | `Teleport Safe Server` | `Multiplayer Utility\|Movement` | `bool` | `AActor* TargetActor, const FVector& DestLocation, const FRotator& DestRotation, bool bStopVelocity = true` | Server Only | `MPNetFunctionLibrary.h:830` |
| `GetOwningPlayerStateSafe` | `Get Owning Player State Safe` | `Multiplayer Utility\|Player` | `APlayerState*` | `const AActor* TargetActor` | Client & Server | `MPNetFunctionLibrary.h:840` |
| `GetOwningPawnSafe` | `Get Owning Pawn Safe` | `Multiplayer Utility\|Player` | `APawn*` | `const AActor* TargetActor` | Client & Server | `MPNetFunctionLibrary.h:850` |
| `ApplyNetPresetToActor` | `Apply Net Preset To Actor` | `Multiplayer Utility\|Preset` | `void` | `AActor* TargetActor, const UMPNetPresetAsset* Preset` | Server Only | `MPNetFunctionLibrary.h:860` |

---

### 10. Level Streaming & World Helpers
*Headers: `Wrappers/MPWorldSessionWrapper.h` & `Wrappers/MPPawnControllerWrapper.h`*

| C++ Function | DisplayName | Category | Return Type | Parameters | Runs On | File:Line |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `SafeServerTravel` | `Safe Server Travel` | `Multiplayer Utility\|World` | `void` | `const UObject* WorldContextObject, const FString& MapURL, bool bAbsolute = false, bool bShouldSkipGameNotify = false` | Server Only | `MPWorldSessionWrapper.h:35` |
| `GetConnectedPlayerCount` | `Get Connected Player Count` | `Multiplayer Utility\|World` | `int32` | `const UObject* WorldContextObject` | Client & Server | `MPWorldSessionWrapper.h:40` |
| `GetServerWorldTime` | `Get Authoritative Server Time` | `Multiplayer Utility\|World` | `double` | `const UObject* WorldContextObject` | Client & Server | `MPWorldSessionWrapper.h:44` |
| `IsLevelLoadedForAllPlayers` | `Is Level Loaded For All Players` | `Multiplayer Utility\|World` | `bool` | `const UObject* WorldContextObject, FName LevelName` | Server Only | `MPWorldSessionWrapper.h:49` |
| `GetPlayersInStreamingLevel` | `Get Players In Streaming Level` | `Multiplayer Utility\|World` | `TArray<APawn*>` | `const UObject* WorldContextObject, FName LevelName` | Client & Server | `MPWorldSessionWrapper.h:60` |
| `IsActorInLoadedLevel` | `Is Actor In Loaded Level` | `Multiplayer Utility\|World` | `bool` | `const AActor* Actor` | Client & Server | `MPWorldSessionWrapper.h:70` |
| `GetLevelNameOfActor` | `Get Level Name Of Actor` | `Multiplayer Utility\|World` | `FString` | `const AActor* Actor, bool bCleanShortName = true` | Client & Server | `MPWorldSessionWrapper.h:81` |
| `IsSeamlessTraveling` | `Is Seamless Traveling` | `Multiplayer Utility\|World` | `bool` | `const UObject* WorldContextObject` | Client & Server | `MPWorldSessionWrapper.h:91` |
| `GetCurrentMapNameClean` | `Get Current Map Name Clean` | `Multiplayer Utility\|World` | `FString` | `const UObject* WorldContextObject, bool bCleanShortName = true` | Client & Server | `MPWorldSessionWrapper.h:102` |
| `GetTravelOptionsFromURL` | `Get Travel Options From URL` | `Multiplayer Utility\|World` | `TMap<FString, FString>` | `const FString& InURL` | Client & Server | `MPWorldSessionWrapper.h:113` |
| `SafeClientTravel` | `Safe Client Travel` | `Multiplayer Utility\|Controller` | `void` | `APlayerController* Controller, const FString& URL, ETravelType TravelType = TRAVEL_Relative` | Client & Server | `MPPawnControllerWrapper.h:58` |

---

### 11. Network Math, Quantization & Bitwise Helpers
*Headers: `Types/MPQuantization.h` & `Types/MPTimeMath.h`*

| C++ Function | DisplayName | Category | Return Type | Parameters | Runs On | File:Line |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `WrapAngleSafe` | `Wrap Angle Safe` | `Multiplayer Utility\|Math` | `float` | `float AngleDegrees` | Client & Server | `MPQuantization.h:74` |
| `IsVectorWithinTolerance` | `Is Vector Within Tolerance` | `Multiplayer Utility\|Math` | `bool` | `const FVector& A, const FVector& B, float Tolerance = 1.0f` | Client & Server | `MPQuantization.h:81` |
| `IsRotatorWithinTolerance` | `Is Rotator Within Tolerance` | `Multiplayer Utility\|Math` | `bool` | `const FRotator& A, const FRotator& B, float ToleranceDegrees = 1.0f` | Client & Server | `MPQuantization.h:88` |
| `HashNameToNetInt` | `Hash Name To Net Int` | `Multiplayer Utility\|Math` | `int32` | `FName InName` | Client & Server | `MPQuantization.h:95` |
| `PackTwoFloatsToInt` | `Pack Two Floats To Int` | `Multiplayer Utility\|Quantization` | `int32` | `float ValA, float ValB, float RangeMin = -100.0f, float RangeMax = 100.0f` | Client & Server | `MPQuantization.h:102` |
| `UnpackTwoFloatsFromInt` | `Unpack Two Floats From Int` | `Multiplayer Utility\|Quantization` | `void` | `int32 PackedInt, float& OutValA, float& OutValB, float RangeMin = -100.0f, float RangeMax = 100.0f` | Client & Server | `MPQuantization.h:109` |
| `SetBit32` | `Set Bit (32-bit)` | `Multiplayer Utility\|Bitwise` | `int32` | `int32 Mask, int32 BitIndex, bool bValue = true` | Client & Server | `MPQuantization.h:115` |
| `ClearBit32` | `Clear Bit (32-bit)` | `Multiplayer Utility\|Bitwise` | `int32` | `int32 Mask, int32 BitIndex` | Client & Server | `MPQuantization.h:121` |
| `TestBit32` | `Test Bit (32-bit)` | `Multiplayer Utility\|Bitwise` | `bool` | `int32 Mask, int32 BitIndex` | Client & Server | `MPQuantization.h:127` |
| `SetBit64` | `Set Bit (64-bit)` | `Multiplayer Utility\|Bitwise` | `int64` | `int64 Mask, int32 BitIndex, bool bValue = true` | Client & Server | `MPQuantization.h:133` |
| `ClearBit64` | `Clear Bit (64-bit)` | `Multiplayer Utility\|Bitwise` | `int64` | `int64 Mask, int32 BitIndex` | Client & Server | `MPQuantization.h:139` |
| `TestBit64` | `Test Bit (64-bit)` | `Multiplayer Utility\|Bitwise` | `bool` | `int64 Mask, int32 BitIndex` | Client & Server | `MPQuantization.h:145` |
| `IsTimestampNewer` | `Is Timestamp Newer` | `Multiplayer Utility\|Math` | `bool` | `float TimestampA, float TimestampB, float HalfRange = 1800.0f` | Client & Server | `MPTimeMath.h:80` |
| `SequenceDistance16` | `Sequence Distance (16-bit)` | `Multiplayer Utility\|Math` | `int32` | `int32 SequenceA, int32 SequenceB` | Client & Server | `MPTimeMath.h:87` |
| `SequenceDistance32` | `Sequence Distance (32-bit)` | `Multiplayer Utility\|Math` | `int64` | `int64 SequenceA, int64 SequenceB` | Client & Server | `MPTimeMath.h:94` |
| `ClampDeltaTimeForNetwork` | `Clamp Delta Time For Network` | `Multiplayer Utility\|Math` | `float` | `float DeltaTime, float MinDelta = 0.001f, float MaxDelta = 0.2f` | Client & Server | `MPTimeMath.h:101` |
| `LerpNetValue` | `Lerp Net Value` | `Multiplayer Utility\|Math` | `float` | `float Current, float Target, float DeltaTime, float InterpolationSpeed` | Client & Server | `MPTimeMath.h:108` |
| `ExponentialSmoothFloat` | `Exponential Smooth (Float)` | `Multiplayer Utility\|Math` | `float` | `float Current, float Target, float DeltaTime, float SmoothingSpeed` | Client & Server | `MPTimeMath.h:115` |
| `ExponentialSmoothVector` | `Exponential Smooth (Vector)` | `Multiplayer Utility\|Math` | `FVector` | `const FVector& Current, const FVector& Target, float DeltaTime, float SmoothingSpeed` | Client & Server | `MPTimeMath.h:122` |

---

### 12. Flow-Control Execution & Latent Nodes
*Headers: `FlowControl/MPFlowControlLibrary.h` & `FlowControl/MPLatentActions.h`*

| C++ Function | DisplayName | Category | Return Type | Parameters / Exec Pins | Runs On | File:Line |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `BranchByNetRole` | `Branch By Net Role` | `Multiplayer Utility\|Flow Control` | `void` | `const AActor* TargetActor, EMPNetRoleBranch& Branches` (defaults to Self) | Client & Server | `MPFlowControlLibrary.h:24` |
| `BranchByNetMode` | `Branch By Net Mode` | `Multiplayer Utility\|Flow Control` | `void` | `const UObject* WorldContextObject, EMPNetModeBranch& Branches` | Client & Server | `MPFlowControlLibrary.h:30` |
| `BranchIsLocallyControlled`| `Branch Is Locally Controlled` | `Multiplayer Utility\|Flow Control` | `void` | `const AActor* TargetActor, EMPLocalControlBranch& Branches` (defaults to Self) | Client & Server | `MPFlowControlLibrary.h:36` |
| `BranchHasAuthority` | `Branch Has Authority` | `Multiplayer Utility\|Flow Control` | `void` | `const AActor* TargetActor, EMPAuthorityBranch& Branches` (defaults to Self) | Client & Server | `MPFlowControlLibrary.h:42` |
| `NetworkGate` | `Network Gate` | `Multiplayer Utility\|Flow Control` | `void` | `const UObject* WorldContextObject, bool bAllowServer, bool bAllowClient, bool bAllowStandalone, EMPGateExec& Branches` | Client & Server | `MPFlowControlLibrary.h:48` |
| `RunOnNextFrame` | `Run On Next Frame` | `Multiplayer Utility\|Flow Control` | `EMPDelayedActionResult` | `const UObject* WorldContextObject, FName ActionKey, FMPTimerCallback Callback` | Client & Server | `MPFlowControlLibrary.h:61` |
| `RunAfterServerSeconds` | `Run After Server Seconds` | `Multiplayer Utility\|Flow Control` | `EMPDelayedActionResult` | `const UObject* WorldContextObject, FName ActionKey, float DelaySeconds, FMPTimerCallback Callback` | Client & Server | `MPFlowControlLibrary.h:75` |
| `CancelPendingByKey` | `Cancel Pending By Key` | `Multiplayer Utility\|Flow Control` | `bool` | `const UObject* WorldContextObject, FName ActionKey` | Client & Server | `MPFlowControlLibrary.h:86` |
| `DoOnServerOnlyAfterDelay`| `Do On Server Only After Delay` | `Multiplayer Utility\|Flow Control` | `EMPDelayedActionResult` | `const UObject* WorldContextObject, FName ActionKey, float DelaySeconds, FMPTimerCallback Callback` | Server Only | `MPFlowControlLibrary.h:98` |
| `CallOncePerPlayer` | `Call Once Per Player` | `Multiplayer Utility\|Flow Control` | `bool` | `APlayerController* PC, FName ActionKey, FMPTimerCallback Callback` | Client & Server | `MPFlowControlLibrary.h:110` |
| `DelayUntilReplicated` | `Delay Until Replicated` | `Multiplayer Utility\|Flow Control` | `void` (Latent) | `const UObject* WorldContextObject, const AActor* TargetActor, float TimeoutSeconds, EMPDelayReplicationResult& Result, FLatentActionInfo LatentInfo` (defaults to Self) | Client & Server | `MPLatentActions.h:27` |
| `WaitServerSeconds` | `Wait Server Seconds` | `Multiplayer Utility\|Flow Control` | `void` (Latent) | `const UObject* WorldContextObject, float DurationSeconds, FLatentActionInfo LatentInfo` | Client & Server | `MPLatentActions.h:37` |

---

### 13. Replicated Helper Structs
*Headers: `Types/MPReplicatedDataTypes.h`, `Types/MPReplicatedTimer.h`, `Types/MPReplicatedSeed.h`, `Types/MPPackedTypes.h`*

- **`FMPReplicatedCounter`** (`Types/MPReplicatedDataTypes.h:12`): Replicated counter supporting `Increment()`, `Decrement()`, `Reset()`, and `GetValue()`. Uses custom `NetSerialize`.
- **`FMPReplicatedFlagSet`** (`Types/MPReplicatedDataTypes.h:61`): Replicated 32-bit bitmask supporting `SetFlag()`, `ClearFlag()`, `HasFlag()`, and `ToggleFlag()`.
- **`FMPReplicatedKeyValue`** (`Types/MPReplicatedDataTypes.h:185`): Dynamic key-value map using Unreal Engine's `FFastArraySerializer`. Replicates string, float, and int entries delta-wise; sends complete current state to late joiners on initial connection.
- **`FMPReplicatedLogRing`** (`Types/MPReplicatedDataTypes.h:117`): Fixed-size FIFO circular buffer of strings for on-screen debug HUDs and kill feeds.
- **`FMPReplicatedTimer`** (`Types/MPReplicatedTimer.h:12`): Stores server start time and duration. Replicates once; remote clients evaluate elapsed/remaining time locally against synchronized server time without ticking.
- **`FMPReplicatedCooldown`** (`Types/MPReplicatedTimer.h:116`): Per-ability cooldown evaluating `IsReady()` and `GetRemainingTime()` without round-trip RPCs.
- **`FMPReplicatedSeed`** (`Types/MPReplicatedSeed.h:12`): Replicates a 32-bit integer seed that initializes a deterministic `FRandomStream` across all connected machines.
- **`FMPPackedBools` / `FMPQuantizedVector` / `FMPQuantizedRotator`** (`Types/MPPackedTypes.h`): Space-efficient replicated structs with Blueprint `Make` and `Break` functions.

---

### 14. Network Components & Presets
*Headers: `Components/MPNetDebugTextComponent.h`, `Components/MPAutoDormancyComponent.h`, `Types/MPNetPresetAsset.h`*

| Component / Asset | Type | Runs On | Description | File:Line |
| :--- | :--- | :--- | :--- | :--- |
| **`UMPNetDebugTextComponent`** | `UActorComponent` | Client & Server (Dev only) | Periodic diagnostic text component showing role, net mode, owner, and `IMPNetDebugInterface` lines. Uses `FTimerManager` (no `Tick`). | `Components/MPNetDebugTextComponent.h:13` |
| **`UMPAutoDormancyComponent`** | `UActorComponent` | Server Only | Monitors actor displacement; transitions actor into `DORM_DormantAll` after `IdleSecondsBeforeDormancy`. Wakes up via `WakeFromDormancy()`. | `Components/MPAutoDormancyComponent.h:15` |
| **`UMPNetPresetAsset`** | `UDataAsset` | Server Only | Data asset holding reusable network settings (`NetUpdateFrequency`, `MinNetUpdateFrequency`, `NetCullDistanceSquared`, `NetPriority`, `NetDormancy`, `bAlwaysRelevant`, `bOnlyRelevantToOwner`). Applied via `ApplyToActor()`. | `Types/MPNetPresetAsset.h:13` |

---

## ⚙️ Settings Reference (`UMPUtilitySettings`)

Defined in `Source/MPUtilityRuntime/Public/MPUtilitySettings.h:8`. Configurable in **Project Settings $\rightarrow$ Plugins $\rightarrow$ Multiplayer Utility** or via `DefaultGame.ini`:

| Property Name | C++ Type | Default Value | Config Category | Functional Effect in Plugin |
| :--- | :--- | :--- | :--- | :--- |
| `HighPingThresholdMs` | `float` | `150.0f` | Network Diagnostics | Ping threshold in ms at or above which connection is considered high-latency / unstable. |
| `CriticalPingThresholdMs` | `float` | `250.0f` | Network Diagnostics | Ping threshold in ms at or above which `EMPConnectionQuality` transitions to `Critical`. |
| `UnstablePacketLossThreshold`| `float` | `3.0f` | Network Diagnostics | Inbound or outbound loss percentage above which `IsConnectionUnstable()` returns `true`. |
| `bEnableVerboseNetLogging` | `bool` | `false` | Network Diagnostics | When true, enables detailed per-packet and per-scope diagnostic logs via `MP_LOG`. |
| `DefaultRPCRefillRatePerSec` | `float` | `25.0f` | Security & Rate Limiting | Default token refill rate per second for token-bucket RPC limiters. |
| `DefaultRPCTokenBucketCapacity` | `int32` | `40` | Security & Rate Limiting | Maximum burst token capacity for token-bucket RPC limiters. |
| `bKickOnValidationFailure` | `bool` | `false` | Security & Rate Limiting | When true, `MP_VALIDATE_OR_KICK` terminates client connections on failed checks. When false, logs a warning only. |
| `bEnableCPUScopes` | `bool` | `true` | Profiling & Diagnostics | Enables Unreal Insights CPU cycle profiling for `MP_NET_SCOPE` in non-shipping builds. |
| `LagCompensationBufferSeconds` | `float` | `1.0f` | Lag Compensation | Default circular history buffer duration in seconds for `FMPRewindBuffer`. |
| `NetLODNearDistance` | `float` | `1500.0f` | Bandwidth & Net LOD | Distance threshold below which actors replicate at `NetLODMaxFrequency`. |
| `NetLODFarDistance` | `float` | `6000.0f` | Bandwidth & Net LOD | Distance threshold above which actors replicate at `NetLODMinFrequency`. |
| `NetLODMaxFrequency` | `float` | `30.0f` | Bandwidth & Net LOD | Maximum replication frequency in Hz for close-proximity actors. |
| `NetLODMinFrequency` | `float` | `2.0f` | Bandwidth & Net LOD | Minimum replication frequency in Hz for distant actors. |

---

## 💻 Console Variables (CVars) Reference

Registered in `Source/MPUtilityRuntime/Private/MPUtilityRuntimeModule.cpp`. Can be toggled in the console (`~`) or manipulated via Blueprint nodes (`GetMPCVarBool`, `SetMPCVarBool`, etc.):

| Console Variable | Type | Default | Description | Runtime Impact |
| :--- | :--- | :--- | :--- | :--- |
| `mp.Debug.Overlay` | `int32` | `0` | Controls on-screen replication debug overlay display. | Setting to `1` renders network roles, ping, and loss on screen. |
| `mp.Debug.LogOnce` | `int32` | `1` | Controls whether wrong-machine execution warnings are logged once or every time. | When `1`, suppresses repeated log flooding from ticking/frequent callsites. |
| `mp.RateLimit.Enabled` | `int32` | `1` | Master toggle for RPC rate limiting. | Setting to `0` bypasses token consumption in `MP_SERVER_RPC_GUARD`. |
| `mp.Validation.KickOnFail` | `int32` | `0` | Runtime toggle overriding `bKickOnValidationFailure`. | When `1`, kicks clients failing authoritative validation checks. |
| `mp.Net.PingSampleInterval` | `float` | `0.5` | Sampling period in seconds for rolling average ping calculations. | Adjusts the update frequency of connection quality smoothing buffers. |

> [!NOTE]
> All console interactions are managed via `TAutoConsoleVariable` (`mp.*`). The plugin registers no raw exec console commands.

---

## 🛡️ Complete Macro Reference (`MPNetMacros.h`)

All macros are wrapped in `do { ... } while(0)` or safe lambda scopes to prevent dangling-else bugs and multiple argument evaluation.

| Macro & Signature | Return-Type Safety | Description | Code Example | File:Line |
| :--- | :--- | :--- | :--- | :--- |
| `MP_CHECK_AUTHORITY(Actor)` | `void` functions | Returns early if Actor lacks `HasAuthority()`. | `MP_CHECK_AUTHORITY(this);` | `MPNetMacros.h:130` |
| `MP_CHECK_AUTHORITY_RET(Actor, RetVal)` | Typed functions (`bool`, pointer, struct) | Returns `RetVal` if Actor lacks `HasAuthority()`. | `MP_CHECK_AUTHORITY_RET(this, false);` | `MPNetMacros.h:137` |
| `MP_CHECK_LOCAL(Actor)` | `void` functions | Returns early if Actor is not locally controlled. | `MP_CHECK_LOCAL(this);` | `MPNetMacros.h:145` |
| `MP_CHECK_LOCAL_RET(Actor, RetVal)` | Typed functions | Returns `RetVal` if Actor is not locally controlled. | `MP_CHECK_LOCAL_RET(this, FVector::ZeroVector);` | `MPNetMacros.h:155` |
| `MP_CHECK_CONNECTION(Actor)` | `void` functions | Returns early if Actor lacks an owning `UNetConnection`. | `MP_CHECK_CONNECTION(this);` | `MPNetMacros.h:165` |
| `MP_CHECK_CONNECTION_RET(Actor, RetVal)` | Typed functions | Returns `RetVal` if Actor lacks an owning `UNetConnection`. | `MP_CHECK_CONNECTION_RET(this, false);` | `MPNetMacros.h:173` |
| `MP_SERVER_RPC_GUARD(RateLimiterRef)` | `void` functions | Validates authority, checks `mp.RateLimit.Enabled`, and consumes 1 token from `RateLimiterRef`. | `MP_SERVER_RPC_GUARD(FireRateLimiter);` | `MPNetMacros.h:663` |
| `MP_SERVER_RPC_GUARD_RET(RateLimiterRef, RetVal)` | Typed functions | Validates authority and rate limits, returning `RetVal` on drop. | `MP_SERVER_RPC_GUARD_RET(FireRateLimiter, false);` | `MPNetMacros.h:677` |
| `MP_VALIDATE_RET(Condition, RetVal, Reason)` | Typed functions | Logs warning with `Reason` and returns `RetVal` if `Condition` is false. | `MP_VALIDATE_RET(DeltaTime > 0.0f, false, "Invalid delta");` | `MPNetMacros.h:754` |
| `MP_RETURN_IF_NO_WORLD(Obj)` | `void` functions | Returns early if `Obj->GetWorld()` is null. | `MP_RETURN_IF_NO_WORLD(this);` | `MPNetMacros.h:739` |
| `MP_RETURN_IF_NO_WORLD_RET(Obj, RetVal)` | Typed functions | Returns `RetVal` if `Obj->GetWorld()` is null. | `MP_RETURN_IF_NO_WORLD_RET(this, nullptr);` | `MPNetMacros.h:747` |
| `MP_ENSURE_SERVER_OR_RETURN(Obj)` | `void` functions | Logs warning and returns early if world net mode is client. | `MP_ENSURE_SERVER_OR_RETURN(this);` | `MPNetMacros.h:523` |
| `MP_ENSURE_SERVER_OR_RETURN_RET(Obj, RetVal)` | Typed functions | Logs warning and returns `RetVal` if world net mode is client. | `MP_ENSURE_SERVER_OR_RETURN_RET(this, false);` | `MPNetMacros.h:533` |
| `MP_BRANCH_NETMODE(WorldContext, ServerExpr, ClientExpr)` | Expression | Evaluates to `ServerExpr` on server, or `ClientExpr` on client/standalone. | `float Rate = MP_BRANCH_NETMODE(this, 60.0f, 30.0f);` | `MPNetMacros.h:697` |
| `MP_FOR_EACH_PLAYER(WorldContext, PCVar)` | Loop statement | Loops through all active `APlayerController` instances in the world. | `MP_FOR_EACH_PLAYER(this, PC) { ... }` | `MPNetMacros.h:231` |
| `MP_CALL_ONREP_ON_SERVER(Actor, OnRepFunc)` | Statement | Invokes `Actor->OnRepFunc()` if local machine has network authority. | `MP_CALL_ONREP_ON_SERVER(this, OnRep_Health);` | `MPNetMacros.h:420` |
| `MP_DOREP_PROPERTY(Class, Var)` | `GetLifetimeReplicatedProps` | Standard push model / property replication registration. | `MP_DOREP_PROPERTY(AMyCharacter, Health);` | `MPNetMacros.h:268` |
| `MP_DOREP_OWNER_ONLY(Class, Var)` | `GetLifetimeReplicatedProps` | Replicates property only to the owning connection (`COND_OwnerOnly`). | `MP_DOREP_OWNER_ONLY(AMyCharacter, Inventory);` | `MPNetMacros.h:277` |
| `MP_DOREP_SKIP_OWNER(Class, Var)` | `GetLifetimeReplicatedProps` | Replicates property to everyone except the owner (`COND_SkipOwner`). | `MP_DOREP_SKIP_OWNER(AMyCharacter, MeshCosmetics);` | `MPNetMacros.h:286` |
| `MP_DOREP_INITIAL_ONLY(Class, Var)` | `GetLifetimeReplicatedProps` | Replicates property only once on initial connection (`COND_InitialOnly`). | `MP_DOREP_INITIAL_ONLY(AMyCharacter, Seed);` | `MPNetMacros.h:295` |
| `MP_REP_NOTIFY_IF_CHANGED(Actor, Var, NewValue, OnRepFunc)` | Statement | Assigns `NewValue`, marks dirty, and invokes `OnRepFunc` if changed. | `MP_REP_NOTIFY_IF_CHANGED(this, Ammo, 30, OnRep_Ammo);` | `MPNetMacros.h:389` |
| `MP_UNLIKELY_RETURN_IF(Condition)` | `void` functions | Branch-prediction hinted early exit when `Condition` is unexpectedly true. | `MP_UNLIKELY_RETURN_IF(DeltaTime <= 0.0f);` | `MPNetMacros.h:715` |
| `MP_LOG_ONCE_PER_ACTOR(Actor, Format, ...)` | Statement | Logs a warning once per unique Actor pointer to prevent log spam. | `MP_LOG_ONCE_PER_ACTOR(Target, TEXT("Invalid target"));` | `MPNetMacros.h:324` |

---

## ⏱️ Pure C++ Data Structures & RAII Scoped Guards

### 1. `FMPRateLimiter` (`Types/MPRateLimiter.h:11`)
Thread-safe token bucket rate limiter for RPC flood prevention:
```cpp
#include "Types/MPRateLimiter.h"

// 10 max burst tokens, refills 5 tokens per second
FMPRateLimiter FireRateLimiter(10.0, 5.0);

void AWeapon::ServerFire_Implementation()
{
    const double CurrentTime = FPlatformTime::Seconds();
    if (!FireRateLimiter.ConsumeToken(CurrentTime, 1.0))
    {
        return; // Exceeded firing rate
    }
    // Proceed with authoritative fire
}
```

### 2. `FMPRewindBuffer` (`Types/MPRewindBuffer.h:17`)
Circular lag compensation snapshot history with Quaternion SLERP and extrapolation:
```cpp
#include "Types/MPRewindBuffer.h"

FMPRewindBuffer HitboxHistory(128, 1.0f /* 1.0s window */);

// Record in timer or movement update:
void AMyCharacter::RecordHistorySnapshot()
{
    HitboxHistory.RecordSnapshot(GetWorld()->GetTimeSeconds(), GetMesh()->GetComponentTransform(), GetVelocity());
}

// Validate client hit timestamp:
FTransform RewoundTransform;
if (HitboxHistory.GetInterpolatedTransform(ClientTimestamp, RewoundTransform))
{
    // Authoritative trace against RewoundTransform
}
```

### 3. RAII Scoped Guards (`Types/MPScopedGuards.h`)
- **`FMPScopedAuthorityCheck`** (`Line 24`): Verifies authority on enter; logs warning if called on client.
- **`FMPScopedNetTimer`** (`Line 66`): Measures execution duration; logs warning if scope exceeds threshold in ms.
- **`FMPScopedRPCCounter`** (`Line 107`): Increments `UMPNetStatsSubsystem` call count for specified RPC name (`#if !UE_BUILD_SHIPPING`).
- **`FMPScopedDormancyWake`** (`Line 165`): Temporarily wakes a dormant actor and restores dormancy upon scope exit.
- **`FMPScopedCVarOverride`** (`Line 210`): Temporarily overrides a CVar value and restores the original value upon scope destruction.

---

## 🚀 Production Gameplay Examples

### Example 1: Dynamic Replication Frequency from Connection Quality
```cpp
#include "MPNetFunctionLibrary.h"
#include "Wrappers/MPActorNetworkWrapper.h"

void ANetworkGameMode::AdjustActorReplicationForConnection(APlayerController* PC, AActor* TargetActor)
{
    if (!PC || !TargetActor || !TargetActor->HasAuthority()) return;

    if (UMPNetFunctionLibrary::ShouldReduceNetworkLoad(PC))
    {
        // Unstable connection: throttle replication to conserve server bandwidth
        UMPActorNetworkWrapper::SetNetUpdateFrequency(TargetActor, 5.0f);
        UMPActorNetworkWrapper::SetMinNetUpdateFrequency(TargetActor, 1.0f);
        UMPActorNetworkWrapper::SetNetCullDistance(TargetActor, 4000.0f);
    }
    else
    {
        // Stable connection: full update rate
        UMPActorNetworkWrapper::SetNetUpdateFrequency(TargetActor, 30.0f);
        UMPActorNetworkWrapper::SetMinNetUpdateFrequency(TargetActor, 5.0f);
        UMPActorNetworkWrapper::SetNetCullDistance(TargetActor, 10000.0f);
    }
}
```

### Example 2: Client Transform Correction (`SnapOrSmoothTransform`)
```cpp
#include "Wrappers/MPMovementPhysicsWrapper.h"

void ARemotePuppet::OnRep_ReplicatedTransform()
{
    // Evaluates spatial error: snaps if error > 300 units; smoothly interpolates if below
    const FTransform Smoothed = UMPMovementPhysicsWrapper::SnapOrSmoothTransform(
        GetActorTransform(),
        ReplicatedTransform,
        300.0f,  /* Snap threshold distance */
        0.20f   /* Interpolation alpha */
    );
    SetActorTransform(Smoothed);
}
```

### Example 3: Server RPC Guard & Lag Compensated Weapon Fire
```cpp
#include "MPNetMacros.h"
#include "Types/MPRateLimiter.h"
#include "GameFramework/GameStateBase.h"

// 1. Client requests fire using synchronized server time
void AWeapon::Fire()
{
    if (IsLocallyControlled())
    {
        PlayMuzzleFX();
    }

    const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
    const double SyncedServerTime = GS ? GS->GetServerWorldTimeSeconds() : (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0);

    Server_FireShot(SyncedServerTime, GetActorLocation(), FollowCamera->GetForwardVector());
}

// 2. Server validates authority, rate limit, and rewind history
void AWeapon::Server_FireShot_Implementation(double ClientShotTime, FVector_NetQuantize ShotOrigin, FVector_NetQuantizeNormal AimDir)
{
    // Guard authority and rate limiter in one macro
    MP_SERVER_RPC_GUARD(FireLimiter);

    const double CurrentServerTime = GetWorld()->GetTimeSeconds();
    const double ClampedTime = FMath::Clamp(ClientShotTime, CurrentServerTime - 0.5, CurrentServerTime);

    for (AMyCharacter* Enemy : ActiveEnemies)
    {
        FTransform HistoricalTransform;
        if (Enemy->HitboxHistory.GetInterpolatedTransform(ClampedTime, HistoricalTransform))
        {
            // Perform authoritative line trace against HistoricalTransform
        }
    }
}
```

### Example 4: Scoped Dormancy Wake on Inventory Modification
```cpp
#include "Types/MPScopedGuards.h"
#include "MPNetMacros.h"

void ALootChest::AddLootItem(const FLootData& NewItem)
{
    MP_CHECK_AUTHORITY(this);

    // RAII guard wakes the chest, forces a net update, and restores DORM_DormantPartial on exit
    {
        FMPScopedDormancyWake WakeGuard(this);
        InventoryItems.Add(NewItem);
        MP_REP_NOTIFY_IF_CHANGED(this, ItemCount, InventoryItems.Num(), OnRep_ItemCount);
    }
}
```

### Example 5: Authoritative Server Movement Validation with Client Correction
```cpp
#include "MPNetFunctionLibrary.h"
#include "MPNetMacros.h"

// Server processes client move request
void APlayerCharacter::Server_ProcessClientMove_Implementation(FTransform ClientProposedTransform, float DeltaTime)
{
    MP_CHECK_AUTHORITY(this);

    FMPValidationResult Validation;
    if (!UMPNetFunctionLibrary::ValidateTransformFromClient(
        this,
        ClientProposedTransform,
        DeltaTime,
        1200.0f, /* Max Speed */
        300.0f,  /* Max Teleport Distance */
        Validation))
    {
        // Validation failed (speed hack, NaN, or illegal teleport):
        // Issue authoritative correction to client via Client RPC
        Client_CorrectTransform(GetActorTransform());
        return;
    }

    SetActorTransform(ClientProposedTransform);
}

// Client applies authoritative correction
void APlayerCharacter::Client_CorrectTransform_Implementation(FTransform AuthoritativeTransform)
{
    SetActorTransform(AuthoritativeTransform);
}
```

---

## ⚠️ Common Multiplayer Mistakes & Solutions

| Common Multiplayer Mistake | Why it Fails in Unreal Engine | Which Node / Macro Prevents It |
| :--- | :--- | :--- |
| **Effect plays twice on Listen Server** | Host player triggers local cosmetic prediction, then NetMulticast runs a second time on the same machine. | In Multicast RPC, skip local controller: `if (IsLocallyControlled()) return;` or use `BranchByNetRole`. |
| **Server RPC dropped silently** | Target Actor lacks an owning `UNetConnection` (spawned into world without an owner). | Guard with `MP_CHECK_CONNECTION(this)` or check `IsRPCSafeToCall(Actor)`. |
| **Client-trusted health or score** | Client computes damage and assigns variable directly; vulnerable to memory spoofing and desyncs. | Guard variable setters with `MP_CHECK_AUTHORITY(this)` and route RPCs through `MP_SERVER_RPC_GUARD`. |
| **Late joiners missing state** | State was sent as a transient NetMulticast RPC rather than a replicated property. | Replicate state with `UPROPERTY(ReplicatedUsing=...)` and `MP_REP_NOTIFY_IF_CHANGED`. |
| **Subobject pointers desyncing** | Subobjects not registered with the actor's subobject replication list. | Register with `RegisterReplicatedSubobjectSafe(Owner, Subobject)`. |
| **Physics desync on client** | Physics enabled on client locally without replicating velocity or movement. | Use `SetSimulatePhysicsServerOnly` and `ApplyImpulseServerOnly`. |

---

## ⚠️ Known Limitations & Technical Boundaries

1. **Character Movement Component Prediction**:
   The plugin's movement wrappers (`SnapOrSmoothTransform`, `GetMovementErrorDistance`) operate on general `AActor` transforms. They do **not** replace Unreal Engine's `UCharacterMovementComponent` prediction pipeline (`FSavedMove_Character`, `FNetworkPredictionData_Client_Character`), which requires dedicated C++ character movement subclassing.
2. **Fast Array Serializer Requirements**:
   `FMPReplicatedKeyValue` relies on `FFastArraySerializer`. Modifications must occur on the server with network authority. Client-side modifications without server replication will trigger local desyncs.
3. **Quantized Client Ping**:
   In Unreal Engine, `PlayerState::CompressedPing` compresses RTT into 1 byte (dividing by 4). Millisecond calculations derived from `CompressedPing` have an intrinsic resolution limit of $\pm 4\text{ms}$.
4. **World Level Bounds in Level Streaming**:
   `ULevelBounds::CalculateLevelBounds` computes bounding volumes at level load. Rapidly streaming sublevels with dynamic actors may produce approximate spatial overlap bounds.

---

## 🔧 Engine Version Notes & Compatibility

- **UE 5.3 & 5.4**:
  `AActor::NetUpdateFrequency`, `MinNetUpdateFrequency`, and `NetCullDistanceSquared` allow direct member assignment.
- **UE 5.5 & 5.6+**:
  Direct public member access on `AActor` for network properties is deprecated in favor of `SetNetUpdateFrequency()`, `GetNetUpdateFrequency()`, etc. The plugin encapsulates these differences using `#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5` preprocessor blocks.
- **Subobject Replication (UE 5.1+)**:
  The plugin utilizes `AddReplicatedSubObject` and `RemoveReplicatedSubObject` registered subobject lists, supported across UE 5.1 through 5.6+.

---

## 🧪 Automated QA & Unit Testing Guide

The plugin includes an automated test suite of **23 automation test suites** built using `FAutomationSpecBase` under the `MPUtilityTests` module (`Source/MPUtilityTests/Private/Specs/MPUtilityAutomationSpecs.cpp`).

### Automated Test Suite Matrix (`MultiplayerUtility.Core.*`)
1. **`MultiplayerUtility.Core.AdvancedMacros`**: Tests `MP_ENSURE_SERVER_OR_RETURN`, `MP_SERVER_RPC_GUARD`, `MP_BRANCH_NETMODE`, and unlikely return macros.
2. **`MultiplayerUtility.Core.CVarSettings`**: Tests registration, reading, and modification of all 5 plugin CVars via C++ and Blueprint nodes.
3. **`MultiplayerUtility.Core.LevelStreamingWorld`**: Validates streaming level actor queries, clean map name parsing, and travel option maps.
4. **`MultiplayerUtility.Core.Macros`**: Tests authority guards, local pawn checks, connection checks, and non-void return macros.
5. **`MultiplayerUtility.Core.MathDataNetworking`**: Validates angle wrapping, component-wise tolerances, 32/64-bit flags, and exponential smoothing.
6. **`MultiplayerUtility.Core.MetadataValidation`**: Reflection test verifying that every `UFUNCTION` has a non-empty `Category`, `ToolTip` docstring, and unique `DisplayName`.
7. **`MultiplayerUtility.Core.MiscExtras`**: Tests `IMPOwnershipInterface`, `GetLogicalOwnerSafe`, and `UMPNetStatsSubsystem` call metrics.
8. **`MultiplayerUtility.Core.MovementPhysics`**: Tests server impulse, safe velocity queries, physics replication checks, and smoothing modes.
9. **`MultiplayerUtility.Core.NullSafety`**: Verifies null-safe behavior across library wrappers when passed nullptr targets.
10. **`MultiplayerUtility.Core.PackedTypes`**: Validates packing and unpacking of normalized floats, packed bools, and quantized vectors/rotators.
11. **`MultiplayerUtility.Core.PlayerQueries`**: Validates player name validation, case-insensitive comparison, net unique names, and player queries.
12. **`MultiplayerUtility.Core.RateLimiter`**: Tests token consumption, burst limits, token replenishment over time, and starvation.
13. **`MultiplayerUtility.Core.ReplicatedDataTypes`**: Tests `FMPReplicatedCounter`, `FMPReplicatedFlagSet`, `FMPReplicatedKeyValue`, and `FMPReplicatedLogRing`.
14. **`MultiplayerUtility.Core.ReplicatedSeed`**: Tests deterministic `FRandomStream` synchronization across simulated server/client seeds.
15. **`MultiplayerUtility.Core.RewindBuffer`**: Tests ring-buffer eviction, time boundary clamping, midpoint SLERP, and forward extrapolation.
16. **`MultiplayerUtility.Core.ScopedGuards`**: Tests `FMPScopedAuthorityCheck`, `FMPScopedNetTimer`, `FMPScopedDormancyWake`, and `FMPScopedCVarOverride`.
17. **`MultiplayerUtility.Core.ServerValidation`**: Tests client transform validation, timestamp rewind clamping, float range, vector NaN, and name sanitization.
18. **`MultiplayerUtility.Core.SmokeTest`**: High-level sanity check verifying module startup and core function availability.
19. **`MultiplayerUtility.Core.SubobjectComponentReplication`**: Tests subobject registration list handling, recursive component replication, and component role checks.
20. **`MultiplayerUtility.Core.TagAndNameHelpers`**: Tests replicated tag queries and Gameplay Tag safe validation.
21. **`MultiplayerUtility.Core.TeamAndPresets`**: Tests team queries (`AreTeammates`, `AreEnemies`), `UMPNetPresetAsset`, and `CallOncePerPlayer`.
22. **`MultiplayerUtility.Core.TimeMath`**: Tests 4-timestamp NTP clock offset calculations, EMA ping smoothing, and RFC 3550 jitter.
23. **`MultiplayerUtility.Core.UrlParser`**: Tests travel URL generation, key extraction, option maps, and type conversions.

### Anti-Duplicate Validation Tool (`scan_duplicates.py`)
Run the standalone validation script in `<YourProject>/Plugins/MultiplayerUtility/Scripts/scan_duplicates.py` to check for conflicting function names, macro collisions, or duplicate DisplayNames:
```powershell
& "<UE_ROOT>\Engine\Binaries\ThirdParty\Python3\Win64\python.exe" `
    "<YourProject>\Plugins\MultiplayerUtility\Scripts\scan_duplicates.py"
```

### Running Tests in Unreal Editor
1. In Unreal Editor, open **Tools** $\rightarrow$ **Test Automation**.
2. In the filter box, type `MultiplayerUtility`.
3. Check the `MultiplayerUtility` root item.
4. Click **Start Tests**.

### Running Headless in CI/CD (Command Line)
```powershell
& "<UE_ROOT>\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
    "<YourProject>\<YourProject>.uproject" `
    -ExecCmds="Automation RunTests MultiplayerUtility; Quit" `
    -unattended -nopause -testexit="Automation Test Queue Empty" -log
```

---

## 📜 Changelog & Version History

- **v1.6.0 (Current)**:
  - Streamlined architecture to focus purely on high-performance runtime function utilities and gameplay networking.
  - Eliminated editor module overhead and heavy editor UI dependencies.
  - Retained all 14 comprehensive function utility areas (110+ UFUNCTIONs and helpers).
  - Maintained 23 automated test suites covering all runtime networking math, buffers, limiters, and wrappers.
- **v1.5.0**:
  - Added `UMPTeamInterface`, `UMPRelevancyInterface`, and `UMPNetDebugInterface`.
  - Added Authoritative Server Validation helpers (`ValidateTransformFromClient`, `ValidateTimestampFromClient`, etc.).
  - Added Replicated Helper Types (`FMPReplicatedCounter`, `FMPReplicatedFlagSet`, `FMPReplicatedKeyValue`, `FMPReplicatedLogRing`).
  - Added `UMPNetPresetAsset`, `UMPNetDebugTextComponent`, and `UMPAutoDormancyComponent`.
  - Added Level Editor scanning utilities (`FindReplicatedActorsInLevel`, etc.).
  - Expanded automated test coverage to 23 automation test suites.
- **v1.4.0**:
  - Added Section L advanced macros (`MP_SERVER_RPC_GUARD`, `MP_ENSURE_SERVER_OR_RETURN`, `MP_BRANCH_NETMODE`).
  - Added Scoped RAII Guards (`FMPScopedAuthorityCheck`, `FMPScopedNetTimer`, `FMPScopedDormancyWake`, `FMPScopedCVarOverride`).
  - Added `UMPOwnershipInterface` and `UMPNetStatsSubsystem`.
- **v1.3.0**:
  - Added Subobject Replication wrappers (`RegisterReplicatedSubobjectSafe`).
  - Added Gameplay Tag query helpers and network unique name generator.
  - Added network math, bitwise flag manipulation (32/64-bit), and float packing.
  - Registered plugin CVars (`mp.*`) and diagnostic stats group.
- **v1.2.0**:
  - Added Connection Quality assessment helpers (`GetConnectionQualityEnum`, `GetPingTrend`, `ShouldReduceNetworkLoad`).
  - Added Movement & Physics replication wrappers (`SnapOrSmoothTransform`, `ApplyImpulseServerOnly`).
  - Added Level Streaming & World helpers.
- **v1.1.0**:
  - Code audit deduplication: consolidated branch enums into `Types/MPNetTypes.h`.
  - Restructured CoreRedirects in `BaseMultiplayerUtility.ini`.
- **v1.0.0**:
  - Initial architecture: actor wrappers, pawn/controller helpers, flow control libraries, rate limiter, rewind buffer, and time math.
