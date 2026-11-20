#pragma once

#include "CoreMinimal.h"
#include "SocialGroupActor.h"
#include "SocialGroupAIActor.generated.h"

class UEventSmartObjectComponent;

UCLASS(BlueprintType, meta = (DisplayName = "Social Group Actor (AI)"))
class MAJICEVENTAIRUNTIME_API ASocialGroupAIActor : public ASocialGroupActor
{
	GENERATED_BODY()

public:
	ASocialGroupAIActor();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smart Object")
	TObjectPtr<UEventSmartObjectComponent> SmartObject;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Object", meta = (ClampMin = "1.0", Units = "s"))
	float OccupancyDuration = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Object", meta = (ClampMin = "0.0", Units = "s"))
	float OccupancyVariance = 0.0f;

private:
	void RefreshSmartObjectSlots();
};
