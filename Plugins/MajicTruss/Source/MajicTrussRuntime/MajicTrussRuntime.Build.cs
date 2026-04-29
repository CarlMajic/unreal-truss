using UnrealBuildTool;

[SupportedTargetTypes(TargetType.Editor, TargetType.Game, TargetType.Client, TargetType.Server)]
public class MajicTrussRuntime : ModuleRules
{
	public MajicTrussRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"AssetRegistry"
		});
	}
}
