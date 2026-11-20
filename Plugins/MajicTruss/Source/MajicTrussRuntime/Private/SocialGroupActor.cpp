#include "SocialGroupActor.h"

#include "Animation/AnimationAsset.h"
#include "Components/ArrowComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/SkeletalMesh.h"

ASocialGroupActor::ASocialGroupActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	AIAnimationPreviewMesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/IdleSitPoseAnimations/Mannequin/Character/Mesh/SK_Mannequin.SK_Mannequin")));
	AIAnimationPreviewDefaultAnimation = TSoftObjectPtr<UAnimationAsset>(FSoftObjectPath(TEXT("/Game/IdleSitPoseAnimations/Animations/ArmsOpen.ArmsOpen")));
}

void ASocialGroupActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshSocialGroupPreview();
}

#if WITH_EDITOR
void ASocialGroupActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RefreshSocialGroupPreview();
}
#endif

void ASocialGroupActor::RefreshSocialGroupPreview()
{
	CharacterCount = FMath::Clamp(CharacterCount, 1, 32);
	GroupRadiusCm = FMath::Max(0.0f, GroupRadiusCm);
	RefreshAISlotPreviewComponents();
}

void ASocialGroupActor::GetAISlotPreviewLocalOffsets(TArray<FVector>& OutOffsets) const
{
	OutOffsets.Reset();

	const int32 Count = FMath::Clamp(CharacterCount, 1, 32);
	const float AngleStepDegrees = Count > 0 ? 360.0f / static_cast<float>(Count) : 0.0f;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float AngleRadians = FMath::DegreesToRadians(StartAngleDegrees + AngleStepDegrees * Index);
		const FVector CircleOffset(FMath::Cos(AngleRadians) * GroupRadiusCm, FMath::Sin(AngleRadians) * GroupRadiusCm, 0.0f);
		const FVector PerSlotOffset = PerSlotOffsetsCm.IsValidIndex(Index)
			? PerSlotOffsetsCm[Index]
			: FVector::ZeroVector;
		OutOffsets.Add(CenterOffsetCm + CircleOffset + PerSlotOffset);
	}
}

void ASocialGroupActor::GetAISlotPreviewLocalRotations(TArray<FRotator>& OutRotations) const
{
	OutRotations.Reset();

	TArray<FVector> PreviewOffsets;
	GetAISlotPreviewLocalOffsets(PreviewOffsets);
	for (int32 Index = 0; Index < PreviewOffsets.Num(); ++Index)
	{
		FVector Direction = CenterOffsetCm - PreviewOffsets[Index];
		Direction.Z = 0.0f;

		FRotator Rotation = Direction.IsNearlyZero()
			? FRotator::ZeroRotator
			: Direction.Rotation();
		Rotation.Pitch = 0.0f;
		Rotation.Roll = 0.0f;

		if (PerSlotRotationOffsets.IsValidIndex(Index))
		{
			Rotation += PerSlotRotationOffsets[Index];
		}

		OutRotations.Add(Rotation);
	}
}

void ASocialGroupActor::GetAISlotPreviewAnimations(TArray<TSoftObjectPtr<UAnimationAsset>>& OutAnimations) const
{
	OutAnimations.Reset();

	const int32 Count = FMath::Clamp(CharacterCount, 1, 32);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		OutAnimations.Add(AIAnimationPreviewPerSlotAnimations.IsValidIndex(Index)
			? AIAnimationPreviewPerSlotAnimations[Index]
			: AIAnimationPreviewDefaultAnimation);
	}
}

void ASocialGroupActor::RefreshAISlotPreviewComponents()
{
#if WITH_EDITOR
	ClearEditorPreviewComponents();

	if (!bShowAISlotPreviewInEditor || !SceneRoot)
	{
		return;
	}

	TArray<FVector> PreviewOffsets;
	GetAISlotPreviewLocalOffsets(PreviewOffsets);
	TArray<FRotator> PreviewRotations;
	GetAISlotPreviewLocalRotations(PreviewRotations);

	while (AISlotPreviewComponents.Num() < PreviewOffsets.Num())
	{
		const FName ComponentName = MakeUniqueObjectName(this, USphereComponent::StaticClass(), TEXT("EditorSocialAISlotPreview"));
		USphereComponent* PreviewComponent = NewObject<USphereComponent>(this, ComponentName, RF_Transient | RF_TextExportTransient);
		if (!PreviewComponent)
		{
			break;
		}

		PreviewComponent->SetupAttachment(SceneRoot);
		PreviewComponent->SetIsVisualizationComponent(true);
		PreviewComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PreviewComponent->SetGenerateOverlapEvents(false);
		PreviewComponent->SetSphereRadius(20.0f);
		PreviewComponent->ShapeColor = FColor::Green;
		PreviewComponent->SetHiddenInGame(true);
		PreviewComponent->SetVisibility(true);
		PreviewComponent->RegisterComponent();
		AddInstanceComponent(PreviewComponent);
		AISlotPreviewComponents.Add(PreviewComponent);
	}

	while (AISlotPreviewArrowComponents.Num() < PreviewOffsets.Num())
	{
		const FName ComponentName = MakeUniqueObjectName(this, UArrowComponent::StaticClass(), TEXT("EditorSocialAISlotArrow"));
		UArrowComponent* ArrowComponent = NewObject<UArrowComponent>(this, ComponentName, RF_Transient | RF_TextExportTransient);
		if (!ArrowComponent)
		{
			break;
		}

		ArrowComponent->SetupAttachment(SceneRoot);
		ArrowComponent->SetIsVisualizationComponent(true);
		ArrowComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ArrowComponent->SetHiddenInGame(true);
		ArrowComponent->SetVisibility(true);
		ArrowComponent->ArrowColor = FColor::Green;
		ArrowComponent->ArrowSize = 1.25f;
		ArrowComponent->ArrowLength = 75.0f;
		ArrowComponent->RegisterComponent();
		AddInstanceComponent(ArrowComponent);
		AISlotPreviewArrowComponents.Add(ArrowComponent);
	}

	for (int32 Index = 0; Index < AISlotPreviewComponents.Num(); ++Index)
	{
		USphereComponent* PreviewComponent = AISlotPreviewComponents[Index];
		if (!PreviewComponent)
		{
			continue;
		}

		const bool bHasPreview = PreviewOffsets.IsValidIndex(Index);
		PreviewComponent->SetVisibility(bHasPreview);
		PreviewComponent->SetHiddenInGame(true);
		if (bHasPreview)
		{
			PreviewComponent->SetRelativeLocation(PreviewOffsets[Index]);
		}
	}

	for (int32 Index = 0; Index < AISlotPreviewArrowComponents.Num(); ++Index)
	{
		UArrowComponent* ArrowComponent = AISlotPreviewArrowComponents[Index];
		if (!ArrowComponent)
		{
			continue;
		}

		const bool bHasPreview = PreviewOffsets.IsValidIndex(Index);
		ArrowComponent->SetVisibility(bHasPreview);
		ArrowComponent->SetHiddenInGame(true);
		if (bHasPreview)
		{
			ArrowComponent->SetRelativeLocation(PreviewOffsets[Index]);
			ArrowComponent->SetRelativeRotation(PreviewRotations.IsValidIndex(Index) ? PreviewRotations[Index] : FRotator::ZeroRotator);
		}
	}

	RefreshAIAnimationPreviewComponents(PreviewOffsets, PreviewRotations);
#endif
}

void ASocialGroupActor::RefreshAIAnimationPreviewComponents(const TArray<FVector>& PreviewOffsets, const TArray<FRotator>& PreviewRotations)
{
#if WITH_EDITOR
	if (!bShowAIAnimationPreviewInEditor || !SceneRoot)
	{
		return;
	}

	USkeletalMesh* PreviewMesh = AIAnimationPreviewMesh.LoadSynchronous();
	UAnimationAsset* DefaultAnimation = AIAnimationPreviewDefaultAnimation.LoadSynchronous();
	if (!PreviewMesh)
	{
		return;
	}

	while (AIAnimationPreviewComponents.Num() < PreviewOffsets.Num())
	{
		const FName ComponentName = MakeUniqueObjectName(this, USkeletalMeshComponent::StaticClass(), TEXT("EditorSocialAIAnimationPreview"));
		USkeletalMeshComponent* PreviewComponent = NewObject<USkeletalMeshComponent>(this, ComponentName, RF_Transient | RF_TextExportTransient);
		if (!PreviewComponent)
		{
			break;
		}

		PreviewComponent->SetupAttachment(SceneRoot);
		PreviewComponent->SetIsVisualizationComponent(true);
		PreviewComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PreviewComponent->SetGenerateOverlapEvents(false);
		PreviewComponent->SetHiddenInGame(true);
		PreviewComponent->SetVisibility(true);
		PreviewComponent->RegisterComponent();
		AddInstanceComponent(PreviewComponent);
		AIAnimationPreviewComponents.Add(PreviewComponent);
	}

	for (int32 Index = 0; Index < AIAnimationPreviewComponents.Num(); ++Index)
	{
		USkeletalMeshComponent* PreviewComponent = AIAnimationPreviewComponents[Index];
		if (!PreviewComponent)
		{
			continue;
		}

		const bool bHasPreview = PreviewOffsets.IsValidIndex(Index);
		PreviewComponent->SetVisibility(bHasPreview);
		PreviewComponent->SetHiddenInGame(true);
		if (!bHasPreview)
		{
			continue;
		}

		UAnimationAsset* SlotAnimation = AIAnimationPreviewPerSlotAnimations.IsValidIndex(Index)
			? AIAnimationPreviewPerSlotAnimations[Index].LoadSynchronous()
			: nullptr;

		PreviewComponent->SetSkeletalMesh(PreviewMesh);
		PreviewComponent->SetRelativeLocation(PreviewOffsets[Index] + AIAnimationPreviewMeshOffsetCm);
		PreviewComponent->SetRelativeRotation((PreviewRotations.IsValidIndex(Index) ? PreviewRotations[Index] : FRotator::ZeroRotator) + AIAnimationPreviewMeshRotation);
		PreviewComponent->SetRelativeScale3D(AIAnimationPreviewMeshScale);
		if (SlotAnimation || DefaultAnimation)
		{
			PreviewComponent->PlayAnimation(SlotAnimation ? SlotAnimation : DefaultAnimation, true);
		}
	}
#endif
}

void ASocialGroupActor::ClearEditorPreviewComponents()
{
#if WITH_EDITOR
	TArray<UActorComponent*> Components;
	GetComponents(Components);
	for (UActorComponent* Component : Components)
	{
		if (!Component)
		{
			continue;
		}

		const FString ComponentName = Component->GetName();
		const bool bIsEditorPreview =
			ComponentName.StartsWith(TEXT("EditorSocialAISlotPreview")) ||
			ComponentName.StartsWith(TEXT("EditorSocialAISlotArrow")) ||
			ComponentName.StartsWith(TEXT("EditorSocialAIAnimationPreview"));

		if (bIsEditorPreview)
		{
			RemoveInstanceComponent(Component);
			Component->DestroyComponent();
		}
	}

	AISlotPreviewComponents.Reset();
	AISlotPreviewArrowComponents.Reset();
	AIAnimationPreviewComponents.Reset();
#endif
}
