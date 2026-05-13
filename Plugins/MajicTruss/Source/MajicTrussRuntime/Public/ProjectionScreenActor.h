#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrussInventoryDataAsset.h"
#include "ProjectionScreenActor.generated.h"

class UBoxComponent;
class UChildActorComponent;
class UMaterialInterface;
class USpotLightComponent;
class UStaticMesh;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EProjectionScreenKitSize : uint8
{
	Screen6x12 UMETA(DisplayName = "6 x 12"),
	Screen8x14 UMETA(DisplayName = "8 x 14"),
	Screen9x16 UMETA(DisplayName = "9 x 16"),
	Screen13x24 UMETA(DisplayName = "13 x 24")
};

UENUM(BlueprintType)
enum class EProjectionLensType : uint8
{
	ILS067HD UMETA(DisplayName = "ILS 0.67 HD Fixed"),
	ILS116149HD UMETA(DisplayName = "ILS 1.16-1.49 HD Zoom"),
	ILS4169HD UMETA(DisplayName = "ILS 4.1-6.9 HD Zoom")
};

UENUM(BlueprintType)
enum class EProjectionProjectorType : uint8
{
	ChristieM4K25RGB UMETA(DisplayName = "Christie M 4K25 RGB")
};

UENUM(BlueprintType)
enum class EProjectionThrowPosition : uint8
{
	Minimum UMETA(DisplayName = "Minimum"),
	Middle UMETA(DisplayName = "Middle"),
	Maximum UMETA(DisplayName = "Maximum"),
	Manual UMETA(DisplayName = "Manual")
};

UENUM(BlueprintType)
enum class EProjectionProjectorMountMode : uint8
{
	None UMETA(DisplayName = "None"),
	AVCart UMETA(DisplayName = "AV Cart"),
	TrussTower UMETA(DisplayName = "Truss Tower"),
	HangingTruss UMETA(DisplayName = "Hanging Truss")
};

UENUM(BlueprintType)
enum class EProjectionProjectorSlingType : uint8
{
	UnderSlung UMETA(DisplayName = "Under Slung"),
	OverSlung UMETA(DisplayName = "Over Slung")
};

USTRUCT(BlueprintType)
struct FProjectionScreenBuildDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection")
	EProjectionScreenKitSize ScreenKitSize = EProjectionScreenKitSize::Screen9x16;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection")
	EProjectionLensType LensType = EProjectionLensType::ILS116149HD;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection")
	EProjectionProjectorType ProjectorType = EProjectionProjectorType::ChristieM4K25RGB;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection")
	EProjectionThrowPosition ThrowPosition = EProjectionThrowPosition::Middle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection", meta = (DisplayName = "Manual Throw Distance (ft)", ClampMin = "1.0"))
	float ManualThrowDistanceFt = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection|Mount", meta = (DisplayName = "Projector Mount Mode"))
	EProjectionProjectorMountMode MountMode = EProjectionProjectorMountMode::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection|Mount", meta = (DisplayName = "Projector Sling Type"))
	EProjectionProjectorSlingType SlingType = EProjectionProjectorSlingType::UnderSlung;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection|Placement", meta = (DisplayName = "Projector Height From Screen Center (ft)"))
	float TowerHeightFt = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection|Placement", meta = (DisplayName = "Projector Left / Right Offset (ft)"))
	float ProjectorHorizontalOffsetFt = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection|Screen Center", meta = (DisplayName = "Screen Center Offset Cm", Units = "cm"))
	FVector ScreenCenterOffsetCm = FVector::ZeroVector;
};

UCLASS(BlueprintType, meta = (DisplayName = "Projection Screen Actor"))
class MAJICTRUSSRUNTIME_API AProjectionScreenActor : public AActor
{
	GENERATED_BODY()

public:
	AProjectionScreenActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projection")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projection")
	TObjectPtr<UBoxComponent> SelectionBounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projection|Generated")
	TObjectPtr<UChildActorComponent> ScreenKitComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projection|Generated")
	TObjectPtr<USpotLightComponent> ProjectionLightComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projection|Generated")
	TObjectPtr<UStaticMeshComponent> ScreenCenterMarkerComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection")
	EProjectionScreenKitSize ScreenKitSize = EProjectionScreenKitSize::Screen9x16;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection")
	EProjectionLensType LensType = EProjectionLensType::ILS116149HD;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection")
	EProjectionProjectorType ProjectorType = EProjectionProjectorType::ChristieM4K25RGB;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection")
	EProjectionThrowPosition ThrowPosition = EProjectionThrowPosition::Middle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection", meta = (DisplayName = "Manual Throw Distance (ft)", ClampMin = "1.0"))
	float ManualThrowDistanceFt = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection")
	EProjectionProjectorMountMode MountMode = EProjectionProjectorMountMode::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection")
	EProjectionProjectorSlingType SlingType = EProjectionProjectorSlingType::UnderSlung;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection")
	bool bBuildOnConstruction = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection|Placement", meta = (Units = "cm"))
	FVector ScreenKitPlacementOffsetCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection|Placement")
	FRotator ScreenKitPlacementRotation = FRotator(0.0f, 180.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection|Placement")
	FVector ScreenKitScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection|Placement", meta = (Units = "cm"))
	FVector ProjectorPlacementOffsetCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection|Placement")
	FRotator ProjectorPlacementRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection|Placement", meta = (DisplayName = "Projector Height Offset Cm", Units = "cm"))
	float ProjectorHeightOffsetCm = -28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection|Placement", meta = (DisplayName = "Under Slung Projector Rotation"))
	FRotator UnderSlungProjectorRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection|Placement", meta = (DisplayName = "Over Slung Projector Rotation"))
	FRotator OverSlungProjectorRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection|Placement")
	FVector ProjectorScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection|Screen Center", meta = (DisplayName = "Screen Center Offset Cm", Units = "cm"))
	FVector ScreenCenterOffsetCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection|Screen Center")
	bool bShowScreenCenterMarker = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection|Screen Center", meta = (ClampMin = "0.01"))
	FVector ScreenCenterMarkerScale = FVector(0.12f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection|Mount", meta = (Units = "cm"))
	FVector MountPlacementOffsetCm = FVector(75.0f, -30.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection|Mount", meta = (DisplayName = "Truss Tower Stick Offset Cm", Units = "cm"))
	FVector TrussTowerStickOffsetCm = FVector(-75.0f, 28.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection|Mount", meta = (DisplayName = "Truss Tower Base Offset Cm", Units = "cm"))
	FVector TrussTowerBaseOffsetCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection|Mount")
	FRotator MountPlacementRotation = FRotator(0.0f, 90.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection|Mount")
	FVector MountScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection|Mount", meta = (DisplayName = "Projector Height From Screen Center (ft)"))
	float TowerHeightFt = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection|Mount", meta = (DisplayName = "Projector Horizontal From Screen Center (ft)"))
	float ProjectorHorizontalOffsetFt = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection|Mount", meta = (DisplayName = "Hanging Truss Length (ft)", ClampMin = "2.0"))
	float HangingTrussLengthFt = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection|Mount", meta = (ClampMin = "0.0001"))
	float TrussMeshScaleMultiplier = 0.0254f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection|Visualization")
	bool bEnableProjectionLight = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection|Visualization", meta = (ClampMin = "1.0"))
	float ProjectionLightIntensityScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection|Visualization", meta = (ClampMin = "1.0", ClampMax = "89.0"))
	float ProjectionLightConeAngleDegrees = 28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection|Visualization", meta = (DisplayName = "Auto Projection Light Cone"))
	bool bAutoProjectionLightConeAngle = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection|Visualization")
	TObjectPtr<UMaterialInterface> ProjectionLightFunctionMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection|Visualization", meta = (ClampMin = "0.01"))
	FVector ProjectionLightFunctionScale = FVector(1.0f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection|Visualization")
	float ProjectionLightFunctionFadeDistance = 100000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection|Visualization", meta = (Units = "cm"))
	FVector ProjectionLightPlacementOffsetCm = FVector(0.0f, 46.0f, -33.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projection|Visualization", meta = (Units = "cm"))
	FVector ProjectionLightTargetOffsetCm = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projection|Debug", meta = (DisplayName = "Min Throw Distance (ft)"))
	float CurrentMinThrowDistanceFt = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projection|Debug", meta = (DisplayName = "Max Throw Distance (ft)"))
	float CurrentMaxThrowDistanceFt = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projection|Debug", meta = (DisplayName = "Current Throw Distance (ft)"))
	float CurrentThrowDistanceFt = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projection|Debug")
	float CurrentScreenWidthFt = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projection|Debug")
	float CurrentScreenHeightFt = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projection|Debug", meta = (DisplayName = "Lens Vertical Shift Min (%)"))
	float CurrentLensVerticalShiftMinPercent = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projection|Debug", meta = (DisplayName = "Lens Vertical Shift Max (%)"))
	float CurrentLensVerticalShiftMaxPercent = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projection|Debug", meta = (DisplayName = "Lens Horizontal Shift Min (%)"))
	float CurrentLensHorizontalShiftMinPercent = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projection|Debug", meta = (DisplayName = "Lens Horizontal Shift Max (%)"))
	float CurrentLensHorizontalShiftMaxPercent = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projection|Debug", meta = (DisplayName = "Lens Shift Data Available"))
	bool bCurrentLensShiftDataAvailable = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projection|Debug", meta = (DisplayName = "Screen Center Height (ft)"))
	float CurrentScreenCenterHeightFt = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projection|Debug", meta = (DisplayName = "Min Projector Height (ft)"))
	float CurrentMinProjectorHeightFt = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projection|Debug", meta = (DisplayName = "Max Projector Height (ft)"))
	float CurrentMaxProjectorHeightFt = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projection|Debug", meta = (DisplayName = "Min Projector Horizontal Offset (ft)"))
	float CurrentMinProjectorHorizontalOffsetFt = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projection|Debug", meta = (DisplayName = "Max Projector Horizontal Offset (ft)"))
	float CurrentMaxProjectorHorizontalOffsetFt = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projection|Debug", meta = (DisplayName = "Requested Projector Height (ft)"))
	float CurrentRequestedProjectorHeightFt = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projection|Debug", meta = (DisplayName = "Actual Buildable Projector Height (ft)"))
	float CurrentActualBuildableProjectorHeightFt = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projection|Debug", meta = (DisplayName = "Actual Projector Horizontal Offset (ft)"))
	float CurrentActualProjectorHorizontalOffsetFt = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projection|Debug", meta = (DisplayName = "Actual Truss Pieces"))
	FString CurrentActualTrussPieces;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projection|Debug", meta = (DisplayName = "Current Projection Light Cone Angle"))
	float CurrentProjectionLightConeAngleDegrees = 0.0f;

	UFUNCTION(BlueprintCallable, Category = "Projection")
	void RebuildProjectionScreen();

	UFUNCTION(BlueprintCallable, Category = "Projection")
	void ApplyBuildDefinition(const FProjectionScreenBuildDefinition& Definition, bool bRebuildNow = true);

	UFUNCTION(BlueprintPure, Category = "Projection")
	FProjectionScreenBuildDefinition GetBuildDefinition() const;

	UFUNCTION(BlueprintCallable, Category = "Projection")
	void SetSelectionHighlighted(bool bHighlighted);

	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	TArray<TObjectPtr<UStaticMeshComponent>> ProjectorComponents;
	TArray<TObjectPtr<UStaticMeshComponent>> MountComponents;

	void ClearProjectorComponents();
	void ClearMountComponents();
	void BuildScreenKit();
	void BuildProjector();
	void BuildMountSupport(const FVector& ProjectorLocation, const FRotator& AimRotation);
	void BuildTrussTowerSupport(const FVector& ProjectorLocation);
	void AddTrussTowerPiece(ETrussPieceType PieceType, const FVector& TargetMinLocation, const FRotator& Rotation);
	void AddStaticMeshComponent(TArray<TObjectPtr<UStaticMeshComponent>>& ComponentArray, UStaticMesh* Mesh, const FString& NamePrefix, const FVector& Location, const FRotator& Rotation, const FVector& Scale, bool bDisallowNanite = true);
	void UpdateProjectionLight(const FVector& ProjectorLocation, const FVector& ScreenCenterLocation);
	void UpdateScreenCenterMarker(const FVector& ScreenCenterLocation);
	void UpdateSelectionBounds();
	void GetScreenDimensionsFt(float& OutWidthFt, float& OutHeightFt) const;
	void GetLensShiftPercent(float& OutVerticalMinPercent, float& OutVerticalMaxPercent, float& OutHorizontalMinPercent, float& OutHorizontalMaxPercent, bool& bOutHasData) const;
	float ResolveProjectorHeightFt(float ScreenCenterHeightFt, float ScreenHeightFt);
	float ResolveProjectorHorizontalOffsetFt(float ScreenWidthFt);
	void ComputeThrowDistances(float ScreenWidthFt, float& OutMinFt, float& OutMaxFt) const;
	float ResolveThrowDistanceFt() const;
	UClass* LoadScreenKitClass() const;
	TArray<UStaticMesh*> LoadChristieProjectorMeshes() const;
	TArray<UStaticMesh*> LoadAVCartMeshes() const;
	UStaticMesh* LoadMajicGearDefaultMesh(ETrussPieceType PieceType) const;
	void GetPieceDefinition(ETrussPieceType PieceType, UStaticMesh*& OutMesh, float& OutLengthCm) const;
	FVector GetTrussMeshPlacementLocation(UStaticMesh* StaticMesh, const FVector& TargetMinLocation, const FRotator& Rotation) const;
	FVector GetScaledRotatedTrussMeshExtent(UStaticMesh* StaticMesh, const FRotator& Rotation) const;
	void ExpandBoundsFromActor(AActor* Actor, FBox& Bounds) const;
};
