#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EventNPCController.generated.h"

class UBehaviorTree;
class UBlackboardComponent;

UCLASS(BlueprintType)
class MAJICEVENTAIRUNTIME_API AEventNPCController : public AAIController
{
	GENERATED_BODY()

public:
	AEventNPCController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

public:
	// ── Blackboard key names — must match keys in the BB asset ────────────────

	static const FName BB_EventPhase;
	static const FName BB_BarAffinity;
	static const FName BB_Sociability;
	static const FName BB_Punctuality;
	static const FName BB_StageProximity;
	static const FName BB_TargetLocation;

	// Push updated personality values into the Blackboard
	UFUNCTION(BlueprintCallable, Category = "NPC Controller")
	void PushPersonalityToBlackboard();

	// Update the phase key so BT decorators can react
	UFUNCTION(BlueprintCallable, Category = "NPC Controller")
	void SetBlackboardPhase(uint8 PhaseIndex);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC Controller")
	TObjectPtr<UBlackboardComponent> BlackboardComp;
};
