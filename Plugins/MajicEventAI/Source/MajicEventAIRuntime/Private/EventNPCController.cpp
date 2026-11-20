#include "EventNPCController.h"

#include "EventNPCActor.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Navigation/CrowdFollowingComponent.h"

const FName AEventNPCController::BB_EventPhase      = TEXT("EventPhase");
const FName AEventNPCController::BB_BarAffinity     = TEXT("BarAffinity");
const FName AEventNPCController::BB_Sociability     = TEXT("Sociability");
const FName AEventNPCController::BB_Punctuality     = TEXT("Punctuality");
const FName AEventNPCController::BB_StageProximity  = TEXT("StageProximity");
const FName AEventNPCController::BB_TargetLocation  = TEXT("TargetLocation");

AEventNPCController::AEventNPCController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
	BlackboardComp = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComp"));
}

void AEventNPCController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	AEventNPCActor* NPC = Cast<AEventNPCActor>(InPawn);
	if (!NPC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EventAI] %s possessed non-event pawn %s"), *GetName(), *GetNameSafe(InPawn));
		return;
	}

	if (!NPC->BehaviorTree)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EventAI] %s has no BehaviorTree assigned"), *NPC->GetName());
		return;
	}

	if (!NPC->BehaviorTree->BlackboardAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EventAI] %s BehaviorTree %s has no Blackboard asset assigned"), *NPC->GetName(), *NPC->BehaviorTree->GetName());
		return;
	}

	UBlackboardComponent* BBComp = nullptr;
	if (!UseBlackboard(NPC->BehaviorTree->BlackboardAsset, BBComp) || !BBComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EventAI] %s failed to initialize Blackboard %s"), *NPC->GetName(), *NPC->BehaviorTree->BlackboardAsset->GetName());
		return;
	}

	BlackboardComp = BBComp;
	if (!RunBehaviorTree(NPC->BehaviorTree))
	{
		UE_LOG(LogTemp, Warning, TEXT("[EventAI] %s failed to run BehaviorTree %s"), *NPC->GetName(), *NPC->BehaviorTree->GetName());
		return;
	}

	PushPersonalityToBlackboard();
	if (UCrowdFollowingComponent* CrowdFollowing = Cast<UCrowdFollowingComponent>(GetPathFollowingComponent()))
	{
		CrowdFollowing->SetCrowdSimulationState(ECrowdSimulationState::Enabled);
		CrowdFollowing->SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::High);
		CrowdFollowing->SetCrowdSeparation(true);
		CrowdFollowing->SetCrowdSeparationWeight(2.0f);
		CrowdFollowing->SetCrowdCollisionQueryRange(450.0f);
		CrowdFollowing->SetCrowdPathOffset(true);
		CrowdFollowing->SetCrowdOptimizeVisibility(true);
		CrowdFollowing->SetCrowdOptimizeTopology(true);
	}
	UE_LOG(LogTemp, Display, TEXT("[EventAI] %s running BehaviorTree %s with Blackboard %s"), *NPC->GetName(), *NPC->BehaviorTree->GetName(), *NPC->BehaviorTree->BlackboardAsset->GetName());
}

void AEventNPCController::OnUnPossess()
{
	StopMovement();
	Super::OnUnPossess();
}

void AEventNPCController::PushPersonalityToBlackboard()
{
	AEventNPCActor* NPC = Cast<AEventNPCActor>(GetPawn());
	if (!NPC || !BlackboardComp)
	{
		return;
	}

	const FNPCPersonality& P = NPC->GetPersonality();
	BlackboardComp->SetValueAsFloat(BB_BarAffinity,    P.BarAffinity);
	BlackboardComp->SetValueAsFloat(BB_Sociability,    P.Sociability);
	BlackboardComp->SetValueAsFloat(BB_Punctuality,    P.Punctuality);
	BlackboardComp->SetValueAsFloat(BB_StageProximity, P.StageProximity);
	BlackboardComp->SetValueAsEnum(BB_EventPhase,      static_cast<uint8>(NPC->GetCurrentPhase()));
}

void AEventNPCController::SetBlackboardPhase(uint8 PhaseIndex)
{
	if (BlackboardComp)
	{
		BlackboardComp->SetValueAsEnum(BB_EventPhase, PhaseIndex);
	}
}
