// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "MPEditorScannerSubsystem.generated.h"

/**
 * Diagnostic scan report for an actor's network replication setup.
 */
USTRUCT(BlueprintType)
struct MPUTILITYEDITOR_API FMPActorNetworkAuditReport
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Networking|Editor")
	FString ActorName;

	UPROPERTY(BlueprintReadOnly, Category = "Networking|Editor")
	bool bReplicates = false;

	UPROPERTY(BlueprintReadOnly, Category = "Networking|Editor")
	bool bReplicatesMovement = false;

	UPROPERTY(BlueprintReadOnly, Category = "Networking|Editor")
	int32 ReplicatedPropertyCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Networking|Editor")
	int32 ReplicatedComponentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Networking|Editor")
	TArray<FString> Warnings;
};

/**
 * Editor Subsystem providing multiplayer best-practice audits, missing replication detection,
 * RPC ownership validation, and PIE network emulation presets.
 */
UCLASS()
class MPUTILITYEDITOR_API UMPEditorScannerSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Scans an actor instance or CDO for missing replication setups and configuration errors.
	 */
	UFUNCTION(BlueprintCallable, Category = "Networking|Editor")
	bool AuditActorReplication(AActor* Actor, FMPActorNetworkAuditReport& OutReport);

	/**
	 * Scans an actor for RPC methods to ensure an owning connection is configured.
	 */
	UFUNCTION(BlueprintCallable, Category = "Networking|Editor")
	bool ValidateRPCOwnership(AActor* Actor, TArray<FString>& OutWarnings);

	/**
	 * Scans all actors in the current editor world and returns replication metrics.
	 */
	UFUNCTION(BlueprintCallable, Category = "Networking|Editor")
	void AuditWorldNetworkActors(int32& OutTotalActors, int32& OutReplicatedActors, int32& OutActorsWithWarnings);

	/**
	 * Configures PIE network emulation simulation (ping, packet loss) via console variables.
	 */
	UFUNCTION(BlueprintCallable, Category = "Networking|Editor")
	static void ApplyPIENetworkEmulation(int32 TargetPingMs, float PacketLossPercent);

	/**
	 * Automatically populates a Blueprint's EventGraph with T3D node text (equivalent to pasting inside the graph).
	 */
	UFUNCTION(BlueprintCallable, Category = "Networking|Editor")
	static bool PopulateBlueprintGraphFromText(UBlueprint* TargetBlueprint, const FString& NodesText);

	/**
	 * Creates or updates a Showcase Blueprint containing all Multiplayer Utility functions neatly categorized
	 * inside comment boxes. Automatically creates nodes with allocated pins and copies the graph text to clipboard.
	 *
	 * @param BlueprintPackagePath Package path to create or update (e.g. "/Game/Blueprints/BP_MultiplayerUtility_Showcase")
	 * @return True if showcase was successfully generated.
	 */
	UFUNCTION(BlueprintCallable, Category = "Multiplayer Utility|Editor")
	static bool GenerateShowcaseBlueprint(const FString& BlueprintPackagePath = TEXT("/Game/Blueprints/BP_MultiplayerUtility_Showcase"));

	/**
	 * Directly populates any Blueprint's EventGraph with all categorized Multiplayer Utility functions,
	 * allocating all pins and comment boxes, and optionally copies the T3D graph text to the clipboard.
	 *
	 * @param TargetBlueprint The Blueprint asset to populate
	 * @param bCopyToClipboard If true, also copies the graph T3D text to Windows Clipboard for manual pasting
	 * @return Number of nodes successfully created
	 */
	UFUNCTION(BlueprintCallable, Category = "Multiplayer Utility|Editor")
	static int32 PopulateBlueprintWithAllUtilityNodes(UBlueprint* TargetBlueprint, bool bCopyToClipboard = true);
};
