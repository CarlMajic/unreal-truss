#include "EventDirectorActor.h"

#include "DrawDebugHelpers.h"
#include "EventEntranceActor.h"
#include "EventNPCActor.h"
#include "Kismet/GameplayStatics.h"

AEventDirectorActor::AEventDirectorActor()
{
	PrimaryActorTick.bCanEverTick = true;

	// Default phase sequence — designers override values in the Details panel
	PhaseConfigs = {
		{ EEventPhase::DoorsClosed,      0.0f,  false, nullptr, { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f } },
		{ EEventPhase::Arriving,         0.0f,  false, nullptr, { 0.6f, 0.4f, 0.2f, 0.0f, 0.0f } },
		{ EEventPhase::CocktailHour,     0.0f,  false, nullptr, { 0.8f, 0.9f, 0.3f, 0.1f, 0.0f } },
		{ EEventPhase::SeatingAnnounced, 30.0f, true,  nullptr, { 0.1f, 0.1f, 1.0f, 0.0f, 0.0f } },
		{ EEventPhase::ProgramActive,    0.0f,  false, nullptr, { 0.0f, 0.0f, 0.0f, 1.0f, 0.0f } },
		{ EEventPhase::Applause,         15.0f, true,  nullptr, { 0.0f, 0.5f, 0.0f, 0.8f, 0.0f } },
		{ EEventPhase::PostShow,         0.0f,  false, nullptr, { 0.7f, 0.9f, 0.2f, 0.1f, 0.0f } },
		{ EEventPhase::Exiting,          0.0f,  false, nullptr, { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f } }
	};
}

void AEventDirectorActor::BeginPlay()
{
	Super::BeginPlay();
	SetPhase(StartingPhase);
}

void AEventDirectorActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	DrawProblemLaneMarker();

	const FEventPhaseConfig* Config = FindPhaseConfig(CurrentPhase);
	if (!Config || !Config->bAutoAdvance || Config->DurationSeconds <= 0.0f)
	{
		return;
	}

	PhaseElapsedSeconds += DeltaTime;
	if (PhaseElapsedSeconds >= Config->DurationSeconds)
	{
		AdvancePhase();
	}
}

void AEventDirectorActor::DrawProblemLaneMarker() const
{
	UWorld* World = GetWorld();
	if (!bDrawProblemLaneMarker || !World)
	{
		return;
	}

	const float MinY = FMath::Min(ProblemLaneMinY, ProblemLaneMaxY);
	const float MaxY = FMath::Max(ProblemLaneMinY, ProblemLaneMaxY);
	const float BaseZ = 5.0f;
	const float TopZ = FMath::Max(BaseZ + 50.0f, ProblemLaneMarkerHeight);
	const FColor MarkerColor = FColor::Magenta;
	const float Duration = 0.1f;
	const float Thickness = 8.0f;

	DrawDebugLine(
		World,
		FVector(ProblemLaneX, MinY, BaseZ),
		FVector(ProblemLaneX, MaxY, BaseZ),
		MarkerColor,
		false,
		Duration,
		0,
		Thickness);

	for (float Y = MinY; Y <= MaxY; Y += 500.0f)
	{
		DrawDebugLine(
			World,
			FVector(ProblemLaneX, Y, BaseZ),
			FVector(ProblemLaneX, Y, TopZ),
			MarkerColor,
			false,
			Duration,
			0,
			Thickness);
	}

	DrawDebugString(
		World,
		FVector(ProblemLaneX, MaxY, TopZ + 80.0f),
		FString::Printf(TEXT("X=%.0f movement trouble lane"), ProblemLaneX),
		nullptr,
		MarkerColor,
		Duration,
		true);
}

void AEventDirectorActor::AdvancePhase()
{
	const uint8 NextIndex = static_cast<uint8>(CurrentPhase) + 1;
	const uint8 MaxPhase = static_cast<uint8>(EEventPhase::Exiting);

	if (NextIndex <= MaxPhase)
	{
		SetPhase(static_cast<EEventPhase>(NextIndex));
	}
}

void AEventDirectorActor::SetPhase(EEventPhase NewPhase)
{
	if (NewPhase == CurrentPhase && HasActorBegunPlay())
	{
		return;
	}

	const EEventPhase PreviousPhase = CurrentPhase;
	CurrentPhase = NewPhase;
	PhaseElapsedSeconds = 0.0f;

	const FEventPhaseConfig* Config = FindPhaseConfig(NewPhase);
	if (Config && Config->AnnouncementSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Config->AnnouncementSound, GetActorLocation());
	}

	BroadcastPhaseToNPCs(NewPhase);
	OnPhaseChanged.Broadcast(PreviousPhase, NewPhase);
	OnPhaseChangedEvent(PreviousPhase, NewPhase);
}

FEventBehaviorWeights AEventDirectorActor::GetCurrentBehaviorWeights() const
{
	const FEventPhaseConfig* Config = FindPhaseConfig(CurrentPhase);
	return Config ? Config->BehaviorWeights : FEventBehaviorWeights{};
}

void AEventDirectorActor::RegisterEntrance(AEventEntranceActor* Entrance)
{
	if (Entrance)
	{
		RegisteredEntrances.AddUnique(Entrance);
	}
}

void AEventDirectorActor::RegisterNPC(AEventNPCActor* NPC)
{
	if (NPC)
	{
		RegisteredNPCs.AddUnique(NPC);
	}
}

void AEventDirectorActor::UnregisterNPC(AEventNPCActor* NPC)
{
	RegisteredNPCs.Remove(NPC);
}

const FEventPhaseConfig* AEventDirectorActor::FindPhaseConfig(EEventPhase Phase) const
{
	for (const FEventPhaseConfig& Config : PhaseConfigs)
	{
		if (Config.Phase == Phase)
		{
			return &Config;
		}
	}
	return nullptr;
}

void AEventDirectorActor::BroadcastPhaseToNPCs(EEventPhase NewPhase)
{
	for (TObjectPtr<AEventNPCActor>& NPC : RegisteredNPCs)
	{
		if (NPC)
		{
			NPC->OnEventPhaseChanged(NewPhase);
		}
	}
}
