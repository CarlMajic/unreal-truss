#include "ProjectionScreenActor.h"

#include "Components/BoxComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Math/RotationMatrix.h"
#include "TrussMathLibrary.h"

namespace
{
constexpr float ProjectionCmPerFoot = 30.48f;
constexpr float ProjectionInchesPerFoot = 12.0f;
constexpr float ChristieM4K25IsoLumens = 25300.0f;
constexpr float TrussBuilderArchCornerConnectionOffsetCm = 15.24f;
constexpr float TrussBuilderArchLegYOffsetCm = 15.24f;
constexpr float TrussBuilderArchVerticalLegXOffsetCm = 30.48f;
constexpr float TrussBuilderArchBaseYOffsetCm = 30.48f;
constexpr float TrussBuilderArchVerticalRotationXDeg = 0.0f;
constexpr float TrussBuilderArchVerticalRotationYDeg = 90.0f;
constexpr float TrussBuilderArchVerticalRotationZDeg = 0.0f;

UStaticMesh* LoadProjectionMesh(const TCHAR* AssetPath)
{
	return Cast<UStaticMesh>(FSoftObjectPath(AssetPath).TryLoad());
}
}

AProjectionScreenActor::AProjectionScreenActor()
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

	ScreenKitComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("ScreenKit"));
	ScreenKitComponent->SetupAttachment(SceneRoot);

	ProjectionLightComponent = CreateDefaultSubobject<USpotLightComponent>(TEXT("ProjectionLight"));
	ProjectionLightComponent->SetupAttachment(SceneRoot);
	ProjectionLightComponent->SetVisibility(true);
	ProjectionLightComponent->SetHiddenInGame(false);
	ProjectionLightComponent->SetCastShadows(false);

	ScreenCenterMarkerComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ScreenCenterMarker"));
	ScreenCenterMarkerComponent->SetupAttachment(SceneRoot);
	ScreenCenterMarkerComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ScreenCenterMarkerComponent->SetHiddenInGame(false);
	ScreenCenterMarkerComponent->bDisallowNanite = true;
	ScreenCenterMarkerComponent->SetStaticMesh(LoadProjectionMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere")));

	ProjectionLightFunctionMaterial = Cast<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Majic_Gear/Projection/Projectors/LFM_Testpattern.LFM_Testpattern")).TryLoad());
}

void AProjectionScreenActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (bBuildOnConstruction)
	{
		RebuildProjectionScreen();
	}
}

#if WITH_EDITOR
void AProjectionScreenActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RebuildProjectionScreen();
}
#endif

void AProjectionScreenActor::RebuildProjectionScreen()
{
	GetScreenDimensionsFt(CurrentScreenWidthFt, CurrentScreenHeightFt);
	ComputeThrowDistances(CurrentScreenWidthFt, CurrentMinThrowDistanceFt, CurrentMaxThrowDistanceFt);
	CurrentThrowDistanceFt = ResolveThrowDistanceFt();
	GetLensShiftPercent(
		CurrentLensVerticalShiftMinPercent,
		CurrentLensVerticalShiftMaxPercent,
		CurrentLensHorizontalShiftMinPercent,
		CurrentLensHorizontalShiftMaxPercent,
		bCurrentLensShiftDataAvailable);

	BuildScreenKit();
	BuildProjector();
	UpdateSelectionBounds();
}

void AProjectionScreenActor::ApplyBuildDefinition(const FProjectionScreenBuildDefinition& Definition, bool bRebuildNow)
{
	ScreenKitSize = Definition.ScreenKitSize;
	LensType = Definition.LensType;
	ProjectorType = Definition.ProjectorType;
	ThrowPosition = Definition.ThrowPosition;
	ManualThrowDistanceFt = FMath::Max(1.0f, Definition.ManualThrowDistanceFt);
	MountMode = Definition.MountMode;
	SlingType = Definition.SlingType;
	TowerHeightFt = Definition.TowerHeightFt;
	ProjectorHorizontalOffsetFt = Definition.ProjectorHorizontalOffsetFt;
	ScreenCenterOffsetCm = Definition.ScreenCenterOffsetCm;

	if (bRebuildNow)
	{
		RebuildProjectionScreen();
	}
}

FProjectionScreenBuildDefinition AProjectionScreenActor::GetBuildDefinition() const
{
	FProjectionScreenBuildDefinition Definition;
	Definition.ScreenKitSize = ScreenKitSize;
	Definition.LensType = LensType;
	Definition.ProjectorType = ProjectorType;
	Definition.ThrowPosition = ThrowPosition;
	Definition.ManualThrowDistanceFt = ManualThrowDistanceFt;
	Definition.MountMode = MountMode;
	Definition.SlingType = SlingType;
	Definition.TowerHeightFt = TowerHeightFt;
	Definition.ProjectorHorizontalOffsetFt = ProjectorHorizontalOffsetFt;
	Definition.ScreenCenterOffsetCm = ScreenCenterOffsetCm;
	return Definition;
}

void AProjectionScreenActor::SetSelectionHighlighted(bool bHighlighted)
{
	if (SelectionBounds)
	{
		SelectionBounds->SetHiddenInGame(!bHighlighted);
	}
}

void AProjectionScreenActor::ClearProjectorComponents()
{
	for (UStaticMeshComponent* Component : ProjectorComponents)
	{
		if (Component)
		{
			Component->DestroyComponent();
		}
	}
	ProjectorComponents.Reset();
}

void AProjectionScreenActor::ClearMountComponents()
{
	for (UStaticMeshComponent* Component : MountComponents)
	{
		if (Component)
		{
			Component->DestroyComponent();
		}
	}
	MountComponents.Reset();
}

void AProjectionScreenActor::BuildScreenKit()
{
	if (!ScreenKitComponent)
	{
		return;
	}

	ScreenKitComponent->SetChildActorClass(LoadScreenKitClass());
	ScreenKitComponent->SetRelativeLocation(ScreenKitPlacementOffsetCm);
	ScreenKitComponent->SetRelativeRotation(ScreenKitPlacementRotation);
	ScreenKitComponent->SetRelativeScale3D(ScreenKitScale);
}

void AProjectionScreenActor::BuildProjector()
{
	ClearProjectorComponents();
	ClearMountComponents();

	const FVector ScreenCenterLocation = FVector(0.0f, 0.0f, CurrentScreenHeightFt * ProjectionCmPerFoot * 0.5f) + ScreenCenterOffsetCm;
	CurrentScreenCenterHeightFt = ScreenCenterLocation.Z / ProjectionCmPerFoot;
	const float ResolvedProjectorHeightFt = ResolveProjectorHeightFt(CurrentScreenCenterHeightFt, CurrentScreenHeightFt);
	const float ResolvedHorizontalOffsetFt = ResolveProjectorHorizontalOffsetFt(CurrentScreenWidthFt);
	const FVector ProjectorLensLocation(
		ScreenCenterLocation.X + (ResolvedHorizontalOffsetFt * ProjectionCmPerFoot),
		ScreenCenterLocation.Y - (CurrentThrowDistanceFt * ProjectionCmPerFoot),
		ResolvedProjectorHeightFt * ProjectionCmPerFoot);
	const FVector ProjectorLocation = ProjectorLensLocation + FVector(0.0f, ProjectorPlacementOffsetCm.Y, ProjectorHeightOffsetCm);
	const FRotator AimRotation = FRotationMatrix::MakeFromXZ((ScreenCenterLocation - ProjectorLocation).GetSafeNormal(), FVector::UpVector).Rotator();
	const FRotator ProjectorRotation = SlingType == EProjectionProjectorSlingType::OverSlung
		? OverSlungProjectorRotation
		: UnderSlungProjectorRotation;

	if (MountMode != EProjectionProjectorMountMode::None)
	{
		BuildMountSupport(ProjectorLocation, AimRotation);
	}

	const TArray<UStaticMesh*> ProjectorMeshes = LoadChristieProjectorMeshes();
	for (int32 Index = 0; Index < ProjectorMeshes.Num(); ++Index)
	{
		UStaticMesh* Mesh = ProjectorMeshes[Index];
		if (!Mesh)
		{
			continue;
		}

		AddStaticMeshComponent(ProjectorComponents, Mesh, TEXT("ChristieM4K25Part"), ProjectorLocation, ProjectorRotation, ProjectorScale);
	}

	UpdateProjectionLight(ProjectorLocation, ScreenCenterLocation);
	UpdateScreenCenterMarker(ScreenCenterLocation);
}

void AProjectionScreenActor::BuildMountSupport(const FVector& ProjectorLocation, const FRotator& AimRotation)
{
	const FVector BaseLocation = ProjectorLocation + MountPlacementOffsetCm;
	if (MountMode == EProjectionProjectorMountMode::None)
	{
		return;
	}

	if (MountMode == EProjectionProjectorMountMode::AVCart)
	{
		for (UStaticMesh* Mesh : LoadAVCartMeshes())
		{
			AddStaticMeshComponent(MountComponents, Mesh, TEXT("AVCartPart"), BaseLocation, MountPlacementRotation, MountScale);
		}
		return;
	}

	if (MountMode == EProjectionProjectorMountMode::TrussTower)
	{
		BuildTrussTowerSupport(ProjectorLocation);
		return;
	}

	const float RequestedRunCm = FMath::Max(2.0f, HangingTrussLengthFt) * ProjectionCmPerFoot;
	const FTrussCombinationResult RunCombination = UTrussMathLibrary::FindBestTrussCombination(RequestedRunCm);
	float CurrentY = -RequestedRunCm * 0.5f;
	for (ETrussPieceType PieceType : RunCombination.Pieces)
	{
		UStaticMesh* PieceMesh = nullptr;
		float PieceLengthCm = 0.0f;
		GetPieceDefinition(PieceType, PieceMesh, PieceLengthCm);
		if (PieceMesh)
		{
			AddStaticMeshComponent(
				MountComponents,
				PieceMesh,
				TEXT("ProjectorHangingTruss"),
				BaseLocation + FVector(0.0f, CurrentY, 0.0f),
				AimRotation,
				FVector(TrussMeshScaleMultiplier));
		}
		CurrentY += PieceLengthCm;
	}
}

void AProjectionScreenActor::BuildTrussTowerSupport(const FVector& ProjectorLocation)
{
	const FVector TowerAnchor(ProjectorLocation.X, ProjectorLocation.Y, 0.0f);
	const FRotator BaseRotation = MountPlacementRotation;
	const FRotator TowerRotation = MountPlacementRotation + FRotator(
		TrussBuilderArchVerticalRotationYDeg,
		TrussBuilderArchVerticalRotationZDeg,
		TrussBuilderArchVerticalRotationXDeg);
	const FVector TowerPlacementOffset = MountPlacementOffsetCm;

	UStaticMesh* BaseMesh = nullptr;
	float BaseLengthCm = 0.0f;
	float BaseHeightCm = 0.0f;
	GetPieceDefinition(ETrussPieceType::Base, BaseMesh, BaseLengthCm);
	if (BaseMesh)
	{
		const FVector BaseExtent = GetScaledRotatedTrussMeshExtent(BaseMesh, BaseRotation);
		BaseHeightCm = BaseExtent.Z;
		const float TowerLegX = TrussBuilderArchVerticalLegXOffsetCm - TrussBuilderArchCornerConnectionOffsetCm;
		const FVector BaseMin = TowerAnchor + TowerPlacementOffset + FVector(
			TowerLegX - BaseExtent.X * 0.5f,
			TrussBuilderArchBaseYOffsetCm - BaseExtent.Y * 0.5f,
			0.0f) + TrussTowerBaseOffsetCm;
		const FVector BaseLocation = GetTrussMeshPlacementLocation(BaseMesh, BaseMin, BaseRotation);
		AddStaticMeshComponent(MountComponents, BaseMesh, TEXT("ProjectorTowerBase"), BaseLocation, BaseRotation, FVector(TrussMeshScaleMultiplier));
	}

	const float RequestedTowerCm = FMath::Max(2.0f, CurrentActualBuildableProjectorHeightFt) * ProjectionCmPerFoot;
	const FTrussCombinationResult TowerCombination = UTrussMathLibrary::FindBestTrussCombination(FMath::Max(0.0f, RequestedTowerCm - BaseHeightCm));
	float CurrentZ = BaseHeightCm;
	const float TowerLegX = TrussBuilderArchVerticalLegXOffsetCm - TrussBuilderArchCornerConnectionOffsetCm;
	for (ETrussPieceType PieceType : TowerCombination.Pieces)
	{
		AddTrussTowerPiece(
			PieceType,
			TowerAnchor + TowerPlacementOffset + FVector(TowerLegX, TrussBuilderArchLegYOffsetCm, CurrentZ) + TrussTowerStickOffsetCm,
			TowerRotation);
		CurrentZ += UTrussMathLibrary::GetDefaultPieceLengthCm(PieceType);
	}
}

void AProjectionScreenActor::AddTrussTowerPiece(ETrussPieceType PieceType, const FVector& TargetMinLocation, const FRotator& Rotation)
{
	UStaticMesh* StaticMesh = nullptr;
	float PieceLengthCm = 0.0f;
	GetPieceDefinition(PieceType, StaticMesh, PieceLengthCm);
	if (!StaticMesh)
	{
		return;
	}

	const FVector Location = GetTrussMeshPlacementLocation(StaticMesh, TargetMinLocation, Rotation);
	AddStaticMeshComponent(MountComponents, StaticMesh, TEXT("ProjectorTowerTruss"), Location, Rotation, FVector(TrussMeshScaleMultiplier));
}

void AProjectionScreenActor::AddStaticMeshComponent(TArray<TObjectPtr<UStaticMeshComponent>>& ComponentArray, UStaticMesh* Mesh, const FString& NamePrefix, const FVector& Location, const FRotator& Rotation, const FVector& Scale, bool bDisallowNanite)
{
	if (!Mesh)
	{
		return;
	}

	UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this, *FString::Printf(TEXT("%s_%d"), *NamePrefix, ComponentArray.Num()));
	if (!Component)
	{
		return;
	}

	Component->SetupAttachment(SceneRoot);
	Component->SetStaticMesh(Mesh);
	Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Component->bDisallowNanite = bDisallowNanite;
	Component->SetRelativeLocation(Location);
	Component->SetRelativeRotation(Rotation);
	Component->SetRelativeScale3D(Scale);
	Component->RegisterComponent();
	AddInstanceComponent(Component);
	ComponentArray.Add(Component);
}

void AProjectionScreenActor::UpdateProjectionLight(const FVector& ProjectorLocation, const FVector& ScreenCenterLocation)
{
	if (!ProjectionLightComponent)
	{
		return;
	}

	const FVector LightLocation = ProjectorLocation + ProjectionLightPlacementOffsetCm;
	const FVector LightTargetLocation = ScreenCenterLocation + ProjectionLightTargetOffsetCm;
	const float LightThrowDistanceCm = FVector::Distance(LightLocation, LightTargetLocation);
	const float HalfScreenWidthCm = CurrentScreenWidthFt * ProjectionCmPerFoot * 0.5f;
	const float HalfScreenHeightCm = CurrentScreenHeightFt * ProjectionCmPerFoot * 0.5f;
	const float AutoConeAngleDegrees = FMath::RadiansToDegrees(FMath::Atan2(FMath::Max(HalfScreenWidthCm, HalfScreenHeightCm), FMath::Max(LightThrowDistanceCm, 1.0f)));
	CurrentProjectionLightConeAngleDegrees = bAutoProjectionLightConeAngle
		? FMath::Clamp(AutoConeAngleDegrees, 1.0f, 89.0f)
		: FMath::Clamp(ProjectionLightConeAngleDegrees, 1.0f, 89.0f);

	ProjectionLightComponent->SetVisibility(bEnableProjectionLight);
	ProjectionLightComponent->SetHiddenInGame(!bEnableProjectionLight);
	ProjectionLightComponent->SetRelativeLocation(LightLocation);
	ProjectionLightComponent->SetRelativeRotation((LightTargetLocation - LightLocation).Rotation());
	ProjectionLightComponent->SetIntensity(ChristieM4K25IsoLumens * FMath::Max(0.0f, ProjectionLightIntensityScale));
	ProjectionLightComponent->SetAttenuationRadius(FMath::Max(CurrentThrowDistanceFt * ProjectionCmPerFoot * 1.25f, 100.0f));
	ProjectionLightComponent->SetInnerConeAngle(FMath::Clamp(CurrentProjectionLightConeAngleDegrees * 0.65f, 1.0f, 85.0f));
	ProjectionLightComponent->SetOuterConeAngle(CurrentProjectionLightConeAngleDegrees);
	ProjectionLightComponent->SetLightColor(FLinearColor(0.82f, 0.90f, 1.0f));
	ProjectionLightComponent->SetLightFunctionMaterial(ProjectionLightFunctionMaterial);
	ProjectionLightComponent->SetLightFunctionScale(ProjectionLightFunctionScale);
	ProjectionLightComponent->SetLightFunctionFadeDistance(ProjectionLightFunctionFadeDistance);
}

void AProjectionScreenActor::UpdateScreenCenterMarker(const FVector& ScreenCenterLocation)
{
	if (!ScreenCenterMarkerComponent)
	{
		return;
	}

	ScreenCenterMarkerComponent->SetRelativeLocation(ScreenCenterLocation);
	ScreenCenterMarkerComponent->SetRelativeRotation(FRotator::ZeroRotator);
	ScreenCenterMarkerComponent->SetRelativeScale3D(ScreenCenterMarkerScale);
	ScreenCenterMarkerComponent->SetVisibility(bShowScreenCenterMarker);
	ScreenCenterMarkerComponent->SetHiddenInGame(!bShowScreenCenterMarker);
}

void AProjectionScreenActor::UpdateSelectionBounds()
{
	if (!SelectionBounds)
	{
		return;
	}

	FBox Bounds(EForceInit::ForceInit);
	if (ScreenKitComponent && ScreenKitComponent->GetChildActor())
	{
		ExpandBoundsFromActor(ScreenKitComponent->GetChildActor(), Bounds);
	}

	for (UStaticMeshComponent* Component : ProjectorComponents)
	{
		if (Component && Component->GetStaticMesh())
		{
			Bounds += Component->GetStaticMesh()->GetBoundingBox().TransformBy(Component->GetRelativeTransform());
		}
	}

	for (UStaticMeshComponent* Component : MountComponents)
	{
		if (Component && Component->GetStaticMesh())
		{
			Bounds += Component->GetStaticMesh()->GetBoundingBox().TransformBy(Component->GetRelativeTransform());
		}
	}

	if (!Bounds.IsValid)
	{
		SelectionBounds->SetRelativeLocation(FVector::ZeroVector);
		SelectionBounds->SetBoxExtent(FVector(50.0f, 50.0f, 50.0f));
		return;
	}

	SelectionBounds->SetWorldScale3D(FVector::OneVector);
	SelectionBounds->SetRelativeLocation(Bounds.GetCenter());
	SelectionBounds->SetBoxExtent(Bounds.GetExtent().ComponentMax(FVector(50.0f)));
}

void AProjectionScreenActor::GetScreenDimensionsFt(float& OutWidthFt, float& OutHeightFt) const
{
	switch (ScreenKitSize)
	{
	case EProjectionScreenKitSize::Screen6x12:
		OutWidthFt = 12.0f;
		OutHeightFt = 6.0f;
		break;
	case EProjectionScreenKitSize::Screen8x14:
		OutWidthFt = 14.0f;
		OutHeightFt = 8.0f;
		break;
	case EProjectionScreenKitSize::Screen13x24:
		OutWidthFt = 24.0f;
		OutHeightFt = 13.0f;
		break;
	case EProjectionScreenKitSize::Screen9x16:
	default:
		OutWidthFt = 16.0f;
		OutHeightFt = 9.0f;
		break;
	}
}

void AProjectionScreenActor::GetLensShiftPercent(float& OutVerticalMinPercent, float& OutVerticalMaxPercent, float& OutHorizontalMinPercent, float& OutHorizontalMaxPercent, bool& bOutHasData) const
{
	bOutHasData = true;
	switch (LensType)
	{
	case EProjectionLensType::ILS067HD:
		OutVerticalMinPercent = -32.0f;
		OutVerticalMaxPercent = 48.0f;
		OutHorizontalMinPercent = -8.0f;
		OutHorizontalMaxPercent = 19.0f;
		break;
	case EProjectionLensType::ILS116149HD:
		OutVerticalMinPercent = -128.0f;
		OutVerticalMaxPercent = 133.0f;
		OutHorizontalMinPercent = -73.0f;
		OutHorizontalMaxPercent = 73.0f;
		break;
	case EProjectionLensType::ILS4169HD:
	default:
		OutVerticalMinPercent = 0.0f;
		OutVerticalMaxPercent = 0.0f;
		OutHorizontalMinPercent = 0.0f;
		OutHorizontalMaxPercent = 0.0f;
		bOutHasData = false;
		break;
	}
}

float AProjectionScreenActor::ResolveProjectorHeightFt(float ScreenCenterHeightFt, float ScreenHeightFt)
{
	float MinShiftPercent = 0.0f;
	float MaxShiftPercent = 0.0f;
	float HorizontalMinPercent = 0.0f;
	float HorizontalMaxPercent = 0.0f;
	bool bHasShiftData = false;
	GetLensShiftPercent(MinShiftPercent, MaxShiftPercent, HorizontalMinPercent, HorizontalMaxPercent, bHasShiftData);

	CurrentMinProjectorHeightFt = ScreenCenterHeightFt + (ScreenHeightFt * MinShiftPercent / 100.0f);
	CurrentMaxProjectorHeightFt = ScreenCenterHeightFt + (ScreenHeightFt * MaxShiftPercent / 100.0f);
	if (!bHasShiftData)
	{
		CurrentMinProjectorHeightFt = ScreenCenterHeightFt;
		CurrentMaxProjectorHeightFt = ScreenCenterHeightFt;
	}

	if (CurrentMinProjectorHeightFt > CurrentMaxProjectorHeightFt)
	{
		Swap(CurrentMinProjectorHeightFt, CurrentMaxProjectorHeightFt);
	}

	const float RequestedWorldHeightFt = ScreenCenterHeightFt + TowerHeightFt;
	CurrentRequestedProjectorHeightFt = RequestedWorldHeightFt;
	CurrentActualBuildableProjectorHeightFt = FMath::Clamp(RequestedWorldHeightFt, CurrentMinProjectorHeightFt, CurrentMaxProjectorHeightFt);
	CurrentActualTrussPieces = TEXT("Support placed separately");

	return CurrentActualBuildableProjectorHeightFt;
}

float AProjectionScreenActor::ResolveProjectorHorizontalOffsetFt(float ScreenWidthFt)
{
	CurrentMinProjectorHorizontalOffsetFt = ScreenWidthFt * CurrentLensHorizontalShiftMinPercent / 100.0f;
	CurrentMaxProjectorHorizontalOffsetFt = ScreenWidthFt * CurrentLensHorizontalShiftMaxPercent / 100.0f;
	if (!bCurrentLensShiftDataAvailable)
	{
		CurrentMinProjectorHorizontalOffsetFt = 0.0f;
		CurrentMaxProjectorHorizontalOffsetFt = 0.0f;
	}

	if (CurrentMinProjectorHorizontalOffsetFt > CurrentMaxProjectorHorizontalOffsetFt)
	{
		Swap(CurrentMinProjectorHorizontalOffsetFt, CurrentMaxProjectorHorizontalOffsetFt);
	}

	CurrentActualProjectorHorizontalOffsetFt = FMath::Clamp(ProjectorHorizontalOffsetFt, CurrentMinProjectorHorizontalOffsetFt, CurrentMaxProjectorHorizontalOffsetFt);
	return CurrentActualProjectorHorizontalOffsetFt;
}

void AProjectionScreenActor::ComputeThrowDistances(float ScreenWidthFt, float& OutMinFt, float& OutMaxFt) const
{
	const float WidthInches = ScreenWidthFt * ProjectionInchesPerFoot;
	float MinInches = 0.0f;
	float MaxInches = 0.0f;

	switch (LensType)
	{
	case EProjectionLensType::ILS067HD:
		MinInches = 0.701f * WidthInches + 6.7f;
		MaxInches = MinInches;
		break;
	case EProjectionLensType::ILS4169HD:
		MinInches = 4.081f * WidthInches + 17.2f;
		MaxInches = 6.866f * WidthInches + 17.0f;
		break;
	case EProjectionLensType::ILS116149HD:
	default:
		MinInches = 1.158f * WidthInches + 7.8f;
		MaxInches = 1.495f * WidthInches + 7.9f;
		break;
	}

	OutMinFt = MinInches / ProjectionInchesPerFoot;
	OutMaxFt = MaxInches / ProjectionInchesPerFoot;
}

float AProjectionScreenActor::ResolveThrowDistanceFt() const
{
	switch (ThrowPosition)
	{
	case EProjectionThrowPosition::Minimum:
		return CurrentMinThrowDistanceFt;
	case EProjectionThrowPosition::Maximum:
		return CurrentMaxThrowDistanceFt;
	case EProjectionThrowPosition::Manual:
		return FMath::Clamp(ManualThrowDistanceFt, CurrentMinThrowDistanceFt, CurrentMaxThrowDistanceFt);
	case EProjectionThrowPosition::Middle:
	default:
		return (CurrentMinThrowDistanceFt + CurrentMaxThrowDistanceFt) * 0.5f;
	}
}

UClass* AProjectionScreenActor::LoadScreenKitClass() const
{
	const TCHAR* AssetPath = nullptr;
	switch (ScreenKitSize)
	{
	case EProjectionScreenKitSize::Screen6x12:
		AssetPath = TEXT("/Game/Majic_Gear/Projection/Screen_Kits/6x12_Screen_Kit/BP_6x12ScreenKit3.BP_6x12ScreenKit3_C");
		break;
	case EProjectionScreenKitSize::Screen8x14:
		AssetPath = TEXT("/Game/Majic_Gear/Projection/Screen_Kits/8x14_Screen_Kit/BP_8x14ScreenKit2.BP_8x14ScreenKit2_C");
		break;
	case EProjectionScreenKitSize::Screen13x24:
		AssetPath = TEXT("/Game/Majic_Gear/Projection/Screen_Kits/13x24_Screen_Kit/BP_13x24ScreenKit4.BP_13x24ScreenKit4_C");
		break;
	case EProjectionScreenKitSize::Screen9x16:
	default:
		AssetPath = TEXT("/Game/Majic_Gear/Projection/Screen_Kits/9x16_Screen_Kit/BP_9x16ScreenKit1.BP_9x16ScreenKit1_C");
		break;
	}

	return AssetPath ? FSoftClassPath(AssetPath).TryLoadClass<AActor>() : nullptr;
}

TArray<UStaticMesh*> AProjectionScreenActor::LoadChristieProjectorMeshes() const
{
	const TCHAR* AssetPaths[] = {
		TEXT("/Game/Majic_Gear/Projection/Projectors/Christie25K/StaticMeshes/Bottom_Peg_1.Bottom_Peg_1"),
		TEXT("/Game/Majic_Gear/Projection/Projectors/Christie25K/StaticMeshes/Bottom_Peg_2.Bottom_Peg_2"),
		TEXT("/Game/Majic_Gear/Projection/Projectors/Christie25K/StaticMeshes/Bottom_Peg_3.Bottom_Peg_3"),
		TEXT("/Game/Majic_Gear/Projection/Projectors/Christie25K/StaticMeshes/Bottom_Peg_4.Bottom_Peg_4"),
		TEXT("/Game/Majic_Gear/Projection/Projectors/Christie25K/StaticMeshes/Frame.Frame"),
		TEXT("/Game/Majic_Gear/Projection/Projectors/Christie25K/StaticMeshes/Pin.Pin"),
		TEXT("/Game/Majic_Gear/Projection/Projectors/Christie25K/StaticMeshes/Pin__1_.Pin__1_"),
		TEXT("/Game/Majic_Gear/Projection/Projectors/Christie25K/StaticMeshes/Pin__1___1_.Pin__1___1_"),
		TEXT("/Game/Majic_Gear/Projection/Projectors/Christie25K/StaticMeshes/Pin__2_.Pin__2_"),
		TEXT("/Game/Majic_Gear/Projection/Projectors/Christie25K/StaticMeshes/Projector.Projector")
	};

	TArray<UStaticMesh*> Meshes;
	Meshes.Reserve(UE_ARRAY_COUNT(AssetPaths));
	for (const TCHAR* AssetPath : AssetPaths)
	{
		Meshes.Add(LoadProjectionMesh(AssetPath));
	}
	return Meshes;
}

TArray<UStaticMesh*> AProjectionScreenActor::LoadAVCartMeshes() const
{
	TArray<UStaticMesh*> Meshes;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	TArray<FAssetData> AssetDataList;
	AssetRegistryModule.Get().GetAssetsByPath(
		FName(TEXT("/Game/Majic_Gear/Projection/AV_Cart/StaticMeshes")),
		AssetDataList,
		false);

	AssetDataList.Sort([](const FAssetData& Left, const FAssetData& Right)
	{
		return Left.AssetName.LexicalLess(Right.AssetName);
	});

	for (const FAssetData& AssetData : AssetDataList)
	{
		if (AssetData.AssetClassPath != UStaticMesh::StaticClass()->GetClassPathName())
		{
			continue;
		}

		if (UStaticMesh* Mesh = Cast<UStaticMesh>(AssetData.GetAsset()))
		{
			Meshes.Add(Mesh);
		}
	}
	return Meshes;
}

UStaticMesh* AProjectionScreenActor::LoadMajicGearDefaultMesh(ETrussPieceType PieceType) const
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

	return AssetPath ? LoadProjectionMesh(AssetPath) : nullptr;
}

void AProjectionScreenActor::GetPieceDefinition(ETrussPieceType PieceType, UStaticMesh*& OutMesh, float& OutLengthCm) const
{
	OutMesh = LoadMajicGearDefaultMesh(PieceType);
	OutLengthCm = UTrussMathLibrary::GetDefaultPieceLengthCm(PieceType);
}

FVector AProjectionScreenActor::GetTrussMeshPlacementLocation(UStaticMesh* StaticMesh, const FVector& TargetMinLocation, const FRotator& Rotation) const
{
	if (!StaticMesh)
	{
		return TargetMinLocation;
	}

	const FBoxSphereBounds Bounds = StaticMesh->GetBounds();
	const FVector LocalMin = Bounds.Origin - Bounds.BoxExtent;
	const FVector ScaledRotatedLocalMin = Rotation.RotateVector(LocalMin * TrussMeshScaleMultiplier);
	return TargetMinLocation - ScaledRotatedLocalMin;
}

FVector AProjectionScreenActor::GetScaledRotatedTrussMeshExtent(UStaticMesh* StaticMesh, const FRotator& Rotation) const
{
	if (!StaticMesh)
	{
		return FVector::ZeroVector;
	}

	const FBoxSphereBounds Bounds = StaticMesh->GetBounds();
	const FVector LocalMin = Bounds.Origin - Bounds.BoxExtent;
	const FVector LocalMax = Bounds.Origin + Bounds.BoxExtent;
	FVector Min(FVector::OneVector * TNumericLimits<float>::Max());
	FVector Max(FVector::OneVector * -TNumericLimits<float>::Max());

	for (int32 XIndex = 0; XIndex < 2; ++XIndex)
	{
		for (int32 YIndex = 0; YIndex < 2; ++YIndex)
		{
			for (int32 ZIndex = 0; ZIndex < 2; ++ZIndex)
			{
				const FVector Corner(
					XIndex == 0 ? LocalMin.X : LocalMax.X,
					YIndex == 0 ? LocalMin.Y : LocalMax.Y,
					ZIndex == 0 ? LocalMin.Z : LocalMax.Z);
				const FVector RotatedCorner = Rotation.RotateVector(Corner * TrussMeshScaleMultiplier);
				Min.X = FMath::Min(Min.X, RotatedCorner.X);
				Min.Y = FMath::Min(Min.Y, RotatedCorner.Y);
				Min.Z = FMath::Min(Min.Z, RotatedCorner.Z);
				Max.X = FMath::Max(Max.X, RotatedCorner.X);
				Max.Y = FMath::Max(Max.Y, RotatedCorner.Y);
				Max.Z = FMath::Max(Max.Z, RotatedCorner.Z);
			}
		}
	}

	return Max - Min;
}

void AProjectionScreenActor::ExpandBoundsFromActor(AActor* Actor, FBox& Bounds) const
{
	if (!Actor)
	{
		return;
	}

	const FTransform ChildTransform = ScreenKitComponent ? ScreenKitComponent->GetRelativeTransform() : FTransform::Identity;
	TInlineComponentArray<UStaticMeshComponent*> StaticMeshComponents(Actor);
	for (UStaticMeshComponent* Component : StaticMeshComponents)
	{
		if (Component && Component->GetStaticMesh())
		{
			const FTransform LocalTransform = Component->GetRelativeTransform() * ChildTransform;
			Bounds += Component->GetStaticMesh()->GetBoundingBox().TransformBy(LocalTransform);
		}
	}
}
