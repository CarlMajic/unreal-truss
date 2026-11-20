#pragma once

#include "CoreMinimal.h"
#include "BarSeatingActor.h"
#include "BarSeatingAIActor.generated.h"

class UEventSmartObjectComponent;

UCLASS(BlueprintType, meta = (DisplayName = "Bar Seating Actor (AI)"))
class MAJICEVENTAIRUNTIME_API ABarSeatingAIActor : public ABarSeatingActor
{
	GENERATED_BODY()

public:
	ABarSeatingAIActor();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smart Object")
	TObjectPtr<UEventSmartObjectComponent> SmartObject;

private:
	void RefreshSmartObjectSlots();
};
