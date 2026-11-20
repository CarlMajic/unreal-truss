#include "EventSmartObjectComponent.h"

#include "EventNPCActor.h"
#include "MajicEventAIModule.h"
#include "Components/ArrowComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Animation/AnimationAsset.h"
#include "DrawDebugHelpers.h"
#include "GameplayTagsManager.h"

namespace
{
	FRotator MakeFacingRotation(const FVector& FromLocalOffset, const FVector& ToLocalOffset, float YawOffsetDegrees)
	{
		FVector Direction = ToLocalOffset - FromLocalOffset;
		Direction.Z = 0.0f;
		if (Direction.IsNearlyZero())
		{
			return FRotator(0.0f, YawOffsetDegrees, 0.0f);
		}

		FRotator Rotation = Direction.Rotation();
		Rotation.Pitch = 0.0f;
		Rotation.Roll = 0.0f;
		Rotation.Yaw += YawOffsetDegrees;
		return Rotation;
	}

	void BuildSpreadSlotsAroundPoint(TArray<FEventInteractionSlot>& Slots, int32 Count, const FVector& CenterLocalOffset, float RadiusCm, float StartAngleDegrees, FRotator FacingRotation)
	{
		Slots.Reset();

		const int32 SafeCount = FMath::Max(Count, 0);
		if (SafeCount <= 0)
		{
			return;
		}

		if (SafeCount == 1)
		{
			FEventInteractionSlot Slot;
			Slot.LocalOffset = CenterLocalOffset;
			Slot.LocalRotation = FacingRotation;
			Slots.Add(Slot);
			return;
		}

		const float SafeRadius = FMath::Max(RadiusCm, 100.0f);
		const float AngleStep = 360.0f / static_cast<float>(SafeCount);
		for (int32 i = 0; i < SafeCount; ++i)
		{
			const float Angle = FMath::DegreesToRadians(StartAngleDegrees + AngleStep * i);
			FEventInteractionSlot Slot;
			Slot.LocalOffset = CenterLocalOffset + FVector(FMath::Cos(Angle) * SafeRadius, FMath::Sin(Angle) * SafeRadius, 0.0f);
			Slot.LocalRotation = FacingRotation;
			Slots.Add(Slot);
		}
	}
}

UEventSmartObjectComponent::UEventSmartObjectComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	bTickInEditor = true;
}

void UEventSmartObjectComponent::BeginPlay()
{
	Super::BeginPlay();

	const FGameplayTag LoungeSitTag = FGameplayTag::RequestGameplayTag(FName(TEXT("EventInteract.Lounge.Sit")), false);
	const bool bIsLoungeInteraction = InteractionTag.IsValid() && InteractionTag == LoungeSitTag;

	if (bIsLoungeInteraction && bUseTestOccupancyTiming)
	{
		BaseOccupancyDuration = TestOccupancyDuration;
		OccupancyVariance = TestOccupancyVariance;
		UE_LOG(LogTemp, Display, TEXT("[EventSmartObject] %s using test occupancy %.1f +/- %.1f seconds"),
			*GetOwner()->GetName(),
			BaseOccupancyDuration,
			OccupancyVariance);
	}

	if (bAutoPopulateOnBeginPlay && AutoPopulateNameFilters.Num() > 0)
	{
		if (bFaceSlotsTowardLocalPoint)
		{
			AutoPopulateSlotsFromMeshComponentsFacingLocalPoint(AutoPopulateNameFilters, SlotFacingLocalPoint, SlotFacingYawOffsetDegrees);
		}
		else
		{
			AutoPopulateSlotsFromMeshComponents(AutoPopulateNameFilters, AutoPopulateFacingRotation);
		}
		UE_LOG(LogTemp, Warning, TEXT("[EventSmartObject] %s: auto-populated %d slots"), *GetOwner()->GetName(), Slots.Num());
	}

	if (bEnsureMinimumTestSlots && MinimumTestSlotCount > 0 && Slots.Num() < MinimumTestSlotCount)
	{
		const int32 OriginalSlotCount = Slots.Num();
		BuildClusterSlots(MinimumTestSlotCount, TestSlotClusterRadiusCm, AutoPopulateFacingRotation);
		if (bFaceSlotsTowardLocalPoint)
		{
			for (FEventInteractionSlot& Slot : Slots)
			{
				Slot.LocalRotation = MakeFacingRotation(Slot.LocalOffset, SlotFacingLocalPoint, SlotFacingYawOffsetDegrees);
			}
		}
		UE_LOG(LogTemp, Warning, TEXT("[EventSmartObject] %s rebuilt %d slots into %d visible test slots"),
			*GetOwner()->GetName(),
			OriginalSlotCount,
			Slots.Num());
	}

	if (bDrawDebugOnPlay)
	{
		DrawDebugSlots();
		UE_LOG(LogTemp, Warning, TEXT("[EventSmartObject] %s: drawing %d debug slots"), *GetOwner()->GetName(), Slots.Num());
	}
}

bool UEventSmartObjectComponent::TryClaimSlot(AEventNPCActor* NPC, int32& OutSlotIndex, FTransform& OutWorldTransform)
{
	if (!NPC)
	{
		return false;
	}

	if (GetClaimedSlotCount() >= GetMaxClaimedSlotCount())
	{
		return false;
	}

	const FTransform& OwnerTransform = GetOwner()->GetActorTransform();
	int32 BestSlotIndex = INDEX_NONE;
	float BestSlotScore = -FLT_MAX;

	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		if (!IsSlotAvailable(i))
		{
			continue;
		}

		const FEventInteractionSlot& Slot = Slots[i];
		const FVector WorldLocation = OwnerTransform.TransformPosition(Slot.LocalOffset);
		const float Score = bPreferDistantAvailableSlot
			? FVector::DistSquared2D(NPC->GetActorLocation(), WorldLocation)
			: -static_cast<float>(i);

		if (BestSlotIndex == INDEX_NONE || Score > BestSlotScore)
		{
			BestSlotIndex = i;
			BestSlotScore = Score;
		}
	}

	if (BestSlotIndex == INDEX_NONE)
	{
		return false;
	}

	FEventInteractionSlot& Slot = Slots[BestSlotIndex];
	Slot.ClaimedBy = NPC;
	OutSlotIndex = BestSlotIndex;

	GetSlotWorldTransform(BestSlotIndex, OutWorldTransform);

	return true;
}

bool UEventSmartObjectComponent::TryClaimSlotByIndex(AEventNPCActor* NPC, int32 SlotIndex, FTransform& OutWorldTransform)
{
	if (!NPC || !Slots.IsValidIndex(SlotIndex))
	{
		return false;
	}

	if (GetClaimedSlotCount() >= GetMaxClaimedSlotCount())
	{
		return false;
	}

	if (!IsSlotAvailable(SlotIndex))
	{
		return false;
	}

	Slots[SlotIndex].ClaimedBy = NPC;
	return GetSlotWorldTransform(SlotIndex, OutWorldTransform);
}

bool UEventSmartObjectComponent::GetSlotWorldTransform(int32 SlotIndex, FTransform& OutWorldTransform) const
{
	if (!Slots.IsValidIndex(SlotIndex) || !GetOwner())
	{
		return false;
	}

	const FTransform& OwnerTransform = GetOwner()->GetActorTransform();
	const FEventInteractionSlot& Slot = Slots[SlotIndex];
	const FVector WorldLocation = OwnerTransform.TransformPosition(Slot.LocalOffset);
	const FRotator WorldRotation = OwnerTransform.TransformRotation(Slot.LocalRotation.Quaternion()).Rotator();
	OutWorldTransform = FTransform(WorldRotation, WorldLocation);
	UE_LOG(LogMajicEventAI, Display, TEXT("SmartObjectSlotTransform actor=%s component=%s slot=%d local_offset=%s local_rotation=%s world_location=%s world_rotation=%s"),
		*GetOwner()->GetName(),
		*GetName(),
		SlotIndex,
		*Slot.LocalOffset.ToCompactString(),
		*Slot.LocalRotation.ToCompactString(),
		*WorldLocation.ToCompactString(),
		*WorldRotation.ToCompactString());

	return true;
}

bool UEventSmartObjectComponent::IsSlotAvailable(int32 SlotIndex) const
{
	if (!Slots.IsValidIndex(SlotIndex) || Slots[SlotIndex].ClaimedBy != nullptr)
	{
		return false;
	}

	if (const float* CooldownEndTime = SlotCooldownEndTimes.Find(SlotIndex))
	{
		const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
		if (CurrentTime < *CooldownEndTime)
		{
			return false;
		}
	}

	return true;
}

void UEventSmartObjectComponent::MarkSlotFailed(int32 SlotIndex)
{
	if (!Slots.IsValidIndex(SlotIndex) || FailedSlotCooldownSeconds <= 0.0f)
	{
		return;
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	SlotCooldownEndTimes.Add(SlotIndex, CurrentTime + FailedSlotCooldownSeconds);
}

void UEventSmartObjectComponent::SetApproachLocalOffsets(const TArray<FVector>& LocalOffsets)
{
	ApproachLocalOffsets = LocalOffsets;
	RefreshEditorPreviewComponents();
}

void UEventSmartObjectComponent::SetSlotAnimations(const TArray<TSoftObjectPtr<UAnimationAsset>>& Animations)
{
	SlotAnimations = Animations;
}

UAnimationAsset* UEventSmartObjectComponent::GetSlotAnimation(int32 SlotIndex) const
{
	if (!SlotAnimations.IsValidIndex(SlotIndex))
	{
		return nullptr;
	}

	return SlotAnimations[SlotIndex].LoadSynchronous();
}

void UEventSmartObjectComponent::GetApproachWorldLocations(TArray<FVector>& OutWorldLocations) const
{
	OutWorldLocations.Reset();

	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	const FTransform& OwnerTransform = Owner->GetActorTransform();
	for (const FVector& LocalOffset : ApproachLocalOffsets)
	{
		OutWorldLocations.Add(OwnerTransform.TransformPosition(LocalOffset));
	}
}

void UEventSmartObjectComponent::ReleaseSlot(AEventNPCActor* NPC)
{
	for (FEventInteractionSlot& Slot : Slots)
	{
		if (Slot.ClaimedBy == NPC)
		{
			Slot.ClaimedBy = nullptr;
			return;
		}
	}
}

void UEventSmartObjectComponent::ReleaseAllSlots()
{
	for (FEventInteractionSlot& Slot : Slots)
	{
		Slot.ClaimedBy = nullptr;
	}
}

int32 UEventSmartObjectComponent::GetAvailableSlotCount() const
{
	int32 UnclaimedCount = 0;
	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		if (IsSlotAvailable(SlotIndex))
		{
			++UnclaimedCount;
		}
	}

	const int32 RemainingCapacity = FMath::Max(0, GetMaxClaimedSlotCount() - GetClaimedSlotCount());
	return FMath::Min(UnclaimedCount, RemainingCapacity);
}

int32 UEventSmartObjectComponent::GetClaimedSlotCount() const
{
	int32 Count = 0;
	for (const FEventInteractionSlot& Slot : Slots)
	{
		if (Slot.ClaimedBy != nullptr)
		{
			++Count;
		}
	}
	return Count;
}

int32 UEventSmartObjectComponent::GetMaxClaimedSlotCount() const
{
	if (Slots.Num() <= 0)
	{
		return 0;
	}

	if (!bLimitOccupancyByPercentage)
	{
		return Slots.Num();
	}

	return FMath::Clamp(FMath::CeilToInt(static_cast<float>(Slots.Num()) * MaxOccupancyRatio), 1, Slots.Num());
}

float UEventSmartObjectComponent::GetOccupancyDuration() const
{
	return BaseOccupancyDuration + FMath::FRandRange(-OccupancyVariance, OccupancyVariance);
}

void UEventSmartObjectComponent::BuildLineSlots(int32 Count, float SpacingCm, FVector StartLocalOffset, FRotator FacingRotation)
{
	Slots.Reset();
	for (int32 i = 0; i < Count; ++i)
	{
		FEventInteractionSlot Slot;
		Slot.LocalOffset   = StartLocalOffset + FVector(0.0f, SpacingCm * i, 0.0f);
		Slot.LocalRotation = FacingRotation;
		Slots.Add(Slot);
	}

	RefreshEditorPreviewComponents();
}

void UEventSmartObjectComponent::AutoPopulateSlotsFromMeshComponents(const TArray<FString>& ComponentNameFilters, FRotator SlotFacingRotation)
{
	AActor* Owner = GetOwner();
	if (!Owner || ComponentNameFilters.Num() == 0)
	{
		return;
	}

	Slots.Reset();

	const FTransform& OwnerTransform = Owner->GetActorTransform();
	constexpr float DuplicateSlotDistanceCm = 75.0f;
	int32 MatchedInstanceCount = 0;
	FVector MatchedLocalOffsetSum = FVector::ZeroVector;

	TArray<UInstancedStaticMeshComponent*> MeshComps;
	Owner->GetComponents<UInstancedStaticMeshComponent>(MeshComps);

	for (UInstancedStaticMeshComponent* Comp : MeshComps)
	{
		const FString CompName = Comp->GetName();
		bool bMatches = false;
		for (const FString& Filter : ComponentNameFilters)
		{
			if (CompName.Contains(Filter))
			{
				bMatches = true;
				break;
			}
		}

		if (!bMatches)
		{
			continue;
		}

		for (int32 i = 0; i < Comp->GetInstanceCount(); ++i)
		{
			FTransform InstanceWorldTransform;
			Comp->GetInstanceTransform(i, InstanceWorldTransform, true);

			FEventInteractionSlot Slot;
			Slot.LocalOffset   = OwnerTransform.InverseTransformPosition(InstanceWorldTransform.GetLocation()) + AutoPopulatePositionOffset;
			Slot.LocalRotation = SlotFacingRotation;
			++MatchedInstanceCount;
			MatchedLocalOffsetSum += Slot.LocalOffset;

			bool bDuplicate = false;
			for (const FEventInteractionSlot& ExistingSlot : Slots)
			{
				if (FVector::DistSquared2D(ExistingSlot.LocalOffset, Slot.LocalOffset) <= FMath::Square(DuplicateSlotDistanceCm))
				{
					bDuplicate = true;
					break;
				}
			}

			if (bDuplicate)
			{
				continue;
			}

			Slots.Add(Slot);
		}
	}

	const int32 DesiredSlotCount = FMath::Max(MatchedInstanceCount, bEnsureMinimumTestSlots ? MinimumTestSlotCount : 0);
	if (DesiredSlotCount > Slots.Num())
	{
		const FVector SpreadCenter = MatchedInstanceCount > 0
			? MatchedLocalOffsetSum / static_cast<float>(MatchedInstanceCount)
			: AutoPopulatePositionOffset;
		const int32 OriginalSlotCount = Slots.Num();
		BuildSpreadSlotsAroundPoint(Slots, DesiredSlotCount, SpreadCenter, TestSlotClusterRadiusCm, TestSlotClusterStartAngleDegrees, SlotFacingRotation);
		UE_LOG(LogTemp, Warning, TEXT("[EventSmartObject] %s spread %d collapsed/generated slots into %d interaction spots around %s"),
			*Owner->GetName(),
			OriginalSlotCount,
			Slots.Num(),
			*SpreadCenter.ToCompactString());
	}

	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		const FVector WorldLocation = OwnerTransform.TransformPosition(Slots[SlotIndex].LocalOffset);
		UE_LOG(LogTemp, Display, TEXT("[EventSmartObject] %s generated slot %d at %s"),
			*Owner->GetName(),
			SlotIndex,
			*WorldLocation.ToCompactString());
	}

	RefreshEditorPreviewComponents();
}

void UEventSmartObjectComponent::AutoPopulateSlotsFromMeshComponentsFacingLocalPoint(const TArray<FString>& ComponentNameFilters, FVector FacingLocalPoint, float YawOffsetDegrees)
{
	bFaceSlotsTowardLocalPoint = true;
	SlotFacingLocalPoint = FacingLocalPoint;
	SlotFacingYawOffsetDegrees = YawOffsetDegrees;

	AutoPopulateSlotsFromMeshComponents(ComponentNameFilters, FRotator::ZeroRotator);
	for (FEventInteractionSlot& Slot : Slots)
	{
		Slot.LocalRotation = MakeFacingRotation(Slot.LocalOffset, FacingLocalPoint, YawOffsetDegrees);
	}

	RefreshEditorPreviewComponents();
}

void UEventSmartObjectComponent::SetSlotsFromLocalOffsets(const TArray<FVector>& LocalOffsets, FRotator SlotFacingRotation)
{
	Slots.Reset();

	for (const FVector& LocalOffset : LocalOffsets)
	{
		FEventInteractionSlot Slot;
		Slot.LocalOffset = LocalOffset;
		Slot.LocalRotation = SlotFacingRotation;
		Slots.Add(Slot);
	}

	RefreshEditorPreviewComponents();
}

void UEventSmartObjectComponent::SetSlotsFromLocalOffsetsAndRotations(const TArray<FVector>& LocalOffsets, const TArray<FRotator>& LocalRotations)
{
	Slots.Reset();

	for (int32 Index = 0; Index < LocalOffsets.Num(); ++Index)
	{
		FEventInteractionSlot Slot;
		Slot.LocalOffset = LocalOffsets[Index];
		Slot.LocalRotation = LocalRotations.IsValidIndex(Index) ? LocalRotations[Index] : FRotator::ZeroRotator;
		Slots.Add(Slot);
	}

	RefreshEditorPreviewComponents();
}

void UEventSmartObjectComponent::SetSlotsFromLocalOffsetsFacingLocalPoint(const TArray<FVector>& LocalOffsets, FVector FacingLocalPoint, float YawOffsetDegrees)
{
	bFaceSlotsTowardLocalPoint = true;
	SlotFacingLocalPoint = FacingLocalPoint;
	SlotFacingYawOffsetDegrees = YawOffsetDegrees;

	Slots.Reset();
	for (const FVector& LocalOffset : LocalOffsets)
	{
		FEventInteractionSlot Slot;
		Slot.LocalOffset = LocalOffset;
		Slot.LocalRotation = MakeFacingRotation(LocalOffset, FacingLocalPoint, YawOffsetDegrees);
		Slots.Add(Slot);
	}

	RefreshEditorPreviewComponents();
}

void UEventSmartObjectComponent::DrawDebugSlots()
{
	AActor* Owner = GetOwner();
	if (!Owner || !GetWorld())
	{
		return;
	}

	const FTransform& OwnerTransform = Owner->GetActorTransform();

	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		const FEventInteractionSlot& Slot = Slots[i];
		const FVector WorldLocation = OwnerTransform.TransformPosition(Slot.LocalOffset);

		// Green = available, Red = claimed
		const FColor SlotColor = Slot.ClaimedBy ? FColor::Red : FColor::Green;

		DrawDebugSphere(GetWorld(), WorldLocation, 20.0f, 8, SlotColor, false, 60.0f);

		// Arrow showing facing direction
		const FVector FacingDir = OwnerTransform.TransformRotation(Slot.LocalRotation.Quaternion()).GetForwardVector();
		DrawDebugDirectionalArrow(GetWorld(), WorldLocation, WorldLocation + FacingDir * 50.0f, 10.0f, SlotColor, false, 60.0f);

		// Slot index label offset slightly above
		DrawDebugString(GetWorld(), WorldLocation + FVector(0, 0, 30), FString::Printf(TEXT("%d"), i), nullptr, SlotColor, 60.0f);
	}

	for (int32 i = 0; i < ApproachLocalOffsets.Num(); ++i)
	{
		const FVector WorldLocation = OwnerTransform.TransformPosition(ApproachLocalOffsets[i]);
		DrawDebugSphere(GetWorld(), WorldLocation, 28.0f, 10, FColor::Cyan, false, 60.0f);
		DrawDebugString(GetWorld(), WorldLocation + FVector(0, 0, 35), FString::Printf(TEXT("Entry %d"), i), nullptr, FColor::Cyan, 60.0f);
	}
}

void UEventSmartObjectComponent::PreviewSlots()
{
	AActor* Owner = GetOwner();
	if (!Owner || !GetWorld())
	{
		return;
	}

	const FTransform& OwnerTransform = Owner->GetActorTransform();

	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		const FEventInteractionSlot& Slot = Slots[i];
		const FVector WorldLocation = OwnerTransform.TransformPosition(Slot.LocalOffset);
		DrawDebugSphere(GetWorld(), WorldLocation, 20.0f, 8, FColor::Green, false, 60.0f);
		DrawDebugString(GetWorld(), WorldLocation + FVector(0, 0, 30), FString::Printf(TEXT("%d"), i), nullptr, FColor::White, 60.0f);
	}

	for (int32 i = 0; i < ApproachLocalOffsets.Num(); ++i)
	{
		const FVector WorldLocation = OwnerTransform.TransformPosition(ApproachLocalOffsets[i]);
		DrawDebugSphere(GetWorld(), WorldLocation, 28.0f, 10, FColor::Cyan, false, 60.0f);
		DrawDebugString(GetWorld(), WorldLocation + FVector(0, 0, 35), FString::Printf(TEXT("Entry %d"), i), nullptr, FColor::Cyan, 60.0f);
	}
}

void UEventSmartObjectComponent::RefreshEditorPreviewComponents()
{
#if WITH_EDITOR
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	USceneComponent* AttachParent = Owner->GetRootComponent();
	if (!AttachParent)
	{
		return;
	}

	while (EditorSlotPreviewComponents.Num() < Slots.Num())
	{
		const FName ComponentName = MakeUniqueObjectName(Owner, USphereComponent::StaticClass(), TEXT("EditorSmartObjectSlot"));
		USphereComponent* PreviewComponent = NewObject<USphereComponent>(Owner, ComponentName, RF_Transient | RF_TextExportTransient);
		if (!PreviewComponent)
		{
			break;
		}

		PreviewComponent->SetupAttachment(AttachParent);
		PreviewComponent->SetIsVisualizationComponent(true);
		PreviewComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PreviewComponent->SetGenerateOverlapEvents(false);
		PreviewComponent->SetSphereRadius(20.0f);
		PreviewComponent->ShapeColor = FColor::Green;
		PreviewComponent->SetHiddenInGame(true);
		PreviewComponent->SetVisibility(true);
		PreviewComponent->RegisterComponent();
		Owner->AddInstanceComponent(PreviewComponent);
		EditorSlotPreviewComponents.Add(PreviewComponent);
	}

	while (EditorApproachPreviewComponents.Num() < ApproachLocalOffsets.Num())
	{
		const FName ComponentName = MakeUniqueObjectName(Owner, USphereComponent::StaticClass(), TEXT("EditorSmartObjectEntry"));
		USphereComponent* PreviewComponent = NewObject<USphereComponent>(Owner, ComponentName, RF_Transient | RF_TextExportTransient);
		if (!PreviewComponent)
		{
			break;
		}

		PreviewComponent->SetupAttachment(AttachParent);
		PreviewComponent->SetIsVisualizationComponent(true);
		PreviewComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PreviewComponent->SetGenerateOverlapEvents(false);
		PreviewComponent->SetSphereRadius(28.0f);
		PreviewComponent->ShapeColor = FColor::Cyan;
		PreviewComponent->SetHiddenInGame(true);
		PreviewComponent->SetVisibility(true);
		PreviewComponent->RegisterComponent();
		Owner->AddInstanceComponent(PreviewComponent);
		EditorApproachPreviewComponents.Add(PreviewComponent);
	}

	while (EditorSlotPreviewArrowComponents.Num() < Slots.Num())
	{
		const FName ComponentName = MakeUniqueObjectName(Owner, UArrowComponent::StaticClass(), TEXT("EditorSmartObjectSlotArrow"));
		UArrowComponent* ArrowComponent = NewObject<UArrowComponent>(Owner, ComponentName, RF_Transient | RF_TextExportTransient);
		if (!ArrowComponent)
		{
			break;
		}

		ArrowComponent->SetupAttachment(AttachParent);
		ArrowComponent->SetIsVisualizationComponent(true);
		ArrowComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ArrowComponent->ArrowColor = FColor::Green;
		ArrowComponent->ArrowSize = 1.25f;
		ArrowComponent->ArrowLength = 75.0f;
		ArrowComponent->SetHiddenInGame(true);
		ArrowComponent->SetVisibility(true);
		ArrowComponent->RegisterComponent();
		Owner->AddInstanceComponent(ArrowComponent);
		EditorSlotPreviewArrowComponents.Add(ArrowComponent);
	}

	for (int32 Index = 0; Index < EditorSlotPreviewComponents.Num(); ++Index)
	{
		USphereComponent* PreviewComponent = EditorSlotPreviewComponents[Index];
		if (!PreviewComponent)
		{
			continue;
		}

		const bool bHasSlot = Slots.IsValidIndex(Index);
		PreviewComponent->SetVisibility(bHasSlot);
		PreviewComponent->SetHiddenInGame(true);
		if (bHasSlot)
		{
			PreviewComponent->SetRelativeLocation(Slots[Index].LocalOffset);
			PreviewComponent->ShapeColor = Slots[Index].ClaimedBy ? FColor::Red : FColor::Green;
		}
	}

	for (int32 Index = 0; Index < EditorApproachPreviewComponents.Num(); ++Index)
	{
		USphereComponent* PreviewComponent = EditorApproachPreviewComponents[Index];
		if (!PreviewComponent)
		{
			continue;
		}

		const bool bHasApproach = ApproachLocalOffsets.IsValidIndex(Index);
		PreviewComponent->SetVisibility(bHasApproach);
		PreviewComponent->SetHiddenInGame(true);
		if (bHasApproach)
		{
			PreviewComponent->SetRelativeLocation(ApproachLocalOffsets[Index]);
			PreviewComponent->ShapeColor = FColor::Cyan;
		}
	}

	for (int32 Index = 0; Index < EditorSlotPreviewArrowComponents.Num(); ++Index)
	{
		UArrowComponent* ArrowComponent = EditorSlotPreviewArrowComponents[Index];
		if (!ArrowComponent)
		{
			continue;
		}

		const bool bHasSlot = Slots.IsValidIndex(Index);
		ArrowComponent->SetVisibility(bHasSlot);
		ArrowComponent->SetHiddenInGame(true);
		if (bHasSlot)
		{
			ArrowComponent->SetRelativeLocation(Slots[Index].LocalOffset);
			ArrowComponent->SetRelativeRotation(Slots[Index].LocalRotation);
			ArrowComponent->ArrowColor = Slots[Index].ClaimedBy ? FColor::Red : FColor::Green;
		}
	}
#endif
}

void UEventSmartObjectComponent::OnRegister()
{
	Super::OnRegister();
	RefreshEditorPreviewComponents();
}

void UEventSmartObjectComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bDrawDebugInEditor || !GetWorld() || GetWorld()->IsGameWorld() || Slots.Num() == 0)
	{
		return;
	}

	EditorDebugDrawElapsedTime += DeltaTime;
	if (EditorDebugDrawElapsedTime < EditorDebugDrawInterval)
	{
		return;
	}

	EditorDebugDrawElapsedTime = 0.0f;
	DrawDebugSlots();
	RefreshEditorPreviewComponents();
}

#if WITH_EDITORONLY_DATA
void UEventSmartObjectComponent::OnComponentCreated()
{
	Super::OnComponentCreated();
}
#endif

void UEventSmartObjectComponent::BuildClusterSlots(int32 Count, float RadiusCm, FRotator FacingRotation)
{
	Slots.Reset();
	const float AngleStep = 360.0f / FMath::Max(Count, 1);
	for (int32 i = 0; i < Count; ++i)
	{
		const float Angle = FMath::DegreesToRadians(TestSlotClusterStartAngleDegrees + AngleStep * i);
		FEventInteractionSlot Slot;
		Slot.LocalOffset   = FVector(FMath::Cos(Angle) * RadiusCm, FMath::Sin(Angle) * RadiusCm, 0.0f);
		Slot.LocalRotation = FacingRotation;
		Slots.Add(Slot);
	}

	RefreshEditorPreviewComponents();
}
