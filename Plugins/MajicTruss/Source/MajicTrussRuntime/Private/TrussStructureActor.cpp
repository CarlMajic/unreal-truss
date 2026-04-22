#include "TrussStructureActor.h"

#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "TrussMathLibrary.h"

ATrussStructureActor::ATrussStructureActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

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
		switch (BuildMode)
		{
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
}

void ATrussStructureActor::BuildStraightRun()
{
	ClearGeneratedTruss();

	const float TargetLengthCm = UTrussMathLibrary::FeetToCentimeters(LengthFt);
	const FTrussCombinationResult Combination = UTrussMathLibrary::FindBestTrussCombination(TargetLengthCm);

	AddStraightRun(Combination.Pieces, FVector::ZeroVector, FRotator::ZeroRotator);
	LastBuiltLengthFt = UTrussMathLibrary::CentimetersToFeet(Combination.ActualLengthCm);
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

	AddPieceInstance(ETrussPieceType::CornerBlock, FVector(0.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
	AddPieceInstance(ETrussPieceType::CornerBlock, FVector(RightX, 0.0f, 0.0f), FRotator::ZeroRotator);
	AddPieceInstance(ETrussPieceType::CornerBlock, FVector(RightX, BackY, 0.0f), FRotator::ZeroRotator);
	AddPieceInstance(ETrussPieceType::CornerBlock, FVector(0.0f, BackY, 0.0f), FRotator::ZeroRotator);

	AddStraightRun(LengthCombination.Pieces, FVector(CornerX, 0.0f, 0.0f), FRotator::ZeroRotator);
	AddStraightRun(LengthCombination.Pieces, FVector(CornerX, BackY, 0.0f), FRotator::ZeroRotator);
	AddStraightRun(WidthCombination.Pieces, FVector(RectangleYRunXOffsetCm, CornerY, 0.0f), FRotator(0.0f, 90.0f, 0.0f));
	AddStraightRun(WidthCombination.Pieces, FVector(RightX + RectangleYRunXOffsetCm, CornerY, 0.0f), FRotator(0.0f, 90.0f, 0.0f));

	LastBuiltLengthFt = UTrussMathLibrary::CentimetersToFeet((2.0f * (CornerX + CornerY)) + (2.0f * LengthCombination.ActualLengthCm) + (2.0f * WidthCombination.ActualLengthCm));
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
}
