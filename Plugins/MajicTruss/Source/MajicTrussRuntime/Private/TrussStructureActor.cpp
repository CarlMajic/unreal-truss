#include "TrussStructureActor.h"

#include "Components/ActorComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "UObject/Field.h"
#include "UObject/UnrealType.h"
#include "TrussMathLibrary.h"

namespace
{
void InvertClampOffsetProperties(UObject* Object)
{
	if (!Object)
	{
		return;
	}

	for (TFieldIterator<FFloatProperty> PropertyIt(Object->GetClass()); PropertyIt; ++PropertyIt)
	{
		FFloatProperty* FloatProperty = *PropertyIt;
		if (!FloatProperty)
		{
			continue;
		}

		const FString PropertyName = FloatProperty->GetName();
		if (!PropertyName.Contains(TEXT("Clamp")) || !PropertyName.Contains(TEXT("Offset")))
		{
			continue;
		}

		float* ValuePtr = FloatProperty->ContainerPtrToValuePtr<float>(Object);
		if (ValuePtr)
		{
			*ValuePtr *= -1.0f;
		}
	}
}
}

ATrussStructureActor::ATrussStructureActor()
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
	SelectionBounds->ShapeColor = FColor::Yellow;

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

	CornerBlockInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("CornerBlockInstances"));
	CornerBlockInstances->SetupAttachment(SceneRoot);

	BaseInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("BaseInstances"));
	BaseInstances->SetupAttachment(SceneRoot);

	HingeInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("HingeInstances"));
	HingeInstances->SetupAttachment(SceneRoot);
}

void ATrussStructureActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (bBuildOnConstruction)
	{
		BuildCurrentMode();
	}

	RebuildMountedFixtures();
}

void ATrussStructureActor::BuildCurrentMode()
{
	switch (BuildMode)
	{
	case ETrussBuildMode::CubeArch:
		BuildCubeArch();
		break;
	case ETrussBuildMode::Cube:
		BuildCube();
		break;
	case ETrussBuildMode::Arch:
		BuildArch();
		break;
	case ETrussBuildMode::Rectangle:
		BuildRectangle();
		break;
	case ETrussBuildMode::StraightRun:
	default:
		BuildStraightRun();
		break;
	}
}

void ATrussStructureActor::ApplyBuildDefinition(const FTrussBuildDefinition& Definition, bool bRebuild)
{
	BuildMode = Definition.BuildMode;
	LengthFt = Definition.LengthFt;
	StraightRunHeightFt = Definition.StraightRunHeightFt;
	RectangleLengthFt = Definition.RectangleLengthFt;
	RectangleWidthFt = Definition.RectangleWidthFt;
	RectangleHeightFt = Definition.RectangleHeightFt;
	ArchHeightFt = Definition.ArchHeightFt;
	ArchWidthFt = Definition.ArchWidthFt;
	CubeLengthFt = Definition.CubeLengthFt;
	CubeWidthFt = Definition.CubeWidthFt;
	CubeHeightFt = Definition.CubeHeightFt;
	CubeArchWidthFt = Definition.CubeArchWidthFt;
	CubeArchHeightFt = Definition.CubeArchHeightFt;
	CubeArchSideSpacingPiece = Definition.CubeArchSideSpacingPiece;
	CubeArchDepthSpacingPiece = Definition.CubeArchDepthSpacingPiece;

	if (bRebuild)
	{
		BuildCurrentMode();
	}
}

FTrussBuildDefinition ATrussStructureActor::GetBuildDefinition() const
{
	FTrussBuildDefinition Definition;
	Definition.BuildMode = BuildMode;
	Definition.LengthFt = LengthFt;
	Definition.StraightRunHeightFt = StraightRunHeightFt;
	Definition.RectangleLengthFt = RectangleLengthFt;
	Definition.RectangleWidthFt = RectangleWidthFt;
	Definition.RectangleHeightFt = RectangleHeightFt;
	Definition.ArchHeightFt = ArchHeightFt;
	Definition.ArchWidthFt = ArchWidthFt;
	Definition.CubeLengthFt = CubeLengthFt;
	Definition.CubeWidthFt = CubeWidthFt;
	Definition.CubeHeightFt = CubeHeightFt;
	Definition.CubeArchWidthFt = CubeArchWidthFt;
	Definition.CubeArchHeightFt = CubeArchHeightFt;
	Definition.CubeArchSideSpacingPiece = CubeArchSideSpacingPiece;
	Definition.CubeArchDepthSpacingPiece = CubeArchDepthSpacingPiece;
	return Definition;
}

void ATrussStructureActor::BuildStraightRun()
{
	ClearGeneratedTruss();

	const float TargetLengthCm = UTrussMathLibrary::FeetToCentimeters(LengthFt);
	const FTrussCombinationResult Combination = UTrussMathLibrary::FindBestTrussCombination(TargetLengthCm);
	const float HeightCm = UTrussMathLibrary::FeetToCentimeters(StraightRunHeightFt);

	AddStraightRun(Combination.Pieces, FVector(0.0f, 0.0f, HeightCm), FRotator::ZeroRotator);
	LastBuiltLengthFt = UTrussMathLibrary::CentimetersToFeet(Combination.ActualLengthCm);
	UpdateSelectionBounds();
}

void ATrussStructureActor::BuildRectangle()
{
	ClearGeneratedTruss();

	FTrussPieceDefinition CornerDefinition;
	UStaticMesh* CornerMesh = nullptr;
	if (!GetPieceDefinition(ETrussPieceType::CornerBlock, CornerDefinition, CornerMesh))
	{
		return;
	}

	const FVector CornerExtent = GetScaledRotatedMeshExtent(CornerMesh, FRotator::ZeroRotator);
	const float CornerX = CornerExtent.X;
	const float CornerY = CornerExtent.Y;
	const float HeightCm = UTrussMathLibrary::FeetToCentimeters(RectangleHeightFt);
	const float TargetLengthCm = UTrussMathLibrary::FeetToCentimeters(RectangleLengthFt);
	const float TargetWidthCm = UTrussMathLibrary::FeetToCentimeters(RectangleWidthFt);
	const float InnerLengthCm = TargetLengthCm - (2.0f * CornerX);
	const float InnerWidthCm = TargetWidthCm - (2.0f * CornerY);

	if (InnerLengthCm <= 0.0f || InnerWidthCm <= 0.0f)
	{
		return;
	}

	const FTrussCombinationResult LengthCombination = UTrussMathLibrary::FindBestTrussCombination(InnerLengthCm);
	const FTrussCombinationResult WidthCombination = UTrussMathLibrary::FindBestTrussCombination(InnerWidthCm);

	const float RightX = CornerX + LengthCombination.ActualLengthCm;
	const float BackY = CornerY + WidthCombination.ActualLengthCm;

	AddPieceInstance(ETrussPieceType::CornerBlock, FVector(0.0f, 0.0f, HeightCm), FRotator::ZeroRotator);
	AddPieceInstance(ETrussPieceType::CornerBlock, FVector(RightX, 0.0f, HeightCm), FRotator::ZeroRotator);
	AddPieceInstance(ETrussPieceType::CornerBlock, FVector(RightX, BackY, HeightCm), FRotator::ZeroRotator);
	AddPieceInstance(ETrussPieceType::CornerBlock, FVector(0.0f, BackY, HeightCm), FRotator::ZeroRotator);

	AddStraightRun(LengthCombination.Pieces, FVector(CornerX, 0.0f, HeightCm), FRotator::ZeroRotator);
	AddStraightRun(LengthCombination.Pieces, FVector(CornerX, BackY, HeightCm), FRotator::ZeroRotator);
	AddStraightRun(WidthCombination.Pieces, FVector(RectangleYRunXOffsetCm, CornerY, HeightCm), FRotator(0.0f, 90.0f, 0.0f));
	AddStraightRun(WidthCombination.Pieces, FVector(RightX + RectangleYRunXOffsetCm, CornerY, HeightCm), FRotator(0.0f, 90.0f, 0.0f));

	LastBuiltLengthFt = UTrussMathLibrary::CentimetersToFeet((2.0f * (CornerX + CornerY)) + (2.0f * LengthCombination.ActualLengthCm) + (2.0f * WidthCombination.ActualLengthCm));
	UpdateSelectionBounds();
}

void ATrussStructureActor::BuildArch()
{
	ClearGeneratedTruss();

	FTrussPieceDefinition CornerDefinition;
	UStaticMesh* CornerMesh = nullptr;
	if (!GetPieceDefinition(ETrussPieceType::CornerBlock, CornerDefinition, CornerMesh))
	{
		return;
	}

	FTrussPieceDefinition BaseDefinition;
	UStaticMesh* BaseMesh = nullptr;
	if (!GetPieceDefinition(ETrussPieceType::Base, BaseDefinition, BaseMesh))
	{
		return;
	}

	const FVector CornerExtent = GetScaledRotatedMeshExtent(CornerMesh, FRotator::ZeroRotator);
	const FVector BaseExtent = GetScaledRotatedMeshExtent(BaseMesh, FRotator::ZeroRotator);
	const float CornerWidthCm = CornerExtent.X;
	const float CornerHeightCm = CornerExtent.Z;
	const float BaseHeightCm = BaseExtent.Z;
	const float TargetHeightCm = UTrussMathLibrary::FeetToCentimeters(ArchHeightFt);
	const float TargetWidthCm = UTrussMathLibrary::FeetToCentimeters(ArchWidthFt);
	const float LegTargetCm = TargetHeightCm - CornerHeightCm;
	const float SpanTargetCm = TargetWidthCm - (2.0f * CornerWidthCm);

	if (LegTargetCm <= 0.0f || SpanTargetCm <= 0.0f)
	{
		return;
	}

	const FTrussCombinationResult LegCombination = UTrussMathLibrary::FindBestTrussCombination(LegTargetCm);
	const FTrussCombinationResult SpanCombination = UTrussMathLibrary::FindBestTrussCombination(SpanTargetCm);
	if (LegCombination.Pieces.IsEmpty() || SpanCombination.Pieces.IsEmpty())
	{
		return;
	}

	const float LeftLegX = 0.0f;
	const float RightLegX = CornerWidthCm + SpanCombination.ActualLengthCm;
	const float TopCornerZ = BaseHeightCm + LegCombination.ActualLengthCm;
	const float LegCenterInsetX = ArchCornerConnectionOffsetCm + (CornerWidthCm * 0.5f);
	const FRotator VerticalRotation(ArchVerticalRotationYDeg, ArchVerticalRotationZDeg, ArchVerticalRotationXDeg);

	for (float LegX : {LeftLegX, RightLegX})
	{
		const float LegCenterX = LegX + LegCenterInsetX;
		const float BaseMinX = LegCenterX - (BaseExtent.X * 0.5f);
		const float BaseMinY = ArchBaseYOffsetCm - (BaseExtent.Y * 0.5f);
		AddPieceInstance(ETrussPieceType::Base, FVector(BaseMinX, BaseMinY, 0.0f), FRotator::ZeroRotator);

		AddStraightRun(
			LegCombination.Pieces,
			FVector(LegCenterX - ArchCornerConnectionOffsetCm + ArchVerticalLegXOffsetCm, ArchLegYOffsetCm, BaseHeightCm),
			VerticalRotation
		);

		AddPieceInstance(
			ETrussPieceType::CornerBlock,
			FVector(LegX + ArchCornerConnectionOffsetCm, ArchSpanYOffsetCm, TopCornerZ),
			FRotator::ZeroRotator
		);
	}

	const float SpanZ = TopCornerZ + (CornerHeightCm * 0.5f) - ArchCornerConnectionOffsetCm;
	AddStraightRun(
		SpanCombination.Pieces,
		FVector(CornerWidthCm + ArchCornerConnectionOffsetCm, ArchSpanYOffsetCm, SpanZ),
		FRotator::ZeroRotator
	);

	LastBuiltLengthFt = UTrussMathLibrary::CentimetersToFeet(BaseHeightCm + LegCombination.ActualLengthCm + CornerHeightCm);
	UpdateSelectionBounds();
}

void ATrussStructureActor::BuildCube()
{
	ClearGeneratedTruss();

	FTrussPieceDefinition CornerDefinition;
	UStaticMesh* CornerMesh = nullptr;
	if (!GetPieceDefinition(ETrussPieceType::CornerBlock, CornerDefinition, CornerMesh))
	{
		return;
	}

	FTrussPieceDefinition BaseDefinition;
	UStaticMesh* BaseMesh = nullptr;
	if (!GetPieceDefinition(ETrussPieceType::Base, BaseDefinition, BaseMesh))
	{
		return;
	}

	const FVector CornerExtent = GetScaledRotatedMeshExtent(CornerMesh, FRotator::ZeroRotator);
	const FVector BaseExtent = GetScaledRotatedMeshExtent(BaseMesh, FRotator::ZeroRotator);
	const float CornerX = CornerExtent.X;
	const float CornerY = CornerExtent.Y;
	const float CornerHeightCm = CornerExtent.Z;
	const float BaseHeightCm = BaseExtent.Z;

	const float InnerLengthCm = UTrussMathLibrary::FeetToCentimeters(CubeLengthFt) - (2.0f * CornerX);
	const float InnerWidthCm = UTrussMathLibrary::FeetToCentimeters(CubeWidthFt) - (2.0f * CornerY);
	const float LegTargetCm = UTrussMathLibrary::FeetToCentimeters(CubeHeightFt) - CornerHeightCm;
	if (InnerLengthCm <= 0.0f || InnerWidthCm <= 0.0f || LegTargetCm <= 0.0f)
	{
		return;
	}

	const FTrussCombinationResult LengthCombination = UTrussMathLibrary::FindBestTrussCombination(InnerLengthCm);
	const FTrussCombinationResult WidthCombination = UTrussMathLibrary::FindBestTrussCombination(InnerWidthCm);
	const FTrussCombinationResult LegCombination = UTrussMathLibrary::FindBestTrussCombination(LegTargetCm);
	if (LengthCombination.Pieces.IsEmpty() || WidthCombination.Pieces.IsEmpty() || LegCombination.Pieces.IsEmpty())
	{
		return;
	}

	const float LeftX = 0.0f;
	const float RightX = CornerX + LengthCombination.ActualLengthCm;
	const float FrontY = 0.0f;
	const float BackY = CornerY + WidthCombination.ActualLengthCm;
	const float TopZ = BaseHeightCm + LegCombination.ActualLengthCm;
	const float SpanZ = TopZ + (CornerHeightCm * 0.5f) - CubeCornerConnectionOffsetCm;
	const FRotator VerticalRotation(ArchVerticalRotationYDeg, ArchVerticalRotationZDeg, ArchVerticalRotationXDeg);

	const TArray<FVector2D> CornerPositions = {
		FVector2D(LeftX, FrontY),
		FVector2D(RightX, FrontY),
		FVector2D(RightX, BackY),
		FVector2D(LeftX, BackY)
	};

	for (const FVector2D& CornerPosition : CornerPositions)
	{
		const float LegCenterX = CornerPosition.X + CubeCornerConnectionOffsetCm + (CornerX * 0.5f);
		const float BaseMinX = LegCenterX - (BaseExtent.X * 0.5f);
		const float BaseMinY = CornerPosition.Y + ArchBaseYOffsetCm - (BaseExtent.Y * 0.5f);
		AddPieceInstance(ETrussPieceType::Base, FVector(BaseMinX, BaseMinY, 0.0f), FRotator::ZeroRotator);

		AddStraightRun(
			LegCombination.Pieces,
			FVector(LegCenterX - CubeCornerConnectionOffsetCm + ArchVerticalLegXOffsetCm, CornerPosition.Y + ArchLegYOffsetCm, BaseHeightCm),
			VerticalRotation
		);
	}

	AddPieceInstance(ETrussPieceType::CornerBlock, FVector(LeftX + CubeCornerConnectionOffsetCm, FrontY + ArchSpanYOffsetCm, TopZ), FRotator::ZeroRotator);
	AddPieceInstance(ETrussPieceType::CornerBlock, FVector(RightX + CubeCornerConnectionOffsetCm, FrontY + ArchSpanYOffsetCm, TopZ), FRotator::ZeroRotator);
	AddPieceInstance(ETrussPieceType::CornerBlock, FVector(RightX + CubeCornerConnectionOffsetCm, BackY + ArchSpanYOffsetCm, TopZ), FRotator::ZeroRotator);
	AddPieceInstance(ETrussPieceType::CornerBlock, FVector(LeftX + CubeCornerConnectionOffsetCm, BackY + ArchSpanYOffsetCm, TopZ), FRotator::ZeroRotator);

	AddStraightRun(LengthCombination.Pieces, FVector(CornerX + CubeCornerConnectionOffsetCm, FrontY + ArchSpanYOffsetCm, SpanZ), FRotator::ZeroRotator);
	AddStraightRun(LengthCombination.Pieces, FVector(CornerX + CubeCornerConnectionOffsetCm, BackY + ArchSpanYOffsetCm, SpanZ), FRotator::ZeroRotator);
	AddStraightRun(WidthCombination.Pieces, FVector(LeftX + CubeYRunXOffsetCm, CornerY + ArchSpanYOffsetCm, SpanZ), FRotator(0.0f, 90.0f, 0.0f));
	AddStraightRun(WidthCombination.Pieces, FVector(RightX + CubeYRunXOffsetCm, CornerY + ArchSpanYOffsetCm, SpanZ), FRotator(0.0f, 90.0f, 0.0f));

	LastBuiltLengthFt = UTrussMathLibrary::CentimetersToFeet(BaseHeightCm + LegCombination.ActualLengthCm + CornerHeightCm);
	UpdateSelectionBounds();
}

void ATrussStructureActor::BuildCubeArch()
{
	ClearGeneratedTruss();

	FTrussPieceDefinition CornerDefinition;
	UStaticMesh* CornerMesh = nullptr;
	if (!GetPieceDefinition(ETrussPieceType::CornerBlock, CornerDefinition, CornerMesh))
	{
		return;
	}

	FTrussPieceDefinition BaseDefinition;
	UStaticMesh* BaseMesh = nullptr;
	if (!GetPieceDefinition(ETrussPieceType::Base, BaseDefinition, BaseMesh))
	{
		return;
	}

	const auto GetValidSpacingPiece = [](ETrussPieceType PieceType)
	{
		return UTrussMathLibrary::GetDefaultPieceLengthCm(PieceType) > 0.0f ? PieceType : ETrussPieceType::FourFoot;
	};

	const ETrussPieceType SidePiece = GetValidSpacingPiece(CubeArchSideSpacingPiece);
	const ETrussPieceType DepthPiece = GetValidSpacingPiece(CubeArchDepthSpacingPiece);
	const float SideSpacingCm = UTrussMathLibrary::GetDefaultPieceLengthCm(SidePiece);
	const float DepthSpacingCm = UTrussMathLibrary::GetDefaultPieceLengthCm(DepthPiece);

	const FVector CornerExtent = GetScaledRotatedMeshExtent(CornerMesh, FRotator::ZeroRotator);
	const FVector BaseExtent = GetScaledRotatedMeshExtent(BaseMesh, FRotator::ZeroRotator);
	const float CornerX = CornerExtent.X;
	const float CornerY = CornerExtent.Y;
	const float CornerHeightCm = CornerExtent.Z;
	const float BaseHeightCm = BaseExtent.Z;
	const float SideClusterWidthCm = SideSpacingCm + CornerX;
	const float SideClusterDepthCm = DepthSpacingCm + CornerY;
	const float SpanTargetCm = UTrussMathLibrary::FeetToCentimeters(CubeArchWidthFt) - (2.0f * SideClusterWidthCm);
	const float LowerLegTargetCm = UTrussMathLibrary::FeetToCentimeters(CubeArchHeightFt) - CornerHeightCm - SideSpacingCm - CornerHeightCm;

	if (SpanTargetCm <= 0.0f || LowerLegTargetCm <= 0.0f)
	{
		return;
	}

	const FTrussCombinationResult SpanCombination = UTrussMathLibrary::FindBestTrussCombination(SpanTargetCm);
	const FTrussCombinationResult LowerLegCombination = UTrussMathLibrary::FindBestTrussCombination(LowerLegTargetCm);
	if (SpanCombination.Pieces.IsEmpty() || LowerLegCombination.Pieces.IsEmpty())
	{
		return;
	}

	const float LeftClusterStartX = CubeArchLeftClusterOffsetCm;
	const float LeftClusterEndX = LeftClusterStartX + SideClusterWidthCm;
	const float RightClusterStartX = SideClusterWidthCm + SpanCombination.ActualLengthCm;
	const float RightClusterEndX = RightClusterStartX + SideClusterWidthCm;
	const float FrontY = 0.0f;
	const float BackY = SideClusterDepthCm;
	const float LowerCornerZ = BaseHeightCm + LowerLegCombination.ActualLengthCm;
	const float UpperCornerZ = LowerCornerZ + CornerHeightCm + SideSpacingCm;
	const float LowerConnectorZ = LowerCornerZ + (CornerHeightCm * 0.5f) - CubeCornerConnectionOffsetCm;
	const float UpperConnectorZ = UpperCornerZ + (CornerHeightCm * 0.5f) - CubeCornerConnectionOffsetCm;
	const FRotator VerticalRotation(ArchVerticalRotationYDeg, ArchVerticalRotationZDeg, ArchVerticalRotationXDeg);
	const FRotator YRunRotation(0.0f, 90.0f, 0.0f);

	const TArray<float> ClusterStarts = {LeftClusterStartX, RightClusterStartX};
	const TArray<float> ClusterEnds = {LeftClusterEndX, RightClusterEndX};
	const TArray<float> DepthPositions = {FrontY, BackY};
	const TArray<float> ConnectorZValues = {LowerConnectorZ, UpperConnectorZ};

	for (int32 ClusterIndex = 0; ClusterIndex < ClusterStarts.Num(); ++ClusterIndex)
	{
		for (const float CornerMinX : {ClusterStarts[ClusterIndex], ClusterEnds[ClusterIndex]})
		{
			for (const float CornerMinY : DepthPositions)
			{
				const float LegCenterX = CornerMinX + CubeCornerConnectionOffsetCm + (CornerX * 0.5f);
				const float BaseMinX = LegCenterX - (BaseExtent.X * 0.5f);
				const float BaseMinY = CornerMinY + ArchBaseYOffsetCm - (BaseExtent.Y * 0.5f);

				AddPieceInstance(ETrussPieceType::Base, FVector(BaseMinX, BaseMinY, 0.0f), FRotator::ZeroRotator);
				AddStraightRun(
					LowerLegCombination.Pieces,
					FVector(LegCenterX - CubeCornerConnectionOffsetCm + ArchVerticalLegXOffsetCm, CornerMinY + ArchLegYOffsetCm, BaseHeightCm),
					VerticalRotation
				);
				AddPieceInstance(ETrussPieceType::CornerBlock, FVector(CornerMinX + CubeCornerConnectionOffsetCm, CornerMinY + ArchSpanYOffsetCm, LowerCornerZ), FRotator::ZeroRotator);
				AddPieceInstance(SidePiece, FVector(LegCenterX - CubeCornerConnectionOffsetCm + ArchVerticalLegXOffsetCm, CornerMinY + ArchLegYOffsetCm, LowerCornerZ + CornerHeightCm), VerticalRotation);
				AddPieceInstance(ETrussPieceType::CornerBlock, FVector(CornerMinX + CubeCornerConnectionOffsetCm, CornerMinY + ArchSpanYOffsetCm, UpperCornerZ), FRotator::ZeroRotator);
			}
		}
	}

	for (int32 ClusterIndex = 0; ClusterIndex < ClusterStarts.Num(); ++ClusterIndex)
	{
		const float ClusterLeftX = ClusterStarts[ClusterIndex];

		for (const float ConnectorZ : ConnectorZValues)
		{
			AddPieceInstance(SidePiece, FVector(ClusterLeftX + CornerX + CubeCornerConnectionOffsetCm, FrontY + ArchSpanYOffsetCm, ConnectorZ), FRotator::ZeroRotator);
			AddPieceInstance(SidePiece, FVector(ClusterLeftX + CornerX + CubeCornerConnectionOffsetCm, BackY + ArchSpanYOffsetCm, ConnectorZ), FRotator::ZeroRotator);
			AddPieceInstance(DepthPiece, FVector(ClusterLeftX + CubeYRunXOffsetCm, CornerY + ArchSpanYOffsetCm, ConnectorZ), YRunRotation);
			AddPieceInstance(DepthPiece, FVector(ClusterLeftX + SideClusterWidthCm + CubeYRunXOffsetCm, CornerY + ArchSpanYOffsetCm, ConnectorZ), YRunRotation);
		}
	}

	for (const float SpanZ : ConnectorZValues)
	{
		for (const float SpanY : DepthPositions)
		{
			AddStraightRun(
				SpanCombination.Pieces,
				FVector(LeftClusterEndX + CornerX + CubeCornerConnectionOffsetCm, SpanY + ArchSpanYOffsetCm, SpanZ),
				FRotator::ZeroRotator
			);
		}
	}

	const float ActualHeightCm = BaseHeightCm + LowerLegCombination.ActualLengthCm + (2.0f * CornerHeightCm) + SideSpacingCm;
	LastBuiltLengthFt = UTrussMathLibrary::CentimetersToFeet(ActualHeightCm);
	UpdateSelectionBounds();
}

void ATrussStructureActor::AddStraightRun(const TArray<ETrussPieceType>& Pieces, const FVector& StartMinLocation, const FRotator& Rotation)
{
	const FVector Direction = Rotation.RotateVector(FVector::ForwardVector);
	float CursorCm = 0.0f;

	for (ETrussPieceType PieceType : Pieces)
	{
		FTrussPieceDefinition PieceDefinition;
		UStaticMesh* PieceMesh = nullptr;
		GetPieceDefinition(PieceType, PieceDefinition, PieceMesh);
		const float PieceLengthCm = PieceDefinition.LengthCm > 0.0f ? PieceDefinition.LengthCm : UTrussMathLibrary::GetDefaultPieceLengthCm(PieceType);
		const FVector TargetMinLocation = StartMinLocation + (Direction * CursorCm);

		AddPieceInstance(PieceType, TargetMinLocation, Rotation);
		CursorCm += PieceLengthCm;
	}
}

void ATrussStructureActor::AddPieceInstance(ETrussPieceType PieceType, const FVector& TargetMinLocation, const FRotator& Rotation)
{
	FTrussPieceDefinition PieceDefinition;
	UStaticMesh* PieceMesh = nullptr;
	GetPieceDefinition(PieceType, PieceDefinition, PieceMesh);
	const float PieceLengthCm = PieceDefinition.LengthCm > 0.0f ? PieceDefinition.LengthCm : UTrussMathLibrary::GetDefaultPieceLengthCm(PieceType);

	if (PieceMesh)
	{
		UInstancedStaticMeshComponent* MeshComponent = FindOrCreateMeshComponent(PieceType, PieceMesh);
		if (MeshComponent)
		{
			const FVector Extent = GetScaledRotatedMeshExtent(PieceMesh, Rotation);
			ExpandGeneratedBounds(FBox(TargetMinLocation, TargetMinLocation + Extent));
			MeshComponent->AddInstance(FTransform(
				Rotation,
				GetMeshPlacementLocation(PieceMesh, TargetMinLocation, Rotation),
				FVector(MeshScaleMultiplier)
			));
		}
	}
	else
	{
		AddDebugPiece(PieceType, PieceLengthCm, TargetMinLocation, Rotation);
	}
}

bool ATrussStructureActor::GetPieceDefinition(ETrussPieceType PieceType, FTrussPieceDefinition& OutPiece, UStaticMesh*& OutMesh) const
{
	const bool bHasInventoryPiece = Inventory && Inventory->FindPiece(PieceType, OutPiece);
	if (!bHasInventoryPiece)
	{
		OutPiece.PieceType = PieceType;
		OutPiece.LengthCm = UTrussMathLibrary::GetDefaultPieceLengthCm(PieceType);
		OutPiece.StaticMesh = nullptr;
	}

	OutMesh = bHasInventoryPiece ? OutPiece.StaticMesh.Get() : nullptr;
	if (!OutMesh && bUseMajicGearDefaultMeshes)
	{
		OutMesh = LoadMajicGearDefaultMesh(PieceType);
	}

	return OutMesh || OutPiece.LengthCm > 0.0f;
}

void ATrussStructureActor::ClearGeneratedTruss()
{
	GeneratedBounds = FBox(EForceInit::ForceInit);

	for (UInstancedStaticMeshComponent* MeshComponent : {
		TenFootTrussInstances.Get(),
		EightFootTrussInstances.Get(),
		FiveFootTrussInstances.Get(),
		FourFootTrussInstances.Get(),
		TwoFootTrussInstances.Get(),
		CornerBlockInstances.Get(),
		BaseInstances.Get(),
		HingeInstances.Get()
	})
	{
		if (MeshComponent)
		{
			MeshComponent->ClearInstances();
		}
	}

	for (UActorComponent* Component : GeneratedComponents)
	{
		if (Component)
		{
			Component->DestroyComponent();
		}
	}

	GeneratedComponents.Reset();
	UpdateSelectionBounds();
}

void ATrussStructureActor::SetSelectionHighlighted(bool bHighlighted)
{
	if (SelectionBounds)
	{
		SelectionBounds->SetHiddenInGame(!bHighlighted);
	}
}

bool ATrussStructureActor::GetFixtureMountTransform(const FVector& WorldHitLocation, ETrussSlingType SlingType, FTransform& OutWorldTransform) const
{
	const FBox Bounds = GetGeneratedBounds();
	if (!Bounds.IsValid)
	{
		return false;
	}

	const FTransform ActorTransform = GetActorTransform();
	const FVector LocalHitLocation = ActorTransform.InverseTransformPosition(WorldHitLocation);
	FMountedFixtureDefinition FixtureDefinition;
	FixtureDefinition.SlingType = SlingType;
	FixtureDefinition.bUseExplicitSpan = true;
	FixtureDefinition.FixtureSpan = GetClosestFixtureSpan(LocalHitLocation);
	FixtureDefinition.bUseExplicitRail = true;
	FixtureDefinition.FixtureRail = GetClosestFixtureRail(LocalHitLocation, SlingType);
	FixtureDefinition.LocalHitLocation = LocalHitLocation;
	return GetFixtureMountTransformFromDefinition(FixtureDefinition, OutWorldTransform);
}

bool ATrussStructureActor::GetFixtureMountTransformFromDefinition(const FMountedFixtureDefinition& FixtureDefinition, FTransform& OutWorldTransform) const
{
	FBox Bounds = GetGeneratedBounds();
	if (!Bounds.IsValid)
	{
		return false;
	}

	GetFixtureSpanBounds(FixtureDefinition, Bounds);

	const FVector Min = Bounds.Min;
	const FVector Max = Bounds.Max;
	const float TopRailZ = Max.Z - RailInsetCm;
	const float BottomRailZ = Min.Z + RailInsetCm;
	const ETrussFixtureRail Rail = FixtureDefinition.bUseExplicitRail
		? FixtureDefinition.FixtureRail
		: GetClosestFixtureRail(FixtureDefinition.LocalHitLocation, FixtureDefinition.SlingType);
	const ETrussFixtureSpan Span = FixtureDefinition.bUseExplicitSpan
		? FixtureDefinition.FixtureSpan
		: GetClosestFixtureSpan(FixtureDefinition.LocalHitLocation);
	const bool bIsYRun = Span == ETrussFixtureSpan::LeftYRun || Span == ETrussFixtureSpan::RightYRun;

	float MountX = FMath::Clamp(FixtureDefinition.LocalHitLocation.X, Min.X, Max.X);
	float MountY = FMath::Clamp(FixtureDefinition.LocalHitLocation.Y, Min.Y, Max.Y);
	float RailZ = TopRailZ;

	if (bIsYRun)
	{
		const float LeftRailX = Min.X + RailInsetCm;
		const float RightRailX = Max.X - RailInsetCm;
		MountX = (Rail == ETrussFixtureRail::RightTop || Rail == ETrussFixtureRail::RightBottom)
			? RightRailX
			: LeftRailX;
		MountX += YRunMountXAdjustmentCm;
	}
	else
	{
		const float LeftRailY = Min.Y + RailInsetCm;
		const float RightRailY = Max.Y - RailInsetCm;
		MountY = (Rail == ETrussFixtureRail::RightTop || Rail == ETrussFixtureRail::RightBottom)
			? RightRailY
			: LeftRailY;
	}

	if (Rail == ETrussFixtureRail::LeftBottom || Rail == ETrussFixtureRail::RightBottom)
	{
		RailZ = BottomRailZ;
	}

	const FVector LocalMountLocation(MountX, MountY, RailZ);
	FRotator WorldRotation = GetActorRotation();
	if (bIsYRun)
	{
		WorldRotation.Yaw += 90.0f;
	}
	if (Rail == ETrussFixtureRail::LeftBottom || Rail == ETrussFixtureRail::RightBottom)
	{
		WorldRotation.Roll += 180.0f;
	}

	OutWorldTransform = FTransform(WorldRotation, GetActorTransform().TransformPosition(LocalMountLocation), FVector::OneVector);
	return true;
}

bool ATrussStructureActor::AddMountedFixtureDefinition(TSubclassOf<AActor> FixtureClass, ETrussSlingType SlingType, const FVector& WorldHitLocation, bool bRespawnFixtures)
{
	if (!FixtureClass)
	{
		return false;
	}

	FMountedFixtureDefinition& NewFixture = MountedFixtures.AddDefaulted_GetRef();
	NewFixture.FixtureClass = FixtureClass;
	NewFixture.SlingType = SlingType;
	NewFixture.LocalHitLocation = GetActorTransform().InverseTransformPosition(WorldHitLocation);
	NewFixture.bUseExplicitSpan = true;
	NewFixture.FixtureSpan = GetClosestFixtureSpan(NewFixture.LocalHitLocation);
	NewFixture.bUseExplicitRail = true;
	NewFixture.FixtureRail = GetClosestFixtureRail(NewFixture.LocalHitLocation, SlingType);

	if (bRespawnFixtures)
	{
		RebuildMountedFixtures();
	}

	return true;
}

void ATrussStructureActor::RebuildMountedFixtures()
{
	DestroyMountedFixtureActors();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (const FMountedFixtureDefinition& FixtureDefinition : MountedFixtures)
	{
		if (!FixtureDefinition.FixtureClass)
		{
			continue;
		}

		FTransform MountTransform;
		if (!GetFixtureMountTransformFromDefinition(FixtureDefinition, MountTransform))
		{
			continue;
		}

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParameters.ObjectFlags |= RF_Transactional;

		AActor* FixtureActor = World->SpawnActor<AActor>(FixtureDefinition.FixtureClass, MountTransform, SpawnParameters);
		if (!FixtureActor)
		{
			continue;
		}

		InvertClampOffsetProperties(FixtureActor);
		for (UActorComponent* Component : FixtureActor->GetComponents())
		{
			InvertClampOffsetProperties(Component);
		}

		FixtureActor->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
		SpawnedMountedFixtureActors.Add(FixtureActor);
	}
}

void ATrussStructureActor::AddEditorMountedFixture()
{
	if (!EditorFixtureClass)
	{
		return;
	}

	FMountedFixtureDefinition& NewFixture = MountedFixtures.AddDefaulted_GetRef();
	NewFixture.FixtureClass = EditorFixtureClass;
	NewFixture.SlingType = EditorFixtureSlingType;
	NewFixture.bUseExplicitSpan = true;
	NewFixture.FixtureSpan = EditorFixtureSpan;
	NewFixture.bUseExplicitRail = true;
	NewFixture.FixtureRail = EditorFixtureRail;
	NewFixture.LocalHitLocation = EditorFixtureLocalHitLocation;
	NewFixture.LocalHitLocation = GetInitialFixtureLocalLocation(NewFixture);
	RebuildMountedFixtures();
}

void ATrussStructureActor::ClearMountedFixtures()
{
	MountedFixtures.Reset();
	DestroyMountedFixtureActors();
}

UInstancedStaticMeshComponent* ATrussStructureActor::FindOrCreateMeshComponent(ETrussPieceType PieceType, UStaticMesh* StaticMesh)
{
	UInstancedStaticMeshComponent* MeshComponent = GetMeshComponentForPiece(PieceType);
	if (!MeshComponent)
	{
		return nullptr;
	}

	if (MeshComponent->GetStaticMesh() != StaticMesh)
	{
		MeshComponent->SetStaticMesh(StaticMesh);
	}

	return MeshComponent;
}

UInstancedStaticMeshComponent* ATrussStructureActor::GetMeshComponentForPiece(ETrussPieceType PieceType) const
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
	case ETrussPieceType::CornerBlock:
		return CornerBlockInstances;
	case ETrussPieceType::Base:
		return BaseInstances;
	case ETrussPieceType::Hinge:
		return HingeInstances;
	default:
		return nullptr;
	}
}

UStaticMesh* ATrussStructureActor::LoadMajicGearDefaultMesh(ETrussPieceType PieceType) const
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
	case ETrussPieceType::CornerBlock:
		AssetPath = TEXT("/Game/Majic_Gear/Truss/5way_Corner_Block/StaticMeshes/SM___Way_Corner_Block_v2.SM___Way_Corner_Block_v2");
		break;
	case ETrussPieceType::Base:
		AssetPath = TEXT("/Game/Majic_Gear/Truss/Base/StaticMeshes/SM_Base_v2.SM_Base_v2");
		break;
	default:
		break;
	}

	if (!AssetPath)
	{
		return nullptr;
	}

	return LoadObject<UStaticMesh>(nullptr, AssetPath);
}

FVector ATrussStructureActor::GetMeshPlacementLocation(UStaticMesh* StaticMesh, const FVector& TargetMinLocation, const FRotator& Rotation) const
{
	if (!StaticMesh)
	{
		return TargetMinLocation;
	}

	const FBoxSphereBounds Bounds = StaticMesh->GetBounds();
	const FVector LocalMin = Bounds.Origin - Bounds.BoxExtent;
	const FVector ScaledRotatedLocalMin = Rotation.RotateVector(LocalMin * MeshScaleMultiplier);
	return TargetMinLocation - ScaledRotatedLocalMin;
}

FVector ATrussStructureActor::GetScaledRotatedMeshExtent(UStaticMesh* StaticMesh, const FRotator& Rotation) const
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
					ZIndex == 0 ? LocalMin.Z : LocalMax.Z
				);
				const FVector RotatedCorner = Rotation.RotateVector(Corner * MeshScaleMultiplier);
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

void ATrussStructureActor::AddDebugPiece(ETrussPieceType PieceType, float PieceLengthCm, float StartX)
{
	AddDebugPiece(PieceType, PieceLengthCm, FVector(StartX, 0.0f, 0.0f), FRotator::ZeroRotator);
}

void ATrussStructureActor::AddDebugPiece(ETrussPieceType PieceType, float PieceLengthCm, const FVector& TargetMinLocation, const FRotator& Rotation)
{
	UBoxComponent* BoxComponent = NewObject<UBoxComponent>(this, NAME_None, RF_Transactional);
	if (!BoxComponent)
	{
		return;
	}

	BoxComponent->SetupAttachment(SceneRoot);
	BoxComponent->SetBoxExtent(FVector(PieceLengthCm * 0.5f, DebugCrossSectionCm * 0.5f, DebugCrossSectionCm * 0.5f));
	BoxComponent->SetRelativeRotation(Rotation);
	BoxComponent->SetRelativeLocation(TargetMinLocation + Rotation.RotateVector(FVector(PieceLengthCm * 0.5f, DebugCrossSectionCm * 0.5f, DebugCrossSectionCm * 0.5f)));
	BoxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BoxComponent->RegisterComponent();
	AddInstanceComponent(BoxComponent);
	GeneratedComponents.Add(BoxComponent);

	const FVector DebugExtent(PieceLengthCm, DebugCrossSectionCm, DebugCrossSectionCm);
	ExpandGeneratedBounds(FBox(TargetMinLocation, TargetMinLocation + DebugExtent));
}

void ATrussStructureActor::UpdateSelectionBounds()
{
	if (!SelectionBounds)
	{
		return;
	}

	const FBox Bounds = GetGeneratedBounds();
	if (!Bounds.IsValid)
	{
		SelectionBounds->SetBoxExtent(FVector(50.0f, 50.0f, 50.0f));
		SelectionBounds->SetRelativeLocation(FVector::ZeroVector);
		return;
	}

	const FVector CenterLocal = Bounds.GetCenter();
	const FVector Extent = Bounds.GetExtent() + FVector(5.0f, 5.0f, 5.0f);
	SelectionBounds->SetBoxExtent(Extent);
	SelectionBounds->SetRelativeLocation(CenterLocal);
}

FBox ATrussStructureActor::GetGeneratedBounds() const
{
	return GeneratedBounds;
}

void ATrussStructureActor::ExpandGeneratedBounds(const FBox& Bounds)
{
	if (!Bounds.IsValid)
	{
		return;
	}

	if (!GeneratedBounds.IsValid)
	{
		GeneratedBounds = Bounds;
		return;
	}

	GeneratedBounds += Bounds;
}

void ATrussStructureActor::DestroyMountedFixtureActors()
{
	for (AActor* FixtureActor : SpawnedMountedFixtureActors)
	{
		if (FixtureActor)
		{
			FixtureActor->Destroy();
		}
	}

	SpawnedMountedFixtureActors.Reset();
}

ETrussFixtureRail ATrussStructureActor::GetClosestFixtureRail(const FVector& LocalHitLocation, ETrussSlingType SlingType) const
{
	const FBox Bounds = GetGeneratedBounds();
	if (!Bounds.IsValid)
	{
		return SlingType == ETrussSlingType::UnderSlung ? ETrussFixtureRail::LeftBottom : ETrussFixtureRail::LeftTop;
	}

	FMountedFixtureDefinition FixtureDefinition;
	FixtureDefinition.LocalHitLocation = LocalHitLocation;
	FixtureDefinition.SlingType = SlingType;
	FixtureDefinition.bUseExplicitSpan = true;
	FixtureDefinition.FixtureSpan = GetClosestFixtureSpan(LocalHitLocation);

	const bool bIsYRun = FixtureDefinition.FixtureSpan == ETrussFixtureSpan::LeftYRun || FixtureDefinition.FixtureSpan == ETrussFixtureSpan::RightYRun;
	bool bUseRightRail = false;

	if (bIsYRun)
	{
		float LeftRailX = Bounds.Min.X + RailInsetCm;
		float RightRailX = Bounds.Max.X - RailInsetCm;
		GetFixtureSpanXRange(FixtureDefinition, LeftRailX, RightRailX);
		bUseRightRail = FMath::Abs(LocalHitLocation.X - RightRailX) < FMath::Abs(LocalHitLocation.X - LeftRailX);
	}
	else
	{
		float LeftRailY = Bounds.Min.Y + RailInsetCm;
		float RightRailY = Bounds.Max.Y - RailInsetCm;
		GetFixtureRailYRange(FixtureDefinition, LeftRailY, RightRailY);
		bUseRightRail = FMath::Abs(LocalHitLocation.Y - RightRailY) < FMath::Abs(LocalHitLocation.Y - LeftRailY);
	}

	if (SlingType == ETrussSlingType::UnderSlung)
	{
		return bUseRightRail ? ETrussFixtureRail::RightBottom : ETrussFixtureRail::LeftBottom;
	}

	return bUseRightRail ? ETrussFixtureRail::RightTop : ETrussFixtureRail::LeftTop;
}

ETrussFixtureSpan ATrussStructureActor::GetClosestFixtureSpan(const FVector& LocalHitLocation) const
{
	switch (BuildMode)
	{
	case ETrussBuildMode::Rectangle:
	case ETrussBuildMode::Cube:
	{
		FTrussPieceDefinition CornerDefinition;
		UStaticMesh* CornerMesh = nullptr;
		if (!GetPieceDefinition(ETrussPieceType::CornerBlock, CornerDefinition, CornerMesh))
		{
			return ETrussFixtureSpan::MainSpan;
		}

		const bool bIsRectangle = BuildMode == ETrussBuildMode::Rectangle;
		const FVector CornerExtent = GetScaledRotatedMeshExtent(CornerMesh, FRotator::ZeroRotator);
		const float InnerLengthCm = UTrussMathLibrary::FeetToCentimeters(bIsRectangle ? RectangleLengthFt : CubeLengthFt) - (2.0f * CornerExtent.X);
		const float InnerWidthCm = UTrussMathLibrary::FeetToCentimeters(bIsRectangle ? RectangleWidthFt : CubeWidthFt) - (2.0f * CornerExtent.Y);
		if (InnerLengthCm <= 0.0f || InnerWidthCm <= 0.0f)
		{
			return ETrussFixtureSpan::MainSpan;
		}

		const FTrussCombinationResult LengthCombination = UTrussMathLibrary::FindBestTrussCombination(InnerLengthCm);
		const FTrussCombinationResult WidthCombination = UTrussMathLibrary::FindBestTrussCombination(InnerWidthCm);
		const float FrontY = bIsRectangle ? 0.0f : ArchSpanYOffsetCm;
		const float BackY = (bIsRectangle ? 0.0f : ArchSpanYOffsetCm) + CornerExtent.Y + WidthCombination.ActualLengthCm;
		const float LeftX = bIsRectangle ? RectangleYRunXOffsetCm : CubeYRunXOffsetCm;
		const float RightX = (bIsRectangle ? RectangleYRunXOffsetCm : CubeYRunXOffsetCm) + CornerExtent.X + LengthCombination.ActualLengthCm;

		float BestDistance = TNumericLimits<float>::Max();
		ETrussFixtureSpan BestSpan = ETrussFixtureSpan::FrontXRun;
		const TArray<TPair<ETrussFixtureSpan, float>> Candidates = {
			TPair<ETrussFixtureSpan, float>(ETrussFixtureSpan::FrontXRun, FMath::Abs(LocalHitLocation.Y - FrontY)),
			TPair<ETrussFixtureSpan, float>(ETrussFixtureSpan::BackXRun, FMath::Abs(LocalHitLocation.Y - BackY)),
			TPair<ETrussFixtureSpan, float>(ETrussFixtureSpan::LeftYRun, FMath::Abs(LocalHitLocation.X - LeftX)),
			TPair<ETrussFixtureSpan, float>(ETrussFixtureSpan::RightYRun, FMath::Abs(LocalHitLocation.X - RightX))
		};

		for (const TPair<ETrussFixtureSpan, float>& Candidate : Candidates)
		{
			if (Candidate.Value < BestDistance)
			{
				BestDistance = Candidate.Value;
				BestSpan = Candidate.Key;
			}
		}

		return BestSpan;
	}

	default:
		return ETrussFixtureSpan::MainSpan;
	}
}

TArray<float> ATrussStructureActor::GetFixtureRailMinZCandidates() const
{
	TArray<float> Candidates;

	switch (BuildMode)
	{
	case ETrussBuildMode::StraightRun:
		Candidates.Add(UTrussMathLibrary::FeetToCentimeters(StraightRunHeightFt));
		break;

	case ETrussBuildMode::Rectangle:
		Candidates.Add(UTrussMathLibrary::FeetToCentimeters(RectangleHeightFt));
		break;

	case ETrussBuildMode::Arch:
	{
		FTrussPieceDefinition CornerDefinition;
		UStaticMesh* CornerMesh = nullptr;
		FTrussPieceDefinition BaseDefinition;
		UStaticMesh* BaseMesh = nullptr;
		if (GetPieceDefinition(ETrussPieceType::CornerBlock, CornerDefinition, CornerMesh)
			&& GetPieceDefinition(ETrussPieceType::Base, BaseDefinition, BaseMesh))
		{
			const FVector CornerExtent = GetScaledRotatedMeshExtent(CornerMesh, FRotator::ZeroRotator);
			const FVector BaseExtent = GetScaledRotatedMeshExtent(BaseMesh, FRotator::ZeroRotator);
			const float CornerWidthCm = CornerExtent.X;
			const float CornerHeightCm = CornerExtent.Z;
			const float BaseHeightCm = BaseExtent.Z;
			const float TargetHeightCm = UTrussMathLibrary::FeetToCentimeters(ArchHeightFt);
			const float TargetWidthCm = UTrussMathLibrary::FeetToCentimeters(ArchWidthFt);
			const float LegTargetCm = TargetHeightCm - CornerHeightCm;
			const float SpanTargetCm = TargetWidthCm - (2.0f * CornerWidthCm);
			if (LegTargetCm > 0.0f && SpanTargetCm > 0.0f)
			{
				const FTrussCombinationResult LegCombination = UTrussMathLibrary::FindBestTrussCombination(LegTargetCm);
				if (!LegCombination.Pieces.IsEmpty())
				{
					const float TopCornerZ = BaseHeightCm + LegCombination.ActualLengthCm;
					const float SpanZ = TopCornerZ + (CornerHeightCm * 0.5f) - ArchCornerConnectionOffsetCm;
					Candidates.Add(SpanZ);
				}
			}
		}
		break;
	}

	case ETrussBuildMode::Cube:
	{
		FTrussPieceDefinition CornerDefinition;
		UStaticMesh* CornerMesh = nullptr;
		FTrussPieceDefinition BaseDefinition;
		UStaticMesh* BaseMesh = nullptr;
		if (GetPieceDefinition(ETrussPieceType::CornerBlock, CornerDefinition, CornerMesh)
			&& GetPieceDefinition(ETrussPieceType::Base, BaseDefinition, BaseMesh))
		{
			const FVector CornerExtent = GetScaledRotatedMeshExtent(CornerMesh, FRotator::ZeroRotator);
			const FVector BaseExtent = GetScaledRotatedMeshExtent(BaseMesh, FRotator::ZeroRotator);
			const float CornerX = CornerExtent.X;
			const float CornerY = CornerExtent.Y;
			const float CornerHeightCm = CornerExtent.Z;
			const float BaseHeightCm = BaseExtent.Z;
			const float InnerLengthCm = UTrussMathLibrary::FeetToCentimeters(CubeLengthFt) - (2.0f * CornerX);
			const float InnerWidthCm = UTrussMathLibrary::FeetToCentimeters(CubeWidthFt) - (2.0f * CornerY);
			const float LegTargetCm = UTrussMathLibrary::FeetToCentimeters(CubeHeightFt) - CornerHeightCm;
			if (InnerLengthCm > 0.0f && InnerWidthCm > 0.0f && LegTargetCm > 0.0f)
			{
				const FTrussCombinationResult LegCombination = UTrussMathLibrary::FindBestTrussCombination(LegTargetCm);
				if (!LegCombination.Pieces.IsEmpty())
				{
					const float TopZ = BaseHeightCm + LegCombination.ActualLengthCm;
					const float SpanZ = TopZ + (CornerHeightCm * 0.5f) - CubeCornerConnectionOffsetCm;
					Candidates.Add(SpanZ);
				}
			}
		}
		break;
	}

	case ETrussBuildMode::CubeArch:
	{
		FTrussPieceDefinition CornerDefinition;
		UStaticMesh* CornerMesh = nullptr;
		FTrussPieceDefinition BaseDefinition;
		UStaticMesh* BaseMesh = nullptr;
		if (GetPieceDefinition(ETrussPieceType::CornerBlock, CornerDefinition, CornerMesh)
			&& GetPieceDefinition(ETrussPieceType::Base, BaseDefinition, BaseMesh))
		{
			const auto GetValidSpacingPiece = [](ETrussPieceType PieceType)
			{
				return UTrussMathLibrary::GetDefaultPieceLengthCm(PieceType) > 0.0f ? PieceType : ETrussPieceType::FourFoot;
			};

			const ETrussPieceType SidePiece = GetValidSpacingPiece(CubeArchSideSpacingPiece);
			const float SideSpacingCm = UTrussMathLibrary::GetDefaultPieceLengthCm(SidePiece);
			const FVector CornerExtent = GetScaledRotatedMeshExtent(CornerMesh, FRotator::ZeroRotator);
			const FVector BaseExtent = GetScaledRotatedMeshExtent(BaseMesh, FRotator::ZeroRotator);
			const float CornerX = CornerExtent.X;
			const float CornerHeightCm = CornerExtent.Z;
			const float BaseHeightCm = BaseExtent.Z;
			const float SideClusterWidthCm = SideSpacingCm + CornerX;
			const float SpanTargetCm = UTrussMathLibrary::FeetToCentimeters(CubeArchWidthFt) - (2.0f * SideClusterWidthCm);
			const float LowerLegTargetCm = UTrussMathLibrary::FeetToCentimeters(CubeArchHeightFt) - CornerHeightCm - SideSpacingCm - CornerHeightCm;
			if (SpanTargetCm > 0.0f && LowerLegTargetCm > 0.0f)
			{
				const FTrussCombinationResult LowerLegCombination = UTrussMathLibrary::FindBestTrussCombination(LowerLegTargetCm);
				if (!LowerLegCombination.Pieces.IsEmpty())
				{
					const float LowerCornerZ = BaseHeightCm + LowerLegCombination.ActualLengthCm;
					const float UpperCornerZ = LowerCornerZ + CornerHeightCm + SideSpacingCm;
					const float LowerConnectorZ = LowerCornerZ + (CornerHeightCm * 0.5f) - CubeCornerConnectionOffsetCm;
					const float UpperConnectorZ = UpperCornerZ + (CornerHeightCm * 0.5f) - CubeCornerConnectionOffsetCm;
					Candidates.Add(LowerConnectorZ);
					Candidates.Add(UpperConnectorZ);
				}
			}
		}
		break;
	}
	}

	if (Candidates.Num() == 0)
	{
		Candidates.Add(GetGeneratedBounds().Min.Z);
	}

	return Candidates;
}

bool ATrussStructureActor::GetFixtureRailYRange(const FMountedFixtureDefinition& FixtureDefinition, float& OutLeftRailY, float& OutRightRailY) const
{
	float RailThicknessY = DebugCrossSectionCm;
	for (ETrussPieceType PieceType : {ETrussPieceType::TenFoot, ETrussPieceType::EightFoot, ETrussPieceType::FiveFoot, ETrussPieceType::FourFoot, ETrussPieceType::TwoFoot})
	{
		FTrussPieceDefinition PieceDefinition;
		UStaticMesh* PieceMesh = nullptr;
		if (GetPieceDefinition(PieceType, PieceDefinition, PieceMesh) && PieceMesh)
		{
			RailThicknessY = GetScaledRotatedMeshExtent(PieceMesh, FRotator::ZeroRotator).Y;
			break;
		}
	}

	TArray<float> CandidateMinYValues;
	switch (BuildMode)
	{
	case ETrussBuildMode::StraightRun:
		CandidateMinYValues.Add(0.0f);
		break;

	case ETrussBuildMode::Arch:
		CandidateMinYValues.Add(ArchSpanYOffsetCm);
		break;

	case ETrussBuildMode::Rectangle:
	{
		FTrussPieceDefinition CornerDefinition;
		UStaticMesh* CornerMesh = nullptr;
		if (GetPieceDefinition(ETrussPieceType::CornerBlock, CornerDefinition, CornerMesh))
		{
			const FVector CornerExtent = GetScaledRotatedMeshExtent(CornerMesh, FRotator::ZeroRotator);
			const float TargetLengthCm = UTrussMathLibrary::FeetToCentimeters(RectangleLengthFt);
			const float TargetWidthCm = UTrussMathLibrary::FeetToCentimeters(RectangleWidthFt);
			const float InnerLengthCm = TargetLengthCm - (2.0f * CornerExtent.X);
			const float InnerWidthCm = TargetWidthCm - (2.0f * CornerExtent.Y);
			if (InnerLengthCm > 0.0f && InnerWidthCm > 0.0f)
			{
				const FTrussCombinationResult WidthCombination = UTrussMathLibrary::FindBestTrussCombination(InnerWidthCm);
				const float BackY = CornerExtent.Y + WidthCombination.ActualLengthCm;
				CandidateMinYValues.Add(0.0f);
				CandidateMinYValues.Add(BackY);
			}
		}
		break;
	}

	case ETrussBuildMode::Cube:
	{
		FTrussPieceDefinition CornerDefinition;
		UStaticMesh* CornerMesh = nullptr;
		if (GetPieceDefinition(ETrussPieceType::CornerBlock, CornerDefinition, CornerMesh))
		{
			const FVector CornerExtent = GetScaledRotatedMeshExtent(CornerMesh, FRotator::ZeroRotator);
			const float InnerWidthCm = UTrussMathLibrary::FeetToCentimeters(CubeWidthFt) - (2.0f * CornerExtent.Y);
			if (InnerWidthCm > 0.0f)
			{
				const FTrussCombinationResult WidthCombination = UTrussMathLibrary::FindBestTrussCombination(InnerWidthCm);
				const float BackY = CornerExtent.Y + WidthCombination.ActualLengthCm;
				CandidateMinYValues.Add(ArchSpanYOffsetCm);
				CandidateMinYValues.Add(BackY + ArchSpanYOffsetCm);
			}
		}
		break;
	}

	case ETrussBuildMode::CubeArch:
	{
		const auto GetValidSpacingPiece = [](ETrussPieceType PieceType)
		{
			return UTrussMathLibrary::GetDefaultPieceLengthCm(PieceType) > 0.0f ? PieceType : ETrussPieceType::FourFoot;
		};
		FTrussPieceDefinition CornerDefinition;
		UStaticMesh* CornerMesh = nullptr;
		if (GetPieceDefinition(ETrussPieceType::CornerBlock, CornerDefinition, CornerMesh))
		{
			const ETrussPieceType DepthPiece = GetValidSpacingPiece(CubeArchDepthSpacingPiece);
			const FVector CornerExtent = GetScaledRotatedMeshExtent(CornerMesh, FRotator::ZeroRotator);
			const float DepthSpacingCm = UTrussMathLibrary::GetDefaultPieceLengthCm(DepthPiece);
			const float SideClusterDepthCm = DepthSpacingCm + CornerExtent.Y;
			CandidateMinYValues.Add(ArchSpanYOffsetCm);
			CandidateMinYValues.Add(SideClusterDepthCm + ArchSpanYOffsetCm);
		}
		break;
	}
	}

	if (CandidateMinYValues.Num() == 0)
	{
		const FBox Bounds = GetGeneratedBounds();
		if (!Bounds.IsValid)
		{
			return false;
		}

		OutLeftRailY = Bounds.Min.Y + RailInsetCm;
		OutRightRailY = Bounds.Max.Y - RailInsetCm;
		return true;
	}

	float SelectedMinY = CandidateMinYValues[0];
	const ETrussFixtureSpan Span = FixtureDefinition.bUseExplicitSpan
		? FixtureDefinition.FixtureSpan
		: GetClosestFixtureSpan(FixtureDefinition.LocalHitLocation);

	if (Span == ETrussFixtureSpan::FrontXRun)
	{
		SelectedMinY = CandidateMinYValues[0];
	}
	else if (Span == ETrussFixtureSpan::BackXRun && CandidateMinYValues.Num() > 1)
	{
		SelectedMinY = CandidateMinYValues.Last();
	}
	else
	{
		float BestDistance = TNumericLimits<float>::Max();
		for (const float CandidateMinY : CandidateMinYValues)
		{
			const float CandidateCenterY = CandidateMinY + (RailThicknessY * 0.5f);
			const float Distance = FMath::Abs(FixtureDefinition.LocalHitLocation.Y - CandidateCenterY);
			if (Distance < BestDistance)
			{
				BestDistance = Distance;
				SelectedMinY = CandidateMinY;
			}
		}
	}

	OutLeftRailY = SelectedMinY + RailInsetCm;
	OutRightRailY = FMath::Max(OutLeftRailY, SelectedMinY + RailThicknessY - RailInsetCm);
	return true;
}

bool ATrussStructureActor::GetFixtureSpanXRange(const FMountedFixtureDefinition& FixtureDefinition, float& OutMinX, float& OutMaxX) const
{
	const FBox Bounds = GetGeneratedBounds();
	if (!Bounds.IsValid)
	{
		return false;
	}

	OutMinX = Bounds.Min.X;
	OutMaxX = Bounds.Max.X;

	const ETrussFixtureSpan Span = FixtureDefinition.bUseExplicitSpan
		? FixtureDefinition.FixtureSpan
		: GetClosestFixtureSpan(FixtureDefinition.LocalHitLocation);

	switch (BuildMode)
	{
	case ETrussBuildMode::Rectangle:
	{
		FTrussPieceDefinition CornerDefinition;
		UStaticMesh* CornerMesh = nullptr;
		if (!GetPieceDefinition(ETrussPieceType::CornerBlock, CornerDefinition, CornerMesh))
		{
			return true;
		}

		const FVector CornerExtent = GetScaledRotatedMeshExtent(CornerMesh, FRotator::ZeroRotator);
		const float InnerLengthCm = UTrussMathLibrary::FeetToCentimeters(RectangleLengthFt) - (2.0f * CornerExtent.X);
		if (InnerLengthCm <= 0.0f)
		{
			return true;
		}

		const FTrussCombinationResult LengthCombination = UTrussMathLibrary::FindBestTrussCombination(InnerLengthCm);
		if (Span == ETrussFixtureSpan::FrontXRun || Span == ETrussFixtureSpan::BackXRun)
		{
			OutMinX = CornerExtent.X;
			OutMaxX = CornerExtent.X + LengthCombination.ActualLengthCm;
		}
		else
		{
			float YRunThicknessX = DebugCrossSectionCm;
			for (ETrussPieceType PieceType : {ETrussPieceType::TenFoot, ETrussPieceType::EightFoot, ETrussPieceType::FiveFoot, ETrussPieceType::FourFoot, ETrussPieceType::TwoFoot})
			{
				FTrussPieceDefinition PieceDefinition;
				UStaticMesh* PieceMesh = nullptr;
				if (GetPieceDefinition(PieceType, PieceDefinition, PieceMesh) && PieceMesh)
				{
					YRunThicknessX = GetScaledRotatedMeshExtent(PieceMesh, FRotator(0.0f, 90.0f, 0.0f)).X;
					break;
				}
			}

			const float LeftRunX = RectangleYRunXOffsetCm;
			const float RightRunX = RectangleYRunXOffsetCm + CornerExtent.X + LengthCombination.ActualLengthCm;
			const float RunMinX = Span == ETrussFixtureSpan::RightYRun ? RightRunX : LeftRunX;
			OutMinX = RunMinX + RailInsetCm;
			OutMaxX = FMath::Max(OutMinX, RunMinX + YRunThicknessX - RailInsetCm);
		}
		break;
	}

	case ETrussBuildMode::Cube:
	{
		FTrussPieceDefinition CornerDefinition;
		UStaticMesh* CornerMesh = nullptr;
		if (!GetPieceDefinition(ETrussPieceType::CornerBlock, CornerDefinition, CornerMesh))
		{
			return true;
		}

		const FVector CornerExtent = GetScaledRotatedMeshExtent(CornerMesh, FRotator::ZeroRotator);
		const float InnerLengthCm = UTrussMathLibrary::FeetToCentimeters(CubeLengthFt) - (2.0f * CornerExtent.X);
		if (InnerLengthCm <= 0.0f)
		{
			return true;
		}

		const FTrussCombinationResult LengthCombination = UTrussMathLibrary::FindBestTrussCombination(InnerLengthCm);
		if (Span == ETrussFixtureSpan::FrontXRun || Span == ETrussFixtureSpan::BackXRun)
		{
			OutMinX = CornerExtent.X + CubeCornerConnectionOffsetCm;
			OutMaxX = OutMinX + LengthCombination.ActualLengthCm;
		}
		else
		{
			float YRunThicknessX = DebugCrossSectionCm;
			for (ETrussPieceType PieceType : {ETrussPieceType::TenFoot, ETrussPieceType::EightFoot, ETrussPieceType::FiveFoot, ETrussPieceType::FourFoot, ETrussPieceType::TwoFoot})
			{
				FTrussPieceDefinition PieceDefinition;
				UStaticMesh* PieceMesh = nullptr;
				if (GetPieceDefinition(PieceType, PieceDefinition, PieceMesh) && PieceMesh)
				{
					YRunThicknessX = GetScaledRotatedMeshExtent(PieceMesh, FRotator(0.0f, 90.0f, 0.0f)).X;
					break;
				}
			}

			const float LeftRunX = CubeYRunXOffsetCm;
			const float RightRunX = CubeYRunXOffsetCm + CornerExtent.X + LengthCombination.ActualLengthCm;
			const float RunMinX = Span == ETrussFixtureSpan::RightYRun ? RightRunX : LeftRunX;
			OutMinX = RunMinX + RailInsetCm;
			OutMaxX = FMath::Max(OutMinX, RunMinX + YRunThicknessX - RailInsetCm);
		}
		break;
	}

	default:
		break;
	}

	return true;
}

bool ATrussStructureActor::GetFixtureSpanYRange(const FMountedFixtureDefinition& FixtureDefinition, float& OutMinY, float& OutMaxY) const
{
	const FBox Bounds = GetGeneratedBounds();
	if (!Bounds.IsValid)
	{
		return false;
	}

	OutMinY = Bounds.Min.Y;
	OutMaxY = Bounds.Max.Y;

	const ETrussFixtureSpan Span = FixtureDefinition.bUseExplicitSpan
		? FixtureDefinition.FixtureSpan
		: GetClosestFixtureSpan(FixtureDefinition.LocalHitLocation);

	switch (BuildMode)
	{
	case ETrussBuildMode::Rectangle:
	{
		if (Span != ETrussFixtureSpan::LeftYRun && Span != ETrussFixtureSpan::RightYRun)
		{
			return true;
		}

		FTrussPieceDefinition CornerDefinition;
		UStaticMesh* CornerMesh = nullptr;
		if (!GetPieceDefinition(ETrussPieceType::CornerBlock, CornerDefinition, CornerMesh))
		{
			return true;
		}

		const FVector CornerExtent = GetScaledRotatedMeshExtent(CornerMesh, FRotator::ZeroRotator);
		const float InnerWidthCm = UTrussMathLibrary::FeetToCentimeters(RectangleWidthFt) - (2.0f * CornerExtent.Y);
		if (InnerWidthCm <= 0.0f)
		{
			return true;
		}

		const FTrussCombinationResult WidthCombination = UTrussMathLibrary::FindBestTrussCombination(InnerWidthCm);
		OutMinY = CornerExtent.Y;
		OutMaxY = CornerExtent.Y + WidthCombination.ActualLengthCm;
		break;
	}

	case ETrussBuildMode::Cube:
	{
		if (Span != ETrussFixtureSpan::LeftYRun && Span != ETrussFixtureSpan::RightYRun)
		{
			return true;
		}

		FTrussPieceDefinition CornerDefinition;
		UStaticMesh* CornerMesh = nullptr;
		if (!GetPieceDefinition(ETrussPieceType::CornerBlock, CornerDefinition, CornerMesh))
		{
			return true;
		}

		const FVector CornerExtent = GetScaledRotatedMeshExtent(CornerMesh, FRotator::ZeroRotator);
		const float InnerWidthCm = UTrussMathLibrary::FeetToCentimeters(CubeWidthFt) - (2.0f * CornerExtent.Y);
		if (InnerWidthCm <= 0.0f)
		{
			return true;
		}

		const FTrussCombinationResult WidthCombination = UTrussMathLibrary::FindBestTrussCombination(InnerWidthCm);
		OutMinY = CornerExtent.Y + ArchSpanYOffsetCm;
		OutMaxY = OutMinY + WidthCombination.ActualLengthCm;
		break;
	}

	default:
		break;
	}

	return true;
}

FVector ATrussStructureActor::GetInitialFixtureLocalLocation(const FMountedFixtureDefinition& FixtureDefinition) const
{
	FBox Bounds = GetGeneratedBounds();
	if (!Bounds.IsValid)
	{
		return FixtureDefinition.LocalHitLocation;
	}

	GetFixtureSpanBounds(FixtureDefinition, Bounds);

	FVector InitialLocation = FixtureDefinition.LocalHitLocation;
	const ETrussFixtureSpan Span = FixtureDefinition.bUseExplicitSpan
		? FixtureDefinition.FixtureSpan
		: GetClosestFixtureSpan(FixtureDefinition.LocalHitLocation);
	const bool bIsYRun = Span == ETrussFixtureSpan::LeftYRun || Span == ETrussFixtureSpan::RightYRun;

	if (bIsYRun)
	{
		const float LeftRailX = Bounds.Min.X + RailInsetCm;
		const float RightRailX = Bounds.Max.X - RailInsetCm;
		InitialLocation.X = (FixtureDefinition.FixtureRail == ETrussFixtureRail::RightTop || FixtureDefinition.FixtureRail == ETrussFixtureRail::RightBottom)
			? RightRailX
			: LeftRailX;
		InitialLocation.X += YRunMountXAdjustmentCm;
	}
	else
	{
		const float LeftRailY = Bounds.Min.Y + RailInsetCm;
		const float RightRailY = Bounds.Max.Y - RailInsetCm;
		InitialLocation.Y = (FixtureDefinition.FixtureRail == ETrussFixtureRail::RightTop || FixtureDefinition.FixtureRail == ETrussFixtureRail::RightBottom)
			? RightRailY
			: LeftRailY;
	}

	return InitialLocation;
}

bool ATrussStructureActor::GetFixtureSpanBounds(const FMountedFixtureDefinition& FixtureDefinition, FBox& OutBounds) const
{
	FTrussPieceDefinition StraightPieceDefinition;
	UStaticMesh* StraightPieceMesh = nullptr;
	GetPieceDefinition(ETrussPieceType::TenFoot, StraightPieceDefinition, StraightPieceMesh);
	if (!StraightPieceMesh)
	{
		GetPieceDefinition(ETrussPieceType::EightFoot, StraightPieceDefinition, StraightPieceMesh);
	}
	if (!StraightPieceMesh)
	{
		GetPieceDefinition(ETrussPieceType::FiveFoot, StraightPieceDefinition, StraightPieceMesh);
	}
	if (!StraightPieceMesh)
	{
		GetPieceDefinition(ETrussPieceType::FourFoot, StraightPieceDefinition, StraightPieceMesh);
	}
	if (!StraightPieceMesh)
	{
		GetPieceDefinition(ETrussPieceType::TwoFoot, StraightPieceDefinition, StraightPieceMesh);
	}

	const auto GetRunThickness = [&](const FRotator& Rotation)
	{
		if (StraightPieceMesh)
		{
			return GetScaledRotatedMeshExtent(StraightPieceMesh, Rotation);
		}

		if (Rotation.Yaw == 90.0f)
		{
			return FVector(DebugCrossSectionCm, DebugCrossSectionCm, DebugCrossSectionCm);
		}

		return FVector(DebugCrossSectionCm, DebugCrossSectionCm, DebugCrossSectionCm);
	};

	const ETrussFixtureSpan Span = FixtureDefinition.bUseExplicitSpan
		? FixtureDefinition.FixtureSpan
		: GetClosestFixtureSpan(FixtureDefinition.LocalHitLocation);

	switch (BuildMode)
	{
	case ETrussBuildMode::StraightRun:
	{
		const float HeightCm = UTrussMathLibrary::FeetToCentimeters(StraightRunHeightFt);
		const FTrussCombinationResult Combination = UTrussMathLibrary::FindBestTrussCombination(UTrussMathLibrary::FeetToCentimeters(LengthFt));
		const FVector Extent = GetRunThickness(FRotator::ZeroRotator);
		OutBounds = FBox(FVector(0.0f, 0.0f, HeightCm), FVector(Combination.ActualLengthCm, Extent.Y, HeightCm + Extent.Z));
		return true;
	}

	case ETrussBuildMode::Arch:
	{
		FTrussPieceDefinition CornerDefinition;
		UStaticMesh* CornerMesh = nullptr;
		FTrussPieceDefinition BaseDefinition;
		UStaticMesh* BaseMesh = nullptr;
		if (!GetPieceDefinition(ETrussPieceType::CornerBlock, CornerDefinition, CornerMesh)
			|| !GetPieceDefinition(ETrussPieceType::Base, BaseDefinition, BaseMesh))
		{
			return false;
		}

		const FVector CornerExtent = GetScaledRotatedMeshExtent(CornerMesh, FRotator::ZeroRotator);
		const FVector BaseExtent = GetScaledRotatedMeshExtent(BaseMesh, FRotator::ZeroRotator);
		const float LegTargetCm = UTrussMathLibrary::FeetToCentimeters(ArchHeightFt) - CornerExtent.Z;
		const float SpanTargetCm = UTrussMathLibrary::FeetToCentimeters(ArchWidthFt) - (2.0f * CornerExtent.X);
		if (LegTargetCm <= 0.0f || SpanTargetCm <= 0.0f)
		{
			return false;
		}

		const FTrussCombinationResult LegCombination = UTrussMathLibrary::FindBestTrussCombination(LegTargetCm);
		const FTrussCombinationResult SpanCombination = UTrussMathLibrary::FindBestTrussCombination(SpanTargetCm);
		if (LegCombination.Pieces.IsEmpty() || SpanCombination.Pieces.IsEmpty())
		{
			return false;
		}

		const float TopCornerZ = BaseExtent.Z + LegCombination.ActualLengthCm;
		const float SpanZ = TopCornerZ + (CornerExtent.Z * 0.5f) - ArchCornerConnectionOffsetCm;
		const FVector Extent = GetRunThickness(FRotator::ZeroRotator);
		OutBounds = FBox(
			FVector(CornerExtent.X + ArchCornerConnectionOffsetCm, ArchSpanYOffsetCm, SpanZ),
			FVector(CornerExtent.X + ArchCornerConnectionOffsetCm + SpanCombination.ActualLengthCm, ArchSpanYOffsetCm + Extent.Y, SpanZ + Extent.Z)
		);
		return true;
	}

	case ETrussBuildMode::Rectangle:
	{
		FTrussPieceDefinition CornerDefinition;
		UStaticMesh* CornerMesh = nullptr;
		if (!GetPieceDefinition(ETrussPieceType::CornerBlock, CornerDefinition, CornerMesh))
		{
			return false;
		}

		const FVector CornerExtent = GetScaledRotatedMeshExtent(CornerMesh, FRotator::ZeroRotator);
		const float HeightCm = UTrussMathLibrary::FeetToCentimeters(RectangleHeightFt);
		const float InnerLengthCm = UTrussMathLibrary::FeetToCentimeters(RectangleLengthFt) - (2.0f * CornerExtent.X);
		const float InnerWidthCm = UTrussMathLibrary::FeetToCentimeters(RectangleWidthFt) - (2.0f * CornerExtent.Y);
		if (InnerLengthCm <= 0.0f || InnerWidthCm <= 0.0f)
		{
			return false;
		}

		const FTrussCombinationResult LengthCombination = UTrussMathLibrary::FindBestTrussCombination(InnerLengthCm);
		const FTrussCombinationResult WidthCombination = UTrussMathLibrary::FindBestTrussCombination(InnerWidthCm);
		const FVector XRunExtent = GetRunThickness(FRotator::ZeroRotator);
		const FVector YRunExtent = GetRunThickness(FRotator(0.0f, 90.0f, 0.0f));
		const float BackY = CornerExtent.Y + WidthCombination.ActualLengthCm;
		const float RightX = CornerExtent.X + LengthCombination.ActualLengthCm;

		switch (Span)
		{
		case ETrussFixtureSpan::BackXRun:
			OutBounds = FBox(FVector(CornerExtent.X, BackY, HeightCm), FVector(CornerExtent.X + LengthCombination.ActualLengthCm, BackY + XRunExtent.Y, HeightCm + XRunExtent.Z));
			return true;
		case ETrussFixtureSpan::LeftYRun:
			OutBounds = FBox(FVector(RectangleYRunXOffsetCm, CornerExtent.Y, HeightCm), FVector(RectangleYRunXOffsetCm + YRunExtent.X, CornerExtent.Y + WidthCombination.ActualLengthCm, HeightCm + YRunExtent.Z));
			return true;
		case ETrussFixtureSpan::RightYRun:
			OutBounds = FBox(FVector(RightX + RectangleYRunXOffsetCm, CornerExtent.Y, HeightCm), FVector(RightX + RectangleYRunXOffsetCm + YRunExtent.X, CornerExtent.Y + WidthCombination.ActualLengthCm, HeightCm + YRunExtent.Z));
			return true;
		case ETrussFixtureSpan::FrontXRun:
		case ETrussFixtureSpan::MainSpan:
		default:
			OutBounds = FBox(FVector(CornerExtent.X, 0.0f, HeightCm), FVector(CornerExtent.X + LengthCombination.ActualLengthCm, XRunExtent.Y, HeightCm + XRunExtent.Z));
			return true;
		}
	}

	case ETrussBuildMode::Cube:
	{
		FTrussPieceDefinition CornerDefinition;
		UStaticMesh* CornerMesh = nullptr;
		FTrussPieceDefinition BaseDefinition;
		UStaticMesh* BaseMesh = nullptr;
		if (!GetPieceDefinition(ETrussPieceType::CornerBlock, CornerDefinition, CornerMesh)
			|| !GetPieceDefinition(ETrussPieceType::Base, BaseDefinition, BaseMesh))
		{
			return false;
		}

		const FVector CornerExtent = GetScaledRotatedMeshExtent(CornerMesh, FRotator::ZeroRotator);
		const FVector BaseExtent = GetScaledRotatedMeshExtent(BaseMesh, FRotator::ZeroRotator);
		const float InnerLengthCm = UTrussMathLibrary::FeetToCentimeters(CubeLengthFt) - (2.0f * CornerExtent.X);
		const float InnerWidthCm = UTrussMathLibrary::FeetToCentimeters(CubeWidthFt) - (2.0f * CornerExtent.Y);
		const float LegTargetCm = UTrussMathLibrary::FeetToCentimeters(CubeHeightFt) - CornerExtent.Z;
		if (InnerLengthCm <= 0.0f || InnerWidthCm <= 0.0f || LegTargetCm <= 0.0f)
		{
			return false;
		}

		const FTrussCombinationResult LengthCombination = UTrussMathLibrary::FindBestTrussCombination(InnerLengthCm);
		const FTrussCombinationResult WidthCombination = UTrussMathLibrary::FindBestTrussCombination(InnerWidthCm);
		const FTrussCombinationResult LegCombination = UTrussMathLibrary::FindBestTrussCombination(LegTargetCm);
		if (LengthCombination.Pieces.IsEmpty() || WidthCombination.Pieces.IsEmpty() || LegCombination.Pieces.IsEmpty())
		{
			return false;
		}

		const float TopZ = BaseExtent.Z + LegCombination.ActualLengthCm;
		const float SpanZ = TopZ + (CornerExtent.Z * 0.5f) - CubeCornerConnectionOffsetCm;
		const FVector XRunExtent = GetRunThickness(FRotator::ZeroRotator);
		const FVector YRunExtent = GetRunThickness(FRotator(0.0f, 90.0f, 0.0f));
		const float BackY = CornerExtent.Y + WidthCombination.ActualLengthCm + ArchSpanYOffsetCm;
		const float RightX = CornerExtent.X + LengthCombination.ActualLengthCm + CubeCornerConnectionOffsetCm;
		const float FrontXStart = CornerExtent.X + CubeCornerConnectionOffsetCm;

		switch (Span)
		{
		case ETrussFixtureSpan::BackXRun:
			OutBounds = FBox(FVector(FrontXStart, BackY, SpanZ), FVector(FrontXStart + LengthCombination.ActualLengthCm, BackY + XRunExtent.Y, SpanZ + XRunExtent.Z));
			return true;
		case ETrussFixtureSpan::LeftYRun:
			OutBounds = FBox(FVector(CubeYRunXOffsetCm, CornerExtent.Y + ArchSpanYOffsetCm, SpanZ), FVector(CubeYRunXOffsetCm + YRunExtent.X, CornerExtent.Y + ArchSpanYOffsetCm + WidthCombination.ActualLengthCm, SpanZ + YRunExtent.Z));
			return true;
		case ETrussFixtureSpan::RightYRun:
			OutBounds = FBox(FVector(RightX + (CubeYRunXOffsetCm - CubeCornerConnectionOffsetCm), CornerExtent.Y + ArchSpanYOffsetCm, SpanZ), FVector(RightX + (CubeYRunXOffsetCm - CubeCornerConnectionOffsetCm) + YRunExtent.X, CornerExtent.Y + ArchSpanYOffsetCm + WidthCombination.ActualLengthCm, SpanZ + YRunExtent.Z));
			return true;
		case ETrussFixtureSpan::FrontXRun:
		case ETrussFixtureSpan::MainSpan:
		default:
			OutBounds = FBox(FVector(FrontXStart, ArchSpanYOffsetCm, SpanZ), FVector(FrontXStart + LengthCombination.ActualLengthCm, ArchSpanYOffsetCm + XRunExtent.Y, SpanZ + XRunExtent.Z));
			return true;
		}
	}

	default:
		break;
	}

	return false;
}
