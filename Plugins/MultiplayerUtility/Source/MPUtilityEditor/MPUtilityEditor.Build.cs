using UnrealBuildTool;

public class MPUtilityEditor : ModuleRules
{
    public MPUtilityEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UnrealEd",
            "EditorSubsystem",
            "MPUtilityRuntime"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Slate",
            "SlateCore",
            "BlueprintGraph",
            "KismetCompiler",
            "ApplicationCore",
            "AssetRegistry"
        });
    }
}

