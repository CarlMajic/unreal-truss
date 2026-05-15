#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AudioGroundSpeakerActor.generated.h"

class UBoxComponent;
class UAudioComponent;
class UInstancedStaticMeshComponent;
class USoundAttenuation;
class USoundBase;
class UStaticMeshComponent;
class UStaticMesh;

UENUM(BlueprintType)
enum class EAudioGroundSpeakerModel : uint8
{
	QSC_K8 UMETA(DisplayName = "QSC K8"),
	QSC_K10 UMETA(DisplayName = "QSC K10"),
	QSC_K12 UMETA(DisplayName = "QSC K12"),
	QSC_KW153 UMETA(DisplayName = "QSC KW153"),
	CDD_LIVE_15 UMETA(DisplayName = "CDD-LIVE 15"),
	Custom UMETA(DisplayName = "Custom Folder")
};

USTRUCT(BlueprintType)
struct FAudioGroundSpeakerBuildDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	EAudioGroundSpeakerModel SpeakerModel = EAudioGroundSpeakerModel::QSC_K8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio", meta = (DisplayName = "Stand Extension (ft)", ClampMin = "0.0", ClampMax = "3.0"))
	float StandExtensionFt = 0.0f;
};

UCLASS(BlueprintType)
class MAJICTRUSSRUNTIME_API AAudioGroundSpeakerActor : public AActor
{
	GENERATED_BODY()

public:
	AAudioGroundSpeakerActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<UBoxComponent> SelectionBounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio|Sound")
	TObjectPtr<UAudioComponent> AudioEmitterComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio|Sound")
	TObjectPtr<UStaticMeshComponent> AudioConePreviewComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	bool bBuildOnConstruction = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Assets")
	FString StandAssetFolder = TEXT("/Game/Majic_Gear/Audio/TS-100_Stand/StaticMeshes");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Assets")
	EAudioGroundSpeakerModel SpeakerModel = EAudioGroundSpeakerModel::QSC_K8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Assets", meta = (EditCondition = "SpeakerModel == EAudioGroundSpeakerModel::Custom"))
	FString SpeakerAssetFolder = TEXT("/Game/Majic_Gear/Audio/QSC_K8_Ground/StaticMeshes");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Sizing", meta = (DisplayName = "Stand Extension (ft)", ClampMin = "0.0", ClampMax = "3.0"))
	float StandExtensionFt = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Sizing", meta = (DisplayName = "Max Stand Extension (ft)", ClampMin = "0.0"))
	float MaxStandExtensionFt = 3.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio|Debug", meta = (Units = "cm"))
	float CurrentStandExtensionCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Placement", meta = (Units = "cm"))
	FVector StandPlacementOffsetCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Placement")
	FRotator StandPlacementRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Placement")
	FVector StandScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Placement", meta = (Units = "cm"))
	FVector TelescopingPlacementOffsetCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Placement")
	FRotator TelescopingPlacementRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Placement")
	FVector TelescopingScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Placement", meta = (Units = "cm"))
	FVector SpeakerPlacementOffsetCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Placement")
	FRotator SpeakerPlacementRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Placement")
	FVector SpeakerScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Sound", meta = (AllowedClasses = "/Script/Engine.SoundBase"))
	TSoftObjectPtr<USoundBase> AudioSource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Sound")
	TObjectPtr<USoundAttenuation> AudioAttenuation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Sound", meta = (DisplayName = "Play Audio On Begin Play"))
	bool bAutoPlayAudio = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Sound")
	bool bSpatializeAudio = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Sound", meta = (DisplayName = "Use Built-In 3D Falloff"))
	bool bUseBuiltInAudioAttenuation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Sound", meta = (DisplayName = "Use Speaker Model Audio Profile"))
	bool bUseSpeakerModelAudioProfile = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Sound", meta = (DisplayName = "Directional Audio Cone"))
	bool bUseDirectionalAudioCone = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Sound", meta = (DisplayName = "Full Volume Radius (ft)", ClampMin = "0.0"))
	float AudioFullVolumeRadiusFt = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Sound", meta = (DisplayName = "Falloff Distance (ft)", ClampMin = "1.0"))
	float AudioFalloffDistanceFt = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Sound", meta = (DisplayName = "Cone Angle (deg)", ClampMin = "1.0", ClampMax = "360.0"))
	float AudioConeAngleDegrees = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Sound", meta = (DisplayName = "Cone Falloff Angle (deg)", ClampMin = "0.0", ClampMax = "360.0"))
	float AudioConeFalloffAngleDegrees = 55.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Sound|Debug", meta = (DisplayName = "Show Audio Cone Preview"))
	bool bShowAudioConePreview = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Sound|Debug")
	FRotator AudioConePreviewRotationOffset = FRotator(90.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Sound", meta = (Units = "cm"))
	FVector AudioSourceOffsetCm = FVector(0.0f, 160.0f, 193.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Sound", meta = (DisplayName = "Audio Direction Rotation"))
	FRotator AudioSourceRotation = FRotator(0.0f, 90.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Sound|Debug", meta = (DisplayName = "Deprecated Audio Cone Direction Rotation"))
	FRotator AudioConeDirectionRotation = FRotator(0.0f, 180.0f, 0.0f);

	UFUNCTION(BlueprintCallable, Category = "Audio")
	void RebuildAudioGroundSpeaker();

	UFUNCTION(BlueprintCallable, Category = "Audio")
	void ApplyBuildDefinition(const FAudioGroundSpeakerBuildDefinition& Definition, bool bRebuildNow = true);

	UFUNCTION(BlueprintPure, Category = "Audio")
	FAudioGroundSpeakerBuildDefinition GetBuildDefinition() const;

	UFUNCTION(BlueprintCallable, Category = "Audio")
	void SetSelectionHighlighted(bool bHighlighted);

	UFUNCTION(BlueprintCallable, Category = "Audio|Sound")
	void PlayAssignedAudio();

	UFUNCTION(BlueprintCallable, Category = "Audio|Sound")
	void StopAssignedAudio();

	UFUNCTION(BlueprintCallable, Category = "Audio|Sound")
	void SetAudioSource(TSoftObjectPtr<USoundBase> InAudioSource, bool bRestartIfPlaying = true);

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> GeneratedMeshComponents;

	TArray<FSoftObjectPath> GetMeshPathsForFolder(const FString& AssetFolderPath) const;
	UInstancedStaticMeshComponent* FindOrCreateMeshBucket(UStaticMesh* StaticMesh, const TCHAR* Prefix);
	UInstancedStaticMeshComponent* FindGeneratedMeshComponentByName(const FName& ComponentName) const;
	void ClearGeneratedComponents();
	void AddStaticMeshPathInstance(const FSoftObjectPath& MeshPath, const TCHAR* Prefix, const FTransform& InstanceTransform, FBox& Bounds);
	void AddStaticMeshInstance(UStaticMesh* StaticMesh, const TCHAR* Prefix, const FTransform& InstanceTransform, FBox& Bounds);
	void AddTransformedMeshBounds(UStaticMesh* StaticMesh, const FTransform& InstanceTransform, FBox& Bounds) const;
	void UpdateSelectionBounds(const FBox& Bounds);
	bool IsTelescopingStandMesh(const FSoftObjectPath& MeshPath) const;
	FString GetSpeakerAssetFolder() const;
	void UpdateAudioEmitter();
	float GetProfileAudioFullVolumeRadiusFt() const;
	float GetProfileAudioFalloffDistanceFt() const;
	float GetProfileAudioConeAngleDegrees() const;
	float GetProfileAudioConeFalloffAngleDegrees() const;
	void UpdateAudioConePreview(float FullVolumeRadiusFt, float ConeAngleDegrees, bool bUseCone);
};
