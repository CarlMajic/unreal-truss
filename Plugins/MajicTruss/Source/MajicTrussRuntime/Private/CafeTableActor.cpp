#include "CafeTableActor.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Animation/AnimationAsset.h"
#include "Components/SphereComponent.h"
#include "Engine/StaticMesh.h"
#include "Modules/ModuleManager.h"

namespace
{
constexpr float CafeTableFallbackExtentCm = 75.0f;
constexpr float DuplicatePreviewSlotDistanceCm = 75.0f;

bool IsCafeTableStaticMeshAsset(const FAssetData& AssetData)
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

void ConfigureGeneratedFurnitureComponent(UInstancedStaticMeshComponent* MeshComponent, const TCHAR* Prefix)
{
	if (!MeshComponent)
	{
		return;
	}

	const bool bBlocksNavigationAndPawn = FCString::Strcmp(Prefix, TEXT("CafeTable")) == 0;
	MeshComponent->SetCollisionEnabled(bBlocksNavigationAndPawn ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	MeshComponent->SetCollisionObjectType(ECC_WorldStatic);
	MeshComponent->SetCollisionResponseToAllChannels(bBlocksNavigationAndPawn ? ECR_Block : ECR_Ignore);
	MeshComponent->SetGenerateOverlapEvents(false);
	MeshComponent->SetCanEverAffectNavigation(bBlocksNavigationAndPawn);
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

ACafeTableActor::ACafeTableActor()
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

	AIAnimationPreviewDefaultAnimation = TSoftObjectPtr<UAnimationAsset>(FSoftObjectPath(TEXT("/Game/IdleSitPoseAnimations/Animations/ArmsOnKnees_Sit.ArmsOnKnees_Sit")));
}

void ACafeTableActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (bBuildOnConstruction)
	{
		RebuildCafeTable();
	}
}

#if WITH_EDITOR
void ACafeTableActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RebuildCafeTable();
}
#endif

void ACafeTableActor::RebuildCafeTable()
{
	RectangleChairPairOffsetXCm = FMath::Max(0.0f, RectangleChairPairOffsetXCm);
	AISlotSourceLocalOffsets.Reset();
	ClearGenerated();

	const FString ResolvedCafeTableItem = ResolveSelectedItem(CafeTableAssetFolder, CafeTableItem);
	const FString ResolvedDiningChairItem = ResolveSelectedItem(DiningChairAssetFolder, DiningChairItem);

	if (CafeTableItem.IsEmpty())
	{
		CafeTableItem = ResolvedCafeTableItem;
	}
	if (DiningChairItem.IsEmpty())
	{
		DiningChairItem = ResolvedDiningChairItem;
	}

	if (!ResolvedCafeTableItem.IsEmpty())
	{
		AddFurnitureItem(
			CafeTableAssetFolder,
			ResolvedCafeTableItem,
			TEXT("CafeTable"),
			FTransform(CafeTableRotation, FVector::ZeroVector, CafeTableScale));
	}

	if (!ResolvedDiningChairItem.IsEmpty())
	{
		AddChairLayout(ResolvedDiningChairItem);
	}

	UpdateSelectionBounds();
	RefreshAISlotPreviewComponents();
}

void ACafeTableActor::ApplyBuildDefinition(const FCafeTableBuildDefinition& Definition, bool bRebuildNow)
{
	CafeTableItem = Definition.CafeTableItem;
	DiningChairItem = Definition.DiningChairItem;
	ChairLayout = Definition.ChairLayout;
	bClassroomStyle = Definition.bClassroomStyle;
	ChairOffsetYCm = Definition.ChairOffsetYCm;
	RectangleChairPairOffsetXCm = FMath::Max(0.0f, Definition.RectangleChairPairOffsetXCm);

	if (bRebuildNow)
	{
		RebuildCafeTable();
	}
}

FCafeTableBuildDefinition ACafeTableActor::GetBuildDefinition() const
{
	FCafeTableBuildDefinition Definition;
	Definition.CafeTableItem = CafeTableItem;
	Definition.DiningChairItem = DiningChairItem;
	Definition.ChairLayout = ChairLayout;
	Definition.bClassroomStyle = bClassroomStyle;
	Definition.ChairOffsetYCm = ChairOffsetYCm;
	Definition.RectangleChairPairOffsetXCm = RectangleChairPairOffsetXCm;
	return Definition;
}

void ACafeTableActor::SetSelectionHighlighted(bool bHighlighted)
{
	if (SelectionBounds)
	{
		SelectionBounds->SetHiddenInGame(!bHighlighted);
	}
}

void ACafeTableActor::GetAISlotPreviewLocalOffsets(TArray<FVector>& OutOffsets) const
{
	OutOffsets.Reset();

	FVector OffsetSum = FVector::ZeroVector;
	for (int32 SourceIndex = 0; SourceIndex < AISlotSourceLocalOffsets.Num(); ++SourceIndex)
	{
		const FVector PerSlotOffset = AISlotPerSlotOffsetsCm.IsValidIndex(SourceIndex)
			? AISlotPerSlotOffsetsCm[SourceIndex]
			: FVector::ZeroVector;
		const FVector AdjustedOffset = AISlotSourceLocalOffsets[SourceIndex] + AISlotPreviewOffsetCm + PerSlotOffset;
		OffsetSum += AdjustedOffset;
		bool bDuplicate = false;
		for (const FVector& ExistingOffset : OutOffsets)
		{
			if (FVector::DistSquared2D(ExistingOffset, AdjustedOffset) <= FMath::Square(DuplicatePreviewSlotDistanceCm))
			{
				bDuplicate = true;
				break;
			}
		}
		if (!bDuplicate)
		{
			OutOffsets.Add(AdjustedOffset);
		}
	}

	if (AISlotSourceLocalOffsets.Num() > OutOffsets.Num())
	{
		const int32 DesiredCount = AISlotSourceLocalOffsets.Num();
		const FVector CenterOffset = DesiredCount > 0 ? OffsetSum / static_cast<float>(DesiredCount) : FVector::ZeroVector;
		OutOffsets.Reset();
		const float AngleStep = 360.0f / FMath::Max(DesiredCount, 1);
		for (int32 Index = 0; Index < DesiredCount; ++Index)
		{
			const float Angle = FMath::DegreesToRadians(AISlotPreviewStartAngleDegrees + AngleStep * Index);
			const FVector PerSlotOffset = AISlotPerSlotOffsetsCm.IsValidIndex(Index)
				? AISlotPerSlotOffsetsCm[Index]
				: FVector::ZeroVector;
			OutOffsets.Add(CenterOffset + FVector(FMath::Cos(Angle) * AISlotPreviewSpreadRadiusCm, FMath::Sin(Angle) * AISlotPreviewSpreadRadiusCm, 0.0f) + PerSlotOffset);
		}
	}
}

void ACafeTableActor::GetAISlotPreviewAnimations(TArray<TSoftObjectPtr<UAnimationAsset>>& OutAnimations) const
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

void ACafeTableActor::GetAISlotPreviewLocalRotations(TArray<FRotator>& OutRotations) const
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

TArray<FString> ACafeTableActor::GetCafeTableOptions() const
{
	return GetFurnitureItemOptions(CafeTableAssetFolder);
}

TArray<FString> ACafeTableActor::GetDiningChairOptions() const
{
	return GetFurnitureItemOptions(DiningChairAssetFolder);
}

void ACafeTableActor::ClearGenerated()
{
	GeneratedBounds = FBox(EForceInit::ForceInit);
	CurrentGeneratedMeshCount = 0;

	TInlineComponentArray<UInstancedStaticMeshComponent*> ExistingInstanceComponents(this);
	for (UInstancedStaticMeshComponent* MeshComponent : ExistingInstanceComponents)
	{
		if (MeshComponent && MeshComponent->GetName().StartsWith(TEXT("CafeTable_")) && !GeneratedMeshComponents.Contains(MeshComponent))
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

void ACafeTableActor::AddFurnitureItem(const FString& CategoryFolder, const FString& ItemName, const TCHAR* Prefix, const FTransform& ItemTransform)
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

void ACafeTableActor::AddChairLayout(const FString& ResolvedChairItem)
{
	if (ChairLayout == ECafeTableChairLayout::NoChairs)
	{
		return;
	}

	if (ChairLayout == ECafeTableChairLayout::TwoTop)
	{
		AddChairPair(ResolvedChairItem, 0.0f);
		return;
	}

	if (ChairLayout == ECafeTableChairLayout::FourTop)
	{
		const FVector BaseOffset(0.0f, -ChairOffsetYCm, 0.0f);
		const int32 ChairCount = bClassroomStyle ? 1 : 4;
		for (int32 ChairIndex = 0; ChairIndex < ChairCount; ++ChairIndex)
		{
			const float YawDegrees = 90.0f * static_cast<float>(ChairIndex);
			const FRotator OrbitRotation(0.0f, YawDegrees, 0.0f);
			FRotator InstanceRotation = DiningChairRotation;
			InstanceRotation.Yaw += YawDegrees;

			AddFurnitureItem(
				DiningChairAssetFolder,
				ResolvedChairItem,
				TEXT("DiningChair"),
				FTransform(InstanceRotation, OrbitRotation.RotateVector(BaseOffset), DiningChairScale));
			AddAISlotSource(OrbitRotation.RotateVector(BaseOffset));
		}
		return;
	}

	if (ChairLayout == ECafeTableChairLayout::RectangleFourTop)
	{
		AddChairPair(ResolvedChairItem, 0.0f);
		AddChairPair(ResolvedChairItem, RectangleChairPairOffsetXCm);
	}
	else if (ChairLayout == ECafeTableChairLayout::RectangleSixTop)
	{
		AddChairPair(ResolvedChairItem, 0.0f);
		AddChairPair(ResolvedChairItem, RectangleChairPairOffsetXCm);
		AddChairPair(ResolvedChairItem, RectangleChairPairOffsetXCm * 2.0f);
	}
}

void ACafeTableActor::AddChairPair(const FString& ResolvedChairItem, float XOffsetCm)
{
	const int32 ChairCount = bClassroomStyle ? 1 : 2;

	for (int32 ChairIndex = 0; ChairIndex < ChairCount; ++ChairIndex)
	{
		const float YawDegrees = 180.0f * static_cast<float>(ChairIndex);
		const float YOffsetCm = ChairIndex == 0 ? -ChairOffsetYCm : ChairOffsetYCm;
		FRotator InstanceRotation = DiningChairRotation;
		InstanceRotation.Yaw += YawDegrees;

		AddFurnitureItem(
			DiningChairAssetFolder,
			ResolvedChairItem,
			TEXT("DiningChair"),
			FTransform(InstanceRotation, FVector(XOffsetCm, YOffsetCm, 0.0f), DiningChairScale));
		AddAISlotSource(FVector(XOffsetCm, YOffsetCm, 0.0f));
	}
}

UInstancedStaticMeshComponent* ACafeTableActor::FindOrCreateMeshBucket(const FSoftObjectPath& MeshPath, const TCHAR* Prefix)
{
	const FString SafeAssetName = MakeComponentSafeName(MeshPath.GetAssetName());
	const FName ComponentName(*FString::Printf(TEXT("CafeTable_%s_%s"), Prefix, *SafeAssetName));
	if (UInstancedStaticMeshComponent* ExistingComponent = FindObjectFast<UInstancedStaticMeshComponent>(this, ComponentName))
	{
		ConfigureGeneratedFurnitureComponent(ExistingComponent, Prefix);
		GeneratedMeshComponents.AddUnique(ExistingComponent);
		return ExistingComponent;
	}

	UInstancedStaticMeshComponent* MeshComponent = NewObject<UInstancedStaticMeshComponent>(this, ComponentName);
	if (!MeshComponent)
	{
		return nullptr;
	}

	MeshComponent->SetupAttachment(SceneRoot);
	ConfigureGeneratedFurnitureComponent(MeshComponent, Prefix);
	MeshComponent->RegisterComponent();
	AddInstanceComponent(MeshComponent);
	GeneratedMeshComponents.Add(MeshComponent);
	return MeshComponent;
}

TArray<FString> ACafeTableActor::GetFurnitureItemOptions(const FString& CategoryFolder) const
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
		if (!IsCafeTableStaticMeshAsset(AssetData))
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
		Options.Add(ItemName);
	}
	Options.Sort();
	return Options;
}

TArray<FSoftObjectPath> ACafeTableActor::GetMeshPathsForFurnitureItem(const FString& CategoryFolder, const FString& ItemName) const
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
		if (IsCafeTableStaticMeshAsset(AssetData))
		{
			MeshPaths.Add(AssetData.ToSoftObjectPath());
		}
	}

	return MeshPaths;
}

FString ACafeTableActor::ResolveSelectedItem(const FString& CategoryFolder, const FString& SelectedItem) const
{
	const TArray<FString> Options = GetFurnitureItemOptions(CategoryFolder);
	if (Options.Contains(SelectedItem))
	{
		return SelectedItem;
	}

	return Options.Num() > 0 ? Options[0] : FString();
}

void ACafeTableActor::ExpandGeneratedBounds(UStaticMesh* Mesh, const FTransform& Transform)
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
		Location - FVector(CafeTableFallbackExtentCm),
		Location + FVector(CafeTableFallbackExtentCm));
}

void ACafeTableActor::UpdateSelectionBounds()
{
	if (!SelectionBounds)
	{
		return;
	}

	SelectionBounds->SetWorldScale3D(FVector::OneVector);

	if (!GeneratedBounds.IsValid)
	{
		SelectionBounds->SetRelativeLocation(FVector::ZeroVector);
		SelectionBounds->SetBoxExtent(FVector(CafeTableFallbackExtentCm));
		return;
	}

	SelectionBounds->SetRelativeLocation(GeneratedBounds.GetCenter());
	SelectionBounds->SetBoxExtent(GeneratedBounds.GetExtent().ComponentMax(FVector(50.0f, 50.0f, 50.0f)));
}

void ACafeTableActor::AddAISlotSource(const FVector& LocalOffset)
{
	AISlotSourceLocalOffsets.Add(LocalOffset);
}

void ACafeTableActor::RefreshAISlotPreviewComponents()
{
#if WITH_EDITOR
	if (!bShowAISlotPreviewInEditor || !SceneRoot)
	{
		for (USphereComponent* PreviewComponent : AISlotPreviewComponents)
		{
			if (PreviewComponent)
			{
				PreviewComponent->SetVisibility(false);
			}
		}
		for (UArrowComponent* ArrowComponent : AISlotPreviewArrowComponents)
		{
			if (ArrowComponent)
			{
				ArrowComponent->SetVisibility(false);
			}
		}
		return;
	}

	TArray<FVector> PreviewOffsets;
	GetAISlotPreviewLocalOffsets(PreviewOffsets);
	TArray<FRotator> PreviewRotations;
	GetAISlotPreviewLocalRotations(PreviewRotations);

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

#endif
}
