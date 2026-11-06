# Multiplayer Utility — Architecture Guide

## 1. Modular Architecture Overview

The `MultiplayerUtility` plugin is structured into three cleanly decoupled Unreal Engine modules:

```
MultiplayerUtility Plugin
 ├── MPUtilityRuntime  (Runtime Gameplay Networking, Subsystems, Components, Fast Arrays, Security)
 ├── MPUtilityEditor   (Editor-only Subsystems, Actor Diagnostics, PIE Network Emulation)
 └── MPUtilityTests    (Automation Test Suites, Network Stress Tester, Test Actors)
```

### Module Roles & Rules:
- **`MPUtilityRuntime`**: Pure runtime code. Shipped with game client and dedicated server builds. Has zero editor dependencies.
- **`MPUtilityEditor`**: Editor-only tools and subsystems (`UMPEditorScannerSubsystem`). Never loaded or referenced in cooked builds.
- **`MPUtilityTests`**: Unit and automation test suites. Only compiled in editor and test configurations.

---

## 2. Subsystems & Lifecycles

### `UMPNetworkRequestSubsystem` (`UWorldSubsystem`)
- Scoped to the active `UWorld` and its `UNetDriver`.
- Orchestrates client-server request/response RPC correlation, lifetime management, timeout cancellations, and telemetry.
- Automatically cleans up pending requests on world transitions or disconnects.

### `UMPNetStatsSubsystem` (`UWorldSubsystem`)
- Tracks active replicated actor counts, network frame times, and RPC invocation frequencies.

### `UMPEditorScannerSubsystem` (`UEditorSubsystem`)
- Evaluates actor CDOs and placed level actors for missing replication properties, improper dormancy configurations, and invalid RPC ownership chains.

---

## 3. Core Component Architecture

1. **`UMPNetworkRequestComponent`**:
   - Attached to `APlayerController` or `APlayerState`.
   - Bridges reliable requests to the authoritative server and delivers validated responses back to client callers.
2. **`UMPCharacterMovementComponent`**:
   - Subclasses `UCharacterMovementComponent`.
   - Implements `FSavedMove_MPCharacter` and `FNetworkPredictionData_Client_MPCharacter` to predict custom movement inputs (sprint, dash, slide, climbing).
3. **`UMPNetLODComponent`**:
   - Server-only optimization component.
   - Dynamically scales `NetUpdateFrequency` and dormancy based on distance to nearest player.
4. **`UMPHealthComponent`**:
   - Server-authoritative shield absorption, health, downed, and revive state management.
