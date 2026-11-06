# Multiplayer Utility — Character Movement Prediction Guide

## 1. Architecture

`UMPCharacterMovementComponent` extends `UCharacterMovementComponent` using Unreal Engine's native `FSavedMove_Character` pipeline.

### Why this matters:
- **No Desyncs**: Client inputs are stamped, compressed, and sent via `ServerMove`.
- **Local Prediction**: Sprinting, Dashing, Sliding, and Climbing activate instantly on autonomous clients without waiting for server RTT.
- **Server Replay**: On correction, the server replays remaining unacknowledged moves seamlessly.

---

## 2. Using `UMPCharacterMovementComponent`

1. In your Character C++ constructor or Blueprint, use `UMPCharacterMovementComponent`:
```cpp
AMyCharacter::AMyCharacter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UMPCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
}
```

2. Call movement actions directly from Enhanced Input:
- `SetSprinting(true / false)`
- `TriggerDash()`
- `SetSliding(true / false)`
- `SetClimbing(true / false)`

3. Configure Smoothing Profiles for Proxies:
```cpp
MoveComp->ApplySmoothingProfile(EMPNetworkSmoothingProfile::TightFastPaced);
```
Profiles available:
- `Default`: Standard exponential smoothing.
- `TightFastPaced`: 92 unit max smooth distance for competitive shooters.
- `CinematicSmooth`: Linear interpolation for smooth cinematic proxies.
- `HighLatencyConservative`: Extended distance margins for high-ping connections.
