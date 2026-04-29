#include "MBPWallActor.h"

#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInterface.h"

namespace
{
UInstancedStaticMeshComponent* CreateWallMeshComponent(AActor* Owner, USceneComponent* Parent, const TCHAR* Name)
{
	UInstancedStaticMeshComponent* Component = Owner->CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
	Component->SetupAttachment(Parent);
	Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	return Component;
}
}

AMBPWallActor::AMBPWallActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	SelectionBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("SelectionBounds"));
	SelectionBounds->SetupAttachment(SceneRoot);
	SelectionBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SelectionBounds->SetHiddenInGame(true);

	AcrylicInstances = CreateWallMeshComponent(this, SceneRoot, TEXT("AcrylicInstances"));
	DriftInstances = CreateWallMeshComponent(this, SceneRoot, TEXT("DriftInstances"));
	GeoInstances = CreateWallMeshComponent(this, SceneRoot, TEXT("GeoInstances"));
	HiveInstances = CreateWallMeshComponent(this, SceneRoot, TEXT("HiveInstances"));
	PlatinumInstances = CreateWallMeshComponent(this, SceneRoot, TEXT("PlatinumInstances"));
	ShimmerGoldInstances = CreateWallMeshComponent(this, SceneRoot, TEXT("ShimmerGoldInstances"));
	ShimmerRedInstances = CreateWallMeshComponent(this, SceneRoot, TEXT("ShimmerRedInstances"));
	ShimmerFuchsiaInstances = CreateWallMeshComponent(this, SceneRoot, TEXT("ShimmerFuchsiaInstances"));
	ShimmerHolographicInstances = CreateWallMeshComponent(this, SceneRoot, TEXT("ShimmerHolographicInstances"));
}

void AMBPWallActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	EnsureSlotCount(false);
	if (bBuildOnConstruction)
	{
		RebuildWall();
	}
}

void AMBPWallActor::RebuildWall()
{
	EnsureSlotCount(false);
	ClearInstances();

	const float StepX = PanelWidthCm + HorizontalSpacingCm;
	const float StepZ = PanelHeightCm + VerticalSpacingCm;
	const float OriginX = bCenterOnActor ? (-0.5f * (Columns - 1) * StepX) : 0.0f;
	const float OriginZ = bCenterOnActor ? (-0.5f * (Rows - 1) * StepZ) : 0.0f;
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
			UInstancedStaticMeshComponent* MeshComponent = GetComponentForSlot(Slot);
			if (!MeshComponent || !MeshComponent->GetStaticMesh())
			{
				continue;
			}

			const FVector Location(
				OriginX + (ColumnIndex * StepX),
				Slot.DepthOffsetCm,
				OriginZ + (RowIndex * StepZ));

			const FTransform InstanceTransform(FRotator::ZeroRotator, Location, FVector::OneVector);
			MeshComponent->AddInstance(InstanceTransform);

			const FBoxSphereBounds MeshBounds = MeshComponent->GetStaticMesh()->GetBounds();
			const FVector Extent = MeshBounds.BoxExtent;
			Bounds += FBox(Location - Extent, Location + Extent);
		}
	}

	UpdateSelectionBounds(Bounds);
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

	AcrylicInstances->SetStaticMesh(LoadMeshForStyle(EMBPPanelStyle::Acrylic));
	DriftInstances->SetStaticMesh(LoadMeshForStyle(EMBPPanelStyle::Drift));
	GeoInstances->SetStaticMesh(LoadMeshForStyle(EMBPPanelStyle::Geo));
	HiveInstances->SetStaticMesh(LoadMeshForStyle(EMBPPanelStyle::Hive));
	PlatinumInstances->SetStaticMesh(LoadMeshForStyle(EMBPPanelStyle::Platinum));
	ShimmerGoldInstances->SetStaticMesh(LoadMeshForStyle(EMBPPanelStyle::Shimmer));
	ShimmerRedInstances->SetStaticMesh(LoadMeshForStyle(EMBPPanelStyle::Shimmer));
	ShimmerFuchsiaInstances->SetStaticMesh(LoadMeshForStyle(EMBPPanelStyle::Shimmer));
	ShimmerHolographicInstances->SetStaticMesh(LoadMeshForStyle(EMBPPanelStyle::Shimmer));

	ShimmerGoldInstances->SetMaterial(0, LoadShimmerMaterial(EMBPShimmerVariant::Gold));
	ShimmerRedInstances->SetMaterial(0, LoadShimmerMaterial(EMBPShimmerVariant::Red));
	ShimmerFuchsiaInstances->SetMaterial(0, LoadShimmerMaterial(EMBPShimmerVariant::Fuchsia));
	ShimmerHolographicInstances->SetMaterial(0, LoadShimmerMaterial(EMBPShimmerVariant::Holographic));
}

FMBPPanelSlot AMBPWallActor::MakeDefaultSlot() const
{
	FMBPPanelSlot Slot;
	Slot.Style = DefaultStyle;
	Slot.ShimmerVariant = DefaultShimmerVariant;
	return Slot;
}

UStaticMesh* AMBPWallActor::LoadMeshForStyle(EMBPPanelStyle Style) const
{
	const TCHAR* AssetPath = nullptr;

	switch (Style)
	{
	case EMBPPanelStyle::Acrylic:
		AssetPath = TEXT("/Game/Majic_Gear/MBP/Acrylic_MBP_Segments/StaticMeshes/SM_Acrylic_Segment_v1.SM_Acrylic_Segment_v1");
		break;
	case EMBPPanelStyle::Drift:
		AssetPath = TEXT("/Game/Majic_Gear/MBP/Drift_MBP_Segments/StaticMeshes/SM_Drift_Segment_v1.SM_Drift_Segment_v1");
		break;
	case EMBPPanelStyle::Geo:
		AssetPath = TEXT("/Game/Majic_Gear/MBP/Geo_MBP_Segments/StaticMeshes/SM_GeoWall_Segment_v1.SM_GeoWall_Segment_v1");
		break;
	case EMBPPanelStyle::Shimmer:
		AssetPath = TEXT("/Game/Majic_Gear/MBP/Gold_Shimmer_MBP_Segments/StaticMeshes/SM_Shimmer_Segment_v1.SM_Shimmer_Segment_v1");
		break;
	case EMBPPanelStyle::Hive:
		AssetPath = TEXT("/Game/Majic_Gear/MBP/Hive_MBP_Segments/StaticMeshes/SM_Hive_Segment_v1.SM_Hive_Segment_v1");
		break;
	case EMBPPanelStyle::Platinum:
		AssetPath = TEXT("/Game/Majic_Gear/MBP/Platinum_MBP_Segments/StaticMeshes/SM_Platinum_Segment_v1.SM_Platinum_Segment_v1");
		break;
	default:
		break;
	}

	return AssetPath ? LoadObject<UStaticMesh>(nullptr, AssetPath) : nullptr;
}

UMaterialInterface* AMBPWallActor::LoadShimmerMaterial(EMBPShimmerVariant Variant) const
{
	const TCHAR* AssetPath = nullptr;

	switch (Variant)
	{
	case EMBPShimmerVariant::Red:
		AssetPath = TEXT("/Game/Majic_Gear/MBP/Gold_Shimmer_MBP_Segments/Materials/RedShimmerSquare.RedShimmerSquare");
		break;
	case EMBPShimmerVariant::Fuchsia:
		AssetPath = TEXT("/Game/Majic_Gear/MBP/Gold_Shimmer_MBP_Segments/Materials/FushaShimmerSquare.FushaShimmerSquare");
		break;
	case EMBPShimmerVariant::Holographic:
		AssetPath = TEXT("/Game/Majic_Gear/MBP/Gold_Shimmer_MBP_Segments/Materials/HolographicSquare_Mat1.HolographicSquare_Mat1");
		break;
	case EMBPShimmerVariant::Gold:
	default:
		AssetPath = TEXT("/Game/Majic_Gear/MBP/Gold_Shimmer_MBP_Segments/Materials/GoldShimmerSquare_Mat.GoldShimmerSquare_Mat");
		break;
	}

	return AssetPath ? LoadObject<UMaterialInterface>(nullptr, AssetPath) : nullptr;
}

UInstancedStaticMeshComponent* AMBPWallActor::GetComponentForSlot(const FMBPPanelSlot& Slot) const
{
	switch (Slot.Style)
	{
	case EMBPPanelStyle::Acrylic:
		return AcrylicInstances;
	case EMBPPanelStyle::Drift:
		return DriftInstances;
	case EMBPPanelStyle::Geo:
		return GeoInstances;
	case EMBPPanelStyle::Hive:
		return HiveInstances;
	case EMBPPanelStyle::Platinum:
		return PlatinumInstances;
	case EMBPPanelStyle::Shimmer:
		switch (Slot.ShimmerVariant)
		{
		case EMBPShimmerVariant::Red:
			return ShimmerRedInstances;
		case EMBPShimmerVariant::Fuchsia:
			return ShimmerFuchsiaInstances;
		case EMBPShimmerVariant::Holographic:
			return ShimmerHolographicInstances;
		case EMBPShimmerVariant::Gold:
		default:
			return ShimmerGoldInstances;
		}
	default:
		return nullptr;
	}
}

void AMBPWallActor::ClearInstances()
{
	for (UInstancedStaticMeshComponent* MeshComponent : {
		AcrylicInstances.Get(),
		DriftInstances.Get(),
		GeoInstances.Get(),
		HiveInstances.Get(),
		PlatinumInstances.Get(),
		ShimmerGoldInstances.Get(),
		ShimmerRedInstances.Get(),
		ShimmerFuchsiaInstances.Get(),
		ShimmerHolographicInstances.Get()})
	{
		if (MeshComponent)
		{
			MeshComponent->ClearInstances();
		}
	}
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
