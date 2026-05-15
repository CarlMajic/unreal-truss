#include "AudioGroundLineArrayActor.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Modules/ModuleManager.h"
#include "Sound/SoundAttenuation.h"

namespace
{
constexpr TCHAR LineArraySubPrefix[] = TEXT("AudioLineArraySub");
constexpr TCHAR LineArrayPolePrefix[] = TEXT("AudioLineArrayPole");
constexpr TCHAR LineArrayTelescopingPrefix[] = TEXT("AudioLineArrayTelescoping");
constexpr TCHAR LineArraySpeakerPrefix[] = TEXT("AudioLineArraySpeaker");
constexpr float CmPerInch = 2.54f;
constexpr float CmPerFoot = 30.48f;
}

AAudioGroundLineArrayActor::AAudioGroundLineArrayActor()
{
	PrimaryActorTick.bCanEverTick = false;

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
	SelectionBounds->ShapeColor = FColor::Cyan;

	AudioEmitterComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioEmitter"));
	AudioEmitterComponent->SetupAttachment(SceneRoot);
	AudioEmitterComponent->bAutoActivate = false;
	AudioEmitterComponent->bAllowSpatialization = true;
	AudioEmitterComponent->bShouldRemainActiveIfDropped = true;

	AudioConePreviewComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AudioConePreview"));
	AudioConePreviewComponent->SetupAttachment(SceneRoot);
	AudioConePreviewComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AudioConePreviewComponent->SetHiddenInGame(true);
	AudioConePreviewComponent->SetVisibility(false);
	AudioConePreviewComponent->bDisallowNanite = true;
	AudioConePreviewComponent->bHiddenInGame = true;
	AudioConePreviewComponent->bIsEditorOnly = true;
	AudioConePreviewComponent->bDrawMeshCollisionIfComplex = true;
	AudioConePreviewComponent->SetStaticMesh(Cast<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cone.Cone")).TryLoad()));
	AudioConePreviewComponent->SetMaterial(0, Cast<UMaterialInterface>(FSoftObjectPath(TEXT("/Engine/EngineDebugMaterials/WireframeMaterial.WireframeMaterial")).TryLoad()));
}

void AAudioGroundLineArrayActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (bBuildOnConstruction)
	{
		RebuildAudioGroundLineArray();
	}
	else
	{
		UpdateAudioEmitter();
	}
}

void AAudioGroundLineArrayActor::BeginPlay()
{
	Super::BeginPlay();

	UpdateAudioEmitter();
	PlayAssignedAudio();
}

#if WITH_EDITOR
void AAudioGroundLineArrayActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RebuildAudioGroundLineArray();
}
#endif

void AAudioGroundLineArrayActor::RebuildAudioGroundLineArray()
{
	ClearGeneratedComponents();

	const float ClampedMaxExtensionInches = FMath::Max(0.0f, MaxPoleExtensionInches);
	const float ClampedExtensionInches = FMath::Clamp(PoleExtensionInches, 0.0f, ClampedMaxExtensionInches);
	CurrentPoleExtensionCm = SupportsTelescopingPole() ? (ClampedExtensionInches * CmPerInch) : 0.0f;

	const FTransform SubTransform(SubPlacementRotation, SubPlacementOffsetCm, SubScale);
	const FTransform PoleTransform(PolePlacementRotation, PolePlacementOffsetCm, PoleScale);
	const FTransform TelescopingTransform(
		TelescopingPlacementRotation,
		TelescopingPlacementOffsetCm + FVector(0.0f, 0.0f, CurrentPoleExtensionCm),
		TelescopingScale);

	FBox Bounds(EForceInit::ForceInit);

	AddMeshPathSet(GetMeshPathsForFolder(ResolveSubAssetFolder()), LineArraySubPrefix, SubTransform, Bounds);

	if (bSubOnly)
	{
		UpdateSelectionBounds(Bounds);
		UpdateAudioEmitter();
		return;
	}

	for (const FSoftObjectPath& MeshPath : GetMeshPathsForFolder(ResolveSupportAssetFolder()))
	{
		const bool bIsTelescopingMesh = SupportsTelescopingPole() && IsTelescopingPoleMesh(MeshPath);
		AddStaticMeshPathInstance(
			MeshPath,
			bIsTelescopingMesh ? LineArrayTelescopingPrefix : LineArrayPolePrefix,
			bIsTelescopingMesh ? TelescopingTransform : PoleTransform,
			Bounds);
	}

	const TArray<FSoftObjectPath> SpeakerMeshPaths = GetMeshPathsForFolder(ResolveSpeakerAssetFolder());
	const int32 ClampedSpeakerCount = FMath::Clamp(SpeakerCount, 1, GetMaxSpeakerCount());
	const float ResolvedSpeakerSpacingCm = GetSpeakerSpacingCm();
	for (int32 SpeakerIndex = 0; SpeakerIndex < ClampedSpeakerCount; ++SpeakerIndex)
	{
		const FVector SpeakerLocation = SpeakerPlacementOffsetCm +
			FVector(0.0f, 0.0f, CurrentPoleExtensionCm + (SpeakerIndex * ResolvedSpeakerSpacingCm));
		const FTransform SpeakerTransform(SpeakerPlacementRotation, SpeakerLocation, SpeakerScale);
		AddMeshPathSet(SpeakerMeshPaths, LineArraySpeakerPrefix, SpeakerTransform, Bounds);
	}

	UpdateSelectionBounds(Bounds);
	UpdateAudioEmitter();
}

void AAudioGroundLineArrayActor::SetSelectionHighlighted(bool bHighlighted)
{
	if (SelectionBounds)
	{
		SelectionBounds->SetHiddenInGame(!bHighlighted);
	}
}

void AAudioGroundLineArrayActor::ApplyBuildDefinition(const FAudioGroundLineArrayBuildDefinition& Definition, bool bRebuildNow)
{
	LineArrayModel = Definition.LineArrayModel;
	bSubOnly = Definition.bSubOnly;
	SpeakerCount = FMath::Clamp(Definition.SpeakerCount, 1, GetMaxSpeakerCount());
	PoleExtensionInches = FMath::Clamp(Definition.PoleExtensionInches, 0.0f, FMath::Max(0.0f, MaxPoleExtensionInches));

	if (bRebuildNow)
	{
		RebuildAudioGroundLineArray();
	}
}

FAudioGroundLineArrayBuildDefinition AAudioGroundLineArrayActor::GetBuildDefinition() const
{
	FAudioGroundLineArrayBuildDefinition Definition;
	Definition.LineArrayModel = LineArrayModel;
	Definition.bSubOnly = bSubOnly;
	Definition.SpeakerCount = SpeakerCount;
	Definition.PoleExtensionInches = PoleExtensionInches;
	return Definition;
}

void AAudioGroundLineArrayActor::PlayAssignedAudio()
{
	UpdateAudioEmitter();
	if (AudioEmitterComponent && AudioEmitterComponent->Sound)
	{
		AudioEmitterComponent->Play();
	}
}

void AAudioGroundLineArrayActor::StopAssignedAudio()
{
	if (AudioEmitterComponent)
	{
		AudioEmitterComponent->Stop();
	}
}

void AAudioGroundLineArrayActor::SetAudioSource(TSoftObjectPtr<USoundBase> InAudioSource, bool bRestartIfPlaying)
{
	const bool bWasPlaying = AudioEmitterComponent && AudioEmitterComponent->IsPlaying();
	AudioSource = InAudioSource;
	UpdateAudioEmitter();

	if (AudioEmitterComponent && bRestartIfPlaying)
	{
		if (AudioEmitterComponent->Sound && (bWasPlaying || bAutoPlayAudio))
		{
			AudioEmitterComponent->Play();
		}
		else if (!AudioEmitterComponent->Sound)
		{
			AudioEmitterComponent->Stop();
		}
	}
}

TArray<FSoftObjectPath> AAudioGroundLineArrayActor::GetMeshPathsForFolder(const FString& AssetFolderPath) const
{
	TArray<FSoftObjectPath> MeshPaths;
	if (AssetFolderPath.IsEmpty())
	{
		return MeshPaths;
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> Assets;
	AssetRegistryModule.Get().GetAssetsByPath(*AssetFolderPath, Assets, false);

	Assets.StableSort([](const FAssetData& A, const FAssetData& B)
	{
		return A.AssetName.LexicalLess(B.AssetName);
	});

	for (const FAssetData& AssetData : Assets)
	{
		if (AssetData.AssetClassPath == UStaticMesh::StaticClass()->GetClassPathName())
		{
			MeshPaths.Add(AssetData.ToSoftObjectPath());
		}
	}

	return MeshPaths;
}

UInstancedStaticMeshComponent* AAudioGroundLineArrayActor::FindOrCreateMeshBucket(UStaticMesh* StaticMesh, const TCHAR* Prefix)
{
	if (!StaticMesh)
	{
		return nullptr;
	}

	const FString BucketKey = FString::Printf(TEXT("%s_%s"), Prefix, *StaticMesh->GetPathName());
	const uint32 PathHash = FCrc::StrCrc32(*BucketKey);
	const FName ComponentName(*FString::Printf(TEXT("%s_%s_%u"), Prefix, *StaticMesh->GetName(), PathHash));

	if (UInstancedStaticMeshComponent* ExistingComponent = FindGeneratedMeshComponentByName(ComponentName))
	{
		ExistingComponent->SetStaticMesh(StaticMesh);
		ExistingComponent->bDisallowNanite = true;
		ExistingComponent->MarkRenderStateDirty();
		ExistingComponent->SetVisibility(true);
		ExistingComponent->SetHiddenInGame(false);
		if (!GeneratedMeshComponents.Contains(ExistingComponent))
		{
			GeneratedMeshComponents.Add(ExistingComponent);
		}
		return ExistingComponent;
	}

	UInstancedStaticMeshComponent* MeshComponent = NewObject<UInstancedStaticMeshComponent>(this, ComponentName);
	if (!MeshComponent)
	{
		return nullptr;
	}

	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->RegisterComponent();
	AddInstanceComponent(MeshComponent);
	GeneratedMeshComponents.Add(MeshComponent);

	MeshComponent->bDisallowNanite = true;
	MeshComponent->SetStaticMesh(StaticMesh);
	return MeshComponent;
}

UInstancedStaticMeshComponent* AAudioGroundLineArrayActor::FindGeneratedMeshComponentByName(const FName& ComponentName) const
{
	for (UInstancedStaticMeshComponent* MeshComponent : GeneratedMeshComponents)
	{
		if (MeshComponent && MeshComponent->GetFName() == ComponentName)
		{
			return MeshComponent;
		}
	}

	return FindObjectFast<UInstancedStaticMeshComponent>(const_cast<AAudioGroundLineArrayActor*>(this), ComponentName);
}

void AAudioGroundLineArrayActor::ClearGeneratedComponents()
{
	TInlineComponentArray<UInstancedStaticMeshComponent*> ExistingInstanceComponents(this);
	for (UInstancedStaticMeshComponent* MeshComponent : ExistingInstanceComponents)
	{
		if (!MeshComponent)
		{
			continue;
		}

		if ((MeshComponent->GetName().StartsWith(LineArraySubPrefix) ||
			MeshComponent->GetName().StartsWith(LineArrayPolePrefix) ||
			MeshComponent->GetName().StartsWith(LineArrayTelescopingPrefix) ||
			MeshComponent->GetName().StartsWith(LineArraySpeakerPrefix)) &&
			!GeneratedMeshComponents.Contains(MeshComponent))
		{
			GeneratedMeshComponents.Add(MeshComponent);
		}
	}

	for (UInstancedStaticMeshComponent* MeshComponent : GeneratedMeshComponents)
	{
		if (MeshComponent)
		{
			MeshComponent->ClearInstances();
			MeshComponent->SetVisibility(false);
			MeshComponent->SetHiddenInGame(true);
		}
	}
}

void AAudioGroundLineArrayActor::AddMeshPathSet(const TArray<FSoftObjectPath>& MeshPaths, const TCHAR* Prefix, const FTransform& InstanceTransform, FBox& Bounds)
{
	for (const FSoftObjectPath& MeshPath : MeshPaths)
	{
		AddStaticMeshPathInstance(MeshPath, Prefix, InstanceTransform, Bounds);
	}
}

void AAudioGroundLineArrayActor::AddStaticMeshPathInstance(const FSoftObjectPath& MeshPath, const TCHAR* Prefix, const FTransform& InstanceTransform, FBox& Bounds)
{
	UStaticMesh* StaticMesh = Cast<UStaticMesh>(MeshPath.TryLoad());
	AddStaticMeshInstance(StaticMesh, Prefix, InstanceTransform, Bounds);
}

void AAudioGroundLineArrayActor::AddStaticMeshInstance(UStaticMesh* StaticMesh, const TCHAR* Prefix, const FTransform& InstanceTransform, FBox& Bounds)
{
	UInstancedStaticMeshComponent* MeshComponent = FindOrCreateMeshBucket(StaticMesh, Prefix);
	if (!MeshComponent)
	{
		return;
	}

	MeshComponent->AddInstance(InstanceTransform);
	AddTransformedMeshBounds(StaticMesh, InstanceTransform, Bounds);
}

void AAudioGroundLineArrayActor::AddTransformedMeshBounds(UStaticMesh* StaticMesh, const FTransform& InstanceTransform, FBox& Bounds) const
{
	if (!StaticMesh)
	{
		return;
	}

	const FBox MeshBox = StaticMesh->GetBoundingBox();
	for (int32 CornerIndex = 0; CornerIndex < 8; ++CornerIndex)
	{
		const FVector Corner(
			(CornerIndex & 1) ? MeshBox.Max.X : MeshBox.Min.X,
			(CornerIndex & 2) ? MeshBox.Max.Y : MeshBox.Min.Y,
			(CornerIndex & 4) ? MeshBox.Max.Z : MeshBox.Min.Z);
		Bounds += InstanceTransform.TransformPosition(Corner);
	}
}

void AAudioGroundLineArrayActor::UpdateSelectionBounds(const FBox& Bounds)
{
	if (!SelectionBounds)
	{
		return;
	}

	if (!Bounds.IsValid)
	{
		SelectionBounds->SetBoxExtent(FVector(50.0f, 50.0f, 50.0f));
		SelectionBounds->SetRelativeLocation(FVector::ZeroVector);
		return;
	}

	SelectionBounds->SetWorldScale3D(FVector::OneVector);
	SelectionBounds->SetRelativeLocation(Bounds.GetCenter());
	SelectionBounds->SetBoxExtent(Bounds.GetExtent().ComponentMax(FVector(50.0f, 50.0f, 50.0f)));
}

bool AAudioGroundLineArrayActor::IsTelescopingPoleMesh(const FSoftObjectPath& MeshPath) const
{
	return MeshPath.GetAssetName().Equals(TEXT("Top"));
}

bool AAudioGroundLineArrayActor::SupportsTelescopingPole() const
{
	return LineArrayModel == EAudioGroundLineArrayModel::MLA_Mini ||
		LineArrayModel == EAudioGroundLineArrayModel::Custom;
}

int32 AAudioGroundLineArrayActor::GetMaxSpeakerCount() const
{
	return LineArrayModel == EAudioGroundLineArrayModel::MLA_Compact ? 6 : 12;
}

float AAudioGroundLineArrayActor::GetSpeakerSpacingCm() const
{
	return LineArrayModel == EAudioGroundLineArrayModel::MLA_Compact
		? CompactSpeakerSpacingCm
		: SpeakerSpacingCm;
}

FString AAudioGroundLineArrayActor::ResolveSubAssetFolder() const
{
	switch (LineArrayModel)
	{
	case EAudioGroundLineArrayModel::MLA_Compact:
		return TEXT("/Game/Majic_Gear/Audio/MLA_Compact_Sub/StaticMeshes");
	case EAudioGroundLineArrayModel::Custom:
		return SubAssetFolder;
	case EAudioGroundLineArrayModel::MLA_Mini:
	default:
		return TEXT("/Game/Majic_Gear/Audio/MLA_Mini_Sub/StaticMeshes");
	}
}

FString AAudioGroundLineArrayActor::ResolveSupportAssetFolder() const
{
	switch (LineArrayModel)
	{
	case EAudioGroundLineArrayModel::MLA_Compact:
		return TEXT("/Game/Majic_Gear/Audio/MLA_Compact_Flybar/StaticMeshes");
	case EAudioGroundLineArrayModel::Custom:
		return PoleAssetFolder;
	case EAudioGroundLineArrayModel::MLA_Mini:
	default:
		return TEXT("/Game/Majic_Gear/Audio/MLS_Mini_Pole_Mount/StaticMeshes");
	}
}

FString AAudioGroundLineArrayActor::ResolveSpeakerAssetFolder() const
{
	switch (LineArrayModel)
	{
	case EAudioGroundLineArrayModel::MLA_Compact:
		return TEXT("/Game/Majic_Gear/Audio/MLA_Compact_Speaker/StaticMeshes");
	case EAudioGroundLineArrayModel::Custom:
		return SpeakerAssetFolder;
	case EAudioGroundLineArrayModel::MLA_Mini:
	default:
		return TEXT("/Game/Majic_Gear/Audio/MLS_Mini_Speaker/StaticMeshes");
	}
}

void AAudioGroundLineArrayActor::UpdateAudioEmitter()
{
	if (!AudioEmitterComponent)
	{
		return;
	}

	const int32 ClampedSpeakerCount = FMath::Clamp(SpeakerCount, 1, GetMaxSpeakerCount());
	const float SpeakerStackCenterOffsetCm = bSubOnly ? 0.0f : ((ClampedSpeakerCount - 1) * GetSpeakerSpacingCm() * 0.5f);
	const FVector BaseAudioLocation = bSubOnly
		? SubPlacementOffsetCm
		: SpeakerPlacementOffsetCm + FVector(0.0f, 0.0f, CurrentPoleExtensionCm + SpeakerStackCenterOffsetCm);

	if (!AudioSource.IsNull())
	{
		USoundBase* ResolvedAudioSource = AudioSource.LoadSynchronous();
		if (ResolvedAudioSource)
		{
			ResolvedAudioSource->VirtualizationMode = EVirtualizationMode::PlayWhenSilent;
			AudioEmitterComponent->SetSound(ResolvedAudioSource);
		}
	}
	else
	{
		AudioEmitterComponent->SetSound(nullptr);
	}
	if (AudioEmitterComponent->Sound)
	{
		AudioEmitterComponent->Sound->VirtualizationMode = EVirtualizationMode::PlayWhenSilent;
	}
	AudioEmitterComponent->SetAttenuationSettings(AudioAttenuation);
	AudioEmitterComponent->bAllowSpatialization = bSpatializeAudio;
	float ResolvedFullVolumeRadiusFt = AudioFullVolumeRadiusFt;
	float ResolvedConeAngleDegrees = AudioConeAngleDegrees;
	bool bUseConeForCurrentMode = bUseDirectionalAudioCone && !bSubOnly;
	if (bUseBuiltInAudioAttenuation && !AudioAttenuation)
	{
		ResolvedFullVolumeRadiusFt = bUseLineArrayModelAudioProfile ? GetProfileAudioFullVolumeRadiusFt() : AudioFullVolumeRadiusFt;
		const float ResolvedFalloffDistanceFt = bUseLineArrayModelAudioProfile ? GetProfileAudioFalloffDistanceFt() : AudioFalloffDistanceFt;
		ResolvedConeAngleDegrees = bUseLineArrayModelAudioProfile ? GetProfileAudioConeAngleDegrees() : AudioConeAngleDegrees;
		const float ResolvedConeFalloffAngleDegrees = bUseLineArrayModelAudioProfile ? GetProfileAudioConeFalloffAngleDegrees() : AudioConeFalloffAngleDegrees;
		bUseConeForCurrentMode = bUseDirectionalAudioCone && !bSubOnly;

		FSoundAttenuationSettings AttenuationSettings;
		AttenuationSettings.bAttenuate = true;
		AttenuationSettings.bSpatialize = bSpatializeAudio;
		AttenuationSettings.DistanceAlgorithm = EAttenuationDistanceModel::Logarithmic;
		AttenuationSettings.AttenuationShape = bUseConeForCurrentMode ? EAttenuationShape::Cone : EAttenuationShape::Sphere;
		AttenuationSettings.AttenuationShapeExtents = bUseConeForCurrentMode
			? FVector(
				FMath::Max(0.0f, ResolvedFullVolumeRadiusFt) * CmPerFoot,
				FMath::Clamp(ResolvedConeAngleDegrees, 1.0f, 360.0f),
				FMath::Clamp(ResolvedConeFalloffAngleDegrees, 0.0f, 360.0f))
			: FVector(FMath::Max(0.0f, ResolvedFullVolumeRadiusFt) * CmPerFoot, 0.0f, 0.0f);
		AttenuationSettings.FalloffDistance = FMath::Max(1.0f, ResolvedFalloffDistanceFt) * CmPerFoot;
		AttenuationSettings.ConeSphereRadius = 0.0f;
		AttenuationSettings.ConeSphereFalloffDistance = 0.0f;
		AttenuationSettings.SpatializationAlgorithm = SPATIALIZATION_Default;
		AttenuationSettings.bApplyNormalizationToStereoSounds = true;
		AttenuationSettings.StereoSpread = 0.0f;
		AudioEmitterComponent->SetOverrideAttenuation(true);
		AudioEmitterComponent->AdjustAttenuation(AttenuationSettings);
	}
	else
	{
		AudioEmitterComponent->SetOverrideAttenuation(false);
	}
	AudioEmitterComponent->SetRelativeLocation(BaseAudioLocation + AudioSourceOffsetCm);
	AudioEmitterComponent->SetRelativeRotation(AudioConeDirectionRotation);
	UpdateAudioConePreview(ResolvedFullVolumeRadiusFt, ResolvedConeAngleDegrees, bUseConeForCurrentMode);
}

float AAudioGroundLineArrayActor::GetProfileAudioFullVolumeRadiusFt() const
{
	if (bSubOnly)
	{
		return LineArrayModel == EAudioGroundLineArrayModel::MLA_Compact ? 20.0f : 16.0f;
	}

	switch (LineArrayModel)
	{
	case EAudioGroundLineArrayModel::MLA_Compact:
		return 22.0f;
	case EAudioGroundLineArrayModel::Custom:
		return AudioFullVolumeRadiusFt;
	case EAudioGroundLineArrayModel::MLA_Mini:
	default:
		return 18.0f;
	}
}

float AAudioGroundLineArrayActor::GetProfileAudioFalloffDistanceFt() const
{
	if (bSubOnly)
	{
		return LineArrayModel == EAudioGroundLineArrayModel::MLA_Compact ? 180.0f : 130.0f;
	}

	switch (LineArrayModel)
	{
	case EAudioGroundLineArrayModel::MLA_Compact:
		return 210.0f;
	case EAudioGroundLineArrayModel::Custom:
		return AudioFalloffDistanceFt;
	case EAudioGroundLineArrayModel::MLA_Mini:
	default:
		return 150.0f;
	}
}

float AAudioGroundLineArrayActor::GetProfileAudioConeAngleDegrees() const
{
	switch (LineArrayModel)
	{
	case EAudioGroundLineArrayModel::MLA_Compact:
		return 100.0f;
	case EAudioGroundLineArrayModel::Custom:
		return AudioConeAngleDegrees;
	case EAudioGroundLineArrayModel::MLA_Mini:
	default:
		return 100.0f;
	}
}

float AAudioGroundLineArrayActor::GetProfileAudioConeFalloffAngleDegrees() const
{
	switch (LineArrayModel)
	{
	case EAudioGroundLineArrayModel::MLA_Compact:
		return 30.0f;
	case EAudioGroundLineArrayModel::Custom:
		return AudioConeFalloffAngleDegrees;
	case EAudioGroundLineArrayModel::MLA_Mini:
	default:
		return 25.0f;
	}
}

void AAudioGroundLineArrayActor::UpdateAudioConePreview(float FullVolumeRadiusFt, float ConeAngleDegrees, bool bUseCone)
{
	if (!AudioConePreviewComponent || !AudioEmitterComponent)
	{
		return;
	}

	if (!bShowAudioConePreview || !bUseCone)
	{
		AudioConePreviewComponent->SetVisibility(false);
		return;
	}

	const float ConeLengthCm = FMath::Max(1.0f, FullVolumeRadiusFt) * CmPerFoot;
	const float ConeRadiusCm = ConeLengthCm * FMath::Tan(FMath::DegreesToRadians(FMath::Clamp(ConeAngleDegrees, 1.0f, 179.0f) * 0.5f));

	AudioConePreviewComponent->SetRelativeLocation(AudioEmitterComponent->GetRelativeLocation());
	AudioConePreviewComponent->SetRelativeRotation(AudioEmitterComponent->GetRelativeRotation() + AudioConePreviewRotationOffset);
	AudioConePreviewComponent->SetRelativeScale3D(FVector(ConeRadiusCm / 50.0f, ConeRadiusCm / 50.0f, ConeLengthCm / 100.0f));
	AudioConePreviewComponent->SetVisibility(true);
}
