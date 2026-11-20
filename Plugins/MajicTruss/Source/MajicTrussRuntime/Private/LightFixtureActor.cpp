#include "LightFixtureActor.h"

#include "Components/BoxComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/LightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DMXAttribute.h"
#include "DMXFixtureActor.h"
#include "DMXFixtureActorBase.h"
#include "DMXTypes.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Game/DMXComponent.h"
#include "Library/DMXEntityFixturePatch.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

ALightFixtureActor::ALightFixtureActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

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
	SelectionBounds->ShapeColor = FColor::Orange;

	FixtureComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("Fixture"));
	FixtureComponent->SetupAttachment(SceneRoot);

	DMX = CreateDefaultSubobject<UDMXComponent>(TEXT("DMX"));
}

void ALightFixtureActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	RebuildFixture();
	if (bAttachedToTruss)
	{
		ApplyStoredMountTransform();
	}
}

void ALightFixtureActor::BeginPlay()
{
	Super::BeginPlay();
	RefreshDMXBinding();
	ApplyLightRenderingSettings();
	ApplyGoboTexture();
}

void ALightFixtureActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ApplyDMXDimmerToSpawnedFixture();
}

void ALightFixtureActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindFixturePatch();
	Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR
bool ALightFixtureActor::ShouldTickIfViewportsOnly() const
{
	return true;
}

void ALightFixtureActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ALightFixtureActor, CustomGoboTexture))
	{
		GoboTexture = CustomGoboTexture;
	}

	const TSubclassOf<AActor> ResolvedFixtureClass = GetResolvedFixtureClass();
	if (MountDefinition.SlingType != SlingType)
	{
		MountDefinition.SlingType = SlingType;
	}
	if (MountDefinition.FixtureClass != ResolvedFixtureClass)
	{
		MountDefinition.FixtureClass = ResolvedFixtureClass;
	}

	RebuildFixture();
	if (bAttachedToTruss)
	{
		ApplyStoredMountTransform();
	}
}

void ALightFixtureActor::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

	if (bApplyingTrussSnap || !bAutoAttachToNearestTrussOnMove)
	{
		return;
	}

	if (!bFinished && !bSnapContinuouslyWhileDragging)
	{
		return;
	}

	Modify();
	AttachToNearestTruss();
}
#endif

void ALightFixtureActor::RebuildFixture()
{
	UpdateFixtureComponent();
	RefreshDMXBinding();
	ApplyLightRenderingSettings();
	ApplyGoboTexture();
	UpdateSelectionBounds();
}

void ALightFixtureActor::SetGoboTexture(UTexture2D* Texture)
{
	GoboTexture = Texture;
	ApplyGoboTexture();
}

bool ALightFixtureActor::IsLekoFixture() const
{
	const UClass* FixtureActorClass = GetResolvedFixtureClass();
	return FixtureActorClass && FixtureActorClass->GetName() == TEXT("BP_Lustr_C");
}

void ALightFixtureActor::SetSelectionHighlighted(bool bHighlighted)
{
	if (SelectionBounds)
	{
		SelectionBounds->SetHiddenInGame(!bHighlighted);
	}
}

void ALightFixtureActor::RefreshDMXBinding()
{
	if (!DMX)
	{
		UnbindFixturePatch();
		return;
	}

	UDMXEntityFixturePatch* FixturePatch = DMX->GetFixturePatch();
	if (!FixturePatch)
	{
		bHasDMXDimmer = false;
		CurrentDMXDimmer = 1.0f;
	}
#if WITH_EDITOR
	if (FixturePatch)
	{
		FixturePatch->Modify();
		FixturePatch->bReceiveDMXInEditor = true;
	}
#endif

	DMX->SetReceiveDMXFromPatch(FixturePatch != nullptr);
	if (UDMXComponent* ChildDMXComponent = GetSpawnedFixtureDMXComponent())
	{
		ChildDMXComponent->SetFixturePatch(FixturePatch);
		ChildDMXComponent->SetReceiveDMXFromPatch(FixturePatch != nullptr);
	}

	BindFixturePatch(FixturePatch);
}

AActor* ALightFixtureActor::GetSpawnedFixtureActor() const
{
	return FixtureComponent ? FixtureComponent->GetChildActor() : nullptr;
}

UDMXComponent* ALightFixtureActor::GetSpawnedFixtureDMXComponent() const
{
	AActor* ChildActor = GetSpawnedFixtureActor();
	if (!ChildActor)
	{
		return nullptr;
	}

	if (const ADMXFixtureActorBase* DMXFixtureActor = Cast<ADMXFixtureActorBase>(ChildActor))
	{
		if (DMXFixtureActor->DMX)
		{
			return DMXFixtureActor->DMX;
		}
	}

	return ChildActor->FindComponentByClass<UDMXComponent>();
}

void ALightFixtureActor::AttachToNearestTruss()
{
	TryAttachToNearestTruss();
}

TSubclassOf<AActor> ALightFixtureActor::GetResolvedFixtureClass() const
{
	UClass* ResolvedClass = FixtureClass.LoadSynchronous();
	if (!ResolvedClass || !ResolvedClass->IsChildOf(AActor::StaticClass()))
	{
		return nullptr;
	}

	return ResolvedClass;
}

void ALightFixtureActor::BindFixturePatch(UDMXEntityFixturePatch* FixturePatch)
{
	if (BoundFixturePatch == FixturePatch)
	{
		if (!FixturePatch)
		{
			UnbindFixturePatch();
		}
		return;
	}

	UnbindFixturePatch();
	BoundFixturePatch = FixturePatch;

	if (!BoundFixturePatch)
	{
		return;
	}

	BoundFixturePatch->OnFixturePatchReceivedDMXNative.AddUObject(this, &ALightFixtureActor::HandleFixturePatchReceivedDMX);

	FDMXNormalizedAttributeValueMap CurrentValues;
	BoundFixturePatch->GetNormalizedAttributeValues(CurrentValues);
	if (CurrentValues.Map.Num() > 0)
	{
		PushDMXValuesToSpawnedFixture(CurrentValues);
	}
}

void ALightFixtureActor::UnbindFixturePatch()
{
	if (BoundFixturePatch)
	{
		BoundFixturePatch->OnFixturePatchReceivedDMXNative.RemoveAll(this);
		BoundFixturePatch = nullptr;
	}
}

void ALightFixtureActor::PushDMXValuesToSpawnedFixture(const FDMXNormalizedAttributeValueMap& ValuePerAttribute)
{
	if (ADMXFixtureActorBase* DMXFixtureActor = Cast<ADMXFixtureActorBase>(GetSpawnedFixtureActor()))
	{
		DMXFixtureActor->PushNormalizedValuesPerAttribute(ValuePerAttribute);
	}

	static const FDMXAttributeName DimmerAttribute(TEXT("Dimmer"));
	if (const float* DimmerValue = ValuePerAttribute.Map.Find(DimmerAttribute))
	{
		CurrentDMXDimmer = FMath::Clamp(*DimmerValue, 0.0f, 1.0f);
		bHasDMXDimmer = true;
		ApplyDMXDimmerToSpawnedFixture();
	}
}

void ALightFixtureActor::ApplyDMXDimmerToSpawnedFixture()
{
	if (!bHasDMXDimmer)
	{
		return;
	}

	ADMXFixtureActor* DMXFixtureActor = Cast<ADMXFixtureActor>(GetSpawnedFixtureActor());
	if (!DMXFixtureActor)
	{
		return;
	}

	if (DMXFixtureActor->SpotLight)
	{
		const float SteradianRatio = UE_PI * 2.0f * (1.0f - DMXFixtureActor->SpotLight->GetCosHalfConeAngle());
		const float FullIntensity = DMXFixtureActor->LightIntensityMax * DMXFixtureActor->SpotlightIntensityScale /
			FMath::Max(SteradianRatio, KINDA_SMALL_NUMBER);
		DMXFixtureActor->SpotLight->SetIntensity(FullIntensity * CurrentDMXDimmer);
	}

	if (DMXFixtureActor->PointLight)
	{
		const float FullIntensity = DMXFixtureActor->LightIntensityMax * DMXFixtureActor->PointlightIntensityScale;
		DMXFixtureActor->PointLight->SetIntensity(FullIntensity * CurrentDMXDimmer);
	}
}

void ALightFixtureActor::ApplyLightRenderingSettings()
{
	AActor* ChildActor = GetSpawnedFixtureActor();
	if (!ChildActor)
	{
		return;
	}

	TInlineComponentArray<ULightComponent*> LightComponents(ChildActor);
	for (ULightComponent* LightComponent : LightComponents)
	{
		if (LightComponent)
		{
			LightComponent->SetCastShadows(bCastShadows);
		}
	}
}

void ALightFixtureActor::HandleFixturePatchReceivedDMX(UDMXEntityFixturePatch* FixturePatch, const FDMXNormalizedAttributeValueMap& ValuePerAttribute)
{
	if (FixturePatch == BoundFixturePatch)
	{
		PushDMXValuesToSpawnedFixture(ValuePerAttribute);
	}
}

bool ALightFixtureActor::TryAttachToNearestTruss()
{
	ATrussStructureActor* TargetTruss = nullptr;
	FTransform MountTransform;
	FTransform LocalMount;
	if (!FindNearestTrussMount(TargetTruss, MountTransform, LocalMount))
	{
		if (bDetachWhenNoTrussInRange)
		{
			DetachFromTruss();
		}
		return false;
	}

	AttachedTruss = TargetTruss;
	MountDefinition.FixtureClass = GetResolvedFixtureClass();
	MountDefinition.SlingType = SlingType;
	bAttachedToTruss = true;
	bFreeMount = true;
	FreeMountLocal = LocalMount;

	bApplyingTrussSnap = true;
	SetActorTransform(MountTransform);
	AttachToActor(TargetTruss, FAttachmentTransformRules::KeepWorldTransform);
	bApplyingTrussSnap = false;
	return true;
}

bool ALightFixtureActor::AttachToTrussAtWorldLocation(ATrussStructureActor* TargetTruss, const FVector& WorldLocation)
{
	if (!TargetTruss)
	{
		return false;
	}

	FTransform LocalMount;
	if (!TargetTruss->FindNearestRailMount(WorldLocation, SlingType, LocalMount))
	{
		return false;
	}

	const FTransform MountTransform = LocalMount * TargetTruss->GetActorTransform();

	AttachedTruss = TargetTruss;
	MountDefinition.FixtureClass = GetResolvedFixtureClass();
	MountDefinition.SlingType = SlingType;
	bAttachedToTruss = true;
	bFreeMount = true;
	FreeMountLocal = LocalMount;

	bApplyingTrussSnap = true;
	SetActorTransform(MountTransform);
	AttachToActor(TargetTruss, FAttachmentTransformRules::KeepWorldTransform);
	bApplyingTrussSnap = false;
	return true;
}

void ALightFixtureActor::DetachFromTruss()
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	AttachedTruss = nullptr;
	bAttachedToTruss = false;
	bFreeMount = false;
}

bool ALightFixtureActor::ApplyStoredMountTransform()
{
	if (!AttachedTruss || !bAttachedToTruss)
	{
		return false;
	}

	MountDefinition.FixtureClass = GetResolvedFixtureClass();
	MountDefinition.SlingType = SlingType;

	FTransform MountTransform;
	if (bFreeMount)
	{
		MountTransform = FreeMountLocal * AttachedTruss->GetActorTransform();
	}
	else if (!AttachedTruss->GetFixtureMountTransformFromDefinition(MountDefinition, MountTransform))
	{
		return false;
	}

	bApplyingTrussSnap = true;
	SetActorTransform(MountTransform);
	AttachToActor(AttachedTruss, FAttachmentTransformRules::KeepWorldTransform);
	bApplyingTrussSnap = false;
	return true;
}

bool ALightFixtureActor::FindNearestTrussMount(ATrussStructureActor*& OutTruss, FTransform& OutMountTransform, FTransform& OutLocalMount) const
{
	OutTruss = nullptr;
	OutMountTransform = FTransform::Identity;
	OutLocalMount = FTransform::Identity;

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const FVector CurrentLocation = GetActorLocation();
	float BestDistance = AttachSearchRadiusCm;

	for (TActorIterator<ATrussStructureActor> It(World); It; ++It)
	{
		ATrussStructureActor* CandidateTruss = *It;
		if (!CandidateTruss)
		{
			continue;
		}

		FTransform CandidateLocal;
		if (!CandidateTruss->FindNearestRailMount(CurrentLocation, SlingType, CandidateLocal))
		{
			continue;
		}

		const FTransform CandidateMountTransform = CandidateLocal * CandidateTruss->GetActorTransform();

		const float CandidateDistance = FVector::Dist(CurrentLocation, CandidateMountTransform.GetLocation());
		if (CandidateDistance <= BestDistance)
		{
			BestDistance = CandidateDistance;
			OutTruss = CandidateTruss;
			OutMountTransform = CandidateMountTransform;
			OutLocalMount = CandidateLocal;
		}
	}

	return OutTruss != nullptr;
}

void ALightFixtureActor::UpdateFixtureComponent()
{
	if (!FixtureComponent)
	{
		return;
	}

	FixtureComponent->SetChildActorClass(GetResolvedFixtureClass());
	FixtureComponent->SetRelativeLocation(FixturePlacementOffsetCm);
	FixtureComponent->SetRelativeRotation(FixturePlacementRotation);
	FixtureComponent->SetRelativeScale3D(FixtureScale);
}

void ALightFixtureActor::ApplyGoboTexture()
{
	AActor* ChildActor = GetSpawnedFixtureActor();
	USpotLightComponent* SpotLight = ChildActor ? ChildActor->FindComponentByClass<USpotLightComponent>() : nullptr;
	if (!SpotLight || !IsLekoFixture())
	{
		GoboMaterialInstance = nullptr;
		return;
	}

	UTexture2D* Texture = GoboTexture.LoadSynchronous();
	if (!Texture)
	{
		SpotLight->SetLightFunctionMaterial(nullptr);
		GoboMaterialInstance = nullptr;
		return;
	}

	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Game/Majic_Gear/Lighting/Lustr/Materials/M_LekoGoboLightFunction.M_LekoGoboLightFunction"));
	if (!Material)
	{
		return;
	}

	if (!GoboMaterialInstance)
	{
		GoboMaterialInstance = UMaterialInstanceDynamic::Create(Material, this);
	}
	GoboMaterialInstance->SetTextureParameterValue(TEXT("GoboTexture"), Texture);
	SpotLight->SetLightFunctionMaterial(GoboMaterialInstance);
}

void ALightFixtureActor::UpdateSelectionBounds()
{
	if (!SelectionBounds)
	{
		return;
	}

	FVector Origin = FixtureComponent ? FixtureComponent->GetRelativeLocation() : FVector::ZeroVector;
	FVector Extent = FVector(35.0f, 35.0f, 35.0f);

	if (AActor* ChildActor = FixtureComponent ? FixtureComponent->GetChildActor() : nullptr)
	{
		FBox PhysicalBounds(EForceInit::ForceInit);
		TInlineComponentArray<UStaticMeshComponent*> MeshComponents(ChildActor);
		for (UStaticMeshComponent* Component : MeshComponents)
		{
			if (!Component || !Component->GetStaticMesh())
			{
				continue;
			}

			const FTransform ComponentToFixture = Component->GetComponentTransform().GetRelativeTransform(GetActorTransform());
			const FBox PartBounds = Component->GetStaticMesh()->GetBoundingBox().TransformBy(ComponentToFixture);
			if (PartBounds.GetSize().GetMax() <= 600.0f)
			{
				PhysicalBounds += PartBounds;
			}
		}

		if (PhysicalBounds.IsValid)
		{
			Origin = PhysicalBounds.GetCenter();
			Extent = PhysicalBounds.GetExtent().ComponentMax(FVector(15.0f, 15.0f, 15.0f));
		}
	}

	SelectionBounds->SetRelativeLocation(Origin);
	SelectionBounds->SetBoxExtent(Extent + FVector(5.0f, 5.0f, 5.0f));
}
