#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CafeTableActor.generated.h"

class UBoxComponent;
class UArrowComponent;
class UInstancedStaticMeshComponent;
class USphereComponent;
class UAnimationAsset;
class UStaticMesh;

UENUM(BlueprintType)
enum class ECafeTableChairLayout : uint8
{
	NoChairs UMETA(DisplayName = "No Chairs"),
	TwoTop UMETA(DisplayName = "Square/Round Two Top"),
	FourTop UMETA(DisplayName = "Square/Round Four Top"),
	RectangleFourTop UMETA(DisplayName = "Rectangle Four Top"),
	RectangleSixTop UMETA(DisplayName = "Rectangle Six Top")
};

USTRUCT(BlueprintType)
struct FCafeTableBuildDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cafe Table")
	FString CafeTableItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cafe Table")
	FString DiningChairItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cafe Table")
	ECafeTableChairLayout ChairLayout = ECafeTableChairLayout::FourTop;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cafe Table")
	bool bClassroomStyle = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cafe Table", meta = (Units = "cm"))
	float ChairOffsetYCm = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cafe Table", meta = (Units = "cm"))
	float RectangleChairPairOffsetXCm = 125.0f;
};

UCLASS(BlueprintType, meta = (DisplayName = "Cafe Table Actor"))
class MAJICTRUSSRUNTIME_API ACafeTableActor : public AActor
{
	GENERATED_BODY()

public:
	ACafeTableActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cafe Table")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cafe Table")
	TObjectPtr<UBoxComponent> SelectionBounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cafe Table|Assets")
	FString CafeTableAssetFolder = TEXT("/Game/Furniture/Cafe_Tables");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cafe Table|Assets")
	FString DiningChairAssetFolder = TEXT("/Game/Furniture/Dining_Seating");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cafe Table|Assets", meta = (GetOptions = "GetCafeTableOptions"))
	FString CafeTableItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cafe Table|Assets", meta = (GetOptions = "GetDiningChairOptions"))
	FString DiningChairItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cafe Table")
	bool bBuildOnConstruction = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cafe Table|Layout")
	ECafeTableChairLayout ChairLayout = ECafeTableChairLayout::FourTop;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cafe Table|Layout")
	bool bClassroomStyle = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cafe Table|Layout", meta = (DisplayName = "Chair Y Offset Cm", Units = "cm"))
	float ChairOffsetYCm = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cafe Table|Layout", meta = (DisplayName = "Rectangle Chair Pair X Offset Cm", Units = "cm", EditCondition = "ChairLayout == ECafeTableChairLayout::RectangleFourTop || ChairLayout == ECafeTableChairLayout::RectangleSixTop"))
	float RectangleChairPairOffsetXCm = 125.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cafe Table|Placement")
	FRotator CafeTableRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cafe Table|Placement")
	FRotator DiningChairRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cafe Table|Placement")
	FVector CafeTableScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cafe Table|Placement")
	FVector DiningChairScale = FVector::OneVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cafe Table|Debug")
	int32 CurrentGeneratedMeshCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cafe Table|AI Preview")
	bool bShowAISlotPreviewInEditor = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cafe Table|AI Preview", meta = (ClampMin = "0.0", Units = "cm"))
	float AISlotPreviewSpreadRadiusCm = 79.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cafe Table|AI Preview", meta = (Units = "cm"))
	FVector AISlotPreviewOffsetCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cafe Table|AI Preview")
	TArray<FVector> AISlotPerSlotOffsetsCm;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cafe Table|AI Preview")
	TArray<FRotator> AISlotPerSlotRotationOffsets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cafe Table|AI Animations")
	TSoftObjectPtr<UAnimationAsset> AIAnimationPreviewDefaultAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cafe Table|AI Animations")
	TArray<TSoftObjectPtr<UAnimationAsset>> AIAnimationPreviewPerSlotAnimations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cafe Table|AI Preview", meta = (Units = "deg"))
	float AISlotPreviewStartAngleDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cafe Table|AI Preview", meta = (Units = "cm"))
	FVector AISlotFacingLocalPointCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cafe Table|AI Preview", meta = (Units = "deg"))
	float AISlotFacingYawOffsetDegrees = 0.0f;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Cafe Table")
	void RebuildCafeTable();

	UFUNCTION(BlueprintCallable, Category = "Cafe Table")
	void ApplyBuildDefinition(const FCafeTableBuildDefinition& Definition, bool bRebuildNow = true);

	UFUNCTION(BlueprintPure, Category = "Cafe Table")
	FCafeTableBuildDefinition GetBuildDefinition() const;

	UFUNCTION(BlueprintCallable, Category = "Cafe Table")
	void SetSelectionHighlighted(bool bHighlighted);

	UFUNCTION(BlueprintCallable, Category = "Cafe Table|AI Preview")
	void GetAISlotPreviewLocalOffsets(TArray<FVector>& OutOffsets) const;

	UFUNCTION(BlueprintCallable, Category = "Cafe Table|AI Preview")
	void GetAISlotPreviewLocalRotations(TArray<FRotator>& OutRotations) const;

	UFUNCTION(BlueprintCallable, Category = "Cafe Table|AI Preview")
	void GetAISlotPreviewAnimations(TArray<TSoftObjectPtr<UAnimationAsset>>& OutAnimations) const;

	UFUNCTION(BlueprintCallable, Category = "Cafe Table|Assets")
	TArray<FString> GetCafeTableOptions() const;

	UFUNCTION(BlueprintCallable, Category = "Cafe Table|Assets")
	TArray<FString> GetDiningChairOptions() const;

	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> GeneratedMeshComponents;

	UPROPERTY(Transient)
	TArray<FVector> AISlotSourceLocalOffsets;

#if WITH_EDITORONLY_DATA
	UPROPERTY(Transient)
	TArray<TObjectPtr<USphereComponent>> AISlotPreviewComponents;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UArrowComponent>> AISlotPreviewArrowComponents;

#endif

	FBox GeneratedBounds;

	void ClearGenerated();
	void AddFurnitureItem(const FString& CategoryFolder, const FString& ItemName, const TCHAR* Prefix, const FTransform& ItemTransform);
	void AddChairLayout(const FString& ResolvedChairItem);
	void AddChairPair(const FString& ResolvedChairItem, float XOffsetCm);
	UInstancedStaticMeshComponent* FindOrCreateMeshBucket(const FSoftObjectPath& MeshPath, const TCHAR* Prefix);
	TArray<FString> GetFurnitureItemOptions(const FString& CategoryFolder) const;
	TArray<FSoftObjectPath> GetMeshPathsForFurnitureItem(const FString& CategoryFolder, const FString& ItemName) const;
	FString ResolveSelectedItem(const FString& CategoryFolder, const FString& SelectedItem) const;
	void ExpandGeneratedBounds(UStaticMesh* Mesh, const FTransform& Transform);
	void UpdateSelectionBounds();
	void AddAISlotSource(const FVector& LocalOffset);
	void RefreshAISlotPreviewComponents();
};
