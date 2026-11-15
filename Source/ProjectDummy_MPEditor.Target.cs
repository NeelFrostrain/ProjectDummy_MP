using UnrealBuildTool;
using System.Collections.Generic;

public class ProjectDummy_MPEditorTarget : TargetRules
{
	public ProjectDummy_MPEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		bOverrideBuildEnvironment = true;
		ExtraModuleNames.Add("ProjectDummy_MP");
	}
}
