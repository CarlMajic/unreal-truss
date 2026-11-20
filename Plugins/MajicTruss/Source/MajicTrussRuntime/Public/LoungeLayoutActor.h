#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LoungeLayoutActor.generated.h"

class UBoxComponent;
class UArrowComponent;
class UInstancedStaticMeshComponent;
class USkeletalMeshComponent;
class USphereComponent;
class UAnimationAsset;
class USkeletalMesh;
class UStaticMesh;

UENUM(BlueprintType)
enum class ELoungeChairMirrorMode : uint8
{
	None UMETA(DisplayName = "None"),
	MirrorEveryOtherChairAcrossX UMETA(DisplayName = "Mirror X"),
	MirrorEveryOtherChairAcrossY UMETA(DisplayName = "Mirror Y"),
	MirrorEveryOtherChairAcrossXAndY UMETA(DisplayName = "Mirror X And Y")
};

struct FLoungeMirroredTransform
{
	FLoungeMirroredTransform() = default;
	FLoungeMirroredTransform(const FTransform& InTransform, ELoungeChairMirrorMode InAppliedMirrorMode)
		: Transform(InTransform)
		, AppliedMirrorMode(InAppliedMirrorMode)
	{
	}

	FTransform Transform = FTransform::Identity;
	ELoungeChairMirrorMode AppliedMirrorMode = ELoungeChairMirrorMode::None;
};

USTRUCT(BlueprintType)
struct FLoungeLayoutBuildDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge")
	FString SofaItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge")
	FString ChairItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge")
	FString CocktailTableItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge")
	FString EndTableItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge")
	FString LampItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge")
	FString AccentItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge", meta = (ClampMin = "0"))
	int32 SofaCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge", meta = (ClampMin = "0"))
	int32 ChairCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge", meta = (ClampMin = "0"))
	int32 EndTableCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge", meta = (ClampMin = "0"))
	int32 LampCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge", meta = (ClampMin = "0"))
	int32 AccentCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge")
	bool bUseTwoSofas = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge")
	bool bUseFourChairs = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge", meta = (Units = "cm"))
	float EndTableOffsetXCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge")
	ELoungeChairMirrorMode ChairMirrorMode = ELoungeChairMirrorMode::MirrorEveryOtherChairAcrossX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge")
	ELoungeChairMirrorMode EndTableMirrorMode = ELoungeChairMirrorMode::MirrorEveryOtherChairAcrossX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge")
	ELoungeChairMirrorMode LampMirrorMode = ELoungeChairMirrorMode::MirrorEveryOtherChairAcrossX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge")
	ELoungeChairMirrorMode AccentMirrorMode = ELoungeChairMirrorMode::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge", meta = (Units = "deg"))
	float MirroredChairYawOffsetDegrees = 20.0f;
};

UCLASS(BlueprintType, meta = (DisplayName = "Lounge Layout Actor"))
class MAJICTRUSSRUNTIME_API ALoungeLayoutActor : public AActor
{
	GENERATED_BODY()

public:
	ALoungeLayoutActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lounge")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lounge")
	TObjectPtr<UBoxComponent> SelectionBounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Assets")
	FString SofaAssetFolder = TEXT("/Game/Furniture/Sofas");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Assets")
	FString ChairAssetFolder = TEXT("/Game/Furniture/Lounge_Chairs");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Assets")
	FString CocktailTableAssetFolder = TEXT("/Game/Furniture/Cocktail_Tables");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Assets")
	FString EndTableAssetFolder = TEXT("/Game/Furniture/End_Tables");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Assets")
	FString LampAssetFolder = TEXT("/Game/Furniture/Lamps");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Assets")
	FString AccentAssetFolder = TEXT("/Game/Furniture/Accents");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Assets", meta = (GetOptions = "GetSofaOptions"))
	FString SofaItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Assets", meta = (GetOptions = "GetChairOptions"))
	FString ChairItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Assets", meta = (GetOptions = "GetCocktailTableOptions"))
	FString CocktailTableItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Assets", meta = (GetOptions = "GetEndTableOptions"))
	FString EndTableItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Assets", meta = (GetOptions = "GetLampOptions"))
	FString LampItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Assets", meta = (GetOptions = "GetAccentOptions"))
	FString AccentItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Layout", meta = (ClampMin = "0"))
	int32 SofaCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Layout", meta = (ClampMin = "0"))
	int32 ChairCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Layout", meta = (ClampMin = "0"))
	int32 EndTableCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Layout", meta = (ClampMin = "0"))
	int32 LampCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Layout", meta = (ClampMin = "0"))
	int32 AccentCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Layout")
	bool bUseTwoSofas = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Layout")
	bool bUseFourChairs = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Layout")
	bool bUseAuthoredCommonOrigin = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Layout", meta = (EditCondition = "!bUseAuthoredCommonOrigin"))
	bool bCenterRowsOnActor = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Layout", meta = (Units = "cm", EditCondition = "!bUseAuthoredCommonOrigin"))
	float SofaDistanceNorthCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Layout", meta = (Units = "cm", EditCondition = "!bUseAuthoredCommonOrigin"))
	float ChairDistanceSouthCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Layout", meta = (Units = "cm", ClampMin = "0.0", EditCondition = "!bUseAuthoredCommonOrigin"))
	float SofaSpacingCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Layout", meta = (Units = "cm", ClampMin = "0.0", EditCondition = "!bUseAuthoredCommonOrigin"))
	float ChairSpacingCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Layout", meta = (DisplayName = "End Table X Offset Cm", Units = "cm"))
	float EndTableOffsetXCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Layout")
	ELoungeChairMirrorMode ChairMirrorMode = ELoungeChairMirrorMode::MirrorEveryOtherChairAcrossX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Layout")
	ELoungeChairMirrorMode EndTableMirrorMode = ELoungeChairMirrorMode::MirrorEveryOtherChairAcrossX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Layout")
	ELoungeChairMirrorMode LampMirrorMode = ELoungeChairMirrorMode::MirrorEveryOtherChairAcrossX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Layout")
	ELoungeChairMirrorMode AccentMirrorMode = ELoungeChairMirrorMode::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Layout", meta = (DisplayName = "Mirrored Chair Yaw Offset", Units = "deg"))
	float MirroredChairYawOffsetDegrees = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge")
	bool bBuildOnConstruction = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Placement")
	FRotator SofaRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Placement")
	FRotator ChairRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Placement")
	FRotator CocktailTableRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Placement")
	FRotator EndTableRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Placement")
	FRotator LampRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Placement")
	FRotator AccentRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Placement")
	FVector SofaScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Placement")
	FVector ChairScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Placement")
	FVector CocktailTableScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Placement")
	FVector EndTableScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Placement")
	FVector LampScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|Placement")
	FVector AccentScale = FVector::OneVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lounge|Debug")
	int32 CurrentGeneratedMeshCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|AI Preview")
	bool bShowAISlotPreviewInEditor = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|AI Preview", meta = (ClampMin = "0.0", Units = "cm"))
	float AISlotPreviewSpreadRadiusCm = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|AI Preview", meta = (Units = "cm"))
	FVector AISlotPreviewOffsetCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|AI Preview")
	TArray<FVector> AISlotPerSlotOffsetsCm;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|AI Preview")
	TArray<FRotator> AISlotPerSlotRotationOffsets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|AI Animation Preview")
	bool bShowAIAnimationPreviewInEditor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|AI Animation Preview", meta = (EditCondition = "bShowAIAnimationPreviewInEditor"))
	TSoftObjectPtr<USkeletalMesh> AIAnimationPreviewMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|AI Animation Preview", meta = (EditCondition = "bShowAIAnimationPreviewInEditor"))
	TSoftObjectPtr<UAnimationAsset> AIAnimationPreviewDefaultAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|AI Animation Preview", meta = (EditCondition = "bShowAIAnimationPreviewInEditor"))
	TArray<TSoftObjectPtr<UAnimationAsset>> AIAnimationPreviewPerSlotAnimations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|AI Animation Preview", meta = (Units = "cm", EditCondition = "bShowAIAnimationPreviewInEditor"))
	FVector AIAnimationPreviewMeshOffsetCm = FVector(0.0f, 0.0f, -90.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|AI Animation Preview", meta = (EditCondition = "bShowAIAnimationPreviewInEditor"))
	FRotator AIAnimationPreviewMeshRotation = FRotator(0.0f, -90.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|AI Animation Preview", meta = (EditCondition = "bShowAIAnimationPreviewInEditor"))
	FVector AIAnimationPreviewMeshScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|AI Preview", meta = (Units = "deg"))
	float AISlotPreviewStartAngleDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|AI Preview")
	bool bUseRowBasedAISlotPreview = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|AI Preview", meta = (Units = "cm", EditCondition = "bUseRowBasedAISlotPreview"))
	FVector SofaAISlotPreviewOffsetCm = FVector(0.0f, -150.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|AI Preview", meta = (ClampMin = "0.0", Units = "cm", EditCondition = "bUseRowBasedAISlotPreview"))
	float SofaAISlotPreviewSpacingCm = 113.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|AI Preview", meta = (Units = "cm", EditCondition = "bUseRowBasedAISlotPreview"))
	FVector ChairAISlotPreviewOffsetCm = FVector(0.0f, 150.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|AI Preview", meta = (ClampMin = "0.0", Units = "cm", EditCondition = "bUseRowBasedAISlotPreview"))
	float ChairAISlotPreviewSpacingCm = 113.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|AI Preview", meta = (Units = "cm"))
	FVector AISlotFacingLocalPointCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lounge|AI Preview", meta = (Units = "deg"))
	float AISlotFacingYawOffsetDegrees = 0.0f;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Lounge")
	void RebuildLoungeLayout();

	UFUNCTION(BlueprintCallable, Category = "Lounge")
	void ApplyBuildDefinition(const FLoungeLayoutBuildDefinition& Definition, bool bRebuildNow = true);

	UFUNCTION(BlueprintPure, Category = "Lounge")
	FLoungeLayoutBuildDefinition GetBuildDefinition() const;

	UFUNCTION(BlueprintCallable, Category = "Lounge")
	void SetSelectionHighlighted(bool bHighlighted);

	UFUNCTION(BlueprintCallable, Category = "Lounge|AI Preview")
	void GetAISlotPreviewLocalOffsets(TArray<FVector>& OutOffsets) const;

	UFUNCTION(BlueprintCallable, Category = "Lounge|AI Preview")
	void GetAISlotPreviewLocalRotations(TArray<FRotator>& OutRotations) const;

	UFUNCTION(BlueprintCallable, Category = "Lounge|AI Preview")
	void GetAISlotPreviewAnimations(TArray<TSoftObjectPtr<UAnimationAsset>>& OutAnimations) const;

	UFUNCTION(BlueprintCallable, Category = "Lounge|Assets")
	TArray<FString> GetSofaOptions() const;

	UFUNCTION(BlueprintCallable, Category = "Lounge|Assets")
	TArray<FString> GetChairOptions() const;

	UFUNCTION(BlueprintCallable, Category = "Lounge|Assets")
	TArray<FString> GetCocktailTableOptions() const;

	UFUNCTION(BlueprintCallable, Category = "Lounge|Assets")
	TArray<FString> GetEndTableOptions() const;

	UFUNCTION(BlueprintCallable, Category = "Lounge|Assets")
	TArray<FString> GetLampOptions() const;

	UFUNCTION(BlueprintCallable, Category = "Lounge|Assets")
	TArray<FString> GetAccentOptions() const;

	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> GeneratedMeshComponents;

	UPROPERTY(Transient)
	TArray<FVector> AISlotSourceLocalOffsets;

	UPROPERTY(Transient)
	int32 AISlotPreviewSofaCount = 0;

	UPROPERTY(Transient)
	int32 AISlotPreviewChairCount = 0;

#if WITH_EDITORONLY_DATA
	UPROPERTY(Transient)
	TArray<TObjectPtr<USphereComponent>> AISlotPreviewComponents;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UArrowComponent>> AISlotPreviewArrowComponents;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USkeletalMeshComponent>> AIAnimationPreviewComponents;
#endif

	FBox GeneratedBounds;

	void ClearGenerated();
	void AddFurnitureItem(const FString& CategoryFolder, const FString& ItemName, const TCHAR* Prefix, const FTransform& ItemTransform, ELoungeChairMirrorMode AppliedMirrorMode = ELoungeChairMirrorMode::None);
	UInstancedStaticMeshComponent* FindOrCreateMeshBucket(const FSoftObjectPath& MeshPath, const TCHAR* Prefix, bool bReverseCulling);
	TArray<FString> GetFurnitureItemOptions(const FString& CategoryFolder) const;
	TArray<FSoftObjectPath> GetMeshPathsForFurnitureItem(const FString& CategoryFolder, const FString& ItemName) const;
	FString ResolveSelectedItem(const FString& CategoryFolder, const FString& SelectedItem) const;
	FVector GetRowLocation(int32 Index, int32 Count, float SpacingCm, float YCm) const;
	TArray<FLoungeMirroredTransform> GetMirroredTransforms(const FVector& BaseLocation, const FRotator& Rotation, const FVector& BaseScale, ELoungeChairMirrorMode MirrorMode, int32 RequestedCount, bool bAlwaysKeepOriginal, float MirroredYawOffsetDegrees = 0.0f) const;
	FVector GetMirroredLocationForIndex(const FVector& BaseLocation, ELoungeChairMirrorMode MirrorMode, int32 Index, int32 Count) const;
	FRotator GetMirroredRotationForIndex(const FRotator& BaseRotation, ELoungeChairMirrorMode MirrorMode, int32 Index, int32 Count, float MirroredYawOffsetDegrees) const;
	bool ShouldMirrorIndex(ELoungeChairMirrorMode MirrorMode, int32 Index, int32 Count) const;
	void AddMeshInstance(UInstancedStaticMeshComponent* Component, UStaticMesh* Mesh, const FTransform& InstanceTransform, ELoungeChairMirrorMode AppliedMirrorMode);
	void ExpandGeneratedBounds(UStaticMesh* Mesh, const FTransform& Transform);
	void UpdateSelectionBounds();
	void AddAISlotSource(const FVector& LocalOffset);
	void RefreshAISlotPreviewComponents();
	void RefreshAIAnimationPreviewComponents(const TArray<FVector>& PreviewOffsets);
};
