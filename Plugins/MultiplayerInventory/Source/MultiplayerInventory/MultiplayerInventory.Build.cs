using UnrealBuildTool;

public class MultiplayerInventory : ModuleRules
{
    public MultiplayerInventory(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "Engine", "NetCore", "UMG", "Slate", "SlateCore", "InputCore",
            "GameplayAbilities", "GameplayTags", "GameplayTasks"
        });
        PrivateDependencyModuleNames.AddRange(new string[] {  });
    }
}
