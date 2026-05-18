#include "LoungeLayoutActor.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Modules/ModuleManager.h"

namespace
{
constexpr float LoungeFallbackExtentCm = 75.0f;

bool IsLoungeStaticMeshAsset(const FAssetData& AssetData)
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

ALoungeLayoutActor::ALoungeLayoutActor()
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

void ALoungeLayoutActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (bBuildOnConstruction)
	{
		RebuildLoungeLayout();
	}
}

#if WITH_EDITOR
void ALoungeLayoutActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RebuildLoungeLayout();
}
#endif

void ALoungeLayoutActor::RebuildLoungeLayout()
{
	SofaCount = FMath::Max(0, SofaCount);
	ChairCount = FMath::Max(0, ChairCount);
	EndTableCount = FMath::Max(0, EndTableCount);
	LampCount = FMath::Max(0, LampCount);
	AccentCount = FMath::Max(0, AccentCount);
	SofaSpacingCm = FMath::Max(0.0f, SofaSpacingCm);
	ChairSpacingCm = FMath::Max(0.0f, ChairSpacingCm);

	ClearGenerated();

	const FString ResolvedSofaItem = ResolveSelectedItem(SofaAssetFolder, SofaItem);
	const FString ResolvedChairItem = ResolveSelectedItem(ChairAssetFolder, ChairItem);
	const FString ResolvedTableItem = ResolveSelectedItem(CocktailTableAssetFolder, CocktailTableItem);
	const FString ResolvedEndTableItem = ResolveSelectedItem(EndTableAssetFolder, EndTableItem);
	const FString ResolvedLampItem = ResolveSelectedItem(LampAssetFolder, LampItem);
	const FString ResolvedAccentItem = ResolveSelectedItem(AccentAssetFolder, AccentItem);

	if (SofaItem.IsEmpty())
	{
		SofaItem = ResolvedSofaItem;
	}
	if (ChairItem.IsEmpty())
	{
		ChairItem = ResolvedChairItem;
	}
	if (CocktailTableItem.IsEmpty())
	{
		CocktailTableItem = ResolvedTableItem;
	}
	if (EndTableItem.IsEmpty())
	{
		EndTableItem = ResolvedEndTableItem;
	}
	if (LampItem.IsEmpty())
	{
		LampItem = ResolvedLampItem;
	}
	if (AccentItem.IsEmpty())
	{
		AccentItem = ResolvedAccentItem;
	}

	if (!ResolvedTableItem.IsEmpty())
	{
		AddFurnitureItem(
			CocktailTableAssetFolder,
			ResolvedTableItem,
			TEXT("LoungeTable"),
			FTransform(CocktailTableRotation, FVector::ZeroVector, CocktailTableScale));
	}

	const bool bFourChairLayout = bUseFourChairs;
	const bool bTwoSofaLayout = bUseTwoSofas && !bFourChairLayout;
	const int32 EffectiveSofaCount = bTwoSofaLayout ? 2 : (bFourChairLayout ? 0 : SofaCount);
	const int32 EffectiveChairCount = bFourChairLayout ? 4 : (bTwoSofaLayout ? 0 : ChairCount);
	const ELoungeChairMirrorMode EffectiveSofaMirrorMode = bTwoSofaLayout ? ELoungeChairMirrorMode::MirrorEveryOtherChairAcrossY : ELoungeChairMirrorMode::None;
	const ELoungeChairMirrorMode EffectiveChairMirrorMode = bFourChairLayout ? ELoungeChairMirrorMode::MirrorEveryOtherChairAcrossXAndY : ChairMirrorMode;

	for (const FLoungeMirroredTransform& SofaTransform : GetMirroredTransforms(
		GetRowLocation(0, EffectiveSofaCount, SofaSpacingCm, SofaDistanceNorthCm),
		SofaRotation,
		SofaScale,
		EffectiveSofaMirrorMode,
		EffectiveSofaCount,
		false))
	{
		if (ResolvedSofaItem.IsEmpty())
		{
			break;
		}

		AddFurnitureItem(
			SofaAssetFolder,
			ResolvedSofaItem,
			TEXT("LoungeSofa"),
			SofaTransform.Transform,
			SofaTransform.AppliedMirrorMode);
	}

	for (const FLoungeMirroredTransform& ChairTransform : GetMirroredTransforms(
		GetRowLocation(0, EffectiveChairCount, ChairSpacingCm, -ChairDistanceSouthCm),
		ChairRotation,
		ChairScale,
		EffectiveChairMirrorMode,
		EffectiveChairCount,
		false,
		MirroredChairYawOffsetDegrees))
	{
		if (ResolvedChairItem.IsEmpty())
		{
			break;
		}

		AddFurnitureItem(
			ChairAssetFolder,
			ResolvedChairItem,
			TEXT("LoungeChair"),
			ChairTransform.Transform,
			ChairTransform.AppliedMirrorMode);
	}

	for (const FLoungeMirroredTransform& EndTableTransform : GetMirroredTransforms(
		FVector(EndTableOffsetXCm, 0.0f, 0.0f),
		EndTableRotation,
		EndTableScale,
		EndTableMirrorMode,
		EndTableCount,
		false))
	{
		if (ResolvedEndTableItem.IsEmpty())
		{
			break;
		}

		AddFurnitureItem(
			EndTableAssetFolder,
			ResolvedEndTableItem,
			TEXT("LoungeEndTable"),
			EndTableTransform.Transform,
			EndTableTransform.AppliedMirrorMode);
	}

	for (const FLoungeMirroredTransform& LampTransform : GetMirroredTransforms(
		FVector::ZeroVector,
		LampRotation,
		LampScale,
		LampMirrorMode,
		LampCount,
		false))
	{
		if (ResolvedLampItem.IsEmpty())
		{
			break;
		}

		AddFurnitureItem(
			LampAssetFolder,
			ResolvedLampItem,
			TEXT("LoungeLamp"),
			LampTransform.Transform,
			LampTransform.AppliedMirrorMode);
	}

	for (const FLoungeMirroredTransform& AccentTransform : GetMirroredTransforms(
		FVector::ZeroVector,
		AccentRotation,
		AccentScale,
		AccentMirrorMode,
		AccentCount,
		true))
	{
		if (ResolvedAccentItem.IsEmpty())
		{
			break;
		}

		AddFurnitureItem(
			AccentAssetFolder,
			ResolvedAccentItem,
			TEXT("LoungeAccent"),
			AccentTransform.Transform,
			AccentTransform.AppliedMirrorMode);
	}

	UpdateSelectionBounds();
}

void ALoungeLayoutActor::ApplyBuildDefinition(const FLoungeLayoutBuildDefinition& Definition, bool bRebuildNow)
{
	SofaItem = Definition.SofaItem;
	ChairItem = Definition.ChairItem;
	CocktailTableItem = Definition.CocktailTableItem;
	EndTableItem = Definition.EndTableItem;
	LampItem = Definition.LampItem;
	AccentItem = Definition.AccentItem;
	SofaCount = FMath::Max(0, Definition.SofaCount);
	ChairCount = FMath::Max(0, Definition.ChairCount);
	EndTableCount = FMath::Max(0, Definition.EndTableCount);
	LampCount = FMath::Max(0, Definition.LampCount);
	AccentCount = FMath::Max(0, Definition.AccentCount);
	bUseTwoSofas = Definition.bUseTwoSofas;
	bUseFourChairs = Definition.bUseFourChairs;
	EndTableOffsetXCm = Definition.EndTableOffsetXCm;
	ChairMirrorMode = Definition.ChairMirrorMode;
	EndTableMirrorMode = Definition.EndTableMirrorMode;
	LampMirrorMode = Definition.LampMirrorMode;
	AccentMirrorMode = Definition.AccentMirrorMode;
	MirroredChairYawOffsetDegrees = Definition.MirroredChairYawOffsetDegrees;

	if (bRebuildNow)
	{
		RebuildLoungeLayout();
	}
}

FLoungeLayoutBuildDefinition ALoungeLayoutActor::GetBuildDefinition() const
{
	FLoungeLayoutBuildDefinition Definition;
	Definition.SofaItem = SofaItem;
	Definition.ChairItem = ChairItem;
	Definition.CocktailTableItem = CocktailTableItem;
	Definition.EndTableItem = EndTableItem;
	Definition.LampItem = LampItem;
	Definition.AccentItem = AccentItem;
	Definition.SofaCount = SofaCount;
	Definition.ChairCount = ChairCount;
	Definition.EndTableCount = EndTableCount;
	Definition.LampCount = LampCount;
	Definition.AccentCount = AccentCount;
	Definition.bUseTwoSofas = bUseTwoSofas;
	Definition.bUseFourChairs = bUseFourChairs;
	Definition.EndTableOffsetXCm = EndTableOffsetXCm;
	Definition.ChairMirrorMode = ChairMirrorMode;
	Definition.EndTableMirrorMode = EndTableMirrorMode;
	Definition.LampMirrorMode = LampMirrorMode;
	Definition.AccentMirrorMode = AccentMirrorMode;
	Definition.MirroredChairYawOffsetDegrees = MirroredChairYawOffsetDegrees;
	return Definition;
}

void ALoungeLayoutActor::SetSelectionHighlighted(bool bHighlighted)
{
	if (SelectionBounds)
	{
		SelectionBounds->SetHiddenInGame(!bHighlighted);
	}
}

TArray<FString> ALoungeLayoutActor::GetSofaOptions() const
{
	return GetFurnitureItemOptions(SofaAssetFolder);
}

TArray<FString> ALoungeLayoutActor::GetChairOptions() const
{
	return GetFurnitureItemOptions(ChairAssetFolder);
}

TArray<FString> ALoungeLayoutActor::GetCocktailTableOptions() const
{
	return GetFurnitureItemOptions(CocktailTableAssetFolder);
}

TArray<FString> ALoungeLayoutActor::GetEndTableOptions() const
{
	return GetFurnitureItemOptions(EndTableAssetFolder);
}

TArray<FString> ALoungeLayoutActor::GetLampOptions() const
{
	return GetFurnitureItemOptions(LampAssetFolder);
}

TArray<FString> ALoungeLayoutActor::GetAccentOptions() const
{
	return GetFurnitureItemOptions(AccentAssetFolder);
}

void ALoungeLayoutActor::ClearGenerated()
{
	GeneratedBounds = FBox(EForceInit::ForceInit);
	CurrentGeneratedMeshCount = 0;

	TInlineComponentArray<UInstancedStaticMeshComponent*> ExistingInstanceComponents(this);
	for (UInstancedStaticMeshComponent* MeshComponent : ExistingInstanceComponents)
	{
		if (MeshComponent && MeshComponent->GetName().StartsWith(TEXT("Lounge_")) && !GeneratedMeshComponents.Contains(MeshComponent))
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

void ALoungeLayoutActor::AddFurnitureItem(const FString& CategoryFolder, const FString& ItemName, const TCHAR* Prefix, const FTransform& ItemTransform, ELoungeChairMirrorMode AppliedMirrorMode)
{
	const FVector TransformScale = ItemTransform.GetScale3D();
	const bool bReverseCulling = TransformScale.X * TransformScale.Y * TransformScale.Z < 0.0f;

	for (const FSoftObjectPath& MeshPath : GetMeshPathsForFurnitureItem(CategoryFolder, ItemName))
	{
		UStaticMesh* Mesh = Cast<UStaticMesh>(MeshPath.TryLoad());
		if (!Mesh)
		{
			continue;
		}

		if (UInstancedStaticMeshComponent* MeshComponent = FindOrCreateMeshBucket(MeshPath, Prefix, bReverseCulling))
		{
			AddMeshInstance(MeshComponent, Mesh, ItemTransform, AppliedMirrorMode);
		}
	}
}

UInstancedStaticMeshComponent* ALoungeLayoutActor::FindOrCreateMeshBucket(const FSoftObjectPath& MeshPath, const TCHAR* Prefix, bool bReverseCulling)
{
	const FString SafeAssetName = MakeComponentSafeName(MeshPath.GetAssetName());
	const FName ComponentName(*FString::Printf(TEXT("Lounge_%s_%s%s"), Prefix, *SafeAssetName, bReverseCulling ? TEXT("_ReverseCull") : TEXT("")));
	if (UInstancedStaticMeshComponent* ExistingComponent = FindObjectFast<UInstancedStaticMeshComponent>(this, ComponentName))
	{
		GeneratedMeshComponents.AddUnique(ExistingComponent);
		ExistingComponent->SetReverseCulling(bReverseCulling);
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
	MeshComponent->SetReverseCulling(bReverseCulling);
	MeshComponent->RegisterComponent();
	AddInstanceComponent(MeshComponent);
	GeneratedMeshComponents.Add(MeshComponent);
	return MeshComponent;
}

TArray<FString> ALoungeLayoutActor::GetFurnitureItemOptions(const FString& CategoryFolder) const
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
		if (!IsLoungeStaticMeshAsset(AssetData))
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

TArray<FSoftObjectPath> ALoungeLayoutActor::GetMeshPathsForFurnitureItem(const FString& CategoryFolder, const FString& ItemName) const
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
		if (IsLoungeStaticMeshAsset(AssetData))
		{
			MeshPaths.Add(AssetData.ToSoftObjectPath());
		}
	}

	return MeshPaths;
}

FString ALoungeLayoutActor::ResolveSelectedItem(const FString& CategoryFolder, const FString& SelectedItem) const
{
	const TArray<FString> Options = GetFurnitureItemOptions(CategoryFolder);
	if (Options.Contains(SelectedItem))
	{
		return SelectedItem;
	}

	return Options.Num() > 0 ? Options[0] : FString();
}

FVector ALoungeLayoutActor::GetRowLocation(int32 Index, int32 Count, float SpacingCm, float YCm) const
{
	if (bUseAuthoredCommonOrigin)
	{
		return FVector::ZeroVector;
	}

	float X = Index * SpacingCm;
	if (bCenterRowsOnActor)
	{
		X -= (FMath::Max(0, Count - 1) * SpacingCm) * 0.5f;
	}

	return FVector(X, YCm, 0.0f);
}

TArray<FLoungeMirroredTransform> ALoungeLayoutActor::GetMirroredTransforms(const FVector& BaseLocation, const FRotator& Rotation, const FVector& BaseScale, ELoungeChairMirrorMode MirrorMode, int32 RequestedCount, bool bAlwaysKeepOriginal, float MirroredYawOffsetDegrees) const
{
	TArray<FLoungeMirroredTransform> Transforms;
	const int32 SafeRequestedCount = FMath::Max(0, RequestedCount);
	if (SafeRequestedCount <= 0)
	{
		return Transforms;
	}

	Transforms.Add(FLoungeMirroredTransform(FTransform(Rotation, BaseLocation, BaseScale), ELoungeChairMirrorMode::None));
	if (MirrorMode == ELoungeChairMirrorMode::None)
	{
		for (int32 Index = 1; Index < SafeRequestedCount; ++Index)
		{
			Transforms.Add(FLoungeMirroredTransform(FTransform(Rotation, BaseLocation, BaseScale), ELoungeChairMirrorMode::None));
		}
		return Transforms;
	}

	const int32 DesiredCount = bAlwaysKeepOriginal ? FMath::Max(2, SafeRequestedCount) : SafeRequestedCount;
	if (MirrorMode == ELoungeChairMirrorMode::MirrorEveryOtherChairAcrossXAndY)
	{
		if (Transforms.Num() < DesiredCount)
		{
			Transforms.Add(FLoungeMirroredTransform(FTransform(GetMirroredRotationForIndex(Rotation, ELoungeChairMirrorMode::MirrorEveryOtherChairAcrossX, 1, 2, MirroredYawOffsetDegrees), GetMirroredLocationForIndex(BaseLocation, ELoungeChairMirrorMode::MirrorEveryOtherChairAcrossX, 1, 2), BaseScale), ELoungeChairMirrorMode::MirrorEveryOtherChairAcrossX));
		}
		if (Transforms.Num() < DesiredCount)
		{
			Transforms.Add(FLoungeMirroredTransform(FTransform(GetMirroredRotationForIndex(Rotation, ELoungeChairMirrorMode::MirrorEveryOtherChairAcrossY, 1, 2, MirroredYawOffsetDegrees), GetMirroredLocationForIndex(BaseLocation, ELoungeChairMirrorMode::MirrorEveryOtherChairAcrossY, 1, 2), BaseScale), ELoungeChairMirrorMode::MirrorEveryOtherChairAcrossY));
		}
		if (Transforms.Num() < DesiredCount)
		{
			Transforms.Add(FLoungeMirroredTransform(FTransform(GetMirroredRotationForIndex(Rotation, ELoungeChairMirrorMode::MirrorEveryOtherChairAcrossXAndY, 1, 2, MirroredYawOffsetDegrees), GetMirroredLocationForIndex(BaseLocation, ELoungeChairMirrorMode::MirrorEveryOtherChairAcrossXAndY, 1, 2), BaseScale), ELoungeChairMirrorMode::MirrorEveryOtherChairAcrossXAndY));
		}
		return Transforms;
	}

	for (int32 Index = 1; Index < DesiredCount; ++Index)
	{
		Transforms.Add(FLoungeMirroredTransform(FTransform(GetMirroredRotationForIndex(Rotation, MirrorMode, Index, DesiredCount, MirroredYawOffsetDegrees), GetMirroredLocationForIndex(BaseLocation, MirrorMode, Index, DesiredCount), BaseScale), MirrorMode));
	}

	return Transforms;
}

FVector ALoungeLayoutActor::GetMirroredLocationForIndex(const FVector& BaseLocation, ELoungeChairMirrorMode MirrorMode, int32 Index, int32 Count) const
{
	FVector ResolvedLocation = BaseLocation;
	if (!ShouldMirrorIndex(MirrorMode, Index, Count))
	{
		return ResolvedLocation;
	}

	if (MirrorMode == ELoungeChairMirrorMode::MirrorEveryOtherChairAcrossX)
	{
		ResolvedLocation.X *= -1.0f;
	}
	else if (MirrorMode == ELoungeChairMirrorMode::MirrorEveryOtherChairAcrossY)
	{
		ResolvedLocation.Y *= -1.0f;
	}
	else if (MirrorMode == ELoungeChairMirrorMode::MirrorEveryOtherChairAcrossXAndY)
	{
		ResolvedLocation.X *= -1.0f;
		ResolvedLocation.Y *= -1.0f;
	}

	return ResolvedLocation;
}

FRotator ALoungeLayoutActor::GetMirroredRotationForIndex(const FRotator& BaseRotation, ELoungeChairMirrorMode MirrorMode, int32 Index, int32 Count, float MirroredYawOffsetDegrees) const
{
	FRotator ResolvedRotation = BaseRotation;
	if (!ShouldMirrorIndex(MirrorMode, Index, Count))
	{
		return ResolvedRotation;
	}

	if (MirrorMode == ELoungeChairMirrorMode::MirrorEveryOtherChairAcrossY ||
		MirrorMode == ELoungeChairMirrorMode::MirrorEveryOtherChairAcrossXAndY)
	{
		ResolvedRotation.Yaw += 180.0f;
	}

	if (MirrorMode == ELoungeChairMirrorMode::MirrorEveryOtherChairAcrossX ||
		MirrorMode == ELoungeChairMirrorMode::MirrorEveryOtherChairAcrossXAndY)
	{
		ResolvedRotation.Yaw += MirroredYawOffsetDegrees;
	}

	return ResolvedRotation;
}

bool ALoungeLayoutActor::ShouldMirrorIndex(ELoungeChairMirrorMode MirrorMode, int32 Index, int32 Count) const
{
	if (MirrorMode == ELoungeChairMirrorMode::None)
	{
		return false;
	}

	if (Count <= 1)
	{
		return true;
	}

	return Index % 2 != 0;
}

void ALoungeLayoutActor::AddMeshInstance(UInstancedStaticMeshComponent* Component, UStaticMesh* Mesh, const FTransform& InstanceTransform, ELoungeChairMirrorMode AppliedMirrorMode)
{
	if (!Component || !Mesh)
	{
		return;
	}

	if (Component->GetStaticMesh() != Mesh)
	{
		Component->SetStaticMesh(Mesh);
	}

	Component->SetVisibility(true);
	Component->SetHiddenInGame(false);

	FTransform ResolvedTransform = InstanceTransform;
	if (AppliedMirrorMode == ELoungeChairMirrorMode::MirrorEveryOtherChairAcrossX ||
		AppliedMirrorMode == ELoungeChairMirrorMode::MirrorEveryOtherChairAcrossY ||
		AppliedMirrorMode == ELoungeChairMirrorMode::MirrorEveryOtherChairAcrossXAndY)
	{
		const FVector ScaledMeshCenter = Mesh->GetBoundingBox().GetCenter() * ResolvedTransform.GetScale3D();
		const FVector TransformLocation = ResolvedTransform.GetLocation();
		const FVector CurrentCenter = ResolvedTransform.GetRotation().RotateVector(ScaledMeshCenter) + ResolvedTransform.GetLocation();
		FVector DesiredCenter = TransformLocation + ScaledMeshCenter;

		if (AppliedMirrorMode == ELoungeChairMirrorMode::MirrorEveryOtherChairAcrossX ||
			AppliedMirrorMode == ELoungeChairMirrorMode::MirrorEveryOtherChairAcrossXAndY)
		{
			DesiredCenter.X = TransformLocation.X - ScaledMeshCenter.X;
		}
		if (AppliedMirrorMode == ELoungeChairMirrorMode::MirrorEveryOtherChairAcrossY ||
			AppliedMirrorMode == ELoungeChairMirrorMode::MirrorEveryOtherChairAcrossXAndY)
		{
			DesiredCenter.Y = TransformLocation.Y - ScaledMeshCenter.Y;
		}

		ResolvedTransform.AddToTranslation(DesiredCenter - CurrentCenter);
	}

	Component->AddInstance(ResolvedTransform);
	++CurrentGeneratedMeshCount;
	ExpandGeneratedBounds(Mesh, ResolvedTransform);
}

void ALoungeLayoutActor::ExpandGeneratedBounds(UStaticMesh* Mesh, const FTransform& Transform)
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
		Location - FVector(LoungeFallbackExtentCm),
		Location + FVector(LoungeFallbackExtentCm));
}

void ALoungeLayoutActor::UpdateSelectionBounds()
{
	if (!SelectionBounds)
	{
		return;
	}

	SelectionBounds->SetWorldScale3D(FVector::OneVector);

	if (!GeneratedBounds.IsValid)
	{
		SelectionBounds->SetRelativeLocation(FVector::ZeroVector);
		SelectionBounds->SetBoxExtent(FVector(LoungeFallbackExtentCm));
		return;
	}

	SelectionBounds->SetRelativeLocation(GeneratedBounds.GetCenter());
	SelectionBounds->SetBoxExtent(GeneratedBounds.GetExtent().ComponentMax(FVector(50.0f, 50.0f, 50.0f)));
}
