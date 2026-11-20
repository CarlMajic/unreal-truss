#include "BarSeatingAIActor.h"

#include "EventSmartObjectComponent.h"
#include "GameplayTagsManager.h"

ABarSeatingAIActor::ABarSeatingAIActor()
{
	SmartObject = CreateDefaultSubobject<UEventSmartObjectComponent>(TEXT("SmartObject"));
	SmartObject->InteractionTag = FGameplayTag::RequestGameplayTag(FName(TEXT("EventInteract.Bar.Order")), false);
	SmartObject->BaseOccupancyDuration = 20.0f;
	SmartObject->OccupancyVariance = 8.0f;
	SmartObject->MinimumTestSlotCount = 2;
	SmartObject->TestSlotClusterRadiusCm = 180.0f;
}

void ABarSeatingAIActor::BeginPlay()
{
	Super::BeginPlay();
	RefreshSmartObjectSlots();
}

void ABarSeatingAIActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshSmartObjectSlots();
}

void ABarSeatingAIActor::RefreshSmartObjectSlots()
{
	if (!SmartObject)
	{
		return;
	}

	SmartObject->TestSlotClusterRadiusCm = AISlotPreviewSpreadRadiusCm;
	SmartObject->TestSlotClusterStartAngleDegrees = AISlotPreviewStartAngleDegrees;
	SmartObject->AutoPopulatePositionOffset = AISlotPreviewOffsetCm;
	SmartObject->AutoPopulateSlotsFromMeshComponentsFacingLocalPoint(
		TArray<FString>{ TEXT("Stool") },
		AISlotFacingLocalPointCm,
		AISlotFacingYawOffsetDegrees);

	SmartObject->DrawDebugSlots();
}
