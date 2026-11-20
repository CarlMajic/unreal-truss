#include "BarLayoutActor.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Modules/ModuleManager.h"

namespace
{
constexpr float BarLayoutFallbackExtentCm = 100.0f;

bool IsBarLayoutStaticMeshAsset(const FAssetData& AssetData)
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
}

ABarLayoutActor::ABarLayoutActor()
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
}

void ABarLayoutActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (bBuildOnConstruction)
	{
		RebuildBarLayout();
	}
}

#if WITH_EDITOR
void ABarLayoutActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RebuildBarLayout();
}
#endif

void ABarLayoutActor::RebuildBarLayout()
{
	WallBarCount = FMath::Max(1, WallBarCount);
	ClearGenerated();

	const FString ResolvedFrontBarItem = bUseFrontBar ? ResolveSelectedItem(FrontBarAssetFolder, FrontBarItem) : FString();
	const FString ResolvedBackBarItem = bUseBackBar ? ResolveSelectedItem(BackBarAssetFolder, BackBarItem) : FString();

	if (bUseFrontBar && FrontBarItem.IsEmpty())
	{
		FrontBarItem = ResolvedFrontBarItem;
	}
	if (bUseBackBar && BackBarItem.IsEmpty())
	{
		BackBarItem = ResolvedBackBarItem;
	}

	if (LayoutShape == EBarLayoutShape::WallLine)
	{
		const float FirstOffsetX = bCenterWallLineOnActor ? -0.5f * static_cast<float>(WallBarCount - 1) * WallBarSpacingCm : 0.0f;
		for (int32 BarIndex = 0; BarIndex < WallBarCount; ++BarIndex)
		{
			const FVector LineLocation(FirstOffsetX + (static_cast<float>(BarIndex) * WallBarSpacingCm), 0.0f, 0.0f);
			AddBarSet(ResolvedFrontBarItem, ResolvedBackBarItem, FTransform(FRotator::ZeroRotator, LineLocation, FVector::OneVector));
		}
	}
	else if (const int32 SideCount = GetCenterSideCount(); SideCount > 0)
	{
		const float YawStep = 360.0f / static_cast<float>(SideCount);
		const float CenterOffsetCm = GetCenterOffsetCm();
		for (int32 SideIndex = 0; SideIndex < SideCount; ++SideIndex)
		{
			const FRotator SideRotation(0.0f, YawStep * SideIndex, 0.0f);
			const FVector SideOffset = SideRotation.RotateVector(FVector(0.0f, CenterOffsetCm, 0.0f));
			AddBarSet(ResolvedFrontBarItem, ResolvedBackBarItem, FTransform(SideRotation, SideOffset, FVector::OneVector));
		}
	}
	else
	{
		AddBarSet(ResolvedFrontBarItem, ResolvedBackBarItem, FTransform::Identity);
	}

	UpdateSelectionBounds();
}

void ABarLayoutActor::ApplyBuildDefinition(const FBarLayoutBuildDefinition& Definition, bool bRebuildNow)
{
	FrontBarItem = Definition.FrontBarItem;
	BackBarItem = Definition.BackBarItem;
	bUseFrontBar = Definition.bUseFrontBar;
	bUseBackBar = Definition.bUseBackBar;
	LayoutShape = Definition.LayoutShape;
	WallBarCount = FMath::Max(1, Definition.WallBarCount);
	WallBarSpacingCm = Definition.WallBarSpacingCm;
	CenterTwoSidedOffsetCm = Definition.CenterTwoSidedOffsetCm;
	CenterFourSidedOffsetCm = Definition.CenterFourSidedOffsetCm;
	CenterSixSidedOffsetCm = Definition.CenterSixSidedOffsetCm;

	if (bRebuildNow)
	{
		RebuildBarLayout();
	}
}

FBarLayoutBuildDefinition ABarLayoutActor::GetBuildDefinition() const
{
	FBarLayoutBuildDefinition Definition;
	Definition.FrontBarItem = FrontBarItem;
	Definition.BackBarItem = BackBarItem;
	Definition.bUseFrontBar = bUseFrontBar;
	Definition.bUseBackBar = bUseBackBar;
	Definition.LayoutShape = LayoutShape;
	Definition.WallBarCount = WallBarCount;
	Definition.WallBarSpacingCm = WallBarSpacingCm;
	Definition.CenterTwoSidedOffsetCm = CenterTwoSidedOffsetCm;
	Definition.CenterFourSidedOffsetCm = CenterFourSidedOffsetCm;
	Definition.CenterSixSidedOffsetCm = CenterSixSidedOffsetCm;
	return Definition;
}

void ABarLayoutActor::SetSelectionHighlighted(bool bHighlighted)
{
	if (SelectionBounds)
	{
		SelectionBounds->SetHiddenInGame(!bHighlighted);
	}
}

TArray<FString> ABarLayoutActor::GetFrontBarOptions() const
{
	return GetFurnitureItemOptions(FrontBarAssetFolder);
}

TArray<FString> ABarLayoutActor::GetBackBarOptions() const
{
	return GetFurnitureItemOptions(BackBarAssetFolder);
}

void ABarLayoutActor::ClearGenerated()
{
	GeneratedBounds = FBox(EForceInit::ForceInit);
	CurrentGeneratedMeshCount = 0;

	TInlineComponentArray<UInstancedStaticMeshComponent*> ExistingInstanceComponents(this);
	for (UInstancedStaticMeshComponent* MeshComponent : ExistingInstanceComponents)
	{
		if (MeshComponent && MeshComponent->GetName().StartsWith(TEXT("BarLayout_")) && !GeneratedMeshComponents.Contains(MeshComponent))
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

void ABarLayoutActor::AddBarSet(const FString& ResolvedFrontBarItem, const FString& ResolvedBackBarItem, const FTransform& LayoutTransform)
{
	if (bUseFrontBar && !ResolvedFrontBarItem.IsEmpty())
	{
		const FTransform FrontTransform(FrontBarRotation, FVector::ZeroVector, FrontBarScale);
		AddFurnitureItem(FrontBarAssetFolder, ResolvedFrontBarItem, TEXT("FrontBar"), FrontTransform * LayoutTransform);
	}

	if (bUseBackBar && !ResolvedBackBarItem.IsEmpty())
	{
		const FTransform BackTransform(BackBarRotation, FVector::ZeroVector, BackBarScale);
		AddFurnitureItem(BackBarAssetFolder, ResolvedBackBarItem, TEXT("BackBar"), BackTransform * LayoutTransform);
	}
}

void ABarLayoutActor::AddFurnitureItem(const FString& CategoryFolder, const FString& ItemName, const TCHAR* Prefix, const FTransform& ItemTransform)
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

UInstancedStaticMeshComponent* ABarLayoutActor::FindOrCreateMeshBucket(const FSoftObjectPath& MeshPath, const TCHAR* Prefix)
{
	const FString SafeAssetName = MakeComponentSafeName(MeshPath.GetAssetName());
	const FName ComponentName(*FString::Printf(TEXT("BarLayout_%s_%s"), Prefix, *SafeAssetName));
	if (UInstancedStaticMeshComponent* ExistingComponent = FindObjectFast<UInstancedStaticMeshComponent>(this, ComponentName))
	{
		GeneratedMeshComponents.AddUnique(ExistingComponent);
		return ExistingComponent;
	}

	UInstancedStaticMeshComponent* MeshComponent = NewObject<UInstancedStaticMeshComponent>(this, ComponentName);
	if (!MeshComponent)
	{
		return nullptr;
	}

	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->bDisallowNanite = true;
	MeshComponent->RegisterComponent();
	AddInstanceComponent(MeshComponent);
	GeneratedMeshComponents.Add(MeshComponent);
	return MeshComponent;
}

TArray<FString> ABarLayoutActor::GetFurnitureItemOptions(const FString& CategoryFolder) const
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
		if (!IsBarLayoutStaticMeshAsset(AssetData))
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

TArray<FSoftObjectPath> ABarLayoutActor::GetMeshPathsForFurnitureItem(const FString& CategoryFolder, const FString& ItemName) const
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
		if (IsBarLayoutStaticMeshAsset(AssetData))
		{
			MeshPaths.Add(AssetData.ToSoftObjectPath());
		}
	}

	return MeshPaths;
}

FString ABarLayoutActor::ResolveSelectedItem(const FString& CategoryFolder, const FString& SelectedItem) const
{
	const TArray<FString> Options = GetFurnitureItemOptions(CategoryFolder);
	if (Options.Contains(SelectedItem))
	{
		return SelectedItem;
	}

	return Options.Num() > 0 ? Options[0] : FString();
}

int32 ABarLayoutActor::GetCenterSideCount() const
{
	switch (LayoutShape)
	{
	case EBarLayoutShape::CenterTwoSided:
		return 2;
	case EBarLayoutShape::CenterFourSided:
		return 4;
	case EBarLayoutShape::CenterSixSided:
		return 6;
	default:
		return 0;
	}
}

float ABarLayoutActor::GetCenterOffsetCm() const
{
	switch (LayoutShape)
	{
	case EBarLayoutShape::CenterTwoSided:
		return CenterTwoSidedOffsetCm;
	case EBarLayoutShape::CenterFourSided:
		return CenterFourSidedOffsetCm;
	case EBarLayoutShape::CenterSixSided:
		return CenterSixSidedOffsetCm;
	default:
		return 0.0f;
	}
}

void ABarLayoutActor::ExpandGeneratedBounds(UStaticMesh* Mesh, const FTransform& Transform)
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
		Location - FVector(BarLayoutFallbackExtentCm),
		Location + FVector(BarLayoutFallbackExtentCm));
}

void ABarLayoutActor::UpdateSelectionBounds()
{
	if (!SelectionBounds)
	{
		return;
	}

	SelectionBounds->SetWorldScale3D(FVector::OneVector);

	if (!GeneratedBounds.IsValid)
	{
		SelectionBounds->SetRelativeLocation(FVector::ZeroVector);
		SelectionBounds->SetBoxExtent(FVector(BarLayoutFallbackExtentCm));
		return;
	}

	SelectionBounds->SetRelativeLocation(GeneratedBounds.GetCenter());
	SelectionBounds->SetBoxExtent(GeneratedBounds.GetExtent().ComponentMax(FVector(50.0f, 50.0f, 50.0f)));
}
