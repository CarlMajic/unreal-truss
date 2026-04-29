# Unreal Truss

Unreal Truss is a runtime-first Unreal Engine 5.6 plugin for building event-production truss structures from real inventory lengths.

The first milestone is a straight truss run. The plugin keeps the truss math in runtime C++ so the same system can be used later by editor tools, Blueprint gameplay, and in-game UI.

## Project Log

### 2026-04-29

Current direction:

- Keep pushing toward a reusable event-build tool instead of a truss-only prototype.
- Keep generation/runtime logic in the runtime plugin so editor, desktop runtime, and later VR can share the same core systems.
- Build MBP as the next major modular system, starting editor-first but keeping the data model runtime-ready.

What has been done:

- Expanded editor-mounted truss fixture workflow beyond the original single-span assumption.
- Added named rail selection for editor-mounted fixtures:
  - `Left Top`
  - `Right Top`
  - `Left Bottom`
  - `Right Bottom`
- Added explicit horizontal span selection for multi-run structures:
  - `Main Span`
  - `Front X Run`
  - `Back X Run`
  - `Left Y Run`
  - `Right Y Run`
- Updated mounted fixture rebuild behavior so top/bottom fixture height follows the current truss span height instead of full-structure floor bounds.
- Improved rectangle/cube light placement logic to distinguish X runs and Y runs rather than treating the whole structure as one horizontal mount area.
- Added a pragmatic Y-run mount correction field on the truss actor:
  - `Y Run Mount X Adjustment Cm`
- Added first-pass MBP wall builder actor in the runtime plugin:
  - `AMBPWallActor`
  - mixed per-slot styles
  - per-slot shimmer material variant
  - per-slot depth offset
  - instanced rendering by style/material bucket
- Wired the first MBP styles from migrated Majic Gear content:
  - `Acrylic`
  - `Drift`
  - `Geo`
  - `Shimmer`
  - `Hive`
  - `Platinum`
- Confirmed the current MBP wall assumption is a 3 ft x 3 ft panel grid with future support needed for deeper pattern tooling and runtime placement.
- Confirmed the current architecture should support later VR work without rewriting truss/MBP generation, as long as VR is added as a new input/targeting/UI layer.

Immediate next steps:

- Validate the final Y-run truss fixture alignment on rectangle and cube using the editable correction field.
- Improve `AMBPWallActor` editor workflow:
  - easier slot editing
  - pattern helpers
  - better default authoring flow for mixed-style walls
- Add MBP as a runtime buildable item using the same runtime build framework used by truss.
- Extend MBP later with:
  - per-panel forward/back offsets
  - faster pattern editing
  - mixed-style wall presets
- Revisit stage deck building after MBP is stable.
- Keep VR as a later integration target:
  - new VR pawn/controller
  - controller-ray targeting
  - world-space UI
  - reuse existing build actors/data definitions

Notes for future sessions:

- `GeneratedBounds` is still fine for selection/highlight, but truss fixture placement on complex structures may need span-specific bounds or explicit corrections instead of whole-structure assumptions.
- MBP should stay data-driven from day one so editor authoring, desktop runtime placement, and later VR can all hit the same wall definition.
- Do not bury too much wall-authoring logic in one-off editor utilities if the same wall data will later be placed at runtime.

### 2026-04-27

Current direction:

- Treat truss as the first buildable item, not the only buildable item.
- Build a lightweight runtime building framework first, then plug truss into it.
- Keep the system low-overhead so it can run on weaker hardware.

What has been done:

- Confirmed the truss generator already lives in the runtime plugin `MajicTrussRuntime`.
- Reviewed the current runtime actor `ATrussStructureActor` and the truss math/inventory code.
- Confirmed GitHub backup remote is `https://github.com/CarlMajic/unreal-truss.git`.
- Added first-pass runtime building framework classes:
  - `UBuildItemDataAsset` for data-driven buildable items
  - `UBuildManagerComponent` for runtime build mode, tracing, preview movement, rotation, and placement
  - `ABuildPreviewActor` for lightweight in-world preview handling
- Added `FTrussBuildDefinition` plus `ApplyBuildDefinition`, `GetBuildDefinition`, and `BuildCurrentMode` on `ATrussStructureActor` so truss can be configured and built at runtime through a generic placement flow.
- Added a code-first playable test setup in the game module:
- Added a code-first playable test setup in the game module:
  - `AUnrealTrussBuildPawn`
  - `AUnrealTrussGameMode`
- Configured project defaults so Play mode uses the C++ game mode and build pawn automatically.
- Added default keyboard/mouse bindings for a first runtime placement test.
- Added a first placeholder UMG build menu in C++:
- Added a first placeholder UMG build menu in C++:
  - `UBuildMenuWidget`
  - runtime item listing from `BuildItemDataAsset` assets
  - item selection that feeds the build manager
  - truss mode selection and mode-dependent size editing in the placeholder menu
  - straight/rectangle hanging height controls
  - cube-arch side/depth spacer piece selection
  - shared create/update action path for later editor-placed truss editing

Why this direction:

- It gives us a reusable building foundation for truss, stage decks, video walls, lights, and other gear.
- It avoids overbuilding a full inventory/economy system before the placement workflow exists.
- It keeps the runtime path centered on data assets, line traces, and instanced geometry instead of expensive always-on systems.

Immediate next steps:

- Test the new runtime building framework in Unreal Engine 5.6 using the default C++ pawn.
- Create one or more `Build Item Data Asset` assets for truss.
- Add a basic UMG build menu that lets the player select a build item and tweak truss dimensions.
- Add editing of additional truss-specific options like cube-arch spacing pieces and alignment offsets when needed.
- Reuse the same truss settings panel later for editing already placed editor-built truss actors during play.
- First-pass in-game editing flow is now based on selecting an existing `ATrussStructureActor`, loading its `FTrussBuildDefinition`, and updating that actor through the same menu.
- Existing truss actors now expose a selection bounds box for reliable hover/edit targeting during play.
- Editing an existing truss now rebuilds it in place instead of moving it to the preview transform.
- Improve preview feedback:
  - valid/invalid placement materials or colors
  - snap behavior
  - optional surface/grid modes
- Add save/load for placed build descriptors after the placement flow is stable.

Notes for future sessions:

- Prefer generic build-system work over truss-only hacks unless the generic path is clearly too expensive.
- Keep actor counts low and favor instanced mesh output where possible.
- Avoid full inventory/crafting complexity until placement and UI feel solid.

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
- Rectangle generation with four corner blocks and four straight runs
- Arch generation with bases, vertical legs, top corner blocks, and a horizontal span
- Cube generation with four bases, four vertical legs, top corner blocks, and top rectangle runs
- Instanced Static Mesh output when meshes are assigned
- Debug box output when meshes are not assigned yet
- First-pass generic building framework:
  - `UBuildItemDataAsset`
  - `UBuildManagerComponent`
  - `ABuildPreviewActor`

## Testing

Open `UnrealTruss.uproject` with Unreal Engine 5.6.

Create a `Truss Inventory Data Asset`, assign meshes when available, place a `Truss Structure Actor`, and call `Build Straight Run` or enable `Build On Construction`.

To test a rectangle, place/select `Truss Structure Actor`, set `Build Mode` to `Rectangle`, then set `Rectangle Length Ft` and `Rectangle Width Ft`. The actor also exposes `Build Rectangle` as a Blueprint-callable runtime API.

Rectangle side runs use `Rectangle Y Run X Offset Cm`, default `30.48` cm / 12 in, to align Y-direction truss sections with the corner block connection face.

To test an arch, set `Build Mode` to `Arch`, then adjust `Arch Height Ft` and `Arch Width Ft`. Arch alignment exposes 6-inch connection defaults through `Arch Corner Connection Offset Cm`, `Arch Leg Y Offset Cm`, `Arch Vertical Leg X Offset Cm`, `Arch Base Y Offset Cm`, and `Arch Span Y Offset Cm`. Vertical section rotation is exposed as explicit X/Y/Z degree fields and defaults to Y 90, Z 0 for the imported truss meshes.

To test a cube, set `Build Mode` to `Cube`, then adjust `Cube Length Ft`, `Cube Width Ft`, and `Cube Height Ft`.

To begin runtime building integration, add `UBuildManagerComponent` to the player controller or pawn, create a `Build Item Data Asset`, set its `Build Actor Class` to `ATrussStructureActor`, then drive `Enter Build Mode`, `Set Selected Build Item`, `Update Preview From Player View`, `Rotate Preview Yaw`, and `Confirm Placement` from Blueprint input/UI.

For the current code-first test path, just create a `Build Item Data Asset` for truss and hit Play. The default C++ game mode and pawn should load automatically.

Current test controls:

- `WASD`: move
- `Space`: move up
- `Left Ctrl`: move down
- `Mouse`: look
- `Tab`: open or close the placeholder build menu
- `B`: toggle build mode
- `E`: edit the truss actor currently under the view
- `L`: toggle truss light placement mode
- Looking at a truss actor with the menu closed should now highlight its selection bounds before pressing `E`.
- `Left Mouse Button`: place the selected build item
- `R`: rotate positive
- `F`: rotate negative
- `Q`: cancel build mode

Light placement first pass:

- Press `L` to enter light placement mode.
- A separate light menu opens with lighting blueprints discovered from `/Game/Majic_Gear/Lighting`.
- Choose a fixture and `Over Slung` or `Under Slung`, then click `Place`.
- After clicking `Place`, a live preview of that light follows the truss pointer.
- Left click on a truss rail to place the selected light.
- The same light remains active so repeated placement does not require reopening the menu every time.
- Press `L` again or `Q` to cancel the active light placement session.
- Runtime light placement now writes to `ATrussStructureActor` mounted-fixture definitions instead of leaving loose attached actors.
- That means runtime-placed lights and editor-placed lights now share the same underlying truss-owned data model.
- This pass uses a simple four-rail mount model on the truss bounds:
  - left/right rail from the click side
  - top rails for `Over Slung`
  - bottom rails for `Under Slung`
- This is intended as the first runtime workflow for hanging fixtures. It should be refined later with fixture-specific orientation, clamp offsets, and better mount previews.

Shared targeting pointer:

- The project now has a shared camera-driven targeting pointer component.
- It uses the same core concept for both truss placement and truss light placement:
  - trace from the player camera
  - draw a visible beam in world space
  - show a hit marker at the impact point
- `B` build mode uses the pointer for world placement.
- `L` light mode uses the pointer for truss rail targeting.
- This keeps the targeting workflow unified so later systems like stage placement can reuse it instead of creating separate one-off traces.
- Current visuals use lightweight engine basic-shape meshes for the beam and hit marker. This is a practical first pass and can later be upgraded to nicer materials or Niagara effects if needed.

Current direction:

- Keep truss create/edit and light placement as separate workflows.
- Keep the targeting layer shared across build systems.
- Use truss-targeted runtime mounting for DMX fixtures next, then expand into better preview/edit tools for placed lights.
- Treat stage building as a separate system later, likely grid/cell based with per-section heights rather than a single repeated mesh array.

Editor light placement first pass:

- `ATrussStructureActor` now owns persistent `Mounted Fixtures`.
- In the editor, select a truss actor and use:
  - `Editor Fixture Class`
  - `Editor Fixture Sling Type`
  - `Editor Fixture Local Hit Location` (has an editor viewport widget)
- Then click `Add Editor Mounted Fixture`.
- `Rebuild Mounted Fixtures` happens through construction, so mounted lights respawn when the truss rebuilds.
- `Clear Mounted Fixtures` removes all stored mounted-light definitions from that truss actor.
- This is the first editor-side workflow. It uses a viewport handle plus `Call In Editor` buttons rather than a full custom editor mode.

The plugin can also be copied into another Unreal Engine 5.6 project's `Plugins` folder.

If no inventory asset is assigned, `Truss Structure Actor` attempts to use the migrated Majic Gear truss meshes at:

- `/Game/Majic_Gear/Truss/10ftTruss/StaticMeshes/SM__0_ft_Truss_v2`
- `/Game/Majic_Gear/Truss/8ftTruss/StaticMeshes/SM___ft_Truss_v1`
- `/Game/Majic_Gear/Truss/5ftTruss/StaticMeshes/SM___ft_Truss_v1`
- `/Game/Majic_Gear/Truss/4ftTruss/StaticMeshes/SM___ft_Truss_v2`
- `/Game/Majic_Gear/Truss/2ftTruss/StaticMeshes/SM___ft_Truss_v1`

The default `Mesh Scale Multiplier` is `0.0254` for migrated assets that arrive at inch-style scale. The truss spacing still uses Unreal centimeters, so a 20 ft run remains 609.6 cm long.

Mesh placement is bounds-aligned: each section is shifted so its scaled local minimum X lands on the current run cursor. This is intended to tolerate imported meshes with inconsistent origins.

## Notes

Unreal uses centimeters internally, so piece lengths are stored in centimeters.
