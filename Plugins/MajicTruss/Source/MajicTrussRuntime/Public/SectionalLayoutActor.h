#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SectionalLayoutActor.generated.h"

class UBoxComponent;
class UArrowComponent;
class UInstancedStaticMeshComponent;
class USkeletalMeshComponent;
class USphereComponent;
class UAnimationAsset;
class USkeletalMesh;
class UStaticMesh;

UENUM(BlueprintType)
enum class ESectionalLayoutShape : uint8
{
	OneSided UMETA(DisplayName = "One Sided"),
	TwoSidedL UMETA(DisplayName = "Two Sided L"),
	ThreeSidedU UMETA(DisplayName = "Three Sided U"),
	FourSidedBox UMETA(DisplayName = "Four Sided Box")
};

USTRUCT(BlueprintType)
struct FSectionalLayoutBuildDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional")
	FString ArmlessItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional")
	FString CornerItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional")
	ESectionalLayoutShape Shape = ESectionalLayoutShape::TwoSidedL;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional", meta = (ClampMin = "1"))
	int32 WidthSections = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional", meta = (ClampMin = "1"))
	int32 DepthSections = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional", meta = (Units = "cm"))
	float SectionSpacingCm = 55.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional")
	bool bUseArmlessItemDefaultSpacing = true;
};

UCLASS(BlueprintType, meta = (DisplayName = "Sectional Layout Actor"))
class MAJICTRUSSRUNTIME_API ASectionalLayoutActor : public AActor
{
	GENERATED_BODY()

public:
	ASectionalLayoutActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sectional")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sectional")
	TObjectPtr<UBoxComponent> SelectionBounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|Assets")
	FString ArmlessAssetFolder = TEXT("/Game/Furniture/Sectionals/Armless");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|Assets")
	FString CornerAssetFolder = TEXT("/Game/Furniture/Sectionals/Corners");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|Assets", meta = (GetOptions = "GetArmlessOptions"))
	FString ArmlessItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|Assets", meta = (GetOptions = "GetCornerOptions"))
	FString CornerItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|Layout")
	ESectionalLayoutShape Shape = ESectionalLayoutShape::TwoSidedL;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|Layout", meta = (ClampMin = "1"))
	int32 WidthSections = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|Layout", meta = (ClampMin = "1"))
	int32 DepthSections = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|Layout", meta = (Units = "cm"))
	float SectionSpacingCm = 55.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|Layout")
	bool bUseArmlessItemDefaultSpacing = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|Layout")
	bool bCenterOnActor = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional")
	bool bBuildOnConstruction = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|Collision")
	bool bBlockPawnCollision = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|Collision")
	bool bAffectNavigation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|Placement", meta = (Units = "cm"))
	FVector ArmlessPlacementOffsetCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|Placement")
	bool bRotateArmlessOffsetWithPiece = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|Placement")
	FRotator ArmlessPlacementRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|Placement")
	FVector ArmlessScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|Placement", meta = (Units = "cm"))
	FVector CornerPlacementOffsetCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|Placement")
	bool bRotateCornerOffsetWithPiece = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|Placement")
	FRotator CornerPlacementRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|Placement")
	FVector CornerScale = FVector::OneVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sectional|Debug")
	int32 CurrentGeneratedMeshCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|AI Preview")
	bool bShowAISlotPreviewInEditor = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|AI Preview", meta = (Units = "cm"))
	FVector AISlotPreviewOffsetCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|AI Preview")
	TArray<FVector> AISlotPerSlotOffsetsCm;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|AI Preview")
	TArray<FRotator> AISlotPerSlotRotationOffsets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|AI Preview", meta = (Units = "cm"))
	FVector FrontAISlotOffsetCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|AI Preview", meta = (Units = "cm"))
	FVector LeftAISlotOffsetCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|AI Preview", meta = (Units = "cm"))
	FVector RightAISlotOffsetCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|AI Preview", meta = (Units = "cm"))
	FVector BackAISlotOffsetCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|AI Preview", meta = (ClampMin = "0.0", Units = "cm"))
	float AIEntryOffsetFromFootprintCm = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|AI Preview", meta = (Units = "cm"))
	FVector AIEntryPreviewOffsetCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|AI Preview", meta = (Units = "cm"))
	FVector AISlotFacingLocalPointCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|AI Preview", meta = (Units = "deg"))
	float AISlotFacingYawOffsetDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|AI Animation Preview")
	bool bShowAIAnimationPreviewInEditor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|AI Animation Preview", meta = (EditCondition = "bShowAIAnimationPreviewInEditor"))
	TSoftObjectPtr<USkeletalMesh> AIAnimationPreviewMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|AI Animation Preview", meta = (EditCondition = "bShowAIAnimationPreviewInEditor"))
	TSoftObjectPtr<UAnimationAsset> AIAnimationPreviewDefaultAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|AI Animation Preview", meta = (EditCondition = "bShowAIAnimationPreviewInEditor"))
	TArray<TSoftObjectPtr<UAnimationAsset>> AIAnimationPreviewPerSlotAnimations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|AI Animation Preview", meta = (Units = "cm", EditCondition = "bShowAIAnimationPreviewInEditor"))
	FVector AIAnimationPreviewMeshOffsetCm = FVector(0.0f, 0.0f, -90.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|AI Animation Preview", meta = (EditCondition = "bShowAIAnimationPreviewInEditor"))
	FRotator AIAnimationPreviewMeshRotation = FRotator(0.0f, -90.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sectional|AI Animation Preview", meta = (EditCondition = "bShowAIAnimationPreviewInEditor"))
	FVector AIAnimationPreviewMeshScale = FVector::OneVector;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Sectional")
	void RebuildSectionalLayout();

	UFUNCTION(BlueprintCallable, Category = "Sectional")
	void ApplyBuildDefinition(const FSectionalLayoutBuildDefinition& Definition, bool bRebuildNow = true);

	UFUNCTION(BlueprintPure, Category = "Sectional")
	FSectionalLayoutBuildDefinition GetBuildDefinition() const;

	UFUNCTION(BlueprintCallable, Category = "Sectional")
	void SetSelectionHighlighted(bool bHighlighted);

	UFUNCTION(BlueprintCallable, Category = "Sectional|AI Preview")
	void GetAISlotPreviewLocalOffsets(TArray<FVector>& OutOffsets) const;

	UFUNCTION(BlueprintCallable, Category = "Sectional|AI Preview")
	void GetAISlotPreviewLocalRotations(TArray<FRotator>& OutRotations) const;

	UFUNCTION(BlueprintCallable, Category = "Sectional|AI Preview")
	void GetAIEntryPreviewLocalOffsets(TArray<FVector>& OutOffsets) const;

	UFUNCTION(BlueprintCallable, Category = "Sectional|AI Preview")
	void GetAISlotPreviewAnimations(TArray<TSoftObjectPtr<UAnimationAsset>>& OutAnimations) const;

	UFUNCTION(BlueprintCallable, Category = "Sectional|Assets")
	TArray<FString> GetArmlessOptions() const;

	UFUNCTION(BlueprintCallable, Category = "Sectional|Assets")
	TArray<FString> GetCornerOptions() const;

	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> GeneratedMeshComponents;

	UPROPERTY(Transient)
	TArray<FVector> AISlotPreviewLocalOffsets;

	UPROPERTY(Transient)
	TArray<FVector> AIEntryPreviewLocalOffsets;

#if WITH_EDITORONLY_DATA
	UPROPERTY(Transient)
	TArray<TObjectPtr<USphereComponent>> AISlotPreviewComponents;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UArrowComponent>> AISlotPreviewArrowComponents;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USphereComponent>> AIEntryPreviewComponents;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USkeletalMeshComponent>> AIAnimationPreviewComponents;
#endif

	FBox GeneratedBounds;

	void ClearGenerated();
	void AddFurnitureItem(const FString& CategoryFolder, const FString& ItemName, const TCHAR* Prefix, const FTransform& ItemTransform);
	UInstancedStaticMeshComponent* FindOrCreateMeshBucket(const FSoftObjectPath& MeshPath, const TCHAR* Prefix);
	TArray<FString> GetFurnitureItemOptions(const FString& CategoryFolder) const;
	TArray<FSoftObjectPath> GetMeshPathsForFurnitureItem(const FString& CategoryFolder, const FString& ItemName) const;
	FString ResolveSelectedItem(const FString& CategoryFolder, const FString& SelectedItem) const;
	float GetDefaultSpacingForArmlessItem(const FString& ResolvedArmlessItem) const;
	void AddArmlessAt(const FString& ResolvedArmlessItem, const FVector& GridLocation, float YawDegrees);
	void AddCornerAt(const FString& ResolvedCornerItem, const FVector& GridLocation, float YawDegrees);
	FVector ApplyFootprintCentering(const FVector& GridLocation) const;
	bool HasLeftSide() const;
	bool HasRightSide() const;
	bool HasBackSide() const;
	void ExpandGeneratedBounds(UStaticMesh* Mesh, const FTransform& Transform);
	void UpdateSelectionBounds();
	void AddAISlotPreviewSource(const FVector& LocalOffset);
	void AddAIEntryPreviewSource(const FVector& LocalOffset);
	void BuildAIEntryPreviewSources();
	void RefreshAISlotPreviewComponents();
	void RefreshAIAnimationPreviewComponents(const TArray<FVector>& PreviewOffsets);
	void ClearEditorPreviewComponents();
};
