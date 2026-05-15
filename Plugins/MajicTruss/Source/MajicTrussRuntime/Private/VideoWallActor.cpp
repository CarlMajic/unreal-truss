#include "VideoWallActor.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Modules/ModuleManager.h"

namespace
{
constexpr float VideoWallDefaultPanelWidthCm = 50.0f;
constexpr float VideoWallDefaultPanelHeightCm = 50.0f;
constexpr float OneMeterBracketWidthCm = 100.0f;
constexpr float HalfMeterBracketWidthCm = 50.0f;

const FString SinglePanelFolder = TEXT("/Game/Majic_Gear/Video_Wall/InfiLED/Single_Panel/StaticMeshes");
const FString Bracket1000Folder = TEXT("/Game/Majic_Gear/Video_Wall/InfiLED/1000mm_Hanging_Bracket/StaticMeshes");
const FString Bracket500Folder = TEXT("/Game/Majic_Gear/Video_Wall/InfiLED/500mm_Hanging_Bracket/StaticMeshes");
const FString SupportSkyFolder = TEXT("/Game/Majic_Gear/Video_Wall/InfiLED/Support_Sky/StaticMeshes");
const FString StackerFolder = TEXT("/Game/Majic_Gear/Video_Wall/InfiLED/Stacking_Stacker/StaticMeshes");
const FString HTubeOuterFolder = TEXT("/Game/Majic_Gear/Video_Wall/InfiLED/H_Tube/StaticMeshes");
const FString HTubeInnerFolder = TEXT("/Game/Majic_Gear/Video_Wall/InfiLED/H_Tube/StaticMeshes/Inner_Parts");

bool IsStaticMeshAsset(const FAssetData& AssetData)
{
	return AssetData.AssetClassPath == UStaticMesh::StaticClass()->GetClassPathName();
}
}

AVideoWallActor::AVideoWallActor()
{
	PrimaryActorTick.bCanEverTick = false;
	GeneratedBounds = FBox(EForceInit::ForceInit);

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

void AVideoWallActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (bBuildOnConstruction)
	{
		RebuildVideoWall();
	}
}

#if WITH_EDITOR
void AVideoWallActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RebuildVideoWall();
}
#endif

void AVideoWallActor::RebuildVideoWall()
{
	Columns = FMath::Max(1, Columns);
	Rows = FMath::Max(1, Rows);
	PanelWidthCm = FMath::Max(1.0f, PanelWidthCm);
	PanelHeightCm = FMath::Max(1.0f, PanelHeightCm);
	StackerLevelHeightCm = FMath::Max(1.0f, StackerLevelHeightCm);

	CurrentWallWidthCm = (Columns * PanelWidthCm) + (FMath::Max(0, Columns - 1) * PanelGapCm);
	CurrentWallHeightCm = (Rows * PanelHeightCm) + (FMath::Max(0, Rows - 1) * PanelGapCm);
	Current1000mmBracketCount = Columns / 2;
	Current500mmBracketCount = Columns % 2;
	CurrentStackerLevels = FMath::Max(1, FMath::CeilToInt(CurrentWallHeightCm / StackerLevelHeightCm));

	const float WallLeftX = bCenterOnActor ? (-0.5f * CurrentWallWidthCm) : 0.0f;
	const float WallRightX = WallLeftX + CurrentWallWidthCm;
	const float WallBottomZ = bCenterOnActor ? (-0.5f * CurrentWallHeightCm) : 0.0f;
	const float WallTopZ = WallBottomZ + CurrentWallHeightCm;

	ClearGenerated();
	BuildPanelGrid(WallLeftX, WallBottomZ);

	if (SupportMode == EVideoWallSupportMode::Flown)
	{
		BuildFlownSupport(WallLeftX, WallRightX, WallTopZ);
	}
	else
	{
		BuildGroundSupport(WallLeftX, WallRightX, WallBottomZ);
	}

	UpdateSelectionBounds();
}

void AVideoWallActor::ApplyBuildDefinition(const FVideoWallBuildDefinition& Definition, bool bRebuildNow)
{
	Columns = FMath::Max(1, Definition.Columns);
	Rows = FMath::Max(1, Definition.Rows);
	SupportMode = Definition.SupportMode;
	SupportSpacing = Definition.SupportSpacing;
	PanelWidthCm = FMath::Max(1.0f, Definition.PanelWidthCm);
	PanelHeightCm = FMath::Max(1.0f, Definition.PanelHeightCm);
	PanelGapCm = Definition.PanelGapCm;
	bCenterOnActor = Definition.bCenterOnActor;

	if (bRebuildNow)
	{
		RebuildVideoWall();
	}
}

FVideoWallBuildDefinition AVideoWallActor::GetBuildDefinition() const
{
	FVideoWallBuildDefinition Definition;
	Definition.Columns = Columns;
	Definition.Rows = Rows;
	Definition.SupportMode = SupportMode;
	Definition.SupportSpacing = SupportSpacing;
	Definition.PanelWidthCm = PanelWidthCm;
	Definition.PanelHeightCm = PanelHeightCm;
	Definition.PanelGapCm = PanelGapCm;
	Definition.bCenterOnActor = bCenterOnActor;
	return Definition;
}

void AVideoWallActor::SetSelectionHighlighted(bool bHighlighted)
{
	if (SelectionBounds)
	{
		SelectionBounds->SetHiddenInGame(!bHighlighted);
	}
}

void AVideoWallActor::ClearGenerated()
{
	GeneratedBounds = FBox(EForceInit::ForceInit);

	TInlineComponentArray<UInstancedStaticMeshComponent*> ExistingInstanceComponents(this);
	for (UInstancedStaticMeshComponent* MeshComponent : ExistingInstanceComponents)
	{
		if (MeshComponent && MeshComponent->GetName().StartsWith(TEXT("VideoWall_")) && !GeneratedInstanceComponents.Contains(MeshComponent))
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

void AVideoWallActor::BuildPanelGrid(float WallLeftX, float WallBottomZ)
{
	const float StepX = PanelWidthCm + PanelGapCm;
	const float StepZ = PanelHeightCm + PanelGapCm;

	for (int32 RowIndex = 0; RowIndex < Rows; ++RowIndex)
	{
		for (int32 ColumnIndex = 0; ColumnIndex < Columns; ++ColumnIndex)
		{
			const FVector PanelLocation = PanelPlacementOffsetCm + FVector(
				WallLeftX + (0.5f * PanelWidthCm) + (ColumnIndex * StepX),
				0.0f,
				WallBottomZ + (0.5f * PanelHeightCm) + (RowIndex * StepZ));
			AddAssemblyInstancesFromFolder(SinglePanelFolder, PanelLocation, PanelPlacementRotation, PanelScale, TEXT("Panel"));
		}
	}
}

void AVideoWallActor::BuildGroundSupport(float WallLeftX, float WallRightX, float WallBottomZ)
{
	BuildBracketRun(WallLeftX, WallBottomZ, false);

	const TArray<float> TowerPositionsX = GetTowerPositions(WallLeftX, WallRightX);
	CurrentTowerCount = TowerPositionsX.Num();

	BuildTowerStack(TowerPositionsX, WallBottomZ);
	if (bBuildHTubes)
	{
		BuildHTubeRuns(TowerPositionsX, WallBottomZ);
	}
}

void AVideoWallActor::BuildFlownSupport(float WallLeftX, float WallRightX, float WallTopZ)
{
	BuildBracketRun(WallLeftX, WallTopZ, true);
	CurrentTowerCount = 0;
}

void AVideoWallActor::BuildBracketRun(float WallLeftX, float WallZ, bool bFlipped)
{
	float CurrentX = WallLeftX;
	const FVector ActiveBracketOffset = bFlipped ? FlownBracketPlacementOffsetCm : BracketPlacementOffsetCm;
	const FRotator ActiveBracketRotation = bFlipped ? FlownBracketPlacementRotation : BracketPlacementRotation;
	const FVector ActiveBracketScale = bFlipped ? FlownBracketScale : BracketScale;

	for (int32 BracketIndex = 0; BracketIndex < Current1000mmBracketCount; ++BracketIndex)
	{
		const float BracketAnchorX = bFlipped ? CurrentX : CurrentX + (0.5f * OneMeterBracketWidthCm);
		const FVector BracketLocation = ActiveBracketOffset + FVector(BracketAnchorX, 0.0f, WallZ);
		AddAssemblyInstancesFromFolder(Bracket1000Folder, BracketLocation, ActiveBracketRotation, ActiveBracketScale, TEXT("Bracket1000"));
		CurrentX += OneMeterBracketWidthCm;
	}

	if (Current500mmBracketCount > 0)
	{
		const float BracketAnchorX = bFlipped ? CurrentX - HalfMeterBracketWidthCm : CurrentX + HalfMeterBracketWidthCm;
		const FVector BracketLocation = ActiveBracketOffset + FVector(BracketAnchorX, 0.0f, WallZ);
		AddAssemblyInstancesFromFolder(Bracket500Folder, BracketLocation, ActiveBracketRotation, ActiveBracketScale, TEXT("Bracket500"));
	}
}

void AVideoWallActor::BuildTowerStack(const TArray<float>& TowerPositionsX, float WallBottomZ)
{
	for (const float TowerX : TowerPositionsX)
	{
		const FVector SupportLocation = SupportSkyPlacementOffsetCm + FVector(TowerX, 0.0f, WallBottomZ);
		AddAssemblyInstancesFromFolder(SupportSkyFolder, SupportLocation, SupportSkyPlacementRotation, SupportSkyScale, TEXT("SupportSky"));

		for (int32 LevelIndex = 0; LevelIndex < CurrentStackerLevels; ++LevelIndex)
		{
			const FVector StackerLocation = StackerPlacementOffsetCm + FVector(TowerX, 0.0f, WallBottomZ + (LevelIndex * StackerLevelHeightCm));
			AddAssemblyInstancesFromFolder(StackerFolder, StackerLocation, StackerPlacementRotation, StackerScale, TEXT("Stacker"));
		}
	}
}

void AVideoWallActor::BuildHTubeRuns(const TArray<float>& TowerPositionsX, float WallBottomZ)
{
	if (TowerPositionsX.Num() < 2)
	{
		return;
	}

	const FVector AssemblyOffset = GetHTubeAssemblyOffsetCm();
	const FVector InnerPartsOffset = GetHTubeInnerPartsOffsetCm();
	for (int32 TowerIndex = 0; TowerIndex < TowerPositionsX.Num() - 1; ++TowerIndex)
	{
		const float MidX = (TowerPositionsX[TowerIndex] + TowerPositionsX[TowerIndex + 1]) * 0.5f;
		for (int32 LevelIndex = 0; LevelIndex < CurrentStackerLevels; ++LevelIndex)
		{
			const FVector HTubeLocation = HTubePlacementOffsetCm + AssemblyOffset + FVector(MidX, 0.0f, WallBottomZ + (LevelIndex * StackerLevelHeightCm));
			AddAssemblyInstancesFromFolder(HTubeOuterFolder, HTubeLocation, HTubePlacementRotation, HTubeScale, TEXT("HTubeOuter"));
			AddAssemblyInstancesFromFolder(HTubeInnerFolder, HTubeLocation + InnerPartsOffset, HTubePlacementRotation, HTubeScale, TEXT("HTubeInner"));
		}
	}
}

TArray<float> AVideoWallActor::GetTowerPositions(float WallLeftX, float WallRightX) const
{
	TArray<float> TowerPositions;
	const float SpacingCm = GetSupportSpacingCm();
	if (SpacingCm <= KINDA_SMALL_NUMBER || WallRightX <= WallLeftX)
	{
		return TowerPositions;
	}

	for (float CurrentX = WallLeftX; CurrentX < WallRightX - KINDA_SMALL_NUMBER; CurrentX += SpacingCm)
	{
		TowerPositions.Add(CurrentX);
	}

	if (bAddRightEdgeTower && (TowerPositions.Num() == 0 || !FMath::IsNearlyEqual(TowerPositions.Last(), WallRightX, 0.1f)))
	{
		TowerPositions.Add(WallRightX);
	}

	return TowerPositions;
}

float AVideoWallActor::GetSupportSpacingCm() const
{
	return SupportSpacing == EVideoWallSupportSpacing::OnePointFiveMeters ? 150.0f : 100.0f;
}

FVector AVideoWallActor::GetHTubeAssemblyOffsetCm() const
{
	return SupportSpacing == EVideoWallSupportSpacing::OnePointFiveMeters ? HTubeAssemblyOffset1500mmCm : HTubeAssemblyOffset1000mmCm;
}

FVector AVideoWallActor::GetHTubeInnerPartsOffsetCm() const
{
	return SupportSpacing == EVideoWallSupportSpacing::OnePointFiveMeters ? HTubeInnerPartsRelativeOffset1500mmCm : HTubeInnerPartsRelativeOffset1000mmCm;
}

void AVideoWallActor::AddAssemblyInstancesFromFolder(
	const FString& MeshFolderPath,
	const FVector& Location,
	const FRotator& Rotation,
	const FVector& Scale,
	const FString& BucketPrefix)
{
	for (const FSoftObjectPath& MeshPath : GetStaticMeshPathsForFolder(MeshFolderPath))
	{
		UStaticMesh* StaticMesh = Cast<UStaticMesh>(MeshPath.TryLoad());
		if (!StaticMesh)
		{
			continue;
		}

		if (UInstancedStaticMeshComponent* MeshComponent = FindOrCreateGeneratedComponent(MeshPath, BucketPrefix))
		{
			AddInstance(MeshComponent, StaticMesh, FTransform(Rotation, Location, Scale));
		}
	}
}

UInstancedStaticMeshComponent* AVideoWallActor::FindOrCreateGeneratedComponent(const FSoftObjectPath& MeshPath, const FString& BucketPrefix)
{
	const FName ComponentName(*FString::Printf(TEXT("VideoWall_%s_%s"), *BucketPrefix, *MeshPath.GetAssetName()));
	if (UInstancedStaticMeshComponent* ExistingComponent = FindObjectFast<UInstancedStaticMeshComponent>(this, ComponentName))
	{
		GeneratedInstanceComponents.AddUnique(ExistingComponent);
		return ExistingComponent;
	}

	UInstancedStaticMeshComponent* MeshComponent = NewObject<UInstancedStaticMeshComponent>(this, ComponentName);
	if (!MeshComponent)
	{
		return nullptr;
	}

	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->bDisallowNanite = true;
	MeshComponent->RegisterComponent();
	AddInstanceComponent(MeshComponent);
	GeneratedInstanceComponents.Add(MeshComponent);
	return MeshComponent;
}

TArray<FSoftObjectPath> AVideoWallActor::GetStaticMeshPathsForFolder(const FString& MeshFolderPath) const
{
	TArray<FSoftObjectPath> MeshPaths;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> Assets;
	AssetRegistryModule.Get().GetAssetsByPath(*MeshFolderPath, Assets, false);

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

void AVideoWallActor::AddInstance(UInstancedStaticMeshComponent* Component, UStaticMesh* Mesh, const FTransform& InstanceTransform)
{
	if (!Component || !Mesh)
	{
		return;
	}

	if (Component->GetStaticMesh() != Mesh)
	{
		Component->SetStaticMesh(Mesh);
	}

	Component->SetVisibility(true);
	Component->SetHiddenInGame(false);
	Component->AddInstance(InstanceTransform);
	ExpandGeneratedBounds(Mesh, InstanceTransform);
}

void AVideoWallActor::ExpandGeneratedBounds(UStaticMesh* Mesh, const FTransform& Transform)
{
	if (!Mesh)
	{
		return;
	}

	const FBox MeshBounds = Mesh->GetBoundingBox().TransformBy(Transform);
	if (MeshBounds.IsValid)
	{
		GeneratedBounds += MeshBounds;
		return;
	}

	const FVector Location = Transform.GetLocation();
	GeneratedBounds += FBox(
		Location - FVector(VideoWallDefaultPanelWidthCm, 5.0f, VideoWallDefaultPanelHeightCm) * 0.5f,
		Location + FVector(VideoWallDefaultPanelWidthCm, 5.0f, VideoWallDefaultPanelHeightCm) * 0.5f);
}

void AVideoWallActor::UpdateSelectionBounds()
{
	if (!SelectionBounds)
	{
		return;
	}

	SelectionBounds->SetWorldScale3D(FVector::OneVector);

	if (!GeneratedBounds.IsValid)
	{
		SelectionBounds->SetRelativeLocation(FVector::ZeroVector);
		SelectionBounds->SetBoxExtent(FVector(50.0f, 50.0f, 50.0f));
		return;
	}

	SelectionBounds->SetRelativeLocation(GeneratedBounds.GetCenter());
	SelectionBounds->SetBoxExtent(GeneratedBounds.GetExtent().ComponentMax(FVector(50.0f, 10.0f, 50.0f)));
}
