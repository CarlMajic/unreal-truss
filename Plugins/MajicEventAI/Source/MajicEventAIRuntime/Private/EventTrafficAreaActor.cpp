#include "EventTrafficAreaActor.h"

#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "NavAreas/NavArea_Null.h"
#include "NavModifierComponent.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"

namespace
{
	void AddUniqueAreaName(TArray<FString>& AreaNames, const FString& AreaName)
	{
		AreaNames.AddUnique(AreaName);
	}

	bool IsLocationAllowedEndpointForNoThroughArea(const AEventTrafficAreaActor& Area, const FVector& FromLocation, const FVector& ToLocation)
	{
		return Area.ContainsWorldLocation(FromLocation) || Area.ContainsWorldLocation(ToLocation);
	}

	float ScoreTrafficLocation(UWorld* World, const FVector& WorldLocation, const FVector* RouteStart, const FVector* RouteEnd, TArray<FString>* OutAreaNames, TArray<FString>* OutNoThroughAreaNames)
	{
		if (!World)
		{
			return 0.0f;
		}

		float Score = 0.0f;
		for (TActorIterator<AEventTrafficAreaActor> It(World); It; ++It)
		{
			const AEventTrafficAreaActor* Area = *It;
			if (!Area || !Area->bEnabled || !Area->ContainsWorldLocation(WorldLocation))
			{
				continue;
			}

			if (OutAreaNames)
			{
				AddUniqueAreaName(*OutAreaNames, Area->GetName());
			}

			switch (Area->AreaType)
			{
			case EEventTrafficAreaType::PreferredPathway:
				Score += Area->Priority;
				break;
			case EEventTrafficAreaType::CrowdedSlowZone:
				Score -= Area->Priority;
				break;
			case EEventTrafficAreaType::NoThroughZone:
				if (!RouteStart || !RouteEnd || !IsLocationAllowedEndpointForNoThroughArea(*Area, *RouteStart, *RouteEnd))
				{
					Score -= Area->Priority * 8.0f;
					if (OutNoThroughAreaNames)
					{
						AddUniqueAreaName(*OutNoThroughAreaNames, Area->GetName());
					}
				}
				break;
			default:
				break;
			}
		}

		return Score;
	}
}

UEventNavArea_PreferredPathway::UEventNavArea_PreferredPathway(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DefaultCost = 0.25f;
	FixedAreaEnteringCost = 0.0f;
	DrawColor = FColor::Cyan;
}

UEventNavArea_CrowdedSlowZone::UEventNavArea_CrowdedSlowZone(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DefaultCost = 12.0f;
	FixedAreaEnteringCost = 0.0f;
	DrawColor = FColor::Orange;
}

AEventTrafficAreaActor::AEventTrafficAreaActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	TrafficBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("TrafficBounds"));
	TrafficBounds->SetupAttachment(SceneRoot);
	TrafficBounds->SetBoxExtent(FVector(500.0f, 150.0f, 160.0f));
	TrafficBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TrafficBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	TrafficBounds->SetGenerateOverlapEvents(false);
	TrafficBounds->ShapeColor = FColor::Cyan;
	TrafficBounds->SetHiddenInGame(true);
	TrafficBounds->SetCanEverAffectNavigation(true);

	NavModifier = CreateDefaultSubobject<UNavModifierComponent>(TEXT("NavModifier"));
	PreferredPathwayNavAreaClass = UEventNavArea_PreferredPathway::StaticClass();
	CrowdedSlowZoneNavAreaClass = UEventNavArea_CrowdedSlowZone::StaticClass();
	NoThroughZoneNavAreaClass = UNavArea_Null::StaticClass();
	UpdateNavigationModifier();
}

void AEventTrafficAreaActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateNavigationModifier();
}

#if WITH_EDITOR
void AEventTrafficAreaActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UpdateNavigationModifier();
}
#endif

void AEventTrafficAreaActor::UpdateNavigationModifier()
{
	if (!NavModifier || !TrafficBounds)
	{
		return;
	}

	TSubclassOf<UNavArea> AreaClass = nullptr;
	if (bEnabled && bAffectNavigationMesh)
	{
		switch (AreaType)
		{
		case EEventTrafficAreaType::PreferredPathway:
			AreaClass = PreferredPathwayNavAreaClass;
			break;
		case EEventTrafficAreaType::CrowdedSlowZone:
			AreaClass = CrowdedSlowZoneNavAreaClass;
			break;
		case EEventTrafficAreaType::NoThroughZone:
			AreaClass = NoThroughZoneNavAreaClass;
			break;
		default:
			break;
		}
	}

	TrafficBounds->SetCanEverAffectNavigation(AreaClass != nullptr);
	TrafficBounds->SetCollisionEnabled(AreaClass != nullptr ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	TrafficBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	NavModifier->SetAreaClass(AreaClass);
	NavModifier->SetNavigationRelevancy(AreaClass != nullptr);
}

bool AEventTrafficAreaActor::ContainsWorldLocation(const FVector& WorldLocation) const
{
	if (!TrafficBounds)
	{
		return false;
	}

	const FTransform ComponentTransform = TrafficBounds->GetComponentTransform();
	const FVector LocalLocation = ComponentTransform.InverseTransformPosition(WorldLocation);
	return FMath::Abs(LocalLocation.X) <= TrafficBounds->GetUnscaledBoxExtent().X &&
		FMath::Abs(LocalLocation.Y) <= TrafficBounds->GetUnscaledBoxExtent().Y &&
		FMath::Abs(LocalLocation.Z) <= TrafficBounds->GetUnscaledBoxExtent().Z;
}

FVector AEventTrafficAreaActor::GetClosestWorldLocation(const FVector& WorldLocation) const
{
	if (!TrafficBounds)
	{
		return GetActorLocation();
	}

	const FTransform ComponentTransform = TrafficBounds->GetComponentTransform();
	const FVector LocalLocation = ComponentTransform.InverseTransformPosition(WorldLocation);
	const FVector Extent = TrafficBounds->GetUnscaledBoxExtent();
	const FVector ClampedLocalLocation(
		FMath::Clamp(LocalLocation.X, -Extent.X, Extent.X),
		FMath::Clamp(LocalLocation.Y, -Extent.Y, Extent.Y),
		FMath::Clamp(LocalLocation.Z, -Extent.Z, Extent.Z));
	return ComponentTransform.TransformPosition(ClampedLocalLocation);
}

float AEventTrafficAreaActor::CalculateTrafficAreaScore(UWorld* World, const FVector& WorldLocation)
{
	if (!World)
	{
		return 0.0f;
	}

	float Score = 0.0f;
	for (TActorIterator<AEventTrafficAreaActor> It(World); It; ++It)
	{
		const AEventTrafficAreaActor* Area = *It;
		if (!Area || !Area->bEnabled || !Area->ContainsWorldLocation(WorldLocation))
		{
			continue;
		}

		float SignedPriority = 0.0f;
		switch (Area->AreaType)
		{
		case EEventTrafficAreaType::PreferredPathway:
			SignedPriority = Area->Priority;
			break;
		case EEventTrafficAreaType::CrowdedSlowZone:
			SignedPriority = -Area->Priority;
			break;
		case EEventTrafficAreaType::NoThroughZone:
			SignedPriority = -Area->Priority * 8.0f;
			break;
		default:
			break;
		}
		Score += SignedPriority;
	}
	return Score;
}

float AEventTrafficAreaActor::CalculateTrafficPathScore(UWorld* World, const FVector& FromLocation, const FVector& ToLocation, FString* OutAreaNames, FString* OutNoThroughAreaNames)
{
	if (OutAreaNames)
	{
		OutAreaNames->Reset();
	}
	if (OutNoThroughAreaNames)
	{
		OutNoThroughAreaNames->Reset();
	}
	if (!World)
	{
		return 0.0f;
	}

	TArray<FVector> PathPoints;
	if (UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(World, FromLocation, ToLocation))
	{
		if (Path->IsValid() && !Path->IsPartial() && Path->PathPoints.Num() >= 2)
		{
			PathPoints = Path->PathPoints;
		}
	}
	if (PathPoints.Num() < 2)
	{
		PathPoints.Add(FromLocation);
		PathPoints.Add(ToLocation);
	}

	TArray<FString> AreaNames;
	TArray<FString> NoThroughAreaNames;
	float Score = 0.0f;
	int32 SampleCount = 0;
	constexpr float SampleSpacingCm = 250.0f;
	for (int32 PointIndex = 0; PointIndex < PathPoints.Num() - 1; ++PointIndex)
	{
		const FVector SegmentStart = PathPoints[PointIndex];
		const FVector SegmentEnd = PathPoints[PointIndex + 1];
		const float SegmentLength = FVector::Dist2D(SegmentStart, SegmentEnd);
		const int32 Steps = FMath::Max(1, FMath::CeilToInt(SegmentLength / SampleSpacingCm));
		for (int32 Step = 0; Step <= Steps; ++Step)
		{
			const float Alpha = static_cast<float>(Step) / static_cast<float>(Steps);
			const FVector SampleLocation = FMath::Lerp(SegmentStart, SegmentEnd, Alpha);
			Score += ScoreTrafficLocation(World, SampleLocation, &FromLocation, &ToLocation, &AreaNames, &NoThroughAreaNames);
			++SampleCount;
		}
	}

	if (SampleCount > 0)
	{
		Score /= static_cast<float>(SampleCount);
	}
	if (OutAreaNames)
	{
		*OutAreaNames = FString::Join(AreaNames, TEXT("|"));
	}
	if (OutNoThroughAreaNames)
	{
		*OutNoThroughAreaNames = FString::Join(NoThroughAreaNames, TEXT("|"));
	}
	return Score;
}

bool AEventTrafficAreaActor::IsRouteBlockedByNoThroughZone(UWorld* World, const FVector& FromLocation, const FVector& ToLocation, FString* OutAreaNames)
{
	FString AreaNames;
	CalculateTrafficPathScore(World, FromLocation, ToLocation, nullptr, &AreaNames);
	if (OutAreaNames)
	{
		*OutAreaNames = AreaNames;
	}
	return !AreaNames.IsEmpty();
}

bool AEventTrafficAreaActor::FindPreferredTrafficWaypoint(UWorld* World, const FVector& FromLocation, const FVector& ToLocation, float MinRouteDistance, float MaxDetourRatio, FVector& OutWaypoint, FEventTrafficRouteResult* OutResult)
{
	if (OutResult)
	{
		*OutResult = FEventTrafficRouteResult();
	}

	if (!World || FVector::Dist2D(FromLocation, ToLocation) < MinRouteDistance)
	{
		return false;
	}

	UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavSystem)
	{
		return false;
	}

	float DirectPathLength = FVector::Dist2D(FromLocation, ToLocation);
	if (UNavigationPath* DirectPath = UNavigationSystemV1::FindPathToLocationSynchronously(World, FromLocation, ToLocation))
	{
		if (DirectPath->IsValid() && !DirectPath->IsPartial())
		{
			DirectPathLength = FMath::Max(DirectPath->GetPathLength(), 1.0f);
		}
	}
	if (OutResult)
	{
		OutResult->DirectPathLength = DirectPathLength;
	}

	const FVector Midpoint = (FromLocation + ToLocation) * 0.5f;
	float BestScore = -FLT_MAX;
	FVector BestWaypoint = FVector::ZeroVector;
	FString BestAreaName;
	float BestRouteLength = 0.0f;

	for (TActorIterator<AEventTrafficAreaActor> It(World); It; ++It)
	{
		const AEventTrafficAreaActor* Area = *It;
		if (!Area || !Area->bEnabled || Area->AreaType != EEventTrafficAreaType::PreferredPathway)
		{
			continue;
		}
		if (OutResult)
		{
			++OutResult->ConsideredAreas;
		}

		FVector Candidate = Area->GetClosestWorldLocation(Midpoint);
		FNavLocation ProjectedCandidate;
		if (!NavSystem->ProjectPointToNavigation(Candidate, ProjectedCandidate, FVector(300.0f, 300.0f, 300.0f)))
		{
			if (OutResult)
			{
				++OutResult->RejectedProjectionCount;
			}
			continue;
		}

		float FirstLegLength = FVector::Dist2D(FromLocation, ProjectedCandidate.Location);
		float SecondLegLength = FVector::Dist2D(ProjectedCandidate.Location, ToLocation);
		FString FirstLegNoThroughAreas;
		FString SecondLegNoThroughAreas;
		const bool bFirstLegBlockedByNoThrough = IsRouteBlockedByNoThroughZone(World, FromLocation, ProjectedCandidate.Location, &FirstLegNoThroughAreas);
		const bool bSecondLegBlockedByNoThrough = IsRouteBlockedByNoThroughZone(World, ProjectedCandidate.Location, ToLocation, &SecondLegNoThroughAreas);
		if (bFirstLegBlockedByNoThrough || bSecondLegBlockedByNoThrough)
		{
			if (OutResult)
			{
				++OutResult->RejectedNoThroughCount;
			}
			continue;
		}
		UNavigationPath* FirstLeg = UNavigationSystemV1::FindPathToLocationSynchronously(World, FromLocation, ProjectedCandidate.Location);
		UNavigationPath* SecondLeg = UNavigationSystemV1::FindPathToLocationSynchronously(World, ProjectedCandidate.Location, ToLocation);
		if (!FirstLeg || !FirstLeg->IsValid() || FirstLeg->IsPartial() || !SecondLeg || !SecondLeg->IsValid() || SecondLeg->IsPartial())
		{
			if (OutResult)
			{
				++OutResult->RejectedPathCount;
			}
			continue;
		}

		FirstLegLength = FirstLeg->GetPathLength();
		SecondLegLength = SecondLeg->GetPathLength();
		const float RouteLength = FirstLegLength + SecondLegLength;
		if (MaxDetourRatio > 1.0f && RouteLength > DirectPathLength * MaxDetourRatio)
		{
			if (OutResult)
			{
				++OutResult->RejectedDetourCount;
			}
			continue;
		}

		const float MidpointDistance = FVector::Dist2D(ProjectedCandidate.Location, Midpoint);
		const float Score = Area->Priority * 10000.0f - RouteLength - MidpointDistance * 0.5f;
		if (Score > BestScore)
		{
			BestScore = Score;
			BestWaypoint = ProjectedCandidate.Location;
			BestAreaName = Area->GetName();
			BestRouteLength = RouteLength;
		}
	}

	if (BestScore <= -FLT_MAX)
	{
		return false;
	}

	OutWaypoint = BestWaypoint;
	if (OutResult)
	{
		OutResult->Waypoint = BestWaypoint;
		OutResult->AreaName = BestAreaName;
		OutResult->RouteLength = BestRouteLength;
		OutResult->DetourRatio = DirectPathLength > 1.0f ? BestRouteLength / DirectPathLength : 0.0f;
	}
	return true;
}

void AEventTrafficAreaActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bDrawDebugInEditor || !GetWorld() || GetWorld()->IsGameWorld() || !TrafficBounds)
	{
		return;
	}

	const FColor Color = AreaType == EEventTrafficAreaType::PreferredPathway
		? FColor::Cyan
		: (AreaType == EEventTrafficAreaType::CrowdedSlowZone ? FColor::Orange : FColor::Red);
	DrawDebugBox(GetWorld(), TrafficBounds->GetComponentLocation(), TrafficBounds->GetScaledBoxExtent(), TrafficBounds->GetComponentQuat(), Color, false, 0.1f, 0, 3.0f);
}
