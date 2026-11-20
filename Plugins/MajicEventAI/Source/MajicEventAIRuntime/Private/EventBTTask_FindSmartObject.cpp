#include "EventBTTask_FindSmartObject.h"

#include "EventNPCActor.h"
#include "EventSmartObjectComponent.h"
#include "EventTrafficAreaActor.h"
#include "MajicEventAIModule.h"
#include "AIController.h"
#include "Animation/AnimationAsset.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EngineUtils.h"
#include "GameplayTagsManager.h"
#include "NavigationPath.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"

namespace
{
	struct FSmartObjectCandidate
	{
		UEventSmartObjectComponent* SmartObject = nullptr;
		int32 SlotIndex = INDEX_NONE;
		FTransform SlotTransform;
		FVector MoveTargetLocation = FVector::ZeroVector;
		FVector ApproachLocation = FVector::ZeroVector;
		FVector FinalSlotLocation = FVector::ZeroVector;
		float PathLength = 0.0f;
		float Score = 1.0f;
		float Weight = 1.0f;
		float DestinationCrowdingPenalty = 0.0f;
		bool bMeetsTravelDistance = true;
		bool bUsesApproachMove = false;
		bool bUsesTrafficWaypoint = false;
		FString TrafficAreaName;
		float TrafficDetourRatio = 0.0f;
		FVector TrafficDestinationLocation = FVector::ZeroVector;
	};

	float GetInteractionPersonalityAffinity(const FGameplayTag& InteractionTag, const FNPCPersonality& Personality)
	{
		const FGameplayTag BarOrderTag = FGameplayTag::RequestGameplayTag(FName(TEXT("EventInteract.Bar.Order")), false);
		const FGameplayTag CafeSitTag = FGameplayTag::RequestGameplayTag(FName(TEXT("EventInteract.Cafe.Sit")), false);
		const FGameplayTag LoungeSitTag = FGameplayTag::RequestGameplayTag(FName(TEXT("EventInteract.Lounge.Sit")), false);
		const FGameplayTag StageWatchTag = FGameplayTag::RequestGameplayTag(FName(TEXT("EventInteract.Stage.Watch")), false);
		const FGameplayTag SocialTalkTag = FGameplayTag::RequestGameplayTag(FName(TEXT("EventInteract.Social.Talk")), false);

		if (InteractionTag == BarOrderTag)
		{
			return Personality.BarAffinity;
		}

		if (InteractionTag == CafeSitTag || InteractionTag == LoungeSitTag || InteractionTag == SocialTalkTag)
		{
			return Personality.Sociability;
		}

		if (InteractionTag == StageWatchTag)
		{
			return Personality.StageProximity;
		}

		return 0.5f;
	}

	int32 ChooseWeightedCandidateIndex(const TArray<FSmartObjectCandidate>& Candidates)
	{
		float TotalWeight = 0.0f;
		for (const FSmartObjectCandidate& Candidate : Candidates)
		{
			TotalWeight += FMath::Max(0.0f, Candidate.Weight);
		}

		if (TotalWeight <= 0.0f)
		{
			return Candidates.Num() > 0 ? FMath::RandRange(0, Candidates.Num() - 1) : INDEX_NONE;
		}

		float Pick = FMath::FRandRange(0.0f, TotalWeight);
		for (int32 Index = 0; Index < Candidates.Num(); ++Index)
		{
			Pick -= FMath::Max(0.0f, Candidates[Index].Weight);
			if (Pick <= 0.0f)
			{
				return Index;
			}
		}

		return Candidates.Num() - 1;
	}

	bool GetCompletePathLength(UWorld* World, const FVector& FromLocation, const FVector& ToLocation, float& OutPathLength)
	{
		OutPathLength = 0.0f;
		if (!World)
		{
			return false;
		}

		UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(World, FromLocation, ToLocation);
		if (!Path || !Path->IsValid() || Path->IsPartial())
		{
			return false;
		}

		OutPathLength = Path->GetPathLength();
		return true;
	}

	bool ProjectToNavigation(UWorld* World, const FVector& WorldLocation, FVector& OutProjectedLocation, float ProjectionExtentXY = 200.0f)
	{
		if (UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
		{
			FNavLocation ProjectedLocation;
			const float SafeProjectionExtentXY = FMath::Max(1.0f, ProjectionExtentXY);
			if (!NavSystem->ProjectPointToNavigation(WorldLocation, ProjectedLocation, FVector(SafeProjectionExtentXY, SafeProjectionExtentXY, 300.0f)))
			{
				return false;
			}

			OutProjectedLocation = ProjectedLocation.Location;
			return true;
		}

		OutProjectedLocation = WorldLocation;
		return true;
	}

	bool PlaceNPCAtClaimedSlot(AEventNPCActor* NPC, bool bApplySlotAnimation)
	{
		if (!NPC)
		{
			return false;
		}

		UEventSmartObjectComponent* ClaimedSmartObject = NPC->GetClaimedSmartObject();
		if (!ClaimedSmartObject)
		{
			return false;
		}

		FTransform ClaimedSlotTransform;
		if (!ClaimedSmartObject->GetSlotWorldTransform(NPC->GetClaimedSlotIndex(), ClaimedSlotTransform))
		{
			return false;
		}

		NPC->SetActorLocationAndRotation(
			ClaimedSlotTransform.GetLocation(),
			ClaimedSlotTransform.Rotator(),
			false,
			nullptr,
			ETeleportType::TeleportPhysics);

		if (bApplySlotAnimation)
		{
			if (AAIController* Controller = Cast<AAIController>(NPC->GetController()))
			{
				Controller->StopMovement();
			}
			NPC->LockMovementAtInteractionSlot();
			if (UAnimationAsset* SlotAnimation = ClaimedSmartObject->GetSlotAnimation(NPC->GetClaimedSlotIndex()))
			{
				NPC->SetInteractionAnimationStateWithAnimation(ClaimedSmartObject->InteractionTag, SlotAnimation);
			}
			else
			{
				NPC->SetInteractionAnimationState(ClaimedSmartObject->InteractionTag);
			}
		}
		else if (AAIController* Controller = Cast<AAIController>(NPC->GetController()))
		{
			if (Controller->GetMoveStatus() != EPathFollowingStatus::Idle)
			{
				Controller->StopMovement();
			}
		}

		return true;
	}

	float CalculateDestinationCrowdingPenalty(
		UWorld* World,
		const FVector& Destination,
		float Radius,
		float SmartObjectPenaltyWeight,
		float NPCPenaltyWeight)
	{
		if (!World || Radius <= 0.0f)
		{
			return 0.0f;
		}

		const float RadiusSq = FMath::Square(Radius);
		int32 NearbySmartObjectCount = 0;
		int32 NearbyNPCCount = 0;

		for (TActorIterator<AActor> It(World); It; ++It)
		{
			const AActor* Actor = *It;
			if (!Actor)
			{
				continue;
			}

			if (Actor->FindComponentByClass<UEventSmartObjectComponent>() &&
				FVector::DistSquared2D(Actor->GetActorLocation(), Destination) <= RadiusSq)
			{
				++NearbySmartObjectCount;
			}
		}

		for (TActorIterator<AEventNPCActor> It(World); It; ++It)
		{
			const AEventNPCActor* OtherNPC = *It;
			if (OtherNPC && FVector::DistSquared2D(OtherNPC->GetActorLocation(), Destination) <= RadiusSq)
			{
				++NearbyNPCCount;
			}
		}

		return static_cast<float>(NearbySmartObjectCount) * SmartObjectPenaltyWeight
			+ static_cast<float>(NearbyNPCCount) * NPCPenaltyWeight;
	}
}

UEventBTTask_FindSmartObject::UEventBTTask_FindSmartObject()
{
	NodeName = TEXT("Find Smart Object");
	bNotifyTick = true;
	TargetLocationKey.SelectedKeyName = TEXT("TargetLocation");
}

EBTNodeResult::Type UEventBTTask_FindSmartObject::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	if (!Controller)
	{
		return EBTNodeResult::Failed;
	}

	AEventNPCActor* NPC = Cast<AEventNPCActor>(Controller->GetPawn());
	if (!NPC)
	{
		return EBTNodeResult::Failed;
	}

	// Release any previously claimed slot before searching for a new one
	if (NPC->HasClaimedSlot())
	{
		if (NPC->ShouldHoldClaimedSocialSlots())
		{
			Controller->StopMovement();
			NPC->ClearDebugMoveTarget();
			UE_LOG(LogMajicEventAI, Display, TEXT("SmartObjectSearchSkipped npc=%s reason=holding_social_slot"),
				*NPC->GetName());
			return EBTNodeResult::Succeeded;
		}
		if (!NPC->HasCompletedRequiredWanderAfterSmartObject())
		{
			UE_LOG(LogMajicEventAI, Display, TEXT("SmartObjectSearchDeferred npc=%s reason=claimed_slot_pending_departure"),
				*NPC->GetName());
			return EBTNodeResult::Failed;
		}
		NPC->ReleaseClaimedSlot();
	}

	const FGameplayTag EffectiveSearchTag = SearchTag;
	const bool bFilterBySearchTag = EffectiveSearchTag.IsValid() && !bSearchAllEventInteractionsForPrototype;
	const FName EffectiveTargetLocationKey = TargetLocationKey.SelectedKeyName.IsNone()
		? FName(TEXT("TargetLocation"))
		: TargetLocationKey.SelectedKeyName;

	const FVector NPCLocation = NPC->GetActorLocation();
	if (bUsePreferredTrafficAreas)
	{
		UE_LOG(LogMajicEventAI, Display, TEXT("TrafficWaypointLayerEnabled npc=%s source=FindSmartObject min_distance=%.0f detour=%.2f"),
			*NPC->GetName(),
			MinTrafficRouteDistance,
			MaxTrafficRouteDetourRatio);
	}
	TArray<FSmartObjectCandidate> PrimaryCandidates;
	TArray<FSmartObjectCandidate> NearbyFallbackCandidates;
	int32 RejectedProjectionCount = 0;
	int32 RejectedPathCount = 0;
	int32 RejectedNoThroughCount = 0;
	int32 RejectedPathLengthCount = 0;
	int32 RejectedFailedTargetCount = 0;
	TMap<FString, int32> RejectedNoThroughAreas;

	// Search all actors in the world for matching smart objects with available slots
	for (TActorIterator<AActor> It(NPC->GetWorld()); It; ++It)
	{
		UEventSmartObjectComponent* SmartObj = It->FindComponentByClass<UEventSmartObjectComponent>();
		if (!SmartObj)
		{
			continue;
		}

		if (!NPC->CanClaimSmartObject(SmartObj))
		{
			continue;
		}

		if (bFilterBySearchTag && SmartObj->InteractionTag != EffectiveSearchTag)
		{
			continue;
		}

		if (!SmartObj->HasAvailableSlot())
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(NPCLocation, It->GetActorLocation());
		const bool bMeetsTravelDistance = NPC->IsFarEnoughFromLastSmartObject(SmartObj);
		for (int32 SlotIndex = 0; SlotIndex < SmartObj->Slots.Num(); ++SlotIndex)
		{
			if (!SmartObj->IsSlotAvailable(SlotIndex))
			{
				continue;
			}

			FTransform SlotTransform;
			if (!SmartObj->GetSlotWorldTransform(SlotIndex, SlotTransform))
			{
				continue;
			}

			const FVector SlotLocation = SlotTransform.GetLocation();
			FVector MoveTargetLocation = SlotLocation;
			if (!ProjectToNavigation(NPC->GetWorld(), MoveTargetLocation, MoveTargetLocation))
			{
				++RejectedProjectionCount;
				continue;
			}

			const float ProjectionDistance = FVector::Dist2D(SlotLocation, MoveTargetLocation);
			if (MaxProjectedSlotDistance > 0.0f && ProjectionDistance > MaxProjectedSlotDistance)
			{
				++RejectedProjectionCount;
				continue;
			}

			if (NPC->IsRecentFailedMoveTarget(MoveTargetLocation))
			{
				++RejectedFailedTargetCount;
				continue;
			}

			float PathLength = 0.0f;
			FVector InitialMoveTargetLocation = MoveTargetLocation;
			FVector ApproachMoveTargetLocation = FVector::ZeroVector;
			bool bUsesApproachMove = false;
			bool bUsesTrafficWaypoint = false;
			FString TrafficAreaName;
			float TrafficDetourRatio = 0.0f;
			FVector TrafficDestinationLocation = FVector::ZeroVector;
			TArray<FVector> ApproachWorldLocations;
			SmartObj->GetApproachWorldLocations(ApproachWorldLocations);
			if (ApproachWorldLocations.Num() == 0 && GeneratedSlotApproachDistance > 0.0f)
			{
				FVector SlotEntryDirection = -SlotTransform.GetRotation().GetForwardVector();
				SlotEntryDirection.Z = 0.0f;
				if (!SlotEntryDirection.IsNearlyZero())
				{
					SlotEntryDirection = SlotEntryDirection.GetSafeNormal();
					const float ApproachAngles[] = { 0.0f, -35.0f, 35.0f };
					for (const float ApproachAngle : ApproachAngles)
					{
						ApproachWorldLocations.Add(SlotLocation + SlotEntryDirection.RotateAngleAxis(ApproachAngle, FVector::UpVector) * GeneratedSlotApproachDistance);
					}
				}
			}
			if (ApproachWorldLocations.Num() > 0)
			{
				float BestApproachPathLength = FLT_MAX;
				FVector BestApproachLocation = FVector::ZeroVector;

				for (const FVector& ApproachWorldLocation : ApproachWorldLocations)
				{
					FVector ProjectedApproachLocation = ApproachWorldLocation;
					if (!ProjectToNavigation(NPC->GetWorld(), ProjectedApproachLocation, ProjectedApproachLocation, GeneratedSlotApproachProjectionExtent))
					{
						++RejectedProjectionCount;
						continue;
					}

					if (NPC->IsRecentFailedMoveTarget(ProjectedApproachLocation))
					{
						++RejectedFailedTargetCount;
						continue;
					}

					float PathToApproachLength = FVector::Dist2D(NPCLocation, ProjectedApproachLocation);
					float PathFromApproachToSlotLength = FVector::Dist2D(ProjectedApproachLocation, MoveTargetLocation);
					if (bRequireCompletePathToSlot)
					{
						if (!GetCompletePathLength(NPC->GetWorld(), NPCLocation, ProjectedApproachLocation, PathToApproachLength))
						{
							++RejectedPathCount;
							continue;
						}

						if (!GetCompletePathLength(NPC->GetWorld(), ProjectedApproachLocation, MoveTargetLocation, PathFromApproachToSlotLength))
						{
							++RejectedPathCount;
							continue;
						}
					}
					FString FirstLegNoThroughAreas;
					FString SecondLegNoThroughAreas;
					const bool bFirstLegBlockedByNoThrough = AEventTrafficAreaActor::IsRouteBlockedByNoThroughZone(NPC->GetWorld(), NPCLocation, ProjectedApproachLocation, &FirstLegNoThroughAreas);
					const bool bSecondLegBlockedByNoThrough = AEventTrafficAreaActor::IsRouteBlockedByNoThroughZone(NPC->GetWorld(), ProjectedApproachLocation, MoveTargetLocation, &SecondLegNoThroughAreas);
					if (bFirstLegBlockedByNoThrough || bSecondLegBlockedByNoThrough)
					{
						++RejectedNoThroughCount;
						if (!FirstLegNoThroughAreas.IsEmpty())
						{
							RejectedNoThroughAreas.FindOrAdd(FirstLegNoThroughAreas)++;
						}
						if (!SecondLegNoThroughAreas.IsEmpty())
						{
							RejectedNoThroughAreas.FindOrAdd(SecondLegNoThroughAreas)++;
						}
						continue;
					}

					const float TotalApproachPathLength = PathToApproachLength + PathFromApproachToSlotLength;
					if (TotalApproachPathLength < BestApproachPathLength)
					{
						BestApproachPathLength = TotalApproachPathLength;
						BestApproachLocation = ProjectedApproachLocation;
					}
				}

				if (BestApproachPathLength == FLT_MAX)
				{
					continue;
				}

				PathLength = BestApproachPathLength;
				InitialMoveTargetLocation = BestApproachLocation;
				ApproachMoveTargetLocation = BestApproachLocation;
				bUsesApproachMove = true;
			}
			else if (bRequireCompletePathToSlot && !GetCompletePathLength(NPC->GetWorld(), NPCLocation, MoveTargetLocation, PathLength))
			{
				++RejectedPathCount;
				continue;
			}
			else
			{
				FString NoThroughAreas;
				if (AEventTrafficAreaActor::IsRouteBlockedByNoThroughZone(NPC->GetWorld(), NPCLocation, MoveTargetLocation, &NoThroughAreas))
				{
					++RejectedNoThroughCount;
					if (!NoThroughAreas.IsEmpty())
					{
						RejectedNoThroughAreas.FindOrAdd(NoThroughAreas)++;
					}
					continue;
				}
			}
			if (!bRequireCompletePathToSlot)
			{
				PathLength = FVector::Dist2D(NPCLocation, MoveTargetLocation);
			}

			if (bUsePreferredTrafficAreas)
			{
				FVector TrafficWaypoint = FVector::ZeroVector;
				FEventTrafficRouteResult TrafficRouteResult;
				const FVector TrafficDestination = bUsesApproachMove ? ApproachMoveTargetLocation : MoveTargetLocation;
				if (AEventTrafficAreaActor::FindPreferredTrafficWaypoint(
					NPC->GetWorld(),
					NPCLocation,
					TrafficDestination,
					MinTrafficRouteDistance,
					MaxTrafficRouteDetourRatio,
					TrafficWaypoint,
					&TrafficRouteResult))
				{
					if (TrafficRouteResult.RejectedNoThroughCount > 0)
					{
						RejectedNoThroughCount += TrafficRouteResult.RejectedNoThroughCount;
					}
					float PathToTrafficLength = FVector::Dist2D(NPCLocation, TrafficWaypoint);
					float PathFromTrafficLength = FVector::Dist2D(TrafficWaypoint, TrafficDestination);
					if (!bRequireCompletePathToSlot ||
						(GetCompletePathLength(NPC->GetWorld(), NPCLocation, TrafficWaypoint, PathToTrafficLength) &&
							GetCompletePathLength(NPC->GetWorld(), TrafficWaypoint, TrafficDestination, PathFromTrafficLength)))
					{
						PathLength = PathToTrafficLength + PathFromTrafficLength;
						if (bUsesApproachMove)
						{
							PathLength += FVector::Dist2D(ApproachMoveTargetLocation, MoveTargetLocation);
						}
						InitialMoveTargetLocation = TrafficWaypoint;
						bUsesTrafficWaypoint = true;
						TrafficAreaName = TrafficRouteResult.AreaName;
						TrafficDetourRatio = TrafficRouteResult.DetourRatio;
						TrafficDestinationLocation = TrafficDestination;
					}
				}
			}

			const float StraightLineDistance = FVector::Dist2D(NPCLocation, InitialMoveTargetLocation);
			if (MaxPathDistanceToSlot > 0.0f && PathLength > MaxPathDistanceToSlot)
			{
				++RejectedPathLengthCount;
				continue;
			}

			if (MaxPathToStraightLineRatio > 1.0f && StraightLineDistance > 100.0f && PathLength > StraightLineDistance * MaxPathToStraightLineRatio)
			{
				++RejectedPathLengthCount;
				continue;
			}

			const float DistanceAlpha = FMath::Clamp(FMath::Sqrt(DistSq) / FMath::Max(FullDistanceScoreAt, 1.0f), 0.0f, 1.0f);
			const float PersonalityAffinity = GetInteractionPersonalityAffinity(SmartObj->InteractionTag, NPC->GetPersonality());
			const float AvailableSlotScore = static_cast<float>(SmartObj->GetAvailableSlotCount()) * AvailableSlotScoreWeight;
			const float ClaimedSlotCount = static_cast<float>(SmartObj->GetClaimedSlotCount());
			const float DestinationCrowdingPenalty = CalculateDestinationCrowdingPenalty(
				NPC->GetWorld(),
				InitialMoveTargetLocation,
				DestinationCrowdingRadius,
				NearbySmartObjectPenaltyWeight,
				NearbyNPCPenaltyWeight);

			FSmartObjectCandidate Candidate;
			Candidate.SmartObject = SmartObj;
			Candidate.SlotIndex = SlotIndex;
			Candidate.SlotTransform = SlotTransform;
			Candidate.MoveTargetLocation = InitialMoveTargetLocation;
			Candidate.ApproachLocation = ApproachMoveTargetLocation;
			Candidate.FinalSlotLocation = MoveTargetLocation;
			Candidate.PathLength = PathLength;
			Candidate.bMeetsTravelDistance = bMeetsTravelDistance;
			Candidate.bUsesApproachMove = bUsesApproachMove;
			Candidate.bUsesTrafficWaypoint = bUsesTrafficWaypoint;
			Candidate.TrafficAreaName = TrafficAreaName;
			Candidate.TrafficDetourRatio = TrafficDetourRatio;
			Candidate.TrafficDestinationLocation = TrafficDestinationLocation;
			Candidate.DestinationCrowdingPenalty = DestinationCrowdingPenalty;
			Candidate.Score = 1.0f
				+ DistanceAlpha * DistanceScoreWeight
				+ AvailableSlotScore
				+ PersonalityAffinity * PersonalityScoreWeight
				- ClaimedSlotCount * OccupancyPenaltyWeight
				- DestinationCrowdingPenalty;
			Candidate.Weight = FMath::Max(0.05f, Candidate.Score);

			if (bMeetsTravelDistance)
			{
				PrimaryCandidates.Add(Candidate);
			}
			else
			{
				Candidate.Weight *= NearbyFallbackWeightScale;
				NearbyFallbackCandidates.Add(Candidate);
			}
		}
	}

	const TArray<FSmartObjectCandidate>* CandidatePool = &PrimaryCandidates;
	if (CandidatePool->Num() == 0 && NearbyFallbackCandidates.Num() > 0)
	{
		CandidatePool = &NearbyFallbackCandidates;
		const AActor* FallbackOwner = NearbyFallbackCandidates.Num() == 1 && NearbyFallbackCandidates[0].SmartObject
			? NearbyFallbackCandidates[0].SmartObject->GetOwner()
			: nullptr;
		const FString FallbackCandidateName = FallbackOwner
			? FallbackOwner->GetName()
			: FString(TEXT("candidate_pool"));
		UE_LOG(LogMajicEventAI, Display, TEXT("SmartObjectFallback npc=%s reason=no_distant candidate=%s"),
			*NPC->GetName(),
			*FallbackCandidateName);
	}

	if (CandidatePool->Num() == 0)
	{
		UE_LOG(LogMajicEventAI, Display, TEXT("SmartObjectSearchFailed npc=%s tag=%s action=%s"),
			*NPC->GetName(),
			bFilterBySearchTag ? *EffectiveSearchTag.ToString() : TEXT("any event interaction"),
			NPC->CanWander() ? TEXT("wander") : TEXT("stationary"));
		if (RejectedProjectionCount > 0 || RejectedPathCount > 0 || RejectedNoThroughCount > 0 || RejectedPathLengthCount > 0 || RejectedFailedTargetCount > 0)
		{
			UE_LOG(LogMajicEventAI, Display, TEXT("SmartObjectCandidatesRejected npc=%s projection=%d path=%d no_through=%d path_length=%d failed_target=%d"),
				*NPC->GetName(),
				RejectedProjectionCount,
				RejectedPathCount,
				RejectedNoThroughCount,
				RejectedPathLengthCount,
				RejectedFailedTargetCount);
		}
		for (const TPair<FString, int32>& RejectedArea : RejectedNoThroughAreas)
		{
			UE_LOG(LogMajicEventAI, Display, TEXT("TrafficRouteRejected npc=%s reason=smart_no_through area=%s count=%d"),
				*NPC->GetName(),
				*RejectedArea.Key,
				RejectedArea.Value);
		}
		return EBTNodeResult::Failed;
	}

	const int32 ChosenCandidateIndex = ChooseWeightedCandidateIndex(*CandidatePool);
	if (ChosenCandidateIndex == INDEX_NONE)
	{
		return EBTNodeResult::Failed;
	}

	const FSmartObjectCandidate& ChosenCandidate = (*CandidatePool)[ChosenCandidateIndex];
	if (RejectedNoThroughCount > 0)
	{
		UE_LOG(LogMajicEventAI, Display, TEXT("SmartObjectCandidatesRejected npc=%s no_through=%d selected_actor=%s selected_slot=%d"),
			*NPC->GetName(),
			RejectedNoThroughCount,
			ChosenCandidate.SmartObject && ChosenCandidate.SmartObject->GetOwner() ? *ChosenCandidate.SmartObject->GetOwner()->GetName() : TEXT("unknown"),
			ChosenCandidate.SlotIndex);
		for (const TPair<FString, int32>& RejectedArea : RejectedNoThroughAreas)
		{
			UE_LOG(LogMajicEventAI, Display, TEXT("TrafficRouteRejected npc=%s reason=smart_no_through area=%s count=%d"),
				*NPC->GetName(),
				*RejectedArea.Key,
				RejectedArea.Value);
		}
	}

	UEventSmartObjectComponent* BestComp = ChosenCandidate.SmartObject;
	int32 BestSlotIndex = INDEX_NONE;
	FTransform BestSlotTransform;
	if (!BestComp || !BestComp->TryClaimSlotByIndex(NPC, ChosenCandidate.SlotIndex, BestSlotTransform))
	{
		UE_LOG(LogMajicEventAI, Display, TEXT("SmartObjectSearchFailed npc=%s reason=selected_unavailable action=%s"),
			*NPC->GetName(),
			NPC->CanWander() ? TEXT("wander") : TEXT("stationary"));
		return EBTNodeResult::Failed;
	}

	BestSlotIndex = ChosenCandidate.SlotIndex;
	NPC->SetClaimedSlot(BestComp, BestSlotIndex);

	const FVector SlotLocation = BestSlotTransform.GetLocation();
	const FVector MoveTargetLocation = ChosenCandidate.MoveTargetLocation;
	const FVector FinalSlotLocation = ChosenCandidate.FinalSlotLocation;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (BB)
	{
		BB->SetValueAsVector(EffectiveTargetLocationKey, MoveTargetLocation);
	}

	if (ChosenCandidate.bUsesTrafficWaypoint)
	{
		UE_LOG(LogMajicEventAI, Display, TEXT("TrafficRouteSelected npc=%s reason=smart_%s area=%s start=%s waypoint=%s destination=%s detour=%.2f path_length=%.0f"),
			*NPC->GetName(),
			ChosenCandidate.bUsesApproachMove ? TEXT("approach") : TEXT("slot"),
			ChosenCandidate.TrafficAreaName.IsEmpty() ? TEXT("unknown") : *ChosenCandidate.TrafficAreaName,
			*NPCLocation.ToCompactString(),
			*MoveTargetLocation.ToCompactString(),
			*ChosenCandidate.TrafficDestinationLocation.ToCompactString(),
			ChosenCandidate.TrafficDetourRatio,
			ChosenCandidate.PathLength);
	}

	UE_LOG(LogMajicEventAI, Display, TEXT("SmartObjectClaim npc=%s actor=%s slot=%d slot_location=%s move_target=%s approach_target=%s final_target=%s path_length=%.0f score=%.2f weight=%.2f crowd_penalty=%.2f fallback=%s approach=%s traffic=%s traffic_area=%s traffic_detour=%.2f"),
		*NPC->GetName(),
		*BestComp->GetOwner()->GetName(),
		BestSlotIndex,
		*SlotLocation.ToCompactString(),
		*MoveTargetLocation.ToCompactString(),
		*ChosenCandidate.ApproachLocation.ToCompactString(),
		*FinalSlotLocation.ToCompactString(),
		ChosenCandidate.PathLength,
		ChosenCandidate.Score,
		ChosenCandidate.Weight,
		ChosenCandidate.DestinationCrowdingPenalty,
		ChosenCandidate.bMeetsTravelDistance ? TEXT("false") : TEXT("true"),
		ChosenCandidate.bUsesApproachMove ? TEXT("true") : TEXT("false"),
		ChosenCandidate.bUsesTrafficWaypoint ? TEXT("true") : TEXT("false"),
		ChosenCandidate.TrafficAreaName.IsEmpty() ? TEXT("none") : *ChosenCandidate.TrafficAreaName,
		ChosenCandidate.TrafficDetourRatio);

	FEventFindSmartObjectTaskMemory* Memory = reinterpret_cast<FEventFindSmartObjectTaskMemory*>(NodeMemory);
	Memory->TargetLocation = MoveTargetLocation;
	Memory->ApproachLocation = ChosenCandidate.ApproachLocation;
	Memory->FinalSlotLocation = FinalSlotLocation;
	Memory->LastObservedLocation = NPC->GetActorLocation();
	Memory->TargetRotation = BestSlotTransform.GetRotation().Rotator();
	Memory->ElapsedTime = 0.0f;
	Memory->StuckTime = 0.0f;
	Memory->BlockedNearTargetTime = 0.0f;
	Memory->OccupancyElapsedTime = 0.0f;
	Memory->OccupancyDuration = BestComp->GetOccupancyDuration();
	Memory->PathIdleRepathAttempts = 0;
	Memory->bMoveRequested = false;
	Memory->bArrivedAtSlot = false;
	Memory->bUsingTrafficWaypoint = ChosenCandidate.bUsesTrafficWaypoint;
	Memory->bUsingApproachMove = ChosenCandidate.bUsesApproachMove;
	Memory->bLoggedDirectMovementFallback = false;
	Memory->bLoggedHeldSocialSlot = false;
	NPC->SetDebugMoveTarget(Memory->TargetLocation, Memory->bUsingTrafficWaypoint ? TEXT("traffic_waypoint") : (Memory->bUsingApproachMove ? TEXT("smart_approach") : TEXT("smart_slot")));

	const EPathFollowingRequestResult::Type MoveResult = Controller->MoveToLocation(
		Memory->TargetLocation,
		AcceptanceRadius,
		true,
		true,
		false,
		true);

	if (MoveResult == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		if (Memory->bUsingTrafficWaypoint || Memory->bUsingApproachMove)
		{
			const bool bReachedTrafficWaypoint = Memory->bUsingTrafficWaypoint;
			Memory->TargetLocation = bReachedTrafficWaypoint && Memory->bUsingApproachMove
				? Memory->ApproachLocation
				: Memory->FinalSlotLocation;
			if (bReachedTrafficWaypoint)
			{
				Memory->bUsingTrafficWaypoint = false;
			}
			else
			{
				Memory->bUsingApproachMove = false;
			}
			Memory->ElapsedTime = 0.0f;
			Memory->StuckTime = 0.0f;
			Memory->BlockedNearTargetTime = 0.0f;
			Memory->PathIdleRepathAttempts = 0;
			Memory->LastObservedLocation = NPC->GetActorLocation();
			Memory->bLoggedDirectMovementFallback = false;
			NPC->SetDebugMoveTarget(Memory->TargetLocation, Memory->bUsingApproachMove ? TEXT("smart_approach") : TEXT("smart_slot"));
			if (BB)
			{
				BB->SetValueAsVector(EffectiveTargetLocationKey, Memory->TargetLocation);
			}
			Controller->MoveToLocation(Memory->TargetLocation, AcceptanceRadius, true, true, false, true);
			UE_LOG(LogMajicEventAI, Display, TEXT("%s npc=%s status=already_at_goal next_target=%s"),
				bReachedTrafficWaypoint ? TEXT("TrafficWaypointArrive") : TEXT("SmartObjectApproachArrive"),
				*NPC->GetName(),
				*Memory->TargetLocation.ToCompactString());
			return EBTNodeResult::InProgress;
		}

		PlaceNPCAtClaimedSlot(NPC, false);
		if (!bWaitAtClaimedSlot)
		{
			NPC->ClearDebugMoveTarget();
			UE_LOG(LogMajicEventAI, Display, TEXT("SmartObjectArrive npc=%s status=already_at_goal wait=false"), *NPC->GetName());
			return EBTNodeResult::Succeeded;
		}

		Memory->bArrivedAtSlot = true;
		NPC->ClearDebugMoveTarget();
		PlaceNPCAtClaimedSlot(NPC, true);
		UE_LOG(LogMajicEventAI, Display, TEXT("SmartObjectArrive npc=%s status=already_at_goal wait=true duration=%.1f"),
			*NPC->GetName(),
			Memory->OccupancyDuration);
		return EBTNodeResult::InProgress;
	}

	if (MoveResult == EPathFollowingRequestResult::Failed)
	{
		if (BestComp)
		{
			BestComp->MarkSlotFailed(BestSlotIndex);
		}
		NPC->RememberFailedMoveTarget(Memory->TargetLocation);
		NPC->ClearDebugMoveTarget();
		NPC->ReleaseClaimedSlot();
		UE_LOG(LogMajicEventAI, Warning, TEXT("SmartObjectMoveFailed npc=%s reason=move_rejected current=%s target=%s"),
			*NPC->GetName(),
			*NPC->GetActorLocation().ToCompactString(),
			*Memory->TargetLocation.ToCompactString());
		return EBTNodeResult::Failed;
	}

	Memory->bMoveRequested = true;
	UE_LOG(LogMajicEventAI, Display, TEXT("SmartObjectMoveStart npc=%s target=%s approach_target=%s final_target=%s approach=%s traffic=%s"),
		*NPC->GetName(),
		*Memory->TargetLocation.ToCompactString(),
		*Memory->ApproachLocation.ToCompactString(),
		*Memory->FinalSlotLocation.ToCompactString(),
		Memory->bUsingApproachMove ? TEXT("true") : TEXT("false"),
		Memory->bUsingTrafficWaypoint ? TEXT("true") : TEXT("false"));
	return EBTNodeResult::InProgress;
}

void UEventBTTask_FindSmartObject::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FEventFindSmartObjectTaskMemory* Memory = reinterpret_cast<FEventFindSmartObjectTaskMemory*>(NodeMemory);
	AAIController* Controller = OwnerComp.GetAIOwner();
	AEventNPCActor* NPC = Controller ? Cast<AEventNPCActor>(Controller->GetPawn()) : nullptr;
	if (!Controller || !NPC)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	if (Memory->bArrivedAtSlot)
	{
		Memory->OccupancyElapsedTime += DeltaSeconds;
		PlaceNPCAtClaimedSlot(NPC, false);

		if (Memory->OccupancyElapsedTime >= Memory->OccupancyDuration)
		{
			if (NPC->ShouldHoldClaimedSocialSlots())
			{
				if (!Memory->bLoggedHeldSocialSlot)
				{
					UE_LOG(LogMajicEventAI, Display, TEXT("SmartObjectHold npc=%s reason=occupancy_complete elapsed=%.1f"),
						*NPC->GetName(),
						Memory->OccupancyElapsedTime);
					Memory->bLoggedHeldSocialSlot = true;
				}
				return;
			}

			UE_LOG(LogMajicEventAI, Display, TEXT("SmartObjectReleaseDeferred npc=%s reason=occupancy_complete elapsed=%.1f"),
				*NPC->GetName(),
				Memory->OccupancyElapsedTime);
			NPC->ClearDebugMoveTarget();
			NPC->MarkClaimedSlotReadyToLeave();
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		}
		return;
	}

	Memory->ElapsedTime += DeltaSeconds;
	if (FVector::DistSquared2D(NPC->GetActorLocation(), Memory->LastObservedLocation) < FMath::Square(1.0f))
	{
		Memory->StuckTime += DeltaSeconds;
	}
	else
	{
		Memory->StuckTime = 0.0f;
		Memory->BlockedNearTargetTime = 0.0f;
		Memory->LastObservedLocation = NPC->GetActorLocation();
	}

	const float DistSq2D = FVector::DistSquared2D(NPC->GetActorLocation(), Memory->TargetLocation);
	if (Memory->StuckTime > 0.25f && DistSq2D <= FMath::Square(NearTargetBlockedDistance))
	{
		Memory->BlockedNearTargetTime += DeltaSeconds;
		if (MaxBlockedNearTargetSeconds > 0.0f && Memory->BlockedNearTargetTime >= MaxBlockedNearTargetSeconds)
		{
			Controller->StopMovement();
			if (UEventSmartObjectComponent* ClaimedSmartObject = NPC->GetClaimedSmartObject())
			{
				ClaimedSmartObject->MarkSlotFailed(NPC->GetClaimedSlotIndex());
			}
			NPC->RememberFailedMoveTarget(Memory->TargetLocation);
			NPC->ClearDebugMoveTarget();
			NPC->ReleaseClaimedSlot();
			UE_LOG(LogMajicEventAI, Warning, TEXT("SmartObjectRelease npc=%s reason=blocked_near_target current=%s target=%s blocked_time=%.1f"),
				*NPC->GetName(),
				*NPC->GetActorLocation().ToCompactString(),
				*Memory->TargetLocation.ToCompactString(),
				Memory->BlockedNearTargetTime);
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
			return;
		}
	}
	else if (Memory->BlockedNearTargetTime > 0.0f)
	{
		Memory->BlockedNearTargetTime = 0.0f;
	}

	if (DistSq2D <= FMath::Square(AcceptanceRadius))
	{
		Controller->StopMovement();

		if (Memory->bUsingTrafficWaypoint || Memory->bUsingApproachMove)
		{
			const bool bReachedTrafficWaypoint = Memory->bUsingTrafficWaypoint;
			Memory->TargetLocation = bReachedTrafficWaypoint && Memory->bUsingApproachMove
				? Memory->ApproachLocation
				: Memory->FinalSlotLocation;
			if (bReachedTrafficWaypoint)
			{
				Memory->bUsingTrafficWaypoint = false;
			}
			else
			{
				Memory->bUsingApproachMove = false;
			}
			Memory->ElapsedTime = 0.0f;
			Memory->StuckTime = 0.0f;
			Memory->BlockedNearTargetTime = 0.0f;
			Memory->LastObservedLocation = NPC->GetActorLocation();
			Memory->bLoggedDirectMovementFallback = false;
			NPC->SetDebugMoveTarget(Memory->TargetLocation, Memory->bUsingApproachMove ? TEXT("smart_approach") : TEXT("smart_slot"));

			const FName EffectiveTargetLocationKey = TargetLocationKey.SelectedKeyName.IsNone()
				? FName(TEXT("TargetLocation"))
				: TargetLocationKey.SelectedKeyName;
			if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
			{
				BB->SetValueAsVector(EffectiveTargetLocationKey, Memory->TargetLocation);
			}

			const EPathFollowingRequestResult::Type MoveResult = Controller->MoveToLocation(
				Memory->TargetLocation,
				AcceptanceRadius,
				true,
				true,
				false,
				true);

			UE_LOG(LogMajicEventAI, Display, TEXT("%s npc=%s status=reached next_target=%s move_result=%d"),
				bReachedTrafficWaypoint ? TEXT("TrafficWaypointArrive") : TEXT("SmartObjectApproachArrive"),
				*NPC->GetName(),
				*Memory->TargetLocation.ToCompactString(),
				static_cast<int32>(MoveResult));

			if (MoveResult == EPathFollowingRequestResult::Failed)
			{
				if (UEventSmartObjectComponent* ClaimedSmartObject = NPC->GetClaimedSmartObject())
				{
					ClaimedSmartObject->MarkSlotFailed(NPC->GetClaimedSlotIndex());
				}
				NPC->RememberFailedMoveTarget(Memory->TargetLocation);
				NPC->ClearDebugMoveTarget();
				NPC->ReleaseClaimedSlot();
				FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
			}
			return;
		}

		PlaceNPCAtClaimedSlot(NPC, false);

		if (!bWaitAtClaimedSlot)
		{
			NPC->ClearDebugMoveTarget();
			UE_LOG(LogMajicEventAI, Display, TEXT("SmartObjectArrive npc=%s status=reached wait=false"), *NPC->GetName());
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			return;
		}

		Memory->bArrivedAtSlot = true;
		Memory->OccupancyElapsedTime = 0.0f;
		NPC->ClearDebugMoveTarget();
		PlaceNPCAtClaimedSlot(NPC, true);
		UE_LOG(LogMajicEventAI, Display, TEXT("SmartObjectArrive npc=%s status=reached wait=true duration=%.1f"),
			*NPC->GetName(),
			Memory->OccupancyDuration);
		return;
	}

	const bool bPathFollowingIdle = Controller->GetMoveStatus() == EPathFollowingStatus::Idle;
	if (!bUseDirectMovementFallback && bPathFollowingIdle && Memory->ElapsedTime > PathIdleGraceSeconds)
	{
		if (DistSq2D <= FMath::Square(NearTargetBlockedDistance))
		{
			Controller->StopMovement();

			if (Memory->bUsingTrafficWaypoint || Memory->bUsingApproachMove)
			{
				const bool bReachedTrafficWaypoint = Memory->bUsingTrafficWaypoint;
				Memory->TargetLocation = bReachedTrafficWaypoint && Memory->bUsingApproachMove
					? Memory->ApproachLocation
					: Memory->FinalSlotLocation;
				if (bReachedTrafficWaypoint)
				{
					Memory->bUsingTrafficWaypoint = false;
				}
				else
				{
					Memory->bUsingApproachMove = false;
				}
				Memory->ElapsedTime = 0.0f;
				Memory->StuckTime = 0.0f;
				Memory->BlockedNearTargetTime = 0.0f;
				Memory->LastObservedLocation = NPC->GetActorLocation();
				Memory->bLoggedDirectMovementFallback = false;
				NPC->SetDebugMoveTarget(Memory->TargetLocation, Memory->bUsingApproachMove ? TEXT("smart_approach") : TEXT("smart_slot"));

				const FName EffectiveTargetLocationKey = TargetLocationKey.SelectedKeyName.IsNone()
					? FName(TEXT("TargetLocation"))
					: TargetLocationKey.SelectedKeyName;
				if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
				{
					BB->SetValueAsVector(EffectiveTargetLocationKey, Memory->TargetLocation);
				}

				const EPathFollowingRequestResult::Type MoveResult = Controller->MoveToLocation(
					Memory->TargetLocation,
					AcceptanceRadius,
					true,
					true,
					false,
					true);

				UE_LOG(LogMajicEventAI, Display, TEXT("%s npc=%s status=path_idle_near next_target=%s move_result=%d"),
					bReachedTrafficWaypoint ? TEXT("TrafficWaypointArrive") : TEXT("SmartObjectApproachArrive"),
					*NPC->GetName(),
					*Memory->TargetLocation.ToCompactString(),
					static_cast<int32>(MoveResult));

				if (MoveResult == EPathFollowingRequestResult::Failed)
				{
					if (UEventSmartObjectComponent* ClaimedSmartObject = NPC->GetClaimedSmartObject())
					{
						ClaimedSmartObject->MarkSlotFailed(NPC->GetClaimedSlotIndex());
					}
					NPC->RememberFailedMoveTarget(Memory->TargetLocation);
					NPC->ClearDebugMoveTarget();
					NPC->ReleaseClaimedSlot();
					FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
				}
				return;
			}

			PlaceNPCAtClaimedSlot(NPC, false);
			if (!bWaitAtClaimedSlot)
			{
				NPC->ClearDebugMoveTarget();
				UE_LOG(LogMajicEventAI, Display, TEXT("SmartObjectArrive npc=%s status=path_idle_near wait=false"), *NPC->GetName());
				FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
				return;
			}

			Memory->bArrivedAtSlot = true;
			Memory->OccupancyElapsedTime = 0.0f;
			NPC->ClearDebugMoveTarget();
			PlaceNPCAtClaimedSlot(NPC, true);
			UE_LOG(LogMajicEventAI, Display, TEXT("SmartObjectArrive npc=%s status=path_idle_near wait=true duration=%.1f"),
				*NPC->GetName(),
				Memory->OccupancyDuration);
			return;
		}

		if (Memory->PathIdleRepathAttempts < MaxPathIdleRepathAttempts)
		{
			++Memory->PathIdleRepathAttempts;
			Controller->StopMovement();
			Memory->ElapsedTime = 0.0f;
			Memory->StuckTime = 0.0f;
			Memory->BlockedNearTargetTime = 0.0f;
			Memory->LastObservedLocation = NPC->GetActorLocation();
			Memory->bLoggedDirectMovementFallback = false;

			const EPathFollowingRequestResult::Type MoveResult = Controller->MoveToLocation(
				Memory->TargetLocation,
				AcceptanceRadius,
				true,
				true,
				false,
				true);

			UE_LOG(LogMajicEventAI, Display, TEXT("SmartObjectRepath npc=%s reason=path_idle attempt=%d target=%s move_result=%d"),
				*NPC->GetName(),
				Memory->PathIdleRepathAttempts,
				*Memory->TargetLocation.ToCompactString(),
				static_cast<int32>(MoveResult));

			if (MoveResult != EPathFollowingRequestResult::Failed)
			{
				return;
			}
		}

		Controller->StopMovement();
		if (UEventSmartObjectComponent* ClaimedSmartObject = NPC->GetClaimedSmartObject())
		{
			ClaimedSmartObject->MarkSlotFailed(NPC->GetClaimedSlotIndex());
		}
		NPC->RememberFailedMoveTarget(Memory->TargetLocation);
		NPC->ClearDebugMoveTarget();
		NPC->ReleaseClaimedSlot();
		UE_LOG(LogMajicEventAI, Warning, TEXT("SmartObjectRelease npc=%s reason=path_idle current=%s target=%s elapsed=%.1f"),
			*NPC->GetName(),
			*NPC->GetActorLocation().ToCompactString(),
			*Memory->TargetLocation.ToCompactString(),
			Memory->ElapsedTime);
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	if (!bUseDirectMovementFallback)
	{
		return;
	}

	const bool bShouldUseDirectFallback = Memory->bLoggedDirectMovementFallback || bPathFollowingIdle || Memory->StuckTime > 1.5f;
	if (!bShouldUseDirectFallback)
	{
		return;
	}

	const FVector ToTarget = Memory->TargetLocation - NPC->GetActorLocation();
	const FVector PlanarDirection = FVector(ToTarget.X, ToTarget.Y, 0.0f).GetSafeNormal();
	if (!PlanarDirection.IsNearlyZero())
	{
		if (!Memory->bLoggedDirectMovementFallback)
		{
			UE_LOG(LogMajicEventAI, Display, TEXT("SmartObjectDirectFallback npc=%s target=%s"), *NPC->GetName(), *Memory->TargetLocation.ToCompactString());
			Memory->bLoggedDirectMovementFallback = true;
		}

		const float DirectMoveSpeed = 260.0f;
		const float RemainingDistance = FMath::Sqrt(DistSq2D);
		const float StepDistance = FMath::Min(DirectMoveSpeed * DeltaSeconds, FMath::Max(0.0f, RemainingDistance - AcceptanceRadius * 0.5f));
		if (StepDistance > 0.0f)
		{
			const FVector StartLocation = NPC->GetActorLocation();
			FVector NewLocation = StartLocation + PlanarDirection * StepDistance;
			NewLocation.Z = StartLocation.Z;

			FHitResult Hit;
			if (NPC->SetActorLocation(NewLocation, true, &Hit))
			{
				NPC->SetActorRotation(PlanarDirection.Rotation());
			}
			else if (Hit.bBlockingHit)
			{
				const FVector SlideDirection = FVector::VectorPlaneProject(PlanarDirection, Hit.Normal).GetSafeNormal();
				if (!SlideDirection.IsNearlyZero())
				{
					FVector SlideLocation = StartLocation + SlideDirection * StepDistance;
					SlideLocation.Z = StartLocation.Z;

					FHitResult SlideHit;
					if (NPC->SetActorLocation(SlideLocation, true, &SlideHit))
					{
						NPC->SetActorRotation(SlideDirection.Rotation());
					}
				}
			}
		}
	}

	if (Memory->ElapsedTime >= MaxMoveTimeSeconds)
	{
		Controller->StopMovement();
		if (UEventSmartObjectComponent* ClaimedSmartObject = NPC->GetClaimedSmartObject())
		{
			ClaimedSmartObject->MarkSlotFailed(NPC->GetClaimedSlotIndex());
		}
		NPC->RememberFailedMoveTarget(Memory->TargetLocation);
		NPC->ClearDebugMoveTarget();
		NPC->ReleaseClaimedSlot();
		UE_LOG(LogMajicEventAI, Warning, TEXT("SmartObjectRelease npc=%s reason=move_timeout current=%s target=%s"),
			*NPC->GetName(),
			*NPC->GetActorLocation().ToCompactString(),
			*Memory->TargetLocation.ToCompactString());
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// If path following has stopped, keep the direct movement fallback active so
	// test levels without runtime Recast nav data still show visible guest motion.
}

EBTNodeResult::Type UEventBTTask_FindSmartObject::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	if (Controller)
	{
		Controller->StopMovement();
		if (AEventNPCActor* NPC = Cast<AEventNPCActor>(Controller->GetPawn()))
		{
			NPC->ClearDebugMoveTarget();
			NPC->ReleaseClaimedSlot();
		}
	}

	return EBTNodeResult::Aborted;
}

FString UEventBTTask_FindSmartObject::GetStaticDescription() const
{
	return FString::Printf(TEXT("Find Smart Object\nTag: %s\nTarget: %s"), *SearchTag.ToString(), *TargetLocationKey.SelectedKeyName.ToString());
}
