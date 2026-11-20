#include "EventEntranceActor.h"

#include "EventNPCActor.h"
#include "EventDirectorActor.h"
#include "EventSmartObjectComponent.h"
#include "MajicEventAIModule.h"
#include "Animation/AnimationAsset.h"
#include "EngineUtils.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

#if WITH_EDITORONLY_DATA
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#endif

AEventEntranceActor::AEventEntranceActor()
{
	PrimaryActorTick.bCanEverTick = false;
	NPCClass = AEventNPCActor::StaticClass();

#if WITH_EDITORONLY_DATA
	ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	ArrowComponent->SetArrowColor(FLinearColor(0.0f, 0.8f, 1.0f));
	ArrowComponent->ArrowSize = 2.0f;
	SetRootComponent(ArrowComponent);

	Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	Billboard->SetupAttachment(ArrowComponent);
#endif
}

void AEventEntranceActor::BeginPlay()
{
	Super::BeginPlay();

	// Auto-find the director if not explicitly assigned
	if (!EventDirector)
	{
		AActor* Found = UGameplayStatics::GetActorOfClass(this, AEventDirectorActor::StaticClass());
		ResolvedDirector = Cast<AEventDirectorActor>(Found);
	}
	else
	{
		ResolvedDirector = EventDirector;
	}

	if (ResolvedDirector)
	{
		ResolvedDirector->RegisterEntrance(this);
		ResolvedDirector->OnPhaseChanged.AddDynamic(this, &AEventEntranceActor::OnEventPhaseChanged);

		// If the game starts mid-phase (e.g. PIE testing), catch up
		if (ResolvedDirector->GetCurrentPhase() == EEventPhase::Arriving)
		{
			StartSpawning();
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[EventAI] %s could not find an EventDirectorActor"), *GetName());
	}

	if (bStartSpawningOnBeginPlay)
	{
		StartSpawning();
	}
}

void AEventEntranceActor::OnEventPhaseChanged(EEventPhase PreviousPhase, EEventPhase NewPhase)
{
	if (NewPhase == EEventPhase::Arriving)
	{
		StartSpawning();
	}
	else if (bSpawning)
	{
		StopSpawning();
	}
}

void AEventEntranceActor::StartSpawning()
{
	if (bSpawning || SpawnedCount >= MaxNPCCount)
	{
		return;
	}
	bSpawning = true;

	const int32 BurstCount = FMath::Clamp(InitialSpawnBurstCount, 0, MaxNPCCount - SpawnedCount);
	const int32 PlacedBurstCount = bPlaceInitialBurstAtSmartObjects
		? FMath::Clamp(InitialPlacedSmartObjectCount, 0, BurstCount)
		: 0;

	int32 PlacedThisBurst = 0;
	for (int32 PlacedIndex = 0; PlacedIndex < PlacedBurstCount && bSpawning; ++PlacedIndex)
	{
		if (!SpawnPlacedNPCAtSmartObject())
		{
			break;
		}
		++PlacedThisBurst;
	}

	const int32 WalkingBurstCount = FMath::Max(0, BurstCount - PlacedThisBurst);
	for (int32 BurstIndex = 0; BurstIndex < WalkingBurstCount && bSpawning; ++BurstIndex)
	{
		SpawnNextNPC();
	}

	ScheduleNextSpawn();
}

void AEventEntranceActor::StopSpawning()
{
	bSpawning = false;
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
}

void AEventEntranceActor::SpawnNextNPC()
{
	if (!bSpawning || SpawnedCount >= MaxNPCCount || !NPCClass)
	{
		if (!NPCClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("[EventAI] %s has no NPCClass assigned"), *GetName());
		}
		StopSpawning();
		return;
	}

	if (bPlaceSpawnsAtSmartObjects && SpawnPlacedNPCAtSmartObject())
	{
		if (SpawnedCount < MaxNPCCount)
		{
			ScheduleNextSpawn();
		}
		else
		{
			StopSpawning();
		}
		return;
	}

	// Find a random point on the NavMesh near this entrance
	UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

	FVector SpawnOrigin = GetActorLocation();
	if (SpawnRadiusCm > 0.0f)
	{
		FVector RandomOffset = FMath::VRand() * FMath::FRandRange(0.0f, SpawnRadiusCm);
		RandomOffset.Z = 0.0f;
		SpawnOrigin += RandomOffset;
	}

	FNavLocation NavLocation;
	bool bFoundNavPoint = NavSystem && NavSystem->ProjectPointToNavigation(SpawnOrigin, NavLocation, FVector(SpawnRadiusCm, SpawnRadiusCm, 200.0f));
	FVector FinalSpawnLocation = bFoundNavPoint ? NavLocation.Location : SpawnOrigin;

	FRotator SpawnRotation = GetActorRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AEventNPCActor* NPC = GetWorld()->SpawnActor<AEventNPCActor>(NPCClass, FinalSpawnLocation, SpawnRotation, SpawnParams);
	if (NPC)
	{
		InitializeSpawnedNPC(NPC);

		++SpawnedCount;
		UE_LOG(LogMajicEventAI, Display, TEXT("Spawn entrance=%s npc=%s count=%d/%d location=%s personality={bar=%.2f social=%.2f punctual=%.2f stage=%.2f}"),
			*GetName(),
			*NPC->GetName(),
			SpawnedCount,
			MaxNPCCount,
			*FinalSpawnLocation.ToCompactString(),
			NPC->GetPersonality().BarAffinity,
			NPC->GetPersonality().Sociability,
			NPC->GetPersonality().Punctuality,
			NPC->GetPersonality().StageProximity);
	}

	if (SpawnedCount < MaxNPCCount)
	{
		ScheduleNextSpawn();
	}
	else
	{
		StopSpawning();
	}
}

bool AEventEntranceActor::SpawnPlacedNPCAtSmartObject()
{
	if (!NPCClass || SpawnedCount >= MaxNPCCount)
	{
		return false;
	}

	UEventSmartObjectComponent* SmartObject = nullptr;
	int32 SlotIndex = INDEX_NONE;
	FTransform SlotTransform;
	if (!FindAvailableInitialSmartObjectSlot(SmartObject, SlotIndex, SlotTransform) || !SmartObject)
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AEventNPCActor* NPC = GetWorld()->SpawnActor<AEventNPCActor>(NPCClass, SlotTransform.GetLocation(), SlotTransform.Rotator(), SpawnParams);
	if (!NPC)
	{
		return false;
	}

	InitializeSpawnedNPC(NPC);

	FTransform ClaimedSlotTransform;
	if (SmartObject->TryClaimSlotByIndex(NPC, SlotIndex, ClaimedSlotTransform))
	{
		NPC->SetActorLocationAndRotation(ClaimedSlotTransform.GetLocation(), ClaimedSlotTransform.Rotator(), false, nullptr, ETeleportType::TeleportPhysics);
		NPC->SetClaimedSlot(SmartObject, SlotIndex);
		NPC->LockMovementAtInteractionSlot();
		if (UAnimationAsset* SlotAnimation = SmartObject->GetSlotAnimation(SlotIndex))
		{
			NPC->SetInteractionAnimationStateWithAnimation(SmartObject->InteractionTag, SlotAnimation);
		}
		else
		{
			NPC->SetInteractionAnimationState(SmartObject->InteractionTag);
		}
	}

	++SpawnedCount;
	UE_LOG(LogMajicEventAI, Display, TEXT("SpawnPlaced entrance=%s npc=%s count=%d/%d smart_object=%s slot=%d location=%s"),
		*GetName(),
		*NPC->GetName(),
		SpawnedCount,
		MaxNPCCount,
		*GetNameSafe(SmartObject->GetOwner()),
		SlotIndex,
		*NPC->GetActorLocation().ToCompactString());

	return true;
}

bool AEventEntranceActor::FindAvailableInitialSmartObjectSlot(UEventSmartObjectComponent*& OutSmartObject, int32& OutSlotIndex, FTransform& OutSlotTransform) const
{
	OutSmartObject = nullptr;
	OutSlotIndex = INDEX_NONE;
	OutSlotTransform = FTransform::Identity;

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	TArray<TPair<UEventSmartObjectComponent*, int32>> Candidates;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		UEventSmartObjectComponent* SmartObject = It->FindComponentByClass<UEventSmartObjectComponent>();
		if (!SmartObject || SmartObject->GetAvailableSlotCount() <= 0)
		{
			continue;
		}

		for (int32 SlotIndex = 0; SlotIndex < SmartObject->Slots.Num(); ++SlotIndex)
		{
			if (SmartObject->IsSlotAvailable(SlotIndex))
			{
				Candidates.Add(TPair<UEventSmartObjectComponent*, int32>(SmartObject, SlotIndex));
			}
		}
	}

	if (Candidates.Num() == 0)
	{
		return false;
	}

	const TPair<UEventSmartObjectComponent*, int32>& Pick = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
	if (!Pick.Key || !Pick.Key->GetSlotWorldTransform(Pick.Value, OutSlotTransform))
	{
		return false;
	}

	OutSmartObject = Pick.Key;
	OutSlotIndex = Pick.Value;
	return true;
}

void AEventEntranceActor::InitializeSpawnedNPC(AEventNPCActor* NPC)
{
	if (!NPC)
	{
		return;
	}

	FNPCPersonality Personality;
	Personality.BarAffinity = FMath::FRandRange(PersonalityRange.BarAffinityMin, PersonalityRange.BarAffinityMax);
	Personality.Sociability = FMath::FRandRange(PersonalityRange.SociabilityMin, PersonalityRange.SociabilityMax);
	Personality.Punctuality = FMath::FRandRange(PersonalityRange.PunctualityMin, PersonalityRange.PunctualityMax);
	Personality.StageProximity = FMath::FRandRange(PersonalityRange.StageProximityMin, PersonalityRange.StageProximityMax);
	NPC->SetPersonality(Personality);

	if (ResolvedDirector)
	{
		NPC->SetEventDirector(ResolvedDirector);
		NPC->OnEventPhaseChanged(ResolvedDirector->GetCurrentPhase());
	}
}

void AEventEntranceActor::ScheduleNextSpawn()
{
	if (!bSpawning || SpawnedCount >= MaxNPCCount)
	{
		StopSpawning();
		return;
	}

	const float Interval = FMath::FRandRange(SpawnIntervalMin, SpawnIntervalMax);
	GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AEventEntranceActor::SpawnNextNPC, Interval, false);
}
