using UnrealBuildTool;

public class ModularInventory : ModuleRules
{
    public ModularInventory(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "Engine", "NetCore", "InputCore",
            "GameplayTags", "UMG", "Slate", "SlateCore"
        });
    }
}
