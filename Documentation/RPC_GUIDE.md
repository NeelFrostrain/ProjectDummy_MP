# Multiplayer Utility — RPC Request / Response Guide

## 1. Overview

Unreal Engine RPCs are fire-and-forget: they provide no built-in mechanism for the server to reply to a client with an execution result or error code.

The **RPC Request / Response Framework** solves this by providing:
- Correlated request and response pairs using unique `FGuid` identifiers.
- Automatic timeout tracking and cancellation.
- Bounded retries for idempotent requests.
- Asynchronous Blueprint nodes with multiple execution pins.
- Clean disconnect and world transition cleanup.

---

## 2. Using in Blueprints

Use the **`Async Send Server Request`** node in any Blueprint:

```
[ Async Send Server Request ]
  ├── Request: (RequestAction = "BuyItem", Payload = [...])
  │
  ├── On Success  ──> (Response.Payload / Confirmation)
  ├── On Failed   ──> (Response.Message / Error Code)
  └── On TimedOut ──> (Notify player of network timeout)
```

### Server Handler Registration:
On the server (e.g. in your GameMode or PlayerController):
1. Obtain the `UMPNetworkRequestComponent` on the PlayerController.
2. Call `Register Server Handler` with the `RequestAction` name.
3. Return an `FMPNetworkResponse` with `EMPNetworkRequestResult::Success` or `EMPNetworkRequestResult::ServerRejected`.

---

## 3. Using in C++

```cpp
#include "Requests/MPNetworkRequestComponent.h"

// 1. Sending a request
FMPNetworkRequest Request;
Request.RequestAction = TEXT("EquipSkin");
Request.TimeoutSeconds = 3.0f;

RequestComp->SendRequestNative(Request, [](const FMPNetworkResponse& Response)
{
    if (Response.IsSuccess())
    {
        UE_LOG(LogNet, Log, TEXT("Skin equip confirmed by server!"));
    }
    else
    {
        UE_LOG(LogNet, Warning, TEXT("Failed: %s"), *Response.Message);
    }
});
```
