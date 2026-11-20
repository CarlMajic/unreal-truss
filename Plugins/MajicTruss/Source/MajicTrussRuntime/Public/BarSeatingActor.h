#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BarSeatingActor.generated.h"

class UBoxComponent;
class UArrowComponent;
class UChildActorComponent;
class UInstancedStaticMeshComponent;
class USkeletalMeshComponent;
class USphereComponent;
class UAnimationAsset;
class USkeletalMesh;
class UStaticMesh;

UENUM(BlueprintType)
enum class EBarSeatingStoolLayout : uint8
{
	NoStools UMETA(DisplayName = "No Stools"),
	TwoTop UMETA(DisplayName = "Square/Round Two Top"),
	FourTop UMETA(DisplayName = "Square/Round Four Top"),
	RectangleFourTop UMETA(DisplayName = "Rectangle Four Top"),
	RectangleSixTop UMETA(DisplayName = "Rectangle Six Top")
};

USTRUCT(BlueprintType)
struct FBarSeatingBuildDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating")
	FString BarTableItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating")
	FString StoolItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating")
	EBarSeatingStoolLayout StoolLayout = EBarSeatingStoolLayout::FourTop;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating")
	bool bClassroomStyle = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating", meta = (Units = "cm"))
	float StoolOffsetYCm = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating", meta = (Units = "cm"))
	float RectangleStoolPairOffsetXCm = 125.0f;
};

UCLASS(BlueprintType, meta = (DisplayName = "Bar Seating Actor"))
class MAJICTRUSSRUNTIME_API ABarSeatingActor : public AActor
{
	GENERATED_BODY()

public:
	ABarSeatingActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bar Seating")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bar Seating")
	TObjectPtr<UBoxComponent> SelectionBounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating|Assets")
	FString BarTableAssetFolder = TEXT("/Game/Furniture/Bar_Tables");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating|Assets")
	FString StoolAssetFolder = TEXT("/Game/Furniture/Stools");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating|Assets", meta = (GetOptions = "GetBarTableOptions"))
	FString BarTableItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating|Assets", meta = (GetOptions = "GetStoolOptions"))
	FString StoolItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating")
	bool bBuildOnConstruction = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating|Layout")
	EBarSeatingStoolLayout StoolLayout = EBarSeatingStoolLayout::FourTop;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating|Layout")
	bool bClassroomStyle = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating|Layout", meta = (DisplayName = "Stool Y Offset Cm", Units = "cm"))
	float StoolOffsetYCm = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating|Layout", meta = (DisplayName = "Rectangle Stool Pair X Offset Cm", Units = "cm", EditCondition = "StoolLayout == EBarSeatingStoolLayout::RectangleFourTop || StoolLayout == EBarSeatingStoolLayout::RectangleSixTop"))
	float RectangleStoolPairOffsetXCm = 125.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating|Placement")
	FRotator BarTableRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating|Placement")
	FRotator StoolRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating|Placement")
	FVector BarTableScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating|Placement")
	FVector StoolScale = FVector::OneVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bar Seating|Debug")
	int32 CurrentGeneratedMeshCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating|AI Preview")
	bool bShowAISlotPreviewInEditor = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating|AI Preview", meta = (ClampMin = "0.0", Units = "cm"))
	float AISlotPreviewSpreadRadiusCm = 55.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating|AI Preview", meta = (ClampMin = "0.0", Units = "cm"))
	float RectangleAISlotHalfWidthCm = 55.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating|AI Preview", meta = (Units = "cm"))
	FVector AISlotPreviewOffsetCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating|AI Preview")
	TArray<FVector> AISlotPerSlotOffsetsCm;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating|AI Preview")
	TArray<FRotator> AISlotPerSlotRotationOffsets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating|AI Animation Preview")
	bool bShowAIAnimationPreviewInEditor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating|AI Animation Preview", meta = (EditCondition = "bShowAIAnimationPreviewInEditor"))
	TSoftObjectPtr<USkeletalMesh> AIAnimationPreviewMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating|AI Animation Preview", meta = (EditCondition = "bShowAIAnimationPreviewInEditor"))
	TSoftObjectPtr<UAnimationAsset> AIAnimationPreviewDefaultAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating|AI Animation Preview", meta = (EditCondition = "bShowAIAnimationPreviewInEditor"))
	TArray<TSoftObjectPtr<UAnimationAsset>> AIAnimationPreviewPerSlotAnimations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating|AI Animation Preview", meta = (Units = "cm", EditCondition = "bShowAIAnimationPreviewInEditor"))
	FVector AIAnimationPreviewMeshOffsetCm = FVector(0.0f, 0.0f, -90.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating|AI Animation Preview", meta = (EditCondition = "bShowAIAnimationPreviewInEditor"))
	FRotator AIAnimationPreviewMeshRotation = FRotator(0.0f, -90.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating|AI Animation Preview", meta = (EditCondition = "bShowAIAnimationPreviewInEditor"))
	FVector AIAnimationPreviewMeshScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating|AI Preview", meta = (Units = "deg"))
	float AISlotPreviewStartAngleDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating|AI Preview", meta = (Units = "cm"))
	FVector AISlotFacingLocalPointCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating|AI Preview", meta = (Units = "deg"))
	float AISlotFacingYawOffsetDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating|Ambient Attendees")
	bool bSpawnAmbientAttendees = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating|Ambient Attendees", meta = (EditCondition = "bSpawnAmbientAttendees"))
	TArray<TSoftClassPtr<AActor>> AmbientAttendeeBlueprints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating|Ambient Attendees", meta = (EditCondition = "bSpawnAmbientAttendees"))
	TArray<TSoftObjectPtr<UAnimationAsset>> AmbientAttendeeAnimations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating|Ambient Attendees", meta = (Units = "cm", EditCondition = "bSpawnAmbientAttendees"))
	FVector AmbientAttendeeMeshOffsetCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating|Ambient Attendees", meta = (Units = "cm", EditCondition = "bSpawnAmbientAttendees"))
	float AmbientAttendeeRadialOffsetCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating|Ambient Attendees", meta = (EditCondition = "bSpawnAmbientAttendees"))
	FRotator AmbientAttendeeMeshRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating|Ambient Attendees", meta = (EditCondition = "bSpawnAmbientAttendees"))
	FVector AmbientAttendeeMeshScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar Seating|Ambient Attendees", meta = (ClampMin = "0", EditCondition = "bSpawnAmbientAttendees"))
	int32 AmbientAttendeeRandomSeed = 1337;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Bar Seating")
	void RebuildBarSeating();

	UFUNCTION(BlueprintCallable, Category = "Bar Seating")
	void ApplyBuildDefinition(const FBarSeatingBuildDefinition& Definition, bool bRebuildNow = true);

	UFUNCTION(BlueprintPure, Category = "Bar Seating")
	FBarSeatingBuildDefinition GetBuildDefinition() const;

	UFUNCTION(BlueprintCallable, Category = "Bar Seating")
	void SetSelectionHighlighted(bool bHighlighted);

	UFUNCTION(BlueprintCallable, Category = "Bar Seating|AI Preview")
	void GetAISlotPreviewLocalOffsets(TArray<FVector>& OutOffsets) const;

	UFUNCTION(BlueprintCallable, Category = "Bar Seating|AI Preview")
	void GetAISlotPreviewLocalRotations(TArray<FRotator>& OutRotations) const;

	UFUNCTION(BlueprintCallable, Category = "Bar Seating|AI Preview")
	void GetAISlotPreviewAnimations(TArray<TSoftObjectPtr<UAnimationAsset>>& OutAnimations) const;

	UFUNCTION(BlueprintCallable, Category = "Bar Seating|Assets")
	TArray<FString> GetBarTableOptions() const;

	UFUNCTION(BlueprintCallable, Category = "Bar Seating|Assets")
	TArray<FString> GetStoolOptions() const;

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
	TArray<FRotator> AISlotSourceLocalRotations;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UChildActorComponent>> AmbientAttendeeComponents;

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
	void AddFurnitureItem(const FString& CategoryFolder, const FString& ItemName, const TCHAR* Prefix, const FTransform& ItemTransform);
	void AddStoolLayout(const FString& ResolvedStoolItem);
	UInstancedStaticMeshComponent* FindOrCreateMeshBucket(const FSoftObjectPath& MeshPath, const TCHAR* Prefix);
	TArray<FString> GetFurnitureItemOptions(const FString& CategoryFolder) const;
	TArray<FSoftObjectPath> GetMeshPathsForFurnitureItem(const FString& CategoryFolder, const FString& ItemName) const;
	FString ResolveSelectedItem(const FString& CategoryFolder, const FString& SelectedItem) const;
	void AddStoolPair(const FString& ResolvedStoolItem, float XOffsetCm);
	void ExpandGeneratedBounds(UStaticMesh* Mesh, const FTransform& Transform);
	void UpdateSelectionBounds();
	void AddAISlotSource(const FVector& LocalOffset, const FRotator& LocalRotation = FRotator::ZeroRotator);
	void RefreshAISlotPreviewComponents();
	void RefreshAIAnimationPreviewComponents(const TArray<FVector>& PreviewOffsets);
	void RefreshAmbientAttendeeComponents(const TArray<FVector>& PreviewOffsets, const TArray<FRotator>& PreviewRotations);
	void PlayAmbientAttendeeAnimation(UChildActorComponent* AttendeeComponent, UAnimationAsset* Animation) const;
};
