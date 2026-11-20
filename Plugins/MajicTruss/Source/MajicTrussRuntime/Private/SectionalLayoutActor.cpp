#include "SectionalLayoutActor.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Animation/AnimationAsset.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Modules/ModuleManager.h"

namespace
{
constexpr float SectionalFallbackExtentCm = 75.0f;

bool IsSectionalStaticMeshAsset(const FAssetData& AssetData)
{
	return AssetData.AssetClassPath == UStaticMesh::StaticClass()->GetClassPathName();
}

FString NormalizeFolderPath(const FString& FolderPath)
{
	FString Normalized = FolderPath;
	Normalized.ReplaceInline(TEXT("\\"), TEXT("/"));
	Normalized.RemoveFromEnd(TEXT("/"));
	return Normalized;
}

FString MakeComponentSafeName(const FString& Value)
{
	FString Safe = Value;
	const TCHAR InvalidChars[] = TEXT(" /\\.-:(),[]{}'\"&");
	for (const TCHAR InvalidChar : InvalidChars)
	{
		if (InvalidChar == TEXT('\0'))
		{
			break;
		}
		Safe.ReplaceCharInline(InvalidChar, TEXT('_'));
	}
	return Safe;
}

void ConfigureGeneratedFurnitureComponent(UInstancedStaticMeshComponent* MeshComponent, bool bBlockPawnCollision, bool bAffectNavigation)
{
	if (!MeshComponent)
	{
		return;
	}

	MeshComponent->SetCollisionEnabled(bBlockPawnCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	MeshComponent->SetCollisionObjectType(ECC_WorldStatic);
	MeshComponent->SetCollisionResponseToAllChannels(bBlockPawnCollision ? ECR_Block : ECR_Ignore);
	MeshComponent->SetGenerateOverlapEvents(false);
	MeshComponent->SetCanEverAffectNavigation(bAffectNavigation);
	MeshComponent->bDisallowNanite = true;
}

FRotator MakeAISlotFacingRotation(const FVector& LocalOffset, const FVector& FacingLocalPoint, float YawOffsetDegrees)
{
	FVector Direction = FacingLocalPoint - LocalOffset;
	Direction.Z = 0.0f;
	if (Direction.IsNearlyZero())
	{
		return FRotator(0.0f, YawOffsetDegrees, 0.0f);
	}

	FRotator Rotation = Direction.Rotation();
	Rotation.Pitch = 0.0f;
	Rotation.Roll = 0.0f;
	Rotation.Yaw += YawOffsetDegrees;
	return Rotation;
}
}

ASectionalLayoutActor::ASectionalLayoutActor()
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
	SelectionBounds->ShapeColor = FColor::Orange;

	AIAnimationPreviewMesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/IdleSitPoseAnimations/Mannequin/Character/Mesh/SK_Mannequin.SK_Mannequin")));
	AIAnimationPreviewDefaultAnimation = TSoftObjectPtr<UAnimationAsset>(FSoftObjectPath(TEXT("/Game/IdleSitPoseAnimations/Animations/Relaxed_Sit.Relaxed_Sit")));
}

void ASectionalLayoutActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (bBuildOnConstruction)
	{
		RebuildSectionalLayout();
	}
}

#if WITH_EDITOR
void ASectionalLayoutActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RebuildSectionalLayout();
}
#endif

void ASectionalLayoutActor::RebuildSectionalLayout()
{
	WidthSections = FMath::Max(1, WidthSections);
	DepthSections = FMath::Max(1, DepthSections);
	AISlotPreviewLocalOffsets.Reset();
	AIEntryPreviewLocalOffsets.Reset();
	ClearGenerated();

	const FString ResolvedArmlessItem = ResolveSelectedItem(ArmlessAssetFolder, ArmlessItem);
	const FString ResolvedCornerItem = ResolveSelectedItem(CornerAssetFolder, CornerItem);

	if (ArmlessItem.IsEmpty())
	{
		ArmlessItem = ResolvedArmlessItem;
	}
	if (CornerItem.IsEmpty())
	{
		CornerItem = ResolvedCornerItem;
	}

	if (bUseArmlessItemDefaultSpacing)
	{
		SectionSpacingCm = GetDefaultSpacingForArmlessItem(ResolvedArmlessItem);
	}

	if (!ResolvedArmlessItem.IsEmpty())
	{
		for (int32 XIndex = 0; XIndex < WidthSections; ++XIndex)
		{
			const FVector GridLocation(XIndex * SectionSpacingCm, 0.0f, 0.0f);
			AddArmlessAt(ResolvedArmlessItem, GridLocation, 0.0f);
			AddAISlotPreviewSource(ApplyFootprintCentering(GridLocation) + FrontAISlotOffsetCm);
		}

		if (HasLeftSide())
		{
			const float LeftX = -SectionSpacingCm;
			for (int32 YIndex = 1; YIndex <= DepthSections; ++YIndex)
			{
				const FVector GridLocation(LeftX, YIndex * SectionSpacingCm, 0.0f);
				AddArmlessAt(ResolvedArmlessItem, GridLocation, -90.0f);
				AddAISlotPreviewSource(ApplyFootprintCentering(GridLocation) + LeftAISlotOffsetCm);
			}
		}

		if (HasRightSide())
		{
			const float RightX = WidthSections * SectionSpacingCm;
			for (int32 YIndex = 1; YIndex <= DepthSections; ++YIndex)
			{
				const FVector GridLocation(RightX, YIndex * SectionSpacingCm, 0.0f);
				AddArmlessAt(ResolvedArmlessItem, GridLocation, 90.0f);
				AddAISlotPreviewSource(ApplyFootprintCentering(GridLocation) + RightAISlotOffsetCm);
			}
		}

		if (HasBackSide())
		{
			const float BackY = (DepthSections + 1) * SectionSpacingCm;
			for (int32 XIndex = 0; XIndex < WidthSections; ++XIndex)
			{
				const FVector GridLocation(XIndex * SectionSpacingCm, BackY, 0.0f);
				AddArmlessAt(ResolvedArmlessItem, GridLocation, 180.0f);
				AddAISlotPreviewSource(ApplyFootprintCentering(GridLocation) + BackAISlotOffsetCm);
			}
		}
	}

	BuildAIEntryPreviewSources();

	if (!ResolvedCornerItem.IsEmpty())
	{
		const float LeftX = -SectionSpacingCm;
		const float RightX = WidthSections * SectionSpacingCm;
		const float BackY = (DepthSections + 1) * SectionSpacingCm;

		AddCornerAt(ResolvedCornerItem, FVector(LeftX, 0.0f, 0.0f), 0.0f);
		AddCornerAt(ResolvedCornerItem, FVector(RightX, 0.0f, 0.0f), 90.0f);

		if (HasLeftSide())
		{
			AddCornerAt(ResolvedCornerItem, FVector(LeftX, BackY, 0.0f), -90.0f);
		}
		if (HasRightSide())
		{
			AddCornerAt(ResolvedCornerItem, FVector(RightX, BackY, 0.0f), 180.0f);
		}
	}

	UpdateSelectionBounds();
	RefreshAISlotPreviewComponents();
}

void ASectionalLayoutActor::ApplyBuildDefinition(const FSectionalLayoutBuildDefinition& Definition, bool bRebuildNow)
{
	ArmlessItem = Definition.ArmlessItem;
	CornerItem = Definition.CornerItem;
	Shape = Definition.Shape;
	WidthSections = FMath::Max(1, Definition.WidthSections);
	DepthSections = FMath::Max(1, Definition.DepthSections);
	SectionSpacingCm = Definition.SectionSpacingCm;
	bUseArmlessItemDefaultSpacing = Definition.bUseArmlessItemDefaultSpacing;

	if (bRebuildNow)
	{
		RebuildSectionalLayout();
	}
}

FSectionalLayoutBuildDefinition ASectionalLayoutActor::GetBuildDefinition() const
{
	FSectionalLayoutBuildDefinition Definition;
	Definition.ArmlessItem = ArmlessItem;
	Definition.CornerItem = CornerItem;
	Definition.Shape = Shape;
	Definition.WidthSections = WidthSections;
	Definition.DepthSections = DepthSections;
	Definition.SectionSpacingCm = SectionSpacingCm;
	Definition.bUseArmlessItemDefaultSpacing = bUseArmlessItemDefaultSpacing;
	return Definition;
}

void ASectionalLayoutActor::SetSelectionHighlighted(bool bHighlighted)
{
	if (SelectionBounds)
	{
		SelectionBounds->SetHiddenInGame(!bHighlighted);
	}
}

void ASectionalLayoutActor::GetAISlotPreviewLocalOffsets(TArray<FVector>& OutOffsets) const
{
	OutOffsets.Reset();

	for (int32 Index = 0; Index < AISlotPreviewLocalOffsets.Num(); ++Index)
	{
		const FVector PerSlotOffset = AISlotPerSlotOffsetsCm.IsValidIndex(Index)
			? AISlotPerSlotOffsetsCm[Index]
			: FVector::ZeroVector;
		OutOffsets.Add(AISlotPreviewLocalOffsets[Index] + AISlotPreviewOffsetCm + PerSlotOffset);
	}
}

void ASectionalLayoutActor::GetAIEntryPreviewLocalOffsets(TArray<FVector>& OutOffsets) const
{
	OutOffsets.Reset();

	for (const FVector& LocalOffset : AIEntryPreviewLocalOffsets)
	{
		OutOffsets.Add(LocalOffset + AIEntryPreviewOffsetCm);
	}
}

void ASectionalLayoutActor::GetAISlotPreviewAnimations(TArray<TSoftObjectPtr<UAnimationAsset>>& OutAnimations) const
{
	TArray<FVector> PreviewOffsets;
	GetAISlotPreviewLocalOffsets(PreviewOffsets);

	OutAnimations.Reset();
	for (int32 Index = 0; Index < PreviewOffsets.Num(); ++Index)
	{
		OutAnimations.Add(AIAnimationPreviewPerSlotAnimations.IsValidIndex(Index)
			? AIAnimationPreviewPerSlotAnimations[Index]
			: AIAnimationPreviewDefaultAnimation);
	}
}

void ASectionalLayoutActor::GetAISlotPreviewLocalRotations(TArray<FRotator>& OutRotations) const
{
	TArray<FVector> PreviewOffsets;
	GetAISlotPreviewLocalOffsets(PreviewOffsets);

	OutRotations.Reset();
	for (int32 Index = 0; Index < PreviewOffsets.Num(); ++Index)
	{
		FRotator Rotation = MakeAISlotFacingRotation(PreviewOffsets[Index], AISlotFacingLocalPointCm, AISlotFacingYawOffsetDegrees);
		if (AISlotPerSlotRotationOffsets.IsValidIndex(Index))
		{
			Rotation += AISlotPerSlotRotationOffsets[Index];
		}
		OutRotations.Add(Rotation);
	}
}

TArray<FString> ASectionalLayoutActor::GetArmlessOptions() const
{
	return GetFurnitureItemOptions(ArmlessAssetFolder);
}

TArray<FString> ASectionalLayoutActor::GetCornerOptions() const
{
	return GetFurnitureItemOptions(CornerAssetFolder);
}

void ASectionalLayoutActor::ClearGenerated()
{
	GeneratedBounds = FBox(EForceInit::ForceInit);
	CurrentGeneratedMeshCount = 0;

	TInlineComponentArray<UInstancedStaticMeshComponent*> ExistingInstanceComponents(this);
	for (UInstancedStaticMeshComponent* MeshComponent : ExistingInstanceComponents)
	{
		if (MeshComponent && MeshComponent->GetName().StartsWith(TEXT("Sectional_")) && !GeneratedMeshComponents.Contains(MeshComponent))
		{
			GeneratedMeshComponents.Add(MeshComponent);
		}
	}

	for (UInstancedStaticMeshComponent* MeshComponent : GeneratedMeshComponents)
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

void ASectionalLayoutActor::AddFurnitureItem(const FString& CategoryFolder, const FString& ItemName, const TCHAR* Prefix, const FTransform& ItemTransform)
{
	for (const FSoftObjectPath& MeshPath : GetMeshPathsForFurnitureItem(CategoryFolder, ItemName))
	{
		UStaticMesh* Mesh = Cast<UStaticMesh>(MeshPath.TryLoad());
		if (!Mesh)
		{
			continue;
		}

		if (UInstancedStaticMeshComponent* MeshComponent = FindOrCreateMeshBucket(MeshPath, Prefix))
		{
			if (MeshComponent->GetStaticMesh() != Mesh)
			{
				MeshComponent->SetStaticMesh(Mesh);
			}

			MeshComponent->SetVisibility(true);
			MeshComponent->SetHiddenInGame(false);
			MeshComponent->AddInstance(ItemTransform);
			++CurrentGeneratedMeshCount;
			ExpandGeneratedBounds(Mesh, ItemTransform);
		}
	}
}

UInstancedStaticMeshComponent* ASectionalLayoutActor::FindOrCreateMeshBucket(const FSoftObjectPath& MeshPath, const TCHAR* Prefix)
{
	const FString SafeAssetName = MakeComponentSafeName(MeshPath.GetAssetName());
	const FName ComponentName(*FString::Printf(TEXT("Sectional_%s_%s"), Prefix, *SafeAssetName));
	if (UInstancedStaticMeshComponent* ExistingComponent = FindObjectFast<UInstancedStaticMeshComponent>(this, ComponentName))
	{
		ConfigureGeneratedFurnitureComponent(ExistingComponent, bBlockPawnCollision, bAffectNavigation);
		GeneratedMeshComponents.AddUnique(ExistingComponent);
		return ExistingComponent;
	}

	UInstancedStaticMeshComponent* MeshComponent = NewObject<UInstancedStaticMeshComponent>(this, ComponentName);
	if (!MeshComponent)
	{
		return nullptr;
	}

	MeshComponent->SetupAttachment(SceneRoot);
	ConfigureGeneratedFurnitureComponent(MeshComponent, bBlockPawnCollision, bAffectNavigation);
	MeshComponent->RegisterComponent();
	AddInstanceComponent(MeshComponent);
	GeneratedMeshComponents.Add(MeshComponent);
	return MeshComponent;
}

TArray<FString> ASectionalLayoutActor::GetFurnitureItemOptions(const FString& CategoryFolder) const
{
	TArray<FString> Options;
	const FString NormalizedCategoryFolder = NormalizeFolderPath(CategoryFolder);
	if (NormalizedCategoryFolder.IsEmpty())
	{
		return Options;
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> Assets;
	AssetRegistryModule.Get().GetAssetsByPath(*NormalizedCategoryFolder, Assets, true);

	TSet<FString> UniqueItems;
	const FString Prefix = NormalizedCategoryFolder + TEXT("/");
	for (const FAssetData& AssetData : Assets)
	{
		if (!IsSectionalStaticMeshAsset(AssetData))
		{
			continue;
		}

		const FString PackagePath = AssetData.PackagePath.ToString();
		if (!PackagePath.StartsWith(Prefix))
		{
			continue;
		}

		FString Remainder = PackagePath.RightChop(Prefix.Len());
		FString ItemName;
		FString Unused;
		if (Remainder.Split(TEXT("/"), &ItemName, &Unused) && !ItemName.IsEmpty())
		{
			UniqueItems.Add(ItemName);
		}
	}

	for (const FString& ItemName : UniqueItems)
	{
		if (ItemName != TEXT("Corners"))
		{
			Options.Add(ItemName);
		}
	}
	Options.Sort();
	return Options;
}

TArray<FSoftObjectPath> ASectionalLayoutActor::GetMeshPathsForFurnitureItem(const FString& CategoryFolder, const FString& ItemName) const
{
	TArray<FSoftObjectPath> MeshPaths;
	if (ItemName.IsEmpty())
	{
		return MeshPaths;
	}

	const FString NormalizedCategoryFolder = NormalizeFolderPath(CategoryFolder);
	const FString ItemFolder = NormalizedCategoryFolder / ItemName;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> Assets;
	AssetRegistryModule.Get().GetAssetsByPath(*ItemFolder, Assets, true);

	Assets.StableSort([](const FAssetData& A, const FAssetData& B)
	{
		if (A.PackagePath == B.PackagePath)
		{
			return A.AssetName.LexicalLess(B.AssetName);
		}

		return A.PackagePath.LexicalLess(B.PackagePath);
	});

	for (const FAssetData& AssetData : Assets)
	{
		if (IsSectionalStaticMeshAsset(AssetData))
		{
			MeshPaths.Add(AssetData.ToSoftObjectPath());
		}
	}

	return MeshPaths;
}

FString ASectionalLayoutActor::ResolveSelectedItem(const FString& CategoryFolder, const FString& SelectedItem) const
{
	const TArray<FString> Options = GetFurnitureItemOptions(CategoryFolder);
	if (Options.Contains(SelectedItem))
	{
		return SelectedItem;
	}

	return Options.Num() > 0 ? Options[0] : FString();
}

float ASectionalLayoutActor::GetDefaultSpacingForArmlessItem(const FString& ResolvedArmlessItem) const
{
	if (ResolvedArmlessItem.Contains(TEXT("Boca"), ESearchCase::IgnoreCase))
	{
		return -55.0f;
	}
	if (ResolvedArmlessItem.Contains(TEXT("Function"), ESearchCase::IgnoreCase))
	{
		return -84.0f;
	}
	if (ResolvedArmlessItem.Contains(TEXT("Grammersey"), ESearchCase::IgnoreCase) ||
		ResolvedArmlessItem.Contains(TEXT("Grammercey"), ESearchCase::IgnoreCase))
	{
		return -71.0f;
	}
	if (ResolvedArmlessItem.Contains(TEXT("Hemingway"), ESearchCase::IgnoreCase))
	{
		return -77.0f;
	}
	if (ResolvedArmlessItem.Contains(TEXT("Sophistication"), ESearchCase::IgnoreCase))
	{
		return -60.0f;
	}

	return SectionSpacingCm;
}

void ASectionalLayoutActor::AddArmlessAt(const FString& ResolvedArmlessItem, const FVector& GridLocation, float YawDegrees)
{
	FRotator Rotation = ArmlessPlacementRotation;
	Rotation.Yaw += YawDegrees;
	const FVector ResolvedOffset = bRotateArmlessOffsetWithPiece
		? Rotation.RotateVector(ArmlessPlacementOffsetCm)
		: ArmlessPlacementOffsetCm;
	AddFurnitureItem(
		ArmlessAssetFolder,
		ResolvedArmlessItem,
		TEXT("Armless"),
		FTransform(Rotation, ApplyFootprintCentering(GridLocation) + ResolvedOffset, ArmlessScale));
}

void ASectionalLayoutActor::AddCornerAt(const FString& ResolvedCornerItem, const FVector& GridLocation, float YawDegrees)
{
	FRotator Rotation = CornerPlacementRotation;
	Rotation.Yaw += YawDegrees;
	const FVector ResolvedOffset = bRotateCornerOffsetWithPiece
		? Rotation.RotateVector(CornerPlacementOffsetCm)
		: CornerPlacementOffsetCm;
	AddFurnitureItem(
		CornerAssetFolder,
		ResolvedCornerItem,
		TEXT("Corner"),
		FTransform(Rotation, ApplyFootprintCentering(GridLocation) + ResolvedOffset, CornerScale));
}

FVector ASectionalLayoutActor::ApplyFootprintCentering(const FVector& GridLocation) const
{
	if (!bCenterOnActor)
	{
		return GridLocation;
	}

	const float WidthCm = FMath::Max(0, WidthSections - 1) * SectionSpacingCm;
	const float DepthCm = HasLeftSide() || HasRightSide() || HasBackSide() ? (DepthSections + 1) * SectionSpacingCm : 0.0f;
	return GridLocation - FVector(WidthCm * 0.5f, DepthCm * 0.5f, 0.0f);
}

bool ASectionalLayoutActor::HasLeftSide() const
{
	return Shape == ESectionalLayoutShape::TwoSidedL ||
		Shape == ESectionalLayoutShape::ThreeSidedU ||
		Shape == ESectionalLayoutShape::FourSidedBox;
}

bool ASectionalLayoutActor::HasRightSide() const
{
	return Shape == ESectionalLayoutShape::ThreeSidedU ||
		Shape == ESectionalLayoutShape::FourSidedBox;
}

bool ASectionalLayoutActor::HasBackSide() const
{
	return Shape == ESectionalLayoutShape::FourSidedBox;
}

void ASectionalLayoutActor::ExpandGeneratedBounds(UStaticMesh* Mesh, const FTransform& Transform)
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
		Location - FVector(SectionalFallbackExtentCm),
		Location + FVector(SectionalFallbackExtentCm));
}

void ASectionalLayoutActor::UpdateSelectionBounds()
{
	if (!SelectionBounds)
	{
		return;
	}

	SelectionBounds->SetWorldScale3D(FVector::OneVector);

	if (!GeneratedBounds.IsValid)
	{
		SelectionBounds->SetRelativeLocation(FVector::ZeroVector);
		SelectionBounds->SetBoxExtent(FVector(SectionalFallbackExtentCm));
		return;
	}

	SelectionBounds->SetRelativeLocation(GeneratedBounds.GetCenter());
	SelectionBounds->SetBoxExtent(GeneratedBounds.GetExtent().ComponentMax(FVector(50.0f, 50.0f, 50.0f)));
}

void ASectionalLayoutActor::AddAISlotPreviewSource(const FVector& LocalOffset)
{
	AISlotPreviewLocalOffsets.Add(LocalOffset);
}

void ASectionalLayoutActor::AddAIEntryPreviewSource(const FVector& LocalOffset)
{
	AIEntryPreviewLocalOffsets.Add(LocalOffset);
}

void ASectionalLayoutActor::BuildAIEntryPreviewSources()
{
	const float DirectionSign = SectionSpacingCm >= 0.0f ? 1.0f : -1.0f;
	const float StepCm = FMath::Max(FMath::Abs(SectionSpacingCm), 50.0f);
	const float EntryDistanceCm = FMath::Max(AIEntryOffsetFromFootprintCm, StepCm);
	const float FrontY = 0.0f;
	const float BackY = (DepthSections + 1) * SectionSpacingCm;
	const float LeftX = -SectionSpacingCm;
	const float RightX = WidthSections * SectionSpacingCm;
	const float MidX = (WidthSections - 1) * SectionSpacingCm * 0.5f;

	const auto AddBackEntries = [this, BackY, DirectionSign, EntryDistanceCm]()
	{
		const int32 EntryCount = FMath::Clamp(WidthSections, 2, 4);
		if (EntryCount == 1)
		{
			AddAIEntryPreviewSource(ApplyFootprintCentering(FVector(0.0f, BackY + DirectionSign * EntryDistanceCm, 0.0f)));
			return;
		}

		for (int32 EntryIndex = 0; EntryIndex < EntryCount; ++EntryIndex)
		{
			const float Alpha = EntryCount > 1 ? static_cast<float>(EntryIndex) / static_cast<float>(EntryCount - 1) : 0.5f;
			const float X = FMath::Lerp(0.0f, static_cast<float>(WidthSections - 1) * SectionSpacingCm, Alpha);
			AddAIEntryPreviewSource(ApplyFootprintCentering(FVector(X, BackY + DirectionSign * EntryDistanceCm, 0.0f)));
		}
	};

	const auto AddSideEntries = [this, DirectionSign, EntryDistanceCm](float X, bool bRightSide)
	{
		const int32 EntryCount = FMath::Clamp(DepthSections, 2, 4);
		for (int32 EntryIndex = 0; EntryIndex < EntryCount; ++EntryIndex)
		{
			const float Alpha = EntryCount > 1 ? static_cast<float>(EntryIndex) / static_cast<float>(EntryCount - 1) : 0.5f;
			const float Y = FMath::Lerp(SectionSpacingCm, static_cast<float>(DepthSections) * SectionSpacingCm, Alpha);
			const float EntryX = bRightSide ? X + DirectionSign * EntryDistanceCm : X - DirectionSign * EntryDistanceCm;
			AddAIEntryPreviewSource(ApplyFootprintCentering(FVector(EntryX, Y, 0.0f)));
		}
	};

	const FVector FrontEntry = ApplyFootprintCentering(FVector(MidX, FrontY - DirectionSign * EntryDistanceCm, 0.0f));

	switch (Shape)
	{
	case ESectionalLayoutShape::OneSided:
		AddSideEntries(LeftX, false);
		AddSideEntries(RightX, true);
		AddBackEntries();
		break;

	case ESectionalLayoutShape::TwoSidedL:
		AddSideEntries(RightX, true);
		AddBackEntries();
		break;

	case ESectionalLayoutShape::ThreeSidedU:
		AddBackEntries();
		break;

	case ESectionalLayoutShape::FourSidedBox:
		AddAIEntryPreviewSource(FrontEntry);
		break;

	default:
		AddBackEntries();
		break;
	}
}

void ASectionalLayoutActor::RefreshAISlotPreviewComponents()
{
#if WITH_EDITOR
	ClearEditorPreviewComponents();

	if (!bShowAISlotPreviewInEditor || !SceneRoot)
	{
		return;
	}

	TArray<FVector> PreviewOffsets;
	GetAISlotPreviewLocalOffsets(PreviewOffsets);
	TArray<FRotator> PreviewRotations;
	GetAISlotPreviewLocalRotations(PreviewRotations);
	TArray<FVector> EntryOffsets;
	GetAIEntryPreviewLocalOffsets(EntryOffsets);

	while (AISlotPreviewComponents.Num() < PreviewOffsets.Num())
	{
		const FName ComponentName = MakeUniqueObjectName(this, USphereComponent::StaticClass(), TEXT("EditorAISlotPreview"));
		USphereComponent* PreviewComponent = NewObject<USphereComponent>(this, ComponentName, RF_Transient | RF_TextExportTransient);
		if (!PreviewComponent)
		{
			break;
		}

		PreviewComponent->SetupAttachment(SceneRoot);
		PreviewComponent->SetIsVisualizationComponent(true);
		PreviewComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PreviewComponent->SetGenerateOverlapEvents(false);
		PreviewComponent->SetSphereRadius(20.0f);
		PreviewComponent->ShapeColor = FColor::Green;
		PreviewComponent->SetHiddenInGame(true);
		PreviewComponent->SetVisibility(true);
		PreviewComponent->RegisterComponent();
		AddInstanceComponent(PreviewComponent);
		AISlotPreviewComponents.Add(PreviewComponent);
	}

	while (AISlotPreviewArrowComponents.Num() < PreviewOffsets.Num())
	{
		const FName ComponentName = MakeUniqueObjectName(this, UArrowComponent::StaticClass(), TEXT("EditorAISlotArrow"));
		UArrowComponent* ArrowComponent = NewObject<UArrowComponent>(this, ComponentName, RF_Transient | RF_TextExportTransient);
		if (!ArrowComponent)
		{
			break;
		}

		ArrowComponent->SetupAttachment(SceneRoot);
		ArrowComponent->SetIsVisualizationComponent(true);
		ArrowComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ArrowComponent->SetHiddenInGame(true);
		ArrowComponent->SetVisibility(true);
		ArrowComponent->ArrowColor = FColor::Green;
		ArrowComponent->ArrowSize = 1.25f;
		ArrowComponent->ArrowLength = 75.0f;
		ArrowComponent->RegisterComponent();
		AddInstanceComponent(ArrowComponent);
		AISlotPreviewArrowComponents.Add(ArrowComponent);
	}

	while (AIEntryPreviewComponents.Num() < EntryOffsets.Num())
	{
		const FName ComponentName = MakeUniqueObjectName(this, USphereComponent::StaticClass(), TEXT("EditorAIEntryPreview"));
		USphereComponent* PreviewComponent = NewObject<USphereComponent>(this, ComponentName, RF_Transient | RF_TextExportTransient);
		if (!PreviewComponent)
		{
			break;
		}

		PreviewComponent->SetupAttachment(SceneRoot);
		PreviewComponent->SetIsVisualizationComponent(true);
		PreviewComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PreviewComponent->SetGenerateOverlapEvents(false);
		PreviewComponent->SetSphereRadius(28.0f);
		PreviewComponent->ShapeColor = FColor::Cyan;
		PreviewComponent->SetHiddenInGame(true);
		PreviewComponent->SetVisibility(true);
		PreviewComponent->RegisterComponent();
		AddInstanceComponent(PreviewComponent);
		AIEntryPreviewComponents.Add(PreviewComponent);
	}

	for (int32 Index = 0; Index < AISlotPreviewComponents.Num(); ++Index)
	{
		USphereComponent* PreviewComponent = AISlotPreviewComponents[Index];
		if (!PreviewComponent)
		{
			continue;
		}

		const bool bHasPreview = PreviewOffsets.IsValidIndex(Index);
		PreviewComponent->SetVisibility(bHasPreview);
		PreviewComponent->SetHiddenInGame(true);
		if (bHasPreview)
		{
			PreviewComponent->SetRelativeLocation(PreviewOffsets[Index]);
		}
	}

	for (int32 Index = 0; Index < AISlotPreviewArrowComponents.Num(); ++Index)
	{
		UArrowComponent* ArrowComponent = AISlotPreviewArrowComponents[Index];
		if (!ArrowComponent)
		{
			continue;
		}

		const bool bHasPreview = PreviewOffsets.IsValidIndex(Index);
		ArrowComponent->SetVisibility(bHasPreview);
		ArrowComponent->SetHiddenInGame(true);
		if (bHasPreview)
		{
			ArrowComponent->SetRelativeLocation(PreviewOffsets[Index]);
			ArrowComponent->SetRelativeRotation(PreviewRotations.IsValidIndex(Index) ? PreviewRotations[Index] : FRotator::ZeroRotator);
		}
	}

	for (int32 Index = 0; Index < AIEntryPreviewComponents.Num(); ++Index)
	{
		USphereComponent* PreviewComponent = AIEntryPreviewComponents[Index];
		if (!PreviewComponent)
		{
			continue;
		}

		const bool bHasPreview = EntryOffsets.IsValidIndex(Index);
		PreviewComponent->SetVisibility(bHasPreview);
		PreviewComponent->SetHiddenInGame(true);
		if (bHasPreview)
		{
			PreviewComponent->SetRelativeLocation(EntryOffsets[Index]);
		}
	}

	RefreshAIAnimationPreviewComponents(PreviewOffsets);
#endif
}

void ASectionalLayoutActor::ClearEditorPreviewComponents()
{
#if WITH_EDITOR
	TArray<UActorComponent*> Components;
	GetComponents(Components);
	for (UActorComponent* Component : Components)
	{
		if (!Component)
		{
			continue;
		}

		const FString ComponentName = Component->GetName();
		const bool bIsEditorPreview =
			ComponentName.StartsWith(TEXT("EditorAISlotPreview")) ||
			ComponentName.StartsWith(TEXT("EditorAISlotArrow")) ||
			ComponentName.StartsWith(TEXT("EditorAIEntryPreview")) ||
			ComponentName.StartsWith(TEXT("EditorAIAnimationPreview"));

		if (bIsEditorPreview)
		{
			RemoveInstanceComponent(Component);
			Component->DestroyComponent();
		}
	}

	AISlotPreviewComponents.Reset();
	AISlotPreviewArrowComponents.Reset();
	AIEntryPreviewComponents.Reset();
	AIAnimationPreviewComponents.Reset();
#endif
}

void ASectionalLayoutActor::RefreshAIAnimationPreviewComponents(const TArray<FVector>& PreviewOffsets)
{
#if WITH_EDITOR
	if (!bShowAIAnimationPreviewInEditor || !SceneRoot)
	{
		for (USkeletalMeshComponent* PreviewComponent : AIAnimationPreviewComponents)
		{
			if (PreviewComponent)
			{
				PreviewComponent->SetVisibility(false);
			}
		}
		return;
	}

	USkeletalMesh* PreviewMesh = AIAnimationPreviewMesh.LoadSynchronous();
	UAnimationAsset* DefaultAnimation = AIAnimationPreviewDefaultAnimation.LoadSynchronous();
	if (!PreviewMesh)
	{
		return;
	}

	while (AIAnimationPreviewComponents.Num() < PreviewOffsets.Num())
	{
		const FName ComponentName = MakeUniqueObjectName(this, USkeletalMeshComponent::StaticClass(), TEXT("EditorAIAnimationPreview"));
		USkeletalMeshComponent* PreviewComponent = NewObject<USkeletalMeshComponent>(this, ComponentName, RF_Transient | RF_TextExportTransient);
		if (!PreviewComponent)
		{
			break;
		}

		PreviewComponent->SetupAttachment(SceneRoot);
		PreviewComponent->SetIsVisualizationComponent(true);
		PreviewComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PreviewComponent->SetGenerateOverlapEvents(false);
		PreviewComponent->SetHiddenInGame(true);
		PreviewComponent->SetVisibility(true);
		PreviewComponent->RegisterComponent();
		AddInstanceComponent(PreviewComponent);
		AIAnimationPreviewComponents.Add(PreviewComponent);
	}

	for (int32 Index = 0; Index < AIAnimationPreviewComponents.Num(); ++Index)
	{
		USkeletalMeshComponent* PreviewComponent = AIAnimationPreviewComponents[Index];
		if (!PreviewComponent)
		{
			continue;
		}

		const bool bHasPreview = PreviewOffsets.IsValidIndex(Index);
		PreviewComponent->SetVisibility(bHasPreview);
		PreviewComponent->SetHiddenInGame(true);
		if (!bHasPreview)
		{
			continue;
		}

		UAnimationAsset* SlotAnimation = AIAnimationPreviewPerSlotAnimations.IsValidIndex(Index)
			? AIAnimationPreviewPerSlotAnimations[Index].LoadSynchronous()
			: nullptr;

		PreviewComponent->SetSkeletalMesh(PreviewMesh);
		PreviewComponent->SetRelativeLocation(PreviewOffsets[Index] + AIAnimationPreviewMeshOffsetCm);
		TArray<FRotator> PreviewRotations;
		GetAISlotPreviewLocalRotations(PreviewRotations);
		PreviewComponent->SetRelativeRotation((PreviewRotations.IsValidIndex(Index) ? PreviewRotations[Index] : FRotator::ZeroRotator) + AIAnimationPreviewMeshRotation);
		PreviewComponent->SetRelativeScale3D(AIAnimationPreviewMeshScale);
		if (SlotAnimation || DefaultAnimation)
		{
			PreviewComponent->PlayAnimation(SlotAnimation ? SlotAnimation : DefaultAnimation, true);
		}
	}
#endif
}
