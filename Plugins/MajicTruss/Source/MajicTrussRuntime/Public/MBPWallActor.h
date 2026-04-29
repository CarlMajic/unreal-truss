#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MBPWallActor.generated.h"

class UBoxComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;

UENUM(BlueprintType)
enum class EMBPPanelStyle : uint8
{
	Empty UMETA(DisplayName = "Empty"),
	Acrylic UMETA(DisplayName = "Acrylic"),
	Drift UMETA(DisplayName = "Drift"),
	Geo UMETA(DisplayName = "Geo"),
	Shimmer UMETA(DisplayName = "Shimmer"),
	Hive UMETA(DisplayName = "Hive"),
	Platinum UMETA(DisplayName = "Platinum"),
	Custom UMETA(DisplayName = "Custom")
};

UENUM(BlueprintType)
enum class EMBPShimmerVariant : uint8
{
	Gold UMETA(DisplayName = "Gold"),
	Red UMETA(DisplayName = "Red"),
	Fuchsia UMETA(DisplayName = "Fuchsia"),
	Holographic UMETA(DisplayName = "Holographic")
};

UENUM(BlueprintType)
enum class EMBPBatchEditAxis : uint8
{
	Row UMETA(DisplayName = "Row"),
	Column UMETA(DisplayName = "Column")
};

USTRUCT(BlueprintType)
struct FMBPPanelSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Panel")
	EMBPPanelStyle Style = EMBPPanelStyle::Geo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Panel")
	EMBPShimmerVariant ShimmerVariant = EMBPShimmerVariant::Gold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Panel", meta = (Units = "cm"))
	float DepthOffsetCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Panel", meta = (EditCondition = "Style == EMBPPanelStyle::Shimmer"))
	TSoftObjectPtr<UMaterialInterface> ShimmerMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Panel", meta = (EditCondition = "Style == EMBPPanelStyle::Custom"))
	TArray<TSoftObjectPtr<UStaticMesh>> CustomStaticMeshes;
};

UCLASS(BlueprintType)
class MAJICTRUSSRUNTIME_API AMBPWallActor : public AActor
{
	GENERATED_BODY()

public:
	AMBPWallActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall")
	TObjectPtr<UBoxComponent> SelectionBounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall", meta = (ClampMin = "1"))
	int32 Columns = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall", meta = (ClampMin = "1"))
	int32 Rows = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall")
	EMBPPanelStyle DefaultStyle = EMBPPanelStyle::Geo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall")
	EMBPShimmerVariant DefaultShimmerVariant = EMBPShimmerVariant::Gold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall")
	TSoftObjectPtr<UMaterialInterface> DefaultShimmerMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall|Shimmer", meta = (Units = "cm"))
	float ShimmerFaceOffsetXCm = -1.902981f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall|Shimmer", meta = (Units = "cm"))
	float ShimmerFaceOffsetYCm = -0.104f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall|Shimmer", meta = (Units = "cm"))
	float ShimmerFaceOffsetZCm = -4.650027f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall", meta = (Units = "cm"))
	float PanelWidthCm = 91.44f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall", meta = (Units = "cm"))
	float PanelHeightCm = 91.44f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall", meta = (Units = "cm"))
	float HorizontalSpacingCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall", meta = (Units = "cm"))
	float VerticalSpacingCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall", meta = (Units = "cm", ClampMin = "0.0"))
	float DepthOffsetStepCm = 30.48f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall")
	bool bCenterOnActor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall")
	bool bBuildOnConstruction = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall")
	TArray<FMBPPanelSlot> PanelSlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall|Batch", meta = (ClampMin = "0"))
	int32 BatchTargetIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall|Batch")
	EMBPBatchEditAxis BatchEditAxis = EMBPBatchEditAxis::Column;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall|Batch")
	EMBPPanelStyle BatchStyle = EMBPPanelStyle::Geo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall|Batch")
	EMBPShimmerVariant BatchShimmerVariant = EMBPShimmerVariant::Gold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall|Batch")
	TSoftObjectPtr<UMaterialInterface> BatchShimmerMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall|Batch", meta = (Units = "cm"))
	float BatchDepthOffsetCm = 0.0f;

	UFUNCTION(BlueprintCallable, Category = "Wall")
	void RebuildWall();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Wall")
	void ResetSlotsToDefault();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Wall")
	void ResizeSlotsToGrid();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Wall|Batch")
	void ApplyBatchEdit();

	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> GeneratedInstanceComponents;

	UPROPERTY(Transient)
	bool bHasCachedDefaultStyle = false;

	UPROPERTY(Transient)
	EMBPPanelStyle CachedDefaultStyle = EMBPPanelStyle::Geo;

	UPROPERTY(Transient)
	EMBPShimmerVariant CachedDefaultShimmerVariant = EMBPShimmerVariant::Gold;

	UPROPERTY(Transient)
	TSoftObjectPtr<UMaterialInterface> CachedDefaultShimmerMaterial;

	void EnsureSlotCount(bool bResetNewSlotsToDefault);
	FMBPPanelSlot MakeDefaultSlot() const;
	void SyncSlotsToDefaultStyleIfNeeded();
	void ApplyDefaultStyleToAllSlots();
	TArray<int32> GetBatchSlotIndices() const;
	float GetSnappedDepthOffsetCm(float RawDepthOffsetCm) const;
	TArray<FSoftObjectPath> GetMeshPathsForFolder(const FString& AssetFolderPath) const;
	TArray<FSoftObjectPath> GetMeshPathsForStyle(EMBPPanelStyle Style) const;
	TArray<FSoftObjectPath> GetMeshPathsForSlot(const FMBPPanelSlot& Slot) const;
	TArray<FSoftObjectPath> GetShimmerFrameMeshPaths() const;
	FSoftObjectPath GetShimmerPlaneMeshPath() const;
	TArray<FSoftObjectPath> GetExtraFrameMeshPaths() const;
	UMaterialInterface* LoadShimmerMaterialOverride(EMBPShimmerVariant Variant) const;
	UMaterialInterface* ResolveShimmerMaterial(const FMBPPanelSlot& Slot) const;
	UInstancedStaticMeshComponent* FindGeneratedComponentByName(const FName& ComponentName) const;
	UInstancedStaticMeshComponent* FindOrCreateComponentBucket(
		const FSoftObjectPath& MeshPath,
		const FMBPPanelSlot& Slot,
		TMap<FString, UInstancedStaticMeshComponent*>& BucketMap);
	void ClearGeneratedComponents();
	void UpdateSelectionBounds(const FBox& Bounds);
};
