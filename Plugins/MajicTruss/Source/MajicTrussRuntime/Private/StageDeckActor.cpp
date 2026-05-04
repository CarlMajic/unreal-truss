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
		ApplyDefaultSettingsToAllCells();
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
		return TEXT("/Game/Majic_Gear/Stage/Podiums/Acrylic_Podium/StaticMeshes");
	case EStagePodiumStyle::LargeWood:
		return TEXT("/Game/Majic_Gear/Stage/Podiums/Large_Wood_Podium/StaticMeshes");
	case EStagePodiumStyle::Screen:
		return TEXT("/Game/Majic_Gear/Stage/Podiums/Screen_Podium/StaticMeshes");
	case EStagePodiumStyle::WhiteAcrylicFront:
		return TEXT("/Game/Majic_Gear/Stage/Podiums/White_Podium_with_Acrylic_Front/StaticMeshes");
	case EStagePodiumStyle::None:
	default:
		return FString();
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

		if ((MeshComponent->GetName().StartsWith(StageDeckPrefix) || MeshComponent->GetName().StartsWith(StagePodiumPrefix)) &&
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
		const float LocalMinZ = MeshBounds.Origin.Z - MeshBounds.BoxExtent.Z;
		const FVector InstanceLocation(
			PodiumOffsetXCm - MeshBounds.Origin.X,
			PodiumOffsetYCm - MeshBounds.Origin.Y,
			PodiumBaseHeightCm + PodiumOffsetZCm - LocalMinZ);
		const FTransform InstanceTransform(FRotator(0.0f, PodiumYawDegrees, 0.0f), InstanceLocation, FVector::OneVector);
		MeshComponent->AddInstance(InstanceTransform);
		Bounds += FBox(InstanceLocation - MeshBounds.BoxExtent, InstanceLocation + MeshBounds.BoxExtent);
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
