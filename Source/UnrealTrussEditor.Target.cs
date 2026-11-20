using UnrealBuildTool;
using System.Collections.Generic;

[SupportedTargetTypes(TargetType.Editor)]
public class UnrealTrussEditorTarget : TargetRules
{
	public UnrealTrussEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("UnrealTruss");
	}
}

