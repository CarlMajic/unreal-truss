#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BuildPreviewActor.generated.h"

class UChildActorComponent;
class USceneComponent;

UCLASS(BlueprintType)
class MAJICTRUSSRUNTIME_API ABuildPreviewActor : public AActor
{
	GENERATED_BODY()

public:
	ABuildPreviewActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Build")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Build")
	TObjectPtr<UChildActorComponent> PreviewActorComponent;

	UFUNCTION(BlueprintCallable, Category = "Build")
	void SetPreviewActorClass(TSubclassOf<AActor> InActorClass);

	UFUNCTION(BlueprintCallable, Category = "Build")
	void SetPlacementValid(bool bIsValid);

	UFUNCTION(BlueprintPure, Category = "Build")
	bool IsPlacementValid() const;

	UFUNCTION(BlueprintPure, Category = "Build")
	AActor* GetPreviewActor() const;

protected:
	virtual void BeginPlay() override;

private:
	bool bPlacementValid = true;

	void ApplyPreviewState() const;
	static void ConfigurePreviewActor(AActor* Actor, bool bVisible);
};
