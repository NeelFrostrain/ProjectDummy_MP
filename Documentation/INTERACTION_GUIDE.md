# Multiplayer Utility — Interaction Framework Guide

## 1. Overview

The Multiplayer Interaction Framework provides:
- Server-authoritative interaction validation (reach distance, obstacle line-of-sight).
- Interface-driven interactable objects (`IMPInteractionInterface`).
- Rate-limiting cooldowns per interactor.
- Client animation state synchronization.

---

## 2. Setting Up Interactable World Objects

1. Open your interactable Actor (e.g. Chest, Door, Lever).
2. Add the interface **`MPInteractionInterface`** in Class Settings.
3. Implement the interface events:
   - **`CanInteract`**: Return `true` if interaction is permitted, or `false` with a failure reason.
   - **`OnInteractServer`**: Executes server-authoritative gameplay logic (e.g. add inventory, open door).
   - **`OnInteractClient`**: Plays local sound effects, particle effects, or HUD updates on the interacting client.

---

## 3. Player Interaction Setup

1. Add **`UMPInteractionComponent`** to your Player Character.
2. In your input action (e.g. `IA_Interact`):
   - Trace forward from the camera or pawn to find the target actor.
   - Call `TryInteractWith(TargetActor)`.
3. The component handles distance validation, line-of-sight checks, server RPC invocation, and result delegates.
