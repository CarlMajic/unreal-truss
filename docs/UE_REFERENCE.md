# Unreal Engine References

## Official Docs Used

- Unreal Engine modules: https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-modules
- Programming with C++: https://dev.epicgames.com/documentation/en-us/unreal-engine/programming-with-cplusplus-in-unreal-engine
- Rendering components and Instanced Static Mesh overview: https://dev.epicgames.com/documentation/en-us/unreal-engine/rendering-components-in-unreal-engine?application_version=5.6
- Instanced Static Mesh Component: https://dev.epicgames.com/documentation/es-mx/unreal-engine/instanced-static-mesh-component-in-unreal-engine
- Editor Utility Widget Python API: https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/EditorUtilityWidget

## Relevant Tooling Patterns

- `UInstancedStaticMeshComponent` is the correct first output path for repeated truss pieces. Unreal's docs describe it as a lower-cost way to render many copies of the same Static Mesh from one component instead of many separate actors.
- Runtime logic should live in a runtime module so editor tools and future in-game UI can call the same API.
- Editor Utility Widgets remain a good later UI layer, but they should not own the truss solver or placement logic.

## Comparable Tools Found

- ProInstance Tools: procedural mesh/actor placement with ISM/HISM output and bake workflows.
- UE4 PHISM Tools: older open-source procedural HISM examples for reducing draw calls.
- Procedural building generators: use a single Actor plus deterministic generation settings, which matches the desired truss-structure workflow.

## Current Decision

Start with a runtime C++ actor, `ATrussStructureActor`, that builds a straight run using real inventory lengths. Add editor UI after the runtime API is stable.

