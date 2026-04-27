#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuildItemDataAsset.h"
#include "BuildManagerComponent.generated.h"

class ABuildPreviewActor;
class APlayerController;
class ATrussStructureActor;

USTRUCT(BlueprintType)
struct FBuildPlacementResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Build")
	bool bSuccess = false;

	UPROPERTY(BlueprintReadOnly, Category = "Build")
	TObjectPtr<AActor> SpawnedActor = nullptr;
};

UCLASS(ClassGroup = (Majic), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class MAJICTRUSSRUNTIME_API UBuildManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBuildManagerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
	TSubclassOf<ABuildPreviewActor> PreviewActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build", meta = (ClampMin = "100.0", Units = "cm"))
	float TraceDistanceCm = 50000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
	TEnumAsByte<ECollisionChannel> PlacementTraceChannel = ECC_Visibility;

	UPROPERTY(BlueprintReadOnly, Category = "Build")
	bool bBuildModeActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Build")
	TObjectPtr<UBuildItemDataAsset> SelectedBuildItem = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Build")
	TObjectPtr<ABuildPreviewActor> ActivePreviewActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Build")
	FTrussBuildDefinition ActiveTrussDefinition;

	UFUNCTION(BlueprintCallable, Category = "Build")
	bool EnterBuildMode();

	UFUNCTION(BlueprintCallable, Category = "Build")
	void ExitBuildMode();

	UFUNCTION(BlueprintCallable, Category = "Build")
	bool SetSelectedBuildItem(UBuildItemDataAsset* BuildItem);

	UFUNCTION(BlueprintCallable, Category = "Build")
	void SetActiveTrussDefinition(const FTrussBuildDefinition& Definition);

	UFUNCTION(BlueprintCallable, Category = "Build")
	bool UpdatePreviewFromPlayerView(APlayerController* PlayerController);

	UFUNCTION(BlueprintCallable, Category = "Build")
	void RotatePreviewYaw(float DeltaDegrees);

	UFUNCTION(BlueprintCallable, Category = "Build")
	FBuildPlacementResult ConfirmPlacement();

	UFUNCTION(BlueprintPure, Category = "Build")
	FRotator GetCurrentPlacementRotation() const;

	UFUNCTION(BlueprintPure, Category = "Build")
	FTransform GetCurrentPlacementTransform() const;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	FTransform CurrentPlacementTransform = FTransform::Identity;
	float CurrentYawDegrees = 0.0f;
	bool bHasValidPlacement = false;

	bool EnsurePreviewActor();
	void DestroyPreviewActor();
	void ApplyCurrentSettingsToActor(AActor* Actor) const;
	AActor* SpawnBuildActor(const FTransform& SpawnTransform) const;
	FVector SnapLocation(const FVector& Location, float GridSizeCm) const;
	FRotator MakePlacementRotation(const FVector& SurfaceNormal) const;
	TSubclassOf<AActor> ResolveBuildActorClass() const;
};
