import unreal


PACKAGE_PATH = "/Game/Furniture/Dining_Seating/BBJ_Linens"
ASSET_NAME = "M_Tablecloth_TextureSwap"
ASSET_PATH = f"{PACKAGE_PATH}/{ASSET_NAME}.{ASSET_NAME}"
DEFAULT_TEXTURE_PATH = "/Game/Furniture/Dining_Seating/BBJ_Linens/Table_Cloths/Arch_Dijon_Table_Linen.Arch_Dijon_Table_Linen"


def main():
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    material = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)

    if material is None:
        factory = unreal.MaterialFactoryNew()
        material = asset_tools.create_asset(ASSET_NAME, PACKAGE_PATH, unreal.Material, factory)

    if material is None:
        raise RuntimeError(f"Failed to create {ASSET_PATH}")

    material.set_editor_property("two_sided", True)
    material.set_editor_property("use_material_attributes", False)

    # Rebuild a tiny, predictable graph: one swapped linen texture into Base Color.
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)

    texture_sample = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionTextureSampleParameter2D,
        -420,
        -80,
    )
    texture_sample.set_editor_property("parameter_name", "TableclothTexture")

    default_texture = unreal.EditorAssetLibrary.load_asset(DEFAULT_TEXTURE_PATH)
    if default_texture is not None:
        texture_sample.set_editor_property("texture", default_texture)

    roughness = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionConstant,
        -420,
        150,
    )
    roughness.set_editor_property("r", 0.72)

    unreal.MaterialEditingLibrary.connect_material_property(
        texture_sample,
        "RGB",
        unreal.MaterialProperty.MP_BASE_COLOR,
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        roughness,
        "",
        unreal.MaterialProperty.MP_ROUGHNESS,
    )

    unreal.MaterialEditingLibrary.layout_material_expressions(material)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_asset(ASSET_PATH)
    unreal.log(f"Created/updated tablecloth material: {ASSET_PATH}")


if __name__ == "__main__":
    main()
