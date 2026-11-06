// Copyright Epic Games, Inc. All Rights Reserved.

#include "Subsystems/MPEditorScannerSubsystem.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "K2Node_CallFunction.h"
#include "EdGraphNode_Comment.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Factories/BlueprintFactory.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "FileHelpers.h"
#include "Misc/FileHelper.h"

// Runtime classes for showcase graph generation:
#include "MPNetFunctionLibrary.h"
#include "Wrappers/MPPawnControllerWrapper.h"
#include "Wrappers/MPActorNetworkWrapper.h"
#include "Wrappers/MPComponentNetworkWrapper.h"
#include "Wrappers/MPMovementPhysicsWrapper.h"
#include "Wrappers/MPWorldSessionWrapper.h"
#include "Types/MPQuantization.h"
#include "Types/MPTimeMath.h"
#include "FlowControl/MPFlowControlLibrary.h"
#include "FlowControl/MPLatentActions.h"
#include "Components/MPAutoDormancyComponent.h"
#include "Types/MPNetPresetAsset.h"
#include "Requests/MPNetworkRequestSubsystem.h"
#include "Requests/MPNetworkRequestComponent.h"
#include "Player/MPPlayerLifecycleLibrary.h"
#include "Movement/MPCharacterMovementComponent.h"
#include "Security/MPServerSecurityLibrary.h"
#include "Optimization/MPNetLODComponent.h"
#include "Gameplay/MPHealthComponent.h"
#include "Physics/MPPhysicsReplicationHelper.h"
#include "Replication/MPReplicationConditions.h"
#include "Gameplay/MPGameplayHelperLibrary.h"
#include "Gameplay/MPRPCGameplayComponent.h"

void UMPEditorScannerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UMPEditorScannerSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

bool UMPEditorScannerSubsystem::AuditActorReplication(AActor* Actor, FMPActorNetworkAuditReport& OutReport)
{
	OutReport = FMPActorNetworkAuditReport();

	if (!Actor)
	{
		return false;
	}

	OutReport.ActorName = Actor->GetName();
	OutReport.bReplicates = Actor->GetIsReplicated();
	OutReport.bReplicatesMovement = Actor->IsReplicatingMovement();

	// 1. Scan replicated properties
	for (TFieldIterator<FProperty> PropIt(Actor->GetClass(), EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
	{
		const FProperty* Prop = *PropIt;
		if (Prop && Prop->HasAnyPropertyFlags(CPF_Net))
		{
			OutReport.ReplicatedPropertyCount++;
		}
	}

	// 2. Scan replicated components
	TInlineComponentArray<UActorComponent*> Components(Actor);
	for (UActorComponent* Comp : Components)
	{
		if (Comp && Comp->GetIsReplicated())
		{
			OutReport.ReplicatedComponentCount++;
		}
	}

	// 3. Rule Checks & Warnings
	if (OutReport.ReplicatedPropertyCount > 0 && !OutReport.bReplicates)
	{
		OutReport.Warnings.Add(FString::Printf(
			TEXT("Actor '%s' has %d replicated properties (CPF_Net) but bReplicates is FALSE! Properties will never replicate."),
			*OutReport.ActorName, OutReport.ReplicatedPropertyCount
		));
	}

	if (OutReport.ReplicatedComponentCount > 0 && !OutReport.bReplicates)
	{
		OutReport.Warnings.Add(FString::Printf(
			TEXT("Actor '%s' has %d replicated components but actor bReplicates is FALSE! Replicated components require the owning actor to replicate."),
			*OutReport.ActorName, OutReport.ReplicatedComponentCount
		));
	}

	if (OutReport.bReplicatesMovement && !OutReport.bReplicates)
	{
		OutReport.Warnings.Add(FString::Printf(
			TEXT("Actor '%s' has bReplicateMovement set to TRUE, but bReplicates is FALSE."),
			*OutReport.ActorName
		));
	}

	if (Actor->NetDormancy == DORM_Never && Actor->GetNetUpdateFrequency() > 100.0f)
	{
		OutReport.Warnings.Add(FString::Printf(
			TEXT("Actor '%s' has NetDormancy=DORM_Never with high NetUpdateFrequency (%.1f Hz). Consider lowering frequency or enabling dormancy for performance."),
			*OutReport.ActorName, Actor->GetNetUpdateFrequency()
		));
	}

	return OutReport.Warnings.Num() == 0;
}

bool UMPEditorScannerSubsystem::ValidateRPCOwnership(AActor* Actor, TArray<FString>& OutWarnings)
{
	OutWarnings.Empty();

	if (!Actor)
	{
		return false;
	}

	bool bHasServerRPC = false;

	for (TFieldIterator<UFunction> FuncIt(Actor->GetClass(), EFieldIteratorFlags::IncludeSuper); FuncIt; ++FuncIt)
	{
		const UFunction* Func = *FuncIt;
		if (Func && Func->HasAnyFunctionFlags(FUNC_NetServer))
		{
			bHasServerRPC = true;
			break;
		}
	}

	if (bHasServerRPC)
	{
		const bool bCanHaveOwner = Actor->IsA<APlayerController>() ||
		                           Actor->IsA<APawn>() ||
		                           Actor->GetOwner() != nullptr;

		if (!bCanHaveOwner)
		{
			OutWarnings.Add(FString::Printf(
				TEXT("Actor '%s' declares Server RPCs, but is not a Pawn, PlayerController, and has no NetOwner assigned. Server RPCs will be dropped if invoked without an owning connection!"),
				*Actor->GetName()
			));
			return false;
		}
	}

	return true;
}

void UMPEditorScannerSubsystem::AuditWorldNetworkActors(int32& OutTotalActors, int32& OutReplicatedActors, int32& OutActorsWithWarnings)
{
	OutTotalActors = 0;
	OutReplicatedActors = 0;
	OutActorsWithWarnings = 0;

	if (!GEditor)
	{
		return;
	}

	UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	if (!EditorWorld)
	{
		return;
	}

	for (TActorIterator<AActor> It(EditorWorld); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor)
		{
			continue;
		}

		OutTotalActors++;

		if (Actor->GetIsReplicated())
		{
			OutReplicatedActors++;
		}

		FMPActorNetworkAuditReport Report;
		AuditActorReplication(Actor, Report);
		if (Report.Warnings.Num() > 0)
		{
			OutActorsWithWarnings++;
		}
	}
}

void UMPEditorScannerSubsystem::ApplyPIENetworkEmulation(int32 TargetPingMs, float PacketLossPercent)
{
	const int32 SafePing = FMath::Clamp(TargetPingMs, 0, 2000);
	const float SafeLoss = FMath::Clamp(PacketLossPercent, 0.0f, 100.0f);

	if (IConsoleVariable* PktLagVar = IConsoleManager::Get().FindConsoleVariable(TEXT("Net.PktLag")))
	{
		PktLagVar->Set(SafePing, ECVF_SetByCode);
	}

	if (IConsoleVariable* PktLossVar = IConsoleManager::Get().FindConsoleVariable(TEXT("Net.PktLoss")))
	{
		PktLossVar->Set(SafeLoss, ECVF_SetByCode);
	}

	UE_LOG(LogNet, Log, TEXT("[MPEditorScannerSubsystem] Applied PIE emulation: Ping=%dms, PacketLoss=%.1f%%"), SafePing, SafeLoss);
}

bool UMPEditorScannerSubsystem::PopulateBlueprintGraphFromText(UBlueprint* TargetBlueprint, const FString& NodesText)
{
	if (!TargetBlueprint || NodesText.IsEmpty())
	{
		return false;
	}

	UEdGraph* EventGraph = FBlueprintEditorUtils::FindEventGraph(TargetBlueprint);
	if (!EventGraph)
	{
		if (TargetBlueprint->UbergraphPages.Num() > 0)
		{
			EventGraph = TargetBlueprint->UbergraphPages[0];
		}
	}

	if (!EventGraph)
	{
		return false;
	}

	TSet<UEdGraphNode*> ImportedNodes;
	FEdGraphUtilities::ImportNodesFromText(EventGraph, NodesText, ImportedNodes);

	FBlueprintEditorUtils::MarkBlueprintAsModified(TargetBlueprint);
	UE_LOG(LogNet, Log, TEXT("[MPEditorScannerSubsystem] Successfully imported %d nodes into '%s' EventGraph"), ImportedNodes.Num(), *TargetBlueprint->GetName());
	return ImportedNodes.Num() > 0;
}

namespace
{
	struct FMPShowcaseFuncItem
	{
		UClass* TargetClass;
		FName FunctionName;
	};

	struct FMPShowcaseCategoryLayout
	{
		FString Title;
		FLinearColor Color;
		TArray<FMPShowcaseFuncItem> Functions;
	};

	TArray<FMPShowcaseCategoryLayout> BuildShowcaseCategories()
	{
		TArray<FMPShowcaseCategoryLayout> Categories;

		// 1. Network State & Net Mode Queries
		{
			FMPShowcaseCategoryLayout Cat;
			Cat.Title = TEXT("1. Network State & Net Mode Queries");
			Cat.Color = FLinearColor(0.10f, 0.45f, 0.90f, 1.0f);
			Cat.Functions = {
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("IsServer") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("IsNetworkGame") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("IsStandalone") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("IsDedicatedServer") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("IsListenServer") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("IsClient") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("HasAuthority") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("IsAutonomousProxy") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("IsSimulatedProxy") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("IsLocallyControlled") },
				{ UMPPawnControllerWrapper::StaticClass(), TEXT("IsPlayerControlled") },
				{ UMPPawnControllerWrapper::StaticClass(), TEXT("IsBotControlled") },
				{ UMPPawnControllerWrapper::StaticClass(), TEXT("IsLocalPlayerController") }
			};
			Categories.Add(Cat);
		}

		// 2. Actor Replication & Lifecycle Control
		{
			FMPShowcaseCategoryLayout Cat;
			Cat.Title = TEXT("2. Actor Replication & Lifecycle Control");
			Cat.Color = FLinearColor(0.12f, 0.65f, 0.35f, 1.0f);
			Cat.Functions = {
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("IsRPCSafeToCall") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("ExplainWhyRPCDropped") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("CanSendServerRPC") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("GetRPCOwnershipChain") },
				{ UMPActorNetworkWrapper::StaticClass(), TEXT("SetReplicates") },
				{ UMPActorNetworkWrapper::StaticClass(), TEXT("SetReplicateMovement") },
				{ UMPActorNetworkWrapper::StaticClass(), TEXT("SetOwner") },
				{ UMPActorNetworkWrapper::StaticClass(), TEXT("GetTopLevelNetOwner") },
				{ UMPActorNetworkWrapper::StaticClass(), TEXT("GetLogicalOwnerSafe") },
				{ UMPActorNetworkWrapper::StaticClass(), TEXT("SetNetDormancy") },
				{ UMPActorNetworkWrapper::StaticClass(), TEXT("FlushNetDormancy") },
				{ UMPActorNetworkWrapper::StaticClass(), TEXT("ForceNetUpdate") },
				{ UMPActorNetworkWrapper::StaticClass(), TEXT("SetNetUpdateFrequency") },
				{ UMPActorNetworkWrapper::StaticClass(), TEXT("SetMinNetUpdateFrequency") },
				{ UMPActorNetworkWrapper::StaticClass(), TEXT("SetNetCullDistance") },
				{ UMPActorNetworkWrapper::StaticClass(), TEXT("SetAlwaysRelevant") },
				{ UMPActorNetworkWrapper::StaticClass(), TEXT("SetOnlyRelevantToOwner") }
			};
			Categories.Add(Cat);
		}

		// 3. Subobject & Component Replication
		{
			FMPShowcaseCategoryLayout Cat;
			Cat.Title = TEXT("3. Subobject & Component Replication");
			Cat.Color = FLinearColor(0.08f, 0.60f, 0.70f, 1.0f);
			Cat.Functions = {
				{ UMPComponentNetworkWrapper::StaticClass(), TEXT("SetComponentReplicated") },
				{ UMPComponentNetworkWrapper::StaticClass(), TEXT("IsComponentReplicated") },
				{ UMPComponentNetworkWrapper::StaticClass(), TEXT("IsComponentNetSimulating") },
				{ UMPComponentNetworkWrapper::StaticClass(), TEXT("HasComponentAuthority") },
				{ UMPComponentNetworkWrapper::StaticClass(), TEXT("RegisterReplicatedSubobjectSafe") },
				{ UMPComponentNetworkWrapper::StaticClass(), TEXT("UnregisterReplicatedSubobjectSafe") },
				{ UMPComponentNetworkWrapper::StaticClass(), TEXT("IsSubobjectReplicated") },
				{ UMPComponentNetworkWrapper::StaticClass(), TEXT("GetReplicatedComponentsOfActor") },
				{ UMPComponentNetworkWrapper::StaticClass(), TEXT("SetComponentReplicatedRecursive") },
				{ UMPComponentNetworkWrapper::StaticClass(), TEXT("PrintReplicatedSubobjects") }
			};
			Categories.Add(Cat);
		}

		// 4. Movement & Physics Replication
		{
			FMPShowcaseCategoryLayout Cat;
			Cat.Title = TEXT("4. Movement & Physics Replication");
			Cat.Color = FLinearColor(0.85f, 0.45f, 0.10f, 1.0f);
			Cat.Functions = {
				{ UMPMovementPhysicsWrapper::StaticClass(), TEXT("SetSimulatePhysicsServerOnly") },
				{ UMPMovementPhysicsWrapper::StaticClass(), TEXT("ApplyImpulseServerOnly") },
				{ UMPMovementPhysicsWrapper::StaticClass(), TEXT("GetReplicatedVelocitySafe") },
				{ UMPMovementPhysicsWrapper::StaticClass(), TEXT("IsPhysicsReplicated") },
				{ UMPMovementPhysicsWrapper::StaticClass(), TEXT("SnapOrSmoothTransform") },
				{ UMPMovementPhysicsWrapper::StaticClass(), TEXT("GetMovementErrorDistance") },
				{ UMPMovementPhysicsWrapper::StaticClass(), TEXT("IsCharacterMovingAuthoritatively") },
				{ UMPMovementPhysicsWrapper::StaticClass(), TEXT("GetNetworkSmoothingMode") },
				{ UMPMovementPhysicsWrapper::StaticClass(), TEXT("SetNetworkSmoothingModeSafe") }
			};
			Categories.Add(Cat);
		}

		// 5. Connection Quality & Network Diagnostics
		{
			FMPShowcaseCategoryLayout Cat;
			Cat.Title = TEXT("5. Connection Quality & Network Diagnostics");
			Cat.Color = FLinearColor(0.55f, 0.25f, 0.85f, 1.0f);
			Cat.Functions = {
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("GetPlayerPingMs") },
				{ UMPPawnControllerWrapper::StaticClass(), TEXT("GetPlayerPingSeconds") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("GetConnectionQualityEnum") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("IsConnectionUnstable") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("GetAveragePingOverNSeconds") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("GetPingTrend") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("GetPacketLossPercentSafe") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("ShouldReduceNetworkLoad") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("EstimateBandwidthPerPlayer") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("GetPlayerIPAddress") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("GetPlayerUniqueIdString") }
			};
			Categories.Add(Cat);
		}

		// 6. Player & Spatial Queries
		{
			FMPShowcaseCategoryLayout Cat;
			Cat.Title = TEXT("6. Player & Spatial Queries");
			Cat.Color = FLinearColor(0.75f, 0.60f, 0.10f, 1.0f);
			Cat.Functions = {
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("GetClosestPlayerPawn") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("GetPlayersInSphere") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("GetAllPlayerPawns") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("GetAllPlayerControllers") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("GetAllRemotePlayers") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("GetDistanceToNearestPlayer") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("IsAnyPlayerWithinRange") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("GetPlayersSortedByDistance") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("GetFurthestPlayer") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("GetPlayersInCone") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("GetPlayersInRadiusWithLineOfSight") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("GetRandomPlayer") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("GetAlivePlayersOnly") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("GetActorsWithTagReplicatedSafe") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("HasAnyTagSafe") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("MakeNetUniqueName") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("IsValidPlayerName") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("ComparePlayerNamesCaseInsensitive") }
			};
			Categories.Add(Cat);
		}

		// 7. Team Affiliation & Relevancy
		{
			FMPShowcaseCategoryLayout Cat;
			Cat.Title = TEXT("7. Team Affiliation & Relevancy");
			Cat.Color = FLinearColor(0.85f, 0.20f, 0.50f, 1.0f);
			Cat.Functions = {
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("AreTeammates") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("AreEnemies") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("GetActorsByTeam") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("GetTeamOfActorSafe") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("IsActorRelevantToPlayerSafe") }
			};
			Categories.Add(Cat);
		}

		// 8. Authoritative Server Validation
		{
			FMPShowcaseCategoryLayout Cat;
			Cat.Title = TEXT("8. Authoritative Server Validation");
			Cat.Color = FLinearColor(0.85f, 0.15f, 0.15f, 1.0f);
			Cat.Functions = {
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("ValidateTransformFromClient") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("ValidateTimestampFromClient") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("ValidateFloatRange") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("ValidateVectorNotNaN") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("ValidateEnumValue") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("ClampClientInputVector") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("IsValidSpawnLocation") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("IsPathClearServer") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("SanitizePlayerName") }
			};
			Categories.Add(Cat);
		}

		// 9. Component & Spawn Helpers
		{
			FMPShowcaseCategoryLayout Cat;
			Cat.Title = TEXT("9. Component & Spawn Helpers");
			Cat.Color = FLinearColor(0.40f, 0.70f, 0.15f, 1.0f);
			Cat.Functions = {
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("AddReplicatedComponentServerOnly") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("FindComponentSafe") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("SpawnActorDeferredServerOnly") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("FinishSpawningActorServerOnly") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("SpawnAtFreePlayerStartSafe") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("TeleportSafeServer") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("GetOwningPlayerStateSafe") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("GetOwningPawnSafe") },
				{ UMPNetFunctionLibrary::StaticClass(), TEXT("ApplyNetPresetToActor") }
			};
			Categories.Add(Cat);
		}

		// 10. Level Streaming & World Helpers
		{
			FMPShowcaseCategoryLayout Cat;
			Cat.Title = TEXT("10. Level Streaming & World Helpers");
			Cat.Color = FLinearColor(0.20f, 0.30f, 0.65f, 1.0f);
			Cat.Functions = {
				{ UMPWorldSessionWrapper::StaticClass(), TEXT("SafeServerTravel") },
				{ UMPWorldSessionWrapper::StaticClass(), TEXT("GetConnectedPlayerCount") },
				{ UMPWorldSessionWrapper::StaticClass(), TEXT("GetServerWorldTime") },
				{ UMPWorldSessionWrapper::StaticClass(), TEXT("IsLevelLoadedForAllPlayers") },
				{ UMPWorldSessionWrapper::StaticClass(), TEXT("GetPlayersInStreamingLevel") },
				{ UMPWorldSessionWrapper::StaticClass(), TEXT("IsActorInLoadedLevel") },
				{ UMPWorldSessionWrapper::StaticClass(), TEXT("GetLevelNameOfActor") },
				{ UMPWorldSessionWrapper::StaticClass(), TEXT("IsSeamlessTraveling") },
				{ UMPWorldSessionWrapper::StaticClass(), TEXT("GetCurrentMapNameClean") },
				{ UMPWorldSessionWrapper::StaticClass(), TEXT("GetTravelOptionsFromURL") },
				{ UMPPawnControllerWrapper::StaticClass(), TEXT("SafeClientTravel") }
			};
			Categories.Add(Cat);
		}

		// 11. Network Math, Quantization & Bitwise
		{
			FMPShowcaseCategoryLayout Cat;
			Cat.Title = TEXT("11. Network Math, Quantization & Bitwise");
			Cat.Color = FLinearColor(0.10f, 0.70f, 0.60f, 1.0f);
			Cat.Functions = {
				{ UMPQuantization::StaticClass(), TEXT("WrapAngleSafe") },
				{ UMPQuantization::StaticClass(), TEXT("IsVectorWithinTolerance") },
				{ UMPQuantization::StaticClass(), TEXT("IsRotatorWithinTolerance") },
				{ UMPQuantization::StaticClass(), TEXT("HashNameToNetInt") },
				{ UMPQuantization::StaticClass(), TEXT("PackTwoFloatsToInt") },
				{ UMPQuantization::StaticClass(), TEXT("UnpackTwoFloatsFromInt") },
				{ UMPQuantization::StaticClass(), TEXT("SetBit32") },
				{ UMPQuantization::StaticClass(), TEXT("ClearBit32") },
				{ UMPQuantization::StaticClass(), TEXT("TestBit32") },
				{ UMPQuantization::StaticClass(), TEXT("SetBit64") },
				{ UMPQuantization::StaticClass(), TEXT("ClearBit64") },
				{ UMPQuantization::StaticClass(), TEXT("TestBit64") },
				{ UMPTimeMath::StaticClass(), TEXT("IsTimestampNewer") },
				{ UMPTimeMath::StaticClass(), TEXT("SequenceDistance16") },
				{ UMPTimeMath::StaticClass(), TEXT("SequenceDistance32") },
				{ UMPTimeMath::StaticClass(), TEXT("ClampDeltaTimeForNetwork") },
				{ UMPTimeMath::StaticClass(), TEXT("LerpNetValue") },
				{ UMPTimeMath::StaticClass(), TEXT("ExponentialSmoothFloat") },
				{ UMPTimeMath::StaticClass(), TEXT("ExponentialSmoothVector") }
			};
			Categories.Add(Cat);
		}

		// 12. Flow-Control Execution & Latent Nodes
		{
			FMPShowcaseCategoryLayout Cat;
			Cat.Title = TEXT("12. Flow-Control Execution & Latent Nodes");
			Cat.Color = FLinearColor(0.50f, 0.15f, 0.75f, 1.0f);
			Cat.Functions = {
				{ UMPFlowControlLibrary::StaticClass(), TEXT("BranchByNetRole") },
				{ UMPFlowControlLibrary::StaticClass(), TEXT("BranchByNetMode") },
				{ UMPFlowControlLibrary::StaticClass(), TEXT("BranchIsLocallyControlled") },
				{ UMPFlowControlLibrary::StaticClass(), TEXT("BranchHasAuthority") },
				{ UMPFlowControlLibrary::StaticClass(), TEXT("NetworkGate") },
				{ UMPFlowControlLibrary::StaticClass(), TEXT("RunOnNextFrame") },
				{ UMPFlowControlLibrary::StaticClass(), TEXT("RunAfterServerSeconds") },
				{ UMPFlowControlLibrary::StaticClass(), TEXT("CancelPendingByKey") },
				{ UMPFlowControlLibrary::StaticClass(), TEXT("DoOnServerOnlyAfterDelay") },
				{ UMPFlowControlLibrary::StaticClass(), TEXT("CallOncePerPlayer") },
				{ UMPLatentActions::StaticClass(), TEXT("DelayUntilReplicated") },
				{ UMPLatentActions::StaticClass(), TEXT("WaitServerSeconds") }
			};
			Categories.Add(Cat);
		}

		// 13. Replicated Components & Presets
		{
			FMPShowcaseCategoryLayout Cat;
			Cat.Title = TEXT("13. Replicated Components & Presets");
			Cat.Color = FLinearColor(0.35f, 0.40f, 0.45f, 1.0f);
			Cat.Functions = {
				{ UMPAutoDormancyComponent::StaticClass(), TEXT("WakeFromDormancy") },
				{ UMPNetPresetAsset::StaticClass(), TEXT("ApplyToActor") }
			};
			Categories.Add(Cat);
		}

		// 14. RPC Request / Response Framework
		{
			FMPShowcaseCategoryLayout Cat;
			Cat.Title = TEXT("14. RPC Request / Response Framework");
			Cat.Color = FLinearColor(0.35f, 0.20f, 0.85f, 1.0f);
			Cat.Functions = {
				{ UMPNetworkRequestSubsystem::StaticClass(), TEXT("SendNetworkRequest") },
				{ UMPNetworkRequestSubsystem::StaticClass(), TEXT("GetTotalDispatchedRequests") },
				{ UMPNetworkRequestSubsystem::StaticClass(), TEXT("GetTotalTimedOutRequests") },
				{ UMPNetworkRequestComponent::StaticClass(), TEXT("SendRequest") },
				{ UMPNetworkRequestComponent::StaticClass(), TEXT("CancelRequest") },
				{ UMPNetworkRequestComponent::StaticClass(), TEXT("RegisterServerHandler") },
				{ UMPNetworkRequestComponent::StaticClass(), TEXT("UnregisterServerHandler") }
			};
			Categories.Add(Cat);
		}

		// 15. Player Lifecycle Management
		{
			FMPShowcaseCategoryLayout Cat;
			Cat.Title = TEXT("15. Player Lifecycle Management");
			Cat.Color = FLinearColor(0.20f, 0.50f, 0.80f, 1.0f);
			Cat.Functions = {
				{ UMPPlayerLifecycleLibrary::StaticClass(), TEXT("SafePossessPawnServerOnly") },
				{ UMPPlayerLifecycleLibrary::StaticClass(), TEXT("SafeUnpossessPawnServerOnly") },
				{ UMPPlayerLifecycleLibrary::StaticClass(), TEXT("SpawnAndPossessPlayerPawnServerOnly") },
				{ UMPPlayerLifecycleLibrary::StaticClass(), TEXT("TransferActorOwnershipSafe") },
				{ UMPPlayerLifecycleLibrary::StaticClass(), TEXT("SetPlayerSpectatorStateServerOnly") },
				{ UMPPlayerLifecycleLibrary::StaticClass(), TEXT("RespawnPlayerPawnServerOnly") },
				{ UMPPlayerLifecycleLibrary::StaticClass(), TEXT("CleanupPlayerPawnSafe") }
			};
			Categories.Add(Cat);
		}

		// 16. Character Movement Prediction Pipeline
		{
			FMPShowcaseCategoryLayout Cat;
			Cat.Title = TEXT("16. Character Movement Prediction Pipeline");
			Cat.Color = FLinearColor(0.95f, 0.40f, 0.10f, 1.0f);
			Cat.Functions = {
				{ UMPCharacterMovementComponent::StaticClass(), TEXT("SetSprinting") },
				{ UMPCharacterMovementComponent::StaticClass(), TEXT("TriggerDash") },
				{ UMPCharacterMovementComponent::StaticClass(), TEXT("SetSliding") },
				{ UMPCharacterMovementComponent::StaticClass(), TEXT("SetClimbing") },
				{ UMPCharacterMovementComponent::StaticClass(), TEXT("IsSprinting") },
				{ UMPCharacterMovementComponent::StaticClass(), TEXT("IsSliding") },
				{ UMPCharacterMovementComponent::StaticClass(), TEXT("IsClimbing") },
				{ UMPCharacterMovementComponent::StaticClass(), TEXT("ApplySmoothingProfile") },
				{ UMPCharacterMovementComponent::StaticClass(), TEXT("ValidateMovementStep") },
				{ UMPCharacterMovementComponent::StaticClass(), TEXT("GetClientCorrectionCount") }
			};
			Categories.Add(Cat);
		}

		// 17. Server Security & Anti-Cheat Validation
		{
			FMPShowcaseCategoryLayout Cat;
			Cat.Title = TEXT("17. Server Security & Anti-Cheat Validation");
			Cat.Color = FLinearColor(0.90f, 0.15f, 0.20f, 1.0f);
			Cat.Functions = {
				{ UMPServerSecurityLibrary::StaticClass(), TEXT("ValidateDamageRequestAuthoritative") },
				{ UMPServerSecurityLibrary::StaticClass(), TEXT("ValidateClientTimestampDrift") },
				{ UMPServerSecurityLibrary::StaticClass(), TEXT("ValidateTransactionNonDuplicated") }
			};
			Categories.Add(Cat);
		}

		// 18. Network Optimization & Dynamic Net LOD
		{
			FMPShowcaseCategoryLayout Cat;
			Cat.Title = TEXT("18. Network Optimization & Dynamic Net LOD");
			Cat.Color = FLinearColor(0.15f, 0.70f, 0.30f, 1.0f);
			Cat.Functions = {
				{ UMPNetLODComponent::StaticClass(), TEXT("ForceEvaluateLOD") }
			};
			Categories.Add(Cat);
		}

		// 19. Modular Health & Vitality Authority
		{
			FMPShowcaseCategoryLayout Cat;
			Cat.Title = TEXT("19. Modular Health & Vitality Authority");
			Cat.Color = FLinearColor(0.85f, 0.30f, 0.40f, 1.0f);
			Cat.Functions = {
				{ UMPHealthComponent::StaticClass(), TEXT("ApplyDamageServerOnly") },
				{ UMPHealthComponent::StaticClass(), TEXT("ApplyHealingServerOnly") },
				{ UMPHealthComponent::StaticClass(), TEXT("RevivePlayerServerOnly") },
				{ UMPHealthComponent::StaticClass(), TEXT("SetInvulnerableServerOnly") },
				{ UMPHealthComponent::StaticClass(), TEXT("IsHealthAlive") },
				{ UMPHealthComponent::StaticClass(), TEXT("IsDowned") },
				{ UMPHealthComponent::StaticClass(), TEXT("IsDead") }
			};
			Categories.Add(Cat);
		}

		// 20. Multiplayer Physics Synchronization
		{
			FMPShowcaseCategoryLayout Cat;
			Cat.Title = TEXT("20. Multiplayer Physics Synchronization");
			Cat.Color = FLinearColor(0.60f, 0.30f, 0.85f, 1.0f);
			Cat.Functions = {
				{ UMPPhysicsReplicationHelper::StaticClass(), TEXT("InitNetworkRagdollSafe") },
				{ UMPPhysicsReplicationHelper::StaticClass(), TEXT("PutPhysicsToSleepServerOnly") },
				{ UMPPhysicsReplicationHelper::StaticClass(), TEXT("WakePhysicsServerOnly") },
				{ UMPPhysicsReplicationHelper::StaticClass(), TEXT("SetNetworkPhysicsDampingSafe") },
				{ UMPPhysicsReplicationHelper::StaticClass(), TEXT("ValidatePhysicsTransformError") }
			};
			Categories.Add(Cat);
		}

		// 21. Advanced Replication & Conditions
		{
			FMPShowcaseCategoryLayout Cat;
			Cat.Title = TEXT("21. Advanced Replication & Conditions");
			Cat.Color = FLinearColor(0.80f, 0.65f, 0.10f, 1.0f);
			Cat.Functions = {
				{ UMPReplicationConditions::StaticClass(), TEXT("DescribeReplicationCondition") }
			};
			Categories.Add(Cat);
		}

		// 22. Multiplayer Player & Network Helpers
		{
			FMPShowcaseCategoryLayout Cat;
			Cat.Title = TEXT("22. Multiplayer Player & Network Helpers");
			Cat.Color = FLinearColor(0.15f, 0.55f, 0.90f, 1.0f);
			Cat.Functions = {
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("GetLocalPlayerState") },
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("GetPlayerStateFromActor") },
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("GetPlayerControllerFromActor") },
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("IsPlayerLocallyViewed") },
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("GetPlayerControllerIndex") },
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("GetAllRemotePlayerControllers") },
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("IsPlayerConnectionActive") },
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("GetPlayerPingFormatted") }
			};
			Categories.Add(Cat);
		}

		// 23. Multiplayer RPC Dispatch & Rate Limiting
		{
			FMPShowcaseCategoryLayout Cat;
			Cat.Title = TEXT("23. Multiplayer RPC Dispatch & Rate Limiting");
			Cat.Color = FLinearColor(0.75f, 0.25f, 0.85f, 1.0f);
			Cat.Functions = {
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("CanActorSendServerRPC") },
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("GetRPCExecutionScope") },
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("CheckRPCRateLimitGate") },
				{ UMPRPCGameplayComponent::StaticClass(), TEXT("SendServerGameplayEvent") },
				{ UMPRPCGameplayComponent::StaticClass(), TEXT("SendClientNotification") },
				{ UMPRPCGameplayComponent::StaticClass(), TEXT("BroadcastCosmeticCue") },
				{ UMPRPCGameplayComponent::StaticClass(), TEXT("BroadcastSoundAtLocation") }
			};
			Categories.Add(Cat);
		}

		// 24. Multiplayer Replication & Dormancy Helpers
		{
			FMPShowcaseCategoryLayout Cat;
			Cat.Title = TEXT("24. Multiplayer Replication & Dormancy Helpers");
			Cat.Color = FLinearColor(0.10f, 0.70f, 0.45f, 1.0f);
			Cat.Functions = {
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("SetActorReplicationSafe") },
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("SetActorNetDormantSafe") },
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("IsActorDormant") },
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("GetActorNetUpdateFrequency") },
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("ForceNetUpdateWithAttachedActors") },
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("TearOffActorSafe") },
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("IsActorTornOff") }
			};
			Categories.Add(Cat);
		}

		// 25. Multiplayer Combat & Authoritative Gameplay
		{
			FMPShowcaseCategoryLayout Cat;
			Cat.Title = TEXT("25. Multiplayer Combat & Authoritative Gameplay");
			Cat.Color = FLinearColor(0.90f, 0.20f, 0.25f, 1.0f);
			Cat.Functions = {
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("CanDamageActorAuthoritative") },
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("ApplyPointDamageServerOnly") },
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("ApplyRadialDamageServerOnly") },
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("SpawnReplicatedActorServerOnly") },
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("RespawnPlayerPawnAtTransform") },
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("FindBestPlayerStart") },
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("TeleportPawnServerOnly") },
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("SetPlayerScoreServerOnly") }
			};
			Categories.Add(Cat);
		}

		// 26. Multiplayer Clock Synchronization & Latency
		{
			FMPShowcaseCategoryLayout Cat;
			Cat.Title = TEXT("26. Multiplayer Clock Synchronization & Latency");
			Cat.Color = FLinearColor(0.20f, 0.75f, 0.80f, 1.0f);
			Cat.Functions = {
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("GetServerTimeSynced") },
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("GetTimeSinceServerTimestamp") },
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("CalculateLatencyCompensatedLocation") },
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("AreTimestampsOrdered") },
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("GetServerTickRate") },
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("IsServerTickCadence") }
			};
			Categories.Add(Cat);
		}

		// 27. Multiplayer Network Diagnostics & Debugging
		{
			FMPShowcaseCategoryLayout Cat;
			Cat.Title = TEXT("27. Multiplayer Network Diagnostics & Debugging");
			Cat.Color = FLinearColor(0.85f, 0.50f, 0.15f, 1.0f);
			Cat.Functions = {
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("GetActorNetworkDebugString") },
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("DrawNetworkRoleDebugString") },
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("LogNetworkEvent") },
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("SimulateNetworkConditionCVar") },
				{ UMPGameplayHelperLibrary::StaticClass(), TEXT("DumpReplicationInfoToLog") }
			};
			Categories.Add(Cat);
		}

		return Categories;
	}
}

int32 UMPEditorScannerSubsystem::PopulateBlueprintWithAllUtilityNodes(UBlueprint* TargetBlueprint, bool bCopyToClipboard)
{
	if (!TargetBlueprint)
	{
		return 0;
	}

	UEdGraph* EventGraph = FBlueprintEditorUtils::FindEventGraph(TargetBlueprint);
	if (!EventGraph)
	{
		if (TargetBlueprint->UbergraphPages.Num() > 0)
		{
			EventGraph = TargetBlueprint->UbergraphPages[0];
		}
	}

	if (!EventGraph)
	{
		return 0;
	}

	// Clean out existing nodes to ensure clean generation
	TArray<UEdGraphNode*> OldNodes = EventGraph->Nodes;
	for (UEdGraphNode* OldNode : OldNodes)
	{
		if (OldNode)
		{
			EventGraph->RemoveNode(OldNode);
		}
	}

	const TArray<FMPShowcaseCategoryLayout> Categories = BuildShowcaseCategories();

	// 10-column organization
	const TArray<TArray<int32>> ColumnMappings = {
		{ 0, 1 },
		{ 2, 3 },
		{ 4, 5, 6 },
		{ 7, 8, 9 },
		{ 10, 11, 12 },
		{ 13, 14 },
		{ 15, 16, 17 },
		{ 18, 19, 20 },
		{ 21, 22, 23 },
		{ 24, 25, 26 }
	};

	const int32 ColWidth = 700;
	const int32 ColPadding = 120;
	const int32 RowPadding = 100;
	const int32 NodeYSpacing = 160;
	const int32 BoxHeader = 80;
	const int32 NodeXIndent = 50;

	int32 TotalNodesCreated = 0;

	for (int32 ColIdx = 0; ColIdx < ColumnMappings.Num(); ++ColIdx)
	{
		const int32 ColX = ColIdx * (ColWidth + ColPadding);
		int32 CurrentY = 0;

		for (int32 CatIdx : ColumnMappings[ColIdx])
		{
			if (!Categories.IsValidIndex(CatIdx))
			{
				continue;
			}

			const FMPShowcaseCategoryLayout& Cat = Categories[CatIdx];
			const int32 BoxHeight = BoxHeader + (Cat.Functions.Num() * NodeYSpacing) + 60;

			// 1. Create Comment Box
			UEdGraphNode_Comment* CommentNode = NewObject<UEdGraphNode_Comment>(EventGraph);
			CommentNode->SetFlags(RF_Transactional);
			CommentNode->NodePosX = ColX;
			CommentNode->NodePosY = CurrentY;
			CommentNode->NodeWidth = ColWidth;
			CommentNode->NodeHeight = BoxHeight;
			CommentNode->NodeComment = Cat.Title;
			CommentNode->CommentColor = Cat.Color;
			CommentNode->CreateNewGuid();
			CommentNode->PostPlacedNewNode();
			EventGraph->AddNode(CommentNode, false, false);
			TotalNodesCreated++;

			// 2. Create CallFunction nodes inside the box
			for (int32 FuncIdx = 0; FuncIdx < Cat.Functions.Num(); ++FuncIdx)
			{
				const FMPShowcaseFuncItem& Item = Cat.Functions[FuncIdx];
				if (!Item.TargetClass)
				{
					continue;
				}

				UFunction* TargetFunc = Item.TargetClass->FindFunctionByName(Item.FunctionName);
				if (!TargetFunc)
				{
					UE_LOG(LogNet, Warning, TEXT("[MPEditorScannerSubsystem] Could not find function '%s' on '%s'"), *Item.FunctionName.ToString(), *Item.TargetClass->GetName());
					continue;
				}

				UK2Node_CallFunction* CallNode = NewObject<UK2Node_CallFunction>(EventGraph);
				CallNode->SetFlags(RF_Transactional);
				EventGraph->AddNode(CallNode, false, false);
				CallNode->SetFromFunction(TargetFunc);
				CallNode->NodePosX = ColX + NodeXIndent;
				CallNode->NodePosY = CurrentY + BoxHeader + (FuncIdx * NodeYSpacing);
				CallNode->CreateNewGuid();
				CallNode->PostPlacedNewNode();
				CallNode->AllocateDefaultPins();
				TotalNodesCreated++;
			}

			CurrentY += BoxHeight + RowPadding;
		}
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(TargetBlueprint);
	FKismetEditorUtilities::CompileBlueprint(TargetBlueprint);

	if (bCopyToClipboard)
	{
		TSet<UObject*> NodesToExport;
		for (UEdGraphNode* Node : EventGraph->Nodes)
		{
			if (Node)
			{
				NodesToExport.Add(Node);
			}
		}

		FString ExportedText;
		FEdGraphUtilities::ExportNodesToText(NodesToExport, ExportedText);
		FPlatformApplicationMisc::ClipboardCopy(*ExportedText);

		const FString OutputPath = FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("MultiplayerUtility/MultiplayerUtility_BlueprintGraph_Clipboard.txt"));
		FFileHelper::SaveStringToFile(ExportedText, *OutputPath);

		const FString RootOutputPath = FPaths::Combine(FPaths::ProjectDir(), TEXT("MultiplayerUtility_BlueprintGraph_Clipboard.txt"));
		FFileHelper::SaveStringToFile(ExportedText, *RootOutputPath);

		UE_LOG(LogNet, Log, TEXT("[MPEditorScannerSubsystem] Copied %d nodes (%d bytes) to Windows Clipboard and saved to '%s' and '%s'"), NodesToExport.Num(), ExportedText.Len(), *OutputPath, *RootOutputPath);
	}

	UE_LOG(LogNet, Log, TEXT("[MPEditorScannerSubsystem] Successfully created %d total nodes in '%s'"), TotalNodesCreated, *TargetBlueprint->GetName());
	return TotalNodesCreated;
}

bool UMPEditorScannerSubsystem::GenerateShowcaseBlueprint(const FString& BlueprintPackagePath)
{
	const FString TargetPath = BlueprintPackagePath.IsEmpty() ? TEXT("/Game/Blueprints/BP_MultiplayerUtility_Showcase") : BlueprintPackagePath;
	const FString AssetName = FPaths::GetBaseFilename(TargetPath);

	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *TargetPath);
	UPackage* Package = nullptr;

	if (!Blueprint)
	{
		Package = CreatePackage(*TargetPath);
		if (!Package)
		{
			UE_LOG(LogNet, Error, TEXT("[MPEditorScannerSubsystem] Failed to create package '%s'"), *TargetPath);
			return false;
		}
		Package->FullyLoad();

		UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
		Factory->ParentClass = AActor::StaticClass();

		Blueprint = Cast<UBlueprint>(Factory->FactoryCreateNew(
			UBlueprint::StaticClass(),
			Package,
			*AssetName,
			RF_Public | RF_Standalone,
			nullptr,
			GWarn
		));

		if (!Blueprint)
		{
			UE_LOG(LogNet, Error, TEXT("[MPEditorScannerSubsystem] Failed to create Blueprint asset '%s'"), *AssetName);
			return false;
		}

		FAssetRegistryModule::AssetCreated(Blueprint);
	}
	else
	{
		Package = Blueprint->GetOutermost();
	}

	const int32 NodeCount = PopulateBlueprintWithAllUtilityNodes(Blueprint, true);
	if (NodeCount <= 0)
	{
		return false;
	}

	if (Package)
	{
		Package->MarkPackageDirty();
		UEditorLoadingAndSavingUtils::SavePackages({ Package }, false);
	}

	return true;
}
