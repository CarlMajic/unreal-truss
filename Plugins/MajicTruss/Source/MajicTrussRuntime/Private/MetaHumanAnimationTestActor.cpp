#include "MetaHumanAnimationTestActor.h"

#include "Animation/AnimationAsset.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

AMetaHumanAnimationTestActor::AMetaHumanAnimationTestActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	SelectionBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("SelectionBounds"));
	SelectionBounds->SetupAttachment(SceneRoot);
	SelectionBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SelectionBounds->SetCollisionObjectType(ECC_WorldDynamic);
	SelectionBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	SelectionBounds->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SelectionBounds->SetGenerateOverlapEvents(false);
	SelectionBounds->SetHiddenInGame(true);
	SelectionBounds->SetBoxExtent(FVector(45.0f, 45.0f, 100.0f));
	SelectionBounds->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));
	SelectionBounds->ShapeColor = FColor::Cyan;

	GuestBlueprintClass = TSoftClassPtr<AActor>(FSoftObjectPath(TEXT("/Game/MetaHumans/MH_Guest_01/BP_MH_Guest_01.BP_MH_Guest_01_C")));
	GuestAnimation = TSoftObjectPtr<UAnimationAsset>(FSoftObjectPath(TEXT("/Game/EventAI/MetaHumanAnimations/ArmsOnLegs_Sit.ArmsOnLegs_Sit")));
	LastStatusMessage = TEXT("Not rebuilt yet");
}

void AMetaHumanAnimationTestActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (bBuildOnConstruction)
	{
		RebuildGuest();
	}
}

void AMetaHumanAnimationTestActor::RebuildGuest()
{
	SetStatus(TEXT("Child-component path is disabled. Use Spawn Standalone Guest."), false, false, NAME_None);
}

void AMetaHumanAnimationTestActor::SpawnStandaloneGuest()
{
	SpawnStandaloneGuestInternal(false);
}

void AMetaHumanAnimationTestActor::SpawnStandaloneAnimatedGuest()
{
	SpawnStandaloneGuestInternal(true);
}

AActor* AMetaHumanAnimationTestActor::SpawnStandaloneGuestInternal(bool bApplyAnimation)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		SetStatus(TEXT("No world exists for spawning."), false, false, NAME_None);
		return nullptr;
	}

	UClass* LoadedGuestClass = GuestBlueprintClass.LoadSynchronous();
	if (!LoadedGuestClass)
	{
		SetStatus(TEXT("GuestBlueprintClass did not load."), false, false, NAME_None);
		return nullptr;
	}

	if (bReplacePreviousStandaloneGuest && LastSpawnedStandaloneGuest)
	{
		LastSpawnedStandaloneGuest->Destroy();
		LastSpawnedStandaloneGuest = nullptr;
	}

	const FTransform SpawnTransform(GuestRelativeRotation, GetActorLocation() + GuestRelativeLocation, GuestRelativeScale);
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = nullptr;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParameters.ObjectFlags |= RF_Transactional;

	AActor* SpawnedGuest = World->SpawnActor<AActor>(LoadedGuestClass, SpawnTransform, SpawnParameters);
	if (!SpawnedGuest)
	{
		SetStatus(TEXT("Standalone guest spawn failed."), false, false, NAME_None);
		return nullptr;
	}

	if (bApplyAnimation)
	{
		ApplyAnimationToGuestActor(SpawnedGuest);
	}

#if WITH_EDITOR
	SpawnedGuest->SetActorLabel(TEXT("Standalone_MH_Guest_01"));
	if (GEditor)
	{
		GEditor->SelectNone(false, true);
		GEditor->SelectActor(SpawnedGuest, true, true);
		GEditor->NoteSelectionChange();
	}
#endif

	LastSpawnedStandaloneGuest = SpawnedGuest;
	if (!bApplyAnimation)
	{
		SetStatus(TEXT("Spawned and selected standalone guest actor. No animation or leader-pose changes were applied."), true, false, NAME_None);
	}

	if (bDestroySpawnerAfterSpawn)
	{
		Destroy();
	}

	return SpawnedGuest;
}

void AMetaHumanAnimationTestActor::ApplyGuestAnimation()
{
	AActor* GuestActor = LastSpawnedStandaloneGuest.Get();
	if (!GuestActor)
	{
		SetStatus(TEXT("No standalone guest exists. Use Spawn Standalone Guest first."), false, false, NAME_None);
		return;
	}

	ApplyAnimationToGuestActor(GuestActor);
}

bool AMetaHumanAnimationTestActor::ApplyAnimationToGuestActor(AActor* GuestActor)
{
	if (!GuestActor)
	{
		SetStatus(TEXT("No guest actor was supplied for animation."), false, false, NAME_None);
		return false;
	}

	UAnimationAsset* LoadedAnimation = GuestAnimation.LoadSynchronous();
	if (!LoadedAnimation)
	{
		SetStatus(TEXT("Animation did not load. Check GuestAnimation."), true, false, NAME_None);
		return false;
	}

	USkeletalMeshComponent* FallbackMeshComponent = nullptr;
	USkeletalMeshComponent* BodyComponent = FindBodyComponent(GuestActor, FallbackMeshComponent);
	USkeletalMeshComponent* MeshToAnimate = BodyComponent ? BodyComponent : FallbackMeshComponent;
	const int32 SkeletalComponentCount = CountSkeletalMeshComponents(GuestActor);
	if (!MeshToAnimate)
	{
		SetStatus(TEXT("No skeletal mesh component with a mesh was found on the child actor."), true, false, NAME_None, SkeletalComponentCount);
		return false;
	}

	MeshToAnimate->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	MeshToAnimate->SetAnimation(LoadedAnimation);
	MeshToAnimate->Play(true);
	MeshToAnimate->SetPlayRate(1.0f);

	// Make Face a leader-pose follower of Body so the head sits at the correct position.
	// Clear Face's ABP so it doesn't fight with the leader pose (neutral expression for previs).
	// Teeth/Eyes then follow Face so they stay anchored at jaw/eye bones.
	USkeletalMeshComponent* FaceComponent = FindFaceComponent(GuestActor);
	if (FaceComponent && BodyComponent)
	{
		SyncFaceComponentToAnimation(GuestActor, FaceComponent, BodyComponent);
	}

	const bool bAnimatedBody = MeshToAnimate == BodyComponent;
	const int32 FollowerComponentsBound = bForceBodyLeaderPose && BodyComponent
		? BindFollowerComponentsToBody(GuestActor, BodyComponent)
		: 0;

	// Force LOD 0 on all skeletal components so lower LODs don't distort the pose.
	if (bForceLOD0ForPrevis)
	{
		TArray<USkeletalMeshComponent*> AllComponents;
		GuestActor->GetComponents<USkeletalMeshComponent>(AllComponents);
		for (USkeletalMeshComponent* Comp : AllComponents)
		{
			if (Comp)
			{
				Comp->SetForcedLOD(1);
			}
		}
	}

	const FString Message = FString::Printf(
		TEXT("Applied %s to %s; skeletal components=%d; followers rebound=%d; face=%s"),
		*LoadedAnimation->GetName(),
		*MeshToAnimate->GetName(),
		SkeletalComponentCount,
		FollowerComponentsBound,
		FaceComponent ? TEXT("synced") : TEXT("not found"));
	SetStatus(Message, true, bAnimatedBody, MeshToAnimate->GetFName(), SkeletalComponentCount, FollowerComponentsBound);
	return true;
}

USkeletalMeshComponent* AMetaHumanAnimationTestActor::FindBodyComponent(AActor* GuestActor, USkeletalMeshComponent*& OutFallbackMeshComponent) const
{
	OutFallbackMeshComponent = nullptr;
	if (!GuestActor)
	{
		return nullptr;
	}

	TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
	GuestActor->GetComponents<USkeletalMeshComponent>(SkeletalMeshComponents);
	for (USkeletalMeshComponent* MeshComponent : SkeletalMeshComponents)
	{
		if (!MeshComponent || !MeshComponent->GetSkeletalMeshAsset())
		{
			continue;
		}

		if (!OutFallbackMeshComponent)
		{
			OutFallbackMeshComponent = MeshComponent;
		}

		if (MeshComponent->GetName() == TEXT("Body"))
		{
			return MeshComponent;
		}
	}

	return nullptr;
}

USkeletalMeshComponent* AMetaHumanAnimationTestActor::FindFaceComponent(AActor* GuestActor) const
{
	if (!GuestActor)
	{
		return nullptr;
	}

	TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
	GuestActor->GetComponents<USkeletalMeshComponent>(SkeletalMeshComponents);
	for (USkeletalMeshComponent* MeshComponent : SkeletalMeshComponents)
	{
		if (MeshComponent && MeshComponent->GetName().Equals(TEXT("Face"), ESearchCase::IgnoreCase))
		{
			return MeshComponent;
		}
	}
	return nullptr;
}

void AMetaHumanAnimationTestActor::SyncFaceComponentToAnimation(AActor* GuestActor, USkeletalMeshComponent* FaceComponent, USkeletalMeshComponent* BodyComponent) const
{
	// Body drives Face bone transforms (correct head/neck positioning).
	// Clearing the ABP prevents it from fighting the leader pose, giving a neutral expression.
	FaceComponent->SetLeaderPoseComponent(BodyComponent, true, true);
	FaceComponent->SetAnimClass(nullptr);

	// Teeth and eye components follow Face so they stay anchored at jaw/eye bones.
	static const TArray<FString> FaceDependentNames = {
		TEXT("Teeth"), TEXT("EyeLeft"), TEXT("EyeRight"),
		TEXT("Eyes_L"), TEXT("Eyes_R"), TEXT("Eyelash"), TEXT("Eyelashes")
	};

	TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
	GuestActor->GetComponents<USkeletalMeshComponent>(SkeletalMeshComponents);
	for (USkeletalMeshComponent* MeshComponent : SkeletalMeshComponents)
	{
		if (!MeshComponent || !MeshComponent->GetSkeletalMeshAsset() || MeshComponent == FaceComponent)
		{
			continue;
		}

		const FString ComponentName = MeshComponent->GetName();
		for (const FString& DepName : FaceDependentNames)
		{
			if (ComponentName.Equals(DepName, ESearchCase::IgnoreCase) ||
				ComponentName.StartsWith(DepName, ESearchCase::IgnoreCase))
			{
				MeshComponent->SetLeaderPoseComponent(FaceComponent, true, true);
				break;
			}
		}
	}
}

int32 AMetaHumanAnimationTestActor::BindFollowerComponentsToBody(AActor* GuestActor, USkeletalMeshComponent* BodyComponent) const
{
	if (!GuestActor || !BodyComponent)
	{
		return 0;
	}

	// Face, Teeth, and Eye components have their own Animation Blueprints driving
	// facial expressions. Making them followers of the Body skeleton overrides their
	// bone transforms with body bone data, causing the twisted/distorted look.
	// Only bind clothing and accessory components that are designed to follow the body.
	static const TArray<FString> FaceComponentNames = {
		TEXT("Face"), TEXT("Teeth"), TEXT("EyeLeft"), TEXT("EyeRight"),
		TEXT("Eyes_L"), TEXT("Eyes_R"), TEXT("Eyelash"), TEXT("Eyelashes")
	};

	int32 BoundCount = 0;
	TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
	GuestActor->GetComponents<USkeletalMeshComponent>(SkeletalMeshComponents);
	for (USkeletalMeshComponent* MeshComponent : SkeletalMeshComponents)
	{
		if (!MeshComponent || MeshComponent == BodyComponent || !MeshComponent->GetSkeletalMeshAsset())
		{
			continue;
		}

		const FString ComponentName = MeshComponent->GetName();
		bool bIsFaceComponent = false;
		for (const FString& FaceName : FaceComponentNames)
		{
			if (ComponentName.Equals(FaceName, ESearchCase::IgnoreCase) ||
				ComponentName.StartsWith(FaceName, ESearchCase::IgnoreCase))
			{
				bIsFaceComponent = true;
				break;
			}
		}

		if (bIsFaceComponent)
		{
			continue;
		}

		MeshComponent->SetLeaderPoseComponent(BodyComponent, true, true);
		++BoundCount;
	}

	return BoundCount;
}

int32 AMetaHumanAnimationTestActor::CountSkeletalMeshComponents(AActor* GuestActor) const
{
	if (!GuestActor)
	{
		return 0;
	}

	TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
	GuestActor->GetComponents<USkeletalMeshComponent>(SkeletalMeshComponents);
	return SkeletalMeshComponents.Num();
}

void AMetaHumanAnimationTestActor::SetStatus(const FString& Message, bool bChildActorCreated, bool bBodyComponentFound, FName AnimatedComponentName, int32 SkeletalComponentCount, int32 FollowerComponentsBound)
{
	LastStatusMessage = Message;
	bLastChildActorCreated = bChildActorCreated;
	bLastBodyComponentFound = bBodyComponentFound;
	LastAnimatedComponentName = AnimatedComponentName;
	LastSkeletalComponentCount = SkeletalComponentCount;
	LastFollowerComponentsBound = FollowerComponentsBound;
}
