#include "DiningTableActor.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Modules/ModuleManager.h"
#include "ProceduralMeshComponent.h"

namespace
{
constexpr float DiningTableFallbackExtentCm = 75.0f;

bool IsDiningStaticMeshAsset(const FAssetData& AssetData)
{
	return AssetData.AssetClassPath == UStaticMesh::StaticClass()->GetClassPathName();
}

bool IsTableclothAsset(const FAssetData& AssetData)
{
	const FName AssetClassName = AssetData.AssetClassPath.GetAssetName();
	return AssetClassName == TEXT("Material") ||
		AssetClassName == TEXT("MaterialInstanceConstant") ||
		AssetClassName == TEXT("MaterialInstance") ||
		AssetClassName == TEXT("Texture2D");
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

	const bool bBlocksNavigationAndPawn = FCString::Strcmp(Prefix, TEXT("DiningTable")) == 0;
	MeshComponent->SetCollisionEnabled(bBlocksNavigationAndPawn ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	MeshComponent->SetCollisionObjectType(ECC_WorldStatic);
	MeshComponent->SetCollisionResponseToAllChannels(bBlocksNavigationAndPawn ? ECR_Block : ECR_Ignore);
	MeshComponent->SetGenerateOverlapEvents(false);
	MeshComponent->SetCanEverAffectNavigation(bBlocksNavigationAndPawn);
	MeshComponent->bDisallowNanite = true;
}

FVector BuildTableclothSkirtNormal(const FVector& OutwardDirection, float UpBias)
{
	const float ClampedUpBias = FMath::Clamp(UpBias, 0.0f, 1.0f);
	const FVector SideNormal = OutwardDirection.GetSafeNormal();
	return (SideNormal * (1.0f - ClampedUpBias) + FVector::UpVector * ClampedUpBias).GetSafeNormal();
}
}

ADiningTableActor::ADiningTableActor()
{
	PrimaryActorTick.bCanEverTick = false;
	GeneratedBounds = FBox(EForceInit::ForceInit);
	CurrentTableBounds = FBox(EForceInit::ForceInit);
	CurrentChairBounds = FBox(EForceInit::ForceInit);

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

	TableclothMeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("DiningTable_Tablecloth"));
	TableclothMeshComponent->SetupAttachment(SceneRoot);
	TableclothMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TableclothMeshComponent->SetGenerateOverlapEvents(false);
	TableclothMeshComponent->SetCanEverAffectNavigation(false);
	TableclothMeshComponent->bUseAsyncCooking = true;
	TableclothMeshComponent->SetHiddenInGame(true);
}

void ADiningTableActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (bBuildOnConstruction)
	{
		RebuildDiningTable();
	}
}

#if WITH_EDITOR
void ADiningTableActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RebuildDiningTable();
}
#endif

void ADiningTableActor::RebuildDiningTable()
{
	ChairGapCm = FMath::Clamp(ChairGapCm, -200.0f, 80.0f);
	RectangularSideSpread = FMath::Clamp(RectangularSideSpread, 0.0f, 1.25f);
	TableclothOverhangCm = FMath::Clamp(TableclothOverhangCm, 0.0f, 80.0f);
	TableclothDropCm = FMath::Clamp(TableclothDropCm, 0.0f, 120.0f);
	RoundTableclothSegments = FMath::Clamp(RoundTableclothSegments, 12, 96);

	ClearGenerated();

	const FString ResolvedDiningTableItem = ResolveSelectedItem(DiningTableAssetFolder, DiningTableItem);
	const FString ResolvedDiningChairItem = ResolveSelectedItem(DiningChairAssetFolder, DiningChairItem);

	if (DiningTableItem.IsEmpty())
	{
		DiningTableItem = ResolvedDiningTableItem;
	}
	if (DiningChairItem.IsEmpty())
	{
		DiningChairItem = ResolvedDiningChairItem;
	}

	if (!ResolvedDiningTableItem.IsEmpty())
	{
		AddFurnitureItem(
			DiningTableAssetFolder,
			ResolvedDiningTableItem,
			TEXT("DiningTable"),
			FTransform(TableRotation, TablePlacementOffsetCm, TableScale),
			&CurrentTableBounds);
	}

	BuildTablecloth();

	if (!ResolvedDiningChairItem.IsEmpty())
	{
		CurrentChairBounds = GetFurnitureItemBounds(
			DiningChairAssetFolder,
			ResolvedDiningChairItem,
			FTransform(ChairRotation, FVector::ZeroVector, ChairScale));
		AddChairLayout(ResolvedDiningChairItem);
	}

	UpdateSelectionBounds();
}

void ADiningTableActor::ApplyBuildDefinition(const FDiningTableBuildDefinition& Definition, bool bRebuildNow)
{
	DiningTableItem = Definition.DiningTableItem;
	DiningChairItem = Definition.DiningChairItem;
	ChairCount = Definition.ChairCount;
	ChairArrangement = Definition.ChairArrangement;
	ChairGapCm = FMath::Clamp(Definition.ChairGapCm, -200.0f, 80.0f);
	bUseTablecloth = Definition.bUseTablecloth;
	TableclothItem = Definition.TableclothItem;
	TableclothShape = Definition.TableclothShape;
	TableclothOverhangCm = FMath::Clamp(Definition.TableclothOverhangCm, 0.0f, 80.0f);
	TableclothDropCm = FMath::Clamp(Definition.TableclothDropCm, 0.0f, 120.0f);

	if (bRebuildNow)
	{
		RebuildDiningTable();
	}
}

FDiningTableBuildDefinition ADiningTableActor::GetBuildDefinition() const
{
	FDiningTableBuildDefinition Definition;
	Definition.DiningTableItem = DiningTableItem;
	Definition.DiningChairItem = DiningChairItem;
	Definition.ChairCount = ChairCount;
	Definition.ChairArrangement = ChairArrangement;
	Definition.ChairGapCm = ChairGapCm;
	Definition.bUseTablecloth = bUseTablecloth;
	Definition.TableclothItem = TableclothItem;
	Definition.TableclothShape = TableclothShape;
	Definition.TableclothOverhangCm = TableclothOverhangCm;
	Definition.TableclothDropCm = TableclothDropCm;
	return Definition;
}

void ADiningTableActor::SetSelectionHighlighted(bool bHighlighted)
{
	if (SelectionBounds)
	{
		SelectionBounds->SetHiddenInGame(!bHighlighted);
	}
}

TArray<FString> ADiningTableActor::GetDiningTableOptions() const
{
	return GetFurnitureItemOptions(DiningTableAssetFolder);
}

TArray<FString> ADiningTableActor::GetDiningChairOptions() const
{
	return GetFurnitureItemOptions(DiningChairAssetFolder);
}

TArray<FString> ADiningTableActor::GetTableclothOptions() const
{
	return GetFlatAssetOptions(TableclothAssetFolder);
}

void ADiningTableActor::ClearGenerated()
{
	GeneratedBounds = FBox(EForceInit::ForceInit);
	CurrentTableBounds = FBox(EForceInit::ForceInit);
	CurrentChairBounds = FBox(EForceInit::ForceInit);
	CurrentGeneratedMeshCount = 0;

	if (TableclothMeshComponent)
	{
		TableclothMeshComponent->ClearAllMeshSections();
		TableclothMeshComponent->SetVisibility(false);
		TableclothMeshComponent->SetHiddenInGame(true);
	}

	TInlineComponentArray<UInstancedStaticMeshComponent*> ExistingInstanceComponents(this);
	for (UInstancedStaticMeshComponent* MeshComponent : ExistingInstanceComponents)
	{
		if (MeshComponent && MeshComponent->GetName().StartsWith(TEXT("DiningTable_")) && !GeneratedMeshComponents.Contains(MeshComponent))
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

void ADiningTableActor::AddFurnitureItem(const FString& CategoryFolder, const FString& ItemName, const TCHAR* Prefix, const FTransform& ItemTransform, FBox* OutItemBounds)
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
			ExpandGeneratedBounds(Mesh, ItemTransform, OutItemBounds);
		}
	}
}

void ADiningTableActor::AddChairLayout(const FString& ResolvedChairItem)
{
	const int32 ResolvedChairCount = GetResolvedChairCount();
	if (ResolvedChairCount <= 0)
	{
		return;
	}

	if (ChairArrangement == EDiningChairArrangement::Radial)
	{
		AddRadialChairLayout(ResolvedChairItem, ResolvedChairCount);
	}
	else
	{
		AddRectangularChairLayout(ResolvedChairItem, ResolvedChairCount);
	}
}

void ADiningTableActor::AddRadialChairLayout(const FString& ResolvedChairItem, int32 ResolvedChairCount)
{
	const FVector TableCenter = CurrentTableBounds.IsValid ? CurrentTableBounds.GetCenter() : FVector::ZeroVector;
	const FVector TableExtent = CurrentTableBounds.IsValid ? CurrentTableBounds.GetExtent() : FVector(DiningTableFallbackExtentCm);
	const float ChairInwardLocalY = GetChairInwardLocalYCm();
	const float RadiusCm = FMath::Max(TableExtent.X, TableExtent.Y) + ChairGapCm - ChairInwardLocalY;
	const float AngleStepDegrees = 360.0f / static_cast<float>(ResolvedChairCount);

	for (int32 ChairIndex = 0; ChairIndex < ResolvedChairCount; ++ChairIndex)
	{
		const float AngleDegrees = -90.0f + AngleStepDegrees * static_cast<float>(ChairIndex);
		const float AngleRadians = FMath::DegreesToRadians(AngleDegrees);
		const FVector LocalOffset(
			FMath::Cos(AngleRadians) * RadiusCm,
			FMath::Sin(AngleRadians) * RadiusCm,
			0.0f);

		FRotator InstanceRotation = ChairRotation;
		InstanceRotation.Yaw += AngleDegrees - 90.0f + ChairYawOffsetDegrees;

		AddFurnitureItem(
			DiningChairAssetFolder,
			ResolvedChairItem,
			TEXT("DiningChair"),
			FTransform(InstanceRotation, TableCenter + LocalOffset + ChairPlacementOffsetCm, ChairScale));
	}
}

void ADiningTableActor::AddRectangularChairLayout(const FString& ResolvedChairItem, int32 ResolvedChairCount)
{
	const FVector TableCenter = CurrentTableBounds.IsValid ? CurrentTableBounds.GetCenter() : FVector::ZeroVector;
	const FVector TableExtent = CurrentTableBounds.IsValid ? CurrentTableBounds.GetExtent() : FVector(DiningTableFallbackExtentCm);
	const float ChairInwardLocalY = GetChairInwardLocalYCm();
	const float ChairDistanceYCm = TableExtent.Y + ChairGapCm - ChairInwardLocalY;
	const int32 ChairsPerSide = FMath::Max(1, ResolvedChairCount / 2);
	const float SpreadCm = TableExtent.X * RectangularSideSpread;

	for (int32 SideIndex = 0; SideIndex < 2; ++SideIndex)
	{
		const bool bBackSide = SideIndex == 1;
		for (int32 ChairIndex = 0; ChairIndex < ChairsPerSide; ++ChairIndex)
		{
			const float Alpha = ChairsPerSide > 1
				? static_cast<float>(ChairIndex) / static_cast<float>(ChairsPerSide - 1)
				: 0.5f;
			const float XOffsetCm = FMath::Lerp(-SpreadCm, SpreadCm, Alpha);
			const float YOffsetCm = bBackSide ? ChairDistanceYCm : -ChairDistanceYCm;

			FRotator InstanceRotation = ChairRotation;
			InstanceRotation.Yaw += (bBackSide ? 0.0f : 180.0f) + ChairYawOffsetDegrees;

			AddFurnitureItem(
				DiningChairAssetFolder,
				ResolvedChairItem,
				TEXT("DiningChair"),
				FTransform(InstanceRotation, TableCenter + FVector(XOffsetCm, YOffsetCm, 0.0f) + ChairPlacementOffsetCm, ChairScale));
		}
	}
}

UInstancedStaticMeshComponent* ADiningTableActor::FindOrCreateMeshBucket(const FSoftObjectPath& MeshPath, const TCHAR* Prefix)
{
	const FString SafeAssetName = MakeComponentSafeName(MeshPath.GetAssetName());
	const FName ComponentName(*FString::Printf(TEXT("DiningTable_%s_%s"), Prefix, *SafeAssetName));
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

TArray<FString> ADiningTableActor::GetFurnitureItemOptions(const FString& CategoryFolder) const
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
		if (!IsDiningStaticMeshAsset(AssetData))
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

TArray<FSoftObjectPath> ADiningTableActor::GetMeshPathsForFurnitureItem(const FString& CategoryFolder, const FString& ItemName) const
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
		if (IsDiningStaticMeshAsset(AssetData))
		{
			MeshPaths.Add(AssetData.ToSoftObjectPath());
		}
	}

	return MeshPaths;
}

TArray<FString> ADiningTableActor::GetFlatAssetOptions(const FString& AssetFolder) const
{
	TArray<FString> Options;
	const FString NormalizedAssetFolder = NormalizeFolderPath(AssetFolder);
	if (NormalizedAssetFolder.IsEmpty())
	{
		return Options;
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> Assets;
	AssetRegistryModule.Get().GetAssetsByPath(*NormalizedAssetFolder, Assets, false);

	for (const FAssetData& AssetData : Assets)
	{
		if (!IsTableclothAsset(AssetData) || AssetData.AssetName == TEXT("UE_Placeholder"))
		{
			continue;
		}

		Options.Add(AssetData.AssetName.ToString());
	}

	Options.Sort();
	return Options;
}

FString ADiningTableActor::ResolveSelectedItem(const FString& CategoryFolder, const FString& SelectedItem) const
{
	const TArray<FString> Options = GetFurnitureItemOptions(CategoryFolder);
	if (Options.Contains(SelectedItem))
	{
		return SelectedItem;
	}

	return Options.Num() > 0 ? Options[0] : FString();
}

FBox ADiningTableActor::GetFurnitureItemBounds(const FString& CategoryFolder, const FString& ItemName, const FTransform& ItemTransform) const
{
	FBox ItemBounds(EForceInit::ForceInit);
	for (const FSoftObjectPath& MeshPath : GetMeshPathsForFurnitureItem(CategoryFolder, ItemName))
	{
		UStaticMesh* Mesh = Cast<UStaticMesh>(MeshPath.TryLoad());
		if (!Mesh)
		{
			continue;
		}

		const FBox MeshBounds = Mesh->GetBoundingBox().TransformBy(ItemTransform);
		if (MeshBounds.IsValid)
		{
			ItemBounds += MeshBounds;
		}
	}
	return ItemBounds;
}

void ADiningTableActor::ExpandGeneratedBounds(UStaticMesh* Mesh, const FTransform& Transform, FBox* OutItemBounds)
{
	if (!Mesh)
	{
		return;
	}

	const FBox MeshBounds = Mesh->GetBoundingBox().TransformBy(Transform);
	if (MeshBounds.IsValid)
	{
		GeneratedBounds += MeshBounds;
		if (OutItemBounds)
		{
			*OutItemBounds += MeshBounds;
		}
		return;
	}

	const FVector Location = Transform.GetLocation();
	const FBox FallbackBounds(
		Location - FVector(DiningTableFallbackExtentCm),
		Location + FVector(DiningTableFallbackExtentCm));
	GeneratedBounds += FallbackBounds;
	if (OutItemBounds)
	{
		*OutItemBounds += FallbackBounds;
	}
}

void ADiningTableActor::BuildTablecloth()
{
	if (!TableclothMeshComponent || !bUseTablecloth || !CurrentTableBounds.IsValid)
	{
		return;
	}

	if (TableclothItem.IsEmpty())
	{
		const TArray<FString> TableclothOptions = GetTableclothOptions();
		if (TableclothOptions.Num() > 0)
		{
			TableclothItem = TableclothOptions[0];
		}
	}

	const FString LowerTableName = DiningTableItem.ToLower();
	const bool bLooksRound = LowerTableName.Contains(TEXT("round")) ||
		LowerTableName.Contains(TEXT("circle")) ||
		LowerTableName.Contains(TEXT("cylinder"));
	const bool bUseRoundShape = TableclothShape == EDiningTableclothShape::Round ||
		(TableclothShape == EDiningTableclothShape::Auto && bLooksRound);

	if (bUseRoundShape)
	{
		BuildRoundTablecloth(CurrentTableBounds);
	}
	else
	{
		BuildRectangularTablecloth(CurrentTableBounds);
	}

	TableclothMeshComponent->SetMaterial(0, nullptr);
	if (UMaterialInterface* TableclothMaterial = ResolveTableclothMaterial())
	{
		TableclothMeshComponent->SetMaterial(0, TableclothMaterial);
	}

	TableclothMeshComponent->SetVisibility(true);
	TableclothMeshComponent->SetHiddenInGame(false);
}

void ADiningTableActor::BuildRectangularTablecloth(const FBox& TableBounds)
{
	const FVector OuterMin(TableBounds.Min.X - TableclothOverhangCm, TableBounds.Min.Y - TableclothOverhangCm, TableBounds.Max.Z + TableclothTopZOffsetCm);
	const FVector OuterMax(TableBounds.Max.X + TableclothOverhangCm, TableBounds.Max.Y + TableclothOverhangCm, TableBounds.Max.Z + TableclothTopZOffsetCm);
	const float TopZ = TableBounds.Max.Z + TableclothTopZOffsetCm;
	const float BottomZ = TopZ - TableclothDropCm;
	constexpr int32 TopSegmentsX = 16;
	constexpr int32 TopSegmentsY = 16;
	constexpr int32 SkirtVerticalSegments = 8;
	const float FoldAmplitudeCm = FMath::Clamp(FMath::Min(TableclothDropCm * 0.06f, TableclothOverhangCm * 0.45f), 1.5f, 7.0f);

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;

	auto AddQuad = [&Vertices, &Triangles, &Normals, &UVs](const FVector& A, const FVector& B, const FVector& C, const FVector& D, const FVector& Normal, const FVector2D& UVA, const FVector2D& UVB, const FVector2D& UVC, const FVector2D& UVD)
	{
		const int32 StartIndex = Vertices.Num();
		Vertices.Add(A);
		Vertices.Add(B);
		Vertices.Add(C);
		Vertices.Add(D);
		Normals.Add(Normal);
		Normals.Add(Normal);
		Normals.Add(Normal);
		Normals.Add(Normal);
		UVs.Add(UVA);
		UVs.Add(UVB);
		UVs.Add(UVC);
		UVs.Add(UVD);
		Triangles.Add(StartIndex);
		Triangles.Add(StartIndex + 1);
		Triangles.Add(StartIndex + 2);
		Triangles.Add(StartIndex);
		Triangles.Add(StartIndex + 2);
		Triangles.Add(StartIndex + 3);
	};

	for (int32 YIndex = 0; YIndex <= TopSegmentsY; ++YIndex)
	{
		const float V = static_cast<float>(YIndex) / static_cast<float>(TopSegmentsY);
		const float Y = FMath::Lerp(OuterMin.Y, OuterMax.Y, V);
		for (int32 XIndex = 0; XIndex <= TopSegmentsX; ++XIndex)
		{
			const float U = static_cast<float>(XIndex) / static_cast<float>(TopSegmentsX);
			const float X = FMath::Lerp(OuterMin.X, OuterMax.X, U);
			const float OutsideTableX = FMath::Max(TableBounds.Min.X - X, X - TableBounds.Max.X);
			const float OutsideTableY = FMath::Max(TableBounds.Min.Y - Y, Y - TableBounds.Max.Y);
			const float OutsideCm = FMath::Max(FMath::Max(OutsideTableX, OutsideTableY), 0.0f);
			const float OverhangAlpha = TableclothOverhangCm > KINDA_SMALL_NUMBER ? FMath::Clamp(OutsideCm / TableclothOverhangCm, 0.0f, 1.0f) : 0.0f;
			const float WaveCm = FMath::Sin(U * PI * 8.0f) * FMath::Sin(V * PI * 8.0f) * 0.75f;
			Vertices.Add(FVector(X, Y, TopZ - OverhangAlpha * 3.0f + WaveCm));
			Normals.Add(FVector::UpVector);
			UVs.Add(FVector2D(U * TableclothTextureScale, V * TableclothTextureScale));
		}
	}

	for (int32 YIndex = 0; YIndex < TopSegmentsY; ++YIndex)
	{
		for (int32 XIndex = 0; XIndex < TopSegmentsX; ++XIndex)
		{
			const int32 A = YIndex * (TopSegmentsX + 1) + XIndex;
			const int32 B = A + 1;
			const int32 C = A + (TopSegmentsX + 1) + 1;
			const int32 D = A + (TopSegmentsX + 1);
			Triangles.Add(A);
			Triangles.Add(C);
			Triangles.Add(B);
			Triangles.Add(A);
			Triangles.Add(D);
			Triangles.Add(C);
		}
	}

	auto AddSkirt = [&AddQuad, TopZ, BottomZ, FoldAmplitudeCm, SkirtVerticalSegments, this](
		const FVector& StartTop,
		const FVector& EndTop,
		const FVector& OutwardDirection,
		int32 HorizontalSegments)
	{
		const float SideLengthCm = FVector::Dist2D(StartTop, EndTop);
		const float UTileScale = FMath::Max(TableclothTextureScale, 0.01f);
		const float VTileScale = FMath::Max(TableclothTextureScale, 0.01f);
		const FVector SideNormal = BuildTableclothSkirtNormal(OutwardDirection, TableclothSideNormalUpBias);
		for (int32 HorizontalIndex = 0; HorizontalIndex < HorizontalSegments; ++HorizontalIndex)
		{
			const float U0 = static_cast<float>(HorizontalIndex) / static_cast<float>(HorizontalSegments);
			const float U1 = static_cast<float>(HorizontalIndex + 1) / static_cast<float>(HorizontalSegments);
			const FVector Top0 = FMath::Lerp(StartTop, EndTop, U0);
			const FVector Top1 = FMath::Lerp(StartTop, EndTop, U1);

			for (int32 VerticalIndex = 0; VerticalIndex < SkirtVerticalSegments; ++VerticalIndex)
			{
				const float V0 = static_cast<float>(VerticalIndex) / static_cast<float>(SkirtVerticalSegments);
				const float V1 = static_cast<float>(VerticalIndex + 1) / static_cast<float>(SkirtVerticalSegments);
				const float Fold0 = FMath::Sin(U0 * PI * 12.0f) * FMath::Sin(V0 * PI) * FoldAmplitudeCm;
				const float Fold1 = FMath::Sin(U1 * PI * 12.0f) * FMath::Sin(V0 * PI) * FoldAmplitudeCm;
				const float Fold2 = FMath::Sin(U1 * PI * 12.0f) * FMath::Sin(V1 * PI) * FoldAmplitudeCm;
				const float Fold3 = FMath::Sin(U0 * PI * 12.0f) * FMath::Sin(V1 * PI) * FoldAmplitudeCm;
				const float Hem0 = V0 > 0.99f ? FMath::Sin(U0 * PI * 10.0f) * 2.5f : 0.0f;
				const float Hem1 = V1 > 0.99f ? FMath::Sin(U1 * PI * 10.0f) * 2.5f : 0.0f;

				FVector A = FMath::Lerp(Top0, FVector(Top0.X, Top0.Y, BottomZ + Hem0), V0) + OutwardDirection * Fold0;
				FVector B = FMath::Lerp(Top1, FVector(Top1.X, Top1.Y, BottomZ + Hem0), V0) + OutwardDirection * Fold1;
				FVector C = FMath::Lerp(Top1, FVector(Top1.X, Top1.Y, BottomZ + Hem1), V1) + OutwardDirection * Fold2;
				FVector D = FMath::Lerp(Top0, FVector(Top0.X, Top0.Y, BottomZ + Hem1), V1) + OutwardDirection * Fold3;
				AddQuad(
					A,
					B,
					C,
					D,
					SideNormal,
					FVector2D((U0 * SideLengthCm / 100.0f) * UTileScale, (V0 * TableclothDropCm / 100.0f) * VTileScale),
					FVector2D((U1 * SideLengthCm / 100.0f) * UTileScale, (V0 * TableclothDropCm / 100.0f) * VTileScale),
					FVector2D((U1 * SideLengthCm / 100.0f) * UTileScale, (V1 * TableclothDropCm / 100.0f) * VTileScale),
					FVector2D((U0 * SideLengthCm / 100.0f) * UTileScale, (V1 * TableclothDropCm / 100.0f) * VTileScale));
			}
		}
	};

	AddSkirt(FVector(OuterMin.X, OuterMin.Y, TopZ), FVector(OuterMax.X, OuterMin.Y, TopZ), FVector(0.0f, -1.0f, 0.0f), TopSegmentsX);
	AddSkirt(FVector(OuterMax.X, OuterMin.Y, TopZ), FVector(OuterMax.X, OuterMax.Y, TopZ), FVector(1.0f, 0.0f, 0.0f), TopSegmentsY);
	AddSkirt(FVector(OuterMax.X, OuterMax.Y, TopZ), FVector(OuterMin.X, OuterMax.Y, TopZ), FVector(0.0f, 1.0f, 0.0f), TopSegmentsX);
	AddSkirt(FVector(OuterMin.X, OuterMax.Y, TopZ), FVector(OuterMin.X, OuterMin.Y, TopZ), FVector(-1.0f, 0.0f, 0.0f), TopSegmentsY);

	TableclothMeshComponent->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, TArray<FColor>(), TArray<FProcMeshTangent>(), false);
	GeneratedBounds += FBox(FVector(OuterMin.X - FoldAmplitudeCm, OuterMin.Y - FoldAmplitudeCm, BottomZ - 3.0f), FVector(OuterMax.X + FoldAmplitudeCm, OuterMax.Y + FoldAmplitudeCm, TopZ + 1.0f));
}

void ADiningTableActor::BuildRoundTablecloth(const FBox& TableBounds)
{
	const FVector Center = TableBounds.GetCenter();
	const float TopZ = TableBounds.Max.Z + TableclothTopZOffsetCm;
	const float BottomZ = TopZ - TableclothDropCm;
	const float Radius = FMath::Max(TableBounds.GetExtent().X, TableBounds.GetExtent().Y) + TableclothOverhangCm;
	const int32 Segments = FMath::Clamp(RoundTableclothSegments, 12, 96);
	constexpr int32 TopRings = 6;
	constexpr int32 SkirtVerticalSegments = 8;
	const float FoldAmplitudeCm = FMath::Clamp(FMath::Min(TableclothDropCm * 0.06f, TableclothOverhangCm * 0.45f), 1.5f, 7.0f);

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;

	for (int32 RingIndex = 0; RingIndex <= TopRings; ++RingIndex)
	{
		const float RingAlpha = static_cast<float>(RingIndex) / static_cast<float>(TopRings);
		const float RingRadius = Radius * RingAlpha;
		for (int32 SegmentIndex = 0; SegmentIndex < Segments; ++SegmentIndex)
		{
			const float Angle = 2.0f * PI * static_cast<float>(SegmentIndex) / static_cast<float>(Segments);
			const float CosA = FMath::Cos(Angle);
			const float SinA = FMath::Sin(Angle);
			const float EdgeSag = FMath::Pow(RingAlpha, 4.0f) * 3.0f;
			const float WaveCm = FMath::Sin(Angle * 8.0f) * RingAlpha * 0.75f;
			Vertices.Add(FVector(Center.X + CosA * RingRadius, Center.Y + SinA * RingRadius, TopZ - EdgeSag + WaveCm));
			Normals.Add(FVector::UpVector);
			UVs.Add(FVector2D((0.5f + CosA * RingAlpha * 0.5f) * TableclothTextureScale, (0.5f + SinA * RingAlpha * 0.5f) * TableclothTextureScale));
		}
	}

	for (int32 SegmentIndex = 1; SegmentIndex < Segments - 1; ++SegmentIndex)
	{
		Triangles.Add(0);
		Triangles.Add(SegmentIndex);
		Triangles.Add(SegmentIndex + 1);
	}

	for (int32 RingIndex = 0; RingIndex < TopRings; ++RingIndex)
	{
		const int32 CurrentRingStart = RingIndex * Segments;
		const int32 NextRingStart = (RingIndex + 1) * Segments;
		for (int32 SegmentIndex = 0; SegmentIndex < Segments; ++SegmentIndex)
		{
			const int32 NextSegmentIndex = (SegmentIndex + 1) % Segments;
			const int32 A = CurrentRingStart + SegmentIndex;
			const int32 B = CurrentRingStart + NextSegmentIndex;
			const int32 C = NextRingStart + NextSegmentIndex;
			const int32 D = NextRingStart + SegmentIndex;
			Triangles.Add(A);
			Triangles.Add(B);
			Triangles.Add(C);
			Triangles.Add(A);
			Triangles.Add(C);
			Triangles.Add(D);
		}
	}

	for (int32 SegmentIndex = 0; SegmentIndex < Segments; ++SegmentIndex)
	{
		const int32 NextIndex = (SegmentIndex + 1) % Segments;
		const float CurrentAngle = 2.0f * PI * static_cast<float>(SegmentIndex) / static_cast<float>(Segments);
		const float NextAngle = 2.0f * PI * static_cast<float>(NextIndex) / static_cast<float>(Segments);

		for (int32 VerticalIndex = 0; VerticalIndex < SkirtVerticalSegments; ++VerticalIndex)
		{
			const float V0 = static_cast<float>(VerticalIndex) / static_cast<float>(SkirtVerticalSegments);
			const float V1 = static_cast<float>(VerticalIndex + 1) / static_cast<float>(SkirtVerticalSegments);
			const float U0 = static_cast<float>(SegmentIndex) / static_cast<float>(Segments);
			const float U1 = static_cast<float>(NextIndex) / static_cast<float>(Segments);
			const float Fold0 = FMath::Sin(CurrentAngle * 10.0f) * FMath::Sin(V0 * PI) * FoldAmplitudeCm;
			const float Fold1 = FMath::Sin(NextAngle * 10.0f) * FMath::Sin(V0 * PI) * FoldAmplitudeCm;
			const float Fold2 = FMath::Sin(NextAngle * 10.0f) * FMath::Sin(V1 * PI) * FoldAmplitudeCm;
			const float Fold3 = FMath::Sin(CurrentAngle * 10.0f) * FMath::Sin(V1 * PI) * FoldAmplitudeCm;
			const float Hem0 = V0 > 0.99f ? FMath::Sin(CurrentAngle * 12.0f) * 2.5f : 0.0f;
			const float Hem1 = V1 > 0.99f ? FMath::Sin(NextAngle * 12.0f) * 2.5f : 0.0f;
			const int32 StartIndex = Vertices.Num();

			const float Radius0Top = Radius + Fold0;
			const float Radius1Top = Radius + Fold1;
			const float Radius1Bottom = Radius + Fold2;
			const float Radius0Bottom = Radius + Fold3;
			Vertices.Add(FVector(Center.X + FMath::Cos(CurrentAngle) * Radius0Top, Center.Y + FMath::Sin(CurrentAngle) * Radius0Top, FMath::Lerp(TopZ, BottomZ + Hem0, V0)));
			Vertices.Add(FVector(Center.X + FMath::Cos(NextAngle) * Radius1Top, Center.Y + FMath::Sin(NextAngle) * Radius1Top, FMath::Lerp(TopZ, BottomZ + Hem0, V0)));
			Vertices.Add(FVector(Center.X + FMath::Cos(NextAngle) * Radius1Bottom, Center.Y + FMath::Sin(NextAngle) * Radius1Bottom, FMath::Lerp(TopZ, BottomZ + Hem1, V1)));
			Vertices.Add(FVector(Center.X + FMath::Cos(CurrentAngle) * Radius0Bottom, Center.Y + FMath::Sin(CurrentAngle) * Radius0Bottom, FMath::Lerp(TopZ, BottomZ + Hem1, V1)));
			const FVector Normal0 = BuildTableclothSkirtNormal(FVector(FMath::Cos(CurrentAngle), FMath::Sin(CurrentAngle), 0.0f), TableclothSideNormalUpBias);
			const FVector Normal1 = BuildTableclothSkirtNormal(FVector(FMath::Cos(NextAngle), FMath::Sin(NextAngle), 0.0f), TableclothSideNormalUpBias);
			Normals.Add(Normal0);
			Normals.Add(Normal1);
			Normals.Add(Normal1);
			Normals.Add(Normal0);
			const float UTileScale = FMath::Max(TableclothTextureScale, 0.01f);
			const float VTileScale = FMath::Max(TableclothTextureScale, 0.01f);
			UVs.Add(FVector2D((U0 * 2.0f * PI * Radius / 100.0f) * UTileScale, (V0 * TableclothDropCm / 100.0f) * VTileScale));
			UVs.Add(FVector2D((U1 * 2.0f * PI * Radius / 100.0f) * UTileScale, (V0 * TableclothDropCm / 100.0f) * VTileScale));
			UVs.Add(FVector2D((U1 * 2.0f * PI * Radius / 100.0f) * UTileScale, (V1 * TableclothDropCm / 100.0f) * VTileScale));
			UVs.Add(FVector2D((U0 * 2.0f * PI * Radius / 100.0f) * UTileScale, (V1 * TableclothDropCm / 100.0f) * VTileScale));

			Triangles.Add(StartIndex);
			Triangles.Add(StartIndex + 1);
			Triangles.Add(StartIndex + 2);
			Triangles.Add(StartIndex);
			Triangles.Add(StartIndex + 2);
			Triangles.Add(StartIndex + 3);
		}
	}

	TableclothMeshComponent->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, TArray<FColor>(), TArray<FProcMeshTangent>(), false);
	GeneratedBounds += FBox(FVector(Center.X - Radius - FoldAmplitudeCm, Center.Y - Radius - FoldAmplitudeCm, BottomZ - 3.0f), FVector(Center.X + Radius + FoldAmplitudeCm, Center.Y + Radius + FoldAmplitudeCm, TopZ + 1.0f));
}

UMaterialInterface* ADiningTableActor::ResolveTableclothMaterial()
{
	const FString NormalizedFolder = NormalizeFolderPath(TableclothAssetFolder);
	UObject* SelectedTableclothAsset = nullptr;
	if (!TableclothItem.IsEmpty())
	{
		const FString AssetPath = FString::Printf(TEXT("%s/%s.%s"), *NormalizedFolder, *TableclothItem, *TableclothItem);
		SelectedTableclothAsset = FSoftObjectPath(AssetPath).TryLoad();
		if (UMaterialInterface* DirectMaterial = Cast<UMaterialInterface>(SelectedTableclothAsset))
		{
			return DirectMaterial;
		}
	}

	UMaterialInterface* BaseMaterial = TableclothBaseMaterial.LoadSynchronous();
	if (!BaseMaterial)
	{
		UE_LOG(LogTemp, Warning, TEXT("DiningTableActor: TableclothBaseMaterial is missing. Create or assign a two-sided material with a Texture2D parameter named '%s'."), *TableclothTextureParameterName.ToString());
		return nullptr;
	}

	if (UTexture2D* Texture = Cast<UTexture2D>(SelectedTableclothAsset))
	{
		UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		TArray<FName> TextureParameterNames;
		if (!TableclothTextureParameterName.IsNone())
		{
			TextureParameterNames.Add(TableclothTextureParameterName);
		}
		TextureParameterNames.AddUnique(TEXT("TableclothTexture"));
		TextureParameterNames.AddUnique(TEXT("BaseColor"));
		TextureParameterNames.AddUnique(TEXT("BaseColorTexture"));
		TextureParameterNames.AddUnique(TEXT("Texture"));
		TextureParameterNames.AddUnique(TEXT("Albedo"));
		TextureParameterNames.AddUnique(TEXT("Diffuse"));

		for (const FName& ParameterName : TextureParameterNames)
		{
			DynamicMaterial->SetTextureParameterValue(ParameterName, Texture);
		}

		if (!TableclothTextureScaleParameterName.IsNone())
		{
			DynamicMaterial->SetScalarParameterValue(TableclothTextureScaleParameterName, FMath::Max(TableclothTextureScale, 0.01f));
		}
		DynamicMaterial->SetScalarParameterValue(TEXT("TableclothTextureScale"), FMath::Max(TableclothTextureScale, 0.01f));
		return DynamicMaterial;
	}

	return BaseMaterial;
}

UTexture2D* ADiningTableActor::ResolveTableclothTexture() const
{
	if (TableclothItem.IsEmpty())
	{
		return nullptr;
	}

	const FString NormalizedFolder = NormalizeFolderPath(TableclothAssetFolder);
	const FString AssetPath = FString::Printf(TEXT("%s/%s.%s"), *NormalizedFolder, *TableclothItem, *TableclothItem);
	return Cast<UTexture2D>(FSoftObjectPath(AssetPath).TryLoad());
}

void ADiningTableActor::UpdateSelectionBounds()
{
	if (!SelectionBounds)
	{
		return;
	}

	SelectionBounds->SetWorldScale3D(FVector::OneVector);

	if (!GeneratedBounds.IsValid)
	{
		SelectionBounds->SetRelativeLocation(FVector::ZeroVector);
		SelectionBounds->SetBoxExtent(FVector(DiningTableFallbackExtentCm));
		return;
	}

	SelectionBounds->SetRelativeLocation(GeneratedBounds.GetCenter());
	SelectionBounds->SetBoxExtent(GeneratedBounds.GetExtent().ComponentMax(FVector(50.0f, 50.0f, 50.0f)));
}

int32 ADiningTableActor::GetResolvedChairCount() const
{
	switch (ChairCount)
	{
	case EDiningChairCount::Two:
		return 2;
	case EDiningChairCount::Four:
		return 4;
	case EDiningChairCount::Six:
		return 6;
	case EDiningChairCount::Eight:
		return 8;
	default:
		return 4;
	}
}

float ADiningTableActor::GetChairInwardLocalYCm() const
{
	if (CurrentChairBounds.IsValid)
	{
		return CurrentChairBounds.Min.Y + ChairInwardAdjustmentCm;
	}

	return ChairInwardAdjustmentCm;
}
