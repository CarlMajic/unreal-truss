#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"
#include "EventBTTask_FindSmartObject.generated.h"

USTRUCT()
struct FEventFindSmartObjectTaskMemory
{
	GENERATED_BODY()

	FVector TargetLocation = FVector::ZeroVector;
	FVector ApproachLocation = FVector::ZeroVector;
	FVector FinalSlotLocation = FVector::ZeroVector;
	FVector LastObservedLocation = FVector::ZeroVector;
	FRotator TargetRotation = FRotator::ZeroRotator;
	float ElapsedTime = 0.0f;
	float StuckTime = 0.0f;
	float BlockedNearTargetTime = 0.0f;
	float OccupancyElapsedTime = 0.0f;
	float OccupancyDuration = 0.0f;
	int32 PathIdleRepathAttempts = 0;
	bool bMoveRequested = false;
	bool bArrivedAtSlot = false;
	bool bUsingTrafficWaypoint = false;
	bool bUsingApproachMove = false;
	bool bLoggedDirectMovementFallback = false;
	bool bLoggedHeldSocialSlot = false;
};

// Finds a weighted-random EventSmartObjectComponent matching InteractionTag that has an
// available slot, claims it, stores the slot world position in BB_TargetLocation,
// and records the claim on the NPC actor.
// Returns Succeeded if a slot was claimed, Failed if none available (BT falls through to Wander).

UCLASS()
class MAJICEVENTAIRUNTIME_API UEventBTTask_FindSmartObject : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UEventBTTask_FindSmartObject();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FEventFindSmartObjectTaskMemory); }
	virtual FString GetStaticDescription() const override;

	// The interaction type to search for — set this on each BT node instance
	UPROPERTY(EditAnywhere, Category = "Smart Object")
	FGameplayTag SearchTag;

	UPROPERTY(EditAnywhere, Category = "Smart Object")
	bool bSearchAllEventInteractionsForPrototype = true;

	// Blackboard key to write the claimed slot world position into
	UPROPERTY(EditAnywhere, Category = "Smart Object")
	FBlackboardKeySelector TargetLocationKey;

	UPROPERTY(EditAnywhere, Category = "Smart Object", meta = (ClampMin = "1.0", Units = "cm"))
	float AcceptanceRadius = 80.0f;

	UPROPERTY(EditAnywhere, Category = "Smart Object", meta = (ClampMin = "1.0", Units = "s"))
	float MaxMoveTimeSeconds = 30.0f;

	UPROPERTY(EditAnywhere, Category = "Smart Object", meta = (ClampMin = "0.0", Units = "cm"))
	float NearTargetBlockedDistance = 180.0f;

	UPROPERTY(EditAnywhere, Category = "Smart Object", meta = (ClampMin = "0.0", Units = "s"))
	float MaxBlockedNearTargetSeconds = 3.0f;

	UPROPERTY(EditAnywhere, Category = "Smart Object", meta = (ClampMin = "0.0", Units = "s"))
	float PathIdleGraceSeconds = 2.5f;

	UPROPERTY(EditAnywhere, Category = "Smart Object", meta = (ClampMin = "0", ClampMax = "5"))
	int32 MaxPathIdleRepathAttempts = 2;

	UPROPERTY(EditAnywhere, Category = "Smart Object", meta = (ClampMin = "0.0", Units = "cm"))
	float MaxProjectedSlotDistance = 140.0f;

	UPROPERTY(EditAnywhere, Category = "Smart Object|Approach", meta = (ClampMin = "0.0", Units = "cm"))
	float GeneratedSlotApproachDistance = 260.0f;

	UPROPERTY(EditAnywhere, Category = "Smart Object|Approach", meta = (ClampMin = "1.0", Units = "cm"))
	float GeneratedSlotApproachProjectionExtent = 220.0f;

	UPROPERTY(EditAnywhere, Category = "Smart Object")
	bool bRequireCompletePathToSlot = true;

	UPROPERTY(EditAnywhere, Category = "Smart Object", meta = (ClampMin = "0.0", Units = "cm"))
	float MaxPathDistanceToSlot = 4500.0f;

	UPROPERTY(EditAnywhere, Category = "Smart Object", meta = (ClampMin = "1.0"))
	float MaxPathToStraightLineRatio = 3.0f;

	UPROPERTY(EditAnywhere, Category = "Smart Object")
	bool bWaitAtClaimedSlot = true;

	UPROPERTY(EditAnywhere, Category = "Smart Object|Scoring", meta = (ClampMin = "0.0"))
	float DistanceScoreWeight = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Smart Object|Scoring", meta = (ClampMin = "100.0", Units = "cm"))
	float FullDistanceScoreAt = 2000.0f;

	UPROPERTY(EditAnywhere, Category = "Smart Object|Scoring", meta = (ClampMin = "0.0"))
	float AvailableSlotScoreWeight = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Smart Object|Scoring", meta = (ClampMin = "0.0"))
	float PersonalityScoreWeight = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Smart Object|Scoring", meta = (ClampMin = "0.0"))
	float NearbyFallbackWeightScale = 0.2f;

	UPROPERTY(EditAnywhere, Category = "Smart Object|Scoring", meta = (ClampMin = "0.0"))
	float OccupancyPenaltyWeight = 0.35f;

	UPROPERTY(EditAnywhere, Category = "Smart Object|Scoring", meta = (ClampMin = "0.0", Units = "cm"))
	float DestinationCrowdingRadius = 900.0f;

	UPROPERTY(EditAnywhere, Category = "Smart Object|Scoring", meta = (ClampMin = "0.0"))
	float NearbySmartObjectPenaltyWeight = 0.35f;

	UPROPERTY(EditAnywhere, Category = "Smart Object|Scoring", meta = (ClampMin = "0.0"))
	float NearbyNPCPenaltyWeight = 0.45f;

	UPROPERTY(EditAnywhere, Category = "Smart Object|Traffic Areas")
	bool bUsePreferredTrafficAreas = false;

	UPROPERTY(EditAnywhere, Category = "Smart Object|Traffic Areas", meta = (ClampMin = "0.0", Units = "cm", EditCondition = "bUsePreferredTrafficAreas"))
	float MinTrafficRouteDistance = 1400.0f;

	UPROPERTY(EditAnywhere, Category = "Smart Object|Traffic Areas", meta = (ClampMin = "1.0", EditCondition = "bUsePreferredTrafficAreas"))
	float MaxTrafficRouteDetourRatio = 1.55f;

	// Prototype-only straight-line fallback. Leave disabled when using real collision/navmesh.
	UPROPERTY(EditAnywhere, Category = "Smart Object")
	bool bUseDirectMovementFallback = false;
};
