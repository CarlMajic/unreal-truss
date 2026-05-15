#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AudioGroundLineArrayActor.generated.h"

class UBoxComponent;
class UAudioComponent;
class UInstancedStaticMeshComponent;
class USoundAttenuation;
class USoundBase;
class UStaticMeshComponent;
class UStaticMesh;

UENUM(BlueprintType)
enum class EAudioGroundLineArrayModel : uint8
{
	MLA_Mini UMETA(DisplayName = "MLA Mini"),
	MLA_Compact UMETA(DisplayName = "MLA Compact"),
	Custom UMETA(DisplayName = "Custom Folders")
};

USTRUCT(BlueprintType)
struct FAudioGroundLineArrayBuildDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	EAudioGroundLineArrayModel LineArrayModel = EAudioGroundLineArrayModel::MLA_Mini;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	bool bSubOnly = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio", meta = (ClampMin = "1", ClampMax = "12"))
	int32 SpeakerCount = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio", meta = (DisplayName = "Pole Extension (in)", ClampMin = "0.0", ClampMax = "17.0"))
	float PoleExtensionInches = 0.0f;
};

UCLASS(BlueprintType)
class MAJICTRUSSRUNTIME_API AAudioGroundLineArrayActor : public AActor
{
	GENERATED_BODY()

public:
	AAudioGroundLineArrayActor();

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
	EAudioGroundLineArrayModel LineArrayModel = EAudioGroundLineArrayModel::MLA_Mini;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	bool bSubOnly = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Assets")
	FString SubAssetFolder = TEXT("/Game/Majic_Gear/Audio/MLA_Mini_Sub/StaticMeshes");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Assets", meta = (DisplayName = "Pole/Flybar Asset Folder"))
	FString PoleAssetFolder = TEXT("/Game/Majic_Gear/Audio/MLS_Mini_Pole_Mount/StaticMeshes");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Assets")
	FString SpeakerAssetFolder = TEXT("/Game/Majic_Gear/Audio/MLS_Mini_Speaker/StaticMeshes");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Sizing", meta = (ClampMin = "1", ClampMax = "12"))
	int32 SpeakerCount = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Sizing", meta = (DisplayName = "Mini Speaker Spacing (cm)", Units = "cm"))
	float SpeakerSpacingCm = 19.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Sizing", meta = (DisplayName = "Compact Speaker Spacing (cm)", Units = "cm"))
	float CompactSpeakerSpacingCm = 28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Sizing", meta = (DisplayName = "Pole Extension (in)", ClampMin = "0.0", ClampMax = "17.0"))
	float PoleExtensionInches = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Sizing", meta = (DisplayName = "Max Pole Extension (in)", ClampMin = "0.0"))
	float MaxPoleExtensionInches = 17.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio|Debug", meta = (Units = "cm"))
	float CurrentPoleExtensionCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Placement", meta = (Units = "cm"))
	FVector SubPlacementOffsetCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Placement")
	FRotator SubPlacementRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Placement")
	FVector SubScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Placement", meta = (Units = "cm"))
	FVector PolePlacementOffsetCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Placement")
	FRotator PolePlacementRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Placement")
	FVector PoleScale = FVector::OneVector;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Sound", meta = (DisplayName = "Use Line Array Model Audio Profile"))
	bool bUseLineArrayModelAudioProfile = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Sound", meta = (DisplayName = "Directional Audio Cone"))
	bool bUseDirectionalAudioCone = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Sound", meta = (DisplayName = "Full Volume Radius (ft)", ClampMin = "0.0"))
	float AudioFullVolumeRadiusFt = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Sound", meta = (DisplayName = "Falloff Distance (ft)", ClampMin = "1.0"))
	float AudioFalloffDistanceFt = 140.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Sound", meta = (DisplayName = "Cone Angle (deg)", ClampMin = "1.0", ClampMax = "360.0"))
	float AudioConeAngleDegrees = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Sound", meta = (DisplayName = "Cone Falloff Angle (deg)", ClampMin = "0.0", ClampMax = "360.0"))
	float AudioConeFalloffAngleDegrees = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Sound|Debug", meta = (DisplayName = "Show Audio Cone Preview"))
	bool bShowAudioConePreview = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Sound|Debug")
	FRotator AudioConePreviewRotationOffset = FRotator(90.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Sound", meta = (Units = "cm"))
	FVector AudioSourceOffsetCm = FVector(0.0f, 245.0f, 120.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Sound", meta = (DisplayName = "Audio Direction Rotation"))
	FRotator AudioSourceRotation = FRotator(0.0f, 90.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Sound|Debug", meta = (DisplayName = "Deprecated Audio Cone Direction Rotation"))
	FRotator AudioConeDirectionRotation = FRotator(0.0f, 180.0f, 0.0f);

	UFUNCTION(BlueprintCallable, Category = "Audio")
	void RebuildAudioGroundLineArray();

	UFUNCTION(BlueprintCallable, Category = "Audio")
	void ApplyBuildDefinition(const FAudioGroundLineArrayBuildDefinition& Definition, bool bRebuildNow = true);

	UFUNCTION(BlueprintPure, Category = "Audio")
	FAudioGroundLineArrayBuildDefinition GetBuildDefinition() const;

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
	void AddMeshPathSet(const TArray<FSoftObjectPath>& MeshPaths, const TCHAR* Prefix, const FTransform& InstanceTransform, FBox& Bounds);
	void AddStaticMeshPathInstance(const FSoftObjectPath& MeshPath, const TCHAR* Prefix, const FTransform& InstanceTransform, FBox& Bounds);
	void AddStaticMeshInstance(UStaticMesh* StaticMesh, const TCHAR* Prefix, const FTransform& InstanceTransform, FBox& Bounds);
	void AddTransformedMeshBounds(UStaticMesh* StaticMesh, const FTransform& InstanceTransform, FBox& Bounds) const;
	void UpdateSelectionBounds(const FBox& Bounds);
	bool IsTelescopingPoleMesh(const FSoftObjectPath& MeshPath) const;
	bool SupportsTelescopingPole() const;
	int32 GetMaxSpeakerCount() const;
	float GetSpeakerSpacingCm() const;
	FString ResolveSubAssetFolder() const;
	FString ResolveSupportAssetFolder() const;
	FString ResolveSpeakerAssetFolder() const;
	void UpdateAudioEmitter();
	float GetProfileAudioFullVolumeRadiusFt() const;
	float GetProfileAudioFalloffDistanceFt() const;
	float GetProfileAudioConeAngleDegrees() const;
	float GetProfileAudioConeFalloffAngleDegrees() const;
	void UpdateAudioConePreview(float FullVolumeRadiusFt, float ConeAngleDegrees, bool bUseCone);
};
