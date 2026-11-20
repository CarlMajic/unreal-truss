using UnrealBuildTool;

[SupportedTargetTypes(TargetType.Editor, TargetType.Game, TargetType.Client, TargetType.Server)]
public class MajicTrussRuntime : ModuleRules
{
	public MajicTrussRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bUseUnity = false;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"AudioMixer",
			"Json",
			"AssetRegistry",
			"MediaAssets",
			"ProceduralMeshComponent",
			"DMXFixtures",
			"DMXProtocol",
			"DMXRuntime"
		});

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new[]
			{
				"DesktopPlatform",
				"UnrealEd"
			});
		}
	}
}
