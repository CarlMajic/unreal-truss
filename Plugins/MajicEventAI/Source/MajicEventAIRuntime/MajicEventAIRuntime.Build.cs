using UnrealBuildTool;

[SupportedTargetTypes(TargetType.Editor, TargetType.Game, TargetType.Client, TargetType.Server)]
public class MajicEventAIRuntime : ModuleRules
{
	public MajicEventAIRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",

			// AI & navigation
			"AIModule",           // AIController, BehaviorTree tasks/decorators/services
			"NavigationSystem",   // NavMesh, pathfinding queries

			// Smart Object System (UE5 — NPC slot claiming on placed actors)
			"SmartObjectsModule",

			// Gameplay Tasks (required by BehaviorTree and AIModule)
			"GameplayTasks",

			// Gameplay Tags (phase names, interaction slot tags)
			"GameplayTags",

			// Majic venue actors — lounge, stage, etc.
			"MajicTrussRuntime"
		});

		PrivateDependencyModuleNames.AddRange(new[]
		{
			// Gameplay Behaviors — bridge between Smart Objects and AI agents
			"GameplayBehaviorsModule",
			"GameplayBehaviorSmartObjectsModule"
		});
	}
}
