using UnrealBuildTool;

[SupportedTargetTypes(TargetType.Editor, TargetType.Game, TargetType.Client, TargetType.Server)]
public class UnrealTruss : ModuleRules
{
	public UnrealTruss(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"AssetRegistry",
			"MajicTrussRuntime",
			"UMG"
		});

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"Slate",
			"SlateCore"
		});
	}
}
