#include "AudioGroundSpeakerActor.h"

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
constexpr TCHAR AudioStandPrefix[] = TEXT("AudioGroundStand");
constexpr TCHAR AudioTelescopingPrefix[] = TEXT("AudioGroundTelescoping");
constexpr TCHAR AudioSpeakerPrefix[] = TEXT("AudioGroundSpeaker");
constexpr float CmPerFoot = 30.48f;
}

AAudioGroundSpeakerActor::AAudioGroundSpeakerActor()
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

void AAudioGroundSpeakerActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (bBuildOnConstruction)
	{
		RebuildAudioGroundSpeaker();
	}
	else
	{
		UpdateAudioEmitter();
	}
}

void AAudioGroundSpeakerActor::BeginPlay()
{
	Super::BeginPlay();

	UpdateAudioEmitter();
	PlayAssignedAudio();
}

#if WITH_EDITOR
void AAudioGroundSpeakerActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RebuildAudioGroundSpeaker();
}
#endif

void AAudioGroundSpeakerActor::RebuildAudioGroundSpeaker()
{
	ClearGeneratedComponents();

	const float ClampedMaxExtensionFt = FMath::Max(0.0f, MaxStandExtensionFt);
	const float ClampedExtensionFt = FMath::Clamp(StandExtensionFt, 0.0f, ClampedMaxExtensionFt);
	CurrentStandExtensionCm = -(ClampedExtensionFt * CmPerFoot);

	const FTransform StandTransform(StandPlacementRotation, StandPlacementOffsetCm, StandScale);
	const FTransform TelescopingTransform(
		TelescopingPlacementRotation,
		TelescopingPlacementOffsetCm + FVector(0.0f, 0.0f, CurrentStandExtensionCm),
		TelescopingScale);
	const FTransform SpeakerTransform(
		SpeakerPlacementRotation,
		SpeakerPlacementOffsetCm + FVector(0.0f, 0.0f, CurrentStandExtensionCm),
		SpeakerScale);

	FBox Bounds(EForceInit::ForceInit);

	for (const FSoftObjectPath& MeshPath : GetMeshPathsForFolder(StandAssetFolder))
	{
		AddStaticMeshPathInstance(
			MeshPath,
			IsTelescopingStandMesh(MeshPath) ? AudioTelescopingPrefix : AudioStandPrefix,
			IsTelescopingStandMesh(MeshPath) ? TelescopingTransform : StandTransform,
			Bounds);
	}

	for (const FSoftObjectPath& MeshPath : GetMeshPathsForFolder(GetSpeakerAssetFolder()))
	{
		AddStaticMeshPathInstance(MeshPath, AudioSpeakerPrefix, SpeakerTransform, Bounds);
	}

	UpdateSelectionBounds(Bounds);
	UpdateAudioEmitter();
}

void AAudioGroundSpeakerActor::SetSelectionHighlighted(bool bHighlighted)
{
	if (SelectionBounds)
	{
		SelectionBounds->SetHiddenInGame(!bHighlighted);
	}
}

void AAudioGroundSpeakerActor::ApplyBuildDefinition(const FAudioGroundSpeakerBuildDefinition& Definition, bool bRebuildNow)
{
	SpeakerModel = Definition.SpeakerModel;
	StandExtensionFt = FMath::Clamp(Definition.StandExtensionFt, 0.0f, FMath::Max(0.0f, MaxStandExtensionFt));

	if (bRebuildNow)
	{
		RebuildAudioGroundSpeaker();
	}
}

FAudioGroundSpeakerBuildDefinition AAudioGroundSpeakerActor::GetBuildDefinition() const
{
	FAudioGroundSpeakerBuildDefinition Definition;
	Definition.SpeakerModel = SpeakerModel;
	Definition.StandExtensionFt = StandExtensionFt;
	return Definition;
}

void AAudioGroundSpeakerActor::PlayAssignedAudio()
{
	UpdateAudioEmitter();
	if (AudioEmitterComponent && AudioEmitterComponent->Sound)
	{
		AudioEmitterComponent->Play();
	}
}

void AAudioGroundSpeakerActor::StopAssignedAudio()
{
	if (AudioEmitterComponent)
	{
		AudioEmitterComponent->Stop();
	}
}

void AAudioGroundSpeakerActor::SetAudioSource(TSoftObjectPtr<USoundBase> InAudioSource, bool bRestartIfPlaying)
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

TArray<FSoftObjectPath> AAudioGroundSpeakerActor::GetMeshPathsForFolder(const FString& AssetFolderPath) const
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

UInstancedStaticMeshComponent* AAudioGroundSpeakerActor::FindOrCreateMeshBucket(UStaticMesh* StaticMesh, const TCHAR* Prefix)
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

UInstancedStaticMeshComponent* AAudioGroundSpeakerActor::FindGeneratedMeshComponentByName(const FName& ComponentName) const
{
	for (UInstancedStaticMeshComponent* MeshComponent : GeneratedMeshComponents)
	{
		if (MeshComponent && MeshComponent->GetFName() == ComponentName)
		{
			return MeshComponent;
		}
	}

	return FindObjectFast<UInstancedStaticMeshComponent>(const_cast<AAudioGroundSpeakerActor*>(this), ComponentName);
}

void AAudioGroundSpeakerActor::ClearGeneratedComponents()
{
	TInlineComponentArray<UInstancedStaticMeshComponent*> ExistingInstanceComponents(this);
	for (UInstancedStaticMeshComponent* MeshComponent : ExistingInstanceComponents)
	{
		if (!MeshComponent)
		{
			continue;
		}

		if ((MeshComponent->GetName().StartsWith(AudioStandPrefix) ||
			MeshComponent->GetName().StartsWith(AudioTelescopingPrefix) ||
			MeshComponent->GetName().StartsWith(AudioSpeakerPrefix)) &&
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

void AAudioGroundSpeakerActor::AddStaticMeshPathInstance(const FSoftObjectPath& MeshPath, const TCHAR* Prefix, const FTransform& InstanceTransform, FBox& Bounds)
{
	UStaticMesh* StaticMesh = Cast<UStaticMesh>(MeshPath.TryLoad());
	AddStaticMeshInstance(StaticMesh, Prefix, InstanceTransform, Bounds);
}

void AAudioGroundSpeakerActor::AddStaticMeshInstance(UStaticMesh* StaticMesh, const TCHAR* Prefix, const FTransform& InstanceTransform, FBox& Bounds)
{
	UInstancedStaticMeshComponent* MeshComponent = FindOrCreateMeshBucket(StaticMesh, Prefix);
	if (!MeshComponent)
	{
		return;
	}

	MeshComponent->AddInstance(InstanceTransform);
	AddTransformedMeshBounds(StaticMesh, InstanceTransform, Bounds);
}

void AAudioGroundSpeakerActor::AddTransformedMeshBounds(UStaticMesh* StaticMesh, const FTransform& InstanceTransform, FBox& Bounds) const
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

void AAudioGroundSpeakerActor::UpdateSelectionBounds(const FBox& Bounds)
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

bool AAudioGroundSpeakerActor::IsTelescopingStandMesh(const FSoftObjectPath& MeshPath) const
{
	const FString AssetName = MeshPath.GetAssetName();
	return AssetName.Equals(TEXT("Upper_001")) ||
		AssetName.Equals(TEXT("Pin_001")) ||
		AssetName.Equals(TEXT("Hand_Grip_001"));
}

FString AAudioGroundSpeakerActor::GetSpeakerAssetFolder() const
{
	switch (SpeakerModel)
	{
	case EAudioGroundSpeakerModel::QSC_K10:
		return TEXT("/Game/Majic_Gear/Audio/QSC_K10_Ground/StaticMeshes");
	case EAudioGroundSpeakerModel::QSC_K12:
		return TEXT("/Game/Majic_Gear/Audio/QSC_K12_Ground/StaticMeshes");
	case EAudioGroundSpeakerModel::QSC_KW153:
		return TEXT("/Game/Majic_Gear/Audio/QSC_KW153_Ground/StaticMeshes");
	case EAudioGroundSpeakerModel::CDD_LIVE_15:
		return TEXT("/Game/Majic_Gear/Audio/CDD-LIVE_15_Ground/StaticMeshes");
	case EAudioGroundSpeakerModel::Custom:
		return SpeakerAssetFolder;
	case EAudioGroundSpeakerModel::QSC_K8:
	default:
		return TEXT("/Game/Majic_Gear/Audio/QSC_K8_Ground/StaticMeshes");
	}
}

void AAudioGroundSpeakerActor::UpdateAudioEmitter()
{
	if (!AudioEmitterComponent)
	{
		return;
	}

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
	bool bUseConeForCurrentMode = bUseDirectionalAudioCone;
	if (bUseBuiltInAudioAttenuation && !AudioAttenuation)
	{
		ResolvedFullVolumeRadiusFt = bUseSpeakerModelAudioProfile ? GetProfileAudioFullVolumeRadiusFt() : AudioFullVolumeRadiusFt;
		const float ResolvedFalloffDistanceFt = bUseSpeakerModelAudioProfile ? GetProfileAudioFalloffDistanceFt() : AudioFalloffDistanceFt;
		ResolvedConeAngleDegrees = bUseSpeakerModelAudioProfile ? GetProfileAudioConeAngleDegrees() : AudioConeAngleDegrees;
		const float ResolvedConeFalloffAngleDegrees = bUseSpeakerModelAudioProfile ? GetProfileAudioConeFalloffAngleDegrees() : AudioConeFalloffAngleDegrees;
		bUseConeForCurrentMode = bUseDirectionalAudioCone;

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
	AudioEmitterComponent->SetRelativeLocation(SpeakerPlacementOffsetCm + AudioSourceOffsetCm + FVector(0.0f, 0.0f, CurrentStandExtensionCm));
	AudioEmitterComponent->SetRelativeRotation(AudioConeDirectionRotation);
	UpdateAudioConePreview(ResolvedFullVolumeRadiusFt, ResolvedConeAngleDegrees, bUseConeForCurrentMode);
}

float AAudioGroundSpeakerActor::GetProfileAudioFullVolumeRadiusFt() const
{
	switch (SpeakerModel)
	{
	case EAudioGroundSpeakerModel::QSC_K10:
		return 14.0f;
	case EAudioGroundSpeakerModel::QSC_K12:
		return 16.0f;
	case EAudioGroundSpeakerModel::QSC_KW153:
		return 22.0f;
	case EAudioGroundSpeakerModel::CDD_LIVE_15:
		return 18.0f;
	case EAudioGroundSpeakerModel::Custom:
		return AudioFullVolumeRadiusFt;
	case EAudioGroundSpeakerModel::QSC_K8:
	default:
		return 12.0f;
	}
}

float AAudioGroundSpeakerActor::GetProfileAudioFalloffDistanceFt() const
{
	switch (SpeakerModel)
	{
	case EAudioGroundSpeakerModel::QSC_K10:
		return 110.0f;
	case EAudioGroundSpeakerModel::QSC_K12:
		return 130.0f;
	case EAudioGroundSpeakerModel::QSC_KW153:
		return 180.0f;
	case EAudioGroundSpeakerModel::CDD_LIVE_15:
		return 145.0f;
	case EAudioGroundSpeakerModel::Custom:
		return AudioFalloffDistanceFt;
	case EAudioGroundSpeakerModel::QSC_K8:
	default:
		return 90.0f;
	}
}

float AAudioGroundSpeakerActor::GetProfileAudioConeAngleDegrees() const
{
	switch (SpeakerModel)
	{
	case EAudioGroundSpeakerModel::QSC_K8:
		return 105.0f;
	case EAudioGroundSpeakerModel::QSC_K10:
		return 90.0f;
	case EAudioGroundSpeakerModel::QSC_K12:
	case EAudioGroundSpeakerModel::QSC_KW153:
		return 75.0f;
	case EAudioGroundSpeakerModel::CDD_LIVE_15:
		return 60.0f;
	case EAudioGroundSpeakerModel::Custom:
		return AudioConeAngleDegrees;
	default:
		return 90.0f;
	}
}

float AAudioGroundSpeakerActor::GetProfileAudioConeFalloffAngleDegrees() const
{
	switch (SpeakerModel)
	{
	case EAudioGroundSpeakerModel::QSC_K8:
	case EAudioGroundSpeakerModel::QSC_K10:
	case EAudioGroundSpeakerModel::QSC_K12:
	case EAudioGroundSpeakerModel::QSC_KW153:
		return 20.0f;
	case EAudioGroundSpeakerModel::CDD_LIVE_15:
		return 40.0f;
	case EAudioGroundSpeakerModel::Custom:
		return AudioConeFalloffAngleDegrees;
	default:
		return 25.0f;
	}
}

void AAudioGroundSpeakerActor::UpdateAudioConePreview(float FullVolumeRadiusFt, float ConeAngleDegrees, bool bUseCone)
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
