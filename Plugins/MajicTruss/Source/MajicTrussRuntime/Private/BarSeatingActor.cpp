#include "BarSeatingActor.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Animation/AnimationAsset.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Modules/ModuleManager.h"
#include "TimerManager.h"

namespace
{
constexpr float BarSeatingFallbackExtentCm = 75.0f;
constexpr float DuplicatePreviewSlotDistanceCm = 75.0f;

bool IsBarSeatingStaticMeshAsset(const FAssetData& AssetData)
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

	const bool bBlocksNavigationAndPawn = FCString::Strcmp(Prefix, TEXT("BarTable")) == 0;
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

USkeletalMeshComponent* FindMetaHumanBodyComponent(AActor* VisualActor, USkeletalMeshComponent*& OutFallbackMeshComponent)
{
	OutFallbackMeshComponent = nullptr;
	if (!VisualActor)
	{
		return nullptr;
	}

	TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
	VisualActor->GetComponents<USkeletalMeshComponent>(SkeletalMeshComponents);
	for (USkeletalMeshComponent* MeshComponent : SkeletalMeshComponents)
	{
		if (!MeshComponent || !MeshComponent->GetSkeletalMeshAsset())
		{
			continue;
		}

		if (!OutFallbackMeshComponent)
		{
			OutFallbackMeshComponent = MeshComponent;
		}

		if (MeshComponent->GetName() == TEXT("Body"))
		{
			return MeshComponent;
		}
	}

	return nullptr;
}

void PlaySingleNodeAnimation(USkeletalMeshComponent* MeshComponent, UAnimationAsset* Animation, bool bLooping, float PlayRate = 1.0f)
{
	if (!MeshComponent || !Animation)
	{
		return;
	}

	MeshComponent->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	MeshComponent->SetAnimation(Animation);
	MeshComponent->Play(bLooping);
	MeshComponent->SetPlayRate(PlayRate);
}

USkeletalMeshComponent* FindMetaHumanFaceComponent(AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}
	TArray<USkeletalMeshComponent*> Components;
	Actor->GetComponents<USkeletalMeshComponent>(Components);
	for (USkeletalMeshComponent* Comp : Components)
	{
		if (Comp && Comp->GetName().Equals(TEXT("Face"), ESearchCase::IgnoreCase))
		{
			return Comp;
		}
	}
	return nullptr;
}

void ApplyFullMetaHumanAnimation(AActor* AttendeeActor, UAnimationAsset* Animation)
{
	if (!AttendeeActor || !Animation)
	{
		return;
	}

	USkeletalMeshComponent* Fallback = nullptr;
	USkeletalMeshComponent* BodyComp = FindMetaHumanBodyComponent(AttendeeActor, Fallback);
	USkeletalMeshComponent* MeshToAnimate = BodyComp ? BodyComp : Fallback;
	if (!MeshToAnimate)
	{
		return;
	}

	PlaySingleNodeAnimation(MeshToAnimate, Animation, true);

	USkeletalMeshComponent* FaceComp = FindMetaHumanFaceComponent(AttendeeActor);

	// Face as leader-pose follower of Body positions the head correctly.
	// Clearing Face's ABP prevents it fighting the leader pose (neutral expression for previs).
	if (FaceComp && BodyComp)
	{
		FaceComp->SetLeaderPoseComponent(BodyComp, true, true);
		FaceComp->SetAnimClass(nullptr);
	}

	static const TArray<FString> FaceComponentNames = {
		TEXT("Face"), TEXT("Teeth"), TEXT("EyeLeft"), TEXT("EyeRight"),
		TEXT("Eyes_L"), TEXT("Eyes_R"), TEXT("Eyelash"), TEXT("Eyelashes")
	};

	// Clothing follows Body; Teeth/Eyes follow Face.
	TArray<USkeletalMeshComponent*> AllComponents;
	AttendeeActor->GetComponents<USkeletalMeshComponent>(AllComponents);
	for (USkeletalMeshComponent* Comp : AllComponents)
	{
		if (!Comp || !Comp->GetSkeletalMeshAsset() || Comp == MeshToAnimate || Comp == FaceComp)
		{
			continue;
		}

		const FString Name = Comp->GetName();
		bool bIsFaceDependent = false;
		for (const FString& FaceName : FaceComponentNames)
		{
			if (Name.Equals(FaceName, ESearchCase::IgnoreCase) || Name.StartsWith(FaceName, ESearchCase::IgnoreCase))
			{
				bIsFaceDependent = true;
				break;
			}
		}

		if (bIsFaceDependent && FaceComp)
		{
			Comp->SetLeaderPoseComponent(FaceComp, true, true);
		}
		else if (!bIsFaceDependent && BodyComp)
		{
			Comp->SetLeaderPoseComponent(BodyComp, true, true);
		}
	}

	// Force LOD 0 so lower LODs don't distort the pose.
	for (USkeletalMeshComponent* Comp : AllComponents)
	{
		if (Comp)
		{
			Comp->SetForcedLOD(1);
		}
	}
}
}

ABarSeatingActor::ABarSeatingActor()
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
	AIAnimationPreviewDefaultAnimation = TSoftObjectPtr<UAnimationAsset>(FSoftObjectPath(TEXT("/Game/IdleSitPoseAnimations/Animations/ArmsOnLegs_Sit.ArmsOnLegs_Sit")));
	AmbientAttendeeBlueprints.Add(TSoftClassPtr<AActor>(FSoftObjectPath(TEXT("/Game/MetaHumans/MH_Guest_01/BP_MH_Guest_01.BP_MH_Guest_01_C"))));
	AmbientAttendeeAnimations.Add(TSoftObjectPtr<UAnimationAsset>(FSoftObjectPath(TEXT("/Game/EventAI/MetaHumanAnimations/ArmsOnLegs_Sit.ArmsOnLegs_Sit"))));
}

void ABarSeatingActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (bBuildOnConstruction)
	{
		RebuildBarSeating();
	}
}

#if WITH_EDITOR
void ABarSeatingActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RebuildBarSeating();
}
#endif

void ABarSeatingActor::RebuildBarSeating()
{
	StoolOffsetYCm = FMath::Max(0.0f, StoolOffsetYCm);
	RectangleStoolPairOffsetXCm = FMath::Max(0.0f, RectangleStoolPairOffsetXCm);
	AISlotSourceLocalOffsets.Reset();
	AISlotSourceLocalRotations.Reset();
	ClearGenerated();

	const FString ResolvedBarTableItem = ResolveSelectedItem(BarTableAssetFolder, BarTableItem);
	const FString ResolvedStoolItem = ResolveSelectedItem(StoolAssetFolder, StoolItem);

	if (BarTableItem.IsEmpty())
	{
		BarTableItem = ResolvedBarTableItem;
	}
	if (StoolItem.IsEmpty())
	{
		StoolItem = ResolvedStoolItem;
	}

	if (!ResolvedBarTableItem.IsEmpty())
	{
		AddFurnitureItem(
			BarTableAssetFolder,
			ResolvedBarTableItem,
			TEXT("BarTable"),
			FTransform(BarTableRotation, FVector::ZeroVector, BarTableScale));
	}

	if (!ResolvedStoolItem.IsEmpty())
	{
		AddStoolLayout(ResolvedStoolItem);
	}

	UpdateSelectionBounds();
	RefreshAISlotPreviewComponents();

	TArray<FVector> PreviewOffsets;
	GetAISlotPreviewLocalOffsets(PreviewOffsets);
	TArray<FRotator> PreviewRotations;
	GetAISlotPreviewLocalRotations(PreviewRotations);
	RefreshAmbientAttendeeComponents(PreviewOffsets, PreviewRotations);
}

void ABarSeatingActor::ApplyBuildDefinition(const FBarSeatingBuildDefinition& Definition, bool bRebuildNow)
{
	BarTableItem = Definition.BarTableItem;
	StoolItem = Definition.StoolItem;
	StoolLayout = Definition.StoolLayout;
	bClassroomStyle = Definition.bClassroomStyle;
	StoolOffsetYCm = FMath::Max(0.0f, Definition.StoolOffsetYCm);
	RectangleStoolPairOffsetXCm = FMath::Max(0.0f, Definition.RectangleStoolPairOffsetXCm);

	if (bRebuildNow)
	{
		RebuildBarSeating();
	}
}

FBarSeatingBuildDefinition ABarSeatingActor::GetBuildDefinition() const
{
	FBarSeatingBuildDefinition Definition;
	Definition.BarTableItem = BarTableItem;
	Definition.StoolItem = StoolItem;
	Definition.StoolLayout = StoolLayout;
	Definition.bClassroomStyle = bClassroomStyle;
	Definition.StoolOffsetYCm = StoolOffsetYCm;
	Definition.RectangleStoolPairOffsetXCm = RectangleStoolPairOffsetXCm;
	return Definition;
}

void ABarSeatingActor::SetSelectionHighlighted(bool bHighlighted)
{
	if (SelectionBounds)
	{
		SelectionBounds->SetHiddenInGame(!bHighlighted);
	}
}

void ABarSeatingActor::GetAISlotPreviewLocalOffsets(TArray<FVector>& OutOffsets) const
{
	OutOffsets.Reset();

	const bool bUseCircularFallback = StoolLayout == EBarSeatingStoolLayout::TwoTop || StoolLayout == EBarSeatingStoolLayout::FourTop;
	if (!bUseCircularFallback)
	{
		for (int32 SourceIndex = 0; SourceIndex < AISlotSourceLocalOffsets.Num(); ++SourceIndex)
		{
			const FVector PerSlotOffset = AISlotPerSlotOffsetsCm.IsValidIndex(SourceIndex)
				? AISlotPerSlotOffsetsCm[SourceIndex]
				: FVector::ZeroVector;
			OutOffsets.Add(AISlotSourceLocalOffsets[SourceIndex] + AISlotPreviewOffsetCm + PerSlotOffset);
		}
		return;
	}

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

void ABarSeatingActor::GetAISlotPreviewAnimations(TArray<TSoftObjectPtr<UAnimationAsset>>& OutAnimations) const
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

void ABarSeatingActor::GetAISlotPreviewLocalRotations(TArray<FRotator>& OutRotations) const
{
	OutRotations.Reset();

	const bool bUseCircularFacing = StoolLayout == EBarSeatingStoolLayout::TwoTop || StoolLayout == EBarSeatingStoolLayout::FourTop;
	TArray<FVector> PreviewOffsets;
	GetAISlotPreviewLocalOffsets(PreviewOffsets);

	for (int32 Index = 0; Index < PreviewOffsets.Num(); ++Index)
	{
		if (bUseCircularFacing)
		{
			FRotator Rotation = MakeAISlotFacingRotation(PreviewOffsets[Index], AISlotFacingLocalPointCm, AISlotFacingYawOffsetDegrees);
			if (AISlotPerSlotRotationOffsets.IsValidIndex(Index))
			{
				Rotation += AISlotPerSlotRotationOffsets[Index];
			}
			OutRotations.Add(Rotation);
		}
		else
		{
			FRotator Rotation = AISlotSourceLocalRotations.IsValidIndex(Index)
				? AISlotSourceLocalRotations[Index]
				: FRotator::ZeroRotator;
			Rotation.Yaw += AISlotFacingYawOffsetDegrees;
			if (AISlotPerSlotRotationOffsets.IsValidIndex(Index))
			{
				Rotation += AISlotPerSlotRotationOffsets[Index];
			}
			OutRotations.Add(Rotation);
		}
	}
}

TArray<FString> ABarSeatingActor::GetBarTableOptions() const
{
	return GetFurnitureItemOptions(BarTableAssetFolder);
}

TArray<FString> ABarSeatingActor::GetStoolOptions() const
{
	return GetFurnitureItemOptions(StoolAssetFolder);
}

void ABarSeatingActor::ClearGenerated()
{
	GeneratedBounds = FBox(EForceInit::ForceInit);
	CurrentGeneratedMeshCount = 0;

	TInlineComponentArray<UInstancedStaticMeshComponent*> ExistingInstanceComponents(this);
	for (UInstancedStaticMeshComponent* MeshComponent : ExistingInstanceComponents)
	{
		if (MeshComponent && MeshComponent->GetName().StartsWith(TEXT("BarSeating_")) && !GeneratedMeshComponents.Contains(MeshComponent))
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

void ABarSeatingActor::AddFurnitureItem(const FString& CategoryFolder, const FString& ItemName, const TCHAR* Prefix, const FTransform& ItemTransform)
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

void ABarSeatingActor::AddStoolLayout(const FString& ResolvedStoolItem)
{
	if (StoolLayout == EBarSeatingStoolLayout::NoStools)
	{
		return;
	}

	if (StoolLayout == EBarSeatingStoolLayout::TwoTop)
	{
		AddStoolPair(ResolvedStoolItem, 0.0f);
		return;
	}

	if (StoolLayout == EBarSeatingStoolLayout::FourTop)
	{
		const FVector BaseOffset(0.0f, -StoolOffsetYCm, 0.0f);
		const int32 StoolCount = bClassroomStyle ? 1 : 4;
		for (int32 StoolIndex = 0; StoolIndex < StoolCount; ++StoolIndex)
		{
			const float YawDegrees = 90.0f * static_cast<float>(StoolIndex);
			const FRotator OrbitRotation(0.0f, YawDegrees, 0.0f);
			FRotator InstanceRotation = StoolRotation;
			InstanceRotation.Yaw += YawDegrees;

			AddFurnitureItem(
				StoolAssetFolder,
				ResolvedStoolItem,
				TEXT("Stool"),
				FTransform(InstanceRotation, OrbitRotation.RotateVector(BaseOffset), StoolScale));
			AddAISlotSource(OrbitRotation.RotateVector(BaseOffset));
		}
		return;
	}

	if (StoolLayout == EBarSeatingStoolLayout::RectangleFourTop)
	{
		const float HalfPairOffsetXCm = RectangleStoolPairOffsetXCm * 0.5f;
		AddStoolPair(ResolvedStoolItem, -HalfPairOffsetXCm);
		AddStoolPair(ResolvedStoolItem, HalfPairOffsetXCm);
	}
	else if (StoolLayout == EBarSeatingStoolLayout::RectangleSixTop)
	{
		AddStoolPair(ResolvedStoolItem, 0.0f);
		AddStoolPair(ResolvedStoolItem, RectangleStoolPairOffsetXCm);
		AddStoolPair(ResolvedStoolItem, -RectangleStoolPairOffsetXCm);
	}
}

void ABarSeatingActor::AddStoolPair(const FString& ResolvedStoolItem, float XOffsetCm)
{
	const FVector BaseOffset(XOffsetCm, -StoolOffsetYCm, 0.0f);
	const FVector AISlotBaseOffset(XOffsetCm, -RectangleAISlotHalfWidthCm, 0.0f);
	const int32 StoolCount = bClassroomStyle ? 1 : 2;

	for (int32 StoolIndex = 0; StoolIndex < StoolCount; ++StoolIndex)
	{
		const float YawDegrees = 180.0f * static_cast<float>(StoolIndex);
		const FRotator OrbitRotation(0.0f, YawDegrees, 0.0f);
		FRotator InstanceRotation = StoolRotation;
		InstanceRotation.Yaw += YawDegrees;

		AddFurnitureItem(
			StoolAssetFolder,
			ResolvedStoolItem,
			TEXT("Stool"),
			FTransform(InstanceRotation, OrbitRotation.RotateVector(BaseOffset), StoolScale));
		AddAISlotSource(OrbitRotation.RotateVector(AISlotBaseOffset), InstanceRotation);
	}
}

UInstancedStaticMeshComponent* ABarSeatingActor::FindOrCreateMeshBucket(const FSoftObjectPath& MeshPath, const TCHAR* Prefix)
{
	const FString SafeAssetName = MakeComponentSafeName(MeshPath.GetAssetName());
	const FName ComponentName(*FString::Printf(TEXT("BarSeating_%s_%s"), Prefix, *SafeAssetName));
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

TArray<FString> ABarSeatingActor::GetFurnitureItemOptions(const FString& CategoryFolder) const
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
		if (!IsBarSeatingStaticMeshAsset(AssetData))
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

TArray<FSoftObjectPath> ABarSeatingActor::GetMeshPathsForFurnitureItem(const FString& CategoryFolder, const FString& ItemName) const
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
		if (IsBarSeatingStaticMeshAsset(AssetData))
		{
			MeshPaths.Add(AssetData.ToSoftObjectPath());
		}
	}

	return MeshPaths;
}

FString ABarSeatingActor::ResolveSelectedItem(const FString& CategoryFolder, const FString& SelectedItem) const
{
	const TArray<FString> Options = GetFurnitureItemOptions(CategoryFolder);
	if (Options.Contains(SelectedItem))
	{
		return SelectedItem;
	}

	return Options.Num() > 0 ? Options[0] : FString();
}

void ABarSeatingActor::ExpandGeneratedBounds(UStaticMesh* Mesh, const FTransform& Transform)
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
		Location - FVector(BarSeatingFallbackExtentCm),
		Location + FVector(BarSeatingFallbackExtentCm));
}

void ABarSeatingActor::UpdateSelectionBounds()
{
	if (!SelectionBounds)
	{
		return;
	}

	SelectionBounds->SetWorldScale3D(FVector::OneVector);

	if (!GeneratedBounds.IsValid)
	{
		SelectionBounds->SetRelativeLocation(FVector::ZeroVector);
		SelectionBounds->SetBoxExtent(FVector(BarSeatingFallbackExtentCm));
		return;
	}

	SelectionBounds->SetRelativeLocation(GeneratedBounds.GetCenter());
	SelectionBounds->SetBoxExtent(GeneratedBounds.GetExtent().ComponentMax(FVector(50.0f, 50.0f, 50.0f)));
}

void ABarSeatingActor::AddAISlotSource(const FVector& LocalOffset, const FRotator& LocalRotation)
{
	AISlotSourceLocalOffsets.Add(LocalOffset);
	AISlotSourceLocalRotations.Add(LocalRotation);
}

void ABarSeatingActor::RefreshAISlotPreviewComponents()
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
		RefreshAIAnimationPreviewComponents(TArray<FVector>());
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

	RefreshAIAnimationPreviewComponents(PreviewOffsets);
#endif
}

void ABarSeatingActor::RefreshAIAnimationPreviewComponents(const TArray<FVector>& PreviewOffsets)
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

void ABarSeatingActor::RefreshAmbientAttendeeComponents(const TArray<FVector>& PreviewOffsets, const TArray<FRotator>& PreviewRotations)
{
	if (!SceneRoot || !bSpawnAmbientAttendees || AmbientAttendeeBlueprints.Num() == 0)
	{
		for (UChildActorComponent* AttendeeComponent : AmbientAttendeeComponents)
		{
			if (AttendeeComponent)
			{
				AttendeeComponent->SetVisibility(false, true);
				AttendeeComponent->SetHiddenInGame(true);
			}
		}
		return;
	}

	while (AmbientAttendeeComponents.Num() < PreviewOffsets.Num())
	{
		const FName ComponentName = MakeUniqueObjectName(this, UChildActorComponent::StaticClass(), TEXT("AmbientAttendee"));
		UChildActorComponent* AttendeeComponent = NewObject<UChildActorComponent>(this, ComponentName, RF_Transactional);
		if (!AttendeeComponent)
		{
			break;
		}

		AttendeeComponent->SetupAttachment(SceneRoot);
		AttendeeComponent->RegisterComponent();
		AddInstanceComponent(AttendeeComponent);
		AmbientAttendeeComponents.Add(AttendeeComponent);
	}

	for (int32 Index = 0; Index < AmbientAttendeeComponents.Num(); ++Index)
	{
		UChildActorComponent* AttendeeComponent = AmbientAttendeeComponents[Index];
		if (!AttendeeComponent)
		{
			continue;
		}

		const bool bUseSlot = PreviewOffsets.IsValidIndex(Index);
		AttendeeComponent->SetVisibility(bUseSlot, true);
		AttendeeComponent->SetHiddenInGame(!bUseSlot);
		if (!bUseSlot)
		{
			continue;
		}

		const int32 BlueprintIndex = AmbientAttendeeBlueprints.Num() > 0
			? FMath::Abs(AmbientAttendeeRandomSeed + Index * 9973) % AmbientAttendeeBlueprints.Num()
			: INDEX_NONE;
		UClass* AttendeeClass = BlueprintIndex != INDEX_NONE ? AmbientAttendeeBlueprints[BlueprintIndex].LoadSynchronous() : nullptr;
		AttendeeComponent->SetChildActorClass(AttendeeClass);
		const FVector SlotPos = PreviewOffsets[Index];
		const FVector RadialDir = SlotPos.IsNearlyZero() ? FVector::ZeroVector : -FVector(SlotPos.X, SlotPos.Y, 0.0f).GetSafeNormal();
		AttendeeComponent->SetRelativeLocation(SlotPos + AmbientAttendeeMeshOffsetCm + RadialDir * AmbientAttendeeRadialOffsetCm);
		AttendeeComponent->SetRelativeRotation((PreviewRotations.IsValidIndex(Index) ? PreviewRotations[Index] : FRotator::ZeroRotator) + AmbientAttendeeMeshRotation);
		AttendeeComponent->SetRelativeScale3D(AmbientAttendeeMeshScale);

		if (AttendeeClass && !AttendeeComponent->GetChildActor())
		{
			AttendeeComponent->CreateChildActor();
		}

		const int32 AnimationIndex = AmbientAttendeeAnimations.Num() > 0
			? FMath::Abs(AmbientAttendeeRandomSeed + Index * 7919) % AmbientAttendeeAnimations.Num()
			: INDEX_NONE;
		UAnimationAsset* Animation = AnimationIndex != INDEX_NONE ? AmbientAttendeeAnimations[AnimationIndex].LoadSynchronous() : nullptr;
		if (UWorld* World = GetWorld(); World && World->IsGameWorld())
		{
			TWeakObjectPtr<ABarSeatingActor> WeakThis(this);
			TWeakObjectPtr<UChildActorComponent> WeakAttendeeComponent(AttendeeComponent);
			TWeakObjectPtr<UAnimationAsset> WeakAnimation(Animation);
			World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([WeakThis, WeakAttendeeComponent, WeakAnimation]()
			{
				if (ABarSeatingActor* StrongThis = WeakThis.Get())
				{
					StrongThis->PlayAmbientAttendeeAnimation(WeakAttendeeComponent.Get(), WeakAnimation.Get());
				}
			}));
		}
		else
		{
			PlayAmbientAttendeeAnimation(AttendeeComponent, Animation);
		}
	}
}

void ABarSeatingActor::PlayAmbientAttendeeAnimation(UChildActorComponent* AttendeeComponent, UAnimationAsset* Animation) const
{
	if (!AttendeeComponent || !Animation)
	{
		return;
	}

	AActor* AttendeeActor = AttendeeComponent->GetChildActor();
	if (!AttendeeActor)
	{
		return;
	}

	ApplyFullMetaHumanAnimation(AttendeeActor, Animation);
}
