#include "MBPWallActor.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "UObject/UnrealType.h"
#include "Materials/MaterialInterface.h"
#include "Modules/ModuleManager.h"

namespace
{
constexpr float DefaultShimmerFaceOffsetXCm = -1.902981f;
constexpr float DefaultShimmerFaceOffsetYCm = -0.104f;
constexpr float DefaultShimmerFaceOffsetZCm = -4.650027f;

FString GetStyleAssetFolder(EMBPPanelStyle Style)
{
	switch (Style)
	{
	case EMBPPanelStyle::Empty:
	case EMBPPanelStyle::Custom:
		return FString();
	case EMBPPanelStyle::Acrylic:
		return TEXT("/Game/Majic_Gear/MBP/Acrylic_Segment/StaticMeshes");
	case EMBPPanelStyle::Boxwood:
		return TEXT("/Game/Majic_Gear/MBP/Boxwood_Segment/StaticMeshes");
	case EMBPPanelStyle::Drift:
		return TEXT("/Game/Majic_Gear/MBP/Drift_Segment/StaticMeshes");
	case EMBPPanelStyle::Geo:
		return TEXT("/Game/Majic_Gear/MBP/Geo_Segment/StaticMeshes");
	case EMBPPanelStyle::Shimmer:
		return TEXT("/Game/Majic_Gear/MBP/Shimmer_Segment/StaticMeshes");
	case EMBPPanelStyle::Hive:
		return TEXT("/Game/Majic_Gear/MBP/Hive_Segment/StaticMeshes");
	case EMBPPanelStyle::Platinum:
		return TEXT("/Game/Majic_Gear/MBP/Platinum_Segment/StaticMeshes");
	default:
		return FString();
	}
}

FString GetShimmerMaterialPath(EMBPShimmerVariant Variant)
{
	switch (Variant)
	{
	case EMBPShimmerVariant::Red:
		return TEXT("/Game/Majic_Gear/MBP/Shimmer_Segment/Materials/RedShimmerSquare.RedShimmerSquare");
	case EMBPShimmerVariant::Fuchsia:
		return TEXT("/Game/Majic_Gear/MBP/Shimmer_Segment/Materials/FushaShimmerSquare.FushaShimmerSquare");
	case EMBPShimmerVariant::Holographic:
		return TEXT("/Game/Majic_Gear/MBP/Shimmer_Segment/Materials/HolographicSquare_Mat1.HolographicSquare_Mat1");
	case EMBPShimmerVariant::Gold:
	default:
		return TEXT("/Game/Majic_Gear/MBP/Shimmer_Segment/Materials/GoldShimmerSquare_Mat.GoldShimmerSquare_Mat");
	}
}

bool IsStaticMeshAsset(const FAssetData& AssetData)
{
	return AssetData.AssetClassPath == UStaticMesh::StaticClass()->GetClassPathName();
}

FString GetExtraFrameAssetFolder()
{
	return TEXT("/Game/Majic_Gear/MBP/Extra_Frame/StaticMeshes");
}

constexpr int32 ShimmerPlaneColumns = 30;
constexpr int32 ShimmerPlaneRows = 30;
}

AMBPWallActor::AMBPWallActor()
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
	SelectionBounds->ShapeColor = FColor::Yellow;
}

void AMBPWallActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (FMath::IsNearlyZero(ShimmerFaceOffsetXCm) &&
		FMath::IsNearlyZero(ShimmerFaceOffsetYCm) &&
		FMath::IsNearlyZero(ShimmerFaceOffsetZCm))
	{
		ShimmerFaceOffsetXCm = DefaultShimmerFaceOffsetXCm;
		ShimmerFaceOffsetYCm = DefaultShimmerFaceOffsetYCm;
		ShimmerFaceOffsetZCm = DefaultShimmerFaceOffsetZCm;
	}

	EnsureSlotCount(false);
	SyncSlotsToDefaultStyleIfNeeded();
	if (bBuildOnConstruction)
	{
		RebuildWall();
	}
}

#if WITH_EDITOR
void AMBPWallActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AMBPWallActor, BatchTargetIndex) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(AMBPWallActor, BatchEditAxis) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(AMBPWallActor, BatchStyle) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(AMBPWallActor, BatchShimmerVariant) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(AMBPWallActor, BatchShimmerMaterial) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(AMBPWallActor, BatchDepthOffsetCm))
	{
		ApplyBatchEdit();
	}
}
#endif

void AMBPWallActor::RebuildWall()
{
	EnsureSlotCount(false);
	ClearGeneratedComponents();

	const float StepX = PanelWidthCm + HorizontalSpacingCm;
	const float StepZ = PanelHeightCm + VerticalSpacingCm;
	const float OriginX = bCenterOnActor ? (-0.5f * (Columns - 1) * StepX) : 0.0f;
	const float OriginZ = bCenterOnActor ? (-0.5f * (Rows - 1) * StepZ) : 0.0f;
	const FRotator PanelRotation(0.0f, 90.0f, 0.0f);

	TMap<FString, UInstancedStaticMeshComponent*> BucketMap;
	FBox Bounds(EForceInit::ForceInit);

	for (int32 RowIndex = 0; RowIndex < Rows; ++RowIndex)
	{
		for (int32 ColumnIndex = 0; ColumnIndex < Columns; ++ColumnIndex)
		{
			const int32 SlotIndex = (RowIndex * Columns) + ColumnIndex;
			if (!PanelSlots.IsValidIndex(SlotIndex))
			{
				continue;
			}

			const FMBPPanelSlot& Slot = PanelSlots[SlotIndex];
			const float SnappedDepthOffsetCm = GetSnappedDepthOffsetCm(Slot.DepthOffsetCm);
			const FVector SlotLocation(
				OriginX + (ColumnIndex * StepX),
				SnappedDepthOffsetCm,
				OriginZ + (RowIndex * StepZ));

			for (const FSoftObjectPath& MeshPath : GetMeshPathsForSlot(Slot))
			{
				UInstancedStaticMeshComponent* MeshComponent = FindOrCreateComponentBucket(MeshPath, Slot, BucketMap);
				if (!MeshComponent)
				{
					continue;
				}

				const FTransform InstanceTransform(PanelRotation, SlotLocation, FVector::OneVector);
				MeshComponent->AddInstance(InstanceTransform);

				if (const UStaticMesh* StaticMesh = MeshComponent->GetStaticMesh())
				{
					const FBoxSphereBounds MeshBounds = StaticMesh->GetBounds();
					const FVector Extent = MeshBounds.BoxExtent;
					Bounds += FBox(SlotLocation - Extent, SlotLocation + Extent);
				}
			}

			if (Slot.Style == EMBPPanelStyle::Shimmer)
			{
				const FSoftObjectPath PlaneMeshPath = GetShimmerPlaneMeshPath();
				if (UInstancedStaticMeshComponent* PlaneComponent = FindOrCreateComponentBucket(PlaneMeshPath, Slot, BucketMap))
				{
					if (const UStaticMesh* PlaneMesh = PlaneComponent->GetStaticMesh())
					{
						const FVector NativePlaneSize = PlaneMesh->GetBounds().BoxExtent * 2.0f;
						const float NativeTileSizeCm = FMath::Max3(NativePlaneSize.X, NativePlaneSize.Y, NativePlaneSize.Z);
						if (NativeTileSizeCm <= KINDA_SMALL_NUMBER)
						{
							continue;
						}

						const float TileWidthCm = PanelWidthCm / static_cast<float>(ShimmerPlaneColumns);
						const float TileHeightCm = PanelHeightCm / static_cast<float>(ShimmerPlaneRows);
						const float TileSizeCm = FMath::Min(TileWidthCm, TileHeightCm);
						const float TileScale = TileSizeCm / NativeTileSizeCm;
						const FVector ScaledPlaneExtent = PlaneMesh->GetBounds().BoxExtent * TileScale;
						const float PlaneStartX = SlotLocation.X + (0.5f * TileWidthCm);
						const float PlaneStartZ = SlotLocation.Z - (0.5f * TileHeightCm);

						for (int32 PlaneRow = 0; PlaneRow < ShimmerPlaneRows; ++PlaneRow)
						{
							for (int32 PlaneColumn = 0; PlaneColumn < ShimmerPlaneColumns; ++PlaneColumn)
							{
								const FVector PlaneLocation(
									PlaneStartX + (PlaneColumn * TileWidthCm) + ShimmerFaceOffsetXCm,
									SlotLocation.Y + ShimmerFaceOffsetYCm,
									PlaneStartZ - (PlaneRow * TileHeightCm) + ShimmerFaceOffsetZCm);
								const FTransform PlaneTransform(PanelRotation, PlaneLocation, FVector(TileScale));
								PlaneComponent->AddInstance(PlaneTransform);
								Bounds += FBox(PlaneLocation - ScaledPlaneExtent, PlaneLocation + ScaledPlaneExtent);
							}
						}
					}
				}
			}

			if (!FMath::IsNearlyZero(SnappedDepthOffsetCm))
			{
				for (const FSoftObjectPath& MeshPath : GetExtraFrameMeshPaths())
				{
					UInstancedStaticMeshComponent* MeshComponent = FindOrCreateComponentBucket(MeshPath, Slot, BucketMap);
					if (!MeshComponent)
					{
						continue;
					}

					const FTransform InstanceTransform(PanelRotation, SlotLocation, FVector::OneVector);
					MeshComponent->AddInstance(InstanceTransform);

					if (const UStaticMesh* StaticMesh = MeshComponent->GetStaticMesh())
					{
						const FBoxSphereBounds MeshBounds = StaticMesh->GetBounds();
						const FVector Extent = MeshBounds.BoxExtent;
						Bounds += FBox(SlotLocation - Extent, SlotLocation + Extent);
					}
				}
			}
		}
	}

	UpdateSelectionBounds(Bounds);
}

void AMBPWallActor::ApplyWallDefinition(const FMBPWallDefinition& Definition, bool bResetSlotsToDefault)
{
	Columns = FMath::Max(1, Definition.Columns);
	Rows = FMath::Max(1, Definition.Rows);
	DefaultStyle = Definition.DefaultStyle;
	DefaultShimmerVariant = Definition.DefaultShimmerVariant;
	DefaultShimmerMaterial = Definition.DefaultShimmerMaterial;
	ShimmerFaceOffsetXCm = Definition.ShimmerFaceOffsetXCm;
	ShimmerFaceOffsetYCm = Definition.ShimmerFaceOffsetYCm;
	ShimmerFaceOffsetZCm = Definition.ShimmerFaceOffsetZCm;
	PanelWidthCm = Definition.PanelWidthCm;
	PanelHeightCm = Definition.PanelHeightCm;
	HorizontalSpacingCm = Definition.HorizontalSpacingCm;
	VerticalSpacingCm = Definition.VerticalSpacingCm;
	DepthOffsetStepCm = Definition.DepthOffsetStepCm;
	bCenterOnActor = Definition.bCenterOnActor;

	if (bResetSlotsToDefault)
	{
		ResetSlotsToDefault();
	}
	else
	{
		EnsureSlotCount(false);
		RebuildWall();
	}
}

FMBPWallDefinition AMBPWallActor::GetWallDefinition() const
{
	FMBPWallDefinition Definition;
	Definition.Columns = Columns;
	Definition.Rows = Rows;
	Definition.DefaultStyle = DefaultStyle;
	Definition.DefaultShimmerVariant = DefaultShimmerVariant;
	Definition.DefaultShimmerMaterial = DefaultShimmerMaterial;
	Definition.ShimmerFaceOffsetXCm = ShimmerFaceOffsetXCm;
	Definition.ShimmerFaceOffsetYCm = ShimmerFaceOffsetYCm;
	Definition.ShimmerFaceOffsetZCm = ShimmerFaceOffsetZCm;
	Definition.PanelWidthCm = PanelWidthCm;
	Definition.PanelHeightCm = PanelHeightCm;
	Definition.HorizontalSpacingCm = HorizontalSpacingCm;
	Definition.VerticalSpacingCm = VerticalSpacingCm;
	Definition.DepthOffsetStepCm = DepthOffsetStepCm;
	Definition.bCenterOnActor = bCenterOnActor;
	return Definition;
}

bool AMBPWallActor::GetSlotIndicesFromWorldLocation(const FVector& WorldLocation, int32& OutRow, int32& OutColumn) const
{
	const float StepX = PanelWidthCm + HorizontalSpacingCm;
	const float StepZ = PanelHeightCm + VerticalSpacingCm;
	if (StepX <= KINDA_SMALL_NUMBER || StepZ <= KINDA_SMALL_NUMBER || Columns <= 0 || Rows <= 0)
	{
		return false;
	}

	const float OriginX = bCenterOnActor ? (-0.5f * (Columns - 1) * StepX) : 0.0f;
	const float OriginZ = bCenterOnActor ? (-0.5f * (Rows - 1) * StepZ) : 0.0f;
	const FVector LocalHitLocation = GetActorTransform().InverseTransformPosition(WorldLocation);

	OutColumn = FMath::Clamp(FMath::RoundToInt((LocalHitLocation.X - OriginX) / StepX), 0, Columns - 1);
	OutRow = FMath::Clamp(FMath::RoundToInt((LocalHitLocation.Z - OriginZ) / StepZ), 0, Rows - 1);
	return IsValidSlotIndexPair(OutRow, OutColumn);
}

bool AMBPWallActor::GetPanelSlot(int32 RowIndex, int32 ColumnIndex, FMBPPanelSlot& OutSlot) const
{
	if (!IsValidSlotIndexPair(RowIndex, ColumnIndex))
	{
		return false;
	}

	OutSlot = PanelSlots[GetSlotLinearIndex(RowIndex, ColumnIndex)];
	return true;
}

bool AMBPWallActor::ApplyPanelEdit(int32 RowIndex, int32 ColumnIndex, EMBPPanelStyle Style, float DepthOffsetCm)
{
	if (!IsValidSlotIndexPair(RowIndex, ColumnIndex))
	{
		return false;
	}

	return ApplyEditToSlotIndices({ GetSlotLinearIndex(RowIndex, ColumnIndex) }, Style, DepthOffsetCm);
}

bool AMBPWallActor::ApplyRowEditByIndex(int32 RowIndex, EMBPPanelStyle Style, float DepthOffsetCm)
{
	if (RowIndex < 0 || RowIndex >= Rows)
	{
		return false;
	}

	TArray<int32> SlotIndices;
	for (int32 ColumnIndex = 0; ColumnIndex < Columns; ++ColumnIndex)
	{
		SlotIndices.Add(GetSlotLinearIndex(RowIndex, ColumnIndex));
	}

	return ApplyEditToSlotIndices(SlotIndices, Style, DepthOffsetCm);
}

bool AMBPWallActor::ApplyColumnEditByIndex(int32 ColumnIndex, EMBPPanelStyle Style, float DepthOffsetCm)
{
	if (ColumnIndex < 0 || ColumnIndex >= Columns)
	{
		return false;
	}

	TArray<int32> SlotIndices;
	for (int32 RowIndex = 0; RowIndex < Rows; ++RowIndex)
	{
		SlotIndices.Add(GetSlotLinearIndex(RowIndex, ColumnIndex));
	}

	return ApplyEditToSlotIndices(SlotIndices, Style, DepthOffsetCm);
}

void AMBPWallActor::SetSelectionHighlighted(bool bHighlighted)
{
	if (SelectionBounds)
	{
		SelectionBounds->SetHiddenInGame(!bHighlighted);
	}
}

void AMBPWallActor::ResetSlotsToDefault()
{
	EnsureSlotCount(true);
	RebuildWall();
}

void AMBPWallActor::ResizeSlotsToGrid()
{
	EnsureSlotCount(false);
	RebuildWall();
}

void AMBPWallActor::ApplyBatchEdit()
{
	for (const int32 SlotIndex : GetBatchSlotIndices())
	{
		if (!PanelSlots.IsValidIndex(SlotIndex))
		{
			continue;
		}

		FMBPPanelSlot& Slot = PanelSlots[SlotIndex];
		Slot.Style = BatchStyle;
		Slot.ShimmerVariant = BatchShimmerVariant;
		Slot.ShimmerMaterial = BatchShimmerMaterial;
		Slot.DepthOffsetCm = BatchDepthOffsetCm;
		if (BatchStyle != EMBPPanelStyle::Custom)
		{
			Slot.CustomStaticMeshes.Reset();
		}
	}

	CachedDefaultStyle = DefaultStyle;
	CachedDefaultShimmerVariant = DefaultShimmerVariant;
	CachedDefaultShimmerMaterial = DefaultShimmerMaterial;
	bHasCachedDefaultStyle = true;
	RebuildWall();
}

void AMBPWallActor::EnsureSlotCount(bool bResetNewSlotsToDefault)
{
	const int32 TargetCount = FMath::Max(1, Columns) * FMath::Max(1, Rows);
	const int32 PreviousCount = PanelSlots.Num();
	PanelSlots.SetNum(TargetCount);

	for (int32 Index = 0; Index < TargetCount; ++Index)
	{
		if (bResetNewSlotsToDefault || Index >= PreviousCount)
		{
			PanelSlots[Index] = MakeDefaultSlot();
		}
	}
}

FMBPPanelSlot AMBPWallActor::MakeDefaultSlot() const
{
	FMBPPanelSlot Slot;
	Slot.Style = DefaultStyle;
	Slot.ShimmerVariant = DefaultShimmerVariant;
	Slot.ShimmerMaterial = DefaultShimmerMaterial;
	return Slot;
}

void AMBPWallActor::SyncSlotsToDefaultStyleIfNeeded()
{
	if (!bHasCachedDefaultStyle)
	{
		ApplyDefaultStyleToAllSlots();
		bHasCachedDefaultStyle = true;
		CachedDefaultStyle = DefaultStyle;
		CachedDefaultShimmerVariant = DefaultShimmerVariant;
		CachedDefaultShimmerMaterial = DefaultShimmerMaterial;
		return;
	}

	if (CachedDefaultStyle == DefaultStyle &&
		CachedDefaultShimmerVariant == DefaultShimmerVariant &&
		CachedDefaultShimmerMaterial == DefaultShimmerMaterial)
	{
		return;
	}

	bool bSlotsStillMatchCachedDefaults = true;
	for (const FMBPPanelSlot& Slot : PanelSlots)
	{
		if (Slot.Style != CachedDefaultStyle)
		{
			bSlotsStillMatchCachedDefaults = false;
			break;
		}

		if (CachedDefaultStyle == EMBPPanelStyle::Shimmer && Slot.ShimmerVariant != CachedDefaultShimmerVariant)
		{
			bSlotsStillMatchCachedDefaults = false;
			break;
		}

		if (CachedDefaultStyle == EMBPPanelStyle::Shimmer && Slot.ShimmerMaterial != CachedDefaultShimmerMaterial)
		{
			bSlotsStillMatchCachedDefaults = false;
			break;
		}
	}

	if (bSlotsStillMatchCachedDefaults)
	{
		ApplyDefaultStyleToAllSlots();
	}

	CachedDefaultStyle = DefaultStyle;
	CachedDefaultShimmerVariant = DefaultShimmerVariant;
	CachedDefaultShimmerMaterial = DefaultShimmerMaterial;
}

void AMBPWallActor::ApplyDefaultStyleToAllSlots()
{
	for (FMBPPanelSlot& Slot : PanelSlots)
	{
		Slot.Style = DefaultStyle;
		Slot.ShimmerVariant = DefaultShimmerVariant;
		Slot.ShimmerMaterial = DefaultShimmerMaterial;
	}
}

TArray<int32> AMBPWallActor::GetBatchSlotIndices() const
{
	TArray<int32> SlotIndices;

	if (BatchEditAxis == EMBPBatchEditAxis::Row)
	{
		if (BatchTargetIndex < 0 || BatchTargetIndex >= Rows)
		{
			return SlotIndices;
		}

		for (int32 ColumnIndex = 0; ColumnIndex < Columns; ++ColumnIndex)
		{
			SlotIndices.Add((BatchTargetIndex * Columns) + ColumnIndex);
		}

		return SlotIndices;
	}

	if (BatchTargetIndex < 0 || BatchTargetIndex >= Columns)
	{
		return SlotIndices;
	}

	for (int32 RowIndex = 0; RowIndex < Rows; ++RowIndex)
	{
		SlotIndices.Add((RowIndex * Columns) + BatchTargetIndex);
	}

	return SlotIndices;
}

bool AMBPWallActor::IsValidSlotIndexPair(int32 RowIndex, int32 ColumnIndex) const
{
	return RowIndex >= 0 && RowIndex < Rows && ColumnIndex >= 0 && ColumnIndex < Columns && PanelSlots.IsValidIndex(GetSlotLinearIndex(RowIndex, ColumnIndex));
}

int32 AMBPWallActor::GetSlotLinearIndex(int32 RowIndex, int32 ColumnIndex) const
{
	return (RowIndex * Columns) + ColumnIndex;
}

void AMBPWallActor::ApplyStyleAndDepthToSlot(FMBPPanelSlot& Slot, EMBPPanelStyle Style, float DepthOffsetCm)
{
	Slot.Style = Style;
	Slot.DepthOffsetCm = DepthOffsetCm;

	if (Style == EMBPPanelStyle::Shimmer)
	{
		Slot.ShimmerVariant = DefaultShimmerVariant;
		if (Slot.ShimmerMaterial.IsNull())
		{
			Slot.ShimmerMaterial = DefaultShimmerMaterial;
		}
	}

	if (Style != EMBPPanelStyle::Custom)
	{
		Slot.CustomStaticMeshes.Reset();
	}
}

bool AMBPWallActor::ApplyEditToSlotIndices(const TArray<int32>& SlotIndices, EMBPPanelStyle Style, float DepthOffsetCm)
{
	bool bChanged = false;
	for (const int32 SlotIndex : SlotIndices)
	{
		if (!PanelSlots.IsValidIndex(SlotIndex))
		{
			continue;
		}

		ApplyStyleAndDepthToSlot(PanelSlots[SlotIndex], Style, DepthOffsetCm);
		bChanged = true;
	}

	if (bChanged)
	{
		RebuildWall();
	}

	return bChanged;
}

float AMBPWallActor::GetSnappedDepthOffsetCm(float RawDepthOffsetCm) const
{
	if (DepthOffsetStepCm <= KINDA_SMALL_NUMBER)
	{
		return RawDepthOffsetCm;
	}

	return FMath::RoundToFloat(RawDepthOffsetCm / DepthOffsetStepCm) * DepthOffsetStepCm;
}

TArray<FSoftObjectPath> AMBPWallActor::GetMeshPathsForFolder(const FString& AssetFolderPath) const
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

TArray<FSoftObjectPath> AMBPWallActor::GetMeshPathsForStyle(EMBPPanelStyle Style) const
{
	return GetMeshPathsForFolder(GetStyleAssetFolder(Style));
}

TArray<FSoftObjectPath> AMBPWallActor::GetMeshPathsForSlot(const FMBPPanelSlot& Slot) const
{
	if (Slot.Style == EMBPPanelStyle::Empty)
	{
		return {};
	}

	if (Slot.Style == EMBPPanelStyle::Custom)
	{
		TArray<FSoftObjectPath> MeshPaths;
		for (const TSoftObjectPtr<UStaticMesh>& Mesh : Slot.CustomStaticMeshes)
		{
			if (!Mesh.IsNull())
			{
				MeshPaths.Add(Mesh.ToSoftObjectPath());
			}
		}

		return MeshPaths;
	}

	if (Slot.Style == EMBPPanelStyle::Shimmer)
	{
		return GetShimmerFrameMeshPaths();
	}

	return GetMeshPathsForStyle(Slot.Style);
}

TArray<FSoftObjectPath> AMBPWallActor::GetShimmerFrameMeshPaths() const
{
	TArray<FSoftObjectPath> MeshPaths;
	for (const FSoftObjectPath& MeshPath : GetMeshPathsForStyle(EMBPPanelStyle::Shimmer))
	{
		const FString AssetName = MeshPath.GetAssetName();
		if (AssetName.Equals(TEXT("Plane")) || AssetName.StartsWith(TEXT("Shimmer__")))
		{
			continue;
		}

		MeshPaths.Add(MeshPath);
	}

	return MeshPaths;
}

FSoftObjectPath AMBPWallActor::GetShimmerPlaneMeshPath() const
{
	for (const FSoftObjectPath& MeshPath : GetMeshPathsForStyle(EMBPPanelStyle::Shimmer))
	{
		if (MeshPath.GetAssetName().Equals(TEXT("Plane")))
		{
			return MeshPath;
		}
	}

	return FSoftObjectPath();
}

TArray<FSoftObjectPath> AMBPWallActor::GetExtraFrameMeshPaths() const
{
	return GetMeshPathsForFolder(GetExtraFrameAssetFolder());
}

UMaterialInterface* AMBPWallActor::LoadShimmerMaterialOverride(EMBPShimmerVariant Variant) const
{
	const FString AssetPath = GetShimmerMaterialPath(Variant);
	return AssetPath.IsEmpty() ? nullptr : LoadObject<UMaterialInterface>(nullptr, *AssetPath);
}

UMaterialInterface* AMBPWallActor::ResolveShimmerMaterial(const FMBPPanelSlot& Slot) const
{
	if (!Slot.ShimmerMaterial.IsNull())
	{
		return Slot.ShimmerMaterial.LoadSynchronous();
	}

	if (!DefaultShimmerMaterial.IsNull())
	{
		return DefaultShimmerMaterial.LoadSynchronous();
	}

	return LoadShimmerMaterialOverride(Slot.ShimmerVariant);
}

UInstancedStaticMeshComponent* AMBPWallActor::FindOrCreateComponentBucket(
	const FSoftObjectPath& MeshPath,
	const FMBPPanelSlot& Slot,
	TMap<FString, UInstancedStaticMeshComponent*>& BucketMap)
{
	const FString VariantSuffix = Slot.Style == EMBPPanelStyle::Shimmer
		? FString::Printf(
			TEXT("_%s"),
			*(
				!Slot.ShimmerMaterial.IsNull()
					? Slot.ShimmerMaterial.ToSoftObjectPath().GetAssetName()
					: (!DefaultShimmerMaterial.IsNull()
						? DefaultShimmerMaterial.ToSoftObjectPath().GetAssetName()
						: FString::FromInt(static_cast<int32>(Slot.ShimmerVariant)))))
		: FString();
	const FString BucketKey = MeshPath.ToString() + VariantSuffix;

	if (UInstancedStaticMeshComponent** ExistingComponent = BucketMap.Find(BucketKey))
	{
		return *ExistingComponent;
	}

	UStaticMesh* StaticMesh = Cast<UStaticMesh>(MeshPath.TryLoad());
	if (!StaticMesh)
	{
		return nullptr;
	}

	const FName ComponentName(*FString::Printf(TEXT("MBP_%s%s"), *StaticMesh->GetName(), *VariantSuffix));
	UInstancedStaticMeshComponent* MeshComponent = FindGeneratedComponentByName(ComponentName);
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
		GeneratedInstanceComponents.Add(MeshComponent);
	}

	MeshComponent->SetStaticMesh(StaticMesh);
	MeshComponent->SetHiddenInGame(false);
	MeshComponent->SetVisibility(true);

	if (Slot.Style == EMBPPanelStyle::Shimmer && StaticMesh->GetName().Equals(TEXT("Plane")))
	{
		if (UMaterialInterface* OverrideMaterial = ResolveShimmerMaterial(Slot))
		{
			MeshComponent->SetMaterial(0, OverrideMaterial);
		}
	}

	BucketMap.Add(BucketKey, MeshComponent);

	return MeshComponent;
}

void AMBPWallActor::ClearGeneratedComponents()
{
	TInlineComponentArray<UInstancedStaticMeshComponent*> TrackedInstanceComponents(this);
	for (UInstancedStaticMeshComponent* MeshComponent : TrackedInstanceComponents)
	{
		if (!MeshComponent)
		{
			continue;
		}

		if (MeshComponent->GetName().StartsWith(TEXT("MBP_")) && !GeneratedInstanceComponents.Contains(MeshComponent))
		{
			GeneratedInstanceComponents.Add(MeshComponent);
		}
	}

	for (UInstancedStaticMeshComponent* MeshComponent : GeneratedInstanceComponents)
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

UInstancedStaticMeshComponent* AMBPWallActor::FindGeneratedComponentByName(const FName& ComponentName) const
{
	for (UInstancedStaticMeshComponent* MeshComponent : GeneratedInstanceComponents)
	{
		if (MeshComponent && MeshComponent->GetFName() == ComponentName)
		{
			return MeshComponent;
		}
	}

	return FindObjectFast<UInstancedStaticMeshComponent>(const_cast<AMBPWallActor*>(this), ComponentName);
}

void AMBPWallActor::UpdateSelectionBounds(const FBox& Bounds)
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
