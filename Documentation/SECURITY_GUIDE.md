# Multiplayer Utility — Server Security & Anti-Cheat Validation

## 1. Golden Rules of Multiplayer Security

1. **Never trust client values**: Health, gold, damage amount, and cooldowns must never be accepted directly from client RPC parameters.
2. **Authoritative verification**: The server must verify that the player has line-of-sight, is within weapon range, and has not exceeded their fire rate before registering hits.

---

## 2. Anti-Cheat Validation Presets

### `FMPFireRateValidator`
Sliding window fire-rate tracker:
```cpp
FMPFireRateValidator Validator;
Validator.MinShotInterval = 0.1f; // Max 10 shots/sec
Validator.MaxBurstCount = 5;
Validator.BurstWindowSeconds = 0.45f;

FString FailReason;
if (!Validator.ValidateShot(ServerTime, FailReason))
{
    // Reject shot & flag trigger automation
}
```

### `FMPMovementExploitDetector`
Detects unauthorized speed-hacks and unregistered teleporting:
```cpp
FMPMovementExploitDetector Detector;
Detector.MaxAllowedSpeed = 1000.0f;
Detector.TeleportThresholdDistance = 2500.0f;

bool bTeleport = false;
float SpeedErr = 0.0f;
if (!Detector.ValidateStep(NewLoc, ServerTime, bTeleport, SpeedErr))
{
    // Log anomaly or correct client location
}
```

### `UMPServerSecurityLibrary::ValidateTransactionNonDuplicated`
Guards against duplication exploits (e.g. rapid duplicate trade or drop packets) by maintaining an authoritative transaction history buffer.
