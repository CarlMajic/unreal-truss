#pragma once

#include "CoreMinimal.h"
#include "CafeTableActor.h"
#include "CafeTableAIActor.generated.h"

class UEventSmartObjectComponent;

UCLASS(BlueprintType, meta = (DisplayName = "Cafe Table Actor (AI)"))
class MAJICEVENTAIRUNTIME_API ACafeTableAIActor : public ACafeTableActor
{
	GENERATED_BODY()

public:
	ACafeTableAIActor();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smart Object")
	TObjectPtr<UEventSmartObjectComponent> SmartObject;

private:
	void RefreshSmartObjectSlots();
};
