# Unreal Truss

Unreal Truss is a runtime-first Unreal Engine 5.6 plugin for building event-production truss structures from real inventory lengths.

The first milestone is a straight truss run. The plugin keeps the truss math in runtime C++ so the same system can be used later by editor tools, Blueprint gameplay, and in-game UI.

## Current Scope

- Runtime plugin module: `MajicTrussRuntime`
- Actor: `ATrussStructureActor`
- Inventory Data Asset: `UTrussInventoryDataAsset`
- Straight run generation using real truss lengths:
  - 10 ft
  - 8 ft
  - 5 ft
  - 4 ft
  - 2 ft
- Instanced Static Mesh output when meshes are assigned
- Debug box output when meshes are not assigned yet

## Testing

Open `UnrealTruss.uproject` with Unreal Engine 5.6.

Create a `Truss Inventory Data Asset`, assign meshes when available, place a `Truss Structure Actor`, and call `Build Straight Run` or enable `Build On Construction`.

The plugin can also be copied into another Unreal Engine 5.6 project's `Plugins` folder.

If no inventory asset is assigned, `Truss Structure Actor` attempts to use the migrated Majic Gear truss meshes at:

- `/Game/Majic_Gear/Truss/10ftTruss/StaticMeshes/SM__0_ft_Truss_v2`
- `/Game/Majic_Gear/Truss/8ftTruss/StaticMeshes/SM___ft_Truss_v1`
- `/Game/Majic_Gear/Truss/5ftTruss/StaticMeshes/SM___ft_Truss_v1`
- `/Game/Majic_Gear/Truss/4ftTruss/StaticMeshes/SM___ft_Truss_v2`
- `/Game/Majic_Gear/Truss/2ftTruss/StaticMeshes/SM___ft_Truss_v1`

The default `Mesh Scale Multiplier` is `0.0254` for migrated assets that arrive at inch-style scale. The truss spacing still uses Unreal centimeters, so a 20 ft run remains 609.6 cm long.

## Notes

Unreal uses centimeters internally, so piece lengths are stored in centimeters.
