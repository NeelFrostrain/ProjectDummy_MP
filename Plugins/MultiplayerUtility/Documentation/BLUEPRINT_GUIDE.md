# Multiplayer Utility — Blueprint Developer Guide

## 1. Quick Reference & Core Nodes

All nodes are organized in the Blueprint action menu under **`Multiplayer Utility`** and sub-categories:

### Common Blueprint Patterns:

#### 1. Checking Authority and Remote Role:
```
[ Event BeginPlay ] ──> [ Branch by Net Role ]
                           ├── Authority        ──> [ Server Initialization Logic ]
                           ├── Autonomous Proxy ──> [ Local HUD / Input Setup ]
                           └── Simulated Proxy  ──> [ Visual Interpolation Setup ]
```

#### 2. Waiting for Replicated Player State:
```
[ Event BeginPlay ] ──> [ Wait Until PlayerState Available ]
                           ├── Controller: (GetPlayerController)
                           ├── Timeout: 5.0s
                           │
                           ├── On Available ──> (Initialize Scoreboard / Player UI)
                           └── On TimedOut  ──> (Log Network Desync Warning)
```

#### 3. Latent Waiting for Replicated Actor:
```
[ Event BeginPlay ] ──> [ Delay Until Replicated ]
                           ├── Target Actor: (defaults to Self)
                           ├── Timeout: 5.0s
                           │
                           ├── Replicated (Valid) ──> [ Enable Local Abilities / Mesh ]
                           └── Timed Out          ──> [ Log Network Desync Warning ]
```

> **Note on `DefaultToSelf`**: All utility, flow control (`Branch by Net Role`, `Branch Has Authority`), and latent functions that take `TargetActor` automatically default to **`Self`** in Blueprint graphs, exactly like native Actor member functions.

#### 4. Asynchronous Request / Response:
```
[ Async Send Server Request ]
  ├── Request Action: "CraftItem"
  ├── Timeout: 4.0s
  │
  ├── On Success  ──> (Play Crafting Complete SFX)
  ├── On Failed   ──> (Show Error Toast in UI)
  └── On TimedOut ──> (Show Network Retry Dialog)
```
