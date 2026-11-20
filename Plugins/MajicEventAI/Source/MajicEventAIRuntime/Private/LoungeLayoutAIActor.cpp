#include "LoungeLayoutAIActor.h"

#include "EventSmartObjectComponent.h"
#include "GameplayTagsManager.h"

ALoungeLayoutAIActor::ALoungeLayoutAIActor()
{
	SmartObject = CreateDefaultSubobject<UEventSmartObjectComponent>(TEXT("SmartObject"));
	SmartObject->InteractionTag = FGameplayTag::RequestGameplayTag(FName("EventInteract.Lounge.Sit"), false);
	SmartObject->BaseOccupancyDuration = 24.0f;
	SmartObject->OccupancyVariance = 8.0f;
}

void ALoungeLayoutAIActor::BeginPlay()
{
	Super::BeginPlay();
	RefreshSmartObjectSlots();
}

void ALoungeLayoutAIActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshSmartObjectSlots();
}

void ALoungeLayoutAIActor::RefreshSmartObjectSlots()
{
	if (!SmartObject)
	{
		return;
	}

	SmartObject->BaseOccupancyDuration = TestOccupancyDuration;
	SmartObject->OccupancyVariance = TestOccupancyVariance;
	SmartObject->TestSlotClusterRadiusCm = AISlotPreviewSpreadRadiusCm;
	SmartObject->TestSlotClusterStartAngleDegrees = AISlotPreviewStartAngleDegrees;
	SmartObject->AutoPopulatePositionOffset = AISlotPreviewOffsetCm;

	TArray<FVector> AISlotOffsets;
	GetAISlotPreviewLocalOffsets(AISlotOffsets);
	SmartObject->SetSlotsFromLocalOffsetsFacingLocalPoint(
		AISlotOffsets,
		AISlotFacingLocalPointCm,
		AISlotFacingYawOffsetDegrees);

	bool bRebuildDebugCluster = SmartObject->Slots.Num() < MinimumTestSlotCount;
	if (!bRebuildDebugCluster && SmartObject->Slots.Num() > 1)
	{
		FBox2D SlotBounds(EForceInit::ForceInit);
		for (const FEventInteractionSlot& Slot : SmartObject->Slots)
		{
			SlotBounds += FVector2D(Slot.LocalOffset.X, Slot.LocalOffset.Y);
		}

		if (SlotBounds.GetSize().GetMax() < 120.0f)
		{
			bRebuildDebugCluster = true;
		}
	}

	if (bRebuildDebugCluster && MinimumTestSlotCount > 0)
	{
		const int32 ClusterSlotCount = FMath::Max(MinimumTestSlotCount, SmartObject->Slots.Num());
		SmartObject->BuildClusterSlots(ClusterSlotCount, TestSlotClusterRadiusCm, FRotator::ZeroRotator);
		for (FEventInteractionSlot& Slot : SmartObject->Slots)
		{
			FVector Direction = AISlotFacingLocalPointCm - Slot.LocalOffset;
			Direction.Z = 0.0f;
			Slot.LocalRotation = Direction.IsNearlyZero()
				? FRotator(0.0f, AISlotFacingYawOffsetDegrees, 0.0f)
				: FRotator(0.0f, Direction.Rotation().Yaw + AISlotFacingYawOffsetDegrees, 0.0f);
		}
		UE_LOG(LogTemp, Warning, TEXT("[EventAI] %s lounge auto slots were insufficient; rebuilt %d visible cluster slots"),
			*GetName(),
			SmartObject->Slots.Num());
	}

	// Show green spheres on each seat in the viewport during play
	SmartObject->DrawDebugSlots();
}
