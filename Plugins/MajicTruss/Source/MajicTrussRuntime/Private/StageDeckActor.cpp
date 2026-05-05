#include "StageDeckActor.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInterface.h"
#include "Modules/ModuleManager.h"

namespace
{
constexpr TCHAR StageDeckPrefix[] = TEXT("StageDeck");
constexpr TCHAR StagePodiumPrefix[] = TEXT("StagePodium");
constexpr TCHAR StageDrapePrefix[] = TEXT("StageDrape");
constexpr TCHAR StageRailingPrefix[] = TEXT("StageRailing");
constexpr TCHAR StageStepPrefix[] = TEXT("StageStep");
constexpr float RailingSpan94Cm = 243.84f;
constexpr float RailingSpan46Cm = 121.92f;
const FVector TunedRailingBaseOffset(249.338089f, -61.650452f, 32.453884f);
const FRotator TunedRailingBaseRotation(0.0f, 180.0f, 0.0f);
const FVector TunedFrontRailingOffset(249.338089f, -61.650452f, 0.0f);
const FVector TunedBackRailingOffset(0.0f, 62.298616f, 0.0f);
const FVector TunedLeftRailingOffset(189.700066f, 120.653588f, 0.0f);
const FVector TunedRightRailingOffset(58.727953f, -121.189095f, 0.0f);
const FVector PreviousFrontRailingOffset(249.338089f, -61.650452f, 32.453884f);
const FVector PreviousBackRailingOffset(0.0f, 62.298616f, 32.453884f);
const FVector PreviousLeftRailingOffset(189.700066f, 120.653588f, 32.453884f);
const FVector PreviousRightRailingOffset(58.727953f, -121.189095f, 32.453884f);
const FVector Tuned46SpanAdjustment(-61.084781f, 0.0f, 0.0f);

bool IsStaticMeshAsset(const FAssetData& AssetData)
{
	return AssetData.AssetClassPath == UStaticMesh::StaticClass()->GetClassPathName();
}

FString GetDrapeMeshPath()
{
	return TEXT("/Game/Majic_Gear/StageDrape/StageDrape.StageDrape");
}
}

AStageDeckActor::AStageDeckActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RailingPlacementOffsetCm = TunedRailingBaseOffset;
	RailingPlacementRotation = TunedRailingBaseRotation;
	FrontRailingOffsetCm = TunedFrontRailingOffset;
	BackRailingOffsetCm = TunedBackRailingOffset;
	LeftRailingOffsetCm = TunedLeftRailingOffset;
	RightRailingOffsetCm = TunedRightRailingOffset;
	Railing46SpanAdjustmentCm = Tuned46SpanAdjustment;
	RailingHeightAdjust8Cm = 0.0f;
	RailingHeightAdjust12Cm = 11.0f;
	RailingHeightAdjust24Cm = 32.0f;
	RailingHeightAdjust27Cm = 55.5f;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	SelectionBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("SelectionBounds"));
	SelectionBounds->SetupAttachment(SceneRoot);
	SelectionBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SelectionBounds->SetCollisionObjectType(ECC_WorldDynamic);
	SelectionBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	SelectionBounds->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SelectionBounds->SetGenerateOverlapEvents(false);
	SelectionBounds->SetHiddenInGame(true);
	SelectionBounds->SetLineThickness(2.0f);
	SelectionBounds->ShapeColor = FColor::Cyan;
}

void AStageDeckActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	const bool bUsingOldZeroedRailingSetup =
		RailingPlacementOffsetCm.IsNearlyZero() &&
		RailingPlacementRotation.Equals(FRotator::ZeroRotator, KINDA_SMALL_NUMBER) &&
		FrontRailingOffsetCm.IsNearlyZero() &&
		BackRailingOffsetCm.IsNearlyZero() &&
		LeftRailingOffsetCm.IsNearlyZero() &&
		RightRailingOffsetCm.IsNearlyZero() &&
		Railing46SpanAdjustmentCm.IsNearlyZero() &&
		FMath::IsNearlyZero(RailingHeightAdjust8Cm) &&
		FMath::IsNearlyZero(RailingHeightAdjust12Cm) &&
		FMath::IsNearlyZero(RailingHeightAdjust24Cm) &&
		FMath::IsNearlyZero(RailingHeightAdjust27Cm);

	const bool bUsingPreviousBakedRailingHeightSetup =
		FrontRailingOffsetCm.Equals(PreviousFrontRailingOffset, 0.01f) &&
		BackRailingOffsetCm.Equals(PreviousBackRailingOffset, 0.01f) &&
		LeftRailingOffsetCm.Equals(PreviousLeftRailingOffset, 0.01f) &&
		RightRailingOffsetCm.Equals(PreviousRightRailingOffset, 0.01f);

	if (bUsingOldZeroedRailingSetup || bUsingPreviousBakedRailingHeightSetup)
	{
		RailingPlacementOffsetCm = TunedRailingBaseOffset;
		RailingPlacementRotation = TunedRailingBaseRotation;
		FrontRailingOffsetCm = TunedFrontRailingOffset;
		BackRailingOffsetCm = TunedBackRailingOffset;
		LeftRailingOffsetCm = TunedLeftRailingOffset;
		RightRailingOffsetCm = TunedRightRailingOffset;
		Railing46SpanAdjustmentCm = Tuned46SpanAdjustment;
		RailingHeightAdjust8Cm = 0.0f;
		RailingHeightAdjust12Cm = 11.0f;
		RailingHeightAdjust24Cm = 32.0f;
		RailingHeightAdjust27Cm = 55.5f;
	}

	EnsureCellCount(false);
	SyncCellsToDefaultIfNeeded();
	if (bBuildOnConstruction)
	{
		RebuildStage();
	}
}

#if WITH_EDITOR
void AStageDeckActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AStageDeckActor, DefaultHeightPreset) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(AStageDeckActor, DefaultSurfaceStyle))
	{
		ApplyDefaultSettingsToAllCells();
		CachedDefaultHeightPreset = DefaultHeightPreset;
		CachedDefaultSurfaceStyle = DefaultSurfaceStyle;
		bHasCachedDefaults = true;
		RebuildStage();
		return;
	}

	if (PropertyName == GET_MEMBER_NAME_CHECKED(AStageDeckActor, BatchTargetIndex) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(AStageDeckActor, BatchEditAxis) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(AStageDeckActor, bBatchEnabled) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(AStageDeckActor, BatchHeightPreset) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(AStageDeckActor, BatchSurfaceStyle) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(AStageDeckActor, BatchSurfaceMaterial))
	{
		ApplyBatchEdit();
	}
}
#endif

void AStageDeckActor::RebuildStage()
{
	EnsureCellCount(false);
	ClearGeneratedComponents();
	ClearGeneratedDrapes();
	ClearGeneratedLabels();

	const float StepX = CellWidthCm + HorizontalSpacingCm;
	const float StepY = CellDepthCm + VerticalSpacingCm;
	const float OriginX = bCenterOnActor ? (-0.5f * (Columns - 1) * StepX) : 0.0f;
	const float OriginY = bCenterOnActor ? (-0.5f * (Rows - 1) * StepY) : 0.0f;

	TMap<FString, UInstancedStaticMeshComponent*> BucketMap;
	FBox Bounds(EForceInit::ForceInit);
	float MaxDeckHeightCm = 0.0f;

	for (int32 RowIndex = 0; RowIndex < Rows; ++RowIndex)
	{
		for (int32 ColumnIndex = 0; ColumnIndex < Columns; ++ColumnIndex)
		{
			const int32 CellIndex = GetCellLinearIndex(RowIndex, ColumnIndex);
			if (!DeckCells.IsValidIndex(CellIndex))
			{
				continue;
			}

			const FVector CellCenter(
				OriginX + (ColumnIndex * StepX),
				OriginY + (RowIndex * StepY),
				0.0f);
			const FStageDeckCell& Cell = DeckCells[CellIndex];
			if (bShowCellLabels)
			{
				AddCellLabel(RowIndex, ColumnIndex, CellCenter, Cell.bEnabled);
			}
			if (!Cell.bEnabled)
			{
				continue;
			}
			const float DeckHeightCm = GetDeckHeightCm(Cell.HeightPreset);
			const float HeightScaleMultiplier = Cell.HeightPreset == EStageDeckHeightPreset::In27 ? SkirtHeightScale27Inch : 1.0f;
			MaxDeckHeightCm = FMath::Max(MaxDeckHeightCm, DeckHeightCm);

			for (const FSoftObjectPath& MeshPath : GetDeckMeshPaths(Cell.HeightPreset, Cell.SurfaceStyle))
			{
				AddDeckCellInstance(MeshPath, Cell.SurfaceStyle, CellCenter, BucketMap, Bounds);
			}

			if (!bEnableAutomaticSkirt)
			{
				continue;
			}

			const bool bFrontExposed = (RowIndex == 0) || !DeckCells.IsValidIndex(GetCellLinearIndex(RowIndex - 1, ColumnIndex)) || !DeckCells[GetCellLinearIndex(RowIndex - 1, ColumnIndex)].bEnabled;
			const bool bBackExposed = (RowIndex == Rows - 1) || !DeckCells.IsValidIndex(GetCellLinearIndex(RowIndex + 1, ColumnIndex)) || !DeckCells[GetCellLinearIndex(RowIndex + 1, ColumnIndex)].bEnabled;
			const bool bLeftExposed = (ColumnIndex == 0) || !DeckCells.IsValidIndex(GetCellLinearIndex(RowIndex, ColumnIndex - 1)) || !DeckCells[GetCellLinearIndex(RowIndex, ColumnIndex - 1)].bEnabled;
			const bool bRightExposed = (ColumnIndex == Columns - 1) || !DeckCells.IsValidIndex(GetCellLinearIndex(RowIndex, ColumnIndex + 1)) || !DeckCells[GetCellLinearIndex(RowIndex, ColumnIndex + 1)].bEnabled;

			if (bFrontExposed)
			{
				const FRotator EdgeRotation(0.0f, 0.0f, 0.0f);
				AddDrapeForEdge(
					FVector(CellCenter.X, CellCenter.Y - (0.5f * CellDepthCm), 0.0f) + EdgeRotation.RotateVector(DebugSkirtOffsetCm + AutoSkirtFrontAdjustmentCm),
					EdgeRotation + DebugSkirtRotation,
					CellWidthCm,
					DeckHeightCm * HeightScaleMultiplier,
					Bounds,
					DebugSkirtScale);
			}

			if (bBackExposed)
			{
				const FRotator EdgeRotation(0.0f, 180.0f, 0.0f);
				AddDrapeForEdge(
					FVector(CellCenter.X, CellCenter.Y + (0.5f * CellDepthCm), 0.0f) + EdgeRotation.RotateVector(DebugSkirtOffsetCm + AutoSkirtBackAdjustmentCm),
					EdgeRotation + DebugSkirtRotation,
					CellWidthCm,
					DeckHeightCm * HeightScaleMultiplier,
					Bounds,
					DebugSkirtScale);
			}

			if (bLeftExposed)
			{
				const FRotator EdgeRotation(0.0f, -90.0f, 0.0f);
				AddDrapeForEdge(
					FVector(CellCenter.X - (0.5f * CellWidthCm), CellCenter.Y, 0.0f) + EdgeRotation.RotateVector(DebugSkirtOffsetCm + AutoSkirtLeftAdjustmentCm),
					EdgeRotation + DebugSkirtRotation,
					CellDepthCm,
					DeckHeightCm * HeightScaleMultiplier,
					Bounds,
					DebugSkirtScale);
			}

			if (bRightExposed)
			{
				const FRotator EdgeRotation(0.0f, 90.0f, 0.0f);
				AddDrapeForEdge(
					FVector(CellCenter.X + (0.5f * CellWidthCm), CellCenter.Y, 0.0f) + EdgeRotation.RotateVector(DebugSkirtOffsetCm + AutoSkirtRightAdjustmentCm),
					EdgeRotation + DebugSkirtRotation,
					CellDepthCm,
					DeckHeightCm * HeightScaleMultiplier,
					Bounds,
					DebugSkirtScale);
			}
		}
	}

	AddRailingsForPerimeter(OriginX, OriginY, StepX, StepY, BucketMap, Bounds);

	if (bUseDebugSingleSkirt)
	{
		int32 DebugRowIndex = INDEX_NONE;
		int32 DebugColumnIndex = INDEX_NONE;
		FStageDeckCell DebugCell;
		if (GetFirstEnabledCell(DebugRowIndex, DebugColumnIndex, DebugCell))
		{
			const FVector DebugCellCenter(
				OriginX + (DebugColumnIndex * StepX),
				OriginY + (DebugRowIndex * StepY),
				0.0f);
			const float DebugDeckHeightCm = GetDeckHeightCm(DebugCell.HeightPreset);
			const float DebugHeightScaleMultiplier = DebugCell.HeightPreset == EStageDeckHeightPreset::In27 ? SkirtHeightScale27Inch : 1.0f;
			const FRotator EdgeRotation(0.0f, 0.0f, 0.0f);
			const FVector FrontEdgeCenter = DebugCellCenter + FVector(0.0f, -(0.5f * CellDepthCm), 0.0f);
			AddDrapeForEdge(
				FrontEdgeCenter + EdgeRotation.RotateVector(DebugSkirtOffsetCm),
				EdgeRotation + DebugSkirtRotation,
				CellWidthCm,
				DebugDeckHeightCm * DebugHeightScaleMultiplier,
				Bounds,
				DebugSkirtScale);
		}
	}

	if (MaxDeckHeightCm > 0.0f && PodiumStyle != EStagePodiumStyle::None)
	{
		AddPodiumInstances(BucketMap, Bounds);
	}

	AddStepInstances(BucketMap, Bounds);
	UpdateSelectionBounds(Bounds);
}

bool AStageDeckActor::GetCellIndicesFromWorldLocation(const FVector& WorldLocation, int32& OutRow, int32& OutColumn) const
{
	const float StepX = CellWidthCm + HorizontalSpacingCm;
	const float StepY = CellDepthCm + VerticalSpacingCm;
	if (StepX <= KINDA_SMALL_NUMBER || StepY <= KINDA_SMALL_NUMBER || Columns <= 0 || Rows <= 0)
	{
		return false;
	}

	const float OriginX = bCenterOnActor ? (-0.5f * (Columns - 1) * StepX) : 0.0f;
	const float OriginY = bCenterOnActor ? (-0.5f * (Rows - 1) * StepY) : 0.0f;
	const FVector LocalHitLocation = GetActorTransform().InverseTransformPosition(WorldLocation);

	OutColumn = FMath::Clamp(FMath::RoundToInt((LocalHitLocation.X - OriginX) / StepX), 0, Columns - 1);
	OutRow = FMath::Clamp(FMath::RoundToInt((LocalHitLocation.Y - OriginY) / StepY), 0, Rows - 1);
	return IsValidCellIndexPair(OutRow, OutColumn);
}

void AStageDeckActor::SetSelectionHighlighted(bool bHighlighted)
{
	if (SelectionBounds)
	{
		SelectionBounds->SetHiddenInGame(!bHighlighted);
	}
}

void AStageDeckActor::ResetCellsToDefault()
{
	EnsureCellCount(true);
	RebuildStage();
}

void AStageDeckActor::ResizeCellsToGrid()
{
	EnsureCellCount(false);
	RebuildStage();
}

void AStageDeckActor::ApplyBuildDefinition(const FStageDeckBuildDefinition& Definition, bool bRebuildNow)
{
	Columns = FMath::Max(1, Definition.Columns);
	Rows = FMath::Max(1, Definition.Rows);
	DefaultHeightPreset = Definition.DefaultHeightPreset;
	DefaultSurfaceStyle = Definition.DefaultSurfaceStyle;
	bEnableFrontRailing = Definition.bEnableFrontRailing;
	bEnableBackRailing = Definition.bEnableBackRailing;
	bEnableLeftRailing = Definition.bEnableLeftRailing;
	bEnableRightRailing = Definition.bEnableRightRailing;
	bEnableLeftStep = Definition.bEnableLeftStep;
	bEnableRightStep = Definition.bEnableRightStep;
	bEnableAutomaticSkirt = Definition.bEnableAutomaticSkirt;

	EnsureCellCount(true);
	CachedDefaultHeightPreset = DefaultHeightPreset;
	CachedDefaultSurfaceStyle = DefaultSurfaceStyle;
	bHasCachedDefaults = true;

	if (bRebuildNow)
	{
		RebuildStage();
	}
}

bool AStageDeckActor::GetCellDefinition(int32 RowIndex, int32 ColumnIndex, FStageDeckCell& OutCell) const
{
	if (!IsValidCellIndexPair(RowIndex, ColumnIndex))
	{
		return false;
	}

	OutCell = DeckCells[GetCellLinearIndex(RowIndex, ColumnIndex)];
	return true;
}

void AStageDeckActor::ApplyCellDefinition(int32 RowIndex, int32 ColumnIndex, const FStageDeckCell& CellDefinition, bool bRebuildNow)
{
	if (!IsValidCellIndexPair(RowIndex, ColumnIndex))
	{
		return;
	}

	DeckCells[GetCellLinearIndex(RowIndex, ColumnIndex)] = CellDefinition;
	if (bRebuildNow)
	{
		RebuildStage();
	}
}

void AStageDeckActor::ApplyBatchEdit()
{
	for (const int32 CellIndex : GetBatchCellIndices())
	{
		if (!DeckCells.IsValidIndex(CellIndex))
		{
			continue;
		}

		FStageDeckCell& Cell = DeckCells[CellIndex];
		Cell.bEnabled = bBatchEnabled;
		Cell.HeightPreset = BatchHeightPreset;
		Cell.SurfaceStyle = BatchSurfaceStyle;
	}

	if (!BatchSurfaceMaterial.IsNull())
	{
		DefaultSurfaceMaterial = BatchSurfaceMaterial;
	}

	CachedDefaultHeightPreset = DefaultHeightPreset;
	CachedDefaultSurfaceStyle = DefaultSurfaceStyle;
	bHasCachedDefaults = true;
	RebuildStage();
}

void AStageDeckActor::EnsureCellCount(bool bResetNewCellsToDefault)
{
	const int32 TargetCount = FMath::Max(1, Columns) * FMath::Max(1, Rows);
	const int32 PreviousCount = DeckCells.Num();
	DeckCells.SetNum(TargetCount);

	for (int32 Index = 0; Index < TargetCount; ++Index)
	{
		if (bResetNewCellsToDefault || Index >= PreviousCount)
		{
			DeckCells[Index] = MakeDefaultCell();
		}
	}
}

FStageDeckCell AStageDeckActor::MakeDefaultCell() const
{
	FStageDeckCell Cell;
	Cell.bEnabled = true;
	Cell.HeightPreset = DefaultHeightPreset;
	Cell.SurfaceStyle = DefaultSurfaceStyle;
	return Cell;
}

void AStageDeckActor::SyncCellsToDefaultIfNeeded()
{
	if (!bHasCachedDefaults)
	{
		bHasCachedDefaults = true;
		CachedDefaultHeightPreset = DefaultHeightPreset;
		CachedDefaultSurfaceStyle = DefaultSurfaceStyle;
		return;
	}

	if (CachedDefaultHeightPreset == DefaultHeightPreset && CachedDefaultSurfaceStyle == DefaultSurfaceStyle)
	{
		return;
	}

	bool bCellsStillMatchCachedDefaults = true;
	for (const FStageDeckCell& Cell : DeckCells)
	{
		if (!Cell.bEnabled || Cell.HeightPreset != CachedDefaultHeightPreset || Cell.SurfaceStyle != CachedDefaultSurfaceStyle)
		{
			bCellsStillMatchCachedDefaults = false;
			break;
		}
	}

	if (bCellsStillMatchCachedDefaults)
	{
		ApplyDefaultSettingsToAllCells();
	}

	CachedDefaultHeightPreset = DefaultHeightPreset;
	CachedDefaultSurfaceStyle = DefaultSurfaceStyle;
}

void AStageDeckActor::ApplyDefaultSettingsToAllCells()
{
	for (FStageDeckCell& Cell : DeckCells)
	{
		Cell.bEnabled = true;
		Cell.HeightPreset = DefaultHeightPreset;
		Cell.SurfaceStyle = DefaultSurfaceStyle;
	}
}

TArray<int32> AStageDeckActor::GetBatchCellIndices() const
{
	TArray<int32> CellIndices;

	if (BatchEditAxis == EStageDeckBatchAxis::Row)
	{
		if (BatchTargetIndex < 0 || BatchTargetIndex >= Rows)
		{
			return CellIndices;
		}

		for (int32 ColumnIndex = 0; ColumnIndex < Columns; ++ColumnIndex)
		{
			CellIndices.Add(GetCellLinearIndex(BatchTargetIndex, ColumnIndex));
		}

		return CellIndices;
	}

	if (BatchTargetIndex < 0 || BatchTargetIndex >= Columns)
	{
		return CellIndices;
	}

	for (int32 RowIndex = 0; RowIndex < Rows; ++RowIndex)
	{
		CellIndices.Add(GetCellLinearIndex(RowIndex, BatchTargetIndex));
	}

	return CellIndices;
}

bool AStageDeckActor::IsValidCellIndexPair(int32 RowIndex, int32 ColumnIndex) const
{
	return RowIndex >= 0 && RowIndex < Rows && ColumnIndex >= 0 && ColumnIndex < Columns && DeckCells.IsValidIndex(GetCellLinearIndex(RowIndex, ColumnIndex));
}

int32 AStageDeckActor::GetCellLinearIndex(int32 RowIndex, int32 ColumnIndex) const
{
	return (RowIndex * Columns) + ColumnIndex;
}

float AStageDeckActor::GetDeckHeightCm(EStageDeckHeightPreset HeightPreset) const
{
	switch (HeightPreset)
	{
	case EStageDeckHeightPreset::In8:
		return 20.32f;
	case EStageDeckHeightPreset::In12:
		return 30.48f;
	case EStageDeckHeightPreset::In27:
		return 68.58f;
	case EStageDeckHeightPreset::In24:
	default:
		return 60.96f;
	}
}

float AStageDeckActor::GetRailingHeightDeltaZCm(EStageDeckHeightPreset HeightPreset) const
{
	switch (HeightPreset)
	{
	case EStageDeckHeightPreset::In8:
		return RailingHeightAdjust8Cm;
	case EStageDeckHeightPreset::In12:
		return RailingHeightAdjust12Cm;
	case EStageDeckHeightPreset::In27:
		return RailingHeightAdjust27Cm;
	case EStageDeckHeightPreset::In24:
	default:
		return RailingHeightAdjust24Cm;
	}
}

FString AStageDeckActor::GetDeckAssetFolder(EStageDeckHeightPreset HeightPreset, EStageDeckSurfaceStyle SurfaceStyle) const
{
	switch (HeightPreset)
	{
	case EStageDeckHeightPreset::In8:
		return TEXT("/Game/Majic_Gear/Stage/Single_Decks/Single_8_inch_Stage_Deck/StaticMeshes");
	case EStageDeckHeightPreset::In12:
		return TEXT("/Game/Majic_Gear/Stage/Single_Decks/Single_12_inch_Stage_Deck/StaticMeshes");
	case EStageDeckHeightPreset::In27:
		return TEXT("/Game/Majic_Gear/Stage/Single_Decks/Single_27_inch_Stage_Deck/StaticMeshes");
	case EStageDeckHeightPreset::In24:
	default:
		return TEXT("/Game/Majic_Gear/Stage/Single_Decks/Single_24_inch_Stage_Deck/StaticMeshes");
	}
}

FString AStageDeckActor::GetPodiumAssetFolder(EStagePodiumStyle InPodiumStyle) const
{
	switch (InPodiumStyle)
	{
	case EStagePodiumStyle::Acrylic:
		return TEXT("/Game/Majic_Gear/Stage/Updated_Podiums/Acrylic_Podium/StaticMeshes");
	case EStagePodiumStyle::LargeWood:
		return TEXT("/Game/Majic_Gear/Stage/Updated_Podiums/Large_Wood_Podium/StaticMeshes");
	case EStagePodiumStyle::Screen:
		return TEXT("/Game/Majic_Gear/Stage/Updated_Podiums/Screen_Podium/StaticMeshes");
	case EStagePodiumStyle::WhiteAcrylicFront:
		return TEXT("/Game/Majic_Gear/Stage/Updated_Podiums/White_Podium/StaticMeshes");
	case EStagePodiumStyle::None:
	default:
		return FString();
	}
}

FString AStageDeckActor::GetRailingAssetFolder(EStageRailingSpanType SpanType) const
{
	switch (SpanType)
	{
	case EStageRailingSpanType::In46:
		return TEXT("/Game/Majic_Gear/Stage/Railing/46_inch_Railing/StaticMeshes");
	case EStageRailingSpanType::In94:
	default:
		return TEXT("/Game/Majic_Gear/Stage/Railing/94_inch_Railing/StaticMeshes");
	}
}

TArray<FSoftObjectPath> AStageDeckActor::GetMeshPathsForFolder(const FString& AssetFolderPath) const
{
	TArray<FSoftObjectPath> MeshPaths;
	if (AssetFolderPath.IsEmpty())
	{
		return MeshPaths;
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> Assets;
	AssetRegistryModule.Get().GetAssetsByPath(*AssetFolderPath, Assets, false);

	Assets.StableSort([](const FAssetData& A, const FAssetData& B)
	{
		return A.AssetName.LexicalLess(B.AssetName);
	});

	for (const FAssetData& AssetData : Assets)
	{
		if (IsStaticMeshAsset(AssetData))
		{
			MeshPaths.Add(AssetData.ToSoftObjectPath());
		}
	}

	return MeshPaths;
}

TArray<FSoftObjectPath> AStageDeckActor::GetDeckMeshPaths(EStageDeckHeightPreset HeightPreset, EStageDeckSurfaceStyle SurfaceStyle) const
{
	return GetMeshPathsForFolder(GetDeckAssetFolder(HeightPreset, SurfaceStyle));
}

TArray<FSoftObjectPath> AStageDeckActor::GetPodiumMeshPaths() const
{
	return GetMeshPathsForFolder(GetPodiumAssetFolder(PodiumStyle));
}

TArray<FSoftObjectPath> AStageDeckActor::GetRailingMeshPaths(EStageRailingSpanType SpanType) const
{
	return GetMeshPathsForFolder(GetRailingAssetFolder(SpanType));
}

TArray<FSoftObjectPath> AStageDeckActor::GetStepMeshPaths(EStageDeckHeightPreset HeightPreset) const
{
	switch (HeightPreset)
	{
	case EStageDeckHeightPreset::In12:
		return GetMeshPathsForFolder(TEXT("/Game/Majic_Gear/Stage/Stageright_Steps/Stageright_2_Step/StaticMeshes"));
	case EStageDeckHeightPreset::In24:
		return GetMeshPathsForFolder(TEXT("/Game/Majic_Gear/Stage/Stageright_Steps/Stageright_3_Step/StaticMeshes"));
	case EStageDeckHeightPreset::In27:
		return GetMeshPathsForFolder(TEXT("/Game/Majic_Gear/Stage/Stageright_Steps/Stageright_4_Step/StaticMeshes"));
	case EStageDeckHeightPreset::In8:
	default:
		return {};
	}
}

UMaterialInterface* AStageDeckActor::ResolveDeckSurfaceMaterial(EStageDeckSurfaceStyle SurfaceStyle) const
{
	if (!DefaultSurfaceMaterial.IsNull())
	{
		return DefaultSurfaceMaterial.LoadSynchronous();
	}

	return nullptr;
}

UInstancedStaticMeshComponent* AStageDeckActor::FindOrCreateMeshBucket(const FSoftObjectPath& MeshPath, const TCHAR* Prefix, EStageDeckSurfaceStyle SurfaceStyle, TMap<FString, UInstancedStaticMeshComponent*>& BucketMap)
{
	const bool bIsDeckSurfaceMesh = MeshPath.GetAssetName().Contains(TEXT("Carpet"));
	const FString MaterialSuffix = bIsDeckSurfaceMesh && !DefaultSurfaceMaterial.IsNull()
		? FString::Printf(TEXT("_%s"), *DefaultSurfaceMaterial.ToSoftObjectPath().GetAssetName())
		: FString();
	const FString BucketKey = FString::Printf(TEXT("%s_%s%s"), Prefix, *MeshPath.ToString(), *MaterialSuffix);
	if (UInstancedStaticMeshComponent** ExistingComponent = BucketMap.Find(BucketKey))
	{
		return *ExistingComponent;
	}

	UStaticMesh* StaticMesh = Cast<UStaticMesh>(MeshPath.TryLoad());
	if (!StaticMesh)
	{
		return nullptr;
	}

	const uint32 PathHash = FCrc::StrCrc32(*MeshPath.ToString());
	const FName ComponentName(*FString::Printf(TEXT("%s_%s_%u"), Prefix, *StaticMesh->GetName(), PathHash));
	UInstancedStaticMeshComponent* MeshComponent = FindGeneratedMeshComponentByName(ComponentName);
	if (!MeshComponent)
	{
		MeshComponent = NewObject<UInstancedStaticMeshComponent>(this, ComponentName);
		if (!MeshComponent)
		{
			return nullptr;
		}

		MeshComponent->SetupAttachment(SceneRoot);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComponent->RegisterComponent();
		AddInstanceComponent(MeshComponent);
		GeneratedMeshComponents.Add(MeshComponent);
	}

	MeshComponent->SetStaticMesh(StaticMesh);
	MeshComponent->SetVisibility(true);
	MeshComponent->SetHiddenInGame(false);

	if (bIsDeckSurfaceMesh)
	{
		if (UMaterialInterface* OverrideMaterial = ResolveDeckSurfaceMaterial(SurfaceStyle))
		{
			MeshComponent->SetMaterial(0, OverrideMaterial);
		}
	}
	BucketMap.Add(BucketKey, MeshComponent);
	return MeshComponent;
}

void AStageDeckActor::ClearGeneratedComponents()
{
	TInlineComponentArray<UInstancedStaticMeshComponent*> TrackedInstanceComponents(this);
	for (UInstancedStaticMeshComponent* MeshComponent : TrackedInstanceComponents)
	{
		if (!MeshComponent)
		{
			continue;
		}

		if ((MeshComponent->GetName().StartsWith(StageDeckPrefix) ||
			MeshComponent->GetName().StartsWith(StagePodiumPrefix) ||
			MeshComponent->GetName().StartsWith(StageRailingPrefix) ||
			MeshComponent->GetName().StartsWith(StageStepPrefix)) &&
			!GeneratedMeshComponents.Contains(MeshComponent))
		{
			GeneratedMeshComponents.Add(MeshComponent);
		}
	}

	for (UInstancedStaticMeshComponent* MeshComponent : GeneratedMeshComponents)
	{
		if (!MeshComponent)
		{
			continue;
		}

		MeshComponent->ClearInstances();
		MeshComponent->SetVisibility(false);
		MeshComponent->SetHiddenInGame(true);
	}
}

UInstancedStaticMeshComponent* AStageDeckActor::FindGeneratedMeshComponentByName(const FName& ComponentName) const
{
	for (UInstancedStaticMeshComponent* MeshComponent : GeneratedMeshComponents)
	{
		if (MeshComponent && MeshComponent->GetFName() == ComponentName)
		{
			return MeshComponent;
		}
	}

	return FindObjectFast<UInstancedStaticMeshComponent>(const_cast<AStageDeckActor*>(this), ComponentName);
}

void AStageDeckActor::ClearGeneratedDrapes()
{
	TInlineComponentArray<UStaticMeshComponent*> TrackedDrapes(this);
	for (UStaticMeshComponent* DrapeComponent : TrackedDrapes)
	{
		if (!DrapeComponent)
		{
			continue;
		}

		if (DrapeComponent->GetName().StartsWith(StageDrapePrefix) && !GeneratedDrapeComponents.Contains(DrapeComponent))
		{
			GeneratedDrapeComponents.Add(DrapeComponent);
		}
	}

	for (UStaticMeshComponent* DrapeComponent : GeneratedDrapeComponents)
	{
		if (DrapeComponent)
		{
			DrapeComponent->DestroyComponent();
		}
	}

	GeneratedDrapeComponents.Reset();
}

void AStageDeckActor::ClearGeneratedLabels()
{
	TInlineComponentArray<UTextRenderComponent*> TrackedLabels(this);
	for (UTextRenderComponent* LabelComponent : TrackedLabels)
	{
		if (!LabelComponent)
		{
			continue;
		}

		if (LabelComponent->GetName().StartsWith(TEXT("StageCellLabel")) && !GeneratedLabelComponents.Contains(LabelComponent))
		{
			GeneratedLabelComponents.Add(LabelComponent);
		}
	}

	for (UTextRenderComponent* LabelComponent : GeneratedLabelComponents)
	{
		if (LabelComponent)
		{
			LabelComponent->DestroyComponent();
		}
	}

	GeneratedLabelComponents.Reset();
}

void AStageDeckActor::AddDeckCellInstance(const FSoftObjectPath& MeshPath, EStageDeckSurfaceStyle SurfaceStyle, const FVector& CellCenter, TMap<FString, UInstancedStaticMeshComponent*>& BucketMap, FBox& Bounds)
{
	UInstancedStaticMeshComponent* MeshComponent = FindOrCreateMeshBucket(MeshPath, StageDeckPrefix, SurfaceStyle, BucketMap);
	if (!MeshComponent || !MeshComponent->GetStaticMesh())
	{
		return;
	}

	const UStaticMesh* StaticMesh = MeshComponent->GetStaticMesh();
	const FBoxSphereBounds MeshBounds = StaticMesh->GetBounds();
	const FVector InstanceLocation = CellCenter + DeckPlacementOffsetCm;
	const FTransform InstanceTransform(DeckPlacementRotation, InstanceLocation, FVector::OneVector);
	MeshComponent->AddInstance(InstanceTransform);

	const FVector MeshCenter = InstanceTransform.TransformPosition(MeshBounds.Origin);
	Bounds += FBox(MeshCenter - MeshBounds.BoxExtent, MeshCenter + MeshBounds.BoxExtent);
}

void AStageDeckActor::AddCellLabel(int32 RowIndex, int32 ColumnIndex, const FVector& CellCenter, bool bCellEnabled)
{
	const int32 CellIndex = GetCellLinearIndex(RowIndex, ColumnIndex);
	UTextRenderComponent* LabelComponent = NewObject<UTextRenderComponent>(this, *FString::Printf(TEXT("StageCellLabel_%d_%d"), RowIndex, ColumnIndex));
	if (!LabelComponent)
	{
		return;
	}

	LabelComponent->SetupAttachment(SceneRoot);
	LabelComponent->SetHorizontalAlignment(EHTA_Center);
	LabelComponent->SetVerticalAlignment(EVRTA_TextCenter);
	LabelComponent->SetWorldSize(32.0f);
	LabelComponent->SetTextRenderColor(bCellEnabled ? FColor::White : FColor::Red);
	LabelComponent->SetText(FText::FromString(FString::Printf(TEXT("Index %d"), CellIndex)));
	LabelComponent->SetRelativeLocation(CellCenter + FVector(0.0f, 0.0f, CellLabelHeightCm));
	LabelComponent->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	LabelComponent->RegisterComponent();
	AddInstanceComponent(LabelComponent);
	GeneratedLabelComponents.Add(LabelComponent);
}

void AStageDeckActor::AddPodiumInstances(TMap<FString, UInstancedStaticMeshComponent*>& BucketMap, FBox& Bounds)
{
	float PodiumBaseHeightCm = 0.0f;
	for (const FStageDeckCell& Cell : DeckCells)
	{
		if (Cell.bEnabled)
		{
			PodiumBaseHeightCm = FMath::Max(PodiumBaseHeightCm, GetDeckHeightCm(Cell.HeightPreset));
		}
	}

	for (const FSoftObjectPath& MeshPath : GetPodiumMeshPaths())
	{
		UInstancedStaticMeshComponent* MeshComponent = FindOrCreateMeshBucket(MeshPath, StagePodiumPrefix, DefaultSurfaceStyle, BucketMap);
		if (!MeshComponent || !MeshComponent->GetStaticMesh())
		{
			continue;
		}

		const UStaticMesh* StaticMesh = MeshComponent->GetStaticMesh();
		const FBoxSphereBounds MeshBounds = StaticMesh->GetBounds();
		const FVector InstanceLocation(
			PodiumOffsetXCm,
			PodiumOffsetYCm,
			PodiumBaseHeightCm + PodiumOffsetZCm);
		const FTransform InstanceTransform(FRotator(0.0f, PodiumYawDegrees, 0.0f), InstanceLocation, FVector::OneVector);
		MeshComponent->AddInstance(InstanceTransform);
		const FVector MeshCenter = InstanceTransform.TransformPosition(MeshBounds.Origin);
		Bounds += FBox(MeshCenter - MeshBounds.BoxExtent, MeshCenter + MeshBounds.BoxExtent);
	}
}

void AStageDeckActor::AddStepInstances(TMap<FString, UInstancedStaticMeshComponent*>& BucketMap, FBox& Bounds)
{
	const float StepX = CellWidthCm + HorizontalSpacingCm;
	const float StepY = CellDepthCm + VerticalSpacingCm;
	const float OriginX = bCenterOnActor ? (-0.5f * (Columns - 1) * StepX) : 0.0f;
	const float OriginY = bCenterOnActor ? (-0.5f * (Rows - 1) * StepY) : 0.0f;

	for (int32 RowIndex = 0; RowIndex < Rows; ++RowIndex)
	{
		for (int32 ColumnIndex = 0; ColumnIndex < Columns; ++ColumnIndex)
		{
			const int32 CellIndex = GetCellLinearIndex(RowIndex, ColumnIndex);
			if (!DeckCells.IsValidIndex(CellIndex) || !DeckCells[CellIndex].bEnabled)
			{
				continue;
			}

			const bool bPlaceLeftStep = ShouldPlaceLeftStepAtCell(RowIndex, ColumnIndex);
			const bool bPlaceRightStep = ShouldPlaceRightStepAtCell(RowIndex, ColumnIndex);
			if (!bPlaceLeftStep && !bPlaceRightStep)
			{
				continue;
			}

			const FVector CellCenter(
				OriginX + (ColumnIndex * StepX),
				OriginY + (RowIndex * StepY),
				0.0f);
			const EStageDeckHeightPreset HeightPreset = DeckCells[CellIndex].HeightPreset;

			for (const FSoftObjectPath& MeshPath : GetStepMeshPaths(HeightPreset))
			{
				UInstancedStaticMeshComponent* MeshComponent = FindOrCreateMeshBucket(MeshPath, StageStepPrefix, DefaultSurfaceStyle, BucketMap);
				if (!MeshComponent || !MeshComponent->GetStaticMesh())
				{
					continue;
				}

				const UStaticMesh* StaticMesh = MeshComponent->GetStaticMesh();
				const FBoxSphereBounds MeshBounds = StaticMesh->GetBounds();

				if (bPlaceLeftStep)
				{
					const FVector InstanceLocation = CellCenter + DeckPlacementOffsetCm + LeftStepOffsetCm;
					const FTransform InstanceTransform(LeftStepRotation, InstanceLocation, FVector::OneVector);
					MeshComponent->AddInstance(InstanceTransform);
					const FVector MeshCenter = InstanceTransform.TransformPosition(MeshBounds.Origin);
					Bounds += FBox(MeshCenter - MeshBounds.BoxExtent, MeshCenter + MeshBounds.BoxExtent);
				}

				if (bPlaceRightStep)
				{
					const FVector InstanceLocation = CellCenter + DeckPlacementOffsetCm + RightStepOffsetCm;
					const FTransform InstanceTransform(RightStepRotation, InstanceLocation, FVector::OneVector);
					MeshComponent->AddInstance(InstanceTransform);
					const FVector MeshCenter = InstanceTransform.TransformPosition(MeshBounds.Origin);
					Bounds += FBox(MeshCenter - MeshBounds.BoxExtent, MeshCenter + MeshBounds.BoxExtent);
				}
			}
		}
	}
}

void AStageDeckActor::AddRailingSpan(EStageRailingSpanType SpanType, const FVector& SpanCenter, const FRotator& SpanRotation, const FVector& EdgeOffset, TMap<FString, UInstancedStaticMeshComponent*>& BucketMap, FBox& Bounds)
{
	const FVector SpanAdjustment = SpanType == EStageRailingSpanType::In94 ? Railing94SpanAdjustmentCm : Railing46SpanAdjustmentCm;
	for (const FSoftObjectPath& MeshPath : GetRailingMeshPaths(SpanType))
	{
		UInstancedStaticMeshComponent* MeshComponent = FindOrCreateMeshBucket(MeshPath, StageRailingPrefix, DefaultSurfaceStyle, BucketMap);
		if (!MeshComponent || !MeshComponent->GetStaticMesh())
		{
			continue;
		}

		const UStaticMesh* StaticMesh = MeshComponent->GetStaticMesh();
		const FBoxSphereBounds MeshBounds = StaticMesh->GetBounds();
		const FTransform InstanceTransform(
			SpanRotation + RailingPlacementRotation,
			SpanCenter + SpanRotation.RotateVector(EdgeOffset + SpanAdjustment),
			FVector::OneVector);
		MeshComponent->AddInstance(InstanceTransform);
		const FVector MeshCenter = InstanceTransform.TransformPosition(MeshBounds.Origin);
		Bounds += FBox(MeshCenter - MeshBounds.BoxExtent, MeshCenter + MeshBounds.BoxExtent);
	}
}

void AStageDeckActor::AddRailingRun(float TotalLengthCm, const FVector& StartLocation, const FVector& AlongDirection, const FRotator& SpanRotation, const FVector& EdgeOffset, TMap<FString, UInstancedStaticMeshComponent*>& BucketMap, FBox& Bounds)
{
	float RemainingLengthCm = TotalLengthCm;
	FVector CursorLocation = StartLocation;

	while (RemainingLengthCm >= (RailingSpan94Cm - KINDA_SMALL_NUMBER))
	{
		const FVector SpanCenter = CursorLocation + (AlongDirection * (0.5f * RailingSpan94Cm));
		AddRailingSpan(EStageRailingSpanType::In94, SpanCenter, SpanRotation, EdgeOffset, BucketMap, Bounds);
		CursorLocation += AlongDirection * RailingSpan94Cm;
		RemainingLengthCm -= RailingSpan94Cm;
	}

	while (RemainingLengthCm >= (RailingSpan46Cm - KINDA_SMALL_NUMBER))
	{
		const FVector SpanCenter = CursorLocation + (AlongDirection * (0.5f * RailingSpan46Cm));
		AddRailingSpan(EStageRailingSpanType::In46, SpanCenter, SpanRotation, EdgeOffset, BucketMap, Bounds);
		CursorLocation += AlongDirection * RailingSpan46Cm;
		RemainingLengthCm -= RailingSpan46Cm;
	}
}

void AStageDeckActor::AddRailingsForPerimeter(const float OriginX, const float OriginY, const float StepX, const float StepY, TMap<FString, UInstancedStaticMeshComponent*>& BucketMap, FBox& Bounds)
{
	auto IsCellEnabled = [this](int32 RowIndex, int32 ColumnIndex) -> bool
	{
		return RowIndex >= 0 &&
			RowIndex < Rows &&
			ColumnIndex >= 0 &&
			ColumnIndex < Columns &&
			DeckCells.IsValidIndex(GetCellLinearIndex(RowIndex, ColumnIndex)) &&
			DeckCells[GetCellLinearIndex(RowIndex, ColumnIndex)].bEnabled;
	};

	auto GetCellHeightPreset = [this](int32 RowIndex, int32 ColumnIndex) -> EStageDeckHeightPreset
	{
		return DeckCells[GetCellLinearIndex(RowIndex, ColumnIndex)].HeightPreset;
	};

	auto GetHeightAdjustedOffset = [this](const FVector& BaseOffset, EStageDeckHeightPreset HeightPreset) -> FVector
	{
		FVector AdjustedOffset = BaseOffset;
		AdjustedOffset.Z += GetRailingHeightDeltaZCm(HeightPreset);
		return AdjustedOffset;
	};

	if (bEnableFrontRailing)
	{
		for (int32 RowIndex = 0; RowIndex < Rows; ++RowIndex)
		{
			int32 ColumnIndex = 0;
			while (ColumnIndex < Columns)
			{
				const bool bFrontExposed = IsCellEnabled(RowIndex, ColumnIndex) &&
					!IsCellEnabled(RowIndex - 1, ColumnIndex);
				if (!bFrontExposed)
				{
					++ColumnIndex;
					continue;
				}

				const int32 StartColumn = ColumnIndex;
				const EStageDeckHeightPreset RunHeightPreset = GetCellHeightPreset(RowIndex, ColumnIndex);
				while (ColumnIndex < Columns &&
					IsCellEnabled(RowIndex, ColumnIndex) &&
					!IsCellEnabled(RowIndex - 1, ColumnIndex) &&
					GetCellHeightPreset(RowIndex, ColumnIndex) == RunHeightPreset)
				{
					++ColumnIndex;
				}

				const float RunLengthCm = static_cast<float>(ColumnIndex - StartColumn) * CellWidthCm;
				const FVector RunStart(
					OriginX + (StartColumn * StepX) - (0.5f * CellWidthCm),
					OriginY + (RowIndex * StepY) - (0.5f * CellDepthCm),
					0.0f);
				AddRailingRun(RunLengthCm, RunStart, FVector::ForwardVector, FRotator::ZeroRotator, GetHeightAdjustedOffset(FrontRailingOffsetCm, RunHeightPreset), BucketMap, Bounds);
			}
		}
	}

	if (bEnableBackRailing)
	{
		for (int32 RowIndex = 0; RowIndex < Rows; ++RowIndex)
		{
			int32 ColumnIndex = 0;
			while (ColumnIndex < Columns)
			{
				const bool bBackExposed = IsCellEnabled(RowIndex, ColumnIndex) &&
					!IsCellEnabled(RowIndex + 1, ColumnIndex);
				if (!bBackExposed)
				{
					++ColumnIndex;
					continue;
				}

				const int32 StartColumn = ColumnIndex;
				const EStageDeckHeightPreset RunHeightPreset = GetCellHeightPreset(RowIndex, ColumnIndex);
				while (ColumnIndex < Columns &&
					IsCellEnabled(RowIndex, ColumnIndex) &&
					!IsCellEnabled(RowIndex + 1, ColumnIndex) &&
					GetCellHeightPreset(RowIndex, ColumnIndex) == RunHeightPreset)
				{
					++ColumnIndex;
				}

				const float RunLengthCm = static_cast<float>(ColumnIndex - StartColumn) * CellWidthCm;
				const FVector RunStart(
					OriginX + (StartColumn * StepX) - (0.5f * CellWidthCm),
					OriginY + (RowIndex * StepY) + (0.5f * CellDepthCm),
					0.0f);
				AddRailingRun(RunLengthCm, RunStart, FVector::ForwardVector, FRotator(0.0f, 180.0f, 0.0f), GetHeightAdjustedOffset(BackRailingOffsetCm, RunHeightPreset), BucketMap, Bounds);
			}
		}
	}

	if (bEnableLeftRailing)
	{
		for (int32 ColumnIndex = 0; ColumnIndex < Columns; ++ColumnIndex)
		{
			int32 RowIndex = 0;
			while (RowIndex < Rows)
			{
				const bool bLeftExposed = IsCellEnabled(RowIndex, ColumnIndex) &&
					!IsCellEnabled(RowIndex, ColumnIndex - 1) &&
					!ShouldPlaceLeftStepAtCell(RowIndex, ColumnIndex);
				if (!bLeftExposed)
				{
					++RowIndex;
					continue;
				}

				const int32 StartRow = RowIndex;
				const EStageDeckHeightPreset RunHeightPreset = GetCellHeightPreset(RowIndex, ColumnIndex);
				while (RowIndex < Rows &&
					IsCellEnabled(RowIndex, ColumnIndex) &&
					!IsCellEnabled(RowIndex, ColumnIndex - 1) &&
					!ShouldPlaceLeftStepAtCell(RowIndex, ColumnIndex) &&
					GetCellHeightPreset(RowIndex, ColumnIndex) == RunHeightPreset)
				{
					++RowIndex;
				}

				const float RunLengthCm = static_cast<float>(RowIndex - StartRow) * CellDepthCm;
				const FVector RunStart(
					OriginX + (ColumnIndex * StepX) - (0.5f * CellWidthCm),
					OriginY + (StartRow * StepY) - (0.5f * CellDepthCm),
					0.0f);
				AddRailingRun(RunLengthCm, RunStart, FVector::RightVector, FRotator(0.0f, -90.0f, 0.0f), GetHeightAdjustedOffset(LeftRailingOffsetCm, RunHeightPreset), BucketMap, Bounds);
			}
		}
	}

	if (bEnableRightRailing)
	{
		for (int32 ColumnIndex = 0; ColumnIndex < Columns; ++ColumnIndex)
		{
			int32 RowIndex = 0;
			while (RowIndex < Rows)
			{
				const bool bRightExposed = IsCellEnabled(RowIndex, ColumnIndex) &&
					!IsCellEnabled(RowIndex, ColumnIndex + 1) &&
					!ShouldPlaceRightStepAtCell(RowIndex, ColumnIndex);
				if (!bRightExposed)
				{
					++RowIndex;
					continue;
				}

				const int32 StartRow = RowIndex;
				const EStageDeckHeightPreset RunHeightPreset = GetCellHeightPreset(RowIndex, ColumnIndex);
				while (RowIndex < Rows &&
					IsCellEnabled(RowIndex, ColumnIndex) &&
					!IsCellEnabled(RowIndex, ColumnIndex + 1) &&
					!ShouldPlaceRightStepAtCell(RowIndex, ColumnIndex) &&
					GetCellHeightPreset(RowIndex, ColumnIndex) == RunHeightPreset)
				{
					++RowIndex;
				}

				const float RunLengthCm = static_cast<float>(RowIndex - StartRow) * CellDepthCm;
				const FVector RunStart(
					OriginX + (ColumnIndex * StepX) + (0.5f * CellWidthCm),
					OriginY + (StartRow * StepY) - (0.5f * CellDepthCm),
					0.0f);
				AddRailingRun(RunLengthCm, RunStart, FVector::RightVector, FRotator(0.0f, 90.0f, 0.0f), GetHeightAdjustedOffset(RightRailingOffsetCm, RunHeightPreset), BucketMap, Bounds);
			}
		}
	}
}

void AStageDeckActor::AddDrapeForEdge(const FVector& EdgeCenter, const FRotator& EdgeRotation, float EdgeLengthCm, float DeckHeightCm, FBox& Bounds, const FVector& ExtraScale)
{
	UStaticMesh* DrapeMesh = LoadObject<UStaticMesh>(nullptr, *GetDrapeMeshPath());
	if (!DrapeMesh)
	{
		return;
	}

	UStaticMeshComponent* DrapeComponent = NewObject<UStaticMeshComponent>(this, *FString::Printf(TEXT("%s_%d"), StageDrapePrefix, GeneratedDrapeComponents.Num()));
	if (!DrapeComponent)
	{
		return;
	}

	DrapeComponent->SetupAttachment(SceneRoot);
	DrapeComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DrapeComponent->SetStaticMesh(DrapeMesh);
	DrapeComponent->RegisterComponent();
	AddInstanceComponent(DrapeComponent);
	GeneratedDrapeComponents.Add(DrapeComponent);

	const FBoxSphereBounds MeshBounds = DrapeMesh->GetBounds();
	const FVector NativeSize = MeshBounds.BoxExtent * 2.0f;
	const bool bWidthUsesX = NativeSize.X >= NativeSize.Y;
	const float NativeWidth = bWidthUsesX ? NativeSize.X : NativeSize.Y;
	const float WidthScale = NativeWidth > KINDA_SMALL_NUMBER ? (EdgeLengthCm / NativeWidth) : 1.0f;
	const float HeightScale = NativeSize.Z > KINDA_SMALL_NUMBER ? (DeckHeightCm / NativeSize.Z) : 1.0f;
	FVector ComponentScale(1.0f, 1.0f, HeightScale);
	if (bWidthUsesX)
	{
		ComponentScale.X = WidthScale;
	}
	else
	{
		ComponentScale.Y = WidthScale;
	}
	ComponentScale.X *= ExtraScale.X;
	ComponentScale.Y *= ExtraScale.Y;
	ComponentScale.Z *= ExtraScale.Z;

	const float LocalMinZ = MeshBounds.Origin.Z - MeshBounds.BoxExtent.Z;
	const FVector DrapeLocation(
		EdgeCenter.X - (MeshBounds.Origin.X * ComponentScale.X),
		EdgeCenter.Y - (MeshBounds.Origin.Y * ComponentScale.Y),
		EdgeCenter.Z - (LocalMinZ * ComponentScale.Z));

	DrapeComponent->SetRelativeTransform(FTransform(EdgeRotation, DrapeLocation, ComponentScale));
	const FVector ScaledExtent(
		MeshBounds.BoxExtent.X * ComponentScale.X,
		MeshBounds.BoxExtent.Y * ComponentScale.Y,
		MeshBounds.BoxExtent.Z * ComponentScale.Z);
	Bounds += FBox(DrapeLocation - ScaledExtent, DrapeLocation + ScaledExtent);
}

bool AStageDeckActor::GetFirstEnabledCell(int32& OutRowIndex, int32& OutColumnIndex, FStageDeckCell& OutCell) const
{
	for (int32 RowIndex = 0; RowIndex < Rows; ++RowIndex)
	{
		for (int32 ColumnIndex = 0; ColumnIndex < Columns; ++ColumnIndex)
		{
			const int32 CellIndex = GetCellLinearIndex(RowIndex, ColumnIndex);
			if (!DeckCells.IsValidIndex(CellIndex))
			{
				continue;
			}

			if (!DeckCells[CellIndex].bEnabled)
			{
				continue;
			}

			OutRowIndex = RowIndex;
			OutColumnIndex = ColumnIndex;
			OutCell = DeckCells[CellIndex];
			return true;
		}
	}

	return false;
}

bool AStageDeckActor::ShouldPlaceLeftStepAtCell(int32 RowIndex, int32 ColumnIndex) const
{
	if (!bEnableLeftStep || !IsValidCellIndexPair(RowIndex, ColumnIndex))
	{
		return false;
	}

	const int32 CellIndex = GetCellLinearIndex(RowIndex, ColumnIndex);
	if (!DeckCells[CellIndex].bEnabled || DeckCells[CellIndex].HeightPreset == EStageDeckHeightPreset::In8)
	{
		return false;
	}

	const bool bLeftExposed = (ColumnIndex == 0) || !IsValidCellIndexPair(RowIndex, ColumnIndex - 1) || !DeckCells[GetCellLinearIndex(RowIndex, ColumnIndex - 1)].bEnabled;
	if (!bLeftExposed)
	{
		return false;
	}

	for (int32 OtherRow = Rows - 1; OtherRow >= 0; --OtherRow)
	{
		for (int32 OtherColumn = 0; OtherColumn < Columns; ++OtherColumn)
		{
			if (!IsValidCellIndexPair(OtherRow, OtherColumn))
			{
				continue;
			}

			const int32 OtherCellIndex = GetCellLinearIndex(OtherRow, OtherColumn);
			if (!DeckCells[OtherCellIndex].bEnabled || DeckCells[OtherCellIndex].HeightPreset == EStageDeckHeightPreset::In8)
			{
				continue;
			}

			const bool bOtherLeftExposed = (OtherColumn == 0) || !IsValidCellIndexPair(OtherRow, OtherColumn - 1) || !DeckCells[GetCellLinearIndex(OtherRow, OtherColumn - 1)].bEnabled;
			if (bOtherLeftExposed)
			{
				return OtherRow == RowIndex && OtherColumn == ColumnIndex;
			}
		}
	}

	return false;
}

bool AStageDeckActor::ShouldPlaceRightStepAtCell(int32 RowIndex, int32 ColumnIndex) const
{
	if (!bEnableRightStep || !IsValidCellIndexPair(RowIndex, ColumnIndex))
	{
		return false;
	}

	const int32 CellIndex = GetCellLinearIndex(RowIndex, ColumnIndex);
	if (!DeckCells[CellIndex].bEnabled || DeckCells[CellIndex].HeightPreset == EStageDeckHeightPreset::In8)
	{
		return false;
	}

	const bool bRightExposed = (ColumnIndex == Columns - 1) || !IsValidCellIndexPair(RowIndex, ColumnIndex + 1) || !DeckCells[GetCellLinearIndex(RowIndex, ColumnIndex + 1)].bEnabled;
	if (!bRightExposed)
	{
		return false;
	}

	for (int32 OtherRow = Rows - 1; OtherRow >= 0; --OtherRow)
	{
		for (int32 OtherColumn = Columns - 1; OtherColumn >= 0; --OtherColumn)
		{
			if (!IsValidCellIndexPair(OtherRow, OtherColumn))
			{
				continue;
			}

			const int32 OtherCellIndex = GetCellLinearIndex(OtherRow, OtherColumn);
			if (!DeckCells[OtherCellIndex].bEnabled || DeckCells[OtherCellIndex].HeightPreset == EStageDeckHeightPreset::In8)
			{
				continue;
			}

			const bool bOtherRightExposed = (OtherColumn == Columns - 1) || !IsValidCellIndexPair(OtherRow, OtherColumn + 1) || !DeckCells[GetCellLinearIndex(OtherRow, OtherColumn + 1)].bEnabled;
			if (bOtherRightExposed)
			{
				return OtherRow == RowIndex && OtherColumn == ColumnIndex;
			}
		}
	}

	return false;
}

void AStageDeckActor::UpdateSelectionBounds(const FBox& Bounds)
{
	if (!SelectionBounds)
	{
		return;
	}

	if (!Bounds.IsValid)
	{
		SelectionBounds->SetBoxExtent(FVector(50.0f, 50.0f, 50.0f));
		SelectionBounds->SetRelativeLocation(FVector::ZeroVector);
		return;
	}

	SelectionBounds->SetBoxExtent(Bounds.GetExtent() + FVector(5.0f));
	SelectionBounds->SetRelativeLocation(Bounds.GetCenter());
}
