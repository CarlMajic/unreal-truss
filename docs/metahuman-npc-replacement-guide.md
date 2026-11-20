# MetaHuman NPC Replacement Guide

Goal: replace the mannequin event guests with clothed MetaHuman-style characters, one step at a time, while keeping the current social group and seating behavior stable.

This guide is meant to be followed slowly. Stop after each step, test, and ask questions before moving on.

## Current Project State

- `AEventNPCActor` supports a random `VisualVariants` pool.
- Social group, cafe, bar, lounge, and sectional actors now provide preview-matched smart object slots.
- Most guests should stay in their assigned social/seating slots instead of wandering.
- The next work is content setup: create/import MetaHumans, assemble them, then add them to `VisualVariants`.

## Important Constraint

We cannot generate brand-new MetaHuman identities at runtime. MetaHuman creation is an Unreal Editor content workflow. The runtime part is random selection from a pool of prepared MetaHuman variants.

For our project, the target pipeline is:

1. Create a small set of MetaHuman characters in Unreal.
2. Give each one clothing and hair.
3. Assemble each MetaHuman into usable Unreal assets.
4. Make one variant work on `AEventNPCActor`.
5. Add more variants after the first one is proven.

## Step 1 - Verify MetaHuman Setup

Do this first.

1. Open the project in Unreal Engine 5.6.
2. Check that the MetaHuman Creator plugin is enabled.
3. Check that MetaHuman Creator Core Data is installed with the engine.
4. Restart the editor if Unreal asks you to.

Expected result:

- In the Content Browser, right-clicking should show a MetaHuman option, including `MetaHuman Character`.

Stop here if the MetaHuman menu is missing.

## Step 2 - Create One Test MetaHuman

Use only one character first.

1. In the Content Browser, create a folder such as:
   - `/Game/EventAI/MetaHumans/TestGuests`
2. Right-click in that folder.
3. Choose `MetaHuman > MetaHuman Character`.
4. Name it something clear, for example:
   - `MH_Guest_Test_01`
5. Open the asset.
6. Pick a preset or make small face/body changes.

Expected result:

- You have one editable MetaHuman Character asset.

Stop here if the asset does not open in MetaHuman Creator.

## Step 3 - Add Clothing And Hair

The character needs clothing before assembly.

1. In MetaHuman Creator, go to the Hair & Clothing controls.
2. Pick a hair/groom option.
3. Pick a clothing/outfit option.
4. Avoid complex clothing for the first test.
5. Save the character.

Expected result:

- The character looks clothed in MetaHuman Creator.

Stop here if the wardrobe is empty or clothing does not display.

## Step 4 - Rig The Character

The character must be rigged before assembly.

1. Use the MetaHuman Creator rigging option.
2. For a first test, choose the lighter practical option unless we specifically need facial fidelity.
3. Wait for rigging to finish.

Expected result:

- The character can be previewed/animated and is ready for assembly.

Stop here if rigging fails or the character remains unrigged.

## Step 5 - Download Textures

Assembly requires downloaded high-resolution textures.

1. Download textures from the MetaHuman Creator interface.
2. Use 2K for the first test to keep iteration fast.
3. Save after download completes.

Expected result:

- The character has real textures available for assembly.

Stop here if texture download fails.

## Step 6 - Assemble The MetaHuman

For our event simulation, start with an optimized assembly if available. Do not start with the most expensive cinematic version unless the optimized path fails.

1. Open the assembly controls.
2. Choose an Unreal assembly pipeline.
3. Prefer a medium/optimized quality for crowd use.
4. Assemble the character into the project.
5. Note the output folder and generated Blueprint/assets.

Expected result:

- The project now has assembled MetaHuman assets, usually including a Blueprint and skeletal mesh components/assets.

Stop here and tell me the generated asset names and folder paths.

## Step 7 - Place The Assembled MetaHuman In The Level

Before wiring into NPCs, verify the MetaHuman itself works.

1. Drag the assembled MetaHuman Blueprint into an empty test area.
2. Press Play or Simulate.
3. Confirm the character appears clothed.
4. Confirm there are no missing material/groom errors.

Expected result:

- One clothed MetaHuman stands in the level.

Stop here if it appears bald, unclothed, invisible, or broken.

## Step 8 - Identify The Runtime Mesh Strategy

This is the decision point.

Option A: use a single assembled skeletal mesh in `VisualVariants`.

- Best if the assembled MetaHuman provides one suitable body/combined mesh.
- Fastest path for our current `AEventNPCActor` code.

Option B: spawn or attach the full MetaHuman Blueprint.

- Better if clothing/hair are separate components.
- More likely for real MetaHumans.
- Requires a new code pass so `AEventNPCActor` can use a visual child actor instead of only `GetMesh()`.

Expected result:

- We decide whether the current `VisualVariants` system is enough or whether we need Blueprint visual variants.

Stop here and ask before changing code.

## Step 9 - Add One Variant To EventNPCActor

Only do this after Step 8 is clear.

1. Open the NPC Blueprint or placed NPC defaults.
2. Enable `bUseRandomVisualVariants`.
3. Add one entry to `VisualVariants`.
4. Set the skeletal mesh or future Blueprint variant fields.
5. Leave animations blank at first so the project falls back to existing mannequin animations.
6. Test one social group with one or two NPCs.

Expected result:

- At least one event NPC uses the MetaHuman visual.

Stop here if the body appears but animations are wrong.

## Step 10 - Retarget Animations

Once the visual appears, fix animation compatibility.

1. Use MetaHuman retargeting assets if available.
2. Retarget our idle/talk/sit animations to the MetaHuman skeleton.
3. Assign those retargeted animations to the variant.
4. Test standing talk first.
5. Test seated animation second.

Expected result:

- A MetaHuman can stand/talk in a social group and stay aligned to the preview slot.

Stop here before doing many characters.

## Step 11 - Build A Small Crowd Pool

After one character works:

1. Create 3-5 more MetaHumans.
2. Use varied clothing/hair/body presets.
3. Assemble each one.
4. Add each as a variant.
5. Test with one social group actor.

Expected result:

- The group has varied human-looking characters.

## New Chat Handoff

Paste this into a new chat:

```text
We are working in C:\Users\Carl\unreal-truss on an Unreal Engine 5.6 event simulation.

Goal: replace mannequin NPCs with clothed MetaHuman-style characters.

Current code state:
- AEventNPCActor supports bUseRandomVisualVariants and a VisualVariants array.
- Each FEventNPCVisualVariant can define skeletal mesh, anim class, idle/walk/sit/bar/cafe/talk/stage animations, and base mesh transform.
- SocialGroupAIActor exists and creates circle smart-object slots with per-slot standing animations.
- Guests currently stay in social/seating slots rather than wandering.

We are following docs/metahuman-npc-replacement-guide.md one step at a time.
Do not jump ahead. Help me complete only the current step, then stop and wait.
```

## References

- MetaHuman Creator in Unreal Engine: https://dev.epicgames.com/documentation/en-us/metahuman/metahuman-creator
- Creating a MetaHuman Character: https://dev.epicgames.com/documentation/en-us/metahuman/creating-a-character
- MetaHuman Assembly: https://dev.epicgames.com/documentation/metahuman/assembly
- Hair and Clothing Controls: https://dev.epicgames.com/documentation/en-us/metahuman/hair-and-clothing-controls
