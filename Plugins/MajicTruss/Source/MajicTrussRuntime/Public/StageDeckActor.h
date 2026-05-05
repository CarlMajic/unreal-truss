#pragma once

#include "CoreMinimal.h"
#include "StageDeckBuildDefinition.h"
#include "GameFramework/Actor.h"
#include "StageDeckActor.generated.h"

class UBoxComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMeshComponent;
class UTextRenderComponent;

UENUM(BlueprintType)
enum class EStageDeckBatchAxis : uint8
{
	Row UMETA(DisplayName = "Row"),
	Column UMETA(DisplayName = "Column")
};

UENUM(BlueprintType)
enum class EStagePodiumStyle : uint8
{
	None UMETA(DisplayName = "None"),
	Acrylic UMETA(DisplayName = "Acrylic Podium"),
	LargeWood UMETA(DisplayName = "Large Wood Podium"),
	Screen UMETA(DisplayName = "Screen Podium"),
	WhiteAcrylicFront UMETA(DisplayName = "White Podium With Acrylic Front")
};

UENUM(BlueprintType)
enum class EStageRailingSpanType : uint8
{
	In46 UMETA(DisplayName = "46 Inch"),
	In94 UMETA(DisplayName = "94 Inch")
};

USTRUCT(BlueprintType)
struct FStageDeckCell
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage", meta = (EditCondition = "bEnabled"))
	EStageDeckHeightPreset HeightPreset = EStageDeckHeightPreset::In24;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage", meta = (EditCondition = "bEnabled"))
	EStageDeckSurfaceStyle SurfaceStyle = EStageDeckSurfaceStyle::BlackTop;
};

UCLASS(BlueprintType)
class MAJICTRUSSRUNTIME_API AStageDeckActor : public AActor
{
	GENERATED_BODY()

public:
	AStageDeckActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stage")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stage")
	TObjectPtr<UBoxComponent> SelectionBounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage", meta = (ClampMin = "1"))
	int32 Columns = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage", meta = (ClampMin = "1"))
	int32 Rows = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
	EStageDeckHeightPreset DefaultHeightPreset = EStageDeckHeightPreset::In24;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
	EStageDeckSurfaceStyle DefaultSurfaceStyle = EStageDeckSurfaceStyle::BlackTop;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
	TSoftObjectPtr<UMaterialInterface> DefaultSurfaceMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage", meta = (Units = "cm", ClampMin = "1.0"))
	float CellWidthCm = 243.84f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage", meta = (Units = "cm", ClampMin = "1.0"))
	float CellDepthCm = 121.92f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage", meta = (Units = "cm"))
	float HorizontalSpacingCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage", meta = (Units = "cm"))
	float VerticalSpacingCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
	bool bCenterOnActor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
	bool bBuildOnConstruction = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
	TArray<FStageDeckCell> DeckCells;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Batch", meta = (ClampMin = "0"))
	int32 BatchTargetIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Batch")
	EStageDeckBatchAxis BatchEditAxis = EStageDeckBatchAxis::Column;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Batch")
	bool bBatchEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Batch")
	EStageDeckHeightPreset BatchHeightPreset = EStageDeckHeightPreset::In24;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Batch")
	EStageDeckSurfaceStyle BatchSurfaceStyle = EStageDeckSurfaceStyle::BlackTop;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Batch")
	TSoftObjectPtr<UMaterialInterface> BatchSurfaceMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Podium")
	EStagePodiumStyle PodiumStyle = EStagePodiumStyle::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Podium", meta = (Units = "cm"))
	float PodiumOffsetXCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Podium", meta = (Units = "cm"))
	float PodiumOffsetYCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Podium", meta = (Units = "cm"))
	float PodiumOffsetZCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Podium")
	float PodiumYawDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage", meta = (Units = "cm"))
	FVector DeckPlacementOffsetCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
	FRotator DeckPlacementRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Editor")
	bool bShowCellLabels = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Editor", meta = (Units = "cm"))
	float CellLabelHeightCm = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Railing")
	bool bEnableFrontRailing = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Railing")
	bool bEnableBackRailing = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Railing")
	bool bEnableLeftRailing = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Railing")
	bool bEnableRightRailing = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Railing", meta = (Units = "cm"))
	FVector RailingPlacementOffsetCm = FVector(249.338089f, -61.650452f, 32.453884f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Railing")
	FRotator RailingPlacementRotation = FRotator(0.0f, 180.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Railing", meta = (Units = "cm"))
	FVector FrontRailingOffsetCm = FVector(249.338089f, -61.650452f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Railing", meta = (Units = "cm"))
	FVector BackRailingOffsetCm = FVector(0.0f, 62.298616f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Railing", meta = (Units = "cm"))
	FVector LeftRailingOffsetCm = FVector(189.700066f, 120.653588f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Railing", meta = (Units = "cm"))
	FVector RightRailingOffsetCm = FVector(58.727953f, -121.189095f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Railing", meta = (Units = "cm"))
	float RailingHeightAdjust8Cm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Railing", meta = (Units = "cm"))
	float RailingHeightAdjust12Cm = 11.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Railing", meta = (Units = "cm"))
	float RailingHeightAdjust24Cm = 32.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Railing", meta = (Units = "cm"))
	float RailingHeightAdjust27Cm = 55.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Railing", meta = (Units = "cm"))
	FVector Railing94SpanAdjustmentCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Railing", meta = (Units = "cm"))
	FVector Railing46SpanAdjustmentCm = FVector(-61.084781f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Steps")
	bool bEnableLeftStep = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Steps")
	bool bEnableRightStep = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Steps", meta = (Units = "cm"))
	FVector LeftStepOffsetCm = FVector(0.0f, -123.745361f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Steps", meta = (Units = "cm"))
	FVector RightStepOffsetCm = FVector(244.693954f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Steps")
	FRotator LeftStepRotation = FRotator(0.0f, 90.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Steps")
	FRotator RightStepRotation = FRotator(0.0f, -90.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Skirt")
	bool bUseDebugSingleSkirt = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Skirt")
	bool bEnableAutomaticSkirt = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Skirt", meta = (Units = "cm"))
	FVector DebugSkirtOffsetCm = FVector(123.166338f, -65.886528f, 5.024358f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Skirt")
	FRotator DebugSkirtRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Skirt")
	FVector DebugSkirtScale = FVector(1.05f, 0.25f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Skirt")
	float SkirtHeightScale27Inch = 1.185277f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Skirt", meta = (Units = "cm"))
	FVector AutoSkirtFrontAdjustmentCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Skirt", meta = (Units = "cm"))
	FVector AutoSkirtBackAdjustmentCm = FVector(0.0f, 121.92f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Skirt", meta = (Units = "cm"))
	FVector AutoSkirtLeftAdjustmentCm = FVector(0.0f, 124.419191f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Skirt", meta = (Units = "cm"))
	FVector AutoSkirtRightAdjustmentCm = FVector(-125.676275f, 3.194805f, 0.0f);

	UFUNCTION(BlueprintCallable, Category = "Stage")
	void RebuildStage();

	UFUNCTION(BlueprintCallable, Category = "Stage")
	void ApplyBuildDefinition(const FStageDeckBuildDefinition& Definition, bool bRebuildNow = true);

	UFUNCTION(BlueprintCallable, Category = "Stage")
	bool GetCellDefinition(int32 RowIndex, int32 ColumnIndex, FStageDeckCell& OutCell) const;

	UFUNCTION(BlueprintCallable, Category = "Stage")
	void ApplyCellDefinition(int32 RowIndex, int32 ColumnIndex, const FStageDeckCell& CellDefinition, bool bRebuildNow = true);

	UFUNCTION(BlueprintCallable, Category = "Stage")
	bool GetCellIndicesFromWorldLocation(const FVector& WorldLocation, int32& OutRow, int32& OutColumn) const;

	UFUNCTION(BlueprintCallable, Category = "Stage")
	void SetSelectionHighlighted(bool bHighlighted);

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Stage")
	void ResetCellsToDefault();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Stage")
	void ResizeCellsToGrid();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Stage|Batch")
	void ApplyBatchEdit();

	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> GeneratedMeshComponents;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> GeneratedDrapeComponents;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextRenderComponent>> GeneratedLabelComponents;

	UPROPERTY(Transient)
	bool bHasCachedDefaults = false;

	UPROPERTY(Transient)
	EStageDeckHeightPreset CachedDefaultHeightPreset = EStageDeckHeightPreset::In24;

	UPROPERTY(Transient)
	EStageDeckSurfaceStyle CachedDefaultSurfaceStyle = EStageDeckSurfaceStyle::BlackTop;

	void EnsureCellCount(bool bResetNewCellsToDefault);
	FStageDeckCell MakeDefaultCell() const;
	void SyncCellsToDefaultIfNeeded();
	void ApplyDefaultSettingsToAllCells();
	TArray<int32> GetBatchCellIndices() const;
	bool IsValidCellIndexPair(int32 RowIndex, int32 ColumnIndex) const;
	int32 GetCellLinearIndex(int32 RowIndex, int32 ColumnIndex) const;
	float GetDeckHeightCm(EStageDeckHeightPreset HeightPreset) const;
	float GetRailingHeightDeltaZCm(EStageDeckHeightPreset HeightPreset) const;
	FString GetDeckAssetFolder(EStageDeckHeightPreset HeightPreset, EStageDeckSurfaceStyle SurfaceStyle) const;
	FString GetPodiumAssetFolder(EStagePodiumStyle InPodiumStyle) const;
	FString GetRailingAssetFolder(EStageRailingSpanType SpanType) const;
	TArray<FSoftObjectPath> GetStepMeshPaths(EStageDeckHeightPreset HeightPreset) const;
	TArray<FSoftObjectPath> GetMeshPathsForFolder(const FString& AssetFolderPath) const;
	TArray<FSoftObjectPath> GetDeckMeshPaths(EStageDeckHeightPreset HeightPreset, EStageDeckSurfaceStyle SurfaceStyle) const;
	TArray<FSoftObjectPath> GetPodiumMeshPaths() const;
	TArray<FSoftObjectPath> GetRailingMeshPaths(EStageRailingSpanType SpanType) const;
	UMaterialInterface* ResolveDeckSurfaceMaterial(EStageDeckSurfaceStyle SurfaceStyle) const;
	UInstancedStaticMeshComponent* FindOrCreateMeshBucket(const FSoftObjectPath& MeshPath, const TCHAR* Prefix, EStageDeckSurfaceStyle SurfaceStyle, TMap<FString, UInstancedStaticMeshComponent*>& BucketMap);
	void ClearGeneratedComponents();
	UInstancedStaticMeshComponent* FindGeneratedMeshComponentByName(const FName& ComponentName) const;
	void ClearGeneratedDrapes();
	void ClearGeneratedLabels();
	void AddDeckCellInstance(const FSoftObjectPath& MeshPath, EStageDeckSurfaceStyle SurfaceStyle, const FVector& CellCenter, TMap<FString, UInstancedStaticMeshComponent*>& BucketMap, FBox& Bounds);
	void AddCellLabel(int32 RowIndex, int32 ColumnIndex, const FVector& CellCenter, bool bCellEnabled);
	void AddPodiumInstances(TMap<FString, UInstancedStaticMeshComponent*>& BucketMap, FBox& Bounds);
	void AddStepInstances(TMap<FString, UInstancedStaticMeshComponent*>& BucketMap, FBox& Bounds);
	void AddRailingSpan(EStageRailingSpanType SpanType, const FVector& SpanCenter, const FRotator& SpanRotation, const FVector& EdgeAdjustment, TMap<FString, UInstancedStaticMeshComponent*>& BucketMap, FBox& Bounds);
	void AddRailingRun(float TotalLengthCm, const FVector& StartLocation, const FVector& AlongDirection, const FRotator& SpanRotation, const FVector& EdgeAdjustment, TMap<FString, UInstancedStaticMeshComponent*>& BucketMap, FBox& Bounds);
	void AddRailingsForPerimeter(const float OriginX, const float OriginY, const float StepX, const float StepY, TMap<FString, UInstancedStaticMeshComponent*>& BucketMap, FBox& Bounds);
	void AddDrapeForEdge(const FVector& EdgeCenter, const FRotator& EdgeRotation, float EdgeLengthCm, float DeckHeightCm, FBox& Bounds, const FVector& ExtraScale = FVector::OneVector);
	bool GetFirstEnabledCell(int32& OutRowIndex, int32& OutColumnIndex, FStageDeckCell& OutCell) const;
	bool ShouldPlaceLeftStepAtCell(int32 RowIndex, int32 ColumnIndex) const;
	bool ShouldPlaceRightStepAtCell(int32 RowIndex, int32 ColumnIndex) const;
	void UpdateSelectionBounds(const FBox& Bounds);
};
