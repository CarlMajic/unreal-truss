#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EventDirectorActor.h"
#include "EventEntranceActor.generated.h"

class AEventNPCActor;
class AEventDirectorActor;
class UEventSmartObjectComponent;
class UArrowComponent;
class UBillboardComponent;

// ── NPC personality ranges set per entrance ───────────────────────────────────
// Each NPC spawned from this entrance gets random values within these ranges.
// Lets you bias different entrances — VIP door spawns high-social guests, etc.

USTRUCT(BlueprintType)
struct FNPCPersonalityRange
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BarAffinityMin = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BarAffinityMax = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SociabilityMin = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SociabilityMax = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PunctualityMin = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PunctualityMax = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StageProximityMin = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StageProximityMax = 0.9f;
};

// ── Actor ─────────────────────────────────────────────────────────────────────

UCLASS(BlueprintType)
class MAJICEVENTAIRUNTIME_API AEventEntranceActor : public AActor
{
	GENERATED_BODY()

public:
	AEventEntranceActor();

protected:
	virtual void BeginPlay() override;

public:
	// ── Configuration ─────────────────────────────────────────────────────────

	// NPC class to spawn — swap to MetaHuman BP subclass when ready
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entrance")
	TSubclassOf<AEventNPCActor> NPCClass;

	// Total guests this entrance will spawn across the Arriving phase
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entrance", meta = (ClampMin = "1"))
	int32 MaxNPCCount = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entrance|Testing")
	bool bStartSpawningOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entrance|Testing", meta = (ClampMin = "0"))
	int32 InitialSpawnBurstCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entrance|Testing")
	bool bPlaceInitialBurstAtSmartObjects = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entrance|Testing")
	bool bPlaceSpawnsAtSmartObjects = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entrance|Testing", meta = (ClampMin = "0", EditCondition = "bPlaceInitialBurstAtSmartObjects"))
	int32 InitialPlacedSmartObjectCount = 12;

	// Seconds between each spawn (randomized between min and max)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entrance", meta = (ClampMin = "0.5"))
	float SpawnIntervalMin = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entrance", meta = (ClampMin = "0.5"))
	float SpawnIntervalMax = 6.0f;

	// Random scatter radius around the entrance point (cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entrance", meta = (ClampMin = "0.0", Units = "cm"))
	float SpawnRadiusCm = 100.0f;

	// Personality ranges for NPCs spawned from this entrance
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entrance")
	FNPCPersonalityRange PersonalityRange;

	// Optional explicit reference — auto-found in the level if left empty
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entrance")
	TObjectPtr<AEventDirectorActor> EventDirector;

	// ── State ─────────────────────────────────────────────────────────────────

	UFUNCTION(BlueprintPure, Category = "Entrance")
	int32 GetSpawnedCount() const { return SpawnedCount; }

	UFUNCTION(BlueprintPure, Category = "Entrance")
	bool IsSpawning() const { return bSpawning; }

	// ── Phase response ────────────────────────────────────────────────────────

	UFUNCTION()
	void OnEventPhaseChanged(EEventPhase PreviousPhase, EEventPhase NewPhase);

private:
	void StartSpawning();
	void StopSpawning();
	void SpawnNextNPC();
	bool SpawnPlacedNPCAtSmartObject();
	bool FindAvailableInitialSmartObjectSlot(UEventSmartObjectComponent*& OutSmartObject, int32& OutSlotIndex, FTransform& OutSlotTransform) const;
	void InitializeSpawnedNPC(AEventNPCActor* NPC);
	void ScheduleNextSpawn();

	FTimerHandle SpawnTimerHandle;

	int32 SpawnedCount = 0;
	bool bSpawning = false;

	UPROPERTY(VisibleInstanceOnly)
	TObjectPtr<AEventDirectorActor> ResolvedDirector;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UArrowComponent> ArrowComponent;

	UPROPERTY()
	TObjectPtr<UBillboardComponent> Billboard;
#endif
};
