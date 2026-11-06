# Multiplayer Utility — API Reference

> **Note:** The dedicated inventory system is fully encapsulated inside the independent [`MultiplayerInventory`](../../MultiplayerInventory) plugin. `MultiplayerUtility` remains strictly lightweight, stateless, and focused on networking helpers, RPC dispatch, latent gates, and diagnostics. All functions taking `TargetActor` default to **`Self`** in Blueprint graphs.

## 1. Subsystems

### `UMPNetworkRequestSubsystem` (`UWorldSubsystem`)
- `SendNetworkRequest(WorldContext, Request, OnResponse, OutHandle)`
- `GetTotalDispatchedRequests()`
- `GetTotalTimedOutRequests()`
- Delegate: `OnRequestLifecycleEvent`

### `UMPNetStatsSubsystem` (`UWorldSubsystem`)
- `GetGlobalNetStatus(WorldContext)`
- `GetActiveReplicatedActorCount(WorldContext)`
- `GetNetworkFrameRate(WorldContext)`
- `LogFullNetworkSummary(WorldContext)`

### `UMPEditorScannerSubsystem` (`UEditorSubsystem`)
- `AuditActorReplication(Actor, OutReport)`
- `ValidateRPCOwnership(Actor, OutWarnings)`
- `AuditWorldNetworkActors(OutTotal, OutReplicated, OutWithWarnings)`
- `ApplyPIENetworkEmulation(TargetPingMs, PacketLossPercent)`
- `GenerateShowcaseBlueprint(BlueprintPackagePath)` — Automatically populates and saves `BP_MultiplayerUtility_Showcase` with 254 nodes across 27 categorized comment boxes and exports the clipboard text.

---

## 2. Components

### `UMPNetworkRequestComponent`
- `SendRequest(Request, OnResponse, OutHandle)`
- `RegisterServerHandler(RequestAction, Handler)`
- `UnregisterServerHandler(RequestAction)`
- `CancelRequest(RequestId)`

### `UMPCharacterMovementComponent`
- `SetSprinting(bNewSprint)`
- `TriggerDash()`
- `SetSliding(bNewSlide)`
- `SetClimbing(bNewClimb)`
- `ApplySmoothingProfile(Profile)`
- `ValidateMovementStep(PrevLoc, PropLoc, DeltaTime, MaxSpeed, Margin, OutError)`

### `UMPNetLODComponent`
- `ForceEvaluateLOD()`
- Delegate: `OnLODTierChanged`

### `UMPHealthComponent`
- `ApplyDamageServerOnly(Amount, Causer, Instigator)`
- `ApplyHealingServerOnly(Amount)`
- `RevivePlayerServerOnly(Percentage)`
- `SetInvulnerableServerOnly(bInvulnerable)`
- Delegates: `OnHealthChanged`, `OnShieldChanged`, `OnDeath`, `OnDowned`, `OnRevived`

---

## 3. Core Function Libraries

- `UMPNetFunctionLibrary`: 30+ Core network queries (`IsServer`, `IsClient`, `HasAuthority`, `GetPlayerPingMs`, etc.).
- `UMPPlayerLifecycleLibrary`: `SafePossessPawnServerOnly`, `SafeUnpossessPawnServerOnly`, `SpawnAndPossessPlayerPawnServerOnly`, `TransferActorOwnershipSafe`, `RespawnPlayerPawnServerOnly`.
- `UMPServerSecurityLibrary`: `ValidateDamageRequestAuthoritative`, `ValidateClientTimestampDrift`, `ValidateTransactionNonDuplicated`.
- `UMPPhysicsReplicationHelper`: `InitNetworkRagdollSafe`, `PutPhysicsToSleepServerOnly`, `WakePhysicsServerOnly`, `SetNetworkPhysicsDampingSafe`, `ValidatePhysicsTransformError`.
- `UMPReplicationConditions`: `DescribeReplicationCondition`, `HasStateChanged`, `SetDynamicPropertyActiveOverride`.
- `UMPQuantization`: World location quantization, rotation packing, 32-bit and 64-bit bitmasks.
- `UMPTimeMath`: Timestamp wrap math, exponential smoothing, network delta time clamping.
- `UMPFlowControlLibrary`: `BranchByNetMode`, `BranchByNetRole`, `NetworkGate`, `RunOnNextFrame`, `RunAfterServerSeconds`.

---

## 4. Gameplay Helper Functions & Reusable Utilities (`UMPGameplayHelperLibrary`)

### A. Player & Network Utilities
| Function Name | Inputs | Outputs | Category | Authority | Description | Limitations |
|---|---|---|---|---|---|---|
| `GetLocalPlayerState` | `WorldContextObject` | `APlayerState*` | `Multiplayer Utility\|Player` | Any | Resolves the primary local player's PlayerState without manual controller casting. | Returns null on dedicated server. |
| `GetPlayerStateFromActor` | `TargetActor` | `APlayerState*` | `Multiplayer Utility\|Player` | Any | Resolves PlayerState by inspecting Pawns, Controllers, or traversing ownership chains. | TargetActor must be owned by or linked to a player entity. |
| `GetPlayerControllerFromActor` | `TargetActor` | `APlayerController*` | `Multiplayer Utility\|Player` | Any | Resolves PlayerController from any Pawn, PlayerState, or owned actor. | Returns null if actor is unpossessed or AI controlled. |
| `IsPlayerLocallyViewed` | `TargetActor` | `bool` | `Multiplayer Utility\|Player` | Client / Listen | Checks if TargetActor is currently viewed or possessed by any local player controller. | Server-only pawns without local view return false. |
| `GetPlayerControllerIndex` | `PlayerController` | `int32` | `Multiplayer Utility\|Player` | Client / Listen | Returns local split-screen player index (0 for primary, 1+ for split-screen, -1 for remote). | Dedicated servers have no local players. |
| `GetAllRemotePlayerControllers` | `WorldContextObject` | `TArray<APlayerController*>` | `Multiplayer Utility\|Network` | Server | Returns array of all remote connected PlayerControllers (excluding local listen host). | Useful for dedicated/listen broadcast targeting. |
| `IsPlayerConnectionActive` | `PlayerController` | `bool` | `Multiplayer Utility\|Network` | Any | Returns true if the controller has an active, open `UNetConnection`. | Local player controllers return false as they use local viewport connections. |
| `GetPlayerPingFormatted` | `PlayerState` | `FString` | `Multiplayer Utility\|Player` | Any | Formats player ping cleanly into display text (e.g. `"45 ms"` or `"-- ms"`). | Requires replicated PlayerState. |

### B. RPC Utilities
| Function Name | Inputs | Outputs | Category | Authority | Description | Limitations |
|---|---|---|---|---|---|---|
| `CanActorSendServerRPC` | `TargetActor` | `bool`, `OutReason` | `Multiplayer Utility\|RPC` | Client | Verifies whether TargetActor satisfies ownership, connection, and role prerequisites to send a Server RPC. | Diagnoses why an RPC would be dropped. |
| `GetRPCExecutionScope` | `TargetActor` | `FString`, `OutScopeDescription` | `Multiplayer Utility\|RPC` | Any | Returns a human-readable explanation of where RPCs invoked on this actor execute. | Informational diagnostic. |
| `CheckRPCRateLimitGate` | `ContextObject`, `RPCIdentifier`, `CooldownSeconds` | `bool (Allowed)` | `Multiplayer Utility\|RPC` | Any | Throttles client or server events to prevent RPC flood/spam in Blueprints. | Memory footprint resets per game session. |

### C. Replication Utilities
| Function Name | Inputs | Outputs | Category | Authority | Description | Limitations |
|---|---|---|---|---|---|---|
| `SetActorReplicationSafe` | `TargetActor`, `bReplicateActor`, `bReplicateMovement` | `void` | `Multiplayer Utility\|Replication` | Server Authority Only | Safely toggles actor replication and movement replication with authority validation. | No-op if called on client. |
| `SetActorNetDormantSafe` | `TargetActor`, `bDormant` | `void` | `Multiplayer Utility\|Replication` | Server Authority Only | Sets actor dormancy (`DORM_DormantAll` or `DORM_Awake` + `FlushNetDormancy`). | No-op if called on client. |
| `IsActorDormant` | `TargetActor` | `bool` | `Multiplayer Utility\|Replication` | Any | Checks if actor is currently dormant. | True for `DORM_DormantAll` and `DORM_DormantPartial`. |
| `GetActorNetUpdateFrequency` | `TargetActor` | `float` | `Multiplayer Utility\|Replication` | Any | Returns current net update frequency in Hz. | Read-only. |
| `ForceNetUpdateWithAttachedActors` | `TargetActor` | `void` | `Multiplayer Utility\|Replication` | Server Authority Only | Forces immediate network update on target and all attached replicated child actors. | Server-only. |
| `TearOffActorSafe` | `TargetActor` | `void` | `Multiplayer Utility\|Replication` | Server Authority Only | Authoritatively detaches actor from networking so clients retain simulated local copies. | Irreversible during the actor's lifecycle. |
| `IsActorTornOff` | `TargetActor` | `bool` | `Multiplayer Utility\|Replication` | Any | Checks whether actor has been torn off from replication. | Returns `TargetActor->GetTearOff()`. |

### D. Multiplayer Gameplay Utilities
| Function Name | Inputs | Outputs | Category | Authority | Description | Limitations |
|---|---|---|---|---|---|---|
| `CanDamageActorAuthoritative` | `Attacker`, `Victim`, `bAllowFriendlyFire` | `bool` | `Multiplayer Utility\|Gameplay` | Server Authority Only | Evaluates whether damage is permitted based on server authority, self-damage, and team affiliation. | Returns false on client. |
| `ApplyPointDamageServerOnly` | `DamagedActor`, `BaseDamage`, `HitFromDirection`, `HitInfo`, `EventInstigator`, `DamageCauser`, `DamageTypeClass` | `bool` | `Multiplayer Utility\|Gameplay` | Server Authority Only | Server-only wrapper executing `UGameplayStatics::ApplyPointDamage` authoritatively. | Safe no-op on clients. |
| `ApplyRadialDamageServerOnly` | `WorldContext`, `BaseDamage`, `Origin`, `Radius`, `DamageTypeClass`, `IgnoreActors`, `DamageCauser`, `InstigatedBy`, `bDoFullDamage` | `bool` | `Multiplayer Utility\|Gameplay` | Server Authority Only | Server-only wrapper executing `UGameplayStatics::ApplyRadialDamage` authoritatively. | Safe no-op on clients. |
| `SpawnReplicatedActorServerOnly` | `WorldContext`, `ClassToSpawn`, `SpawnTransform`, `CollisionHandling`, `Owner`, `Instigator` | `AActor*` | `Multiplayer Utility\|Gameplay` | Server Authority Only | Spawns a replicated actor on server, configuring ownership and instigator prior to spawn completion. | Safe no-op on clients. |
| `RespawnPlayerPawnAtTransform` | `PlayerController`, `PawnClass`, `SpawnTransform` | `APawn*` | `Multiplayer Utility\|Gameplay` | Server Authority Only | Authoritatively unpossesses/destroys old pawn, spawns new replicated pawn, and possesses it. | Server-only. |
| `FindBestPlayerStart` | `WorldContext`, `PreferredLocation`, `IncomingName` | `AActor*` | `Multiplayer Utility\|Gameplay` | Server Authority Only | Finds the nearest unoccupied `APlayerStart` matching criteria. | Requires player starts in level. |
| `TeleportPawnServerOnly` | `Pawn`, `DestLocation`, `DestRotation`, `bResetVelocity` | `bool` | `Multiplayer Utility\|Gameplay` | Server Authority Only | Authoritative teleport resetting physics velocity and updating movement components smoothly. | Server-only. |
| `SetPlayerScoreServerOnly` | `PlayerState`, `NewScore` | `bool` | `Multiplayer Utility\|Gameplay` | Server Authority Only | Authoritatively modifies player score on the PlayerState. | Server-only. |

### E. Multiplayer Synchronization Utilities
| Function Name | Inputs | Outputs | Category | Authority | Description | Limitations |
|---|---|---|---|---|---|---|
| `GetServerTimeSynced` | `WorldContextObject` | `float` | `Multiplayer Utility\|Synchronization` | Any | Returns synchronized server world time (seconds) using `AGameStateBase::GetServerWorldTimeSeconds()`. | Falls back to local time if GameState unavailable. |
| `GetTimeSinceServerTimestamp` | `WorldContextObject`, `ServerTimestamp` | `float` | `Multiplayer Utility\|Synchronization` | Any | Returns elapsed seconds since an authoritative server event timestamp. | Clamped `>= 0`. |
| `CalculateLatencyCompensatedLocation` | `CurrentLocation`, `Velocity`, `PingSeconds` | `FVector` | `Multiplayer Utility\|Synchronization` | Any | Extrapolates position based on linear velocity and one-way network latency (`Ping * 0.5`). | Linear assumption. |
| `AreTimestampsOrdered` | `EarlierTimestamp`, `LaterTimestamp` | `bool` | `Multiplayer Utility\|Synchronization` | Any | Verifies chronological sequence of two network timestamps. | Pure math. |
| `GetServerTickRate` | `WorldContextObject` | `float` | `Multiplayer Utility\|Synchronization` | Any | Returns current engine tick rate / frame rate. | Informational. |
| `IsServerTickCadence` | `WorldContextObject`, `TickInterval` | `bool` | `Multiplayer Utility\|Synchronization` | Any | Returns true every Nth frame via `GFrameCounter` for staggered distributed polling. | Deterministic tick interval. |

### F. Multiplayer Debugging Utilities
| Function Name | Inputs | Outputs | Category | Authority | Description | Limitations |
|---|---|---|---|---|---|---|
| `GetActorNetworkDebugString` | `TargetActor` | `FString` | `Multiplayer Utility\|Debug` | Any | Formats complete 1-line network summary: `[Actor | Role | RemoteRole | Owner | Dormancy | Freq]`. | Formats clean string. |
| `DrawNetworkRoleDebugString` | `TargetActor`, `WorldOffset`, `TextColor`, `Duration` | `void` | `Multiplayer Utility\|Debug` | Any | Renders overhead world text showing network role and net mode. | Stripped in shipping builds. |
| `LogNetworkEvent` | `ContextObject`, `EventName`, `bServerOnly` | `void` | `Multiplayer Utility\|Debug` | Any | Logs event with `[SERVER]`, `[CLIENT]`, `[LISTEN]`, or `[DEDICATED]` net mode prefix. | Standard UE log. |
| `SimulateNetworkConditionCVar` | `TargetPingMs`, `PacketLossPercent` | `void` | `Multiplayer Utility\|Debug` | Development / Editor | Injects simulated ping lag and packet loss via `Net.PktLag` and `Net.PktLoss` CVars. | PIE / testing only. |
| `DumpReplicationInfoToLog` | `TargetActor` | `void` | `Multiplayer Utility\|Debug` | Any | Dumps actor's net properties, owner, role, dormancy, update rate, and cull distance to `LogNet`. | Diagnostic utility. |

---

## 5. Reusable RPC Gameplay Component (`UMPRPCGameplayComponent`)

Attachable actor component for server-authoritative event handling, client notifications, and multicast cues:
- `SendServerGameplayEvent(EventName, Location, TargetActor)`: Client-to-server RPC with parameter validation.
- `SendClientNotification(Message, Color, Duration)`: Server-to-client reliable notification RPC.
- `BroadcastCosmeticCue(CueName, Location, Rotation)`: Server-to-all multicast unreliable cosmetic cue.
- `BroadcastSoundAtLocation(Sound, Location, Volume, Pitch)`: Multicast sound playback.
- Dynamic Multicast Delegates:
  - `OnServerEventReceived(EventName, Location, TargetActor)`
  - `OnClientNotificationReceived(Message, Color, Duration)`
  - `OnCosmeticCueReceived(CueName, Location, Rotation)`

---

## 6. Replicated Synchronization Timer (`FMPReplicatedSyncTimer`)

Stateless evaluation struct for countdown mechanics:
- `Start(Duration, CurrentServerTime)`: Initiates countdown with server timestamp.
- `Pause(CurrentServerTime)` / `Resume(CurrentServerTime)`: Preserves elapsed duration across pause periods.
- `Stop()`: Resets timer state.
- `GetRemainingTime(CurrentServerTime)`: Returns seconds left until expiration.
- `GetNormalizedProgress(CurrentServerTime)`: Returns clamped `0.0` to `1.0` progress.
- `IsFinished(CurrentServerTime)`: Returns true when duration has expired.

---

## 7. Section N Gameplay Macros (`MPNetMacros.h`)

| Macro | Description | Usage Example |
|---|---|---|
| `MP_CHECK_AUTONOMOUS(Actor)` | Returns immediately if actor is not an Autonomous Proxy. | `MP_CHECK_AUTONOMOUS(this);` |
| `MP_CHECK_AUTONOMOUS_RET(Actor, RetVal)` | Returns `RetVal` if actor is not an Autonomous Proxy. | `MP_CHECK_AUTONOMOUS_RET(this, false);` |
| `MP_CHECK_SIMULATED(Actor)` | Returns immediately if actor is not a Simulated Proxy. | `MP_CHECK_SIMULATED(this);` |
| `MP_CHECK_SIMULATED_RET(Actor, RetVal)` | Returns `RetVal` if actor is not a Simulated Proxy. | `MP_CHECK_SIMULATED_RET(this, false);` |
| `MP_CHECK_NOT_TORN_OFF(Actor)` | Returns immediately if actor has been torn off from networking. | `MP_CHECK_NOT_TORN_OFF(this);` |
| `MP_CHECK_NOT_TORN_OFF_RET(Actor, RetVal)` | Returns `RetVal` if actor has been torn off from networking. | `MP_CHECK_NOT_TORN_OFF_RET(this, false);` |
| `MP_REP_NOTIFY_CHANGED(OldVal, NewVal)` | Conditional guard in `OnRep_` functions skipping logic if unchanged. | `MP_REP_NOTIFY_CHANGED(OldVal, Health) { UpdateHUD(); }` |

