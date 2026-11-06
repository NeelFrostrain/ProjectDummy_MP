using UnrealBuildTool;

public class MPUtilityTests : ModuleRules
{
    public MPUtilityTests(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "MPUtilityRuntime",
            "GameplayTags"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "NetCore",
            "CoreOnline",
            "UnrealEd",
            "MPUtilityEditor"
        });
    }
}

