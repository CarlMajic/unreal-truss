import unreal


def main():
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    assets = registry.get_assets_by_path("/Game/Venue", recursive=True)
    static_mesh_assets = [asset for asset in assets if asset.asset_class_path.asset_name == "StaticMesh"]

    fixed_count = 0
    skipped_count = 0

    for asset_data in static_mesh_assets:
        mesh = asset_data.get_asset()
        if not mesh:
            skipped_count += 1
            continue

        body_setup = mesh.get_editor_property("body_setup")
        if not body_setup:
            skipped_count += 1
            unreal.log_warning(f"[VenueCollision] Skipped {asset_data.package_name}: no BodySetup")
            continue

        current_flag = body_setup.get_editor_property("collision_trace_flag")
        desired_flag = unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE
        if current_flag != desired_flag:
            mesh.modify()
            body_setup.modify()
            body_setup.set_editor_property("collision_trace_flag", desired_flag)
            unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False)
            fixed_count += 1
            unreal.log(f"[VenueCollision] Fixed {asset_data.package_name}: {current_flag} -> {desired_flag}")
        else:
            skipped_count += 1

    unreal.log(f"[VenueCollision] Complete. Fixed {fixed_count}, skipped {skipped_count}.")


if __name__ == "__main__":
    main()
