#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SocialGroupActor.generated.h"

class UArrowComponent;
class UAnimationAsset;
class USkeletalMesh;
class USkeletalMeshComponent;
class USphereComponent;

UCLASS(BlueprintType, meta = (DisplayName = "Social Group Actor"))
class MAJICTRUSSRUNTIME_API ASocialGroupActor : public AActor
{
	GENERATED_BODY()

public:
	ASocialGroupActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Social Group")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social Group", meta = (ClampMin = "1", ClampMax = "32"))
	int32 CharacterCount = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social Group", meta = (ClampMin = "0.0", Units = "cm"))
	float GroupRadiusCm = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social Group", meta = (Units = "deg"))
	float StartAngleDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social Group", meta = (Units = "cm"))
	FVector CenterOffsetCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social Group")
	TArray<FVector> PerSlotOffsetsCm;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social Group")
	TArray<FRotator> PerSlotRotationOffsets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social Group|Preview")
	bool bShowAISlotPreviewInEditor = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social Group|Preview")
	bool bShowAIAnimationPreviewInEditor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social Group|Preview", meta = (EditCondition = "bShowAIAnimationPreviewInEditor"))
	TSoftObjectPtr<USkeletalMesh> AIAnimationPreviewMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social Group|Preview", meta = (EditCondition = "bShowAIAnimationPreviewInEditor"))
	TSoftObjectPtr<UAnimationAsset> AIAnimationPreviewDefaultAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social Group|Preview", meta = (EditCondition = "bShowAIAnimationPreviewInEditor"))
	TArray<TSoftObjectPtr<UAnimationAsset>> AIAnimationPreviewPerSlotAnimations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social Group|Preview", meta = (Units = "cm", EditCondition = "bShowAIAnimationPreviewInEditor"))
	FVector AIAnimationPreviewMeshOffsetCm = FVector(0.0f, 0.0f, -90.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social Group|Preview", meta = (Units = "deg", EditCondition = "bShowAIAnimationPreviewInEditor"))
	FRotator AIAnimationPreviewMeshRotation = FRotator(0.0f, -90.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social Group|Preview", meta = (EditCondition = "bShowAIAnimationPreviewInEditor"))
	FVector AIAnimationPreviewMeshScale = FVector::OneVector;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Social Group")
	void RefreshSocialGroupPreview();

	UFUNCTION(BlueprintCallable, Category = "Social Group|AI Preview")
	void GetAISlotPreviewLocalOffsets(TArray<FVector>& OutOffsets) const;

	UFUNCTION(BlueprintCallable, Category = "Social Group|AI Preview")
	void GetAISlotPreviewLocalRotations(TArray<FRotator>& OutRotations) const;

	UFUNCTION(BlueprintCallable, Category = "Social Group|AI Preview")
	void GetAISlotPreviewAnimations(TArray<TSoftObjectPtr<UAnimationAsset>>& OutAnimations) const;

	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
#if WITH_EDITORONLY_DATA
	UPROPERTY(Transient)
	TArray<TObjectPtr<USphereComponent>> AISlotPreviewComponents;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UArrowComponent>> AISlotPreviewArrowComponents;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USkeletalMeshComponent>> AIAnimationPreviewComponents;
#endif

	void RefreshAISlotPreviewComponents();
	void RefreshAIAnimationPreviewComponents(const TArray<FVector>& PreviewOffsets, const TArray<FRotator>& PreviewRotations);
	void ClearEditorPreviewComponents();
};
