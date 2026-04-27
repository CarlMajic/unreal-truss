#include "BuildManagerComponent.h"

#include "BuildPreviewActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TrussStructureActor.h"

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

	AActor* SpawnedActor = SpawnBuildActor(CurrentPlacementTransform);
	Result.bSuccess = SpawnedActor != nullptr;
	Result.SpawnedActor = SpawnedActor;
	return Result;
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
	return SelectedBuildItem ? SelectedBuildItem->BuildActorClass : nullptr;
}
