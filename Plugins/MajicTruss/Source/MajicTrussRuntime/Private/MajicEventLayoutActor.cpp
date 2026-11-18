#include "MajicEventLayoutActor.h"

#include "DrapeRunActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "MBPWallActor.h"
#include "MajicEventLayout.h"
#include "MajicLayoutStampComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "StageDeckActor.h"
#include "TrussStructureActor.h"

#if WITH_EDITOR
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#endif

namespace
{
using FObj = TSharedPtr<FJsonObject>;

constexpr double CmPerFoot = 30.48;
constexpr float MaxDrapeHeightFt = 18.333f;

const TCHAR* TypeStage = TEXT("stage");
const TCHAR* TypeTruss = TEXT("truss");
const TCHAR* TypeDrape = TEXT("drape");
const TCHAR* TypeMbp = TEXT("mbp_wall");

// ---- JSON helpers ----------------------------------------------------------------------

double Num(const FObj& Object, const TCHAR* Key, double Default)
{
	double Value = Default;
	if (Object.IsValid())
	{
		Object->TryGetNumberField(Key, Value);
	}
	return Value;
}

bool Flag(const FObj& Object, const TCHAR* Key, bool Default)
{
	bool Value = Default;
	if (Object.IsValid())
	{
		Object->TryGetBoolField(Key, Value);
	}
	return Value;
}

FString Str(const FObj& Object, const TCHAR* Key, const FString& Default)
{
	FString Value = Default;
	if (Object.IsValid())
	{
		Object->TryGetStringField(Key, Value);
	}
	return Value;
}

FObj Child(const FObj& Object, const TCHAR* Key)
{
	const TSharedPtr<FJsonObject>* Value = nullptr;
	if (Object.IsValid() && Object->TryGetObjectField(Key, Value))
	{
		return *Value;
	}
	return nullptr;
}

TArray<FObj> ObjectList(const FObj& Object, const TCHAR* Key)
{
	TArray<FObj> Out;
	const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
	if (Object.IsValid() && Object->TryGetArrayField(Key, Values))
	{
		for (const TSharedPtr<FJsonValue>& Value : *Values)
		{
			if (Value.IsValid() && Value->AsObject().IsValid())
			{
				Out.Add(Value->AsObject());
			}
		}
	}
	return Out;
}

void CopyFields(const FObj& From, const FObj& To)
{
	if (From.IsValid())
	{
		for (const auto& Pair : From->Values)
		{
			To->SetField(FString(Pair.Key), Pair.Value);
		}
	}
}

double Round4(double Value)
{
	return FMath::RoundToDouble(Value * 10000.0) / 10000.0 + 0.0;
}

FString ToText(const FObj& Object)
{
	FString Text;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Text);
	FJsonSerializer::Serialize(Object.ToSharedRef(), Writer);
	return Text;
}

FObj FromText(const FString& Text)
{
	FObj Object;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
	if (!FJsonSerializer::Deserialize(Reader, Object) || !Object.IsValid())
	{
		return MakeShared<FJsonObject>();
	}
	return Object;
}

// ---- Stamp -----------------------------------------------------------------------------

UMajicLayoutStampComponent* FindStamp(AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<UMajicLayoutStampComponent>() : nullptr;
}

void StampActor(AActor* Actor, const FString& Id, const FString& Name, const FObj& Params)
{
	UMajicLayoutStampComponent* Stamp = FindStamp(Actor);
	if (!Stamp)
	{
		Stamp = NewObject<UMajicLayoutStampComponent>(Actor, TEXT("MajicLayoutStamp"));
		Actor->AddInstanceComponent(Stamp);
		Stamp->RegisterComponent();
	}
	Stamp->Modify();
	Stamp->ItemId = Id;
	Stamp->ItemName = Name;
	Stamp->ParamsJson = ToText(Params.IsValid() ? Params : MakeShared<FJsonObject>());
}

FString NewId(const TCHAR* Prefix, TSet<FString>& Used)
{
	for (;;)
	{
		const FString Candidate = FString::Printf(TEXT("%s-%s"), Prefix, *FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(4).ToLower());
		if (!Used.Contains(Candidate))
		{
			Used.Add(Candidate);
			return Candidate;
		}
	}
}

FString ActorName(AActor* Actor, const FString& Fallback)
{
#if WITH_EDITOR
	return Actor->GetActorLabel();
#else
	return Fallback;
#endif
}

void SetActorName(AActor* Actor, const FString& Name)
{
#if WITH_EDITOR
	Actor->SetActorLabel(Name);
#endif
}

// ---- Origin points (in each actor's own frame, cm) ---------------------------------------
// The file's origin is the front-left corner of the footprint; the actors face -Y and the
// world turns them 180 degrees, so that corner is the actor's +X, -Y corner.

FVector StageOrigin(const AStageDeckActor& Actor)
{
	const float StepX = Actor.CellWidthCm + Actor.HorizontalSpacingCm;
	const float StepY = Actor.CellDepthCm + Actor.VerticalSpacingCm;
	const float OriginX = Actor.bCenterOnActor ? (-0.5f * (Actor.Columns - 1) * StepX) : 0.0f;
	const float OriginY = Actor.bCenterOnActor ? (-0.5f * (Actor.Rows - 1) * StepY) : 0.0f;
	return FVector(OriginX + ((Actor.Columns - 1) * StepX) + (0.5f * Actor.CellWidthCm), OriginY - (0.5f * Actor.CellDepthCm), 0.0);
}

FVector BoundsOrigin(AActor& Actor, bool bCenter)
{
	const FBox Box = Actor.CalculateComponentsBoundingBoxInLocalSpace(true);
	if (!Box.IsValid)
	{
		return FVector::ZeroVector;
	}
	if (bCenter)
	{
		return FVector(0.5 * (Box.Min.X + Box.Max.X), 0.5 * (Box.Min.Y + Box.Max.Y), 0.0);
	}
	return FVector(Box.Max.X, Box.Min.Y, 0.0);
}

FVector TrussOrigin(ATrussStructureActor& Actor)
{
	return BoundsOrigin(Actor, Actor.BuildMode == ETrussBuildMode::Tower);
}

FVector DrapeOrigin(const ADrapeRunActor& Actor)
{
	const double RunLengthCm = static_cast<double>(Actor.CurrentSectionCount) * Actor.CurrentSectionLengthFt * CmPerFoot;
	return FVector(Actor.bCenterOnActor ? 0.5 * RunLengthCm : RunLengthCm, 0.0, 0.0);
}

FVector MbpOrigin(AMBPWallActor& Actor)
{
	const FBox Box = Actor.CalculateComponentsBoundingBoxInLocalSpace(true);
	return FVector(Box.IsValid ? Box.Max.X : 0.0, 0.0, 0.0);
}

// ---- Stage -----------------------------------------------------------------------------

bool HeightToPreset(double HeightIn, EStageDeckHeightPreset& OutPreset)
{
	if (FMath::IsNearlyEqual(HeightIn, 8.0)) { OutPreset = EStageDeckHeightPreset::In8; return true; }
	if (FMath::IsNearlyEqual(HeightIn, 12.0)) { OutPreset = EStageDeckHeightPreset::In12; return true; }
	// The inventory deck is 20 in; Unreal's preset for it is labelled 24 in.
	if (FMath::IsNearlyEqual(HeightIn, 20.0) || FMath::IsNearlyEqual(HeightIn, 24.0)) { OutPreset = EStageDeckHeightPreset::In24; return true; }
	if (FMath::IsNearlyEqual(HeightIn, 27.5) || FMath::IsNearlyEqual(HeightIn, 27.0)) { OutPreset = EStageDeckHeightPreset::In27; return true; }
	return false;
}

double PresetToHeight(EStageDeckHeightPreset Preset)
{
	switch (Preset)
	{
	case EStageDeckHeightPreset::In8: return 8.0;
	case EStageDeckHeightPreset::In12: return 12.0;
	case EStageDeckHeightPreset::In24: return 20.0;
	default: return 27.5;
	}
}

// The file's rows run back to front and its columns left to right as seen from the audience.
// The stage actor's row 0 is its front, and its column 0 ends up house right after the 180 degree
// turn, so both indices are flipped, and so are the left and right railings and steps.
bool ApplyStage(AStageDeckActor& Actor, const FObj& Params, TArray<FString>& Notes, FString& OutError)
{
	const int32 Columns = FMath::RoundToInt(Num(Params, TEXT("columns"), 0.0));
	const int32 Rows = FMath::RoundToInt(Num(Params, TEXT("rows"), 0.0));
	if (Columns < 1 || Rows < 1)
	{
		OutError = TEXT("A stage needs columns and rows of at least 1.");
		return false;
	}

	const bool bGrayCarpet = Str(Params, TEXT("surface"), TEXT("black_top")) == TEXT("gray_carpet");
	const FObj Railings = Child(Params, TEXT("railings"));
	const bool bSkirt = Flag(Params, TEXT("skirt"), false);

	FStageDeckBuildDefinition Definition;
	Definition.Columns = Columns;
	Definition.Rows = Rows;
	Definition.DefaultSurfaceStyle = bGrayCarpet ? EStageDeckSurfaceStyle::GrayCarpet : EStageDeckSurfaceStyle::BlackTop;
	Definition.bEnableFrontRailing = Flag(Railings, TEXT("front"), false);
	Definition.bEnableBackRailing = Flag(Railings, TEXT("back"), false);
	Definition.bEnableLeftRailing = Flag(Railings, TEXT("right"), false);
	Definition.bEnableRightRailing = Flag(Railings, TEXT("left"), false);
	Definition.bEnableAutomaticSkirt = bSkirt;

	bool bStepNoted = false;
	for (const FObj& Step : ObjectList(Params, TEXT("steps")))
	{
		const FString Side = Str(Step, TEXT("side"), TEXT(""));
		if (Side == TEXT("right")) { Definition.bEnableLeftStep = true; }
		else if (Side == TEXT("left")) { Definition.bEnableRightStep = true; }
		if (!bStepNoted && Str(Step, TEXT("style"), TEXT("stage_right")) == TEXT("black_carpet"))
		{
			Notes.Add(TEXT("Unreal has only the Stage Right steps; the black carpet style is not built."));
			bStepNoted = true;
		}
	}

	Actor.ApplyBuildDefinition(Definition, false);
	if (bSkirt)
	{
		Actor.bUseDebugSingleSkirt = false;
	}

	FStageDeckCell Off;
	Off.bEnabled = false;
	for (int32 Row = 0; Row < Rows; ++Row)
	{
		for (int32 Column = 0; Column < Columns; ++Column)
		{
			Actor.ApplyCellDefinition(Row, Column, Off, false);
		}
	}

	bool b20Noted = false;
	for (const FObj& Cell : ObjectList(Params, TEXT("cells")))
	{
		const int32 FileRow = FMath::RoundToInt(Num(Cell, TEXT("row"), -1.0));
		const int32 FileColumn = FMath::RoundToInt(Num(Cell, TEXT("col"), -1.0));
		const double HeightIn = Num(Cell, TEXT("height_in"), 0.0);
		FStageDeckCell Out;
		Out.bEnabled = true;
		Out.SurfaceStyle = Definition.DefaultSurfaceStyle;
		if (!HeightToPreset(HeightIn, Out.HeightPreset))
		{
			OutError = FString::Printf(TEXT("Unsupported deck height %g in."), HeightIn);
			return false;
		}
		if (FMath::IsNearlyEqual(HeightIn, 20.0) && !b20Noted)
		{
			Notes.Add(TEXT("20 in decks use Unreal's 24 in preset."));
			b20Noted = true;
		}
		Actor.ApplyCellDefinition(Rows - 1 - FileRow, Columns - 1 - FileColumn, Out, false);
	}

	Actor.RebuildStage();
	return true;
}

FObj ReadStage(const AStageDeckActor& Actor, const FObj& Stamped)
{
	const FObj Out = MakeShared<FJsonObject>();
	CopyFields(Stamped, Out);

	Out->SetNumberField(TEXT("columns"), Actor.Columns);
	Out->SetNumberField(TEXT("rows"), Actor.Rows);

	TArray<TSharedPtr<FJsonValue>> Cells;
	for (int32 FileRow = 0; FileRow < Actor.Rows; ++FileRow)
	{
		for (int32 FileColumn = 0; FileColumn < Actor.Columns; ++FileColumn)
		{
			FStageDeckCell Cell;
			if (!Actor.GetCellDefinition(Actor.Rows - 1 - FileRow, Actor.Columns - 1 - FileColumn, Cell) || !Cell.bEnabled)
			{
				continue;
			}
			const FObj Json = MakeShared<FJsonObject>();
			Json->SetNumberField(TEXT("row"), FileRow);
			Json->SetNumberField(TEXT("col"), FileColumn);
			Json->SetNumberField(TEXT("height_in"), PresetToHeight(Cell.HeightPreset));
			Cells.Add(MakeShared<FJsonValueObject>(Json));
		}
	}
	Out->SetArrayField(TEXT("cells"), Cells);

	Out->SetStringField(TEXT("surface"), Actor.DefaultSurfaceStyle == EStageDeckSurfaceStyle::GrayCarpet ? TEXT("gray_carpet") : TEXT("black_top"));

	const FObj Railings = MakeShared<FJsonObject>();
	Railings->SetBoolField(TEXT("front"), Actor.bEnableFrontRailing);
	Railings->SetBoolField(TEXT("back"), Actor.bEnableBackRailing);
	Railings->SetBoolField(TEXT("left"), Actor.bEnableRightRailing);
	Railings->SetBoolField(TEXT("right"), Actor.bEnableLeftRailing);
	Out->SetObjectField(TEXT("railings"), Railings);

	// Keep the step style the file had for a side; Unreal itself has only one style.
	TMap<FString, FString> StyleBySide;
	for (const FObj& Step : ObjectList(Stamped, TEXT("steps")))
	{
		StyleBySide.Add(Str(Step, TEXT("side"), TEXT("")), Str(Step, TEXT("style"), TEXT("stage_right")));
	}
	TArray<TSharedPtr<FJsonValue>> Steps;
	auto AddStep = [&](bool bEnabled, const TCHAR* Side)
	{
		if (!bEnabled)
		{
			return;
		}
		const FObj Step = MakeShared<FJsonObject>();
		Step->SetStringField(TEXT("side"), Side);
		Step->SetStringField(TEXT("style"), StyleBySide.Contains(Side) ? StyleBySide[Side] : TEXT("stage_right"));
		Steps.Add(MakeShared<FJsonValueObject>(Step));
	};
	AddStep(Actor.bEnableRightStep, TEXT("left"));
	AddStep(Actor.bEnableLeftStep, TEXT("right"));
	Out->SetArrayField(TEXT("steps"), Steps);

	Out->SetBoolField(TEXT("skirt"), Actor.bEnableAutomaticSkirt);
	return Out;
}

// ---- Truss -----------------------------------------------------------------------------

bool PieceFromName(const FString& Name, ETrussPieceType& OutPiece)
{
	if (Name == TEXT("2ft")) { OutPiece = ETrussPieceType::TwoFoot; return true; }
	if (Name == TEXT("4ft")) { OutPiece = ETrussPieceType::FourFoot; return true; }
	if (Name == TEXT("5ft")) { OutPiece = ETrussPieceType::FiveFoot; return true; }
	if (Name == TEXT("8ft")) { OutPiece = ETrussPieceType::EightFoot; return true; }
	if (Name == TEXT("10ft")) { OutPiece = ETrussPieceType::TenFoot; return true; }
	return false;
}

FString NameFromPiece(ETrussPieceType Piece)
{
	switch (Piece)
	{
	case ETrussPieceType::TwoFoot: return TEXT("2ft");
	case ETrussPieceType::FiveFoot: return TEXT("5ft");
	case ETrussPieceType::EightFoot: return TEXT("8ft");
	case ETrussPieceType::TenFoot: return TEXT("10ft");
	default: return TEXT("4ft");
	}
}

bool ApplyTruss(ATrussStructureActor& Actor, const FObj& Params, TArray<FString>& Notes, FString& OutError)
{
	const FString Shape = Str(Params, TEXT("shape"), TEXT(""));
	FTrussBuildDefinition Definition = Actor.GetBuildDefinition();

	if (Shape == TEXT("straight_run"))
	{
		Definition.BuildMode = ETrussBuildMode::StraightRun;
		Definition.LengthFt = Num(Params, TEXT("length_ft"), Definition.LengthFt);
		Definition.StraightRunHeightFt = Num(Params, TEXT("height_ft"), Definition.StraightRunHeightFt);
	}
	else if (Shape == TEXT("rectangle"))
	{
		Definition.BuildMode = ETrussBuildMode::Rectangle;
		Definition.RectangleLengthFt = Num(Params, TEXT("length_ft"), Definition.RectangleLengthFt);
		Definition.RectangleWidthFt = Num(Params, TEXT("width_ft"), Definition.RectangleWidthFt);
		Definition.RectangleHeightFt = Num(Params, TEXT("height_ft"), Definition.RectangleHeightFt);
	}
	else if (Shape == TEXT("tower"))
	{
		Definition.BuildMode = ETrussBuildMode::Tower;
		Definition.TowerHeightFt = Num(Params, TEXT("height_ft"), Definition.TowerHeightFt);
		if (Flag(Params, TEXT("t_top"), false))
		{
			Notes.Add(TEXT("Unreal does not build a T-top tower; built a plain tower."));
		}
	}
	else if (Shape == TEXT("arch"))
	{
		Definition.BuildMode = ETrussBuildMode::Arch;
		Definition.ArchWidthFt = Num(Params, TEXT("width_ft"), Definition.ArchWidthFt);
		Definition.ArchHeightFt = Num(Params, TEXT("height_ft"), Definition.ArchHeightFt);
	}
	else if (Shape == TEXT("cube"))
	{
		Definition.BuildMode = ETrussBuildMode::Cube;
		Definition.CubeLengthFt = Num(Params, TEXT("length_ft"), Definition.CubeLengthFt);
		Definition.CubeWidthFt = Num(Params, TEXT("width_ft"), Definition.CubeWidthFt);
		Definition.CubeHeightFt = Num(Params, TEXT("height_ft"), Definition.CubeHeightFt);
	}
	else if (Shape == TEXT("cube_arch"))
	{
		Definition.BuildMode = ETrussBuildMode::CubeArch;
		Definition.CubeArchWidthFt = Num(Params, TEXT("width_ft"), Definition.CubeArchWidthFt);
		Definition.CubeArchHeightFt = Num(Params, TEXT("height_ft"), Definition.CubeArchHeightFt);
		PieceFromName(Str(Params, TEXT("side_spacing_piece"), TEXT("4ft")), Definition.CubeArchSideSpacingPiece);
		PieceFromName(Str(Params, TEXT("depth_spacing_piece"), TEXT("4ft")), Definition.CubeArchDepthSpacingPiece);
	}
	else
	{
		OutError = FString::Printf(TEXT("Unknown truss shape \"%s\"."), *Shape);
		return false;
	}

	Actor.ApplyBuildDefinition(Definition, true);
	return true;
}

FObj ReadTruss(const ATrussStructureActor& Actor, const FObj& Stamped)
{
	const FObj Out = MakeShared<FJsonObject>();
	CopyFields(Stamped, Out);

	const FTrussBuildDefinition Definition = Actor.GetBuildDefinition();
	switch (Definition.BuildMode)
	{
	case ETrussBuildMode::StraightRun:
		Out->SetStringField(TEXT("shape"), TEXT("straight_run"));
		Out->SetNumberField(TEXT("length_ft"), Round4(Definition.LengthFt));
		Out->SetNumberField(TEXT("height_ft"), Round4(Definition.StraightRunHeightFt));
		break;
	case ETrussBuildMode::Rectangle:
		Out->SetStringField(TEXT("shape"), TEXT("rectangle"));
		Out->SetNumberField(TEXT("length_ft"), Round4(Definition.RectangleLengthFt));
		Out->SetNumberField(TEXT("width_ft"), Round4(Definition.RectangleWidthFt));
		Out->SetNumberField(TEXT("height_ft"), Round4(Definition.RectangleHeightFt));
		break;
	case ETrussBuildMode::Tower:
		Out->SetStringField(TEXT("shape"), TEXT("tower"));
		Out->SetNumberField(TEXT("height_ft"), Round4(Definition.TowerHeightFt));
		break;
	case ETrussBuildMode::Arch:
		Out->SetStringField(TEXT("shape"), TEXT("arch"));
		Out->SetNumberField(TEXT("width_ft"), Round4(Definition.ArchWidthFt));
		Out->SetNumberField(TEXT("height_ft"), Round4(Definition.ArchHeightFt));
		break;
	case ETrussBuildMode::Cube:
		Out->SetStringField(TEXT("shape"), TEXT("cube"));
		Out->SetNumberField(TEXT("length_ft"), Round4(Definition.CubeLengthFt));
		Out->SetNumberField(TEXT("width_ft"), Round4(Definition.CubeWidthFt));
		Out->SetNumberField(TEXT("height_ft"), Round4(Definition.CubeHeightFt));
		break;
	case ETrussBuildMode::CubeArch:
		Out->SetStringField(TEXT("shape"), TEXT("cube_arch"));
		Out->SetNumberField(TEXT("width_ft"), Round4(Definition.CubeArchWidthFt));
		Out->SetNumberField(TEXT("height_ft"), Round4(Definition.CubeArchHeightFt));
		Out->SetStringField(TEXT("side_spacing_piece"), NameFromPiece(Definition.CubeArchSideSpacingPiece));
		Out->SetStringField(TEXT("depth_spacing_piece"), NameFromPiece(Definition.CubeArchDepthSpacingPiece));
		break;
	}
	return Out;
}

// ---- Drape -----------------------------------------------------------------------------

bool ApplyDrape(ADrapeRunActor& Actor, const FObj& Params, TArray<FString>& Notes, FString& OutError)
{
	if (Str(Params, TEXT("kind"), TEXT("regular")) != TEXT("regular"))
	{
		OutError = TEXT("Control drapes are not built in Unreal.");
		return false;
	}

	const double HeightFt = Num(Params, TEXT("height_ft"), 8.0);
	if (HeightFt < 6.0 || HeightFt > MaxDrapeHeightFt)
	{
		Notes.Add(FString::Printf(TEXT("Height %g ft is outside Unreal's 6 to 18.33 ft range; clamped."), HeightFt));
	}

	FDrapeRunBuildDefinition Definition = Actor.GetBuildDefinition();
	Definition.LengthFt = static_cast<float>(FMath::Max(1.0, Num(Params, TEXT("length_ft"), Definition.LengthFt)));
	Definition.HeightFt = static_cast<float>(FMath::Clamp(HeightFt, 6.0, static_cast<double>(MaxDrapeHeightFt)));
	Definition.bShowHardware = Flag(Params, TEXT("hardware"), true);
	Definition.bCenterOnActor = false;
	Actor.bShowDrape = Flag(Params, TEXT("fabric"), true);
	Actor.ApplyBuildDefinition(Definition, true);
	return true;
}

FObj ReadDrape(const ADrapeRunActor& Actor, const FObj& Stamped)
{
	const FObj Out = MakeShared<FJsonObject>();
	CopyFields(Stamped, Out);
	Out->SetStringField(TEXT("kind"), TEXT("regular"));
	Out->SetNumberField(TEXT("length_ft"), Round4(Actor.LengthFt));
	Out->SetNumberField(TEXT("height_ft"), Round4(Actor.HeightFt));
	Out->SetBoolField(TEXT("fabric"), Actor.bShowDrape);
	Out->SetBoolField(TEXT("hardware"), Actor.bShowHardware);
	return Out;
}

// ---- MBP wall --------------------------------------------------------------------------
// The wall's params are the web builder's own payload. The actor's columns run the other way
// round (its column 0 is house right once turned), so columns and end caps are mirrored.

int32 MbpColumns(const FObj& Params)
{
	return FMath::Max(1, FMath::RoundToInt(Num(Params, TEXT("width_ft"), 12.0) / 3.0));
}

bool ApplyMbp(AMBPWallActor& Actor, const FObj& Params, TArray<FString>& Notes, FString& OutError)
{
	const int32 Columns = MbpColumns(Params);
	const FObj Mirrored = MakeShared<FJsonObject>();
	CopyFields(Params, Mirrored);
	Mirrored->SetBoolField(TEXT("left_cap"), Flag(Params, TEXT("right_cap"), false));
	Mirrored->SetBoolField(TEXT("right_cap"), Flag(Params, TEXT("left_cap"), false));

	TArray<TSharedPtr<FJsonValue>> Cells;
	for (const FObj& Cell : ObjectList(Params, TEXT("cells")))
	{
		const FObj Copy = MakeShared<FJsonObject>();
		CopyFields(Cell, Copy);
		Copy->SetNumberField(TEXT("col"), Columns - 1 - FMath::RoundToInt(Num(Cell, TEXT("col"), 0.0)));
		Cells.Add(MakeShared<FJsonValueObject>(Copy));
	}
	Mirrored->SetArrayField(TEXT("cells"), Cells);

	return Actor.ImportWebLayoutFromJson(ToText(Mirrored), OutError);
}

bool WebStyleForSlot(const FMBPPanelSlot& Slot, FString& OutStyle)
{
	switch (Slot.Style)
	{
	case EMBPPanelStyle::Acrylic: OutStyle = TEXT("acrylic"); return true;
	case EMBPPanelStyle::Boxwood: OutStyle = TEXT("greenwall"); return true;
	case EMBPPanelStyle::Drift: OutStyle = TEXT("drift"); return true;
	case EMBPPanelStyle::Geo: OutStyle = TEXT("geowall"); return true;
	case EMBPPanelStyle::Hive: OutStyle = TEXT("hive"); return true;
	case EMBPPanelStyle::Platinum: OutStyle = TEXT("platinum"); return true;
	case EMBPPanelStyle::Blank: OutStyle = TEXT("blank"); return true;
	case EMBPPanelStyle::Shimmer:
		switch (Slot.ShimmerVariant)
		{
		case EMBPShimmerVariant::Red: OutStyle = TEXT("redshimmer"); break;
		case EMBPShimmerVariant::Fuchsia: OutStyle = TEXT("fuchsiashimmer"); break;
		case EMBPShimmerVariant::Holographic: OutStyle = TEXT("holographicshimmer"); break;
		default: OutStyle = TEXT("goldshimmer"); break;
		}
		return true;
	default:
		return false;
	}
}

FObj ReadMbp(const AMBPWallActor& Actor, const FObj& Stamped, TArray<FString>& Notes)
{
	const FObj Out = MakeShared<FJsonObject>();
	CopyFields(Stamped, Out);

	Out->SetNumberField(TEXT("version"), 1);
	Out->SetNumberField(TEXT("width_ft"), Actor.Columns * 3);
	Out->SetNumberField(TEXT("height_ft"), Actor.Rows * 3);

	FString Spacing = TEXT("every_other");
	if (Actor.SupportInterval == 1) { Spacing = TEXT("every"); }
	else if (Actor.SupportInterval == 3) { Spacing = TEXT("every_third"); }
	else if (Actor.SupportInterval >= 99) { Spacing = TEXT("ends"); }
	Out->SetStringField(TEXT("support_spacing"), Spacing);
	Out->SetBoolField(TEXT("left_cap"), Actor.bRightCap);
	Out->SetBoolField(TEXT("right_cap"), Actor.bLeftCap);

	TArray<TSharedPtr<FJsonValue>> Cells;
	bool bCustom = false;
	for (int32 Row = 0; Row < Actor.Rows; ++Row)
	{
		for (int32 Column = 0; Column < Actor.Columns; ++Column)
		{
			FMBPPanelSlot Slot;
			if (!Actor.GetPanelSlot(Row, Column, Slot) || Slot.Style == EMBPPanelStyle::Empty)
			{
				continue;
			}
			FString WebStyle;
			if (!WebStyleForSlot(Slot, WebStyle))
			{
				bCustom = true;
				continue;
			}

			const int32 RawDepthIn = FMath::RoundToInt(Slot.DepthOffsetCm / 2.54f);
			const int32 DepthIn = FMath::Clamp(FMath::RoundToInt(RawDepthIn / 12.0f) * 12, 0, 36);

			const FObj Cell = MakeShared<FJsonObject>();
			Cell->SetNumberField(TEXT("row"), Row);
			Cell->SetNumberField(TEXT("col"), Actor.Columns - 1 - Column);
			Cell->SetStringField(TEXT("style"), WebStyle);
			Cell->SetNumberField(TEXT("depth_in"), DepthIn);
			Cells.Add(MakeShared<FJsonValueObject>(Cell));
		}
	}
	if (bCustom)
	{
		Notes.Add(TEXT("Custom panels have no web style and were left out."));
	}
	Out->SetArrayField(TEXT("cells"), Cells);
	return Out;
}

// ---- Import / export of one kind of actor ------------------------------------------------

template <typename T>
T* SpawnActorAtOrigin(UWorld& World)
{
	FActorSpawnParameters Parameters;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return World.SpawnActor<T>(T::StaticClass(), FTransform::Identity, Parameters);
}

void CollectStampedIds(UWorld& World, TSet<FString>& OutIds)
{
	for (TActorIterator<AActor> It(&World); It; ++It)
	{
		if (const UMajicLayoutStampComponent* Stamp = FindStamp(*It))
		{
			OutIds.Add(Stamp->ItemId);
		}
	}
}

template <typename T>
void ExportActors(UWorld& World, const TCHAR* Type, const TCHAR* IdPrefix, TSet<FString>& UsedIds, TArray<FMajicLayoutItem>& OutItems, TArray<FString>& OutNotes,
	TFunctionRef<FObj(T&, const FObj& Stamped, TArray<FString>& Notes)> ReadParams, TFunctionRef<FVector(T&)> GetOrigin)
{
	for (TActorIterator<T> It(&World); It; ++It)
	{
		T* Actor = *It;
		UMajicLayoutStampComponent* Stamp = FindStamp(Actor);
		const FObj Stamped = Stamp ? FromText(Stamp->ParamsJson) : MakeShared<FJsonObject>();

		FMajicLayoutItem Item;
		Item.Type = Type;
		Item.Id = (Stamp && !Stamp->ItemId.IsEmpty()) ? Stamp->ItemId : NewId(IdPrefix, UsedIds);
		Item.Name = ActorName(Actor, Stamp ? Stamp->ItemName : Item.Id);
		if (Item.Name.IsEmpty())
		{
			Item.Name = Item.Id;
		}

		TArray<FString> Notes;
		Item.Params = ReadParams(*Actor, Stamped, Notes);
		for (const FString& Note : Notes)
		{
			OutNotes.Add(FString::Printf(TEXT("%s: %s"), *Item.Id, *Note));
		}

		MajicEventLayout::ReadOriginFromTransform(Actor->GetActorTransform(), GetOrigin(*Actor), Item.XFt, Item.YFt, Item.RotationDeg);
		StampActor(Actor, Item.Id, Item.Name, Item.Params);
		OutItems.Add(MoveTemp(Item));
	}
}
}

AMajicEventLayoutActor::AMajicEventLayoutActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

bool AMajicEventLayoutActor::ImportLayoutFile(const FString& FilePath, TArray<FString>& OutLines)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		OutLines.Add(TEXT("There is no world to build in."));
		return false;
	}

	FMajicLayoutDoc Doc;
	FString Error;
	if (!MajicEventLayout::LoadFile(FilePath, Doc, Error))
	{
		OutLines.Add(Error);
		return false;
	}

	TSet<FString> ExistingIds;
	CollectStampedIds(*World, ExistingIds);

	int32 Built = 0;
	int32 Skipped = 0;
	for (const FMajicLayoutItem& Item : Doc.Items)
	{
		const bool bStage = Item.Type == TypeStage;
		const bool bTruss = Item.Type == TypeTruss;
		const bool bDrape = Item.Type == TypeDrape;
		const bool bMbp = Item.Type == TypeMbp;
		if (!bStage && !bTruss && !bDrape && !bMbp)
		{
			continue; // another program's item type: kept in the file, nothing to build
		}
		if (ExistingIds.Contains(Item.Id))
		{
			OutLines.Add(FString::Printf(TEXT("Skipped '%s': already in the level."), *Item.Id));
			continue;
		}

		TArray<FString> Notes;
		FString ItemError;
		AActor* Actor = nullptr;
		FVector Origin = FVector::ZeroVector;
		bool bOk = false;

		if (bStage)
		{
			AStageDeckActor* Stage = SpawnActorAtOrigin<AStageDeckActor>(*World);
			Actor = Stage;
			bOk = Stage && ApplyStage(*Stage, Item.Params, Notes, ItemError);
			if (bOk) { Origin = StageOrigin(*Stage); }
		}
		else if (bTruss)
		{
			ATrussStructureActor* Truss = SpawnActorAtOrigin<ATrussStructureActor>(*World);
			Actor = Truss;
			bOk = Truss && ApplyTruss(*Truss, Item.Params, Notes, ItemError);
			if (bOk) { Origin = TrussOrigin(*Truss); }
		}
		else if (bDrape)
		{
			ADrapeRunActor* Drape = SpawnActorAtOrigin<ADrapeRunActor>(*World);
			Actor = Drape;
			bOk = Drape && ApplyDrape(*Drape, Item.Params, Notes, ItemError);
			if (bOk) { Origin = DrapeOrigin(*Drape); }
		}
		else
		{
			AMBPWallActor* Wall = SpawnActorAtOrigin<AMBPWallActor>(*World);
			Actor = Wall;
			bOk = Wall && ApplyMbp(*Wall, Item.Params, Notes, ItemError);
			if (bOk) { Origin = MbpOrigin(*Wall); }
		}

		if (!bOk)
		{
			OutLines.Add(FString::Printf(TEXT("Failed '%s': %s"), *Item.Id, ItemError.IsEmpty() ? TEXT("could not spawn the actor") : *ItemError));
			if (Actor)
			{
				Actor->Destroy();
			}
			++Skipped;
			continue;
		}

		Actor->SetActorTransform(MajicEventLayout::ActorTransformForOrigin(Item.XFt, Item.YFt, Item.RotationDeg, Origin));
		SetActorName(Actor, Item.Name);
		StampActor(Actor, Item.Id, Item.Name, Item.Params);
		for (const FString& Note : Notes)
		{
			OutLines.Add(FString::Printf(TEXT("'%s': %s"), *Item.Id, *Note));
		}
		++Built;
	}

	OutLines.Insert(FString::Printf(TEXT("Imported %d item(s)%s."), Built, Skipped > 0 ? *FString::Printf(TEXT(", %d failed"), Skipped) : TEXT("")), 0);
	if (!FilePath.IsEmpty())
	{
		LayoutFile = FPaths::ConvertRelativePathToFull(FilePath);
	}
	return true;
}

bool AMajicEventLayoutActor::ExportLayoutFile(const FString& FilePath, TArray<FString>& OutLines)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		OutLines.Add(TEXT("There is no world to read."));
		return false;
	}

	FMajicLayoutDoc Doc;
	FString Error;
	if (FPaths::FileExists(FilePath))
	{
		if (!MajicEventLayout::LoadFile(FilePath, Doc, Error))
		{
			OutLines.Add(Error);
			return false;
		}
	}

	TSet<FString> UsedIds;
	CollectStampedIds(*World, UsedIds);
	for (const FMajicLayoutItem& Existing : Doc.Items)
	{
		UsedIds.Add(Existing.Id);
	}

	TArray<FMajicLayoutItem> Items;
	TArray<FString> Notes;
	ExportActors<AStageDeckActor>(*World, TypeStage, TEXT("stage"), UsedIds, Items, Notes,
		[](AStageDeckActor& Actor, const FObj& Stamped, TArray<FString>&) { return ReadStage(Actor, Stamped); },
		[](AStageDeckActor& Actor) { return StageOrigin(Actor); });
	ExportActors<ATrussStructureActor>(*World, TypeTruss, TEXT("truss"), UsedIds, Items, Notes,
		[](ATrussStructureActor& Actor, const FObj& Stamped, TArray<FString>&) { return ReadTruss(Actor, Stamped); },
		[](ATrussStructureActor& Actor) { return TrussOrigin(Actor); });
	ExportActors<ADrapeRunActor>(*World, TypeDrape, TEXT("drape"), UsedIds, Items, Notes,
		[](ADrapeRunActor& Actor, const FObj& Stamped, TArray<FString>&) { return ReadDrape(Actor, Stamped); },
		[](ADrapeRunActor& Actor) { return DrapeOrigin(Actor); });
	ExportActors<AMBPWallActor>(*World, TypeMbp, TEXT("mbp"), UsedIds, Items, Notes,
		[](AMBPWallActor& Actor, const FObj& Stamped, TArray<FString>& ItemNotes) { return ReadMbp(Actor, Stamped, ItemNotes); },
		[](AMBPWallActor& Actor) { return MbpOrigin(Actor); });

	const TSet<FString> Replaced = { TypeStage, TypeTruss, TypeDrape, TypeMbp };
	MajicEventLayout::MergeItems(Doc, Items, Replaced, [](const FMajicLayoutItem& Existing)
	{
		// Control drapes are only built by Blender; Unreal can not hold them, so the file keeps them.
		return Existing.Type == TypeDrape && Str(Existing.Params, TEXT("kind"), TEXT("regular")) != TEXT("regular");
	});

	if (!MajicEventLayout::SaveFile(FilePath, Doc, Error))
	{
		OutLines.Add(Error);
		return false;
	}

	OutLines.Add(FString::Printf(TEXT("Exported %d item(s) to %s."), Items.Num(), *FilePath));
	OutLines.Append(Notes);
	LayoutFile = FPaths::ConvertRelativePathToFull(FilePath);
	return true;
}

void AMajicEventLayoutActor::ImportEventLayout()
{
#if WITH_EDITOR
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("The file picker is unavailable.")));
		return;
	}

	TArray<FString> SelectedFiles;
	const FString StartDirectory = LayoutFile.IsEmpty() ? FPaths::ProjectDir() : FPaths::GetPath(LayoutFile);
	if (!DesktopPlatform->OpenFileDialog(nullptr, TEXT("Import Event Layout"), StartDirectory, TEXT(""),
		TEXT("Majic Event Layout (*.json)|*.json"), EFileDialogFlags::None, SelectedFiles) || SelectedFiles.IsEmpty())
	{
		return;
	}

	TArray<FString> Lines;
	ImportLayoutFile(SelectedFiles[0], Lines);
	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(FString::Join(Lines, TEXT("\n"))), FText::FromString(TEXT("Import Event Layout")));
#endif
}

void AMajicEventLayoutActor::ExportEventLayout()
{
#if WITH_EDITOR
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("The file picker is unavailable.")));
		return;
	}

	TArray<FString> SelectedFiles;
	const FString StartDirectory = LayoutFile.IsEmpty() ? FPaths::ProjectDir() : FPaths::GetPath(LayoutFile);
	const FString StartName = LayoutFile.IsEmpty() ? TEXT("event-layout.json") : FPaths::GetCleanFilename(LayoutFile);
	if (!DesktopPlatform->SaveFileDialog(nullptr, TEXT("Export Event Layout"), StartDirectory, StartName,
		TEXT("Majic Event Layout (*.json)|*.json"), EFileDialogFlags::None, SelectedFiles) || SelectedFiles.IsEmpty())
	{
		return;
	}

	TArray<FString> Lines;
	ExportLayoutFile(SelectedFiles[0], Lines);
	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(FString::Join(Lines, TEXT("\n"))), FText::FromString(TEXT("Export Event Layout")));
#endif
}
