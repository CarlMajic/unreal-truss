#include "EventBTTask_WaitAtSlot.h"

#include "EventNPCActor.h"
#include "EventSmartObjectComponent.h"
#include "AIController.h"

UEventBTTask_WaitAtSlot::UEventBTTask_WaitAtSlot()
{
	NodeName = TEXT("Wait At Slot");
	bNotifyTick = true;
}

EBTNodeResult::Type UEventBTTask_WaitAtSlot::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	AEventNPCActor* NPC = Controller ? Cast<AEventNPCActor>(Controller->GetPawn()) : nullptr;

	if (!NPC || !NPC->HasClaimedSlot())
	{
		UE_LOG(LogTemp, Display, TEXT("[EventAI] WaitAtSlot skipped for %s: no claimed slot"),
			*GetNameSafe(NPC));
		return EBTNodeResult::Succeeded;
	}

	FEventWaitAtSlotMemory* Memory = reinterpret_cast<FEventWaitAtSlotMemory*>(NodeMemory);
	Memory->ElapsedTime = 0.0f;
	Memory->WaitDuration = NPC->GetClaimedSmartObject()->GetOccupancyDuration();
	Memory->bLoggedHeldSocialSlot = false;
	UE_LOG(LogTemp, Display, TEXT("[EventAI] %s waiting at claimed slot for %.1f seconds"),
		*NPC->GetName(),
		Memory->WaitDuration);

	return EBTNodeResult::InProgress;
}

void UEventBTTask_WaitAtSlot::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FEventWaitAtSlotMemory* Memory = reinterpret_cast<FEventWaitAtSlotMemory*>(NodeMemory);
	Memory->ElapsedTime += DeltaSeconds;

	if (Memory->ElapsedTime >= Memory->WaitDuration)
	{
		AAIController* Controller = OwnerComp.GetAIOwner();
		AEventNPCActor* NPC = Controller ? Cast<AEventNPCActor>(Controller->GetPawn()) : nullptr;
		if (NPC)
		{
			if (NPC->ShouldHoldClaimedSocialSlots())
			{
				if (!Memory->bLoggedHeldSocialSlot)
				{
					UE_LOG(LogTemp, Display, TEXT("[EventAI] %s holding claimed social slot after WaitAtSlot"), *NPC->GetName());
					Memory->bLoggedHeldSocialSlot = true;
				}
				return;
			}
			UE_LOG(LogTemp, Display, TEXT("[EventAI] %s marked claimed slot ready to leave after WaitAtSlot"), *NPC->GetName());
			NPC->MarkClaimedSlotReadyToLeave();
		}
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

EBTNodeResult::Type UEventBTTask_WaitAtSlot::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	AEventNPCActor* NPC = Controller ? Cast<AEventNPCActor>(Controller->GetPawn()) : nullptr;
	if (NPC)
	{
		NPC->ReleaseClaimedSlot();
	}
	return EBTNodeResult::Aborted;
}
