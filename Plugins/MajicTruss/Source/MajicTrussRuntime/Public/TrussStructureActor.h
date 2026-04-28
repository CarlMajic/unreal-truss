#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrussInventoryDataAsset.h"
#include "TrussStructureActor.generated.h"

class UBoxComponent;
class UInstancedStaticMeshComponent;

UENUM(BlueprintType)
enum class ETrussBuildMode : uint8
{
	StraightRun UMETA(DisplayName = "Straight Run"),
	Rectangle UMETA(DisplayName = "Rectangle"),
	Arch UMETA(DisplayName = "Arch"),
	Cube UMETA(DisplayName = "Cube"),
	CubeArch UMETA(DisplayName = "Cube Arch")
};

UENUM(BlueprintType)
enum class ETrussSlingType : uint8
{
	OverSlung UMETA(DisplayName = "Over Slung"),
	UnderSlung UMETA(DisplayName = "Under Slung")
};

USTRUCT(BlueprintType)
struct FTrussBuildDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Truss")
	ETrussBuildMode BuildMode = ETrussBuildMode::StraightRun;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Straight Run", meta = (ClampMin = "2.0", Units = "ft"))
	float LengthFt = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Straight Run", meta = (ClampMin = "0.0", Units = "ft"))
	float StraightRunHeightFt = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rectangle", meta = (ClampMin = "2.0", Units = "ft"))
	float RectangleLengthFt = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rectangle", meta = (ClampMin = "2.0", Units = "ft"))
	float RectangleWidthFt = 16.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rectangle", meta = (ClampMin = "0.0", Units = "ft"))
	float RectangleHeightFt = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arch", meta = (ClampMin = "4.0", Units = "ft"))
	float ArchHeightFt = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arch", meta = (ClampMin = "4.0", Units = "ft"))
	float ArchWidthFt = 16.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube", meta = (ClampMin = "4.0", Units = "ft"))
	float CubeLengthFt = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube", meta = (ClampMin = "4.0", Units = "ft"))
	float CubeWidthFt = 16.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube", meta = (ClampMin = "4.0", Units = "ft"))
	float CubeHeightFt = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Arch", meta = (ClampMin = "8.0", Units = "ft"))
	float CubeArchWidthFt = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Arch", meta = (ClampMin = "4.0", Units = "ft"))
	float CubeArchHeightFt = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Arch")
	ETrussPieceType CubeArchSideSpacingPiece = ETrussPieceType::FourFoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Arch")
	ETrussPieceType CubeArchDepthSpacingPiece = ETrussPieceType::FourFoot;
};

USTRUCT(BlueprintType)
struct FMountedFixtureDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fixture")
	TSubclassOf<AActor> FixtureClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fixture")
	ETrussSlingType SlingType = ETrussSlingType::OverSlung;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fixture", meta = (MakeEditWidget = "true", Units = "cm"))
	FVector LocalHitLocation = FVector::ZeroVector;
};

UCLASS(BlueprintType)
class MAJICTRUSSRUNTIME_API ATrussStructureActor : public AActor
{
	GENERATED_BODY()

public:
	ATrussStructureActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Truss")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Truss")
	TObjectPtr<UBoxComponent> SelectionBounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Truss|Generated")
	TObjectPtr<UInstancedStaticMeshComponent> TenFootTrussInstances;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Truss|Generated")
	TObjectPtr<UInstancedStaticMeshComponent> EightFootTrussInstances;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Truss|Generated")
	TObjectPtr<UInstancedStaticMeshComponent> FiveFootTrussInstances;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Truss|Generated")
	TObjectPtr<UInstancedStaticMeshComponent> FourFootTrussInstances;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Truss|Generated")
	TObjectPtr<UInstancedStaticMeshComponent> TwoFootTrussInstances;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Truss|Generated")
	TObjectPtr<UInstancedStaticMeshComponent> CornerBlockInstances;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Truss|Generated")
	TObjectPtr<UInstancedStaticMeshComponent> BaseInstances;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Truss|Generated")
	TObjectPtr<UInstancedStaticMeshComponent> HingeInstances;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Truss")
	TObjectPtr<UTrussInventoryDataAsset> Inventory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Truss")
	bool bUseMajicGearDefaultMeshes = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Truss", meta = (ClampMin = "0.0001"))
	float MeshScaleMultiplier = 0.0254f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Truss")
	ETrussBuildMode BuildMode = ETrussBuildMode::StraightRun;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Straight Run", meta = (ClampMin = "2.0", Units = "ft"))
	float LengthFt = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Straight Run", meta = (ClampMin = "0.0", Units = "ft"))
	float StraightRunHeightFt = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Straight Run")
	bool bBuildOnConstruction = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rectangle", meta = (ClampMin = "2.0", Units = "ft"))
	float RectangleLengthFt = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rectangle", meta = (ClampMin = "2.0", Units = "ft"))
	float RectangleWidthFt = 16.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rectangle", meta = (ClampMin = "0.0", Units = "ft"))
	float RectangleHeightFt = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rectangle", meta = (Units = "cm"))
	float RectangleYRunXOffsetCm = 30.48f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arch", meta = (ClampMin = "4.0", Units = "ft"))
	float ArchHeightFt = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arch", meta = (ClampMin = "4.0", Units = "ft"))
	float ArchWidthFt = 16.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arch", meta = (Units = "cm"))
	float ArchCornerConnectionOffsetCm = 15.24f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arch", meta = (Units = "cm"))
	float ArchLegYOffsetCm = 15.24f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arch", meta = (Units = "cm"))
	float ArchVerticalLegXOffsetCm = 30.48f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arch", meta = (Units = "cm"))
	float ArchBaseYOffsetCm = 30.48f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arch", meta = (Units = "cm"))
	float ArchSpanYOffsetCm = 15.24f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arch", meta = (Units = "deg"))
	float ArchVerticalRotationXDeg = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arch", meta = (Units = "deg"))
	float ArchVerticalRotationYDeg = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arch", meta = (Units = "deg"))
	float ArchVerticalRotationZDeg = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube", meta = (ClampMin = "4.0", Units = "ft"))
	float CubeLengthFt = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube", meta = (ClampMin = "4.0", Units = "ft"))
	float CubeWidthFt = 16.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube", meta = (ClampMin = "4.0", Units = "ft"))
	float CubeHeightFt = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube", meta = (Units = "cm"))
	float CubeCornerConnectionOffsetCm = 15.24f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube", meta = (Units = "cm"))
	float CubeYRunXOffsetCm = 45.720001f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Arch", meta = (ClampMin = "8.0", Units = "ft"))
	float CubeArchWidthFt = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Arch", meta = (ClampMin = "4.0", Units = "ft"))
	float CubeArchHeightFt = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Arch")
	ETrussPieceType CubeArchSideSpacingPiece = ETrussPieceType::FourFoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Arch")
	ETrussPieceType CubeArchDepthSpacingPiece = ETrussPieceType::FourFoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cube Arch", meta = (Units = "cm"))
	float CubeArchLeftClusterOffsetCm = -30.48f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (ClampMin = "1.0", Units = "cm"))
	float DebugCrossSectionCm = 30.48f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rigging", meta = (ClampMin = "0.0", Units = "cm"))
	float RailInsetCm = 5.08f;

	UPROPERTY(BlueprintReadOnly, Category = "Truss", meta = (Units = "ft"))
	float LastBuiltLengthFt = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mounted Fixtures")
	TArray<FMountedFixtureDefinition> MountedFixtures;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mounted Fixtures|Editor")
	TSubclassOf<AActor> EditorFixtureClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mounted Fixtures|Editor")
	ETrussSlingType EditorFixtureSlingType = ETrussSlingType::OverSlung;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mounted Fixtures|Editor", meta = (MakeEditWidget = "true", Units = "cm"))
	FVector EditorFixtureLocalHitLocation = FVector::ZeroVector;

	UFUNCTION(BlueprintCallable, Category = "Truss")
	void BuildStraightRun();

	UFUNCTION(BlueprintCallable, Category = "Truss")
	void BuildRectangle();

	UFUNCTION(BlueprintCallable, Category = "Truss")
	void BuildArch();

	UFUNCTION(BlueprintCallable, Category = "Truss")
	void BuildCube();

	UFUNCTION(BlueprintCallable, Category = "Truss")
	void BuildCubeArch();

	UFUNCTION(BlueprintCallable, Category = "Truss")
	void BuildCurrentMode();

	UFUNCTION(BlueprintCallable, Category = "Truss")
	void ApplyBuildDefinition(const FTrussBuildDefinition& Definition, bool bRebuild = true);

	UFUNCTION(BlueprintPure, Category = "Truss")
	FTrussBuildDefinition GetBuildDefinition() const;

	UFUNCTION(BlueprintCallable, Category = "Truss")
	void ClearGeneratedTruss();

	UFUNCTION(BlueprintCallable, Category = "Truss")
	void SetSelectionHighlighted(bool bHighlighted);

	UFUNCTION(BlueprintCallable, Category = "Truss|Rigging")
	bool GetFixtureMountTransform(const FVector& WorldHitLocation, ETrussSlingType SlingType, FTransform& OutWorldTransform) const;

	UFUNCTION(BlueprintCallable, Category = "Truss|Rigging")
	bool AddMountedFixtureDefinition(TSubclassOf<AActor> FixtureClass, ETrussSlingType SlingType, const FVector& WorldHitLocation, bool bRespawnFixtures = true);

	UFUNCTION(BlueprintCallable, Category = "Truss|Rigging")
	void RebuildMountedFixtures();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Truss|Rigging")
	void AddEditorMountedFixture();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Truss|Rigging")
	void ClearMountedFixtures();

	virtual void OnConstruction(const FTransform& Transform) override;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UActorComponent>> GeneratedComponents;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> SpawnedMountedFixtureActors;

	FBox GeneratedBounds;

	UInstancedStaticMeshComponent* FindOrCreateMeshComponent(ETrussPieceType PieceType, UStaticMesh* StaticMesh);
	UInstancedStaticMeshComponent* GetMeshComponentForPiece(ETrussPieceType PieceType) const;
	UStaticMesh* LoadMajicGearDefaultMesh(ETrussPieceType PieceType) const;
	bool GetPieceDefinition(ETrussPieceType PieceType, FTrussPieceDefinition& OutPiece, UStaticMesh*& OutMesh) const;
	FVector GetMeshPlacementLocation(UStaticMesh* StaticMesh, const FVector& TargetMinLocation, const FRotator& Rotation) const;
	FVector GetScaledRotatedMeshExtent(UStaticMesh* StaticMesh, const FRotator& Rotation) const;
	void AddPieceInstance(ETrussPieceType PieceType, const FVector& TargetMinLocation, const FRotator& Rotation);
	void AddStraightRun(const TArray<ETrussPieceType>& Pieces, const FVector& StartMinLocation, const FRotator& Rotation);
	void AddDebugPiece(ETrussPieceType PieceType, float PieceLengthCm, const FVector& TargetMinLocation, const FRotator& Rotation);
	void AddDebugPiece(ETrussPieceType PieceType, float PieceLengthCm, float StartX);
	void UpdateSelectionBounds();
	FBox GetGeneratedBounds() const;
	void ExpandGeneratedBounds(const FBox& Bounds);
	void DestroyMountedFixtureActors();
};
