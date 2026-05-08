#include "DrapeRunActor.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Modules/ModuleManager.h"

namespace
{
constexpr TCHAR DrapeBasePrefix[] = TEXT("DrapeRunBase");
constexpr TCHAR DrapeUprightPrefix[] = TEXT("DrapeRunUpright");
constexpr TCHAR DrapeCrossbarPrefix[] = TEXT("DrapeRunCrossbar");
constexpr TCHAR DrapePanelPrefix[] = TEXT("DrapeRunPanel");
constexpr TCHAR DrapeChaosPrefix[] = TEXT("DrapeRunChaosPanel");
constexpr float CmPerFoot = 30.48f;
}

ADrapeRunActor::ADrapeRunActor()
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
	SelectionBounds->ShapeColor = FColor::Purple;

	StaticDrapeMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Majic_Gear/Drape/Static_Drape/StaticMeshes/Static_Drape.Static_Drape")));
	ChaosDrapeMesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Majic_Gear/StageDrape/StageDrape/SkeletalMeshes/SK_Drape.SK_Drape")));
}

void ADrapeRunActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (bBuildOnConstruction)
	{
		RebuildDrapeRun();
	}
}

#if WITH_EDITOR
void ADrapeRunActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RebuildDrapeRun();
}
#endif

void ADrapeRunActor::RebuildDrapeRun()
{
	ClearGeneratedComponents();

	const int32 SectionCount = GetSectionCount();
	const float ComputedSectionLengthFt = GetSectionLengthFt();
	const float SectionLengthCm = ComputedSectionLengthFt * CmPerFoot;
	const float HeightCm = GetHeightCm();
	const float SlidingAssemblyHeightAdjustmentCm = HeightCm - NativeRodHeightCm;
	const float InsideUprightHeightAdjustmentCm = HeightCm - GetInsideUprightNativeHeightCm() + GetInsideUprightCalibrationOffsetCm();
	CurrentSlidingHeightAdjustmentCm = SlidingAssemblyHeightAdjustmentCm;
	CurrentInsideUprightHeightAdjustmentCm = InsideUprightHeightAdjustmentCm;
	CurrentSectionCount = SectionCount;
	CurrentSectionLengthFt = ComputedSectionLengthFt;
	CurrentActualLengthFt = SectionCount * ComputedSectionLengthFt;
	const float StartX = bCenterOnActor ? (-0.5f * SectionCount * SectionLengthCm) : 0.0f;

	UStaticMesh* ResolvedStaticDrapeMesh = StaticDrapeMesh.LoadSynchronous();
	const TArray<FSoftObjectPath> BaseMeshPaths = GetBaseMeshPaths();
	const TArray<FSoftObjectPath> OutsidePoleMeshPaths = GetOutsidePoleMeshPaths();
	const TArray<FSoftObjectPath> InsidePoleMeshPaths = GetInsidePoleMeshPaths();
	const TArray<FSoftObjectPath> OutsideRodMeshPaths = GetOutsideRodMeshPaths();
	const TArray<FSoftObjectPath> InsideRodMeshPaths = GetInsideRodMeshPaths();

	FBox Bounds(EForceInit::ForceInit);

	if (bShowHardware)
	{
		for (int32 UprightIndex = 0; UprightIndex <= SectionCount; ++UprightIndex)
		{
			const float X = StartX + (UprightIndex * SectionLengthCm);
			const FVector UprightLocation(X, 0.0f, 0.0f);
			const FTransform BaseTransform(
				BasePlacementRotation,
				UprightLocation + BasePlacementOffsetCm,
				BaseScale);
			AddMeshPathSet(BaseMeshPaths, DrapeBasePrefix, BaseTransform, false, Bounds);

			const FTransform OutsideUprightTransform(
				UprightPlacementRotation,
				UprightLocation + UprightPlacementOffsetCm,
				UprightScale);
			AddMeshPathSet(OutsidePoleMeshPaths, DrapeUprightPrefix, OutsideUprightTransform, false, Bounds);

			const FTransform InsideUprightTransform(
				UprightPlacementRotation,
				UprightLocation + UprightPlacementOffsetCm + InsideUprightPlacementOffsetCm + FVector(0.0f, 0.0f, InsideUprightHeightAdjustmentCm),
				UprightScale);
			AddMeshPathSet(InsidePoleMeshPaths, DrapeUprightPrefix, InsideUprightTransform, false, Bounds);
		}
	}

	for (int32 SectionIndex = 0; SectionIndex < SectionCount; ++SectionIndex)
	{
		const float SectionStartX = StartX + (SectionIndex * SectionLengthCm);
		const float SectionCenterX = StartX + ((SectionIndex + 0.5f) * SectionLengthCm);
		const FVector SectionStart(SectionStartX, 0.0f, 0.0f);
		const FVector SectionCenter(SectionCenterX, 0.0f, 0.0f);

		if (bShowHardware)
		{
			const float NativeRodLengthCm = FMath::Max(1.0f, NativeRodLengthFt) * CmPerFoot;
			const float RodSlideCm = SectionLengthCm - NativeRodLengthCm;
			const float OutsideRodSlideCm = (bMoveOutsideRodForSectionLength ? RodSlideCm : 0.0f) + OutsideRodSpanAdjustmentCm;
			const float InsideRodSlideCm = bMoveInsideRodForSectionLength ? RodSlideCm : 0.0f;

			const FTransform OutsideRodTransform(
				CrossbarPlacementRotation,
				SectionStart + FVector(OutsideRodSlideCm, 0.0f, SlidingAssemblyHeightAdjustmentCm) + CrossbarPlacementOffsetCm,
				CrossbarScale);
			AddMeshPathSet(OutsideRodMeshPaths, DrapeCrossbarPrefix, OutsideRodTransform, false, Bounds);

			const FTransform InsideRodTransform(
				CrossbarPlacementRotation,
				SectionStart + FVector(InsideRodSlideCm, 0.0f, SlidingAssemblyHeightAdjustmentCm) + CrossbarPlacementOffsetCm,
				CrossbarScale);
			AddMeshPathSet(InsideRodMeshPaths, DrapeCrossbarPrefix, InsideRodTransform, false, Bounds);
		}

		if (!bShowDrape)
		{
			continue;
		}

		FVector ResolvedDrapeScale = DrapeScale;
		const float BaseDrapeScaleZ = ResolvedDrapeScale.Z;
		if (bScaleDrapeToSection && NativeSectionLengthCm > KINDA_SMALL_NUMBER)
		{
			ResolvedDrapeScale.X *= (SectionLengthCm * FMath::Max(0.1f, Fullness)) / NativeSectionLengthCm;
		}
		if (bScaleDrapeToHeight && NativeDrapeHeightCm > KINDA_SMALL_NUMBER)
		{
			ResolvedDrapeScale.Z *= HeightCm / NativeDrapeHeightCm;
		}

		FVector DrapeScaleAnchorAdjustment = FVector::ZeroVector;
		if (bAnchorDrapeTopWhenScaling)
		{
			DrapeScaleAnchorAdjustment.Z = -(ResolvedDrapeScale.Z - BaseDrapeScaleZ) * DrapeTopAnchorLocalZCm;
		}

		const FTransform DrapeTransform(
			DrapePlacementRotation,
			SectionCenter + FVector(0.0f, 0.0f, SlidingAssemblyHeightAdjustmentCm) + DrapePlacementOffsetCm + DrapeScaleAnchorAdjustment,
			ResolvedDrapeScale);

		if (DrapeMode == EDrapeRunDrapeMode::ChaosCloth)
		{
			AddChaosDrapeSection(SectionIndex, DrapeTransform, Bounds);
		}
		else
		{
			AddStaticMeshInstance(ResolvedStaticDrapeMesh, DrapePanelPrefix, DrapeTransform, true, Bounds);
		}
	}

	UpdateSelectionBounds(Bounds);
}

void ADrapeRunActor::ApplyBuildDefinition(const FDrapeRunBuildDefinition& Definition, bool bRebuildNow)
{
	LengthFt = FMath::Max(1.0f, Definition.LengthFt);
	SectionLengthFt = FMath::Max(1.0f, Definition.SectionLengthFt);
	HeightFt = FMath::Clamp(Definition.HeightFt, 6.0f, 18.333f);
	DrapeMode = Definition.DrapeMode;
	Fullness = FMath::Max(0.1f, Definition.Fullness);
	bShowHardware = Definition.bShowHardware;
	bCenterOnActor = Definition.bCenterOnActor;
	DrapeMaterialOverride = Definition.DrapeMaterialOverride;

	if (bRebuildNow)
	{
		RebuildDrapeRun();
	}
}

FDrapeRunBuildDefinition ADrapeRunActor::GetBuildDefinition() const
{
	FDrapeRunBuildDefinition Definition;
	Definition.LengthFt = LengthFt;
	Definition.SectionLengthFt = SectionLengthFt;
	Definition.HeightFt = HeightFt;
	Definition.DrapeMode = DrapeMode;
	Definition.Fullness = Fullness;
	Definition.bShowHardware = bShowHardware;
	Definition.bCenterOnActor = bCenterOnActor;
	Definition.DrapeMaterialOverride = DrapeMaterialOverride;
	return Definition;
}

void ADrapeRunActor::SetSelectionHighlighted(bool bHighlighted)
{
	if (SelectionBounds)
	{
		SelectionBounds->SetHiddenInGame(!bHighlighted);
	}
}

float ADrapeRunActor::GetHeightCm() const
{
	return FMath::Clamp(HeightFt, 6.0f, 18.333f) * CmPerFoot;
}

int32 ADrapeRunActor::GetSectionCount() const
{
	if (!bAutoCalculateSections)
	{
		const float SafeSectionLengthFt = FMath::Max(1.0f, SectionLengthFt);
		return FMath::Max(1, FMath::CeilToInt(FMath::Max(1.0f, LengthFt) / SafeSectionLengthFt));
	}

	const float SafeLengthFt = FMath::Max(1.0f, LengthFt);
	const float SafeMaxCrossbarLengthFt = FMath::Max(1.0f, MaxCrossbarLengthFt);
	return FMath::Max(1, FMath::CeilToInt(SafeLengthFt / SafeMaxCrossbarLengthFt));
}

float ADrapeRunActor::GetSectionLengthFt() const
{
	if (!bAutoCalculateSections)
	{
		return FMath::Max(1.0f, SectionLengthFt);
	}

	const int32 SectionCount = GetSectionCount();
	const float RequestedSectionLengthFt = FMath::Max(1.0f, LengthFt) / FMath::Max(1, SectionCount);
	const float SafeMinCrossbarLengthFt = FMath::Max(1.0f, MinCrossbarLengthFt);
	const float SafeMaxCrossbarLengthFt = FMath::Max(SafeMinCrossbarLengthFt, MaxCrossbarLengthFt);
	return FMath::Clamp(RequestedSectionLengthFt, SafeMinCrossbarLengthFt, SafeMaxCrossbarLengthFt);
}

float ADrapeRunActor::GetSectionLengthCm() const
{
	return GetSectionLengthFt() * CmPerFoot;
}

FString ADrapeRunActor::GetUprightAssetFolder() const
{
	const float HeightInches = FMath::Clamp(HeightFt, 6.0f, 18.333f) * 12.0f;
	if (HeightInches <= 120.0f)
	{
		return Upright6FtAssetFolder;
	}

	if (HeightInches <= 144.0f)
	{
		return Upright8FtAssetFolder;
	}

	return Upright10FtAssetFolder;
}

float ADrapeRunActor::GetInsideUprightNativeHeightCm() const
{
	const float HeightInches = FMath::Clamp(HeightFt, 6.0f, 18.333f) * 12.0f;
	if (HeightInches <= 120.0f)
	{
		return Native72InchPoleInsideHeightCm;
	}

	if (HeightInches <= 144.0f)
	{
		return Native96InchPoleInsideHeightCm;
	}

	return Native120InchPoleInsideHeightCm;
}

float ADrapeRunActor::GetInsideUprightCalibrationOffsetCm() const
{
	const float HeightInches = FMath::Clamp(HeightFt, 6.0f, 18.333f) * 12.0f;
	if (HeightInches <= 120.0f)
	{
		return Inside72InchPoleCalibrationOffsetCm;
	}

	if (HeightInches <= 144.0f)
	{
		return Inside96InchPoleCalibrationOffsetCm;
	}

	return Inside120InchPoleCalibrationOffsetCm;
}

TArray<FSoftObjectPath> ADrapeRunActor::GetMeshPathsForFolder(const FString& AssetFolderPath) const
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
		if (AssetData.AssetClassPath == UStaticMesh::StaticClass()->GetClassPathName())
		{
			MeshPaths.Add(AssetData.ToSoftObjectPath());
		}
	}

	return MeshPaths;
}

TArray<FSoftObjectPath> ADrapeRunActor::GetOutsidePoleMeshPaths() const
{
	TArray<FSoftObjectPath> MeshPaths;
	for (const FSoftObjectPath& MeshPath : GetMeshPathsForFolder(GetUprightAssetFolder()))
	{
		if (MeshPath.GetAssetName().Contains(TEXT("inch_Outside")))
		{
			MeshPaths.Add(MeshPath);
		}
	}
	return MeshPaths;
}

TArray<FSoftObjectPath> ADrapeRunActor::GetInsidePoleMeshPaths() const
{
	TArray<FSoftObjectPath> MeshPaths;
	for (const FSoftObjectPath& MeshPath : GetMeshPathsForFolder(GetUprightAssetFolder()))
	{
		if (MeshPath.GetAssetName().Contains(TEXT("inch_Inside")))
		{
			MeshPaths.Add(MeshPath);
		}
	}
	return MeshPaths;
}

TArray<FSoftObjectPath> ADrapeRunActor::GetBaseMeshPaths() const
{
	return GetMeshPathsForFolder(BaseAssetFolder);
}

TArray<FSoftObjectPath> ADrapeRunActor::GetOutsideRodMeshPaths() const
{
	TArray<FSoftObjectPath> MeshPaths;
	for (const FSoftObjectPath& MeshPath : GetMeshPathsForFolder(RodAssetFolder))
	{
		if (MeshPath.GetAssetName().StartsWith(TEXT("Outside")))
		{
			MeshPaths.Add(MeshPath);
		}
	}
	return MeshPaths;
}

TArray<FSoftObjectPath> ADrapeRunActor::GetInsideRodMeshPaths() const
{
	TArray<FSoftObjectPath> MeshPaths;
	for (const FSoftObjectPath& MeshPath : GetMeshPathsForFolder(RodAssetFolder))
	{
		if (MeshPath.GetAssetName().StartsWith(TEXT("Inside")))
		{
			MeshPaths.Add(MeshPath);
		}
	}
	return MeshPaths;
}

UMaterialInterface* ADrapeRunActor::ResolveDrapeMaterial() const
{
	return DrapeMaterialOverride.IsNull() ? nullptr : DrapeMaterialOverride.LoadSynchronous();
}

UInstancedStaticMeshComponent* ADrapeRunActor::FindOrCreateMeshBucket(UStaticMesh* StaticMesh, const TCHAR* Prefix, bool bApplyDrapeMaterial)
{
	if (!StaticMesh)
	{
		return nullptr;
	}

	const FString MaterialSuffix = bApplyDrapeMaterial && !DrapeMaterialOverride.IsNull()
		? FString::Printf(TEXT("_%s"), *DrapeMaterialOverride.ToSoftObjectPath().GetAssetName())
		: FString();
	const FString BucketKey = FString::Printf(TEXT("%s_%s%s"), Prefix, *StaticMesh->GetPathName(), *MaterialSuffix);
	const uint32 PathHash = FCrc::StrCrc32(*BucketKey);
	const FName ComponentName(*FString::Printf(TEXT("%s_%s_%u"), Prefix, *StaticMesh->GetName(), PathHash));

	if (UInstancedStaticMeshComponent* ExistingComponent = FindGeneratedMeshComponentByName(ComponentName))
	{
		ExistingComponent->SetStaticMesh(StaticMesh);
		ExistingComponent->SetVisibility(true);
		ExistingComponent->SetHiddenInGame(false);
		if (!GeneratedMeshComponents.Contains(ExistingComponent))
		{
			GeneratedMeshComponents.Add(ExistingComponent);
		}
		return ExistingComponent;
	}

	UInstancedStaticMeshComponent* MeshComponent = NewObject<UInstancedStaticMeshComponent>(this, ComponentName);
	if (!MeshComponent)
	{
		return nullptr;
	}

	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->RegisterComponent();
	AddInstanceComponent(MeshComponent);
	GeneratedMeshComponents.Add(MeshComponent);

	MeshComponent->SetStaticMesh(StaticMesh);
	if (bApplyDrapeMaterial)
	{
		if (UMaterialInterface* OverrideMaterial = ResolveDrapeMaterial())
		{
			MeshComponent->SetMaterial(0, OverrideMaterial);
		}
	}

	return MeshComponent;
}

UInstancedStaticMeshComponent* ADrapeRunActor::FindGeneratedMeshComponentByName(const FName& ComponentName) const
{
	for (UInstancedStaticMeshComponent* MeshComponent : GeneratedMeshComponents)
	{
		if (MeshComponent && MeshComponent->GetFName() == ComponentName)
		{
			return MeshComponent;
		}
	}

	return FindObjectFast<UInstancedStaticMeshComponent>(const_cast<ADrapeRunActor*>(this), ComponentName);
}

USkeletalMeshComponent* ADrapeRunActor::FindOrCreateChaosDrapeComponent(int32 SectionIndex)
{
	const FName ComponentName(*FString::Printf(TEXT("%s_%d"), DrapeChaosPrefix, SectionIndex));
	for (USkeletalMeshComponent* ExistingComponent : GeneratedChaosDrapeComponents)
	{
		if (ExistingComponent && ExistingComponent->GetFName() == ComponentName)
		{
			return ExistingComponent;
		}
	}

	if (USkeletalMeshComponent* ExistingComponent = FindObjectFast<USkeletalMeshComponent>(this, ComponentName))
	{
		if (!GeneratedChaosDrapeComponents.Contains(ExistingComponent))
		{
			GeneratedChaosDrapeComponents.Add(ExistingComponent);
		}
		return ExistingComponent;
	}

	USkeletalMeshComponent* DrapeComponent = NewObject<USkeletalMeshComponent>(this, ComponentName);
	if (!DrapeComponent)
	{
		return nullptr;
	}

	DrapeComponent->SetupAttachment(SceneRoot);
	DrapeComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DrapeComponent->RegisterComponent();
	AddInstanceComponent(DrapeComponent);
	GeneratedChaosDrapeComponents.Add(DrapeComponent);
	return DrapeComponent;
}

void ADrapeRunActor::ClearGeneratedComponents()
{
	TInlineComponentArray<UInstancedStaticMeshComponent*> DrapeInstanceComponents(this);
	for (UInstancedStaticMeshComponent* MeshComponent : DrapeInstanceComponents)
	{
		if (!MeshComponent)
		{
			continue;
		}

		if ((MeshComponent->GetName().StartsWith(DrapeBasePrefix) ||
			MeshComponent->GetName().StartsWith(DrapeUprightPrefix) ||
			MeshComponent->GetName().StartsWith(DrapeCrossbarPrefix) ||
			MeshComponent->GetName().StartsWith(DrapePanelPrefix)) &&
			!GeneratedMeshComponents.Contains(MeshComponent))
		{
			GeneratedMeshComponents.Add(MeshComponent);
		}
	}

	for (UInstancedStaticMeshComponent* MeshComponent : GeneratedMeshComponents)
	{
		if (MeshComponent)
		{
			MeshComponent->ClearInstances();
			MeshComponent->SetVisibility(false);
			MeshComponent->SetHiddenInGame(true);
		}
	}

	TInlineComponentArray<USkeletalMeshComponent*> SkeletalComponents(this);
	for (USkeletalMeshComponent* DrapeComponent : SkeletalComponents)
	{
		if (DrapeComponent && DrapeComponent->GetName().StartsWith(DrapeChaosPrefix) && !GeneratedChaosDrapeComponents.Contains(DrapeComponent))
		{
			GeneratedChaosDrapeComponents.Add(DrapeComponent);
		}
	}

	for (USkeletalMeshComponent* DrapeComponent : GeneratedChaosDrapeComponents)
	{
		if (DrapeComponent)
		{
			DrapeComponent->SetVisibility(false);
			DrapeComponent->SetHiddenInGame(true);
		}
	}
}

void ADrapeRunActor::AddStaticMeshInstance(UStaticMesh* StaticMesh, const TCHAR* Prefix, const FTransform& InstanceTransform, bool bApplyDrapeMaterial, FBox& Bounds)
{
	UInstancedStaticMeshComponent* MeshComponent = FindOrCreateMeshBucket(StaticMesh, Prefix, bApplyDrapeMaterial);
	if (!MeshComponent)
	{
		return;
	}

	MeshComponent->AddInstance(InstanceTransform);
	if (StaticMesh)
	{
		Bounds += InstanceTransform.TransformPosition(StaticMesh->GetBoundingBox().GetCenter());
	}
}

void ADrapeRunActor::AddStaticMeshPathInstance(const FSoftObjectPath& MeshPath, const TCHAR* Prefix, const FTransform& InstanceTransform, bool bApplyDrapeMaterial, FBox& Bounds)
{
	UStaticMesh* StaticMesh = Cast<UStaticMesh>(MeshPath.TryLoad());
	AddStaticMeshInstance(StaticMesh, Prefix, InstanceTransform, bApplyDrapeMaterial, Bounds);
}

void ADrapeRunActor::AddMeshPathSet(const TArray<FSoftObjectPath>& MeshPaths, const TCHAR* Prefix, const FTransform& InstanceTransform, bool bApplyDrapeMaterial, FBox& Bounds)
{
	for (const FSoftObjectPath& MeshPath : MeshPaths)
	{
		AddStaticMeshPathInstance(MeshPath, Prefix, InstanceTransform, bApplyDrapeMaterial, Bounds);
	}
}

void ADrapeRunActor::AddChaosDrapeSection(int32 SectionIndex, const FTransform& SectionTransform, FBox& Bounds)
{
	USkeletalMesh* SkeletalMesh = ChaosDrapeMesh.LoadSynchronous();
	if (!SkeletalMesh)
	{
		AddStaticMeshInstance(StaticDrapeMesh.LoadSynchronous(), DrapePanelPrefix, SectionTransform, true, Bounds);
		return;
	}

	USkeletalMeshComponent* DrapeComponent = FindOrCreateChaosDrapeComponent(SectionIndex);
	if (!DrapeComponent)
	{
		return;
	}

	DrapeComponent->SetSkeletalMesh(SkeletalMesh);
	DrapeComponent->SetRelativeTransform(SectionTransform);
	DrapeComponent->SetVisibility(true);
	DrapeComponent->SetHiddenInGame(false);
	if (UMaterialInterface* OverrideMaterial = ResolveDrapeMaterial())
	{
		DrapeComponent->SetMaterial(0, OverrideMaterial);
	}

	Bounds += SectionTransform.GetLocation();
}

void ADrapeRunActor::UpdateSelectionBounds(const FBox& Bounds)
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

	SelectionBounds->SetWorldScale3D(FVector::OneVector);
	SelectionBounds->SetRelativeLocation(Bounds.GetCenter());
	SelectionBounds->SetBoxExtent(Bounds.GetExtent().ComponentMax(FVector(50.0f, 25.0f, 50.0f)));
}
