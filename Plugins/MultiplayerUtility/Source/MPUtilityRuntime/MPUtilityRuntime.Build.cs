using UnrealBuildTool;

public class MPUtilityRuntime : ModuleRules
{
    public MPUtilityRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "NetCore",
            "CoreOnline",
            "InputCore",
            "DeveloperSettings",
            "OnlineSubsystem",
            "OnlineSubsystemUtils",
            "Sockets",
            "GameplayTags"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Slate",
            "SlateCore"
        });
    }
}

