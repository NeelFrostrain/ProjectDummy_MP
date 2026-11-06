#!/usr/bin/env python3
# Copyright Epic Games, Inc. All Rights Reserved.
"""
Multiplayer Utility - Automated Blueprint Graph Generator
==========================================================
Generates a complete, organized Unreal Engine Blueprint graph containing every
single function, wrapper, math utility, flow-control branch, and replication helper
from the Multiplayer Utility plugin (110+ nodes across 13 categorized comment boxes).

Usage:
------
1. Run from command line (using standard Python or Unreal Engine Python):
       python Plugins/MultiplayerUtility/Scripts/place_nodes_in_blueprint.py
   or with Engine Python:
       & "<UE_ROOT>\Engine\Binaries\ThirdParty\Python3\Win64\python.exe" "Plugins\MultiplayerUtility\Scripts\place_nodes_in_blueprint.py"

2. The script will automatically copy the full graph text directly into your Windows Clipboard.
3. Open ANY Blueprint in Unreal Editor (Actor, Character, GameMode, Blueprint Function, etc.).
4. Click inside the Event Graph or Function Graph, and press Ctrl+V!
5. All 110+ nodes will instantly appear, beautifully laid out and categorized into color-coded comment boxes!
"""

import sys
import os
import uuid
import ctypes
from ctypes import wintypes
from typing import List, Dict, Any, Tuple

# ==============================================================================
# Complete Registry of all 110+ Multiplayer Utility UFUNCTIONs
# ==============================================================================

CATEGORIES: List[Dict[str, Any]] = [
    {
        "title": "1. Network State & Net Mode Queries",
        "color": (0.10, 0.45, 0.90, 1.0),
        "module": "MPUtilityRuntime",
        "nodes": [
            ("MPNetFunctionLibrary", "IsServer", True),
            ("MPNetFunctionLibrary", "IsNetworkGame", True),
            ("MPNetFunctionLibrary", "IsStandalone", True),
            ("MPNetFunctionLibrary", "IsDedicatedServer", True),
            ("MPNetFunctionLibrary", "IsListenServer", True),
            ("MPNetFunctionLibrary", "IsClient", True),
            ("MPNetFunctionLibrary", "HasAuthority", True),
            ("MPNetFunctionLibrary", "IsAutonomousProxy", True),
            ("MPNetFunctionLibrary", "IsSimulatedProxy", True),
            ("MPNetFunctionLibrary", "IsLocallyControlled", True),
            ("MPPawnControllerWrapper", "IsPlayerControlled", True),
            ("MPPawnControllerWrapper", "IsBotControlled", True),
            ("MPPawnControllerWrapper", "IsLocalPlayerController", True),
        ]
    },
    {
        "title": "2. Actor Replication & Lifecycle Control",
        "color": (0.12, 0.65, 0.35, 1.0),
        "module": "MPUtilityRuntime",
        "nodes": [
            ("MPNetFunctionLibrary", "IsRPCSafeToCall", True),
            ("MPNetFunctionLibrary", "ExplainWhyRPCDropped", True),
            ("MPNetFunctionLibrary", "CanSendServerRPC", True),
            ("MPNetFunctionLibrary", "GetRPCOwnershipChain", True),
            ("MPActorNetworkWrapper", "SetReplicates", False),
            ("MPActorNetworkWrapper", "SetReplicateMovement", False),
            ("MPActorNetworkWrapper", "SetNetOwner", False),
            ("MPActorNetworkWrapper", "GetTopLevelNetOwner", True),
            ("MPActorNetworkWrapper", "GetLogicalOwnerSafe", True),
            ("MPActorNetworkWrapper", "SetNetDormancy", False),
            ("MPActorNetworkWrapper", "FlushNetDormancy", False),
            ("MPActorNetworkWrapper", "ForceNetUpdate", False),
            ("MPActorNetworkWrapper", "SetNetUpdateFrequency", False),
            ("MPActorNetworkWrapper", "SetMinNetUpdateFrequency", False),
            ("MPActorNetworkWrapper", "SetNetCullDistance", False),
            ("MPActorNetworkWrapper", "SetAlwaysRelevant", False),
            ("MPActorNetworkWrapper", "SetOnlyRelevantToOwner", False),
        ]
    },
    {
        "title": "3. Subobject & Component Replication",
        "color": (0.08, 0.60, 0.70, 1.0),
        "module": "MPUtilityRuntime",
        "nodes": [
            ("MPComponentNetworkWrapper", "SetComponentReplicated", False),
            ("MPComponentNetworkWrapper", "IsComponentReplicated", True),
            ("MPComponentNetworkWrapper", "IsComponentNetSimulating", True),
            ("MPComponentNetworkWrapper", "HasComponentAuthority", True),
            ("MPComponentNetworkWrapper", "RegisterReplicatedSubobjectSafe", False),
            ("MPComponentNetworkWrapper", "UnregisterReplicatedSubobjectSafe", False),
            ("MPComponentNetworkWrapper", "IsSubobjectReplicated", True),
            ("MPComponentNetworkWrapper", "GetReplicatedComponentsOfActor", True),
            ("MPComponentNetworkWrapper", "SetComponentReplicatedRecursive", False),
            ("MPComponentNetworkWrapper", "PrintReplicatedSubobjects", True),
        ]
    },
    {
        "title": "4. Movement & Physics Replication",
        "color": (0.85, 0.45, 0.10, 1.0),
        "module": "MPUtilityRuntime",
        "nodes": [
            ("MPMovementPhysicsWrapper", "SetSimulatePhysicsServerOnly", False),
            ("MPMovementPhysicsWrapper", "ApplyImpulseServerOnly", False),
            ("MPMovementPhysicsWrapper", "GetReplicatedVelocitySafe", True),
            ("MPMovementPhysicsWrapper", "IsPhysicsReplicated", True),
            ("MPMovementPhysicsWrapper", "SnapOrSmoothTransform", True),
            ("MPMovementPhysicsWrapper", "GetMovementErrorDistance", True),
            ("MPMovementPhysicsWrapper", "IsCharacterMovingAuthoritatively", True),
            ("MPMovementPhysicsWrapper", "GetNetworkSmoothingMode", True),
            ("MPMovementPhysicsWrapper", "SetNetworkSmoothingModeSafe", False),
        ]
    },
    {
        "title": "5. Connection Quality & Network Diagnostics",
        "color": (0.55, 0.25, 0.85, 1.0),
        "module": "MPUtilityRuntime",
        "nodes": [
            ("MPNetFunctionLibrary", "GetPlayerPingMs", True),
            ("MPPawnControllerWrapper", "GetPlayerPingSeconds", True),
            ("MPNetFunctionLibrary", "GetConnectionQualityEnum", True),
            ("MPNetFunctionLibrary", "IsConnectionUnstable", True),
            ("MPNetFunctionLibrary", "GetAveragePingOverNSeconds", True),
            ("MPNetFunctionLibrary", "GetPingTrend", True),
            ("MPNetFunctionLibrary", "GetPacketLossPercentSafe", True),
            ("MPNetFunctionLibrary", "ShouldReduceNetworkLoad", True),
            ("MPNetFunctionLibrary", "EstimateBandwidthPerPlayer", True),
            ("MPNetFunctionLibrary", "GetPlayerIPAddress", True),
            ("MPNetFunctionLibrary", "GetPlayerUniqueIdString", True),
        ]
    },
    {
        "title": "6. Player & Spatial Queries",
        "color": (0.75, 0.60, 0.10, 1.0),
        "module": "MPUtilityRuntime",
        "nodes": [
            ("MPNetFunctionLibrary", "GetClosestPlayerPawn", True),
            ("MPNetFunctionLibrary", "GetPlayersInSphere", True),
            ("MPNetFunctionLibrary", "GetAllPlayerPawns", True),
            ("MPNetFunctionLibrary", "GetAllPlayerControllers", True),
            ("MPNetFunctionLibrary", "GetAllRemotePlayers", True),
            ("MPNetFunctionLibrary", "GetDistanceToNearestPlayer", True),
            ("MPNetFunctionLibrary", "IsAnyPlayerWithinRange", True),
            ("MPNetFunctionLibrary", "GetPlayersSortedByDistance", True),
            ("MPNetFunctionLibrary", "GetFurthestPlayer", True),
            ("MPNetFunctionLibrary", "GetPlayersInCone", True),
            ("MPNetFunctionLibrary", "GetPlayersInRadiusWithLineOfSight", False),
            ("MPNetFunctionLibrary", "GetRandomPlayer", True),
            ("MPNetFunctionLibrary", "GetAlivePlayersOnly", True),
            ("MPNetFunctionLibrary", "GetActorsWithTagReplicatedSafe", True),
            ("MPNetFunctionLibrary", "HasAnyTagSafe", True),
            ("MPNetFunctionLibrary", "MakeNetUniqueName", True),
            ("MPNetFunctionLibrary", "IsValidPlayerName", True),
            ("MPNetFunctionLibrary", "ComparePlayerNamesCaseInsensitive", True),
        ]
    },
    {
        "title": "7. Team Affiliation & Relevancy",
        "color": (0.85, 0.20, 0.50, 1.0),
        "module": "MPUtilityRuntime",
        "nodes": [
            ("MPNetFunctionLibrary", "AreTeammates", True),
            ("MPNetFunctionLibrary", "AreEnemies", True),
            ("MPNetFunctionLibrary", "GetActorsByTeam", True),
            ("MPNetFunctionLibrary", "GetTeamOfActorSafe", True),
            ("MPNetFunctionLibrary", "IsActorRelevantToPlayerSafe", True),
        ]
    },
    {
        "title": "8. Authoritative Server Validation",
        "color": (0.85, 0.15, 0.15, 1.0),
        "module": "MPUtilityRuntime",
        "nodes": [
            ("MPNetFunctionLibrary", "ValidateTransformFromClient", False),
            ("MPNetFunctionLibrary", "ValidateTimestampFromClient", False),
            ("MPNetFunctionLibrary", "ValidateFloatRange", True),
            ("MPNetFunctionLibrary", "ValidateVectorNotNaN", True),
            ("MPNetFunctionLibrary", "ValidateEnumValue", True),
            ("MPNetFunctionLibrary", "ClampClientInputVector", True),
            ("MPNetFunctionLibrary", "IsValidSpawnLocation", False),
            ("MPNetFunctionLibrary", "IsPathClearServer", False),
            ("MPNetFunctionLibrary", "SanitizePlayerName", True),
        ]
    },
    {
        "title": "9. Component & Spawn Helpers",
        "color": (0.40, 0.70, 0.15, 1.0),
        "module": "MPUtilityRuntime",
        "nodes": [
            ("MPNetFunctionLibrary", "AddReplicatedComponentServerOnly", False),
            ("MPNetFunctionLibrary", "FindComponentSafe", True),
            ("MPNetFunctionLibrary", "SpawnActorDeferredServerOnly", False),
            ("MPNetFunctionLibrary", "FinishSpawningActorServerOnly", False),
            ("MPNetFunctionLibrary", "SpawnAtFreePlayerStartSafe", False),
            ("MPNetFunctionLibrary", "TeleportSafeServer", False),
            ("MPNetFunctionLibrary", "GetOwningPlayerStateSafe", True),
            ("MPNetFunctionLibrary", "GetOwningPawnSafe", True),
            ("MPNetFunctionLibrary", "ApplyNetPresetToActor", False),
        ]
    },
    {
        "title": "10. Level Streaming & World Helpers",
        "color": (0.20, 0.30, 0.65, 1.0),
        "module": "MPUtilityRuntime",
        "nodes": [
            ("MPWorldSessionWrapper", "SafeServerTravel", False),
            ("MPWorldSessionWrapper", "GetConnectedPlayerCount", True),
            ("MPWorldSessionWrapper", "GetServerWorldTime", True),
            ("MPWorldSessionWrapper", "IsLevelLoadedForAllPlayers", False),
            ("MPWorldSessionWrapper", "GetPlayersInStreamingLevel", True),
            ("MPWorldSessionWrapper", "IsActorInLoadedLevel", True),
            ("MPWorldSessionWrapper", "GetLevelNameOfActor", True),
            ("MPWorldSessionWrapper", "IsSeamlessTraveling", True),
            ("MPWorldSessionWrapper", "GetCurrentMapNameClean", True),
            ("MPWorldSessionWrapper", "GetTravelOptionsFromURL", True),
            ("MPPawnControllerWrapper", "SafeClientTravel", False),
        ]
    },
    {
        "title": "11. Network Math, Quantization & Bitwise",
        "color": (0.10, 0.70, 0.60, 1.0),
        "module": "MPUtilityRuntime",
        "nodes": [
            ("MPQuantization", "WrapAngleSafe", True),
            ("MPQuantization", "IsVectorWithinTolerance", True),
            ("MPQuantization", "IsRotatorWithinTolerance", True),
            ("MPQuantization", "HashNameToNetInt", True),
            ("MPQuantization", "PackTwoFloatsToInt", True),
            ("MPQuantization", "UnpackTwoFloatsFromInt", True),
            ("MPQuantization", "SetBit32", True),
            ("MPQuantization", "ClearBit32", True),
            ("MPQuantization", "TestBit32", True),
            ("MPQuantization", "SetBit64", True),
            ("MPQuantization", "ClearBit64", True),
            ("MPQuantization", "TestBit64", True),
            ("MPTimeMath", "IsTimestampNewer", True),
            ("MPTimeMath", "SequenceDistance16", True),
            ("MPTimeMath", "SequenceDistance32", True),
            ("MPTimeMath", "ClampDeltaTimeForNetwork", True),
            ("MPTimeMath", "LerpNetValue", True),
            ("MPTimeMath", "ExponentialSmoothFloat", True),
            ("MPTimeMath", "ExponentialSmoothVector", True),
        ]
    },
    {
        "title": "12. Flow-Control Execution & Latent Nodes",
        "color": (0.50, 0.15, 0.75, 1.0),
        "module": "MPUtilityRuntime",
        "nodes": [
            ("MPFlowControlLibrary", "BranchByNetRole", False),
            ("MPFlowControlLibrary", "BranchByNetMode", False),
            ("MPFlowControlLibrary", "BranchIsLocallyControlled", False),
            ("MPFlowControlLibrary", "BranchHasAuthority", False),
            ("MPFlowControlLibrary", "NetworkGate", False),
            ("MPFlowControlLibrary", "RunOnNextFrame", False),
            ("MPFlowControlLibrary", "RunAfterServerSeconds", False),
            ("MPFlowControlLibrary", "CancelPendingByKey", True),
            ("MPFlowControlLibrary", "DoOnServerOnlyAfterDelay", False),
            ("MPFlowControlLibrary", "CallOncePerPlayer", False),
            ("MPLatentActions", "DelayUntilReplicated", False),
            ("MPLatentActions", "WaitServerSeconds", False),
        ]
    },
    {
        "title": "13. Replicated Components & Presets",
        "color": (0.35, 0.40, 0.45, 1.0),
        "module": "MPUtilityRuntime",
        "nodes": [
            ("MPAutoDormancyComponent", "WakeFromDormancy", False),
            ("MPNetPresetAsset", "ApplyToActor", False),
        ]
    },
    {
        "title": "14. RPC Request / Response Framework",
        "color": (0.35, 0.20, 0.85, 1.0),
        "module": "MPUtilityRuntime",
        "nodes": [
            ("UMPNetworkRequestSubsystem", "SendNetworkRequest", False),
            ("UMPNetworkRequestSubsystem", "GetTotalDispatchedRequests", True),
            ("UMPNetworkRequestSubsystem", "GetTotalTimedOutRequests", True),
            ("UMPNetworkRequestComponent", "SendRequest", False),
            ("UMPNetworkRequestComponent", "CancelRequest", False),
            ("UMPNetworkRequestComponent", "RegisterServerHandler", False),
            ("UMPNetworkRequestComponent", "UnregisterServerHandler", False),
        ]
    },
    {
        "title": "15. Multiplayer Interaction Framework",
        "color": (0.10, 0.75, 0.50, 1.0),
        "module": "MPUtilityRuntime",
        "nodes": [
            ("UMPInteractionComponent", "TryInteractWith", False),
            ("UMPInteractionComponent", "CancelCurrentInteraction", False),
            ("UMPInteractionComponent", "ValidateInteractionAuthoritative", True),
        ]
    },
    {
        "title": "16. Player Lifecycle Management",
        "color": (0.20, 0.50, 0.80, 1.0),
        "module": "MPUtilityRuntime",
        "nodes": [
            ("UMPPlayerLifecycleLibrary", "SafePossessPawnServerOnly", False),
            ("UMPPlayerLifecycleLibrary", "SafeUnpossessPawnServerOnly", False),
            ("UMPPlayerLifecycleLibrary", "SpawnAndPossessPlayerPawnServerOnly", False),
            ("UMPPlayerLifecycleLibrary", "TransferActorOwnershipSafe", False),
            ("UMPPlayerLifecycleLibrary", "SetPlayerSpectatorStateServerOnly", False),
            ("UMPPlayerLifecycleLibrary", "RespawnPlayerPawnServerOnly", False),
            ("UMPPlayerLifecycleLibrary", "CleanupPlayerPawnSafe", False),
        ]
    },
    {
        "title": "17. Character Movement Prediction Pipeline",
        "color": (0.95, 0.40, 0.10, 1.0),
        "module": "MPUtilityRuntime",
        "nodes": [
            ("UMPCharacterMovementComponent", "SetSprinting", False),
            ("UMPCharacterMovementComponent", "TriggerDash", False),
            ("UMPCharacterMovementComponent", "SetSliding", False),
            ("UMPCharacterMovementComponent", "SetClimbing", False),
            ("UMPCharacterMovementComponent", "IsSprinting", True),
            ("UMPCharacterMovementComponent", "IsSliding", True),
            ("UMPCharacterMovementComponent", "IsClimbing", True),
            ("UMPCharacterMovementComponent", "ApplySmoothingProfile", False),
            ("UMPCharacterMovementComponent", "ValidateMovementStep", True),
            ("UMPCharacterMovementComponent", "GetClientCorrectionCount", True),
        ]
    },
    {
        "title": "18. Server Security & Anti-Cheat Validation",
        "color": (0.90, 0.15, 0.20, 1.0),
        "module": "MPUtilityRuntime",
        "nodes": [
            ("UMPServerSecurityLibrary", "ValidateDamageRequestAuthoritative", True),
            ("UMPServerSecurityLibrary", "ValidateClientTimestampDrift", True),
            ("UMPServerSecurityLibrary", "ValidateTransactionNonDuplicated", False),
        ]
    },
    {
        "title": "19. Network Optimization & Dynamic Net LOD",
        "color": (0.15, 0.70, 0.30, 1.0),
        "module": "MPUtilityRuntime",
        "nodes": [
            ("UMPNetLODComponent", "ForceEvaluateLOD", False),
        ]
    },
    {
        "title": "20. Modular Health & Vitality Authority",
        "color": (0.85, 0.30, 0.40, 1.0),
        "module": "MPUtilityRuntime",
        "nodes": [
            ("UMPHealthComponent", "ApplyDamageServerOnly", False),
            ("UMPHealthComponent", "ApplyHealingServerOnly", False),
            ("UMPHealthComponent", "RevivePlayerServerOnly", False),
            ("UMPHealthComponent", "SetInvulnerableServerOnly", False),
            ("UMPHealthComponent", "IsHealthAlive", True),
            ("UMPHealthComponent", "IsDowned", True),
            ("UMPHealthComponent", "IsDead", True),
        ]
    },
    {
        "title": "21. Multiplayer Physics Synchronization",
        "color": (0.60, 0.30, 0.85, 1.0),
        "module": "MPUtilityRuntime",
        "nodes": [
            ("UMPPhysicsReplicationHelper", "InitNetworkRagdollSafe", False),
            ("UMPPhysicsReplicationHelper", "PutPhysicsToSleepServerOnly", False),
            ("UMPPhysicsReplicationHelper", "WakePhysicsServerOnly", False),
            ("UMPPhysicsReplicationHelper", "SetNetworkPhysicsDampingSafe", False),
            ("UMPPhysicsReplicationHelper", "ValidatePhysicsTransformError", True),
        ]
    },
    {
        "title": "22. Advanced Replication & Conditions",
        "color": (0.80, 0.65, 0.10, 1.0),
        "module": "MPUtilityRuntime",
        "nodes": [
            ("UMPReplicationConditions", "DescribeReplicationCondition", True),
        ]
    },
    {
        "title": "23. Multiplayer Player & Network Helpers",
        "color": (0.15, 0.55, 0.90, 1.0),
        "module": "MPUtilityRuntime",
        "nodes": [
            ("UMPGameplayHelperLibrary", "GetLocalPlayerState", True),
            ("UMPGameplayHelperLibrary", "GetPlayerStateFromActor", True),
            ("UMPGameplayHelperLibrary", "GetPlayerControllerFromActor", True),
            ("UMPGameplayHelperLibrary", "IsPlayerLocallyViewed", True),
            ("UMPGameplayHelperLibrary", "GetPlayerControllerIndex", True),
            ("UMPGameplayHelperLibrary", "GetAllRemotePlayerControllers", True),
            ("UMPGameplayHelperLibrary", "IsPlayerConnectionActive", True),
            ("UMPGameplayHelperLibrary", "GetPlayerPingFormatted", True),
        ]
    },
    {
        "title": "24. Multiplayer RPC Dispatch & Rate Limiting",
        "color": (0.75, 0.25, 0.85, 1.0),
        "module": "MPUtilityRuntime",
        "nodes": [
            ("UMPGameplayHelperLibrary", "CanActorSendServerRPC", True),
            ("UMPGameplayHelperLibrary", "GetRPCExecutionScope", True),
            ("UMPGameplayHelperLibrary", "CheckRPCRateLimitGate", False),
            ("UMPRPCGameplayComponent", "SendServerGameplayEvent", False),
            ("UMPRPCGameplayComponent", "SendClientNotification", False),
            ("UMPRPCGameplayComponent", "BroadcastCosmeticCue", False),
            ("UMPRPCGameplayComponent", "BroadcastSoundAtLocation", False),
        ]
    },
    {
        "title": "25. Multiplayer Replication & Dormancy Helpers",
        "color": (0.10, 0.70, 0.45, 1.0),
        "module": "MPUtilityRuntime",
        "nodes": [
            ("UMPGameplayHelperLibrary", "SetActorReplicationSafe", False),
            ("UMPGameplayHelperLibrary", "SetActorNetDormantSafe", False),
            ("UMPGameplayHelperLibrary", "IsActorDormant", True),
            ("UMPGameplayHelperLibrary", "GetActorNetUpdateFrequency", True),
            ("UMPGameplayHelperLibrary", "ForceNetUpdateWithAttachedActors", False),
            ("UMPGameplayHelperLibrary", "TearOffActorSafe", False),
            ("UMPGameplayHelperLibrary", "IsActorTornOff", True),
        ]
    },
    {
        "title": "26. Multiplayer Combat & Authoritative Gameplay",
        "color": (0.90, 0.20, 0.25, 1.0),
        "module": "MPUtilityRuntime",
        "nodes": [
            ("UMPGameplayHelperLibrary", "CanDamageActorAuthoritative", True),
            ("UMPGameplayHelperLibrary", "ApplyPointDamageServerOnly", False),
            ("UMPGameplayHelperLibrary", "ApplyRadialDamageServerOnly", False),
            ("UMPGameplayHelperLibrary", "SpawnReplicatedActorServerOnly", False),
            ("UMPGameplayHelperLibrary", "RespawnPlayerPawnAtTransform", False),
            ("UMPGameplayHelperLibrary", "FindBestPlayerStart", False),
            ("UMPGameplayHelperLibrary", "TeleportPawnServerOnly", False),
            ("UMPGameplayHelperLibrary", "SetPlayerScoreServerOnly", False),
        ]
    },
    {
        "title": "27. Multiplayer Clock Synchronization & Latency",
        "color": (0.20, 0.75, 0.80, 1.0),
        "module": "MPUtilityRuntime",
        "nodes": [
            ("UMPGameplayHelperLibrary", "GetServerTimeSynced", True),
            ("UMPGameplayHelperLibrary", "GetTimeSinceServerTimestamp", True),
            ("UMPGameplayHelperLibrary", "CalculateLatencyCompensatedLocation", True),
            ("UMPGameplayHelperLibrary", "AreTimestampsOrdered", True),
            ("UMPGameplayHelperLibrary", "GetServerTickRate", True),
            ("UMPGameplayHelperLibrary", "IsServerTickCadence", True),
        ]
    },
    {
        "title": "28. Multiplayer Network Diagnostics & Debugging",
        "color": (0.85, 0.50, 0.15, 1.0),
        "module": "MPUtilityRuntime",
        "nodes": [
            ("UMPGameplayHelperLibrary", "GetActorNetworkDebugString", True),
            ("UMPGameplayHelperLibrary", "DrawNetworkRoleDebugString", False),
            ("UMPGameplayHelperLibrary", "LogNetworkEvent", False),
            ("UMPGameplayHelperLibrary", "SimulateNetworkConditionCVar", False),
            ("UMPGameplayHelperLibrary", "DumpReplicationInfoToLog", False),
        ]
    }
]

# ==============================================================================
# Clipboard Setup (64-bit Windows Safe)
# ==============================================================================

def set_windows_clipboard(text: str) -> bool:
    """Copies text directly to Windows Clipboard using win32 API via ctypes."""
    try:
        user32 = ctypes.windll.user32
        kernel32 = ctypes.windll.kernel32

        CF_UNICODETEXT = 13
        GMEM_MOVEABLE = 0x0002

        kernel32.GlobalAlloc.argtypes = [wintypes.UINT, ctypes.c_size_t]
        kernel32.GlobalAlloc.restype = wintypes.HGLOBAL
        kernel32.GlobalLock.argtypes = [wintypes.HGLOBAL]
        kernel32.GlobalLock.restype = ctypes.c_void_p
        kernel32.GlobalUnlock.argtypes = [wintypes.HGLOBAL]
        user32.OpenClipboard.argtypes = [wintypes.HWND]
        user32.SetClipboardData.argtypes = [wintypes.UINT, wintypes.HANDLE]

        if not user32.OpenClipboard(None):
            return False

        try:
            user32.EmptyClipboard()
            data = text.encode('utf-16-le') + b'\x00\x00'
            h_mem = kernel32.GlobalAlloc(GMEM_MOVEABLE, len(data))
            if not h_mem:
                return False
            p_mem = kernel32.GlobalLock(h_mem)
            if not p_mem:
                kernel32.GlobalFree(h_mem)
                return False
            ctypes.memmove(p_mem, data, len(data))
            kernel32.GlobalUnlock(h_mem)
            user32.SetClipboardData(CF_UNICODETEXT, h_mem)
            return True
        finally:
            user32.CloseClipboard()
    except Exception as e:
        print(f"[WARN] Could not set Windows clipboard: {e}")
        return False

# ==============================================================================
# Graph Builder
# ==============================================================================

def generate_blueprint_t3d() -> Tuple[str, int]:
    """Generates complete Unreal Engine Blueprint graph clipboard text."""
    lines: List[str] = []
    node_index = 0

    # Layout Parameters
    col_width = 1100
    col_padding = 120
    row_padding = 100
    node_y_spacing = 150
    node_x_indent = 50

    # Column mappings (arrange all 28 categories across 10 organized vertical columns)
    columns = [
        [0, 1],        # Column 0: Role & Replication Control
        [2, 3],        # Column 1: Subobjects & Movement
        [4, 5, 6],     # Column 2: Connection Quality, Spatial Queries, Team Affiliation
        [7, 8, 9],     # Column 3: Server Validation, Spawning, World Helpers
        [10, 11, 12],  # Column 4: Math & Quantization, Flow Control, Components
        [13, 14, 15],  # Column 5: RPC Requests, Interaction, Player Lifecycle
        [16, 17, 18],  # Column 6: Movement Prediction, Security Anti-Cheat, Net LOD
        [19, 20, 21],  # Column 7: Health & Vitality, Physics Sync, Replication Conditions
        [22, 23, 24],  # Column 8: Player/Net Helpers, RPC Dispatch, Replication Dormancy
        [25, 26, 27],  # Column 9: Combat/Damage Authority, Clock Sync, Network Diagnostics
    ]

    total_nodes = 0

    for col_idx, cat_indices in enumerate(columns):
        col_x = col_idx * (col_width + col_padding)
        current_y = 0

        for cat_idx in cat_indices:
            cat = CATEGORIES[cat_idx]
            cat_title = cat["title"]
            r, g, b, a = cat["color"]
            nodes = cat["nodes"]
            module_name = cat["module"]

            comment_guid = uuid.uuid4().hex.upper()
            comment_name = f"EdGraphNode_Comment_{node_index}"
            node_index += 1

            # Calculate box height based on node count
            box_header = 70
            box_height = box_header + (len(nodes) * node_y_spacing) + 60
            box_width = col_width

            # Emit Comment Box
            lines.append(f'Begin Object Class=/Script/UnrealEd.EdGraphNode_Comment Name="{comment_name}"')
            lines.append(f'   NodePosX={col_x}')
            lines.append(f'   NodePosY={current_y}')
            lines.append(f'   NodeWidth={box_width}')
            lines.append(f'   NodeHeight={box_height}')
            lines.append(f'   NodeComment="{cat_title}"')
            lines.append(f'   CommentColor=(R={r:.6f},G={g:.6f},B={b:.6f},A={a:.6f})')
            lines.append(f'   NodeGuid={comment_guid}')
            lines.append('End Object')

            # Emit CallFunction nodes inside the box
            for i, (class_name, func_name, b_is_pure) in enumerate(nodes):
                total_nodes += 1
                node_guid = uuid.uuid4().hex.upper()
                call_node_name = f"K2Node_CallFunction_{node_index}"
                node_index += 1

                node_x = col_x + node_x_indent
                node_y = current_y + box_header + (i * node_y_spacing)

                lines.append(f'Begin Object Class=/Script/BlueprintGraph.K2Node_CallFunction Name="{call_node_name}"')
                lines.append(f'   FunctionReference=(MemberParent=Class\'"/Script/{module_name}.{class_name}"\',MemberName="{func_name}")')
                lines.append(f'   NodePosX={node_x}')
                lines.append(f'   NodePosY={node_y}')
                lines.append(f'   NodeGuid={node_guid}')
                lines.append('End Object')

            current_y += box_height + row_padding

    return "\n".join(lines), total_nodes

# ==============================================================================
# Main Runner
# ==============================================================================

def main():
    print("=" * 80)
    print("  MULTIPLAYER UTILITY - AUTOMATED BLUEPRINT GRAPH GENERATOR")
    print("=" * 80)

    t3d_text, node_count = generate_blueprint_t3d()
    print(f"[*] Generated Blueprint graph containing {node_count} nodes across {len(CATEGORIES)} categorized comment boxes.")

    # 1. Determine safe directory for output file
    plugin_dir = None
    try:
        import unreal  # type: ignore
        plugin_dir = os.path.normpath(os.path.join(unreal.Paths.project_plugins_dir(), "MultiplayerUtility"))
    except Exception:
        pass

    if not plugin_dir or not os.path.exists(plugin_dir):
        # Look relative to current working directory or file if present
        cwd = os.getcwd()
        candidate = os.path.join(cwd, "Plugins", "MultiplayerUtility")
        if os.path.exists(candidate):
            plugin_dir = candidate
        elif os.path.exists(os.path.join(cwd, "MultiplayerUtility.uplugin")):
            plugin_dir = cwd
        else:
            plugin_dir = cwd

    output_file = os.path.join(plugin_dir, "MultiplayerUtility_BlueprintGraph_Clipboard.txt")

    try:
        with open(output_file, "w", encoding="utf-8") as f:
            f.write(t3d_text)
        print(f"[+] Saved formatted Blueprint graph to:\n    {output_file}")
    except Exception as io_err:
        print(f"[!] Note on saving text file: {io_err}")

    # 2. Copy to Windows Clipboard
    b_clipboard_ok = set_windows_clipboard(t3d_text)
    if b_clipboard_ok:
        print("\n" + "#" * 80)
        print(f"  [SUCCESS] All {node_count} nodes have been copied directly to your CLIPBOARD!")
        print("#" * 80)
        print("\n>> HOW TO USE:")
        print("   1. In Unreal Editor, open ANY Blueprint (Actor, Character, GameMode, etc.).")
        print("   2. Click inside the Event Graph.")
        print("   3. Press Ctrl+V (Paste).")
        print(f"   4. All {node_count} nodes and {len(CATEGORIES)} categorized comment boxes will instantly appear!")
    else:
        print("\n[!] Clipboard copy was not available directly.")
        print(f"    You can open '{output_file}', press Ctrl+A, Ctrl+C, then press Ctrl+V in Unreal!")

    # 3. If running inside Unreal Editor Python: Automatically populate Showcase Blueprint!
    try:
        import unreal  # type: ignore
        print("\n[*] Unreal Engine Editor detected! Setting up Showcase Blueprint...")
        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
        package_path = "/Game/Blueprints"
        asset_name = "BP_MultiplayerUtility_Showcase"
        full_path = f"{package_path}/{asset_name}"

        bp_asset = None
        if not unreal.EditorAssetLibrary.does_asset_exist(full_path):
            factory = unreal.BlueprintFactory()
            factory.set_editor_property("parent_class", unreal.Actor)
            bp_asset = asset_tools.create_asset(asset_name, package_path, unreal.Blueprint, factory)
            print(f"[+] Created showcase Blueprint: {full_path}")
        else:
            bp_asset = unreal.EditorAssetLibrary.load_asset(full_path)
            print(f"[*] Loaded existing Blueprint: {full_path}")

        # Programmatically place all nodes directly into the Blueprint EventGraph!
        b_inserted = False
        try:
            if hasattr(unreal, "MPEditorScannerSubsystem"):
                b_inserted = unreal.MPEditorScannerSubsystem.populate_blueprint_graph_from_text(bp_asset, t3d_text)
                if b_inserted:
                    print(f"[SUCCESS] Programmatically inserted all {node_count} nodes directly into {asset_name} EventGraph!")
                    unreal.EditorAssetLibrary.save_loaded_asset(bp_asset)
        except Exception as pop_err:
            print(f"[*] Automated node insertion notice: {pop_err}")

        # Open asset in editor window
        subsystem = unreal.get_editor_subsystem(unreal.AssetEditorSubsystem)
        if subsystem and bp_asset:
            subsystem.open_editor_for_assets([bp_asset])
            print(f"[*] Opened {asset_name} in Blueprint Editor!")

        if not b_inserted:
            print("[*] Tip: If nodes are not yet in the graph, click the EventGraph grid and press Ctrl+V to paste them!")

    except (ImportError, Exception) as ex:
        pass

    print("\n" + "=" * 80)

if __name__ == "__main__":
    main()

