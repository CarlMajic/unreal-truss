#pragma once

#include "CoreMinimal.h"
#include "LoungeLayoutActor.h"
#include "LoungeLayoutAIActor.generated.h"

class UEventSmartObjectComponent;

// Drop this into the level instead of LoungeLayoutActor.
// Identical furniture configuration — just also registers NPC seating slots automatically.

UCLASS(BlueprintType, meta = (DisplayName = "Lounge Layout Actor (AI)"))
class MAJICEVENTAIRUNTIME_API ALoungeLayoutAIActor : public ALoungeLayoutActor
{
	GENERATED_BODY()

public:
	ALoungeLayoutAIActor();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smart Object")
	TObjectPtr<UEventSmartObjectComponent> SmartObject;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Object|Debug", meta = (ClampMin = "0"))
	int32 MinimumTestSlotCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Object|Debug", meta = (ClampMin = "50.0", Units = "cm"))
	float TestSlotClusterRadiusCm = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Object|Debug", meta = (ClampMin = "1.0", Units = "s"))
	float TestOccupancyDuration = 24.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Object|Debug", meta = (ClampMin = "0.0", Units = "s"))
	float TestOccupancyVariance = 8.0f;

private:
	void RefreshSmartObjectSlots();
};
