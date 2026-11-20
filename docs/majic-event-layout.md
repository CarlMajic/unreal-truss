# Majic Event Layout in Unreal

The MajicTruss plugin reads and writes the shared Majic Event Layout file (the same file
Blender and Fusion use, format in `majic-layout-format/docs/FORMAT.md`).

## Use

1. Place a **Majic Event Layout** actor (`AMajicEventLayoutActor`) in the level.
2. In its Details panel press **Import Event Layout** and pick a file. Every stage, truss,
   drape and MBP wall in the file is built in one go, each with its name, position and rotation.
3. Press **Export Event Layout** to write the level's stages, trusses, drapes and MBP walls to a
   file. Item types Unreal does not build (and control drapes) already in that file are kept.

Both are also callable from Blueprint or C++ as `ImportLayoutFile` / `ExportLayoutFile`.

Importing again skips items whose id is already in the level. Every built actor carries a
`MajicLayoutStamp` component with the item id and the params from the file, so export writes the
same ids back and keeps fields Unreal cannot show. Changes you make to an actor (size, heights,
rails, panels) are read live on export.

## Placement

The file's floor is x to house right, y away from the audience. The Unreal actors are authored
with their front toward -Y, so the world puts the audience at +Y: world X = x, world Y = -y, and
every actor is turned 180 degrees (yaw = 180 - rotation). A column index that counts left to right in
the file therefore counts right to left in the actor; the importer flips stage columns, MBP
columns and end caps, and swaps left and right rails and steps.

An item's position is its origin point: the front-left corner of the footprint for stage and
truss, the footprint centre for a tower, the left end of the run for a drape and the left end of
a wall.

## What does not carry over

| Item | Limit |
|---|---|
| Stage heights | The file's 20 in deck uses Unreal's "24 in" preset, and back again on export. 27.5 in uses "27 in". |
| Stage steps | Unreal has only the Stage Right steps (black carpet is noted and ignored) and always puts the stairs on the last row (the back), not the front row. |
| Stage skirt | `skirt: true` turns on Unreal's automatic skirt and turns off its debug single skirt. |
| Truss | T-top towers are built as plain towers. Rectangles take `length_bays`/`width_bays` (center support lines, 1 to 20 each); lights snap to any run, including the center runs (see free light mounting in the truss actor). |
| Drape | Control drapes are skipped (and kept in the file); height is clamped to 6 to 18.33 ft. |
| MBP | Custom panels have no web style and are left out on export. Rows other than 3 ft high are not supported by the importer. |

## Tests

Headless, from the project folder:

```
UnrealEditor-Cmd.exe UnrealTruss.uproject -ExecCmds="Automation RunTests Majic.EventLayout; Quit" -unattended -nullrhi
```

`Majic.EventLayout.Parse`, `.Merge`, `.Placement` and `.Actors` check reading, writing, merging,
the placement maths, and a full import then export of one item of each type in the editor world.
