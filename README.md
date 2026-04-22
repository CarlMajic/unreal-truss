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

## Notes

Unreal uses centimeters internally, so piece lengths are stored in centimeters.
