import unreal


def main():
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    assets = registry.get_assets_by_path("/Game/Venue", recursive=True)
    static_mesh_assets = [asset for asset in assets if asset.asset_class_path.asset_name == "StaticMesh"]

    unreal.log(f"[VenueCollision] Found {len(static_mesh_assets)} Venue static meshes")
    for asset_data in static_mesh_assets:
        mesh = asset_data.get_asset()
        if not mesh:
            continue

        body_setup = mesh.get_editor_property("body_setup")
        collision_trace_flag = body_setup.get_editor_property("collision_trace_flag") if body_setup else "None"
        bounds = mesh.get_bounds()
        unreal.log(
            "[VenueCollision] "
            f"{asset_data.package_name} "
            f"collision={collision_trace_flag} "
            f"extent={bounds.box_extent}"
        )


if __name__ == "__main__":
    main()
