#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "EventBTTask_Wander.generated.h"

USTRUCT()
struct FEventWanderTaskMemory
{
	GENERATED_BODY()

	FVector TargetLocation = FVector::ZeroVector;
	FVector LastObservedLocation = FVector::ZeroVector;
	float ElapsedTime = 0.0f;
	float StuckTime = 0.0f;
	float BlockedNearTargetTime = 0.0f;
	float StationaryWaitDuration = 0.0f;
	bool bLoggedDirectMovementFallback = false;
	bool bRegisteredActiveWander = false;
	bool bStationaryWait = false;
	bool bExitClearanceMove = false;
};

// Picks a mostly-forward point on the NavMesh and moves the NPC there.
// Returns Succeeded on arrival, Failed if no valid point found.

UCLASS()
class MAJICEVENTAIRUNTIME_API UEventBTTask_Wander : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UEventBTTask_Wander();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FEventWanderTaskMemory); }

	// Maximum distance from the NPC's current position to pick a wander destination (cm).
	UPROPERTY(EditAnywhere, Category = "Wander", meta = (ClampMin = "100.0", Units = "cm"))
	float WanderRadius = 915.0f;

	// Minimum distance for a wander segment. Higher values reduce short back-and-forth triangle walking.
	UPROPERTY(EditAnywhere, Category = "Wander", meta = (ClampMin = "100.0", Units = "cm"))
	float MinWanderDistance = 650.0f;

	// Small random heading variation used when the forward path is clear.
	UPROPERTY(EditAnywhere, Category = "Wander", meta = (ClampMin = "0.0", ClampMax = "90.0", Units = "deg"))
	float ForwardDirectionJitterDegrees = 14.0f;

	// Larger turn angle used only after forward-biased candidates fail.
	UPROPERTY(EditAnywhere, Category = "Wander", meta = (ClampMin = "15.0", ClampMax = "180.0", Units = "deg"))
	float ObstacleTurnDegrees = 55.0f;

	// Number of forward-biased candidates to try before looking sideways.
	UPROPERTY(EditAnywhere, Category = "Wander", meta = (ClampMin = "1", ClampMax = "12"))
	int32 ForwardCandidateAttempts = 8;

	UPROPERTY(EditAnywhere, Category = "Wander|Open Space", meta = (ClampMin = "0.0", Units = "cm"))
	float OpenSpaceCheckRadius = 900.0f;

	UPROPERTY(EditAnywhere, Category = "Wander|Open Space", meta = (ClampMin = "0.0"))
	float NearbySmartObjectPenalty = 0.35f;

	UPROPERTY(EditAnywhere, Category = "Wander|Open Space", meta = (ClampMin = "0.0"))
	float NearbyNPCPenalty = 0.45f;

	UPROPERTY(EditAnywhere, Category = "Wander|Open Space", meta = (ClampMin = "0.0"))
	float TravelDistanceScoreWeight = 0.35f;

	UPROPERTY(EditAnywhere, Category = "Wander|Traffic Areas", meta = (ClampMin = "0.0"))
	float TrafficAreaScoreWeight = 0.0f;

	// Short first step after leaving a smart-object slot. Keeps guests from immediately pathing across the table/bar they just used.
	UPROPERTY(EditAnywhere, Category = "Wander|Exit Clearance", meta = (ClampMin = "0.0", Units = "cm"))
	float ExitClearanceDistance = 320.0f;

	UPROPERTY(EditAnywhere, Category = "Wander|Exit Clearance", meta = (ClampMin = "1.0", Units = "cm"))
	float ExitClearanceProjectionExtent = 220.0f;

	UPROPERTY(EditAnywhere, Category = "Wander|Open Space", meta = (ClampMin = "1", ClampMax = "24"))
	int32 RandomFallbackCandidateAttempts = 10;

	UPROPERTY(EditAnywhere, Category = "Wander|Crowd Limit", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxActiveWanderRatio = 0.10f;

	UPROPERTY(EditAnywhere, Category = "Wander|Crowd Limit", meta = (ClampMin = "0.0", Units = "s"))
	float MinStationaryWaitWhenWanderLimited = 8.0f;

	UPROPERTY(EditAnywhere, Category = "Wander|Crowd Limit", meta = (ClampMin = "0.0", Units = "s"))
	float MaxStationaryWaitWhenWanderLimited = 20.0f;

	UPROPERTY(EditAnywhere, Category = "Wander")
	bool bRequireCompletePath = true;

	UPROPERTY(EditAnywhere, Category = "Wander", meta = (ClampMin = "0.0", ClampMax = "180.0", Units = "deg"))
	float EscapeTurnDegrees = 125.0f;

	UPROPERTY(EditAnywhere, Category = "Wander", meta = (ClampMin = "1.0", Units = "cm"))
	float AcceptanceRadius = 80.0f;

	UPROPERTY(EditAnywhere, Category = "Wander", meta = (ClampMin = "1.0", Units = "s"))
	float MaxMoveTimeSeconds = 20.0f;

	UPROPERTY(EditAnywhere, Category = "Wander", meta = (ClampMin = "0.0", Units = "cm"))
	float NearTargetBlockedDistance = 180.0f;

	UPROPERTY(EditAnywhere, Category = "Wander", meta = (ClampMin = "0.0", Units = "s"))
	float MaxBlockedNearTargetSeconds = 3.0f;

	// Prototype-only straight-line fallback. Leave disabled when using real collision/navmesh.
	UPROPERTY(EditAnywhere, Category = "Wander")
	bool bUseDirectMovementFallback = false;
};
