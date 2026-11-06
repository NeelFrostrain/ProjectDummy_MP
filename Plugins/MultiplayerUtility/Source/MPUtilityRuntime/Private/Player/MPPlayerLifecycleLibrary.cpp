// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/MPPlayerLifecycleLibrary.h"
#include "GameFramework/PlayerState.h"
#include "Engine/World.h"
#include "TimerManager.h"

bool UMPPlayerLifecycleLibrary::SafePossessPawnServerOnly(APlayerController* Controller, APawn* NewPawn)
{
	if (!Controller || !NewPawn)
	{
		return false;
	}

	// Must have server authority
	if (!Controller->HasAuthority())
	{
		UE_LOG(LogNet, Warning, TEXT("[MPPlayerLifecycleLibrary] SafePossessPawnServerOnly called on non-authoritative client."));
		return false;
	}

	// If pawn is already possessed by another controller, safely unpossess first
	if (AController* OldController = NewPawn->GetController())
	{
		if (OldController != Controller)
		{
			OldController->UnPossess();
		}
		else
		{
			return true; // Already possessing this pawn
		}
	}

	// Unpossess current pawn if any
	if (Controller->GetPawn() && Controller->GetPawn() != NewPawn)
	{
		Controller->UnPossess();
	}

	Controller->Possess(NewPawn);
	return Controller->GetPawn() == NewPawn;
}

bool UMPPlayerLifecycleLibrary::SafeUnpossessPawnServerOnly(APlayerController* Controller)
{
	if (!Controller)
	{
		return false;
	}

	if (!Controller->HasAuthority())
	{
		return false;
	}

	if (Controller->GetPawn())
	{
		Controller->UnPossess();
		return true;
	}

	return false;
}

APawn* UMPPlayerLifecycleLibrary::SpawnAndPossessPlayerPawnServerOnly(
	APlayerController* Controller,
	TSubclassOf<APawn> PawnClass,
	const FTransform& SpawnTransform,
	ESpawnActorCollisionHandlingMethod CollisionHandling)
{
	if (!Controller || !*PawnClass)
	{
		return nullptr;
	}

	if (!Controller->HasAuthority())
	{
		return nullptr;
	}

	UWorld* World = Controller->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Controller;
	SpawnParams.Instigator = nullptr;
	SpawnParams.SpawnCollisionHandlingOverride = CollisionHandling;

	APawn* NewPawn = World->SpawnActor<APawn>(PawnClass, SpawnTransform, SpawnParams);
	if (NewPawn)
	{
		SafePossessPawnServerOnly(Controller, NewPawn);
	}

	return NewPawn;
}

bool UMPPlayerLifecycleLibrary::TransferActorOwnershipSafe(AActor* TargetActor, APlayerController* NewOwnerPC)
{
	if (!TargetActor)
	{
		return false;
	}

	if (!TargetActor->HasAuthority())
	{
		return false;
	}

	TargetActor->SetOwner(NewOwnerPC);
	return TargetActor->GetOwner() == NewOwnerPC;
}

bool UMPPlayerLifecycleLibrary::SetPlayerSpectatorStateServerOnly(APlayerController* Controller, bool bEnableSpectator)
{
	if (!Controller || !Controller->HasAuthority())
	{
		return false;
	}

	if (bEnableSpectator)
	{
		if (Controller->GetPawn())
		{
			Controller->UnPossess();
		}
		Controller->StartSpectatingOnly();
		return true;
	}
	else
	{
		// Note: caller will need to spawn or possess a new pawn to resume play
		return true;
	}
}

APawn* UMPPlayerLifecycleLibrary::RespawnPlayerPawnServerOnly(
	APlayerController* Controller,
	TSubclassOf<APawn> PawnClass,
	const FTransform& RespawnTransform,
	bool bDestroyOldPawn)
{
	if (!Controller || !Controller->HasAuthority())
	{
		return nullptr;
	}

	APawn* OldPawn = Controller->GetPawn();
	if (OldPawn)
	{
		Controller->UnPossess();
		if (bDestroyOldPawn)
		{
			OldPawn->Destroy();
		}
	}

	return SpawnAndPossessPlayerPawnServerOnly(Controller, PawnClass, RespawnTransform);
}

bool UMPPlayerLifecycleLibrary::CleanupPlayerPawnSafe(APawn* PawnToClean, float DelayBeforeDestroy)
{
	if (!PawnToClean || !PawnToClean->HasAuthority())
	{
		return false;
	}

	// Unpossess immediately
	if (AController* C = PawnToClean->GetController())
	{
		C->UnPossess();
	}

	if (DelayBeforeDestroy <= 0.0f)
	{
		PawnToClean->Destroy();
		return true;
	}

	UWorld* World = PawnToClean->GetWorld();
	if (World)
	{
		FTimerHandle TimerHandle;
		TWeakObjectPtr<APawn> WeakPawn = PawnToClean;
		World->GetTimerManager().SetTimer(
			TimerHandle,
			[WeakPawn]()
			{
				if (WeakPawn.IsValid())
				{
					WeakPawn->Destroy();
				}
			},
			DelayBeforeDestroy,
			false
		);
		return true;
	}

	PawnToClean->Destroy();
	return true;
}

