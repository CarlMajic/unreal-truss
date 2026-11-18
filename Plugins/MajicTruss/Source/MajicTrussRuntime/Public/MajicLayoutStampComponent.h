#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MajicLayoutStampComponent.generated.h"

// Added to every actor the Event Layout importer creates (and to unstamped actors on export). It
// carries the item id from the file and the item's params as they were read, so that export
// writes the same id back and keeps fields this plugin cannot show (for example a stage's
// surface or skirt that a different program set).
UCLASS(ClassGroup = "Majic", meta = (BlueprintSpawnableComponent))
class MAJICTRUSSRUNTIME_API UMajicLayoutStampComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Majic Layout")
	FString ItemId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Majic Layout")
	FString ItemName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Majic Layout")
	FString ParamsJson;
};
