#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DrapeRunActor.generated.h"

class UBoxComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class USkeletalMesh;
class USkeletalMeshComponent;
class UStaticMesh;

UENUM(BlueprintType)
enum class EDrapeRunHeightPreset : uint8
{
	Ft6 UMETA(DisplayName = "6 ft"),
	Ft8 UMETA(DisplayName = "8 ft"),
	Ft10 UMETA(DisplayName = "10 ft"),
	Ft16 UMETA(DisplayName = "16 ft")
};

UENUM(BlueprintType)
enum class EDrapeRunDrapeMode : uint8
{
	StaticMesh UMETA(DisplayName = "Static Mesh"),
	ChaosCloth UMETA(DisplayName = "Chaos Cloth"),
	MaterialSway UMETA(DisplayName = "Material Sway")
};

USTRUCT(BlueprintType)
struct FDrapeRunBuildDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape", meta = (DisplayName = "Length (ft)", ClampMin = "1.0"))
	float LengthFt = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape", meta = (DisplayName = "Section Length (ft)", ClampMin = "1.0"))
	float SectionLengthFt = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape", meta = (DisplayName = "Height (ft)", ClampMin = "6.0", ClampMax = "18.333"))
	float HeightFt = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape")
	EDrapeRunHeightPreset HeightPreset = EDrapeRunHeightPreset::Ft8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape")
	EDrapeRunDrapeMode DrapeMode = EDrapeRunDrapeMode::StaticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape")
	bool bUseChaosCloth = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape", meta = (ClampMin = "0.1"))
	float Fullness = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape")
	bool bShowHardware = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape")
	bool bCenterOnActor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape")
	TSoftObjectPtr<UMaterialInterface> DrapeMaterialOverride;
};

UCLASS(BlueprintType)
class MAJICTRUSSRUNTIME_API ADrapeRunActor : public AActor
{
	GENERATED_BODY()

public:
	ADrapeRunActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drape")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drape")
	TObjectPtr<UBoxComponent> SelectionBounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape", meta = (DisplayName = "Length (ft)", ClampMin = "1.0"))
	float LengthFt = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape", meta = (DisplayName = "Section Length (ft)", ClampMin = "1.0", EditCondition = "!bAutoCalculateSections"))
	float SectionLengthFt = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape", meta = (DisplayName = "Height (ft)", ClampMin = "6.0", ClampMax = "18.333"))
	float HeightFt = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape")
	bool bAutoCalculateSections = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape", meta = (DisplayName = "Min Crossbar Length (ft)", ClampMin = "1.0"))
	float MinCrossbarLengthFt = 7.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape", meta = (DisplayName = "Max Crossbar Length (ft)", ClampMin = "1.0"))
	float MaxCrossbarLengthFt = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape")
	EDrapeRunDrapeMode DrapeMode = EDrapeRunDrapeMode::StaticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape")
	bool bUseChaosCloth = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape", meta = (ClampMin = "0.1"))
	float Fullness = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape")
	bool bShowHardware = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape")
	bool bShowDrape = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape")
	bool bCenterOnActor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape")
	bool bBuildOnConstruction = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Assets")
	FString BaseAssetFolder = TEXT("/Game/Majic_Gear/Drape/Drape_Base/StaticMeshes");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Assets")
	FString Upright6FtAssetFolder = TEXT("/Game/Majic_Gear/Drape/72inch_Pole/StaticMeshes");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Assets")
	FString Upright8FtAssetFolder = TEXT("/Game/Majic_Gear/Drape/96inch_Pole/StaticMeshes");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Assets")
	FString Upright10FtAssetFolder = TEXT("/Game/Majic_Gear/Drape/120inch_Drape_Pole/StaticMeshes");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Assets")
	FString RodAssetFolder = TEXT("/Game/Majic_Gear/Drape/7-12_ft_Drape_Rod/StaticMeshes");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Assets")
	TSoftObjectPtr<UStaticMesh> StaticDrapeMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Assets")
	TSoftObjectPtr<USkeletalMesh> ChaosDrapeMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Material")
	TSoftObjectPtr<UMaterialInterface> DrapeMaterialOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Sizing", meta = (Units = "cm", ClampMin = "1.0"))
	float NativeSectionLengthCm = 304.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Sizing", meta = (Units = "cm", ClampMin = "1.0"))
	float NativeDrapeHeightCm = 243.84f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Sizing", meta = (Units = "cm", ClampMin = "1.0"))
	float NativeRodHeightCm = 487.68f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Sizing", meta = (ClampMin = "1.0"))
	float NativeRodLengthFt = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Sizing")
	bool bMoveInsideRodForSectionLength = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Sizing")
	bool bMoveOutsideRodForSectionLength = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Placement", meta = (DisplayName = "Outside Rod Span Adjustment Cm", Units = "cm"))
	float OutsideRodSpanAdjustmentCm = 49.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Sizing", meta = (Units = "cm", ClampMin = "1.0"))
	float Native72InchPoleInsideHeightCm = 304.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Sizing", meta = (Units = "cm", ClampMin = "1.0"))
	float Native96InchPoleInsideHeightCm = 365.76f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Sizing", meta = (Units = "cm", ClampMin = "1.0"))
	float Native120InchPoleInsideHeightCm = 487.68f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Sizing", meta = (Units = "cm"))
	float Inside72InchPoleCalibrationOffsetCm = 58.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Sizing", meta = (Units = "cm"))
	float Inside96InchPoleCalibrationOffsetCm = 89.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Sizing", meta = (Units = "cm"))
	float Inside120InchPoleCalibrationOffsetCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Sizing")
	bool bScaleDrapeToSection = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Sizing")
	bool bScaleDrapeToHeight = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Sizing")
	bool bAnchorDrapeTopWhenScaling = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Sizing", meta = (Units = "cm"))
	float DrapeTopAnchorLocalZCm = 486.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drape|Debug", meta = (Units = "cm"))
	float CurrentSlidingHeightAdjustmentCm = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drape|Debug", meta = (Units = "cm"))
	float CurrentInsideUprightHeightAdjustmentCm = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drape|Debug")
	int32 CurrentSectionCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drape|Debug", meta = (DisplayName = "Current Section Length (ft)"))
	float CurrentSectionLengthFt = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drape|Debug", meta = (DisplayName = "Current Actual Length (ft)"))
	float CurrentActualLengthFt = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Placement", meta = (Units = "cm"))
	FVector BasePlacementOffsetCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Placement")
	FRotator BasePlacementRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Placement")
	FVector BaseScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Placement", meta = (Units = "cm"))
	FVector UprightPlacementOffsetCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Placement", meta = (Units = "cm"))
	FVector InsideUprightPlacementOffsetCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Placement")
	FRotator UprightPlacementRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Placement")
	FVector UprightScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Placement", meta = (Units = "cm"))
	FVector CrossbarPlacementOffsetCm = FVector(317.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Placement")
	FRotator CrossbarPlacementRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Placement")
	FVector CrossbarScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Placement", meta = (Units = "cm"))
	FVector DrapePlacementOffsetCm = FVector(150.0f, 0.1f, 242.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Placement")
	FRotator DrapePlacementRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drape|Placement")
	FVector DrapeScale = FVector(1.0f, 1.0f, 0.5f);

	UFUNCTION(BlueprintCallable, Category = "Drape")
	void RebuildDrapeRun();

	UFUNCTION(BlueprintCallable, Category = "Drape")
	void ApplyBuildDefinition(const FDrapeRunBuildDefinition& Definition, bool bRebuildNow = true);

	UFUNCTION(BlueprintPure, Category = "Drape")
	FDrapeRunBuildDefinition GetBuildDefinition() const;

	UFUNCTION(BlueprintCallable, Category = "Drape")
	void SetSelectionHighlighted(bool bHighlighted);

	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> GeneratedMeshComponents;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USkeletalMeshComponent>> GeneratedChaosDrapeComponents;

	float GetHeightCm() const;
	int32 GetSectionCount() const;
	float GetSectionLengthFt() const;
	float GetSectionLengthCm() const;
	FString GetUprightAssetFolder() const;
	float GetInsideUprightNativeHeightCm() const;
	float GetInsideUprightCalibrationOffsetCm() const;
	TArray<FSoftObjectPath> GetMeshPathsForFolder(const FString& AssetFolderPath) const;
	TArray<FSoftObjectPath> GetOutsidePoleMeshPaths() const;
	TArray<FSoftObjectPath> GetInsidePoleMeshPaths() const;
	TArray<FSoftObjectPath> GetBaseMeshPaths() const;
	TArray<FSoftObjectPath> GetOutsideRodMeshPaths() const;
	TArray<FSoftObjectPath> GetInsideRodMeshPaths() const;
	UMaterialInterface* ResolveDrapeMaterial() const;
	UInstancedStaticMeshComponent* FindOrCreateMeshBucket(UStaticMesh* StaticMesh, const TCHAR* Prefix, bool bApplyDrapeMaterial);
	UInstancedStaticMeshComponent* FindGeneratedMeshComponentByName(const FName& ComponentName) const;
	USkeletalMeshComponent* FindOrCreateChaosDrapeComponent(int32 SectionIndex);
	void ClearGeneratedComponents();
	void AddStaticMeshInstance(UStaticMesh* StaticMesh, const TCHAR* Prefix, const FTransform& InstanceTransform, bool bApplyDrapeMaterial, FBox& Bounds);
	void AddStaticMeshPathInstance(const FSoftObjectPath& MeshPath, const TCHAR* Prefix, const FTransform& InstanceTransform, bool bApplyDrapeMaterial, FBox& Bounds);
	void AddMeshPathSet(const TArray<FSoftObjectPath>& MeshPaths, const TCHAR* Prefix, const FTransform& InstanceTransform, bool bApplyDrapeMaterial, FBox& Bounds);
	void AddChaosDrapeSection(int32 SectionIndex, const FTransform& SectionTransform, FBox& Bounds);
	void UpdateSelectionBounds(const FBox& Bounds);
};
