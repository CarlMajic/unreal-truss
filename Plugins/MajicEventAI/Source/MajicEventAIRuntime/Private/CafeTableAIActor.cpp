#include "CafeTableAIActor.h"

#include "EventSmartObjectComponent.h"
#include "GameplayTagsManager.h"

ACafeTableAIActor::ACafeTableAIActor()
{
	SmartObject = CreateDefaultSubobject<UEventSmartObjectComponent>(TEXT("SmartObject"));
	SmartObject->InteractionTag = FGameplayTag::RequestGameplayTag(FName(TEXT("EventInteract.Cafe.Sit")), false);
	SmartObject->BaseOccupancyDuration = 24.0f;
	SmartObject->OccupancyVariance = 8.0f;
	SmartObject->MinimumTestSlotCount = 2;
	SmartObject->TestSlotClusterRadiusCm = 170.0f;
}

void ACafeTableAIActor::BeginPlay()
{
	Super::BeginPlay();
	RefreshSmartObjectSlots();
}

void ACafeTableAIActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshSmartObjectSlots();
}

void ACafeTableAIActor::RefreshSmartObjectSlots()
{
	if (!SmartObject)
	{
		return;
	}

	SmartObject->TestSlotClusterRadiusCm = AISlotPreviewSpreadRadiusCm;
	SmartObject->TestSlotClusterStartAngleDegrees = AISlotPreviewStartAngleDegrees;
	SmartObject->AutoPopulatePositionOffset = AISlotPreviewOffsetCm;
	SmartObject->AutoPopulateSlotsFromMeshComponentsFacingLocalPoint(
		TArray<FString>{ TEXT("DiningChair") },
		AISlotFacingLocalPointCm,
		AISlotFacingYawOffsetDegrees);

	SmartObject->DrawDebugSlots();
}
