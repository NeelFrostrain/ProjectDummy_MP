// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/MPGameplayHelperLibrary.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/NetConnection.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"
#include "Types/MPNetTypes.h"

// =========================================================================
// 1. PLAYER AND NETWORK UTILITIES
// =========================================================================

APlayerState* UMPGameplayHelperLibrary::GetLocalPlayerState(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World)
	{
		return nullptr;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	return PC ? PC->PlayerState : nullptr;
}

APlayerState* UMPGameplayHelperLibrary::GetPlayerStateFromActor(const AActor* TargetActor)
{
	if (!TargetActor)
	{
		return nullptr;
	}

	if (const APawn* Pawn = Cast<const APawn>(TargetActor))
	{
		return Pawn->GetPlayerState();
	}
	if (const AController* Controller = Cast<const AController>(TargetActor))
	{
		return Controller->PlayerState;
	}
	if (const APlayerState* PS = Cast<const APlayerState>(TargetActor))
	{
		return const_cast<APlayerState*>(PS);
	}

	// Traverse ownership chain
	const AActor* Curr = TargetActor->GetOwner();
	while (Curr)
	{
		if (const APawn* P = Cast<const APawn>(Curr))
		{
			return P->GetPlayerState();
		}
		if (const AController* C = Cast<const AController>(Curr))
		{
			return C->PlayerState;
		}
		if (const APlayerState* PS = Cast<const APlayerState>(Curr))
		{
			return const_cast<APlayerState*>(PS);
		}
		Curr = Curr->GetOwner();
	}

	return nullptr;
}

APlayerController* UMPGameplayHelperLibrary::GetPlayerControllerFromActor(const AActor* TargetActor)
{
	if (!TargetActor)
	{
		return nullptr;
	}

	if (const APawn* Pawn = Cast<const APawn>(TargetActor))
	{
		return Cast<APlayerController>(Pawn->GetController());
	}
	if (const APlayerController* PC = Cast<const APlayerController>(TargetActor))
	{
		return const_cast<APlayerController*>(PC);
	}
	if (const APlayerState* PS = Cast<const APlayerState>(TargetActor))
	{
		return PS->GetPlayerController();
	}

	// Traverse ownership chain
	const AActor* Curr = TargetActor->GetOwner();
	while (Curr)
	{
		if (const APawn* P = Cast<const APawn>(Curr))
		{
			return Cast<APlayerController>(P->GetController());
		}
		if (const APlayerController* PC = Cast<const APlayerController>(Curr))
		{
			return const_cast<APlayerController*>(PC);
		}
		Curr = Curr->GetOwner();
	}

	return nullptr;
}

bool UMPGameplayHelperLibrary::IsPlayerLocallyViewed(const AActor* TargetActor)
{
	if (!TargetActor)
	{
		return false;
	}

	UWorld* World = TargetActor->GetWorld();
	if (!World)
	{
		return false;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && PC->IsLocalPlayerController())
		{
			if (PC->GetViewTarget() == TargetActor || PC->GetPawn() == TargetActor)
			{
				return true;
			}
		}
	}

	return false;
}

int32 UMPGameplayHelperLibrary::GetPlayerControllerIndex(const APlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return -1;
	}

	if (const ULocalPlayer* LP = PlayerController->GetLocalPlayer())
	{
		return LP->GetControllerId();
	}

	return -1;
}

TArray<APlayerController*> UMPGameplayHelperLibrary::GetAllRemotePlayerControllers(const UObject* WorldContextObject)
{
	TArray<APlayerController*> Result;
	if (!WorldContextObject)
	{
		return Result;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World)
	{
		return Result;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && !PC->IsLocalController())
		{
			Result.Add(PC);
		}
	}

	return Result;
}

bool UMPGameplayHelperLibrary::IsPlayerConnectionActive(const APlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return false;
	}

	UNetConnection* NetConn = PlayerController->GetNetConnection();
	return NetConn && (NetConn->GetConnectionState() == USOCK_Open);
}

FString UMPGameplayHelperLibrary::GetPlayerPingFormatted(const APlayerState* PlayerState)
{
	if (!PlayerState)
	{
		return TEXT("-- ms");
	}

	const int32 PingMs = FMath::RoundToInt(PlayerState->GetPingInMilliseconds());
	if (PingMs < 0 || PingMs > 9999)
	{
		return TEXT("-- ms");
	}

	return FString::Printf(TEXT("%d ms"), PingMs);
}

// =========================================================================
// 2. RPC UTILITIES
// =========================================================================

bool UMPGameplayHelperLibrary::CanActorSendServerRPC(const AActor* TargetActor, FString& OutReason)
{
	if (!TargetActor)
	{
		OutReason = TEXT("Target actor is null");
		return false;
	}

	if (!TargetActor->GetIsReplicated())
	{
		OutReason = TEXT("Target actor is not replicated (bReplicates is false)");
		return false;
	}

	if (!TargetActor->GetNetConnection())
	{
		OutReason = TEXT("Target actor ownership chain lacks an active NetConnection to server");
		return false;
	}

	const ENetRole Role = TargetActor->GetLocalRole();
	if (Role == ROLE_Authority)
	{
		OutReason = TEXT("Actor has Server Authority; RPC executes locally");
		return true;
	}

	if (Role != ROLE_AutonomousProxy)
	{
		OutReason = TEXT("Actor is SimulatedProxy on client; only AutonomousProxy or Net Owner can send Server RPC");
		return false;
	}

	OutReason = TEXT("Actor has valid ownership and connection to dispatch Server RPC");
	return true;
}

FString UMPGameplayHelperLibrary::GetRPCExecutionScope(const AActor* TargetActor, FString& OutScopeDescription)
{
	if (!TargetActor)
	{
		OutScopeDescription = TEXT("Invalid Actor");
		return OutScopeDescription;
	}

	UWorld* World = TargetActor->GetWorld();
	if (!World)
	{
		OutScopeDescription = TEXT("No World");
		return OutScopeDescription;
	}

	const ENetMode NetMode = World->GetNetMode();
	if (NetMode == NM_Standalone)
	{
		OutScopeDescription = TEXT("Standalone (All RPCs execute locally)");
		return OutScopeDescription;
	}

	if (TargetActor->HasAuthority())
	{
		OutScopeDescription = TEXT("Server Authority: Server RPCs execute locally; Client RPCs dispatch to Owning Client; Multicast dispatches to all replicated clients");
	}
	else
	{
		OutScopeDescription = TEXT("Client: Server RPCs dispatch to Server (if owned); Client/Multicast RPCs execute locally or dropped");
	}

	return OutScopeDescription;
}

bool UMPGameplayHelperLibrary::CheckRPCRateLimitGate(UObject* ContextObject, FName RPCIdentifier, float CooldownSeconds, bool& bOutAllowed)
{
	bOutAllowed = false;
	if (!ContextObject || RPCIdentifier.IsNone() || CooldownSeconds <= 0.0f)
	{
		bOutAllowed = true;
		return true;
	}

	UWorld* World = ContextObject->GetWorld();
	if (!World)
	{
		bOutAllowed = true;
		return true;
	}

	const double Now = World->GetTimeSeconds();
	static TMap<TPair<TWeakObjectPtr<UObject>, FName>, double> LastCallMap;
	const TPair<TWeakObjectPtr<UObject>, FName> Key(ContextObject, RPCIdentifier);

	if (double* LastTime = LastCallMap.Find(Key))
	{
		if (Now - *LastTime < CooldownSeconds)
		{
			bOutAllowed = false;
			return false;
		}
	}

	LastCallMap.Add(Key, Now);
	bOutAllowed = true;
	return true;
}

// =========================================================================
// 3. REPLICATION UTILITIES
// =========================================================================

void UMPGameplayHelperLibrary::SetActorReplicationSafe(AActor* TargetActor, bool bReplicateActor, bool bReplicateMovement)
{
	if (!TargetActor || !TargetActor->HasAuthority())
	{
		return;
	}

	TargetActor->SetReplicates(bReplicateActor);
	TargetActor->SetReplicateMovement(bReplicateMovement);
}

void UMPGameplayHelperLibrary::SetActorNetDormantSafe(AActor* TargetActor, bool bDormant)
{
	if (!TargetActor || !TargetActor->HasAuthority())
	{
		return;
	}

	if (bDormant)
	{
		TargetActor->SetNetDormancy(DORM_DormantAll);
	}
	else
	{
		TargetActor->SetNetDormancy(DORM_Awake);
		TargetActor->FlushNetDormancy();
	}
}

bool UMPGameplayHelperLibrary::IsActorDormant(const AActor* TargetActor)
{
	if (!TargetActor)
	{
		return false;
	}

	return TargetActor->NetDormancy == DORM_DormantAll || TargetActor->NetDormancy == DORM_DormantPartial;
}

float UMPGameplayHelperLibrary::GetActorNetUpdateFrequency(const AActor* TargetActor)
{
	if (!TargetActor)
	{
		return 0.0f;
	}

	return TargetActor->GetNetUpdateFrequency();
}

void UMPGameplayHelperLibrary::ForceNetUpdateWithAttachedActors(AActor* TargetActor)
{
	if (!TargetActor || !TargetActor->HasAuthority())
	{
		return;
	}

	TargetActor->ForceNetUpdate();

	TArray<AActor*> Attached;
	TargetActor->GetAttachedActors(Attached, true, true);
	for (AActor* Child : Attached)
	{
		if (Child && Child->GetIsReplicated())
		{
			Child->ForceNetUpdate();
		}
	}
}

void UMPGameplayHelperLibrary::TearOffActorSafe(AActor* TargetActor)
{
	if (!TargetActor || !TargetActor->HasAuthority())
	{
		return;
	}

	TargetActor->TearOff();
}

bool UMPGameplayHelperLibrary::IsActorTornOff(const AActor* TargetActor)
{
	if (!TargetActor)
	{
		return false;
	}

	return TargetActor->GetTearOff();
}

// =========================================================================
// 4. MULTIPLAYER GAMEPLAY UTILITIES
// =========================================================================

bool UMPGameplayHelperLibrary::CanDamageActorAuthoritative(const AActor* Attacker, const AActor* Victim, bool bAllowFriendlyFire)
{
	if (!Victim || !Victim->HasAuthority())
	{
		return false;
	}

	if (!Attacker)
	{
		return true; // World or environment damage
	}

	if (Attacker == Victim)
	{
		return bAllowFriendlyFire;
	}

	// Team check if interfaces implemented
	if (Victim->GetClass()->ImplementsInterface(UMPTeamInterface::StaticClass()) &&
		Attacker->GetClass()->ImplementsInterface(UMPTeamInterface::StaticClass()))
	{
		const int32 VictimTeam = IMPTeamInterface::Execute_GetTeamId(Victim);
		const int32 AttackerTeam = IMPTeamInterface::Execute_GetTeamId(Attacker);
		if (VictimTeam == AttackerTeam && VictimTeam != -1)
		{
			return bAllowFriendlyFire;
		}
	}

	return true;
}

bool UMPGameplayHelperLibrary::ApplyPointDamageServerOnly(AActor* DamagedActor, float BaseDamage, const FVector& HitFromDirection, const FHitResult& HitInfo, AController* EventInstigator, AActor* DamageCauser, TSubclassOf<UDamageType> DamageTypeClass)
{
	if (!DamagedActor || !DamagedActor->HasAuthority() || BaseDamage <= 0.0f)
	{
		return false;
	}

	UGameplayStatics::ApplyPointDamage(DamagedActor, BaseDamage, HitFromDirection, HitInfo, EventInstigator, DamageCauser, DamageTypeClass);
	return true;
}

bool UMPGameplayHelperLibrary::ApplyRadialDamageServerOnly(const UObject* WorldContextObject, float BaseDamage, const FVector& Origin, float DamageRadius, TSubclassOf<UDamageType> DamageTypeClass, const TArray<AActor*>& IgnoreActors, AActor* DamageCauser, AController* InstigatedByController, bool bDoFullDamage)
{
	if (!WorldContextObject || BaseDamage <= 0.0f || DamageRadius <= 0.0f)
	{
		return false;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World || World->GetNetMode() == NM_Client)
	{
		return false;
	}

	return UGameplayStatics::ApplyRadialDamage(World, BaseDamage, Origin, DamageRadius, DamageTypeClass, IgnoreActors, DamageCauser, InstigatedByController, bDoFullDamage);
}

AActor* UMPGameplayHelperLibrary::SpawnReplicatedActorServerOnly(const UObject* WorldContextObject, TSubclassOf<AActor> ClassToSpawn, const FTransform& SpawnTransform, ESpawnActorCollisionHandlingMethod CollisionHandling, AActor* Owner, APawn* Instigator)
{
	if (!WorldContextObject || !ClassToSpawn)
	{
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World || World->GetNetMode() == NM_Client)
	{
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.Owner = Owner;
	Params.Instigator = Instigator;
	Params.SpawnCollisionHandlingOverride = CollisionHandling;

	AActor* SpawnedActor = World->SpawnActor<AActor>(ClassToSpawn, SpawnTransform, Params);
	if (SpawnedActor)
	{
		SpawnedActor->SetReplicates(true);
	}

	return SpawnedActor;
}

APawn* UMPGameplayHelperLibrary::RespawnPlayerPawnAtTransform(APlayerController* PlayerController, TSubclassOf<APawn> PawnClass, const FTransform& SpawnTransform)
{
	if (!PlayerController || !PlayerController->HasAuthority() || !PawnClass)
	{
		return nullptr;
	}

	UWorld* World = PlayerController->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	APawn* OldPawn = PlayerController->GetPawn();
	if (OldPawn)
	{
		PlayerController->UnPossess();
		OldPawn->Destroy();
	}

	FActorSpawnParameters Params;
	Params.Owner = PlayerController;
	Params.Instigator = nullptr;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	APawn* NewPawn = World->SpawnActor<APawn>(PawnClass, SpawnTransform, Params);
	if (NewPawn)
	{
		NewPawn->SetReplicates(true);
		PlayerController->Possess(NewPawn);
	}

	return NewPawn;
}

AActor* UMPGameplayHelperLibrary::FindBestPlayerStart(const UObject* WorldContextObject, const FVector& PreferredLocation, const FString& IncomingName)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World)
	{
		return nullptr;
	}

	AActor* BestStart = nullptr;
	float ClosestDistSq = MAX_flt;

	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		APlayerStart* Start = *It;
		if (Start)
		{
			if (!IncomingName.IsEmpty() && Start->PlayerStartTag != FName(*IncomingName))
			{
				continue;
			}

			const float DistSq = FVector::DistSquared(Start->GetActorLocation(), PreferredLocation);
			if (DistSq < ClosestDistSq)
			{
				ClosestDistSq = DistSq;
				BestStart = Start;
			}
		}
	}

	return BestStart;
}

bool UMPGameplayHelperLibrary::TeleportPawnServerOnly(APawn* Pawn, const FVector& DestLocation, const FRotator& DestRotation, bool bResetVelocity)
{
	if (!Pawn || !Pawn->HasAuthority())
	{
		return false;
	}

	if (bResetVelocity)
	{
		if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(Pawn->GetRootComponent()))
		{
			RootPrim->SetPhysicsLinearVelocity(FVector::ZeroVector);
			RootPrim->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		}
		if (UPawnMovementComponent* MoveComp = Pawn->GetMovementComponent())
		{
			MoveComp->Velocity = FVector::ZeroVector;
			MoveComp->UpdateComponentVelocity();
		}
	}

	return Pawn->TeleportTo(DestLocation, DestRotation);
}

bool UMPGameplayHelperLibrary::SetPlayerScoreServerOnly(APlayerState* PlayerState, float NewScore)
{
	if (!PlayerState || !PlayerState->HasAuthority())
	{
		return false;
	}

	PlayerState->SetScore(NewScore);
	return true;
}

// =========================================================================
// 5. MULTIPLAYER SYNCHRONIZATION UTILITIES
// =========================================================================

float UMPGameplayHelperLibrary::GetServerTimeSynced(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return 0.0f;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World)
	{
		return 0.0f;
	}

	if (AGameStateBase* GS = World->GetGameState())
	{
		return GS->GetServerWorldTimeSeconds();
	}

	return World->GetTimeSeconds();
}

float UMPGameplayHelperLibrary::GetTimeSinceServerTimestamp(const UObject* WorldContextObject, float ServerTimestamp)
{
	const float ServerNow = GetServerTimeSynced(WorldContextObject);
	return FMath::Max(0.0f, ServerNow - ServerTimestamp);
}

FVector UMPGameplayHelperLibrary::CalculateLatencyCompensatedLocation(const FVector& CurrentLocation, const FVector& Velocity, float PingSeconds)
{
	const float HalfPingSeconds = FMath::Max(0.0f, PingSeconds * 0.5f);
	return CurrentLocation + (Velocity * HalfPingSeconds);
}

bool UMPGameplayHelperLibrary::AreTimestampsOrdered(float EarlierTimestamp, float LaterTimestamp)
{
	return EarlierTimestamp <= LaterTimestamp;
}

float UMPGameplayHelperLibrary::GetServerTickRate(const UObject* WorldContextObject)
{
	extern ENGINE_API float GAverageFPS;
	return GAverageFPS;
}

bool UMPGameplayHelperLibrary::IsServerTickCadence(const UObject* WorldContextObject, int32 TickInterval)
{
	const int32 SafeInterval = FMath::Max(1, TickInterval);
	return (GFrameCounter % SafeInterval) == 0;
}

// =========================================================================
// 6. MULTIPLAYER DEBUGGING UTILITIES
// =========================================================================

FString UMPGameplayHelperLibrary::GetActorNetworkDebugString(const AActor* TargetActor)
{
	if (!TargetActor)
	{
		return TEXT("[Null Actor]");
	}

	const FString RoleStr = UEnum::GetValueAsString(TargetActor->GetLocalRole());
	const FString RemoteRoleStr = UEnum::GetValueAsString(TargetActor->GetRemoteRole());
	const FString DormancyStr = (TargetActor->NetDormancy == DORM_Never) ? TEXT("Never") :
	                            (TargetActor->NetDormancy == DORM_Awake) ? TEXT("Awake") : TEXT("Dormant");

	return FString::Printf(
		TEXT("[%s | Role: %s | RemoteRole: %s | Owner: %s | Dormancy: %s | Freq: %.1f Hz]"),
		*TargetActor->GetName(),
		*RoleStr,
		*RemoteRoleStr,
		*GetNameSafe(TargetActor->GetOwner()),
		*DormancyStr,
		TargetActor->GetNetUpdateFrequency()
	);
}

void UMPGameplayHelperLibrary::DrawNetworkRoleDebugString(const AActor* TargetActor, const FVector& WorldOffset, FColor TextColor, float Duration)
{
#if !UE_BUILD_SHIPPING
	if (TargetActor && TargetActor->GetWorld())
	{
		const FString DebugStr = GetActorNetworkDebugString(TargetActor);
		DrawDebugString(TargetActor->GetWorld(), TargetActor->GetActorLocation() + WorldOffset, DebugStr, nullptr, TextColor, Duration, false);
	}
#endif
}

void UMPGameplayHelperLibrary::LogNetworkEvent(const UObject* ContextObject, const FString& EventName, bool bServerOnly)
{
	if (!ContextObject)
	{
		return;
	}

	UWorld* World = ContextObject->GetWorld();
	if (!World)
	{
		return;
	}

	const ENetMode Mode = World->GetNetMode();
	if (bServerOnly && Mode == NM_Client)
	{
		return;
	}

	const TCHAR* Prefix = (Mode == NM_Client) ? TEXT("[CLIENT]") :
	                      (Mode == NM_DedicatedServer) ? TEXT("[DEDICATED]") :
	                      (Mode == NM_ListenServer) ? TEXT("[LISTEN]") : TEXT("[STANDALONE]");

	UE_LOG(LogNet, Log, TEXT("%s %s"), Prefix, *EventName);
}

void UMPGameplayHelperLibrary::SimulateNetworkConditionCVar(int32 TargetPingMs, float PacketLossPercent)
{
	const int32 SafePing = FMath::Clamp(TargetPingMs, 0, 2000);
	const float SafeLoss = FMath::Clamp(PacketLossPercent, 0.0f, 100.0f);

	if (IConsoleVariable* PktLag = IConsoleManager::Get().FindConsoleVariable(TEXT("Net.PktLag")))
	{
		PktLag->Set(SafePing, ECVF_SetByCode);
	}

	if (IConsoleVariable* PktLoss = IConsoleManager::Get().FindConsoleVariable(TEXT("Net.PktLoss")))
	{
		PktLoss->Set(SafeLoss, ECVF_SetByCode);
	}
}

void UMPGameplayHelperLibrary::DumpReplicationInfoToLog(const AActor* TargetActor)
{
	if (!TargetActor)
	{
		return;
	}

	UE_LOG(LogNet, Log, TEXT("=== REPLICATION DUMP FOR '%s' ==="), *TargetActor->GetName());
	UE_LOG(LogNet, Log, TEXT("  bReplicates: %d"), TargetActor->GetIsReplicated());
	UE_LOG(LogNet, Log, TEXT("  bReplicateMovement: %d"), TargetActor->IsReplicatingMovement());
	UE_LOG(LogNet, Log, TEXT("  LocalRole: %d, RemoteRole: %d"), (int32)TargetActor->GetLocalRole(), (int32)TargetActor->GetRemoteRole());
	UE_LOG(LogNet, Log, TEXT("  Owner: %s"), *GetNameSafe(TargetActor->GetOwner()));
	UE_LOG(LogNet, Log, TEXT("  NetDormancy: %d"), (int32)TargetActor->NetDormancy);
	UE_LOG(LogNet, Log, TEXT("  NetUpdateFrequency: %.1f Hz"), TargetActor->GetNetUpdateFrequency());
	UE_LOG(LogNet, Log, TEXT("  NetCullDistanceSquared: %.1f"), TargetActor->GetNetCullDistanceSquared());
	UE_LOG(LogNet, Log, TEXT("================================="));
}
