# Multiplayer Utility — Engine Compatibility Guide

## Supported Engine Versions

| Engine Version | Status | Notes |
|---|---|---|
| **Unreal Engine 5.6** | `VERIFIED` | Primary active toolchain. Tested with Visual Studio 2022 MSVC 14.38 and Windows SDK 10.0.22621.0. |
| **Unreal Engine 5.5** | `VERIFIED` | Full compatibility. `FVector ServerGravityDirection` parameter handled via `#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4`. |
| **Unreal Engine 5.4** | `VERIFIED` | Full compatibility. Push Model, Iris condition wrappers enabled. |
| **Unreal Engine 5.3** | `VERIFIED` | Legacy fallback for `OnClientCorrectionReceived` (9 arguments) handled seamlessly. |

---

## Compiler & Platform Support

- **Target Platforms**: Windows 64-bit (Win64), Linux (Dedicated Server), Android, iOS.
- **Compilers**: MSVC v143+ (VS 2022), Clang 15+.
- **Build Configurations**: DebugGame, Development, Shipping, Test.
- **Target Types**: Game, Editor, Server (Dedicated).

