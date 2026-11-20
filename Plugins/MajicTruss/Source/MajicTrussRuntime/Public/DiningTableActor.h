#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DiningTableActor.generated.h"

class UBoxComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UProceduralMeshComponent;
class UStaticMesh;
class UTexture2D;

UENUM(BlueprintType)
enum class EDiningChairArrangement : uint8
{
	Radial UMETA(DisplayName = "Radial"),
	Rectangular UMETA(DisplayName = "Rectangular")
};

UENUM(BlueprintType)
enum class EDiningChairCount : uint8
{
	Two UMETA(DisplayName = "2 Chairs"),
	Four UMETA(DisplayName = "4 Chairs"),
	Six UMETA(DisplayName = "6 Chairs"),
	Eight UMETA(DisplayName = "8 Chairs")
};

UENUM(BlueprintType)
enum class EDiningTableclothShape : uint8
{
	Auto UMETA(DisplayName = "Auto"),
	Rectangular UMETA(DisplayName = "Rectangular"),
	Round UMETA(DisplayName = "Round")
};

USTRUCT(BlueprintType)
struct FDiningTableBuildDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table")
	FString DiningTableItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table")
	FString DiningChairItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table")
	EDiningChairCount ChairCount = EDiningChairCount::Four;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table")
	EDiningChairArrangement ChairArrangement = EDiningChairArrangement::Rectangular;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table", meta = (Units = "cm"))
	float ChairGapCm = -120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table|Tablecloth")
	bool bUseTablecloth = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table|Tablecloth")
	FString TableclothItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table|Tablecloth")
	EDiningTableclothShape TableclothShape = EDiningTableclothShape::Auto;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table|Tablecloth", meta = (Units = "cm"))
	float TableclothOverhangCm = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table|Tablecloth", meta = (Units = "cm"))
	float TableclothDropCm = 72.0f;
};

UCLASS(BlueprintType, meta = (DisplayName = "Dining Table Actor"))
class MAJICTRUSSRUNTIME_API ADiningTableActor : public AActor
{
	GENERATED_BODY()

public:
	ADiningTableActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dining Table")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dining Table")
	TObjectPtr<UBoxComponent> SelectionBounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table|Assets")
	FString DiningTableAssetFolder = TEXT("/Game/Furniture/Dining_Tables");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table|Assets")
	FString DiningChairAssetFolder = TEXT("/Game/Furniture/Dining_Seating");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table|Assets")
	FString TableclothAssetFolder = TEXT("/Game/Furniture/Dining_Seating/BBJ_Linens/Table_Cloths");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table|Assets", meta = (GetOptions = "GetDiningTableOptions"))
	FString DiningTableItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table|Assets", meta = (GetOptions = "GetDiningChairOptions"))
	FString DiningChairItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table|Assets", meta = (GetOptions = "GetTableclothOptions"))
	FString TableclothItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table")
	bool bBuildOnConstruction = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table|Layout")
	EDiningChairCount ChairCount = EDiningChairCount::Four;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table|Layout")
	EDiningChairArrangement ChairArrangement = EDiningChairArrangement::Rectangular;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table|Layout", meta = (DisplayName = "Chair Gap Cm", ClampMin = "-200.0", ClampMax = "80.0", Units = "cm"))
	float ChairGapCm = -120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table|Tablecloth")
	bool bUseTablecloth = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table|Tablecloth")
	EDiningTableclothShape TableclothShape = EDiningTableclothShape::Auto;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table|Tablecloth", meta = (ClampMin = "0.0", ClampMax = "80.0", Units = "cm", EditCondition = "bUseTablecloth"))
	float TableclothOverhangCm = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table|Tablecloth", meta = (ClampMin = "0.0", ClampMax = "120.0", Units = "cm", EditCondition = "bUseTablecloth"))
	float TableclothDropCm = 72.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table|Tablecloth", meta = (Units = "cm", EditCondition = "bUseTablecloth"))
	float TableclothTopZOffsetCm = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table|Tablecloth", meta = (ClampMin = "12", ClampMax = "96", EditCondition = "bUseTablecloth"))
	int32 RoundTableclothSegments = 48;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table|Tablecloth", meta = (EditCondition = "bUseTablecloth"))
	TSoftObjectPtr<UMaterialInterface> TableclothBaseMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Furniture/Dining_Seating/BBJ_Linens/M_Tablecloth_TextureSwap.M_Tablecloth_TextureSwap")));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table|Tablecloth", meta = (EditCondition = "bUseTablecloth"))
	FName TableclothTextureParameterName = TEXT("TableclothTexture");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table|Tablecloth", meta = (ClampMin = "0.01", ClampMax = "20.0", EditCondition = "bUseTablecloth"))
	float TableclothTextureScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Dining Table|Tablecloth", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bUseTablecloth", ToolTip = "Blends skirt normals upward so vertical cloth sides do not render much darker than the tabletop."))
	float TableclothSideNormalUpBias = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table|Tablecloth", meta = (EditCondition = "bUseTablecloth"))
	FName TableclothTextureScaleParameterName = TEXT("TableclothTextureScale");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table|Layout", meta = (DisplayName = "Rectangular Side Spread", ClampMin = "0.0", ClampMax = "1.25", EditCondition = "ChairArrangement == EDiningChairArrangement::Rectangular"))
	float RectangularSideSpread = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Dining Table|Calibration", meta = (DisplayName = "Chair Bounds Inward Calibration Cm", Units = "cm", ToolTip = "Advanced mesh-origin calibration. Use Chair Gap Cm for normal chair spacing."))
	float ChairInwardAdjustmentCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table|Placement")
	FVector TablePlacementOffsetCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table|Placement")
	FRotator TableRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table|Placement")
	FVector TableScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table|Placement")
	FVector ChairPlacementOffsetCm = FVector(0.0f, 0.0f, -38.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table|Placement")
	FRotator ChairRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table|Placement")
	FVector ChairScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dining Table|Placement", meta = (Units = "deg"))
	float ChairYawOffsetDegrees = 180.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dining Table|Debug")
	int32 CurrentGeneratedMeshCount = 0;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Dining Table")
	void RebuildDiningTable();

	UFUNCTION(BlueprintCallable, Category = "Dining Table")
	void ApplyBuildDefinition(const FDiningTableBuildDefinition& Definition, bool bRebuildNow = true);

	UFUNCTION(BlueprintPure, Category = "Dining Table")
	FDiningTableBuildDefinition GetBuildDefinition() const;

	UFUNCTION(BlueprintCallable, Category = "Dining Table")
	void SetSelectionHighlighted(bool bHighlighted);

	UFUNCTION(BlueprintCallable, Category = "Dining Table|Assets")
	TArray<FString> GetDiningTableOptions() const;

	UFUNCTION(BlueprintCallable, Category = "Dining Table|Assets")
	TArray<FString> GetDiningChairOptions() const;

	UFUNCTION(BlueprintCallable, Category = "Dining Table|Assets")
	TArray<FString> GetTableclothOptions() const;

	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> GeneratedMeshComponents;

	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> TableclothMeshComponent = nullptr;

	FBox GeneratedBounds;
	FBox CurrentTableBounds;
	FBox CurrentChairBounds;

	void ClearGenerated();
	void AddFurnitureItem(const FString& CategoryFolder, const FString& ItemName, const TCHAR* Prefix, const FTransform& ItemTransform, FBox* OutItemBounds = nullptr);
	void AddChairLayout(const FString& ResolvedChairItem);
	void AddRadialChairLayout(const FString& ResolvedChairItem, int32 ResolvedChairCount);
	void AddRectangularChairLayout(const FString& ResolvedChairItem, int32 ResolvedChairCount);
	UInstancedStaticMeshComponent* FindOrCreateMeshBucket(const FSoftObjectPath& MeshPath, const TCHAR* Prefix);
	TArray<FString> GetFurnitureItemOptions(const FString& CategoryFolder) const;
	TArray<FSoftObjectPath> GetMeshPathsForFurnitureItem(const FString& CategoryFolder, const FString& ItemName) const;
	TArray<FString> GetFlatAssetOptions(const FString& AssetFolder) const;
	FString ResolveSelectedItem(const FString& CategoryFolder, const FString& SelectedItem) const;
	FBox GetFurnitureItemBounds(const FString& CategoryFolder, const FString& ItemName, const FTransform& ItemTransform) const;
	void ExpandGeneratedBounds(UStaticMesh* Mesh, const FTransform& Transform, FBox* OutItemBounds);
	void BuildTablecloth();
	void BuildRectangularTablecloth(const FBox& TableBounds);
	void BuildRoundTablecloth(const FBox& TableBounds);
	UMaterialInterface* ResolveTableclothMaterial();
	UTexture2D* ResolveTableclothTexture() const;
	void UpdateSelectionBounds();
	int32 GetResolvedChairCount() const;
	float GetChairInwardLocalYCm() const;
};
