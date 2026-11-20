using UnrealBuildTool;

public class MajicTrussEditor : ModuleRules
{
	public MajicTrussEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"AssetRegistry",
			"MajicTrussRuntime",
			"PropertyEditor",
			"Slate",
			"SlateCore"
		});
	}
}
