#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrussInventoryDataAsset.h"
#include "VideoPlacementActor.generated.h"

class UBoxComponent;
class UInstancedStaticMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EVideoTVModel : uint8
{
	Hisense58 UMETA(DisplayName = "Hisense 58 4K"),
	Insignia43 UMETA(DisplayName = "Insignia 43 1080p"),
	Philips46 UMETA(DisplayName = "Philips 46 Monitor"),
	Samsung22 UMETA(DisplayName = "Samsung 22 Monitor"),
	Samsung55Outdoor UMETA(DisplayName = "Samsung 55 Outdoor"),
	Samsung58 UMETA(DisplayName = "Samsung 58 UHD"),
	Samsung60 UMETA(DisplayName = "Samsung 60 HD"),
	Samsung82Crystal UMETA(DisplayName = "Samsung 82 Crystal"),
	Samsung82Smart UMETA(DisplayName = "Samsung 82 Smart"),
	Sharp55 UMETA(DisplayName = "Sharp 55 4K"),
	Sharp60 UMETA(DisplayName = "Sharp 60 4K"),
	Sharp80 UMETA(DisplayName = "Sharp 80 LED"),
	Sharp90 UMETA(DisplayName = "Sharp 90 LED"),
	Vizio70 UMETA(DisplayName = "Vizio 70 LED"),
	Benq25Preview UMETA(DisplayName = "BenQ 25 Preview")
};

USTRUCT(BlueprintType)
struct FVideoPlacementBuildDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video")
	EVideoTVModel TVModel = EVideoTVModel::Samsung58;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video", meta = (DisplayName = "TV Center Height (ft)", ClampMin = "1.0"))
	float TVCenterHeightFt = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video", meta = (DisplayName = "Tower Height (ft)", ClampMin = "2.0"))
	float TowerHeightFt = 8.0f;
};

UCLASS(BlueprintType)
class MAJICTRUSSRUNTIME_API AVideoPlacementActor : public AActor
{
	GENERATED_BODY()

public:
	AVideoPlacementActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Video")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Video")
	TObjectPtr<UBoxComponent> SelectionBounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Video|Generated")
	TObjectPtr<UInstancedStaticMeshComponent> TenFootTrussInstances;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Video|Generated")
	TObjectPtr<UInstancedStaticMeshComponent> EightFootTrussInstances;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Video|Generated")
	TObjectPtr<UInstancedStaticMeshComponent> FiveFootTrussInstances;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Video|Generated")
	TObjectPtr<UInstancedStaticMeshComponent> FourFootTrussInstances;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Video|Generated")
	TObjectPtr<UInstancedStaticMeshComponent> TwoFootTrussInstances;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Video|Generated")
	TObjectPtr<UInstancedStaticMeshComponent> BaseInstances;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Video|Generated")
	TObjectPtr<UStaticMeshComponent> TVComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Video|Generated")
	TArray<TObjectPtr<UStaticMeshComponent>> UpperUPMComponents;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Video|Generated")
	TArray<TObjectPtr<UStaticMeshComponent>> LowerUPMComponents;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video")
	EVideoTVModel TVModel = EVideoTVModel::Samsung58;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video", meta = (DisplayName = "TV Center Height (ft)", ClampMin = "1.0"))
	float TVCenterHeightFt = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video", meta = (DisplayName = "Tower Height (ft)", ClampMin = "2.0"))
	float TowerHeightFt = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video")
	bool bBuildOnConstruction = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video|Assets")
	TObjectPtr<UTrussInventoryDataAsset> Inventory = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video|Assets")
	bool bUseMajicGearDefaultMeshes = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video|Assets", meta = (ClampMin = "0.0001"))
	float TrussMeshScaleMultiplier = 0.0254f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video|Placement", meta = (Units = "cm"))
	FVector TowerPlacementOffsetCm = FVector(-12.0f, -16.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video|Placement")
	FRotator TowerRotation = FRotator(90.0f, 90.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video|Placement", meta = (Units = "cm"))
	FVector TVPlacementOffsetCm = FVector(1.786592f, -19.894883f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video|Placement")
	FRotator TVPlacementRotation = FRotator(0.0f, 0.0f, 180.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video|Placement")
	FVector TVScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video|Placement", meta = (Units = "cm"))
	FVector UPMPlacementOffsetCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video|Placement", meta = (Units = "cm", ClampMin = "0.0"))
	float UPMVerticalSpacingCm = 15.24f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video|Placement")
	FRotator UPMPlacementRotation = FRotator(0.0f, 0.0f, 180.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Video|Placement")
	FVector UPMScale = FVector::OneVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Video|Debug", meta = (DisplayName = "Current Actual Tower Height (ft)"))
	float CurrentActualTowerHeightFt = 0.0f;

	UFUNCTION(BlueprintCallable, Category = "Video")
	void RebuildVideoPlacement();

	UFUNCTION(BlueprintCallable, Category = "Video")
	void ApplyBuildDefinition(const FVideoPlacementBuildDefinition& Definition, bool bRebuildNow = true);

	UFUNCTION(BlueprintPure, Category = "Video")
	FVideoPlacementBuildDefinition GetBuildDefinition() const;

	UFUNCTION(BlueprintCallable, Category = "Video")
	void SetSelectionHighlighted(bool bHighlighted);

	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	FBox GeneratedBounds;

	void ClearGenerated();
	void AddTowerPiece(ETrussPieceType PieceType, const FVector& TargetMinLocation, const FRotator& Rotation);
	UInstancedStaticMeshComponent* GetMeshComponentForPiece(ETrussPieceType PieceType) const;
	UStaticMesh* LoadMajicGearDefaultMesh(ETrussPieceType PieceType) const;
	UStaticMesh* LoadTVMesh(EVideoTVModel Model) const;
	void LoadUPMMeshes();
	void PlaceUPMComponents(const FVector& TVCenterLocation);
	bool GetPieceDefinition(ETrussPieceType PieceType, FTrussPieceDefinition& OutPiece, UStaticMesh*& OutMesh) const;
	FVector GetMeshPlacementLocation(UStaticMesh* StaticMesh, const FVector& TargetMinLocation, const FRotator& Rotation) const;
	FVector GetScaledRotatedMeshExtent(UStaticMesh* StaticMesh, const FRotator& Rotation) const;
	void ExpandGeneratedBounds(const FBox& Bounds);
	void UpdateSelectionBounds();
};
