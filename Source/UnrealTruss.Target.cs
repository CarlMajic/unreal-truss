using UnrealBuildTool;
using System.Collections.Generic;

[SupportedTargetTypes(TargetType.Game)]
public class UnrealTrussTarget : TargetRules
{
	public UnrealTrussTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
		ExtraModuleNames.Add("UnrealTruss");
	}
}

