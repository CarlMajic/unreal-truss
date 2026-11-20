#include "Misc/AutomationTest.h"

#include "DrapeRunActor.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "LightFixtureActor.h"
#include "MBPWallActor.h"
#include "MajicEventLayout.h"
#include "MajicEventLayoutActor.h"
#include "MajicLayoutStampComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "StageDeckActor.h"
#include "TrussStructureActor.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
const TCHAR* SampleLayout = TEXT(R"JSON({
  "format": "majic-event-layout",
  "version": 1,
  "event": { "name": "Test Event", "notes": "kept" },
  "items": [
    { "id": "stage-1", "type": "stage", "name": "Main Stage",
      "position": { "x_ft": 6, "y_ft": 10, "rotation_deg": 0 },
      "params": { "columns": 3, "rows": 2,
        "cells": [ { "row": 0, "col": 0, "height_in": 12 }, { "row": 0, "col": 1, "height_in": 12 },
                   { "row": 1, "col": 0, "height_in": 20 }, { "row": 1, "col": 1, "height_in": 20 }, { "row": 1, "col": 2, "height_in": 20 } ],
        "railings": { "front": true, "back": false, "left": true, "right": false },
        "steps": [ { "side": "right", "style": "stage_right" } ], "skirt": true } },
    { "id": "truss-1", "type": "truss", "name": "Rect",
      "position": { "x_ft": -20, "y_ft": 8, "rotation_deg": 90 },
      "params": { "shape": "rectangle", "length_ft": 20, "width_ft": 16, "height_ft": 10, "length_bays": 2, "width_bays": 1 } },
    { "id": "drape-1", "type": "drape", "name": "Back Drape",
      "position": { "x_ft": 0, "y_ft": 32, "rotation_deg": 0 },
      "params": { "kind": "regular", "length_ft": 40, "height_ft": 8, "fabric": true, "hardware": true } },
    { "id": "drape-ctl", "type": "drape", "name": "Control",
      "position": { "x_ft": 0, "y_ft": 0, "rotation_deg": 0 },
      "params": { "kind": "control", "panels": 3, "height_ft": 8 } },
    { "id": "mbp-1", "type": "mbp_wall", "name": "Wall",
      "position": { "x_ft": 10, "y_ft": 30, "rotation_deg": 180 },
      "params": { "version": 1, "width_ft": 6, "height_ft": 6, "support_spacing": "every_other", "left_cap": true, "right_cap": false,
        "cells": [ { "row": 0, "col": 0, "style": "geowall", "depth_in": 0 }, { "row": 0, "col": 1, "style": "acrylic", "depth_in": 12 } ] } },
    { "id": "lights-1", "type": "lighting_rig", "name": "Unknown", "position": { "x_ft": 1, "y_ft": 2, "rotation_deg": 3 }, "params": { "anything": 7 } }
  ]
})JSON");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMajicLayoutParseTest, "Majic.EventLayout.Parse", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMajicLayoutParseTest::RunTest(const FString& Parameters)
{
	FMajicLayoutDoc Doc;
	FString Error;
	TestTrue(TEXT("sample parses"), MajicEventLayout::Parse(SampleLayout, Doc, Error));
	TestEqual(TEXT("item count"), Doc.Items.Num(), 6);
	TestEqual(TEXT("stage x"), Doc.Items[0].XFt, 6.0);
	TestEqual(TEXT("truss rotation"), Doc.Items[1].RotationDeg, 90.0);
	TestEqual(TEXT("unknown type kept"), Doc.Items[5].Type, FString(TEXT("lighting_rig")));

	TestFalse(TEXT("wrong format rejected"), MajicEventLayout::Parse(TEXT(R"({"format":"other","version":1,"items":[]})"), Doc, Error));
	TestFalse(TEXT("later version rejected"), MajicEventLayout::Parse(TEXT(R"({"format":"majic-event-layout","version":2,"items":[]})"), Doc, Error));
	TestFalse(TEXT("duplicate id rejected"), MajicEventLayout::Parse(
		TEXT(R"({"format":"majic-event-layout","version":1,"items":[{"id":"a","type":"x","params":{}},{"id":"a","type":"x","params":{}}]})"), Doc, Error));
	TestFalse(TEXT("missing params rejected"), MajicEventLayout::Parse(
		TEXT(R"({"format":"majic-event-layout","version":1,"items":[{"id":"a","type":"x"}]})"), Doc, Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMajicLayoutMergeTest, "Majic.EventLayout.Merge", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMajicLayoutMergeTest::RunTest(const FString& Parameters)
{
	FMajicLayoutDoc Doc;
	FString Error;
	MajicEventLayout::Parse(SampleLayout, Doc, Error);

	FMajicLayoutItem NewStage;
	NewStage.Id = TEXT("stage-new");
	NewStage.Type = TEXT("stage");
	NewStage.Params = MakeShared<FJsonObject>();
	const TSet<FString> Replaced = { TEXT("stage"), TEXT("drape") };
	MajicEventLayout::MergeItems(Doc, { NewStage }, Replaced, [](const FMajicLayoutItem& Item) { return Item.Id == TEXT("drape-ctl"); });

	TArray<FString> Ids;
	for (const FMajicLayoutItem& Item : Doc.Items) { Ids.Add(Item.Id); }
	TestFalse(TEXT("old stage replaced"), Ids.Contains(TEXT("stage-1")));
	TestFalse(TEXT("old regular drape replaced"), Ids.Contains(TEXT("drape-1")));
	TestTrue(TEXT("control drape kept"), Ids.Contains(TEXT("drape-ctl")));
	TestTrue(TEXT("new stage added"), Ids.Contains(TEXT("stage-new")));
	TestTrue(TEXT("truss kept"), Ids.Contains(TEXT("truss-1")));
	TestTrue(TEXT("unknown type kept"), Ids.Contains(TEXT("lights-1")));

	FMajicLayoutDoc Again;
	TestTrue(TEXT("serialized file parses again"), MajicEventLayout::Parse(MajicEventLayout::Serialize(Doc), Again, Error));
	TestEqual(TEXT("round trip item count"), Again.Items.Num(), Doc.Items.Num());
	TestTrue(TEXT("event metadata kept"), Again.Event.IsValid() && Again.Event->GetStringField(TEXT("notes")) == TEXT("kept"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMajicLayoutPlacementTest, "Majic.EventLayout.Placement", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMajicLayoutPlacementTest::RunTest(const FString& Parameters)
{
	// Origin at x 10 ft, y 20 ft (away from the audience), not rotated: world X = 304.8, Y = -609.6,
	// and the actor is turned 180 degrees so its front (-Y) faces the audience at +Y.
	const FTransform Plain = MajicEventLayout::ActorTransformForOrigin(10.0, 20.0, 0.0, FVector::ZeroVector);
	TestTrue(TEXT("location"), Plain.GetLocation().Equals(FVector(304.8, -609.6, 0.0), 0.01));
	TestEqual(TEXT("yaw 180"), FMath::Abs(Plain.Rotator().Yaw), 180.0, 0.01);

	// A rotation of +90 (counter-clockwise from above) is a clockwise yaw of -90 from the base 180.
	const FTransform Turned = MajicEventLayout::ActorTransformForOrigin(0.0, 0.0, 90.0, FVector::ZeroVector);
	TestEqual(TEXT("yaw 90"), Turned.Rotator().Yaw, 90.0, 0.01);

	const FVector Corner(731.52, -60.96, 0.0);
	const double Cases[][3] = { {6.0, 10.0, 0.0}, {-20.0, 8.0, 90.0}, {0.0, 32.0, 180.0}, {12.5, -3.25, -45.0} };
	for (const double* Case : Cases)
	{
		const FTransform Transform = MajicEventLayout::ActorTransformForOrigin(Case[0], Case[1], Case[2], Corner);
		double X = 0.0, Y = 0.0, Rotation = 0.0;
		MajicEventLayout::ReadOriginFromTransform(Transform, Corner, X, Y, Rotation);
		TestEqual(TEXT("round trip x"), X, Case[0], 0.001);
		TestEqual(TEXT("round trip y"), Y, Case[1], 0.001);
		TestEqual(TEXT("round trip rotation"), Rotation, Case[2], 0.001);
	}
	return true;
}

// A rectangle with bays has a corner block at every grid crossing, like the Fusion rectangle.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMajicTrussRectangleBaysTest, "Majic.Truss.RectangleBays", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMajicTrussRectangleBaysTest::RunTest(const FString& Parameters)
{
	UWorld* World = nullptr;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (Context.WorldType == EWorldType::Editor && Context.World())
		{
			World = Context.World();
		}
	}
	if (!TestNotNull(TEXT("editor world"), World))
	{
		return false;
	}

	ATrussStructureActor* Truss = World->SpawnActor<ATrussStructureActor>();
	Truss->BuildMode = ETrussBuildMode::Rectangle;
	Truss->RectangleLengthFt = 40.0f;
	Truss->RectangleWidthFt = 16.0f;

	auto CountDistinct = [](UInstancedStaticMeshComponent* Ism, bool bX)
	{
		TArray<int32> Values;
		for (int32 I = 0; I < Ism->GetInstanceCount(); ++I)
		{
			FTransform Local;
			Ism->GetInstanceTransform(I, Local, false);
			Values.AddUnique(FMath::RoundToInt((bX ? Local.GetLocation().X : Local.GetLocation().Y) / 5.0));
		}
		return Values.Num();
	};

	Truss->RectangleLengthBays = 1;
	Truss->RectangleWidthBays = 1;
	Truss->BuildRectangle();
	TestEqual(TEXT("1x1 has 4 corners"), Truss->CornerBlockInstances->GetInstanceCount(), 4);
	const FBox Plain = Truss->SelectionBounds->Bounds.GetBox();

	Truss->RectangleLengthBays = 2;
	Truss->RectangleWidthBays = 1;
	Truss->BuildRectangle();
	TestEqual(TEXT("2x1 has 6 corners"), Truss->CornerBlockInstances->GetInstanceCount(), 6);
	TestEqual(TEXT("2x1 has 3 corner columns"), CountDistinct(Truss->CornerBlockInstances, true), 3);
	TestEqual(TEXT("2x1 has 2 corner rows"), CountDistinct(Truss->CornerBlockInstances, false), 2);
	const FBox Bayed = Truss->SelectionBounds->Bounds.GetBox();
	// Truss comes in fixed piece sizes, so each bay rounds to the nearest combination: the outer size
	// may move by a couple of centimetres, never by a piece.
	TestEqual(TEXT("bays keep the outer length"), Bayed.GetSize().X, Plain.GetSize().X, 15.0);
	TestEqual(TEXT("bays keep the outer width"), Bayed.GetSize().Y, Plain.GetSize().Y, 15.0);

	Truss->RectangleLengthBays = 3;
	Truss->RectangleWidthBays = 2;
	Truss->BuildRectangle();
	TestEqual(TEXT("3x2 has 12 corners"), Truss->CornerBlockInstances->GetInstanceCount(), 12);

	Truss->Destroy();
	return true;
}

// ---- Free light mounting ------------------------------------------------------------------

namespace
{
UWorld* EditorWorldForTests()
{
	UWorld* World = nullptr;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (Context.WorldType == EWorldType::Editor && Context.World())
		{
			World = Context.World();
		}
	}
	return World;
}

ATrussStructureActor* SpawnBayedTruss(UWorld* World)
{
	ATrussStructureActor* Truss = World->SpawnActor<ATrussStructureActor>();
	Truss->BuildMode = ETrussBuildMode::Rectangle;
	Truss->RectangleLengthFt = 40.0f;
	Truss->RectangleWidthFt = 16.0f;
	Truss->RectangleHeightFt = 10.0f;
	Truss->RectangleLengthBays = 2;
	Truss->RectangleWidthBays = 1;
	Truss->BuildRectangle();
	return Truss;
}
}

// A point at the middle of a 2x1 rectangle snaps to the center support run, not the front or back run.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMajicTrussRailMountTest, "Majic.Truss.RailMount", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMajicTrussRailMountTest::RunTest(const FString& Parameters)
{
	UWorld* World = EditorWorldForTests();
	if (!TestNotNull(TEXT("editor world"), World))
	{
		return false;
	}
	ATrussStructureActor* Truss = SpawnBayedTruss(World);
	const FBox Bounds = Truss->SelectionBounds->Bounds.GetBox();
	const FVector Middle = Bounds.GetCenter();

	FTransform Top;
	TestTrue(TEXT("over slung snap found"), Truss->FindNearestRailMount(FVector(Middle.X, Middle.Y, Bounds.Max.Z), ETrussSlingType::OverSlung, Top));
	// The center run lies across the width, at the middle of the length: the mount is on it, a long way
	// (about 8 ft) from the front and back runs and from either end.
	TestEqual(TEXT("mount x is on the center run"), Top.GetLocation().X, Middle.X, 60.0);

	FTransform Bottom;
	TestTrue(TEXT("under slung snap found"), Truss->FindNearestRailMount(FVector(Middle.X, Middle.Y, Bounds.Min.Z), ETrussSlingType::UnderSlung, Bottom));
	TestTrue(TEXT("under slung rail is lower than over slung"), Bottom.GetLocation().Z < Top.GetLocation().Z - 15.0);
	TestTrue(TEXT("under slung light is turned over"), Bottom.GetUnitAxis(EAxis::Z).Z < -0.5);
	TestTrue(TEXT("over slung light is upright"), Top.GetUnitAxis(EAxis::Z).Z > 0.5);

	// A point at the far end snaps near that end, so the snap follows the point and not a fixed span.
	FTransform End;
	TestTrue(TEXT("end snap found"), Truss->FindNearestRailMount(FVector(Bounds.Min.X + 20.0, Middle.Y, Bounds.Max.Z), ETrussSlingType::OverSlung, End));
	TestTrue(TEXT("end snap is near the end, not the center"), End.GetLocation().X < Middle.X - 300.0);

	Truss->Destroy();
	return true;
}

// A light keeps its place when the truss is resized, and moves with the truss when it is moved or turned.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMajicLightFreeMountTest, "Majic.Truss.LightFreeMount", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMajicLightFreeMountTest::RunTest(const FString& Parameters)
{
	UWorld* World = EditorWorldForTests();
	if (!TestNotNull(TEXT("editor world"), World))
	{
		return false;
	}
	ATrussStructureActor* Truss = SpawnBayedTruss(World);
	const FBox Bounds = Truss->SelectionBounds->Bounds.GetBox();
	const FVector Point(Bounds.GetCenter().X, Bounds.GetCenter().Y, Bounds.Max.Z);

	ALightFixtureActor* Light = World->SpawnActor<ALightFixtureActor>();
	TestTrue(TEXT("attach succeeds"), Light->AttachToTrussAtWorldLocation(Truss, Point));
	TestTrue(TEXT("attach is a free mount"), Light->bFreeMount);
	const FVector Placed = Light->GetActorLocation();
	const FVector LocalPlaced = Truss->GetActorTransform().InverseTransformPosition(Placed);

	// Resize: the light stays where it was.
	Truss->RectangleLengthFt = 60.0f;
	Truss->BuildRectangle();
	Light->ApplyStoredMountTransform();
	TestTrue(TEXT("light stays put when the truss is resized"), Light->GetActorLocation().Equals(Placed, 0.5));

	// Move and turn: the light keeps its place relative to the truss.
	Truss->SetActorTransform(FTransform(FRotator(0.0, 90.0, 0.0), FVector(500.0, 200.0, 0.0)));
	Light->ApplyStoredMountTransform();
	const FVector Expected = Truss->GetActorTransform().TransformPosition(LocalPlaced);
	TestTrue(TEXT("light moves with the truss"), Light->GetActorLocation().Equals(Expected, 0.5));

	Light->Destroy();
	Truss->Destroy();
	return true;
}

// A light saved with the old span and rail data still lands where the old code put it.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMajicLightLegacyMountTest, "Majic.Truss.LightLegacyMount", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMajicLightLegacyMountTest::RunTest(const FString& Parameters)
{
	UWorld* World = EditorWorldForTests();
	if (!TestNotNull(TEXT("editor world"), World))
	{
		return false;
	}
	ATrussStructureActor* Truss = World->SpawnActor<ATrussStructureActor>();
	Truss->BuildMode = ETrussBuildMode::Rectangle;
	Truss->BuildRectangle();
	const FBox Bounds = Truss->SelectionBounds->Bounds.GetBox();

	FMountedFixtureDefinition Definition;
	TestTrue(TEXT("legacy definition built"), Truss->BuildMountedFixtureDefinition(nullptr, ETrussSlingType::OverSlung, FVector(Bounds.GetCenter().X, Bounds.Min.Y, Bounds.Max.Z), Definition));
	FTransform Expected;
	TestTrue(TEXT("legacy transform found"), Truss->GetFixtureMountTransformFromDefinition(Definition, Expected));

	ALightFixtureActor* Light = World->SpawnActor<ALightFixtureActor>();
	Light->AttachedTruss = Truss;
	Light->MountDefinition = Definition;
	Light->bAttachedToTruss = true;
	Light->bFreeMount = false;
	TestTrue(TEXT("legacy apply succeeds"), Light->ApplyStoredMountTransform());
	TestTrue(TEXT("legacy light lands where the old code put it"), Light->GetActorLocation().Equals(Expected.GetLocation(), 0.5));

	Light->Destroy();
	Truss->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMajicLayoutActorsTest, "Majic.EventLayout.Actors", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMajicLayoutActorsTest::RunTest(const FString& Parameters)
{
	UWorld* World = nullptr;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (Context.WorldType == EWorldType::Editor && Context.World())
		{
			World = Context.World();
		}
	}
	if (!TestNotNull(TEXT("editor world"), World))
	{
		return false;
	}
	TArray<AActor*> Spawned;

	const FString InPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("MajicLayoutTest"), TEXT("in.json"));
	const FString OutPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("MajicLayoutTest"), TEXT("out.json"));
	FFileHelper::SaveStringToFile(SampleLayout, *InPath);

	AMajicEventLayoutActor* Layout = World->SpawnActor<AMajicEventLayoutActor>();
	Spawned.Add(Layout);
	TArray<FString> Lines;
	TestTrue(TEXT("import succeeds"), Layout->ImportLayoutFile(InPath, Lines));
	for (const FString& Line : Lines) { AddInfo(Line); }

	int32 Stages = 0, Trusses = 0, Drapes = 0, Walls = 0;
	for (TActorIterator<AStageDeckActor> It(World); It; ++It) { ++Stages; }
	for (TActorIterator<ATrussStructureActor> It(World); It; ++It) { ++Trusses; }
	for (TActorIterator<ADrapeRunActor> It(World); It; ++It) { ++Drapes; }
	for (TActorIterator<AMBPWallActor> It(World); It; ++It) { ++Walls; }
	TestEqual(TEXT("stages built"), Stages, 1);
	TestEqual(TEXT("trusses built"), Trusses, 1);
	TestEqual(TEXT("drapes built (control skipped)"), Drapes, 1);
	TestEqual(TEXT("mbp walls built"), Walls, 1);

	// The stage's front-left corner must land at (6 ft, 10 ft).
	for (TActorIterator<AStageDeckActor> It(World); It; ++It)
	{
		const FVector Corner(((It->Columns - 1) * It->CellWidthCm) + (0.5f * It->CellWidthCm), -0.5f * It->CellDepthCm, 0.0);
		double X = 0.0, Y = 0.0, Rotation = 0.0;
		MajicEventLayout::ReadOriginFromTransform(It->GetActorTransform(), Corner, X, Y, Rotation);
		TestEqual(TEXT("stage x"), X, 6.0, 0.01);
		TestEqual(TEXT("stage y"), Y, 10.0, 0.01);
	}


	// The stage's deck cells must sit exactly on the 8 ft x 4 ft grid, front row (file row 1) at
	// y 10..14 ft and the back row at y 14..18 ft, with house left at the lower x.
	for (TActorIterator<AStageDeckActor> It(World); It; ++It)
	{
		TArray<FVector2D> Centers;
		TArray<UInstancedStaticMeshComponent*> Isms;
		It->GetComponents<UInstancedStaticMeshComponent>(Isms);
		for (UInstancedStaticMeshComponent* Ism : Isms)
		{
			for (int32 I = 0; I < Ism->GetInstanceCount(); ++I)
			{
				FTransform World_;
				Ism->GetInstanceTransform(I, World_, true);
				Centers.Add(FVector2D(World_.GetLocation().X / 30.48, -World_.GetLocation().Y / 30.48));
			}
		}
		const FVector2D Expected[] = { {10.0, 12.0}, {18.0, 12.0}, {26.0, 12.0}, {10.0, 16.0}, {18.0, 16.0} };
		for (const FVector2D& Cell : Expected)
		{
			TestTrue(*FString::Printf(TEXT("deck at (%g, %g) ft"), Cell.X, Cell.Y),
				Centers.ContainsByPredicate([&Cell](const FVector2D& Center) { return FVector2D::Distance(Center, Cell) < 0.1; }));
		}
		TestFalse(TEXT("no deck where the file left a cell empty"),
			Centers.ContainsByPredicate([](const FVector2D& Center) { return FVector2D::Distance(Center, FVector2D(26.0, 16.0)) < 0.1; }));
	}

	// Where the built geometry really sits, in file coordinates (x right, y away from the audience),
	// as an independent check of each actor's origin point. Pieces overhang by up to a foot or so.
	auto ExpectFootprint = [this](UBoxComponent* Selection, const TCHAR* What, double MinX, double MaxX, double MinY, double MaxY)
	{
		const FBox Box = Selection->Bounds.GetBox();
		const double Tolerance = 2.5;
		TestEqual(*FString::Printf(TEXT("%s min x"), What), Box.Min.X / 30.48, MinX, Tolerance);
		TestEqual(*FString::Printf(TEXT("%s max x"), What), Box.Max.X / 30.48, MaxX, Tolerance);
		TestEqual(*FString::Printf(TEXT("%s min y"), What), -Box.Max.Y / 30.48, MinY, Tolerance);
		TestEqual(*FString::Printf(TEXT("%s max y"), What), -Box.Min.Y / 30.48, MaxY, Tolerance);
	};
		for (TActorIterator<ATrussStructureActor> It(World); It; ++It) { ExpectFootprint(It->SelectionBounds, TEXT("truss"), -36.0, -20.0, 8.0, 28.0); }
	for (TActorIterator<ATrussStructureActor> It(World); It; ++It)
	{
		TestEqual(TEXT("truss length bays imported"), It->RectangleLengthBays, 2);
		TestEqual(TEXT("truss width bays imported"), It->RectangleWidthBays, 1);
	}
	for (TActorIterator<ADrapeRunActor> It(World); It; ++It) { ExpectFootprint(It->SelectionBounds, TEXT("drape"), 0.0, 40.0, 31.0, 33.0); }
	for (TActorIterator<AMBPWallActor> It(World); It; ++It) { ExpectFootprint(It->SelectionBounds, TEXT("mbp wall"), 4.0, 10.0, 28.0, 32.0); }

	// Export into a copy of the input: every built item comes back with its id, and the item types
	// Unreal does not build (the unknown rig, the control drape) are still in the file.
	FFileHelper::SaveStringToFile(SampleLayout, *OutPath);
	Lines.Reset();
	TestTrue(TEXT("export succeeds"), Layout->ExportLayoutFile(OutPath, Lines));
	for (const FString& Line : Lines) { AddInfo(Line); }

	FMajicLayoutDoc Out;
	FString Error;
	TestTrue(TEXT("exported file parses"), MajicEventLayout::LoadFile(OutPath, Out, Error));
	TArray<FString> Ids;
	for (const FMajicLayoutItem& Item : Out.Items) { Ids.Add(Item.Id); }
	for (const TCHAR* Id : { TEXT("stage-1"), TEXT("truss-1"), TEXT("drape-1"), TEXT("drape-ctl"), TEXT("mbp-1"), TEXT("lights-1") })
	{
		TestTrue(FString::Printf(TEXT("%s kept"), Id), Ids.Contains(Id));
	}
	for (const FMajicLayoutItem& Item : Out.Items)
	{
		if (Item.Id == TEXT("stage-1") || Item.Id == TEXT("truss-1") || Item.Id == TEXT("drape-1") || Item.Id == TEXT("mbp-1"))
		{
			const FMajicLayoutItem* Original = nullptr;
			FMajicLayoutDoc In;
			MajicEventLayout::Parse(SampleLayout, In, Error);
			for (const FMajicLayoutItem& Candidate : In.Items) { if (Candidate.Id == Item.Id) { Original = &Candidate; } }
			if (Item.Id == TEXT("truss-1"))
			{
				int32 LengthBays = 0, WidthBays = 0;
				Item.Params->TryGetNumberField(TEXT("length_bays"), LengthBays);
				Item.Params->TryGetNumberField(TEXT("width_bays"), WidthBays);
				TestEqual(TEXT("truss length bays exported"), LengthBays, 2);
				TestEqual(TEXT("truss width bays exported"), WidthBays, 1);
			}
			if (Original)
			{
				TestEqual(*FString::Printf(TEXT("%s x"), *Item.Id), Item.XFt, Original->XFt, 0.01);
				TestEqual(*FString::Printf(TEXT("%s y"), *Item.Id), Item.YFt, Original->YFt, 0.01);
				TestEqual(*FString::Printf(TEXT("%s rotation"), *Item.Id), Item.RotationDeg, Original->RotationDeg, 0.01);
			}
		}
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->FindComponentByClass<UMajicLayoutStampComponent>())
		{
			Spawned.Add(*It);
		}
	}
	for (AActor* Actor : Spawned)
	{
		Actor->Destroy();
	}
	return true;
}

#endif
