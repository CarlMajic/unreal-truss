#include "SocialGroupAIActor.h"

#include "EventSmartObjectComponent.h"
#include "GameplayTagsManager.h"

ASocialGroupAIActor::ASocialGroupAIActor()
{
	SmartObject = CreateDefaultSubobject<UEventSmartObjectComponent>(TEXT("SmartObject"));
	SmartObject->InteractionTag = FGameplayTag::RequestGameplayTag(FName(TEXT("EventInteract.Social.Talk")), false);
	SmartObject->BaseOccupancyDuration = OccupancyDuration;
	SmartObject->OccupancyVariance = OccupancyVariance;
	SmartObject->MinimumTestSlotCount = 1;
	SmartObject->bEnsureMinimumTestSlots = false;
}

void ASocialGroupAIActor::BeginPlay()
{
	Super::BeginPlay();
	RefreshSmartObjectSlots();
}

void ASocialGroupAIActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshSmartObjectSlots();
}

void ASocialGroupAIActor::RefreshSmartObjectSlots()
{
	if (!SmartObject)
	{
		return;
	}

	TArray<FVector> AISlotOffsets;
	GetAISlotPreviewLocalOffsets(AISlotOffsets);
	TArray<FRotator> AISlotRotations;
	GetAISlotPreviewLocalRotations(AISlotRotations);
	TArray<TSoftObjectPtr<UAnimationAsset>> AISlotAnimations;
	GetAISlotPreviewAnimations(AISlotAnimations);

	SmartObject->BaseOccupancyDuration = OccupancyDuration;
	SmartObject->OccupancyVariance = OccupancyVariance;
	SmartObject->MinimumTestSlotCount = CharacterCount;
	SmartObject->TestSlotClusterRadiusCm = GroupRadiusCm;
	SmartObject->TestSlotClusterStartAngleDegrees = StartAngleDegrees;
	SmartObject->SetSlotsFromLocalOffsetsAndRotations(AISlotOffsets, AISlotRotations);
	SmartObject->SetSlotAnimations(AISlotAnimations);
	SmartObject->DrawDebugSlots();
}
