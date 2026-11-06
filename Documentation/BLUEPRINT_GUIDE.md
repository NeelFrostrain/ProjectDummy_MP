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

#### 3. Client Interaction Execution:
```
[ Input Action Interact ] ──> [ Line Trace By Channel ]
                                 └── Hit Actor ──> [ Try Interact With ] (Target Actor)
```

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
