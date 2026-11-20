#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EventDirectorActor.generated.h"

class AEventEntranceActor;
class AEventNPCActor;
class USoundBase;

// ── Phase sequence ────────────────────────────────────────────────────────────

UENUM(BlueprintType)
enum class EEventPhase : uint8
{
	DoorsClosed     UMETA(DisplayName = "Doors Closed"),
	Arriving        UMETA(DisplayName = "Arriving"),
	CocktailHour    UMETA(DisplayName = "Cocktail Hour"),
	SeatingAnnounced UMETA(DisplayName = "Seating Announced"),
	ProgramActive   UMETA(DisplayName = "Program Active"),
	Applause        UMETA(DisplayName = "Applause"),
	PostShow        UMETA(DisplayName = "Post Show"),
	Exiting         UMETA(DisplayName = "Exiting")
};

// ── Behavior weights per phase ────────────────────────────────────────────────
// Each float is a 0-1 multiplier applied to NPC personality affinities.
// A weight of 0 effectively disables that behavior in this phase.

USTRUCT(BlueprintType)
struct FEventBehaviorWeights
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weights", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Bar = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weights", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Social = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weights", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Seating = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weights", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Stage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weights", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Exit = 0.0f;
};

// ── Per-phase configuration ───────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct FEventPhaseConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase")
	EEventPhase Phase = EEventPhase::DoorsClosed;

	// 0 = manual advance only
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase", meta = (ClampMin = "0.0", Units = "s"))
	float DurationSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase")
	bool bAutoAdvance = false;

	// Played at the moment this phase begins (announcements, music stings, etc.)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase")
	TObjectPtr<USoundBase> AnnouncementSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase")
	FEventBehaviorWeights BehaviorWeights;
};

// ── Delegate ──────────────────────────────────────────────────────────────────

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEventPhaseChanged, EEventPhase, PreviousPhase, EEventPhase, NewPhase);

// ── Actor ─────────────────────────────────────────────────────────────────────

UCLASS(BlueprintType)
class MAJICEVENTAIRUNTIME_API AEventDirectorActor : public AActor
{
	GENERATED_BODY()

public:
	AEventDirectorActor();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }

public:
	// ── Phase control ─────────────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "Event Director")
	void AdvancePhase();

	UFUNCTION(BlueprintCallable, Category = "Event Director")
	void SetPhase(EEventPhase NewPhase);

	UFUNCTION(BlueprintPure, Category = "Event Director")
	EEventPhase GetCurrentPhase() const { return CurrentPhase; }

	UFUNCTION(BlueprintPure, Category = "Event Director")
	FEventBehaviorWeights GetCurrentBehaviorWeights() const;

	// ── Registration ──────────────────────────────────────────────────────────
	// Entrance and NPC actors call these on BeginPlay to register themselves.

	UFUNCTION(BlueprintCallable, Category = "Event Director")
	void RegisterEntrance(AEventEntranceActor* Entrance);

	UFUNCTION(BlueprintCallable, Category = "Event Director")
	void RegisterNPC(AEventNPCActor* NPC);

	UFUNCTION(BlueprintCallable, Category = "Event Director")
	void UnregisterNPC(AEventNPCActor* NPC);

	// ── Events ────────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "Event Director")
	FOnEventPhaseChanged OnPhaseChanged;

	// Blueprint-implementable hook fired on every phase transition
	UFUNCTION(BlueprintImplementableEvent, Category = "Event Director")
	void OnPhaseChangedEvent(EEventPhase PreviousPhase, EEventPhase NewPhase);

	// ── Configuration ─────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Director")
	TArray<FEventPhaseConfig> PhaseConfigs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Director")
	EEventPhase StartingPhase = EEventPhase::DoorsClosed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Director|Debug")
	bool bDrawProblemLaneMarker = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Director|Debug", meta = (EditCondition = "bDrawProblemLaneMarker", Units = "cm"))
	float ProblemLaneX = 1120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Director|Debug", meta = (EditCondition = "bDrawProblemLaneMarker", Units = "cm"))
	float ProblemLaneMinY = -3300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Director|Debug", meta = (EditCondition = "bDrawProblemLaneMarker", Units = "cm"))
	float ProblemLaneMaxY = 1300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Director|Debug", meta = (EditCondition = "bDrawProblemLaneMarker", Units = "cm"))
	float ProblemLaneMarkerHeight = 420.0f;

private:
	UPROPERTY(VisibleInstanceOnly, Category = "Event Director")
	EEventPhase CurrentPhase = EEventPhase::DoorsClosed;

	UPROPERTY(VisibleInstanceOnly, Category = "Event Director")
	float PhaseElapsedSeconds = 0.0f;

	UPROPERTY()
	TArray<TObjectPtr<AEventEntranceActor>> RegisteredEntrances;

	UPROPERTY()
	TArray<TObjectPtr<AEventNPCActor>> RegisteredNPCs;

	const FEventPhaseConfig* FindPhaseConfig(EEventPhase Phase) const;
	void BroadcastPhaseToNPCs(EEventPhase NewPhase);
	void DrawProblemLaneMarker() const;
};
