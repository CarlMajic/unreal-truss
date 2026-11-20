# Free light mounting on any truss: design

Status: implemented 2026-11-20. Tests: Majic.Truss.RailMount, LightFreeMount, LightLegacyMount.

## Intent
Carl wants to hang lights from any truss, including the center runs the new rectangle bays add, and
later arches and cubes, without writing mounting code per truss shape.

Rules Carl gave:
- If the truss is moved or rotated, its lights move with it.
- If the truss is resized or rebuilt, lights stay where they are.
- Lights already placed in existing levels keep working.

## Today
`ALightFixtureActor` finds the nearest truss in range (`FindNearestTrussMount`) and attaches to it.
The truss re-derives the mount from its own settings (`BuildMountedFixtureDefinition`,
`GetFixtureMountTransformFromDefinition`: a fixed span name plus a left or right rail) and
`ApplyStoredMountTransform` re-applies it after every rebuild. The geometry is re-worked per build
mode in about six functions, and only Front, Back, Left and Right runs exist. Bay center runs and
new shapes cannot hold lights. Only the truss selection box has collision, so snapping cannot trace
the truss meshes.

## Approaches considered
1. Record every run while building, and mount by run id. Lights follow resizes, which Carl does not
   want. Needs stable ids across rebuilds.
2. Add more named spans. Keeps the per-shape geometry. Rejected.
3. Make each piece a component with sockets. Breaks the instanced-mesh setup. Rejected.
4. Free placement with snapping (chosen by Carl). The light snaps to the nearest rail of the
   nearest truss piece when placed and keeps its position relative to the truss.

## Design
- The truss gets a function that takes a world point and a sling type and returns the nearest rail
  point as a truss-local transform. It searches the piece instances the truss already holds
  (straight pieces and corner blocks), finds the nearest piece, and picks the nearest of its four
  rail lines (two sides, top and bottom), using the existing `RailInsetCm`. Over-slung picks a top
  rail, under-slung a bottom rail.
- Because it reads the built pieces, it works for every shape, bays included.
- The light stores that local transform and re-applies it on construction and after a truss
  rebuild. A rebuild no longer repositions it. Moving the truss moves it, since it stays attached.
- Lights saved with the old span and rail data keep today's behaviour until they are moved or
  re-snapped, so existing levels do not change.
- A "Re-snap Lights" button on the truss moves each attached light back to the nearest rail, for
  when a resize leaves one floating.

## Goals
- Hang a light from a bay center run, with the right over or under slung rail.
- Lights follow truss moves and rotations.
- Lights stay put on a resize.
- No change to lights already in levels.

## Non-goals
- Lights in the Majic Event Layout file (the file has no light type yet).
- Mounting on vertical legs and towers (horizontal pieces only for now).
- Mounting across more than one truss actor at once.
- Migrating old lights automatically.

## Tests
- A point near the center run of a 2x1 rectangle snaps to within a few centimetres of that run,
  not to the front or back run.
- The same point snapped with both sling types lands on a top rail and a bottom rail.
- After changing the truss length, the light's world position is unchanged.
- After moving and rotating the truss, the light's position relative to the truss is unchanged.
- A light with old span and rail data still lands where the old code put it.

## Left to the builder
- The light's orientation at the mount: match what `GetFixtureMountTransformFromDefinition` does
  today.
- The exact rail offsets from each piece's mesh bounds.
