#include "TrussStructureActor.h"

#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "TrussMathLibrary.h"

ATrussStructureActor::ATrussStructureActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void ATrussStructureActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (bBuildOnConstruction)
	{
		BuildStraightRun();
	}
}

void ATrussStructureActor::BuildStraightRun()
{
	ClearGeneratedTruss();

	const float TargetLengthCm = UTrussMathLibrary::FeetToCentimeters(LengthFt);
	const FTrussCombinationResult Combination = UTrussMathLibrary::FindBestTrussCombination(TargetLengthCm);

	float CursorX = 0.0f;
	for (ETrussPieceType PieceType : Combination.Pieces)
	{
		FTrussPieceDefinition PieceDefinition;
		const bool bHasInventoryPiece = Inventory && Inventory->FindPiece(PieceType, PieceDefinition);
		const float PieceLengthCm = bHasInventoryPiece ? PieceDefinition.LengthCm : UTrussMathLibrary::GetDefaultPieceLengthCm(PieceType);

		if (bHasInventoryPiece && PieceDefinition.StaticMesh)
		{
			UInstancedStaticMeshComponent* MeshComponent = FindOrCreateMeshComponent(PieceType, PieceDefinition.StaticMesh);
			if (MeshComponent)
			{
				MeshComponent->AddInstance(FTransform(FRotator::ZeroRotator, FVector(CursorX, 0.0f, 0.0f)));
			}
		}
		else
		{
			AddDebugPiece(PieceType, PieceLengthCm, CursorX);
		}

		CursorX += PieceLengthCm;
	}

	LastBuiltLengthFt = UTrussMathLibrary::CentimetersToFeet(CursorX);
}

void ATrussStructureActor::ClearGeneratedTruss()
{
	for (UActorComponent* Component : GeneratedComponents)
	{
		if (Component)
		{
			Component->DestroyComponent();
		}
	}

	GeneratedComponents.Reset();
}

UInstancedStaticMeshComponent* ATrussStructureActor::FindOrCreateMeshComponent(ETrussPieceType PieceType, UStaticMesh* StaticMesh)
{
	const FString ComponentName = FString::Printf(TEXT("ISM_%s"), *UTrussMathLibrary::PieceTypeToLabel(PieceType).Replace(TEXT(" "), TEXT("_")));

	for (UActorComponent* Component : GeneratedComponents)
	{
		UInstancedStaticMeshComponent* Existing = Cast<UInstancedStaticMeshComponent>(Component);
		if (Existing && Existing->GetFName() == FName(*ComponentName))
		{
			return Existing;
		}
	}

	UInstancedStaticMeshComponent* MeshComponent = NewObject<UInstancedStaticMeshComponent>(this, FName(*ComponentName), RF_Transactional);
	if (!MeshComponent)
	{
		return nullptr;
	}

	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetStaticMesh(StaticMesh);
	MeshComponent->RegisterComponent();
	AddInstanceComponent(MeshComponent);
	GeneratedComponents.Add(MeshComponent);
	return MeshComponent;
}

void ATrussStructureActor::AddDebugPiece(ETrussPieceType PieceType, float PieceLengthCm, float StartX)
{
	const FString ComponentName = FString::Printf(TEXT("Debug_%s_%d"), *UTrussMathLibrary::PieceTypeToLabel(PieceType).Replace(TEXT(" "), TEXT("_")), GeneratedComponents.Num());

	UBoxComponent* BoxComponent = NewObject<UBoxComponent>(this, FName(*ComponentName), RF_Transactional);
	if (!BoxComponent)
	{
		return;
	}

	BoxComponent->SetupAttachment(SceneRoot);
	BoxComponent->SetBoxExtent(FVector(PieceLengthCm * 0.5f, DebugCrossSectionCm * 0.5f, DebugCrossSectionCm * 0.5f));
	BoxComponent->SetRelativeLocation(FVector(StartX + (PieceLengthCm * 0.5f), 0.0f, 0.0f));
	BoxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BoxComponent->RegisterComponent();
	AddInstanceComponent(BoxComponent);
	GeneratedComponents.Add(BoxComponent);
}
