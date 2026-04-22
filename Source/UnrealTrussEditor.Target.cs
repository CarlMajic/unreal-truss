using UnrealBuildTool;
using System.Collections.Generic;

[SupportedTargetTypes(TargetType.Editor)]
public class UnrealTrussEditorTarget : TargetRules
{
	public UnrealTrussEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
		ExtraModuleNames.Add("UnrealTruss");
	}
}

