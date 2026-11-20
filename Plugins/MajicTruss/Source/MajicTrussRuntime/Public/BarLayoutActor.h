#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BarLayoutActor.generated.h"

class UBoxComponent;
class UInstancedStaticMeshComponent;
class UStaticMesh;

UENUM(BlueprintType)
enum class EBarLayoutShape : uint8
{
	Single UMETA(DisplayName = "Single"),
	WallLine UMETA(DisplayName = "Wall Line"),
	CenterTwoSided UMETA(DisplayName = "Center Two Sided"),
	CenterFourSided UMETA(DisplayName = "Center Four Sided"),
	CenterSixSided UMETA(DisplayName = "Center Six Sided")
};

USTRUCT(BlueprintType)
struct FBarLayoutBuildDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar")
	FString FrontBarItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar")
	FString BackBarItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar")
	bool bUseFrontBar = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar")
	bool bUseBackBar = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar")
	EBarLayoutShape LayoutShape = EBarLayoutShape::Single;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar", meta = (ClampMin = "1"))
	int32 WallBarCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar", meta = (Units = "cm"))
	float WallBarSpacingCm = 244.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar", meta = (Units = "cm"))
	float CenterTwoSidedOffsetCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar", meta = (Units = "cm"))
	float CenterFourSidedOffsetCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar", meta = (Units = "cm"))
	float CenterSixSidedOffsetCm = 0.0f;
};

UCLASS(BlueprintType, meta = (DisplayName = "Bar Layout Actor"))
class MAJICTRUSSRUNTIME_API ABarLayoutActor : public AActor
{
	GENERATED_BODY()

public:
	ABarLayoutActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bar")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bar")
	TObjectPtr<UBoxComponent> SelectionBounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar|Assets")
	FString FrontBarAssetFolder = TEXT("/Game/Furniture/Bars/Front_Bars");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar|Assets")
	FString BackBarAssetFolder = TEXT("/Game/Furniture/Bars/Back_Bars");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar|Assets", meta = (GetOptions = "GetFrontBarOptions"))
	FString FrontBarItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar|Assets", meta = (GetOptions = "GetBackBarOptions"))
	FString BackBarItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar|Assets")
	bool bUseFrontBar = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar|Assets")
	bool bUseBackBar = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar")
	bool bBuildOnConstruction = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar|Layout")
	EBarLayoutShape LayoutShape = EBarLayoutShape::Single;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar|Layout", meta = (ClampMin = "1", EditCondition = "LayoutShape == EBarLayoutShape::WallLine"))
	int32 WallBarCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar|Layout", meta = (Units = "cm", EditCondition = "LayoutShape == EBarLayoutShape::WallLine"))
	float WallBarSpacingCm = 244.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar|Layout", meta = (EditCondition = "LayoutShape == EBarLayoutShape::WallLine"))
	bool bCenterWallLineOnActor = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar|Layout", meta = (DisplayName = "Two Sided Center Offset Cm", Units = "cm", EditCondition = "LayoutShape == EBarLayoutShape::CenterTwoSided"))
	float CenterTwoSidedOffsetCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar|Layout", meta = (DisplayName = "Four Sided Center Offset Cm", Units = "cm", EditCondition = "LayoutShape == EBarLayoutShape::CenterFourSided"))
	float CenterFourSidedOffsetCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar|Layout", meta = (DisplayName = "Six Sided Center Offset Cm", Units = "cm", EditCondition = "LayoutShape == EBarLayoutShape::CenterSixSided"))
	float CenterSixSidedOffsetCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar|Placement")
	FRotator FrontBarRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar|Placement")
	FRotator BackBarRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar|Placement")
	FVector FrontBarScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bar|Placement")
	FVector BackBarScale = FVector::OneVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bar|Debug")
	int32 CurrentGeneratedMeshCount = 0;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Bar")
	void RebuildBarLayout();

	UFUNCTION(BlueprintCallable, Category = "Bar")
	void ApplyBuildDefinition(const FBarLayoutBuildDefinition& Definition, bool bRebuildNow = true);

	UFUNCTION(BlueprintPure, Category = "Bar")
	FBarLayoutBuildDefinition GetBuildDefinition() const;

	UFUNCTION(BlueprintCallable, Category = "Bar")
	void SetSelectionHighlighted(bool bHighlighted);

	UFUNCTION(BlueprintCallable, Category = "Bar|Assets")
	TArray<FString> GetFrontBarOptions() const;

	UFUNCTION(BlueprintCallable, Category = "Bar|Assets")
	TArray<FString> GetBackBarOptions() const;

	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> GeneratedMeshComponents;

	FBox GeneratedBounds;

	void ClearGenerated();
	void AddBarSet(const FString& ResolvedFrontBarItem, const FString& ResolvedBackBarItem, const FTransform& LayoutTransform);
	void AddFurnitureItem(const FString& CategoryFolder, const FString& ItemName, const TCHAR* Prefix, const FTransform& ItemTransform);
	UInstancedStaticMeshComponent* FindOrCreateMeshBucket(const FSoftObjectPath& MeshPath, const TCHAR* Prefix);
	TArray<FString> GetFurnitureItemOptions(const FString& CategoryFolder) const;
	TArray<FSoftObjectPath> GetMeshPathsForFurnitureItem(const FString& CategoryFolder, const FString& ItemName) const;
	FString ResolveSelectedItem(const FString& CategoryFolder, const FString& SelectedItem) const;
	int32 GetCenterSideCount() const;
	float GetCenterOffsetCm() const;
	void ExpandGeneratedBounds(UStaticMesh* Mesh, const FTransform& Transform);
	void UpdateSelectionBounds();
};
