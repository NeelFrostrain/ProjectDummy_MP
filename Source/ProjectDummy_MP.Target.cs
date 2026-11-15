using UnrealBuildTool;
using System.Collections.Generic;

public class ProjectDummy_MPTarget : TargetRules
{
	public ProjectDummy_MPTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		bOverrideBuildEnvironment = true;
		ExtraModuleNames.Add("ProjectDummy_MP");
	}
}
