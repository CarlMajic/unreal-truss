#include "BuildPreviewActor.h"

#include "Components/ChildActorComponent.h"
#include "Components/PrimitiveComponent.h"

ABuildPreviewActor::ABuildPreviewActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	PreviewActorComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("PreviewActor"));
	PreviewActorComponent->SetupAttachment(SceneRoot);

	SetActorEnableCollision(false);
}

void ABuildPreviewActor::BeginPlay()
{
	Super::BeginPlay();
	ApplyPreviewState();
}

void ABuildPreviewActor::SetPreviewActorClass(TSubclassOf<AActor> InActorClass)
{
	if (PreviewActorComponent->GetChildActorClass() == InActorClass)
	{
		ApplyPreviewState();
		return;
	}

	PreviewActorComponent->SetChildActorClass(InActorClass);
	ApplyPreviewState();
}

void ABuildPreviewActor::SetPlacementValid(bool bIsValid)
{
	bPlacementValid = bIsValid;
	ApplyPreviewState();
}

bool ABuildPreviewActor::IsPlacementValid() const
{
	return bPlacementValid;
}

AActor* ABuildPreviewActor::GetPreviewActor() const
{
	return PreviewActorComponent ? PreviewActorComponent->GetChildActor() : nullptr;
}

void ABuildPreviewActor::ApplyPreviewState() const
{
	ConfigurePreviewActor(GetPreviewActor(), true);
}

void ABuildPreviewActor::ConfigurePreviewActor(AActor* Actor, bool bVisible)
{
	if (!Actor)
	{
		return;
	}

	Actor->SetActorEnableCollision(false);
	Actor->SetActorHiddenInGame(!bVisible);
	Actor->SetCanBeDamaged(false);

	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents;
	Actor->GetComponents(PrimitiveComponents);
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (!PrimitiveComponent)
		{
			continue;
		}

		PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PrimitiveComponent->SetGenerateOverlapEvents(false);
		PrimitiveComponent->SetHiddenInGame(!bVisible);
		PrimitiveComponent->SetRenderCustomDepth(true);
	}
}
