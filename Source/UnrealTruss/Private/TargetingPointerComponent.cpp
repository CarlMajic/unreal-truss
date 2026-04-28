#include "TargetingPointerComponent.h"

#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TrussStructureActor.h"

UTargetingPointerComponent::UTargetingPointerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	BeamThicknessCm = 5.0f;
	MarkerDiameterCm = 12.0f;
}

void UTargetingPointerComponent::BeginPlay()
{
	Super::BeginPlay();
	EnsureVisualComponents();
	HidePointer();
}

void UTargetingPointerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (BeamMeshComponent)
	{
		BeamMeshComponent->DestroyComponent();
		BeamMeshComponent = nullptr;
	}

	if (MarkerMeshComponent)
	{
		MarkerMeshComponent->DestroyComponent();
		MarkerMeshComponent = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void UTargetingPointerComponent::UpdatePointer(APlayerController* PlayerController, ETargetingPointerMode Mode, AActor* PrimaryIgnoredActor, AActor* SecondaryIgnoredActor)
{
	EnsureVisualComponents();

	bHasValidHit = false;
	CurrentHitResult = FHitResult();
	CurrentTrussActor = nullptr;

	if (Mode == ETargetingPointerMode::Disabled || !PlayerController)
	{
		HidePointer();
		return;
	}

	FVector ViewLocation = FVector::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
	const FVector VisualStart = ViewLocation + ViewRotation.RotateVector(VisualOriginOffset);
	const FVector TraceEnd = ViewLocation + (ViewRotation.Vector() * TraceDistanceCm);

	FHitResult HitResult;
	ATrussStructureActor* TrussActor = nullptr;
	const bool bHit = Mode == ETargetingPointerMode::TrussSelection
		? TraceTrussSelection(PlayerController, PrimaryIgnoredActor, SecondaryIgnoredActor, HitResult, TrussActor)
		: TraceWorldPlacement(PlayerController, PrimaryIgnoredActor, SecondaryIgnoredActor, HitResult);

	if (bHit)
	{
		bHasValidHit = true;
		CurrentHitResult = HitResult;
		CurrentTrussActor = TrussActor;
		UpdateVisuals(VisualStart, HitResult.ImpactPoint, true);
		DrawPointerDebug(VisualStart, HitResult.ImpactPoint, true);
		return;
	}

	UpdateVisuals(VisualStart, TraceEnd, false);
	DrawPointerDebug(VisualStart, TraceEnd, false);
}

void UTargetingPointerComponent::HidePointer()
{
	if (BeamMeshComponent)
	{
		BeamMeshComponent->SetHiddenInGame(true);
	}

	if (MarkerMeshComponent)
	{
		MarkerMeshComponent->SetHiddenInGame(true);
	}
}

void UTargetingPointerComponent::EnsureVisualComponents()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	USceneComponent* RootComponent = Owner->GetRootComponent();
	if (!RootComponent)
	{
		return;
	}

	if (!BeamMeshComponent)
	{
		BeamMeshComponent = NewObject<UStaticMeshComponent>(Owner, TEXT("TargetingPointerBeam"));
		BeamMeshComponent->SetupAttachment(RootComponent);
		BeamMeshComponent->RegisterComponent();
		Owner->AddInstanceComponent(BeamMeshComponent);
		ConfigureMeshAppearance(BeamMeshComponent, FLinearColor(0.2f, 0.8f, 1.0f));
	}

	if (!MarkerMeshComponent)
	{
		MarkerMeshComponent = NewObject<UStaticMeshComponent>(Owner, TEXT("TargetingPointerMarker"));
		MarkerMeshComponent->SetupAttachment(RootComponent);
		MarkerMeshComponent->RegisterComponent();
		Owner->AddInstanceComponent(MarkerMeshComponent);
		ConfigureMeshAppearance(MarkerMeshComponent, FLinearColor(1.0f, 0.9f, 0.2f));
	}
}

void UTargetingPointerComponent::UpdateVisuals(const FVector& Start, const FVector& End, bool bShowMarker)
{
	if (!BeamMeshComponent || !MarkerMeshComponent)
	{
		return;
	}

	const FVector Direction = End - Start;
	const float Distance = Direction.Size();
	if (Distance <= KINDA_SMALL_NUMBER)
	{
		HidePointer();
		return;
	}

	BeamMeshComponent->SetHiddenInGame(false);
	BeamMeshComponent->SetWorldLocation((Start + End) * 0.5f);
	BeamMeshComponent->SetWorldRotation(FRotationMatrix::MakeFromX(Direction.GetSafeNormal()).Rotator());

	if (UStaticMesh* BeamMesh = BeamMeshComponent->GetStaticMesh())
	{
		const FVector BeamSize = BeamMesh->GetBounds().BoxExtent * 2.0f;
		const FVector BeamScale(
			BeamSize.X > KINDA_SMALL_NUMBER ? Distance / BeamSize.X : 1.0f,
			BeamSize.Y > KINDA_SMALL_NUMBER ? BeamThicknessCm / BeamSize.Y : 0.05f,
			BeamSize.Z > KINDA_SMALL_NUMBER ? BeamThicknessCm / BeamSize.Z : 0.05f);
		BeamMeshComponent->SetWorldScale3D(BeamScale);
	}

	MarkerMeshComponent->SetHiddenInGame(!bShowMarker);
	if (!bShowMarker)
	{
		return;
	}

	MarkerMeshComponent->SetWorldLocation(End);
	if (UStaticMesh* MarkerMesh = MarkerMeshComponent->GetStaticMesh())
	{
		const FVector MarkerSize = MarkerMesh->GetBounds().BoxExtent * 2.0f;
		const float UniformScale = MarkerSize.GetMax() > KINDA_SMALL_NUMBER ? MarkerDiameterCm / MarkerSize.GetMax() : 0.1f;
		MarkerMeshComponent->SetWorldScale3D(FVector(UniformScale));
	}
}

void UTargetingPointerComponent::DrawPointerDebug(const FVector& Start, const FVector& End, bool bHitTarget) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FColor BeamColor = bHitTarget ? FColor(40, 210, 255) : FColor(120, 120, 120);
	DrawDebugLine(World, Start, End, BeamColor, false, 0.0f, 0, 3.0f);

	if (bHitTarget)
	{
		DrawDebugSphere(World, End, MarkerDiameterCm * 0.5f, 16, FColor::Yellow, false, 0.0f, 0, 2.0f);
	}
}

bool UTargetingPointerComponent::TraceWorldPlacement(APlayerController* PlayerController, AActor* PrimaryIgnoredActor, AActor* SecondaryIgnoredActor, FHitResult& OutHitResult)
{
	UWorld* World = GetWorld();
	if (!World || !PlayerController)
	{
		return false;
	}

	FVector ViewLocation = FVector::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
	const FVector TraceEnd = ViewLocation + (ViewRotation.Vector() * TraceDistanceCm);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TargetingPointerWorldTrace), false, GetOwner());
	if (PrimaryIgnoredActor)
	{
		QueryParams.AddIgnoredActor(PrimaryIgnoredActor);
	}
	if (SecondaryIgnoredActor)
	{
		QueryParams.AddIgnoredActor(SecondaryIgnoredActor);
	}

	return World->LineTraceSingleByChannel(OutHitResult, ViewLocation, TraceEnd, ECC_Visibility, QueryParams);
}

bool UTargetingPointerComponent::TraceTrussSelection(APlayerController* PlayerController, AActor* PrimaryIgnoredActor, AActor* SecondaryIgnoredActor, FHitResult& OutHitResult, ATrussStructureActor*& OutTrussActor)
{
	UWorld* World = GetWorld();
	if (!World || !PlayerController)
	{
		return false;
	}

	FVector ViewLocation = FVector::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
	const FVector TraceEnd = ViewLocation + (ViewRotation.Vector() * TraceDistanceCm);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TargetingPointerTrussTrace), true, GetOwner());
	if (PrimaryIgnoredActor)
	{
		QueryParams.AddIgnoredActor(PrimaryIgnoredActor);
	}
	if (SecondaryIgnoredActor)
	{
		QueryParams.AddIgnoredActor(SecondaryIgnoredActor);
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	TArray<FHitResult> HitResults;
	if (!World->LineTraceMultiByObjectType(HitResults, ViewLocation, TraceEnd, ObjectQueryParams, QueryParams))
	{
		return false;
	}

	for (const FHitResult& HitResult : HitResults)
	{
		if (ATrussStructureActor* TrussActor = Cast<ATrussStructureActor>(HitResult.GetActor()))
		{
			OutHitResult = HitResult;
			OutTrussActor = TrussActor;
			return true;
		}

		if (const UActorComponent* HitComponent = HitResult.GetComponent())
		{
			if (ATrussStructureActor* OwnerTrussActor = Cast<ATrussStructureActor>(HitComponent->GetOwner()))
			{
				OutHitResult = HitResult;
				OutTrussActor = OwnerTrussActor;
				return true;
			}
		}
	}

	return false;
}

void UTargetingPointerComponent::ConfigureMeshAppearance(UStaticMeshComponent* MeshComponent, const FLinearColor& Color) const
{
	if (!MeshComponent)
	{
		return;
	}

	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetGenerateOverlapEvents(false);
	MeshComponent->SetCastShadow(false);
	MeshComponent->SetReceivesDecals(false);
	MeshComponent->SetVisibleInSceneCaptureOnly(false);
	MeshComponent->SetHiddenInGame(true);
	MeshComponent->SetTranslucentSortPriority(10);

	UStaticMesh* ShapeMesh = nullptr;
	if (MeshComponent == MarkerMeshComponent)
	{
		ShapeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	}
	else
	{
		ShapeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	}

	if (ShapeMesh)
	{
		MeshComponent->SetStaticMesh(ShapeMesh);
	}

	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (Material)
	{
		MeshComponent->SetMaterial(0, Material);
		if (UMaterialInstanceDynamic* DynamicMaterial = MeshComponent->CreateAndSetMaterialInstanceDynamic(0))
		{
			DynamicMaterial->SetVectorParameterValue(TEXT("Color"), Color);
		}
	}
}
