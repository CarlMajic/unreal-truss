#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MetaHumanAnimationTestActor.generated.h"

class UAnimationAsset;
class UBoxComponent;
class UChildActorComponent;
class USkeletalMeshComponent;

UCLASS(BlueprintType, meta = (DisplayName = "MetaHuman Animation Test Actor"))
class MAJICTRUSSRUNTIME_API AMetaHumanAnimationTestActor : public AActor
{
	GENERATED_BODY()

public:
	AMetaHumanAnimationTestActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MetaHuman Test")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MetaHuman Test")
	TObjectPtr<UBoxComponent> SelectionBounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MetaHuman Test")
	bool bBuildOnConstruction = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MetaHuman Test")
	bool bPlayOnConstruction = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MetaHuman Test")
	bool bForceBodyLeaderPose = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MetaHuman Test")
	bool bForceLOD0ForPrevis = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MetaHuman Test|Standalone")
	bool bReplacePreviousStandaloneGuest = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MetaHuman Test|Standalone")
	bool bDestroySpawnerAfterSpawn = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MetaHuman Test")
	TSoftClassPtr<AActor> GuestBlueprintClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MetaHuman Test")
	TSoftObjectPtr<UAnimationAsset> GuestAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MetaHuman Test|Placement", meta = (Units = "cm"))
	FVector GuestRelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MetaHuman Test|Placement")
	FRotator GuestRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MetaHuman Test|Placement")
	FVector GuestRelativeScale = FVector::OneVector;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "MetaHuman Test|Status")
	bool bLastChildActorCreated = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "MetaHuman Test|Status")
	bool bLastBodyComponentFound = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "MetaHuman Test|Status")
	FName LastAnimatedComponentName = NAME_None;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "MetaHuman Test|Status")
	int32 LastSkeletalComponentCount = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "MetaHuman Test|Status")
	int32 LastFollowerComponentsBound = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "MetaHuman Test|Status")
	FString LastStatusMessage;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "MetaHuman Test")
	void RebuildGuest();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "MetaHuman Test")
	void SpawnStandaloneGuest();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "MetaHuman Test")
	void SpawnStandaloneAnimatedGuest();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "MetaHuman Test")
	void ApplyGuestAnimation();

	virtual void OnConstruction(const FTransform& Transform) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<AActor> LastSpawnedStandaloneGuest;

	AActor* SpawnStandaloneGuestInternal(bool bApplyAnimation);
	bool ApplyAnimationToGuestActor(AActor* GuestActor);
	USkeletalMeshComponent* FindBodyComponent(AActor* GuestActor, USkeletalMeshComponent*& OutFallbackMeshComponent) const;
	USkeletalMeshComponent* FindFaceComponent(AActor* GuestActor) const;
	void SyncFaceComponentToAnimation(AActor* GuestActor, USkeletalMeshComponent* FaceComponent, USkeletalMeshComponent* BodyComponent) const;
	int32 BindFollowerComponentsToBody(AActor* GuestActor, USkeletalMeshComponent* BodyComponent) const;
	int32 CountSkeletalMeshComponents(AActor* GuestActor) const;
	void SetStatus(const FString& Message, bool bChildActorCreated, bool bBodyComponentFound, FName AnimatedComponentName, int32 SkeletalComponentCount = 0, int32 FollowerComponentsBound = 0);
};
