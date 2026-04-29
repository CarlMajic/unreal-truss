#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MBPWallActor.generated.h"

class UInstancedStaticMeshComponent;
class UBoxComponent;

UENUM(BlueprintType)
enum class EMBPPanelStyle : uint8
{
	Acrylic UMETA(DisplayName = "Acrylic"),
	Drift UMETA(DisplayName = "Drift"),
	Geo UMETA(DisplayName = "Geo"),
	Shimmer UMETA(DisplayName = "Shimmer"),
	Hive UMETA(DisplayName = "Hive"),
	Platinum UMETA(DisplayName = "Platinum")
};

UENUM(BlueprintType)
enum class EMBPShimmerVariant : uint8
{
	Gold UMETA(DisplayName = "Gold"),
	Red UMETA(DisplayName = "Red"),
	Fuchsia UMETA(DisplayName = "Fuchsia"),
	Holographic UMETA(DisplayName = "Holographic")
};

USTRUCT(BlueprintType)
struct FMBPPanelSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Panel")
	EMBPPanelStyle Style = EMBPPanelStyle::Geo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Panel")
	EMBPShimmerVariant ShimmerVariant = EMBPShimmerVariant::Gold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Panel", meta = (Units = "cm"))
	float DepthOffsetCm = 0.0f;
};

UCLASS(BlueprintType)
class MAJICTRUSSRUNTIME_API AMBPWallActor : public AActor
{
	GENERATED_BODY()

public:
	AMBPWallActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall")
	TObjectPtr<UBoxComponent> SelectionBounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall|Generated")
	TObjectPtr<UInstancedStaticMeshComponent> AcrylicInstances;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall|Generated")
	TObjectPtr<UInstancedStaticMeshComponent> DriftInstances;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall|Generated")
	TObjectPtr<UInstancedStaticMeshComponent> GeoInstances;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall|Generated")
	TObjectPtr<UInstancedStaticMeshComponent> HiveInstances;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall|Generated")
	TObjectPtr<UInstancedStaticMeshComponent> PlatinumInstances;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall|Generated")
	TObjectPtr<UInstancedStaticMeshComponent> ShimmerGoldInstances;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall|Generated")
	TObjectPtr<UInstancedStaticMeshComponent> ShimmerRedInstances;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall|Generated")
	TObjectPtr<UInstancedStaticMeshComponent> ShimmerFuchsiaInstances;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall|Generated")
	TObjectPtr<UInstancedStaticMeshComponent> ShimmerHolographicInstances;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall", meta = (ClampMin = "1"))
	int32 Columns = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall", meta = (ClampMin = "1"))
	int32 Rows = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall")
	EMBPPanelStyle DefaultStyle = EMBPPanelStyle::Geo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall")
	EMBPShimmerVariant DefaultShimmerVariant = EMBPShimmerVariant::Gold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall", meta = (Units = "cm"))
	float PanelWidthCm = 91.44f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall", meta = (Units = "cm"))
	float PanelHeightCm = 91.44f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall", meta = (Units = "cm"))
	float HorizontalSpacingCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall", meta = (Units = "cm"))
	float VerticalSpacingCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall")
	bool bCenterOnActor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall")
	bool bBuildOnConstruction = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall")
	TArray<FMBPPanelSlot> PanelSlots;

	UFUNCTION(BlueprintCallable, Category = "Wall")
	void RebuildWall();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Wall")
	void ResetSlotsToDefault();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Wall")
	void ResizeSlotsToGrid();

	virtual void OnConstruction(const FTransform& Transform) override;

private:
	void EnsureSlotCount(bool bResetNewSlotsToDefault);
	FMBPPanelSlot MakeDefaultSlot() const;
	UStaticMesh* LoadMeshForStyle(EMBPPanelStyle Style) const;
	UMaterialInterface* LoadShimmerMaterial(EMBPShimmerVariant Variant) const;
	UInstancedStaticMeshComponent* GetComponentForSlot(const FMBPPanelSlot& Slot) const;
	void ClearInstances();
	void UpdateSelectionBounds(const FBox& Bounds);
};
