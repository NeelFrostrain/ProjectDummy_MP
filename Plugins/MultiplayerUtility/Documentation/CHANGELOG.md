# Multiplayer Utility — Changelog

## [v2.1.0] - Ergonomics, Blueprint Showcase & Decoupling (November 2026)

### Added:
- **`DefaultToSelf` Ergonomics Across All Custom Nodes**:
  - `UMPLatentActions::DelayUntilReplicated` now takes `const AActor* TargetActor` with `meta = (DefaultToSelf = "TargetActor")`, automatically defaulting to `Self` in any Actor/Character graph.
  - Added `DefaultToSelf = "TargetActor"` to all Flow Control branch splitters (`BranchByNetRole`, `BranchHasAuthority`, `BranchIsLocallyControlled`).
  - Added `DefaultToSelf = "TargetActor"` to all `MPActorNetworkWrapper` replication/dormancy/frequency functions, `MPGameplayHelperLibrary` utilities, and `MPNetFunctionLibrary` queries.
  - Added `DefaultToSelf = "Controller"` to async actions `WaitUntilPlayerStateAvailable` and `WaitUntilPawnPossessed`.
- **Automated Blueprint Graph Showcase & Native Generator**:
  - `UMPEditorScannerSubsystem::GenerateShowcaseBlueprint`: automatically creates, allocates pins, compiles, and saves `Content/Blueprints/BP_MultiplayerUtility_Showcase.uasset` with 254 nodes across 27 color-coded comment boxes.
  - Native clipboard exporter `ExportNodesToText` generating 897 KB of authentic Unreal Engine T3D with complete pin definitions in `MultiplayerUtility_BlueprintGraph_Clipboard.txt`.

### Changed & Cleaned:
- **Decoupled Interaction and Inventory Systems**:
  - Removed `UMPInteractionComponent`, `MPInteractionTypes`, and `FMPFastInventoryArray` from `MultiplayerUtility` to keep the plugin strictly lightweight and stateless.
  - Full inventory components and UI remain housed in the dedicated [`MultiplayerInventory`](../../MultiplayerInventory) plugin.
  - Re-indexed graph layout from 28 down to 27 clean categories.

---

## [v2.0.0] - Master Enterprise Expansion (November 2026)

### Added:
- **RPC Request / Response Framework**:
  - `UMPNetworkRequestSubsystem` World Subsystem for global lifetime tracking and telemetry.
  - `UMPNetworkRequestComponent` for bidirectional client-server reliable request/response correlation.
  - `FMPNetworkRequest` and `FMPNetworkResponse` with `FGuid` correlation IDs and idempotent retry limits.
  - `UMPNetworkRequestHandle` Blueprint object for polling and cancelling in-flight requests.
- **Advanced Replication & Fast Arrays**:
  - Reusable Fast Array framework: `FMPFastArrayItemBase`, `FMPFastArrayBase`.
  - `FMPReplicatedKeyValueArray` for delta-serialized string/name dictionaries.
  - Dynamic replication condition overrides via `UE::Net::Private::FNetPropertyConditionManager`.
  - Push Model integration macro `MP_MARK_PROPERTY_DIRTY`.
- **Character Movement Prediction Pipeline**:
  - `UMPCharacterMovementComponent` subclassing `UCharacterMovementComponent`.
  - `FSavedMove_MPCharacter` and `FNetworkPredictionData_Client_MPCharacter` client prediction implementation.
  - Authoritative sprint, dash, slide, and climb synchronization.
  - 4 network smoothing profile presets (`TightFastPaced`, `CinematicSmooth`, etc.).
  - `ValidateMovementStep` authoritative velocity validation.
- **Player Lifecycle Management**:
  - `UMPPlayerLifecycleLibrary` safe possession, unpossession, spawning, ownership transfer, spectator mode, and cleanup.
- **Replicated Gameplay State Machines & Timers**:
  - `FMPReplicatedStateMachine` for network-synchronized gameplay tag states.
  - `FMPReplicatedCountdownTimer` for latency-immune match and round clocks.
- **Multiplayer Physics Extensions**:
  - `UMPPhysicsReplicationHelper` ragdoll network handover, linear/angular damping, and transform error validation.
- **Server Security & Anti-Cheat Validation**:
  - `FMPFireRateValidator` sliding-window trigger automation detection.
  - `FMPMovementExploitDetector` speedhack and teleport exploit detection.
  - `UMPServerSecurityLibrary` damage request and client timestamp drift validation.
- **Async Blueprint Action Proxies**:
  - `UMPAsyncWaitUntilPlayerState`, `UMPAsyncWaitUntilPawnPossessed`, `UMPAsyncWaitUntilActorReplicated`, `UMPAsyncServerRequest`.
- **Network Optimization & Net LOD**:
  - `UMPNetLODComponent` server-only spatial frequency throttling and dynamic dormancy.
- **Modular Gameplay Modules**:
  - `UMPHealthComponent` authoritative health, shield, downed, and revive states.
- **Advanced Editor Module (`MPUtilityEditor`)**:
  - `UMPEditorScannerSubsystem` Editor Subsystem auditing actors for missing replication, invalid RPC ownership, and PIE network emulation.
- **Testing & Verification**:
  - `UMPNetworkStressTester` load simulation benchmark runner.
  - Expanded automation suite from 23 to 36 test suites.
