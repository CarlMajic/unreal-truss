#include "VideoPlacementActor.h"

#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "TrussMathLibrary.h"

namespace
{
constexpr float CmPerFoot = 30.48f;

FSoftObjectPath GetTVMeshPath(EVideoTVModel Model)
{
	switch (Model)
	{
	case EVideoTVModel::Hisense58:
		return FSoftObjectPath(TEXT("/Game/Majic_Gear/TV_s/Hisense_58__4k_UltraHD_TV/StaticMeshes/Hisense_58__4k_UltraHD_TV.Hisense_58__4k_UltraHD_TV"));
	case EVideoTVModel::Insignia43:
		return FSoftObjectPath(TEXT("/Game/Majic_Gear/TV_s/Insignia_43__1080p_LED_TV/StaticMeshes/Insignia_43__1080p_LED_TV.Insignia_43__1080p_LED_TV"));
	case EVideoTVModel::Philips46:
		return FSoftObjectPath(TEXT("/Game/Majic_Gear/TV_s/Philips_46__LED_TV_Monitor/StaticMeshes/Philips_46__LED_TV_Monitor.Philips_46__LED_TV_Monitor"));
	case EVideoTVModel::Samsung22:
		return FSoftObjectPath(TEXT("/Game/Majic_Gear/TV_s/Samsung_22__HDTV_T22C350_LED_Monitor/StaticMeshes/Samsung_22__HDTV_T22C350_LED_Monitor.Samsung_22__HDTV_T22C350_LED_Monitor"));
	case EVideoTVModel::Samsung55Outdoor:
		return FSoftObjectPath(TEXT("/Game/Majic_Gear/TV_s/Samsung_55__4K_Outdoor_Rated_TV/StaticMeshes/Samsung_55__4K_Outdoor_Rated_TV.Samsung_55__4K_Outdoor_Rated_TV"));
	case EVideoTVModel::Samsung60:
		return FSoftObjectPath(TEXT("/Game/Majic_Gear/TV_s/Samsung_60__HD_1080p_LED_TV/StaticMeshes/Samsung_60__HD_1080p_LED_TV.Samsung_60__HD_1080p_LED_TV"));
	case EVideoTVModel::Samsung82Crystal:
		return FSoftObjectPath(TEXT("/Game/Majic_Gear/TV_s/Samsung_Crystal_UHD_82__4K_TV_Monitor/StaticMeshes/Samsung_Crystal_UHD_82__4K_TV_Monitor.Samsung_Crystal_UHD_82__4K_TV_Monitor"));
	case EVideoTVModel::Samsung82Smart:
		return FSoftObjectPath(TEXT("/Game/Majic_Gear/TV_s/Samsung__82__Smart_4K_TV_Monitor/StaticMeshes/Samsung__82__Smart_4K_TV_Monitor.Samsung__82__Smart_4K_TV_Monitor"));
	case EVideoTVModel::Sharp55:
		return FSoftObjectPath(TEXT("/Game/Majic_Gear/TV_s/Sharp_55__4K_LED_TV/StaticMeshes/Sharp_55__4K_LED_TV.Sharp_55__4K_LED_TV"));
	case EVideoTVModel::Sharp60:
		return FSoftObjectPath(TEXT("/Game/Majic_Gear/TV_s/Sharp_60__4K_LED_TV/StaticMeshes/Sharp_60__4K_LED_TV.Sharp_60__4K_LED_TV"));
	case EVideoTVModel::Sharp80:
		return FSoftObjectPath(TEXT("/Game/Majic_Gear/TV_s/Sharp_80__LED_TV/StaticMeshes/Sharp_80__LED_TV.Sharp_80__LED_TV"));
	case EVideoTVModel::Sharp90:
		return FSoftObjectPath(TEXT("/Game/Majic_Gear/TV_s/Sharp_90__LED_TV/StaticMeshes/Sharp_90__LED_TV.Sharp_90__LED_TV"));
	case EVideoTVModel::Vizio70:
		return FSoftObjectPath(TEXT("/Game/Majic_Gear/TV_s/Vizio_70__LED_TV/StaticMeshes/Vizio_70__LED_TV.Vizio_70__LED_TV"));
	case EVideoTVModel::Benq25Preview:
		return FSoftObjectPath(TEXT("/Game/Majic_Gear/TV_s/Benq_25_Preview_Monitor/StaticMeshes/Monitor.Monitor"));
	case EVideoTVModel::Samsung58:
	default:
		return FSoftObjectPath(TEXT("/Game/Majic_Gear/TV_s/Samsung_58__UHD_6_Series_Smart_TV/StaticMeshes/Samsung_58__UHD_6_Series_Smart_TV.Samsung_58__UHD_6_Series_Smart_TV"));
	}
}

UStaticMesh* LoadMesh(const TCHAR* AssetPath)
{
	return Cast<UStaticMesh>(FSoftObjectPath(AssetPath).TryLoad());
}
}

AVideoPlacementActor::AVideoPlacementActor()
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

	TenFootTrussInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("TenFootTrussInstances"));
	TenFootTrussInstances->SetupAttachment(SceneRoot);

	EightFootTrussInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("EightFootTrussInstances"));
	EightFootTrussInstances->SetupAttachment(SceneRoot);

	FiveFootTrussInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FiveFootTrussInstances"));
	FiveFootTrussInstances->SetupAttachment(SceneRoot);

	FourFootTrussInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FourFootTrussInstances"));
	FourFootTrussInstances->SetupAttachment(SceneRoot);

	TwoFootTrussInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("TwoFootTrussInstances"));
	TwoFootTrussInstances->SetupAttachment(SceneRoot);

	BaseInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("BaseInstances"));
	BaseInstances->SetupAttachment(SceneRoot);

	TVComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TVComponent"));
	TVComponent->SetupAttachment(SceneRoot);
	TVComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	const TCHAR* UPMPartNames[] = {
		TEXT("Bolt"),
		TEXT("Bolt1"),
		TEXT("Burger"),
		TEXT("Burger1"),
		TEXT("Clamp"),
		TEXT("Clamp1"),
		TEXT("UPM_Mount"),
		TEXT("Wing"),
		TEXT("Wing1")
	};

	for (const TCHAR* PartName : UPMPartNames)
	{
		UStaticMeshComponent* UpperComponent = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("UpperUPM_%s"), PartName));
		UpperComponent->SetupAttachment(SceneRoot);
		UpperComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		UpperUPMComponents.Add(UpperComponent);

		UStaticMeshComponent* LowerComponent = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("LowerUPM_%s"), PartName));
		LowerComponent->SetupAttachment(SceneRoot);
		LowerComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		LowerUPMComponents.Add(LowerComponent);
	}
}

void AVideoPlacementActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (bBuildOnConstruction)
	{
		RebuildVideoPlacement();
	}
}

#if WITH_EDITOR
void AVideoPlacementActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RebuildVideoPlacement();
}
#endif

void AVideoPlacementActor::RebuildVideoPlacement()
{
	ClearGenerated();

	FTrussPieceDefinition BaseDefinition;
	UStaticMesh* BaseMesh = nullptr;
	float BaseHeightCm = 0.0f;
	if (GetPieceDefinition(ETrussPieceType::Base, BaseDefinition, BaseMesh) && BaseMesh)
	{
		BaseInstances->SetStaticMesh(BaseMesh);
		BaseInstances->bDisallowNanite = true;
		const FVector BaseExtent = GetScaledRotatedMeshExtent(BaseMesh, FRotator::ZeroRotator);
		BaseHeightCm = BaseExtent.Z;
		const FVector BaseMin(-BaseExtent.X * 0.5f, -BaseExtent.Y * 0.5f, 0.0f);
		const FVector BaseLocation = GetMeshPlacementLocation(BaseMesh, BaseMin, FRotator::ZeroRotator);
		BaseInstances->AddInstance(FTransform(FRotator::ZeroRotator, BaseLocation, FVector(TrussMeshScaleMultiplier)));
		ExpandGeneratedBounds(FBox(BaseMin, BaseMin + BaseExtent));
	}

	const float RequestedTowerCm = FMath::Max(2.0f, TowerHeightFt) * CmPerFoot;
	const FTrussCombinationResult TowerCombination = UTrussMathLibrary::FindBestTrussCombination(FMath::Max(0.0f, RequestedTowerCm - BaseHeightCm));
	float CurrentZ = BaseHeightCm;
	for (ETrussPieceType PieceType : TowerCombination.Pieces)
	{
		AddTowerPiece(PieceType, TowerPlacementOffsetCm + FVector(0.0f, 0.0f, CurrentZ), TowerRotation);
		CurrentZ += UTrussMathLibrary::GetDefaultPieceLengthCm(PieceType);
	}
	CurrentActualTowerHeightFt = CurrentZ / CmPerFoot;

	if (UStaticMesh* TVMesh = LoadTVMesh(TVModel))
	{
		const FVector TVLocation = FVector(0.0f, 0.0f, FMath::Max(1.0f, TVCenterHeightFt) * CmPerFoot) + TVPlacementOffsetCm;
		TVComponent->SetStaticMesh(TVMesh);
		TVComponent->SetRelativeLocation(TVLocation);
		TVComponent->SetRelativeRotation(TVPlacementRotation);
		TVComponent->SetRelativeScale3D(TVScale);
		TVComponent->SetVisibility(true);
		TVComponent->SetHiddenInGame(false);

		const FBox TVBounds = TVMesh->GetBoundingBox().TransformBy(TVComponent->GetRelativeTransform());
		ExpandGeneratedBounds(TVBounds);
		LoadUPMMeshes();
		PlaceUPMComponents(TVLocation);
	}
	else
	{
		TVComponent->SetStaticMesh(nullptr);
	}

	UpdateSelectionBounds();
}

void AVideoPlacementActor::ApplyBuildDefinition(const FVideoPlacementBuildDefinition& Definition, bool bRebuildNow)
{
	TVModel = Definition.TVModel;
	TVCenterHeightFt = FMath::Max(1.0f, Definition.TVCenterHeightFt);
	TowerHeightFt = FMath::Max(2.0f, Definition.TowerHeightFt);

	if (bRebuildNow)
	{
		RebuildVideoPlacement();
	}
}

FVideoPlacementBuildDefinition AVideoPlacementActor::GetBuildDefinition() const
{
	FVideoPlacementBuildDefinition Definition;
	Definition.TVModel = TVModel;
	Definition.TVCenterHeightFt = TVCenterHeightFt;
	Definition.TowerHeightFt = TowerHeightFt;
	return Definition;
}

void AVideoPlacementActor::SetSelectionHighlighted(bool bHighlighted)
{
	if (SelectionBounds)
	{
		SelectionBounds->SetHiddenInGame(!bHighlighted);
	}
}

void AVideoPlacementActor::ClearGenerated()
{
	GeneratedBounds = FBox(EForceInit::ForceInit);

	for (UInstancedStaticMeshComponent* Component : {TenFootTrussInstances.Get(), EightFootTrussInstances.Get(), FiveFootTrussInstances.Get(), FourFootTrussInstances.Get(), TwoFootTrussInstances.Get(), BaseInstances.Get()})
	{
		if (Component)
		{
			Component->ClearInstances();
			Component->SetVisibility(true);
			Component->SetHiddenInGame(false);
		}
	}

	for (UStaticMeshComponent* Component : UpperUPMComponents)
	{
		if (Component)
		{
			Component->SetVisibility(false);
			Component->SetHiddenInGame(true);
		}
	}

	for (UStaticMeshComponent* Component : LowerUPMComponents)
	{
		if (Component)
		{
			Component->SetVisibility(false);
			Component->SetHiddenInGame(true);
		}
	}
}

void AVideoPlacementActor::AddTowerPiece(ETrussPieceType PieceType, const FVector& TargetMinLocation, const FRotator& Rotation)
{
	FTrussPieceDefinition PieceDefinition;
	UStaticMesh* StaticMesh = nullptr;
	if (!GetPieceDefinition(PieceType, PieceDefinition, StaticMesh) || !StaticMesh)
	{
		return;
	}

	UInstancedStaticMeshComponent* Component = GetMeshComponentForPiece(PieceType);
	if (!Component)
	{
		return;
	}

	Component->SetStaticMesh(StaticMesh);
	Component->bDisallowNanite = true;
	const FVector Location = GetMeshPlacementLocation(StaticMesh, TargetMinLocation, Rotation);
	Component->AddInstance(FTransform(Rotation, Location, FVector(TrussMeshScaleMultiplier)));
	ExpandGeneratedBounds(FBox(TargetMinLocation, TargetMinLocation + GetScaledRotatedMeshExtent(StaticMesh, Rotation)));
}

UInstancedStaticMeshComponent* AVideoPlacementActor::GetMeshComponentForPiece(ETrussPieceType PieceType) const
{
	switch (PieceType)
	{
	case ETrussPieceType::TenFoot:
		return TenFootTrussInstances;
	case ETrussPieceType::EightFoot:
		return EightFootTrussInstances;
	case ETrussPieceType::FiveFoot:
		return FiveFootTrussInstances;
	case ETrussPieceType::FourFoot:
		return FourFootTrussInstances;
	case ETrussPieceType::TwoFoot:
		return TwoFootTrussInstances;
	case ETrussPieceType::Base:
		return BaseInstances;
	default:
		return nullptr;
	}
}

UStaticMesh* AVideoPlacementActor::LoadMajicGearDefaultMesh(ETrussPieceType PieceType) const
{
	const TCHAR* AssetPath = nullptr;
	switch (PieceType)
	{
	case ETrussPieceType::TenFoot:
		AssetPath = TEXT("/Game/Majic_Gear/Truss/10ftTruss/StaticMeshes/SM__0_ft_Truss_v2.SM__0_ft_Truss_v2");
		break;
	case ETrussPieceType::EightFoot:
		AssetPath = TEXT("/Game/Majic_Gear/Truss/8ftTruss/StaticMeshes/SM___ft_Truss_v1.SM___ft_Truss_v1");
		break;
	case ETrussPieceType::FiveFoot:
		AssetPath = TEXT("/Game/Majic_Gear/Truss/5ftTruss/StaticMeshes/SM___ft_Truss_v1.SM___ft_Truss_v1");
		break;
	case ETrussPieceType::FourFoot:
		AssetPath = TEXT("/Game/Majic_Gear/Truss/4ftTruss/StaticMeshes/SM___ft_Truss_v2.SM___ft_Truss_v2");
		break;
	case ETrussPieceType::TwoFoot:
		AssetPath = TEXT("/Game/Majic_Gear/Truss/2ftTruss/StaticMeshes/SM___ft_Truss_v1.SM___ft_Truss_v1");
		break;
	case ETrussPieceType::Base:
		AssetPath = TEXT("/Game/Majic_Gear/Truss/Base/StaticMeshes/SM_Base_v2.SM_Base_v2");
		break;
	default:
		break;
	}

	return AssetPath ? Cast<UStaticMesh>(FSoftObjectPath(AssetPath).TryLoad()) : nullptr;
}

UStaticMesh* AVideoPlacementActor::LoadTVMesh(EVideoTVModel Model) const
{
	return Cast<UStaticMesh>(GetTVMeshPath(Model).TryLoad());
}

void AVideoPlacementActor::LoadUPMMeshes()
{
	const TCHAR* UPMAssetPaths[] = {
		TEXT("/Game/Majic_Gear/Rigging/UPM/StaticMeshes/Bolt.Bolt"),
		TEXT("/Game/Majic_Gear/Rigging/UPM/StaticMeshes/Bolt1.Bolt1"),
		TEXT("/Game/Majic_Gear/Rigging/UPM/StaticMeshes/Burger.Burger"),
		TEXT("/Game/Majic_Gear/Rigging/UPM/StaticMeshes/Burger1.Burger1"),
		TEXT("/Game/Majic_Gear/Rigging/UPM/StaticMeshes/Clamp.Clamp"),
		TEXT("/Game/Majic_Gear/Rigging/UPM/StaticMeshes/Clamp1.Clamp1"),
		TEXT("/Game/Majic_Gear/Rigging/UPM/StaticMeshes/UPM_Mount.UPM_Mount"),
		TEXT("/Game/Majic_Gear/Rigging/UPM/StaticMeshes/Wing.Wing"),
		TEXT("/Game/Majic_Gear/Rigging/UPM/StaticMeshes/Wing1.Wing1")
	};

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(UPMAssetPaths); ++Index)
	{
		UStaticMesh* Mesh = LoadMesh(UPMAssetPaths[Index]);
		if (UpperUPMComponents.IsValidIndex(Index) && UpperUPMComponents[Index])
		{
			UpperUPMComponents[Index]->SetStaticMesh(Mesh);
		}
		if (LowerUPMComponents.IsValidIndex(Index) && LowerUPMComponents[Index])
		{
			LowerUPMComponents[Index]->SetStaticMesh(Mesh);
		}
	}
}

void AVideoPlacementActor::PlaceUPMComponents(const FVector& TVCenterLocation)
{
	auto PlaceComponents = [this](const TArray<TObjectPtr<UStaticMeshComponent>>& Components, const FVector& Location)
	{
		for (UStaticMeshComponent* Component : Components)
		{
			if (!Component || !Component->GetStaticMesh())
			{
				continue;
			}

			Component->SetRelativeLocation(Location);
			Component->SetRelativeRotation(UPMPlacementRotation);
			Component->SetRelativeScale3D(UPMScale);
			Component->SetVisibility(true);
			Component->SetHiddenInGame(false);
			ExpandGeneratedBounds(Component->GetStaticMesh()->GetBoundingBox().TransformBy(Component->GetRelativeTransform()));
		}
	};

	PlaceComponents(UpperUPMComponents, TVCenterLocation + UPMPlacementOffsetCm + FVector(0.0f, 0.0f, UPMVerticalSpacingCm));
	PlaceComponents(LowerUPMComponents, TVCenterLocation + UPMPlacementOffsetCm - FVector(0.0f, 0.0f, UPMVerticalSpacingCm));
}

bool AVideoPlacementActor::GetPieceDefinition(ETrussPieceType PieceType, FTrussPieceDefinition& OutPiece, UStaticMesh*& OutMesh) const
{
	if (Inventory && Inventory->FindPiece(PieceType, OutPiece))
	{
		OutMesh = OutPiece.StaticMesh;
		return OutPiece.LengthCm > 0.0f;
	}

	if (bUseMajicGearDefaultMeshes)
	{
		OutPiece.PieceType = PieceType;
		OutPiece.LengthCm = UTrussMathLibrary::GetDefaultPieceLengthCm(PieceType);
		OutPiece.StaticMesh = LoadMajicGearDefaultMesh(PieceType);
		OutMesh = OutPiece.StaticMesh;
		return OutMesh != nullptr;
	}

	OutMesh = nullptr;
	return false;
}

FVector AVideoPlacementActor::GetMeshPlacementLocation(UStaticMesh* StaticMesh, const FVector& TargetMinLocation, const FRotator& Rotation) const
{
	if (!StaticMesh)
	{
		return TargetMinLocation;
	}

	const FBox LocalBounds = StaticMesh->GetBoundingBox();
	const FTransform RotationTransform(Rotation, FVector::ZeroVector, FVector(TrussMeshScaleMultiplier));
	const FBox RotatedScaledBounds = LocalBounds.TransformBy(RotationTransform.ToMatrixWithScale());
	return TargetMinLocation - RotatedScaledBounds.Min;
}

FVector AVideoPlacementActor::GetScaledRotatedMeshExtent(UStaticMesh* StaticMesh, const FRotator& Rotation) const
{
	if (!StaticMesh)
	{
		return FVector(30.48f, 30.48f, 30.48f);
	}

	const FBox LocalBounds = StaticMesh->GetBoundingBox();
	const FTransform RotationTransform(Rotation, FVector::ZeroVector, FVector(TrussMeshScaleMultiplier));
	const FBox RotatedScaledBounds = LocalBounds.TransformBy(RotationTransform.ToMatrixWithScale());
	return RotatedScaledBounds.GetSize();
}

void AVideoPlacementActor::ExpandGeneratedBounds(const FBox& Bounds)
{
	if (Bounds.IsValid)
	{
		GeneratedBounds += Bounds;
	}
}

void AVideoPlacementActor::UpdateSelectionBounds()
{
	if (!SelectionBounds)
	{
		return;
	}

	if (!GeneratedBounds.IsValid)
	{
		SelectionBounds->SetRelativeLocation(FVector::ZeroVector);
		SelectionBounds->SetBoxExtent(FVector(50.0f, 50.0f, 50.0f));
		return;
	}

	SelectionBounds->SetWorldScale3D(FVector::OneVector);
	SelectionBounds->SetRelativeLocation(GeneratedBounds.GetCenter());
	SelectionBounds->SetBoxExtent(GeneratedBounds.GetExtent().ComponentMax(FVector(50.0f, 50.0f, 50.0f)));
}
