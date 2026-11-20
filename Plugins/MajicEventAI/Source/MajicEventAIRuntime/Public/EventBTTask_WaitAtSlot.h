#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "EventBTTask_WaitAtSlot.generated.h"

USTRUCT()
struct FEventWaitAtSlotMemory
{
	GENERATED_BODY()
	float ElapsedTime = 0.0f;
	float WaitDuration = 30.0f;
	bool bLoggedHeldSocialSlot = false;
};

// Waits at the NPC's currently claimed smart object slot for its occupancy duration,
// then releases the slot and returns Succeeded so the BT loops back to find a new one.

UCLASS()
class MAJICEVENTAIRUNTIME_API UEventBTTask_WaitAtSlot : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UEventBTTask_WaitAtSlot();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FEventWaitAtSlotMemory); }
};
