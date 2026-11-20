#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrussStructureActor.h"
#include "LightFixtureActor.generated.h"

class UBoxComponent;
class UChildActorComponent;
class UDMXComponent;
class UDMXEntityFixturePatch;
class UMaterialInstanceDynamic;
class UTexture2D;
struct FDMXNormalizedAttributeValueMap;

UCLASS(BlueprintType, Blueprintable)
class MAJICTRUSSRUNTIME_API ALightFixtureActor : public AActor
{
	GENERATED_BODY()

public:
	ALightFixtureActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light Fixture")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light Fixture")
	TObjectPtr<UBoxComponent> SelectionBounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light Fixture")
	TObjectPtr<UChildActorComponent> FixtureComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light Fixture|DMX")
	TObjectPtr<UDMXComponent> DMX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Fixture", meta = (DisplayName = "Fixture Blueprint Class"))
	TSoftClassPtr<AActor> FixtureClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Fixture|Rendering", meta = (DisplayName = "Cast Shadows"))
	bool bCastShadows = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Fixture|Gobo", meta = (DisplayName = "Gobo", AllowedClasses = "/Script/Engine.Texture2D"))
	TSoftObjectPtr<UTexture2D> GoboTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Fixture|Gobo", meta = (DisplayName = "Custom Gobo", AllowedClasses = "/Script/Engine.Texture2D"))
	TSoftObjectPtr<UTexture2D> CustomGoboTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Fixture|Placement", meta = (Units = "cm"))
	FVector FixturePlacementOffsetCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Fixture|Placement")
	FRotator FixturePlacementRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Fixture|Placement", meta = (ClampMin = "0.01"))
	FVector FixtureScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Fixture|Truss")
	ETrussSlingType SlingType = ETrussSlingType::OverSlung;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light Fixture|Truss")
	TObjectPtr<ATrussStructureActor> AttachedTruss = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Fixture|Truss")
	FMountedFixtureDefinition MountDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Fixture|Truss")
	bool bAttachedToTruss = false;

	// Free mount: the light was snapped to the nearest truss rail and keeps this place relative to the
	// truss, so it moves with the truss but stays put when the truss is resized. Lights saved before
	// free mounting have this off and keep the old span and rail behaviour until they are moved.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Fixture|Truss")
	bool bFreeMount = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Fixture|Truss", meta = (EditCondition = "bFreeMount"))
	FTransform FreeMountLocal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Fixture|Editor", meta = (ClampMin = "0.0", Units = "cm"))
	float AttachSearchRadiusCm = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Fixture|Editor")
	bool bAutoAttachToNearestTrussOnMove = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Fixture|Editor")
	bool bSnapContinuouslyWhileDragging = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Fixture|Editor")
	bool bDetachWhenNoTrussInRange = false;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Light Fixture")
	void RebuildFixture();

	UFUNCTION(BlueprintCallable, Category = "Light Fixture|Gobo")
	void SetGoboTexture(UTexture2D* Texture);

	UFUNCTION(BlueprintPure, Category = "Light Fixture|Gobo")
	bool IsLekoFixture() const;

	UFUNCTION(BlueprintCallable, Category = "Light Fixture")
	void SetSelectionHighlighted(bool bHighlighted);

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Light Fixture|DMX")
	void RefreshDMXBinding();

	UFUNCTION(BlueprintPure, Category = "Light Fixture|DMX")
	AActor* GetSpawnedFixtureActor() const;

	UFUNCTION(BlueprintPure, Category = "Light Fixture|DMX")
	UDMXComponent* GetSpawnedFixtureDMXComponent() const;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Light Fixture|Truss")
	void AttachToNearestTruss();

	UFUNCTION(BlueprintCallable, Category = "Light Fixture|Truss")
	bool AttachToTrussAtWorldLocation(ATrussStructureActor* TargetTruss, const FVector& WorldLocation);

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Light Fixture|Truss")
	void DetachFromTruss();

	UFUNCTION(BlueprintCallable, Category = "Light Fixture|Truss")
	bool ApplyStoredMountTransform();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#if WITH_EDITOR
	virtual bool ShouldTickIfViewportsOnly() const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;
#endif

private:
	UPROPERTY(Transient)
	bool bApplyingTrussSnap = false;

	UPROPERTY(Transient)
	TObjectPtr<UDMXEntityFixturePatch> BoundFixturePatch = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GoboMaterialInstance = nullptr;

	float CurrentDMXDimmer = 1.0f;
	bool bHasDMXDimmer = false;

	TSubclassOf<AActor> GetResolvedFixtureClass() const;
	void BindFixturePatch(UDMXEntityFixturePatch* FixturePatch);
	void UnbindFixturePatch();
	void PushDMXValuesToSpawnedFixture(const FDMXNormalizedAttributeValueMap& ValuePerAttribute);
	void ApplyDMXDimmerToSpawnedFixture();
	void ApplyLightRenderingSettings();
	void HandleFixturePatchReceivedDMX(UDMXEntityFixturePatch* FixturePatch, const FDMXNormalizedAttributeValueMap& ValuePerAttribute);
	bool TryAttachToNearestTruss();
	bool FindNearestTrussMount(ATrussStructureActor*& OutTruss, FTransform& OutMountTransform, FTransform& OutLocalMount) const;
	void UpdateFixtureComponent();
	void ApplyGoboTexture();
	void UpdateSelectionBounds();
};
