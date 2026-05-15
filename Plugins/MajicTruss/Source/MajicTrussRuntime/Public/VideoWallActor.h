#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VideoWallActor.generated.h"

class UBoxComponent;
class UInstancedStaticMeshComponent;

UENUM(BlueprintType)
enum class EVideoWallSupportMode : uint8
{
	GroundStacked UMETA(DisplayName = "Ground Stacked"),
	Flown UMETA(DisplayName = "Flown")
};

UENUM(BlueprintType)
enum class EVideoWallSupportSpacing : uint8
{
	OneMeter UMETA(DisplayName = "1000 mm"),
	OnePointFiveMeters UMETA(DisplayName = "1500 mm")
};

USTRUCT(BlueprintType)
struct FVideoWallBuildDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall", meta = (ClampMin = "1"))
	int32 Columns = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall", meta = (ClampMin = "1"))
	int32 Rows = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall")
	EVideoWallSupportMode SupportMode = EVideoWallSupportMode::GroundStacked;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall")
	EVideoWallSupportSpacing SupportSpacing = EVideoWallSupportSpacing::OneMeter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall", meta = (Units = "cm", ClampMin = "1.0"))
	float PanelWidthCm = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall", meta = (Units = "cm", ClampMin = "1.0"))
	float PanelHeightCm = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall", meta = (Units = "cm"))
	float PanelGapCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall")
	bool bCenterOnActor = false;
};

UCLASS(BlueprintType, meta = (DisplayName = "Video Wall Actor"))
class MAJICTRUSSRUNTIME_API AVideoWallActor : public AActor
{
	GENERATED_BODY()

public:
	AVideoWallActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Video Wall")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Video Wall")
	TObjectPtr<UBoxComponent> SelectionBounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall", meta = (DisplayName = "Panel Columns", ClampMin = "1"))
	int32 Columns = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall", meta = (DisplayName = "Panel Rows", ClampMin = "1"))
	int32 Rows = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall")
	EVideoWallSupportMode SupportMode = EVideoWallSupportMode::GroundStacked;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall")
	EVideoWallSupportSpacing SupportSpacing = EVideoWallSupportSpacing::OneMeter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall", meta = (Units = "cm", ClampMin = "1.0"))
	float PanelWidthCm = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall", meta = (Units = "cm", ClampMin = "1.0"))
	float PanelHeightCm = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall", meta = (Units = "cm"))
	float PanelGapCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall")
	bool bCenterOnActor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall")
	bool bBuildOnConstruction = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall|Ground Support", meta = (Units = "cm", ClampMin = "1.0"))
	float StackerLevelHeightCm = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall|Ground Support")
	bool bAddRightEdgeTower = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall|H Tube")
	bool bBuildHTubes = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall|H Tube", meta = (Units = "cm"))
	FVector HTubeAssemblyOffset1000mmCm = FVector(-50.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall|H Tube", meta = (Units = "cm"))
	FVector HTubeAssemblyOffset1500mmCm = FVector(-75.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall|H Tube", meta = (Units = "cm"))
	FVector HTubeInnerPartsRelativeOffset1000mmCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall|H Tube", meta = (Units = "cm"))
	FVector HTubeInnerPartsRelativeOffset1500mmCm = FVector(50.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall|Placement", meta = (Units = "cm"))
	FVector PanelPlacementOffsetCm = FVector(-25.0f, -4.5f, -25.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall|Placement")
	FRotator PanelPlacementRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall|Placement")
	FVector PanelScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall|Placement", meta = (Units = "cm"))
	FVector BracketPlacementOffsetCm = FVector(-50.0f, -4.5f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall|Placement")
	FRotator BracketPlacementRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall|Placement")
	FVector BracketScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall|Placement", meta = (Units = "cm"))
	FVector FlownBracketPlacementOffsetCm = FVector(101.0f, -4.5f, 28.5f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall|Placement", meta = (DisplayName = "Flown Bracket Placement Rotation"))
	FRotator FlownBracketPlacementRotation = FRotator(0.0f, 180.0f, 180.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall|Placement")
	FVector FlownBracketScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall|Placement", meta = (Units = "cm"))
	FVector SupportSkyPlacementOffsetCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall|Placement")
	FRotator SupportSkyPlacementRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall|Placement")
	FVector SupportSkyScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall|Placement", meta = (Units = "cm"))
	FVector StackerPlacementOffsetCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall|Placement")
	FRotator StackerPlacementRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall|Placement")
	FVector StackerScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall|Placement", meta = (Units = "cm"))
	FVector HTubePlacementOffsetCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall|Placement")
	FRotator HTubePlacementRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video Wall|Placement")
	FVector HTubeScale = FVector::OneVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Video Wall|Debug", meta = (Units = "cm"))
	float CurrentWallWidthCm = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Video Wall|Debug", meta = (Units = "cm"))
	float CurrentWallHeightCm = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Video Wall|Debug")
	int32 Current1000mmBracketCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Video Wall|Debug")
	int32 Current500mmBracketCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Video Wall|Debug")
	int32 CurrentTowerCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Video Wall|Debug")
	int32 CurrentStackerLevels = 0;

	UFUNCTION(BlueprintCallable, Category = "Video Wall")
	void RebuildVideoWall();

	UFUNCTION(BlueprintCallable, Category = "Video Wall")
	void ApplyBuildDefinition(const FVideoWallBuildDefinition& Definition, bool bRebuildNow = true);

	UFUNCTION(BlueprintPure, Category = "Video Wall")
	FVideoWallBuildDefinition GetBuildDefinition() const;

	UFUNCTION(BlueprintCallable, Category = "Video Wall")
	void SetSelectionHighlighted(bool bHighlighted);

	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> GeneratedInstanceComponents;

	FBox GeneratedBounds;

	void ClearGenerated();
	void BuildPanelGrid(float WallLeftX, float WallBottomZ);
	void BuildGroundSupport(float WallLeftX, float WallRightX, float WallBottomZ);
	void BuildFlownSupport(float WallLeftX, float WallRightX, float WallTopZ);
	void BuildBracketRun(float WallLeftX, float WallBottomZ, bool bFlipped);
	void BuildTowerStack(const TArray<float>& TowerPositionsX, float WallBottomZ);
	void BuildHTubeRuns(const TArray<float>& TowerPositionsX, float WallBottomZ);
	TArray<float> GetTowerPositions(float WallLeftX, float WallRightX) const;
	float GetSupportSpacingCm() const;
	FVector GetHTubeAssemblyOffsetCm() const;
	FVector GetHTubeInnerPartsOffsetCm() const;
	void AddAssemblyInstancesFromFolder(
		const FString& MeshFolderPath,
		const FVector& Location,
		const FRotator& Rotation,
		const FVector& Scale,
		const FString& BucketPrefix);
	UInstancedStaticMeshComponent* FindOrCreateGeneratedComponent(const FSoftObjectPath& MeshPath, const FString& BucketPrefix);
	TArray<FSoftObjectPath> GetStaticMeshPathsForFolder(const FString& FolderPath) const;
	void AddInstance(UInstancedStaticMeshComponent* Component, UStaticMesh* Mesh, const FTransform& InstanceTransform);
	void ExpandGeneratedBounds(UStaticMesh* Mesh, const FTransform& Transform);
	void UpdateSelectionBounds();
};
