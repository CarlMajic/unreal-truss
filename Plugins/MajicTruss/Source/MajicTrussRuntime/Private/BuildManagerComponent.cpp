#include "BuildManagerComponent.h"

#include "AudioGroundLineArrayActor.h"
#include "AudioGroundSpeakerActor.h"
#include "BuildPreviewActor.h"
#include "DrapeRunActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "LoungeLayoutActor.h"
#include "MBPWallActor.h"
#include "StageDeckActor.h"
#include "TrussStructureActor.h"
#include "VideoPlacementActor.h"
#include "VideoWallActor.h"
#include "ProjectionScreenActor.h"

UBuildManagerComponent::UBuildManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UBuildManagerComponent::EnterBuildMode()
{
	bBuildModeActive = true;
	return EnsurePreviewActor();
}

void UBuildManagerComponent::ExitBuildMode()
{
	bBuildModeActive = false;
	bHasValidPlacement = false;
	DestroyPreviewActor();
}

bool UBuildManagerComponent::SetSelectedBuildItem(UBuildItemDataAsset* BuildItem)
{
	SelectedBuildItem = BuildItem;
	ActiveTrussDefinition = BuildItem ? BuildItem->DefaultTrussDefinition : FTrussBuildDefinition();
	ActiveMBPWallDefinition = BuildItem ? BuildItem->DefaultMBPWallDefinition : FMBPWallDefinition();
	ActiveStageDeckDefinition = BuildItem ? BuildItem->DefaultStageDeckDefinition : FStageDeckBuildDefinition();
	ActiveDrapeRunDefinition = BuildItem ? BuildItem->DefaultDrapeRunDefinition : FDrapeRunBuildDefinition();
	ActiveVideoPlacementDefinition = BuildItem ? BuildItem->DefaultVideoPlacementDefinition : FVideoPlacementBuildDefinition();
	ActiveVideoWallDefinition = BuildItem ? BuildItem->DefaultVideoWallDefinition : FVideoWallBuildDefinition();
	ActiveProjectionScreenDefinition = BuildItem ? BuildItem->DefaultProjectionScreenDefinition : FProjectionScreenBuildDefinition();
	ActiveAudioPlacementDefinition = BuildItem ? BuildItem->DefaultAudioPlacementDefinition : FAudioPlacementBuildDefinition();
	ActiveLoungeLayoutDefinition = BuildItem ? BuildItem->DefaultLoungeLayoutDefinition : FLoungeLayoutBuildDefinition();
	CurrentYawDegrees = 0.0f;

	if (!bBuildModeActive)
	{
		return true;
	}

	if (!EnsurePreviewActor())
	{
		return false;
	}

	if (ActivePreviewActor)
	{
		ActivePreviewActor->SetPreviewActorClass(ResolveBuildActorClass());
		ApplyCurrentSettingsToActor(ActivePreviewActor->GetPreviewActor());
	}

	return true;
}

void UBuildManagerComponent::SetActiveTrussDefinition(const FTrussBuildDefinition& Definition)
{
	ActiveTrussDefinition = Definition;

	if (ActivePreviewActor)
	{
		ApplyCurrentSettingsToActor(ActivePreviewActor->GetPreviewActor());
	}
}

void UBuildManagerComponent::SetActiveMBPWallDefinition(const FMBPWallDefinition& Definition)
{
	ActiveMBPWallDefinition = Definition;

	if (ActivePreviewActor)
	{
		ApplyCurrentSettingsToActor(ActivePreviewActor->GetPreviewActor());
	}
}

void UBuildManagerComponent::SetActiveStageDeckDefinition(const FStageDeckBuildDefinition& Definition)
{
	ActiveStageDeckDefinition = Definition;

	if (ActivePreviewActor)
	{
		ApplyCurrentSettingsToActor(ActivePreviewActor->GetPreviewActor());
	}
}

void UBuildManagerComponent::SetActiveDrapeRunDefinition(const FDrapeRunBuildDefinition& Definition)
{
	ActiveDrapeRunDefinition = Definition;

	if (ActivePreviewActor)
	{
		ApplyCurrentSettingsToActor(ActivePreviewActor->GetPreviewActor());
	}
}

void UBuildManagerComponent::SetActiveVideoPlacementDefinition(const FVideoPlacementBuildDefinition& Definition)
{
	ActiveVideoPlacementDefinition = Definition;

	if (ActivePreviewActor)
	{
		ApplyCurrentSettingsToActor(ActivePreviewActor->GetPreviewActor());
	}
}

void UBuildManagerComponent::SetActiveVideoWallDefinition(const FVideoWallBuildDefinition& Definition)
{
	ActiveVideoWallDefinition = Definition;

	if (ActivePreviewActor)
	{
		ApplyCurrentSettingsToActor(ActivePreviewActor->GetPreviewActor());
	}
}

void UBuildManagerComponent::SetActiveProjectionScreenDefinition(const FProjectionScreenBuildDefinition& Definition)
{
	ActiveProjectionScreenDefinition = Definition;

	if (ActivePreviewActor)
	{
		ApplyCurrentSettingsToActor(ActivePreviewActor->GetPreviewActor());
	}
}

void UBuildManagerComponent::SetActiveAudioPlacementDefinition(const FAudioPlacementBuildDefinition& Definition)
{
	ActiveAudioPlacementDefinition = Definition;

	if (ActivePreviewActor)
	{
		ActivePreviewActor->SetPreviewActorClass(ResolveBuildActorClass());
		ApplyCurrentSettingsToActor(ActivePreviewActor->GetPreviewActor());
	}
}

void UBuildManagerComponent::SetActiveLoungeLayoutDefinition(const FLoungeLayoutBuildDefinition& Definition)
{
	ActiveLoungeLayoutDefinition = Definition;

	if (ActivePreviewActor)
	{
		ApplyCurrentSettingsToActor(ActivePreviewActor->GetPreviewActor());
	}
}

bool UBuildManagerComponent::UpdatePreviewFromPlayerView(APlayerController* PlayerController)
{
	if (!bBuildModeActive || !PlayerController || !SelectedBuildItem)
	{
		bHasValidPlacement = false;
		return false;
	}

	if (!EnsurePreviewActor())
	{
		bHasValidPlacement = false;
		return false;
	}

	FVector ViewLocation = FVector::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

	const FVector TraceEnd = ViewLocation + (ViewRotation.Vector() * TraceDistanceCm);
	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(BuildPlacementTrace), false, GetOwner());
	QueryParams.AddIgnoredActor(ActivePreviewActor);

	UWorld* World = GetWorld();
	if (!World || !World->LineTraceSingleByChannel(HitResult, ViewLocation, TraceEnd, PlacementTraceChannel, QueryParams))
	{
		bHasValidPlacement = false;
		if (ActivePreviewActor)
		{
			ActivePreviewActor->SetActorHiddenInGame(true);
		}
		return false;
	}

	FVector PlacementLocation = HitResult.ImpactPoint;
	if (SelectedBuildItem->bUseGridSnap && SelectedBuildItem->GridSnapSizeCm > 0.0f)
	{
		PlacementLocation = SnapLocation(PlacementLocation, SelectedBuildItem->GridSnapSizeCm);
	}

	const FRotator PlacementRotation = MakePlacementRotation(HitResult.ImpactNormal);
	CurrentPlacementTransform = FTransform(PlacementRotation, PlacementLocation);
	bHasValidPlacement = true;

	ActivePreviewActor->SetActorHiddenInGame(false);
	ActivePreviewActor->SetActorTransform(CurrentPlacementTransform);
	ActivePreviewActor->SetPlacementValid(true);
	ApplyCurrentSettingsToActor(ActivePreviewActor->GetPreviewActor());
	return true;
}

void UBuildManagerComponent::RotatePreviewYaw(float DeltaDegrees)
{
	CurrentYawDegrees += DeltaDegrees;

	if (SelectedBuildItem && SelectedBuildItem->RotationStepDegrees > 0.0f)
	{
		CurrentYawDegrees = FMath::GridSnap(CurrentYawDegrees, SelectedBuildItem->RotationStepDegrees);
	}

	if (ActivePreviewActor && bHasValidPlacement)
	{
		CurrentPlacementTransform.SetRotation(GetCurrentPlacementRotation().Quaternion());
		ActivePreviewActor->SetActorTransform(CurrentPlacementTransform);
	}
}

FBuildPlacementResult UBuildManagerComponent::ConfirmPlacement()
{
	FBuildPlacementResult Result;

	if (!bBuildModeActive || !SelectedBuildItem || !bHasValidPlacement)
	{
		return Result;
	}

	if (EditingTrussActor && SelectedBuildItem->ItemType == EBuildItemType::TrussStructure)
	{
		EditingTrussActor->ApplyBuildDefinition(ActiveTrussDefinition, true);
		Result.bSuccess = true;
		Result.SpawnedActor = EditingTrussActor;
		return Result;
	}

	AActor* SpawnedActor = SpawnBuildActor(CurrentPlacementTransform);
	Result.bSuccess = SpawnedActor != nullptr;
	Result.SpawnedActor = SpawnedActor;
	return Result;
}

void UBuildManagerComponent::SetEditingTrussActor(ATrussStructureActor* TrussActor)
{
	EditingTrussActor = TrussActor;
	if (EditingTrussActor)
	{
		ActiveTrussDefinition = EditingTrussActor->GetBuildDefinition();
	}
}

void UBuildManagerComponent::ClearEditingTrussActor()
{
	EditingTrussActor = nullptr;
}

bool UBuildManagerComponent::IsEditingExistingActor() const
{
	return EditingTrussActor != nullptr;
}

FRotator UBuildManagerComponent::GetCurrentPlacementRotation() const
{
	FRotator Rotation = CurrentPlacementTransform.GetRotation().Rotator();
	Rotation.Yaw = CurrentYawDegrees;
	return Rotation;
}

FTransform UBuildManagerComponent::GetCurrentPlacementTransform() const
{
	return CurrentPlacementTransform;
}

void UBuildManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroyPreviewActor();
	Super::EndPlay(EndPlayReason);
}

bool UBuildManagerComponent::EnsurePreviewActor()
{
	if (ActivePreviewActor)
	{
		return true;
	}

	UWorld* World = GetWorld();
	TSubclassOf<ABuildPreviewActor> ResolvedPreviewClass = PreviewActorClass;
	if (!ResolvedPreviewClass)
	{
		ResolvedPreviewClass = ABuildPreviewActor::StaticClass();
	}
	if (!World || !ResolvedPreviewClass || !SelectedBuildItem)
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = GetOwner();
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ActivePreviewActor = World->SpawnActor<ABuildPreviewActor>(ResolvedPreviewClass, FTransform::Identity, SpawnParameters);
	if (!ActivePreviewActor)
	{
		return false;
	}

	ActivePreviewActor->SetPreviewActorClass(ResolveBuildActorClass());
	ActivePreviewActor->SetActorHiddenInGame(true);
	ApplyCurrentSettingsToActor(ActivePreviewActor->GetPreviewActor());
	return true;
}

void UBuildManagerComponent::DestroyPreviewActor()
{
	if (ActivePreviewActor)
	{
		ActivePreviewActor->Destroy();
		ActivePreviewActor = nullptr;
	}
}

void UBuildManagerComponent::ApplyCurrentSettingsToActor(AActor* Actor) const
{
	if (!Actor || !SelectedBuildItem)
	{
		return;
	}

	if (SelectedBuildItem->ItemType == EBuildItemType::TrussStructure)
	{
		if (ATrussStructureActor* TrussActor = Cast<ATrussStructureActor>(Actor))
		{
			TrussActor->bBuildOnConstruction = false;
			TrussActor->ApplyBuildDefinition(ActiveTrussDefinition, true);
		}
		return;
	}

	if (SelectedBuildItem->ItemType == EBuildItemType::MBPWall)
	{
		if (AMBPWallActor* MBPWallActor = Cast<AMBPWallActor>(Actor))
		{
			MBPWallActor->bBuildOnConstruction = false;
			MBPWallActor->ApplyWallDefinition(ActiveMBPWallDefinition, true);
		}
		return;
	}

	if (SelectedBuildItem->ItemType == EBuildItemType::StageDeck)
	{
		if (AStageDeckActor* StageDeckActor = Cast<AStageDeckActor>(Actor))
		{
			StageDeckActor->bBuildOnConstruction = false;
			StageDeckActor->ApplyBuildDefinition(ActiveStageDeckDefinition, true);
		}
		return;
	}

	if (SelectedBuildItem->ItemType == EBuildItemType::DrapeRun)
	{
		if (ADrapeRunActor* DrapeRunActor = Cast<ADrapeRunActor>(Actor))
		{
			DrapeRunActor->bBuildOnConstruction = false;
			DrapeRunActor->ApplyBuildDefinition(ActiveDrapeRunDefinition, true);
		}
		return;
	}

	if (SelectedBuildItem->ItemType == EBuildItemType::VideoPlacement)
	{
		if (AVideoPlacementActor* VideoPlacementActor = Cast<AVideoPlacementActor>(Actor))
		{
			VideoPlacementActor->bBuildOnConstruction = false;
			VideoPlacementActor->ApplyBuildDefinition(ActiveVideoPlacementDefinition, true);
		}
		return;
	}

	if (SelectedBuildItem->ItemType == EBuildItemType::VideoWall)
	{
		if (AVideoWallActor* VideoWallActor = Cast<AVideoWallActor>(Actor))
		{
			VideoWallActor->bBuildOnConstruction = false;
			VideoWallActor->ApplyBuildDefinition(ActiveVideoWallDefinition, true);
		}
		return;
	}

	if (SelectedBuildItem->ItemType == EBuildItemType::ProjectionScreen)
	{
		if (AProjectionScreenActor* ProjectionScreenActor = Cast<AProjectionScreenActor>(Actor))
		{
			ProjectionScreenActor->bBuildOnConstruction = false;
			ProjectionScreenActor->ApplyBuildDefinition(ActiveProjectionScreenDefinition, true);
		}
		return;
	}

	if (SelectedBuildItem->ItemType == EBuildItemType::AudioPlacement)
	{
		if (AAudioGroundSpeakerActor* GroundSpeakerActor = Cast<AAudioGroundSpeakerActor>(Actor))
		{
			GroundSpeakerActor->bBuildOnConstruction = false;
			GroundSpeakerActor->ApplyBuildDefinition(ActiveAudioPlacementDefinition.GroundSpeakerDefinition, true);
		}
		else if (AAudioGroundLineArrayActor* LineArrayActor = Cast<AAudioGroundLineArrayActor>(Actor))
		{
			LineArrayActor->bBuildOnConstruction = false;
			LineArrayActor->ApplyBuildDefinition(ActiveAudioPlacementDefinition.GroundLineArrayDefinition, true);
		}
	}

	if (SelectedBuildItem->ItemType == EBuildItemType::LoungeLayout)
	{
		if (ALoungeLayoutActor* LoungeActor = Cast<ALoungeLayoutActor>(Actor))
		{
			LoungeActor->bBuildOnConstruction = false;
			LoungeActor->ApplyBuildDefinition(ActiveLoungeLayoutDefinition, true);
		}
	}
}

AActor* UBuildManagerComponent::SpawnBuildActor(const FTransform& SpawnTransform) const
{
	UWorld* World = GetWorld();
	TSubclassOf<AActor> BuildClass = ResolveBuildActorClass();
	if (!World || !BuildClass)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = GetOwner();
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AActor* SpawnedActor = World->SpawnActor<AActor>(BuildClass, SpawnTransform, SpawnParameters);
	ApplyCurrentSettingsToActor(SpawnedActor);
	return SpawnedActor;
}

FVector UBuildManagerComponent::SnapLocation(const FVector& Location, float GridSizeCm) const
{
	if (GridSizeCm <= 0.0f)
	{
		return Location;
	}

	return FVector(
		FMath::GridSnap(Location.X, GridSizeCm),
		FMath::GridSnap(Location.Y, GridSizeCm),
		FMath::GridSnap(Location.Z, GridSizeCm)
	);
}

FRotator UBuildManagerComponent::MakePlacementRotation(const FVector& SurfaceNormal) const
{
	FRotator Rotation = SelectedBuildItem && SelectedBuildItem->bAlignToSurfaceNormal
		? FRotationMatrix::MakeFromZ(SurfaceNormal).Rotator()
		: FRotator::ZeroRotator;

	Rotation.Yaw = CurrentYawDegrees;
	return Rotation;
}

TSubclassOf<AActor> UBuildManagerComponent::ResolveBuildActorClass() const
{
	if (SelectedBuildItem && SelectedBuildItem->ItemType == EBuildItemType::AudioPlacement)
	{
		return ActiveAudioPlacementDefinition.PlacementType == EAudioPlacementRuntimeType::GroundLineArray
			? AAudioGroundLineArrayActor::StaticClass()
			: AAudioGroundSpeakerActor::StaticClass();
	}

	return SelectedBuildItem ? SelectedBuildItem->BuildActorClass : nullptr;
}
