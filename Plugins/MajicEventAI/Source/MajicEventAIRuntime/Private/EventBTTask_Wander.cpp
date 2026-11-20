#include "EventBTTask_Wander.h"

#include "EventNPCActor.h"
#include "EventSmartObjectComponent.h"
#include "EventTrafficAreaActor.h"
#include "MajicEventAIModule.h"
#include "AIController.h"
#include "EngineUtils.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Navigation/PathFollowingComponent.h"

namespace
{
	TMap<TWeakObjectPtr<UWorld>, TSet<TWeakObjectPtr<AEventNPCActor>>> ActiveWanderersByWorld;

	struct FWanderCandidate
	{
		FNavLocation Location;
		FString SelectionMode;
		FString TrafficAreas;
		float Score = -FLT_MAX;
		float TrafficScore = 0.0f;
	};

	float GetTrafficAreaScoreDetails(UWorld* World, const FVector& FromLocation, const FVector& ToLocation, FString& OutAreaNames, FString& OutNoThroughAreaNames)
	{
		OutAreaNames.Reset();
		OutNoThroughAreaNames.Reset();
		if (!World)
		{
			return 0.0f;
		}

		return AEventTrafficAreaActor::CalculateTrafficPathScore(World, FromLocation, ToLocation, &OutAreaNames, &OutNoThroughAreaNames);
	}

	float ScoreWanderDestination(
		UWorld* World,
		const FVector& Origin,
		const FVector& Destination,
		float MaxDistance,
		float OpenSpaceRadius,
		float NearbySmartObjectPenalty,
		float NearbyNPCPenalty,
		float TravelDistanceScoreWeight,
		float TrafficAreaScoreWeight,
		float* OutTrafficScore = nullptr,
		FString* OutTrafficAreas = nullptr)
	{
		if (!World || OpenSpaceRadius <= 0.0f)
		{
			return 0.0f;
		}

		const float RadiusSq = FMath::Square(OpenSpaceRadius);
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

		const float DistanceAlpha = MaxDistance > 0.0f
			? FMath::Clamp(FVector::Dist2D(Origin, Destination) / MaxDistance, 0.0f, 1.0f)
			: 0.0f;
		FString TrafficAreas;
		FString NoThroughAreas;
		const float TrafficScore = GetTrafficAreaScoreDetails(World, Origin, Destination, TrafficAreas, NoThroughAreas);
		if (OutTrafficScore)
		{
			*OutTrafficScore = TrafficScore;
		}
		if (OutTrafficAreas)
		{
			*OutTrafficAreas = NoThroughAreas.IsEmpty()
				? TrafficAreas
				: FString::Printf(TEXT("%s no_through=%s"), *TrafficAreas, *NoThroughAreas);
		}

		return DistanceAlpha * TravelDistanceScoreWeight
			- static_cast<float>(NearbySmartObjectCount) * NearbySmartObjectPenalty
			- static_cast<float>(NearbyNPCCount) * NearbyNPCPenalty
			+ TrafficScore * TrafficAreaScoreWeight;
	}

	bool ProjectWanderCandidate(
		UNavigationSystemV1& NavSystem,
		UWorld* World,
		AEventNPCActor* EventNPC,
		const FVector& Origin,
		const FVector& Direction,
		float Distance,
		bool bRequireCompletePath,
		FNavLocation& OutLocation)
	{
		const FVector DesiredLocation = Origin + Direction.GetSafeNormal2D() * Distance;
		if (!NavSystem.ProjectPointToNavigation(DesiredLocation, OutLocation, FVector(250.0f, 250.0f, 300.0f)))
		{
			return false;
		}

		if (EventNPC && EventNPC->IsRecentFailedMoveTarget(OutLocation.Location))
		{
			return false;
		}

		if (!bRequireCompletePath)
		{
			return !AEventTrafficAreaActor::IsRouteBlockedByNoThroughZone(World, Origin, OutLocation.Location);
		}

		UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(World, Origin, OutLocation.Location);
		return Path && Path->IsValid() && !Path->IsPartial() &&
			!AEventTrafficAreaActor::IsRouteBlockedByNoThroughZone(World, Origin, OutLocation.Location);
	}

	bool ProjectWanderCandidateWithNoThroughStats(
		UNavigationSystemV1& NavSystem,
		UWorld* World,
		AEventNPCActor* EventNPC,
		const FVector& Origin,
		const FVector& Direction,
		float Distance,
		bool bRequireCompletePath,
		FNavLocation& OutLocation,
		int32& OutRejectedNoThroughCount,
		TMap<FString, int32>& OutRejectedNoThroughAreas)
	{
		const FVector DesiredLocation = Origin + Direction.GetSafeNormal2D() * Distance;
		if (!NavSystem.ProjectPointToNavigation(DesiredLocation, OutLocation, FVector(250.0f, 250.0f, 300.0f)))
		{
			return false;
		}

		if (EventNPC && EventNPC->IsRecentFailedMoveTarget(OutLocation.Location))
		{
			return false;
		}

		if (bRequireCompletePath)
		{
			UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(World, Origin, OutLocation.Location);
			if (!Path || !Path->IsValid() || Path->IsPartial())
			{
				return false;
			}
		}

		FString NoThroughAreas;
		if (AEventTrafficAreaActor::IsRouteBlockedByNoThroughZone(World, Origin, OutLocation.Location, &NoThroughAreas))
		{
			++OutRejectedNoThroughCount;
			if (!NoThroughAreas.IsEmpty())
			{
				OutRejectedNoThroughAreas.FindOrAdd(NoThroughAreas)++;
			}
			return false;
		}

		return true;
	}

	void FaceEscapeDirection(APawn& Pawn, const FVector& FailedTarget, float EscapeTurnDegrees)
	{
		FVector AwayFromTarget = Pawn.GetActorLocation() - FailedTarget;
		AwayFromTarget.Z = 0.0f;
		if (AwayFromTarget.IsNearlyZero())
		{
			AwayFromTarget = Pawn.GetActorForwardVector().RotateAngleAxis(EscapeTurnDegrees, FVector::UpVector);
		}
		else
		{
			AwayFromTarget = AwayFromTarget.GetSafeNormal().RotateAngleAxis(FMath::RandBool() ? EscapeTurnDegrees * 0.25f : -EscapeTurnDegrees * 0.25f, FVector::UpVector);
		}

		if (!AwayFromTarget.IsNearlyZero())
		{
			Pawn.SetActorRotation(AwayFromTarget.Rotation());
		}
	}

	void PruneActiveWanderers(UWorld* World)
	{
		if (!World)
		{
			return;
		}

		TSet<TWeakObjectPtr<AEventNPCActor>>& ActiveWanderers = ActiveWanderersByWorld.FindOrAdd(World);
		TArray<TWeakObjectPtr<AEventNPCActor>> InvalidNPCs;
		for (const TWeakObjectPtr<AEventNPCActor>& NPC : ActiveWanderers)
		{
			if (!NPC.IsValid())
			{
				InvalidNPCs.Add(NPC);
			}
		}
		for (const TWeakObjectPtr<AEventNPCActor>& InvalidNPC : InvalidNPCs)
		{
			ActiveWanderers.Remove(InvalidNPC);
		}
	}

	int32 CountEventNPCs(UWorld* World)
	{
		if (!World)
		{
			return 0;
		}

		int32 NPCCount = 0;
		for (TActorIterator<AEventNPCActor> It(World); It; ++It)
		{
			if (*It)
			{
				++NPCCount;
			}
		}
		return NPCCount;
	}

	bool TryRegisterActiveWanderer(AEventNPCActor* NPC, float MaxActiveWanderRatio, int32& OutActiveCount, int32& OutAllowedCount, int32& OutNPCCount)
	{
		OutActiveCount = 0;
		OutAllowedCount = 0;
		OutNPCCount = 0;

		if (!NPC || MaxActiveWanderRatio <= 0.0f)
		{
			return false;
		}

		UWorld* World = NPC->GetWorld();
		if (!World)
		{
			return false;
		}

		PruneActiveWanderers(World);
		TSet<TWeakObjectPtr<AEventNPCActor>>& ActiveWanderers = ActiveWanderersByWorld.FindOrAdd(World);
		if (ActiveWanderers.Contains(NPC))
		{
			OutActiveCount = ActiveWanderers.Num();
			OutNPCCount = CountEventNPCs(World);
			OutAllowedCount = FMath::Max(1, FMath::CeilToInt(static_cast<float>(OutNPCCount) * MaxActiveWanderRatio));
			return true;
		}

		OutNPCCount = CountEventNPCs(World);
		OutAllowedCount = FMath::Max(1, FMath::CeilToInt(static_cast<float>(OutNPCCount) * MaxActiveWanderRatio));
		OutActiveCount = ActiveWanderers.Num();
		if (OutActiveCount >= OutAllowedCount)
		{
			return false;
		}

		ActiveWanderers.Add(NPC);
		OutActiveCount = ActiveWanderers.Num();
		return true;
	}

	void UnregisterActiveWanderer(AEventNPCActor* NPC)
	{
		if (!NPC)
		{
			return;
		}

		UWorld* World = NPC->GetWorld();
		if (!World)
		{
			return;
		}

		if (TSet<TWeakObjectPtr<AEventNPCActor>>* ActiveWanderers = ActiveWanderersByWorld.Find(World))
		{
			ActiveWanderers->Remove(NPC);
			PruneActiveWanderers(World);
		}
	}

	bool PickForwardWanderPoint(
		UNavigationSystemV1& NavSystem,
		const APawn& Pawn,
		AEventNPCActor* EventNPC,
		const FVector* PreferredDirection,
		bool bRequireCompletePath,
		float MinDistance,
		float MaxDistance,
		float ForwardJitterDegrees,
		float ObstacleTurnDegrees,
		int32 ForwardAttempts,
		float OpenSpaceRadius,
		float NearbySmartObjectPenalty,
		float NearbyNPCPenalty,
		float TravelDistanceScoreWeight,
		float TrafficAreaScoreWeight,
		FNavLocation& OutLocation,
		FString& OutSelectionMode,
		int32& OutRejectedNoThroughCount,
		TMap<FString, int32>& OutRejectedNoThroughAreas)
	{
		const FVector Origin = Pawn.GetActorLocation();
		FVector Forward = PreferredDirection ? *PreferredDirection : Pawn.GetActorForwardVector();
		Forward.Z = 0.0f;
		if (Forward.IsNearlyZero())
		{
			Forward = FVector::ForwardVector;
		}
		Forward.Normalize();

		const float ClampedMaxDistance = FMath::Max(MaxDistance, 100.0f);
		const float ClampedMinDistance = FMath::Clamp(MinDistance, 100.0f, ClampedMaxDistance);
		const int32 Attempts = FMath::Max(1, ForwardAttempts);
		FWanderCandidate BestCandidate;

		for (int32 Attempt = 0; Attempt < Attempts; ++Attempt)
		{
			const float AngleDegrees = FMath::RandRange(-ForwardJitterDegrees, ForwardJitterDegrees);
			const float Distance = FMath::RandRange(ClampedMinDistance, ClampedMaxDistance);
			const FVector Direction = Forward.RotateAngleAxis(AngleDegrees, FVector::UpVector);
			FNavLocation CandidateLocation;
			if (ProjectWanderCandidateWithNoThroughStats(NavSystem, Pawn.GetWorld(), EventNPC, Origin, Direction, Distance, bRequireCompletePath, CandidateLocation, OutRejectedNoThroughCount, OutRejectedNoThroughAreas))
			{
				float TrafficScore = 0.0f;
				FString TrafficAreas;
				const float Score = ScoreWanderDestination(Pawn.GetWorld(), Origin, CandidateLocation.Location, ClampedMaxDistance, OpenSpaceRadius, NearbySmartObjectPenalty, NearbyNPCPenalty, TravelDistanceScoreWeight, TrafficAreaScoreWeight, &TrafficScore, &TrafficAreas);
				if (Score > BestCandidate.Score)
				{
					BestCandidate.Location = CandidateLocation;
					BestCandidate.Score = Score;
					BestCandidate.TrafficScore = TrafficScore;
					BestCandidate.TrafficAreas = TrafficAreas;
					BestCandidate.SelectionMode = FString::Printf(TEXT("forward angle=%.1f distance=%.0f open_score=%.2f"), AngleDegrees, Distance, Score);
				}
			}
		}

		const float TurnAngles[] =
		{
			ObstacleTurnDegrees,
			-ObstacleTurnDegrees,
			ObstacleTurnDegrees * 1.7f,
			-ObstacleTurnDegrees * 1.7f,
			180.0f
		};

		for (const float TurnAngle : TurnAngles)
		{
			const float Distance = FMath::RandRange(ClampedMinDistance, ClampedMaxDistance);
			const FVector Direction = Forward.RotateAngleAxis(TurnAngle, FVector::UpVector);
			FNavLocation CandidateLocation;
			if (ProjectWanderCandidateWithNoThroughStats(NavSystem, Pawn.GetWorld(), EventNPC, Origin, Direction, Distance, bRequireCompletePath, CandidateLocation, OutRejectedNoThroughCount, OutRejectedNoThroughAreas))
			{
				float TrafficScore = 0.0f;
				FString TrafficAreas;
				const float Score = ScoreWanderDestination(Pawn.GetWorld(), Origin, CandidateLocation.Location, ClampedMaxDistance, OpenSpaceRadius, NearbySmartObjectPenalty, NearbyNPCPenalty, TravelDistanceScoreWeight, TrafficAreaScoreWeight, &TrafficScore, &TrafficAreas);
				if (Score > BestCandidate.Score)
				{
					BestCandidate.Location = CandidateLocation;
					BestCandidate.Score = Score;
					BestCandidate.TrafficScore = TrafficScore;
					BestCandidate.TrafficAreas = TrafficAreas;
					BestCandidate.SelectionMode = FString::Printf(TEXT("obstacle_turn angle=%.1f distance=%.0f open_score=%.2f"), TurnAngle, Distance, Score);
				}
			}
		}

		if (BestCandidate.Score > -FLT_MAX)
		{
			OutLocation = BestCandidate.Location;
			OutSelectionMode = BestCandidate.SelectionMode;
			if (!BestCandidate.TrafficAreas.IsEmpty())
			{
				OutSelectionMode += FString::Printf(TEXT(" traffic_score=%.2f traffic_areas=%s"), BestCandidate.TrafficScore, *BestCandidate.TrafficAreas);
			}
			return true;
		}

		return false;
	}
}

UEventBTTask_Wander::UEventBTTask_Wander()
{
	NodeName = TEXT("Wander");
	bNotifyTick = true;
}

EBTNodeResult::Type UEventBTTask_Wander::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	if (!Controller || !Controller->GetPawn())
	{
		return EBTNodeResult::Failed;
	}

	const FVector Origin = Controller->GetPawn()->GetActorLocation();
	UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Controller->GetWorld());
	if (!NavSystem)
	{
		return EBTNodeResult::Failed;
	}

	FNavLocation WanderPoint;
	FString SelectionMode;
	FVector PreferredDirection = FVector::ZeroVector;
	AEventNPCActor* MutableEventNPC = Cast<AEventNPCActor>(Controller->GetPawn());
	const bool bLeavingClaimedSlot = MutableEventNPC && MutableEventNPC->HasClaimedSlot();
	FEventWanderTaskMemory* Memory = reinterpret_cast<FEventWanderTaskMemory*>(NodeMemory);
	Memory->TargetLocation = FVector::ZeroVector;
	Memory->LastObservedLocation = Controller->GetPawn()->GetActorLocation();
	Memory->ElapsedTime = 0.0f;
	Memory->StuckTime = 0.0f;
	Memory->BlockedNearTargetTime = 0.0f;
	Memory->StationaryWaitDuration = 0.0f;
	Memory->bLoggedDirectMovementFallback = false;
	Memory->bRegisteredActiveWander = false;
	Memory->bStationaryWait = false;
	Memory->bExitClearanceMove = false;

	if (MutableEventNPC && !MutableEventNPC->CanWander())
	{
		Controller->StopMovement();
		MutableEventNPC->ClearDebugMoveTarget();
		Memory->StationaryWaitDuration = 60.0f;
		Memory->bStationaryWait = true;
		UE_LOG(LogMajicEventAI, Display, TEXT("WanderDisabled npc=%s claimed=%s wait=%.1f action=stationary_social_guest"),
			*MutableEventNPC->GetName(),
			MutableEventNPC->HasClaimedSlot() ? TEXT("true") : TEXT("false"),
			Memory->StationaryWaitDuration);
		return EBTNodeResult::InProgress;
	}

	int32 ActiveWanderCount = 0;
	int32 AllowedWanderCount = 0;
	int32 TotalNPCCount = 0;
	if (!bLeavingClaimedSlot && !TryRegisterActiveWanderer(MutableEventNPC, MaxActiveWanderRatio, ActiveWanderCount, AllowedWanderCount, TotalNPCCount))
	{
		const float MinWait = FMath::Max(0.0f, MinStationaryWaitWhenWanderLimited);
		const float MaxWait = FMath::Max(MinWait, MaxStationaryWaitWhenWanderLimited);
		Memory->StationaryWaitDuration = FMath::RandRange(MinWait, MaxWait);
		Memory->bStationaryWait = true;
		if (Controller)
		{
			Controller->StopMovement();
		}
		if (MutableEventNPC)
		{
			MutableEventNPC->ClearDebugMoveTarget();
		}

		UE_LOG(LogMajicEventAI, Display, TEXT("WanderLimited npc=%s active=%d allowed=%d total=%d wait=%.1f action=stationary"),
			Controller->GetPawn() ? *Controller->GetPawn()->GetName() : TEXT("unknown"),
			ActiveWanderCount,
			AllowedWanderCount,
			TotalNPCCount,
			Memory->StationaryWaitDuration);
		return EBTNodeResult::InProgress;
	}

	Memory->bRegisteredActiveWander = !bLeavingClaimedSlot;
	const bool bHasPreferredDirection = MutableEventNPC && MutableEventNPC->ConsumePreferredWanderDirection(PreferredDirection);
	bool bPickedWanderPoint = false;
	int32 RejectedNoThroughCount = 0;
	TMap<FString, int32> RejectedNoThroughAreas;
	if (bLeavingClaimedSlot && bHasPreferredDirection && ExitClearanceDistance > 0.0f)
	{
		FVector ExitDirection = PreferredDirection;
		ExitDirection.Z = 0.0f;
		ExitDirection = ExitDirection.GetSafeNormal();
		if (!ExitDirection.IsNearlyZero())
		{
			const float CandidateAngles[] = { 0.0f, -25.0f, 25.0f, -50.0f, 50.0f, -80.0f, 80.0f, 120.0f, -120.0f, 180.0f };
			const float CandidateDistances[] = { 120.0f, 180.0f, 260.0f, ExitClearanceDistance, 420.0f };
			float BestClearanceScore = -FLT_MAX;
			FNavLocation BestClearanceLocation;
			float BestClearanceAngle = 0.0f;
			float BestClearanceDistance = 0.0f;

			for (const float CandidateDistance : CandidateDistances)
			{
				for (const float CandidateAngle : CandidateAngles)
				{
					const FVector CandidateDirection = ExitDirection.RotateAngleAxis(CandidateAngle, FVector::UpVector).GetSafeNormal();
					const FVector DesiredExitLocation = Origin + CandidateDirection * CandidateDistance;
					FNavLocation ExitClearanceLocation;
					if (!NavSystem->ProjectPointToNavigation(
						DesiredExitLocation,
						ExitClearanceLocation,
						FVector(FMath::Max(ExitClearanceProjectionExtent, 420.0f), FMath::Max(ExitClearanceProjectionExtent, 420.0f), 500.0f)))
					{
						continue;
					}

					const FVector CandidateOffset = ExitClearanceLocation.Location - Origin;
					const FVector CandidatePlanarDirection(CandidateOffset.X, CandidateOffset.Y, 0.0f);
					const float ProjectedDistance = CandidatePlanarDirection.Size();
					if (ProjectedDistance < AcceptanceRadius)
					{
						continue;
					}

					const float Alignment = FVector::DotProduct(CandidatePlanarDirection.GetSafeNormal(), ExitDirection);
					const float Score = Alignment * 1000.0f + ProjectedDistance - FMath::Abs(ProjectedDistance - CandidateDistance) * 0.5f;
					if (Score > BestClearanceScore)
					{
						BestClearanceScore = Score;
						BestClearanceLocation = ExitClearanceLocation;
						BestClearanceAngle = CandidateAngle;
						BestClearanceDistance = ProjectedDistance;
					}
				}
			}

			if (BestClearanceScore > -FLT_MAX)
			{
				WanderPoint = BestClearanceLocation;
				SelectionMode = FString::Printf(TEXT("seat_exit_nav angle=%.0f distance=%.0f"), BestClearanceAngle, BestClearanceDistance);
				Memory->bExitClearanceMove = true;
				bPickedWanderPoint = true;
			}
		}
	}

	if (!bPickedWanderPoint && bHasPreferredDirection && ExitClearanceDistance > 0.0f)
	{
		FVector ExitDirection = PreferredDirection;
		ExitDirection.Z = 0.0f;
		ExitDirection = ExitDirection.GetSafeNormal();
		if (!ExitDirection.IsNearlyZero())
		{
			const float CandidateAngles[] = { 0.0f, -35.0f, 35.0f, -70.0f, 70.0f, 110.0f, -110.0f };
			float BestClearanceScore = -FLT_MAX;
			FNavLocation BestClearanceLocation;
			float BestClearanceAngle = 0.0f;
			float BestClearanceDistance = 0.0f;

			for (const float CandidateAngle : CandidateAngles)
			{
				const FVector CandidateDirection = ExitDirection.RotateAngleAxis(CandidateAngle, FVector::UpVector).GetSafeNormal();
				const FVector DesiredExitLocation = Origin + CandidateDirection * ExitClearanceDistance;
				FNavLocation ExitClearanceLocation;
				if (!NavSystem->ProjectPointToNavigation(
					DesiredExitLocation,
					ExitClearanceLocation,
					FVector(ExitClearanceProjectionExtent, ExitClearanceProjectionExtent, ExitClearanceProjectionExtent)))
				{
					continue;
				}

				bool bHasCompleteClearancePath = !bRequireCompletePath;
				float ClearancePathLength = FVector::Dist2D(Origin, ExitClearanceLocation.Location);
				if (!bHasCompleteClearancePath)
				{
					if (UNavigationPath* ExitPath = UNavigationSystemV1::FindPathToLocationSynchronously(Controller->GetWorld(), Origin, ExitClearanceLocation.Location))
					{
						bHasCompleteClearancePath = ExitPath->IsValid() && !ExitPath->IsPartial();
						if (bHasCompleteClearancePath)
						{
							ClearancePathLength = ExitPath->GetPathLength();
						}
					}
				}
				if (!bHasCompleteClearancePath)
				{
					continue;
				}

				const FVector CandidateOffset = ExitClearanceLocation.Location - Origin;
				const FVector CandidatePlanarDirection(CandidateOffset.X, CandidateOffset.Y, 0.0f);
				const float CandidateDistance = CandidatePlanarDirection.Size();
				if (CandidateDistance < AcceptanceRadius * 1.5f)
				{
					continue;
				}

				const float Alignment = FVector::DotProduct(CandidatePlanarDirection.GetSafeNormal(), ExitDirection);
				const float Score = Alignment * 1000.0f + CandidateDistance - ClearancePathLength * 0.1f;
				if (Score > BestClearanceScore)
				{
					BestClearanceScore = Score;
					BestClearanceLocation = ExitClearanceLocation;
					BestClearanceAngle = CandidateAngle;
					BestClearanceDistance = CandidateDistance;
				}
			}

			if (BestClearanceScore > -FLT_MAX)
			{
				WanderPoint = BestClearanceLocation;
				SelectionMode = FString::Printf(TEXT("preferred_exit clearance angle=%.0f distance=%.0f"), BestClearanceAngle, BestClearanceDistance);
				Memory->bExitClearanceMove = true;
				bPickedWanderPoint = true;
			}
		}
	}

	if (!bPickedWanderPoint && !PickForwardWanderPoint(
		*NavSystem,
		*Controller->GetPawn(),
		MutableEventNPC,
		bHasPreferredDirection ? &PreferredDirection : nullptr,
		bRequireCompletePath,
		MinWanderDistance,
		WanderRadius,
		ForwardDirectionJitterDegrees,
		ObstacleTurnDegrees,
		ForwardCandidateAttempts,
		OpenSpaceCheckRadius,
		NearbySmartObjectPenalty,
		NearbyNPCPenalty,
		TravelDistanceScoreWeight,
		TrafficAreaScoreWeight,
		WanderPoint,
		SelectionMode,
		RejectedNoThroughCount,
		RejectedNoThroughAreas))
	{
		const float MaxDistance = FMath::Max(WanderRadius, 100.0f);
		const float MinDistance = FMath::Clamp(MinWanderDistance, 100.0f, MaxDistance);
		FWanderCandidate BestRandomCandidate;
		const int32 RandomAttempts = FMath::Max(1, RandomFallbackCandidateAttempts);
		for (int32 Attempt = 0; Attempt < RandomAttempts; ++Attempt)
		{
			const float AngleRadians = FMath::RandRange(0.0f, 2.0f * PI);
			const float Distance = FMath::RandRange(MinDistance, MaxDistance);
			const FVector RandomDirection(FMath::Cos(AngleRadians), FMath::Sin(AngleRadians), 0.0f);
			FNavLocation CandidateLocation;
			if (ProjectWanderCandidateWithNoThroughStats(*NavSystem, Controller->GetWorld(), MutableEventNPC, Origin, RandomDirection, Distance, bRequireCompletePath, CandidateLocation, RejectedNoThroughCount, RejectedNoThroughAreas))
			{
				float TrafficScore = 0.0f;
				FString TrafficAreas;
				const float Score = ScoreWanderDestination(Controller->GetWorld(), Origin, CandidateLocation.Location, MaxDistance, OpenSpaceCheckRadius, NearbySmartObjectPenalty, NearbyNPCPenalty, TravelDistanceScoreWeight, TrafficAreaScoreWeight, &TrafficScore, &TrafficAreas);
				if (Score > BestRandomCandidate.Score)
				{
					BestRandomCandidate.Location = CandidateLocation;
					BestRandomCandidate.Score = Score;
					BestRandomCandidate.TrafficScore = TrafficScore;
					BestRandomCandidate.TrafficAreas = TrafficAreas;
					BestRandomCandidate.SelectionMode = FString::Printf(TEXT("random_complete_path_fallback distance=%.0f open_score=%.2f"), Distance, Score);
				}
			}
		}

		if (BestRandomCandidate.Score <= -FLT_MAX)
		{
			const float AngleRadians = FMath::RandRange(0.0f, 2.0f * PI);
			const float Distance = FMath::RandRange(MinDistance, MaxDistance);
			const FVector RandomDirection(FMath::Cos(AngleRadians), FMath::Sin(AngleRadians), 0.0f);
			FaceEscapeDirection(*Controller->GetPawn(), Origin + RandomDirection * Distance, EscapeTurnDegrees);
			const float MinWait = FMath::Max(0.0f, MinStationaryWaitWhenWanderLimited);
			const float MaxWait = FMath::Max(MinWait, MaxStationaryWaitWhenWanderLimited);
			Memory->StationaryWaitDuration = FMath::RandRange(MinWait, MaxWait);
			Memory->bStationaryWait = true;
			Controller->StopMovement();
			if (MutableEventNPC)
			{
				MutableEventNPC->ClearDebugMoveTarget();
			}
			UE_LOG(LogMajicEventAI, Warning, TEXT("WanderNoNavFallback npc=%s current=%s radius=%.0f wait=%.1f action=stationary"),
				*Controller->GetPawn()->GetName(),
				*Origin.ToCompactString(),
				WanderRadius,
				Memory->StationaryWaitDuration);
			if (MutableEventNPC && Memory->bRegisteredActiveWander)
			{
				UnregisterActiveWanderer(MutableEventNPC);
				Memory->bRegisteredActiveWander = false;
			}
			return EBTNodeResult::InProgress;
		}

		WanderPoint = BestRandomCandidate.Location;
		SelectionMode = BestRandomCandidate.SelectionMode;
		if (!BestRandomCandidate.TrafficAreas.IsEmpty())
		{
			SelectionMode += FString::Printf(TEXT(" traffic_score=%.2f traffic_areas=%s"), BestRandomCandidate.TrafficScore, *BestRandomCandidate.TrafficAreas);
		}
		UE_LOG(LogMajicEventAI, Warning, TEXT("WanderNoNavFallback npc=%s current=%s radius=%.0f target=%s"),
			*Controller->GetPawn()->GetName(),
			*Origin.ToCompactString(),
			WanderRadius,
			*WanderPoint.Location.ToCompactString());
	}
	else if (bHasPreferredDirection && !Memory->bExitClearanceMove)
	{
		SelectionMode = FString::Printf(TEXT("preferred_exit %s"), *SelectionMode);
	}

	if (RejectedNoThroughCount > 0)
	{
		UE_LOG(LogMajicEventAI, Display, TEXT("WanderCandidatesRejected npc=%s no_through=%d selected_target=%s"),
			*Controller->GetPawn()->GetName(),
			RejectedNoThroughCount,
			*WanderPoint.Location.ToCompactString());
		for (const TPair<FString, int32>& RejectedArea : RejectedNoThroughAreas)
		{
			UE_LOG(LogMajicEventAI, Display, TEXT("TrafficRouteRejected npc=%s reason=wander_no_through area=%s count=%d"),
				*Controller->GetPawn()->GetName(),
				*RejectedArea.Key,
				RejectedArea.Value);
		}
	}

	Memory->TargetLocation = WanderPoint.Location;
	if (APawn* Pawn = Controller->GetPawn())
	{
		FVector MoveDirection = WanderPoint.Location - Pawn->GetActorLocation();
		MoveDirection.Z = 0.0f;
		if (!MoveDirection.IsNearlyZero())
		{
			Pawn->SetActorRotation(MoveDirection.GetSafeNormal().Rotation());
		}
	}

	if (MutableEventNPC)
	{
		MutableEventNPC->SetDebugMoveTarget(WanderPoint.Location, TEXT("wander"));
	}

	if (bLeavingClaimedSlot && MutableEventNPC)
	{
		Controller->StopMovement();
		MutableEventNPC->ReleaseClaimedSlot(false);
		MutableEventNPC->StartLocomotionAnimationState();
		MutableEventNPC->SetActorLocation(WanderPoint.Location, false, nullptr, ETeleportType::TeleportPhysics);
		Memory->TargetLocation = WanderPoint.Location;
		Memory->LastObservedLocation = WanderPoint.Location;
		MutableEventNPC->ClearDebugMoveTarget();
		UE_LOG(LogMajicEventAI, Display, TEXT("SmartObjectRelease npc=%s reason=seat_exit_nav exit_location=%s mode=\"%s\""),
			*MutableEventNPC->GetName(),
			*WanderPoint.Location.ToCompactString(),
			*SelectionMode);
		return EBTNodeResult::Succeeded;
	}

	const EPathFollowingRequestResult::Type MoveResult = Controller->MoveToLocation(WanderPoint.Location, AcceptanceRadius, true, true, false, true);
	if (MoveResult == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		if (MutableEventNPC)
		{
			MutableEventNPC->ClearDebugMoveTarget();
			if (Memory->bRegisteredActiveWander)
			{
				UnregisterActiveWanderer(MutableEventNPC);
				Memory->bRegisteredActiveWander = false;
			}
		}
		UE_LOG(LogMajicEventAI, Display, TEXT("WanderArrive npc=%s status=already_at_goal target=%s"),
			*Controller->GetPawn()->GetName(),
			*WanderPoint.Location.ToCompactString());
		return EBTNodeResult::Succeeded;
	}

	if (MoveResult == EPathFollowingRequestResult::Failed)
	{
		if (MutableEventNPC)
		{
			MutableEventNPC->RememberFailedMoveTarget(WanderPoint.Location);
			MutableEventNPC->ClearDebugMoveTarget();
			if (Memory->bRegisteredActiveWander)
			{
				UnregisterActiveWanderer(MutableEventNPC);
				Memory->bRegisteredActiveWander = false;
			}
		}
		FaceEscapeDirection(*Controller->GetPawn(), WanderPoint.Location, EscapeTurnDegrees);
		UE_LOG(LogMajicEventAI, Warning, TEXT("WanderMoveRejected npc=%s current=%s target=%s action=escape_turn"),
			*Controller->GetPawn()->GetName(),
			*Controller->GetPawn()->GetActorLocation().ToCompactString(),
			*WanderPoint.Location.ToCompactString());
		return EBTNodeResult::Failed;
	}
	else
	{
		if (MutableEventNPC)
		{
			if (MutableEventNPC->HasClaimedSlot())
			{
				MutableEventNPC->ReleaseClaimedSlot(false);
				UE_LOG(LogMajicEventAI, Display, TEXT("SmartObjectRelease npc=%s reason=wander_started"),
					*MutableEventNPC->GetName());
			}
			MutableEventNPC->StartLocomotionAnimationState();
		}
		if (SelectionMode.Contains(TEXT("traffic_areas=")))
		{
			UE_LOG(LogMajicEventAI, Display, TEXT("TrafficRouteSelected npc=%s reason=wander start=%s destination=%s mode=\"%s\""),
				*Controller->GetPawn()->GetName(),
				*Origin.ToCompactString(),
				*WanderPoint.Location.ToCompactString(),
				*SelectionMode);
		}
		if (Memory->bExitClearanceMove)
		{
			UE_LOG(LogMajicEventAI, Display, TEXT("WanderExitClearanceStart npc=%s origin=%s target=%s mode=\"%s\""),
				*Controller->GetPawn()->GetName(),
				*Origin.ToCompactString(),
				*WanderPoint.Location.ToCompactString(),
				*SelectionMode);
		}
		UE_LOG(LogMajicEventAI, Display, TEXT("WanderStart npc=%s origin=%s target=%s radius=%.0f mode=\"%s\""),
			*Controller->GetPawn()->GetName(),
			*Origin.ToCompactString(),
			*WanderPoint.Location.ToCompactString(),
			WanderRadius,
			*SelectionMode);
	}

	return EBTNodeResult::InProgress;
}

void UEventBTTask_Wander::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (!Controller || !Pawn)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FEventWanderTaskMemory* Memory = reinterpret_cast<FEventWanderTaskMemory*>(NodeMemory);
	Memory->ElapsedTime += DeltaSeconds;
	if (Memory->bStationaryWait)
	{
		if (Memory->ElapsedTime >= Memory->StationaryWaitDuration)
		{
			UE_LOG(LogMajicEventAI, Display, TEXT("WanderLimitedComplete npc=%s waited=%.1f"),
				*Pawn->GetName(),
				Memory->ElapsedTime);
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		}
		return;
	}

	if (AEventNPCActor* NPC = Cast<AEventNPCActor>(Pawn))
	{
		NPC->MarkWanderProgress();
	}

	if (FVector::DistSquared2D(Pawn->GetActorLocation(), Memory->LastObservedLocation) < FMath::Square(1.0f))
	{
		Memory->StuckTime += DeltaSeconds;
	}
	else
	{
		Memory->StuckTime = 0.0f;
		Memory->BlockedNearTargetTime = 0.0f;
		Memory->LastObservedLocation = Pawn->GetActorLocation();
	}

	const float DistSq2D = FVector::DistSquared2D(Pawn->GetActorLocation(), Memory->TargetLocation);
	if (Memory->StuckTime > 0.25f && DistSq2D <= FMath::Square(NearTargetBlockedDistance))
	{
		Memory->BlockedNearTargetTime += DeltaSeconds;
		if (MaxBlockedNearTargetSeconds > 0.0f && Memory->BlockedNearTargetTime >= MaxBlockedNearTargetSeconds)
		{
			Controller->StopMovement();
			if (AEventNPCActor* NPC = Cast<AEventNPCActor>(Pawn))
			{
				NPC->RememberFailedMoveTarget(Memory->TargetLocation);
				NPC->ClearDebugMoveTarget();
				if (Memory->bRegisteredActiveWander)
				{
					UnregisterActiveWanderer(NPC);
					Memory->bRegisteredActiveWander = false;
				}
			}
			FaceEscapeDirection(*Pawn, Memory->TargetLocation, EscapeTurnDegrees);
			UE_LOG(LogMajicEventAI, Warning, TEXT("WanderAbandon npc=%s reason=blocked_near_target current=%s target=%s blocked_time=%.1f action=escape_turn"),
				*Pawn->GetName(),
				*Pawn->GetActorLocation().ToCompactString(),
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
		UE_LOG(LogMajicEventAI, Display, TEXT("%s npc=%s status=reached target=%s elapsed=%.1f"),
			Memory->bExitClearanceMove ? TEXT("WanderExitClearanceArrive") : TEXT("WanderArrive"),
			*Pawn->GetName(),
			*Memory->TargetLocation.ToCompactString(),
			Memory->ElapsedTime);
		if (AEventNPCActor* NPC = Cast<AEventNPCActor>(Pawn))
		{
			NPC->ClearDebugMoveTarget();
			if (Memory->bRegisteredActiveWander)
			{
				UnregisterActiveWanderer(NPC);
				Memory->bRegisteredActiveWander = false;
			}
		}
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	const bool bPathFollowingIdle = Controller->GetMoveStatus() == EPathFollowingStatus::Idle;
	if (!bUseDirectMovementFallback && bPathFollowingIdle && Memory->ElapsedTime > 1.0f)
	{
		if (DistSq2D <= FMath::Square(NearTargetBlockedDistance))
		{
			UE_LOG(LogMajicEventAI, Display, TEXT("%s npc=%s status=path_idle_near target=%s elapsed=%.1f"),
				Memory->bExitClearanceMove ? TEXT("WanderExitClearanceArrive") : TEXT("WanderArrive"),
				*Pawn->GetName(),
				*Memory->TargetLocation.ToCompactString(),
				Memory->ElapsedTime);
			if (AEventNPCActor* NPC = Cast<AEventNPCActor>(Pawn))
			{
				NPC->ClearDebugMoveTarget();
				if (Memory->bRegisteredActiveWander)
				{
					UnregisterActiveWanderer(NPC);
					Memory->bRegisteredActiveWander = false;
				}
			}
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			return;
		}

		Controller->StopMovement();
		if (AEventNPCActor* NPC = Cast<AEventNPCActor>(Pawn))
		{
			NPC->RememberFailedMoveTarget(Memory->TargetLocation);
			NPC->ClearDebugMoveTarget();
			if (Memory->bRegisteredActiveWander)
			{
				UnregisterActiveWanderer(NPC);
				Memory->bRegisteredActiveWander = false;
			}
		}
		FaceEscapeDirection(*Pawn, Memory->TargetLocation, EscapeTurnDegrees);
		UE_LOG(LogMajicEventAI, Warning, TEXT("WanderAbandon npc=%s reason=path_idle current=%s target=%s elapsed=%.1f action=retry"),
			*Pawn->GetName(),
			*Pawn->GetActorLocation().ToCompactString(),
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

	const FVector ToTarget = Memory->TargetLocation - Pawn->GetActorLocation();
	const FVector PlanarDirection = FVector(ToTarget.X, ToTarget.Y, 0.0f).GetSafeNormal();
	if (!PlanarDirection.IsNearlyZero())
	{
		if (!Memory->bLoggedDirectMovementFallback)
		{
			UE_LOG(LogMajicEventAI, Display, TEXT("WanderDirectFallback npc=%s target=%s stuck_time=%.1f"),
				*Pawn->GetName(),
				*Memory->TargetLocation.ToCompactString(),
				Memory->StuckTime);
			Memory->bLoggedDirectMovementFallback = true;
		}

		const float DirectMoveSpeed = 260.0f;
		const float RemainingDistance = FVector::Dist2D(Pawn->GetActorLocation(), Memory->TargetLocation);
		const float StepDistance = FMath::Min(DirectMoveSpeed * DeltaSeconds, FMath::Max(0.0f, RemainingDistance - AcceptanceRadius * 0.5f));
		if (StepDistance > 0.0f)
		{
			const FVector StartLocation = Pawn->GetActorLocation();
			FVector NewLocation = StartLocation + PlanarDirection * StepDistance;
			NewLocation.Z = StartLocation.Z;

			FHitResult Hit;
			if (Pawn->SetActorLocation(NewLocation, true, &Hit))
			{
				Pawn->SetActorRotation(PlanarDirection.Rotation());
			}
			else if (Hit.bBlockingHit)
			{
				const FVector SlideDirection = FVector::VectorPlaneProject(PlanarDirection, Hit.Normal).GetSafeNormal();
				if (!SlideDirection.IsNearlyZero())
				{
					FVector SlideLocation = StartLocation + SlideDirection * StepDistance;
					SlideLocation.Z = StartLocation.Z;

					FHitResult SlideHit;
					if (Pawn->SetActorLocation(SlideLocation, true, &SlideHit))
					{
						Pawn->SetActorRotation(SlideDirection.Rotation());
					}
				}
			}
		}
	}

	if (Memory->ElapsedTime >= MaxMoveTimeSeconds)
	{
		Controller->StopMovement();
		if (AEventNPCActor* NPC = Cast<AEventNPCActor>(Pawn))
		{
			NPC->RememberFailedMoveTarget(Memory->TargetLocation);
			NPC->ClearDebugMoveTarget();
			if (Memory->bRegisteredActiveWander)
			{
				UnregisterActiveWanderer(NPC);
				Memory->bRegisteredActiveWander = false;
			}
		}
		FaceEscapeDirection(*Pawn, Memory->TargetLocation, EscapeTurnDegrees);
		UE_LOG(LogMajicEventAI, Warning, TEXT("WanderTimeout npc=%s current=%s target=%s elapsed=%.1f action=escape_turn"),
			*Pawn->GetName(),
			*Pawn->GetActorLocation().ToCompactString(),
			*Memory->TargetLocation.ToCompactString(),
			Memory->ElapsedTime);
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// Keep direct movement fallback active if path following is idle.
}

EBTNodeResult::Type UEventBTTask_Wander::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AAIController* Controller = OwnerComp.GetAIOwner())
	{
		Controller->StopMovement();
		if (AEventNPCActor* NPC = Cast<AEventNPCActor>(Controller->GetPawn()))
		{
			NPC->ClearDebugMoveTarget();
			FEventWanderTaskMemory* Memory = reinterpret_cast<FEventWanderTaskMemory*>(NodeMemory);
			if (Memory && Memory->bRegisteredActiveWander)
			{
				UnregisterActiveWanderer(NPC);
				Memory->bRegisteredActiveWander = false;
			}
		}
	}

	return EBTNodeResult::Aborted;
}
