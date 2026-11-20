#include "EventNPCActor.h"

#include "EventNPCController.h"
#include "EventDirectorActor.h"
#include "EventSmartObjectComponent.h"
#include "BarSeatingActor.h"
#include "BehaviorTree/BehaviorTree.h"
#include "CafeTableActor.h"
#include "Components/CapsuleComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Animation/AnimationAsset.h"
#include "AIController.h"
#include "Engine/SkeletalMesh.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTagsManager.h"
#include "Kismet/GameplayStatics.h"
#include "LoungeLayoutActor.h"
#include "MajicEventAIModule.h"
#include "NavigationData.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "SectionalLayoutActor.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	TWeakObjectPtr<UWorld> LastNavigationBuildWorld;
	TWeakObjectPtr<UWorld> LastSmartObjectBootstrapWorld;

	const TCHAR* DefaultNPCSkeletalMeshPath = TEXT("/Game/IdleSitPoseAnimations/Mannequin/Character/Mesh/SK_Mannequin.SK_Mannequin");
	const TCHAR* DefaultNPCAnimClassPath = TEXT("/Game/Variant_Combat/Anims/ABP_Manny_Combat.ABP_Manny_Combat_C");
	const TCHAR* DefaultNPCIdleAnimationPath = TEXT("/Game/IdleSitPoseAnimations/Animations/IdleBase.IdleBase");
	const TCHAR* DefaultNPCWalkAnimationPath = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd.MF_Unarmed_Walk_Fwd");
	const TCHAR* DefaultNPCSitAnimationPath = TEXT("/Game/IdleSitPoseAnimations/Animations/Relaxed_Sit.Relaxed_Sit");
	const TCHAR* DefaultNPCBarIdleAnimationPath = TEXT("/Game/IdleSitPoseAnimations/Animations/ArmsOnLegs_Sit.ArmsOnLegs_Sit");
	const TCHAR* DefaultNPCCafeIdleAnimationPath = TEXT("/Game/IdleSitPoseAnimations/Animations/ArmsOnKnees_Sit.ArmsOnKnees_Sit");
	const TCHAR* DefaultNPCTalkAnimationPath = TEXT("/Game/IdleSitPoseAnimations/Animations/ArmsOpen.ArmsOpen");
	const TCHAR* DefaultNPCStageWatchAnimationPath = TEXT("/Game/IdleSitPoseAnimations/Animations/Captivated.Captivated");
	const TCHAR* DefaultMetaHumanVisualBlueprintPath = TEXT("/Game/MetaHumans/MH_Guest_01/BP_MH_Guest_01.BP_MH_Guest_01_C");
	const TCHAR* DefaultMetaHumanBarIdleAnimationPath = TEXT("/Game/EventAI/MetaHumanAnimations/ArmsOnLegs_Sit.ArmsOnLegs_Sit");

	USkeletalMeshComponent* FindMetaHumanBodyComponent(AActor* VisualActor, USkeletalMeshComponent*& OutFallbackMeshComponent)
	{
		OutFallbackMeshComponent = nullptr;
		if (!VisualActor)
		{
			return nullptr;
		}

		TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
		VisualActor->GetComponents<USkeletalMeshComponent>(SkeletalMeshComponents);
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

	void PlaySingleNodeAnimation(USkeletalMeshComponent* MeshComponent, UAnimationAsset* Animation, bool bLooping, float PlayRate)
	{
		if (!MeshComponent || !Animation)
		{
			return;
		}

		MeshComponent->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		MeshComponent->SetAnimation(Animation);
		MeshComponent->Play(bLooping);
		MeshComponent->SetPlayRate(PlayRate);
	}

	void HoldSingleNodeAnimation(USkeletalMeshComponent* MeshComponent, UAnimationAsset* Animation, float NormalizedTime)
	{
		if (!MeshComponent || !Animation)
		{
			return;
		}

		const float AnimationLength = FMath::Max(Animation->GetPlayLength(), 0.0f);
		const float HoldPosition = AnimationLength * FMath::Clamp(NormalizedTime, 0.0f, 1.0f);

		MeshComponent->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		MeshComponent->SetAnimation(Animation);
		MeshComponent->Play(false);
		MeshComponent->SetPosition(HoldPosition, false);
		MeshComponent->SetPlayRate(0.0f);
	}

	bool IsSeatedInteractionTag(const FGameplayTag& InteractionTag)
	{
		const FGameplayTag BarOrderTag = FGameplayTag::RequestGameplayTag(FName(TEXT("EventInteract.Bar.Order")), false);
		const FGameplayTag CafeSitTag = FGameplayTag::RequestGameplayTag(FName(TEXT("EventInteract.Cafe.Sit")), false);
		const FGameplayTag LoungeSitTag = FGameplayTag::RequestGameplayTag(FName(TEXT("EventInteract.Lounge.Sit")), false);

		return InteractionTag == BarOrderTag || InteractionTag == CafeSitTag || InteractionTag == LoungeSitTag;
	}

	void BootstrapPlacedSeatingSmartObjects(UWorld* World)
	{
		if (!World || LastSmartObjectBootstrapWorld.Get() == World)
		{
			return;
		}

		LastSmartObjectBootstrapWorld = World;

		const FGameplayTag CafeSitTag = FGameplayTag::RequestGameplayTag(FName(TEXT("EventInteract.Cafe.Sit")), false);
		const FGameplayTag BarOrderTag = FGameplayTag::RequestGameplayTag(FName(TEXT("EventInteract.Bar.Order")), false);
		const FGameplayTag LoungeSitTag = FGameplayTag::RequestGameplayTag(FName(TEXT("EventInteract.Lounge.Sit")), false);

		for (TActorIterator<ACafeTableActor> It(World); It; ++It)
		{
			ACafeTableActor* CafeTable = *It;
			if (!CafeTable || CafeTable->FindComponentByClass<UEventSmartObjectComponent>())
			{
				continue;
			}

			CafeTable->RebuildCafeTable();

			UEventSmartObjectComponent* SmartObject = NewObject<UEventSmartObjectComponent>(CafeTable, TEXT("AutoCafeSmartObject"));
			if (!SmartObject)
			{
				continue;
			}

			SmartObject->InteractionTag = CafeSitTag;
			SmartObject->MinimumTestSlotCount = 2;
			SmartObject->TestSlotClusterRadiusCm = CafeTable->AISlotPreviewSpreadRadiusCm;
			SmartObject->TestSlotClusterStartAngleDegrees = CafeTable->AISlotPreviewStartAngleDegrees;
			TArray<FVector> AISlotOffsets;
			CafeTable->GetAISlotPreviewLocalOffsets(AISlotOffsets);
			if (AISlotOffsets.Num() == 0)
			{
				continue;
			}
			TArray<FRotator> AISlotRotations;
			CafeTable->GetAISlotPreviewLocalRotations(AISlotRotations);

			CafeTable->AddInstanceComponent(SmartObject);
			SmartObject->RegisterComponent();
			SmartObject->SetSlotsFromLocalOffsetsAndRotations(AISlotOffsets, AISlotRotations);
			TArray<TSoftObjectPtr<UAnimationAsset>> AISlotAnimations;
			CafeTable->GetAISlotPreviewAnimations(AISlotAnimations);
			SmartObject->SetSlotAnimations(AISlotAnimations);
			SmartObject->DrawDebugSlots();
			UE_LOG(LogMajicEventAI, Display, TEXT("SmartObjectBootstrap actor=%s type=cafe slots=%d"),
				*CafeTable->GetName(),
				SmartObject->Slots.Num());
		}

		for (TActorIterator<ABarSeatingActor> It(World); It; ++It)
		{
			ABarSeatingActor* BarSeating = *It;
			if (!BarSeating || BarSeating->FindComponentByClass<UEventSmartObjectComponent>())
			{
				continue;
			}

			BarSeating->RebuildBarSeating();

			UEventSmartObjectComponent* SmartObject = NewObject<UEventSmartObjectComponent>(BarSeating, TEXT("AutoBarSmartObject"));
			if (!SmartObject)
			{
				continue;
			}

			SmartObject->InteractionTag = BarOrderTag;
			SmartObject->MinimumTestSlotCount = 2;
			SmartObject->TestSlotClusterRadiusCm = BarSeating->AISlotPreviewSpreadRadiusCm;
			SmartObject->TestSlotClusterStartAngleDegrees = BarSeating->AISlotPreviewStartAngleDegrees;
			TArray<FVector> AISlotOffsets;
			BarSeating->GetAISlotPreviewLocalOffsets(AISlotOffsets);
			if (AISlotOffsets.Num() == 0)
			{
				continue;
			}
			TArray<FRotator> AISlotRotations;
			BarSeating->GetAISlotPreviewLocalRotations(AISlotRotations);

			BarSeating->AddInstanceComponent(SmartObject);
			SmartObject->RegisterComponent();
			SmartObject->SetSlotsFromLocalOffsetsAndRotations(AISlotOffsets, AISlotRotations);
			TArray<TSoftObjectPtr<UAnimationAsset>> AISlotAnimations;
			BarSeating->GetAISlotPreviewAnimations(AISlotAnimations);
			SmartObject->SetSlotAnimations(AISlotAnimations);
			SmartObject->DrawDebugSlots();
			UE_LOG(LogMajicEventAI, Display, TEXT("SmartObjectBootstrap actor=%s type=bar slots=%d"),
				*BarSeating->GetName(),
				SmartObject->Slots.Num());
		}

		for (TActorIterator<ALoungeLayoutActor> It(World); It; ++It)
		{
			ALoungeLayoutActor* LoungeLayout = *It;
			if (!LoungeLayout || LoungeLayout->FindComponentByClass<UEventSmartObjectComponent>())
			{
				continue;
			}

			LoungeLayout->RebuildLoungeLayout();

			TArray<FVector> AISlotOffsets;
			LoungeLayout->GetAISlotPreviewLocalOffsets(AISlotOffsets);
			if (AISlotOffsets.Num() == 0)
			{
				continue;
			}
			TArray<FRotator> AISlotRotations;
			LoungeLayout->GetAISlotPreviewLocalRotations(AISlotRotations);

			UEventSmartObjectComponent* SmartObject = NewObject<UEventSmartObjectComponent>(LoungeLayout, TEXT("AutoLoungeSmartObject"));
			if (!SmartObject)
			{
				continue;
			}

			SmartObject->InteractionTag = LoungeSitTag;
			SmartObject->MinimumTestSlotCount = 1;
			LoungeLayout->AddInstanceComponent(SmartObject);
			SmartObject->RegisterComponent();
			SmartObject->SetSlotsFromLocalOffsetsAndRotations(AISlotOffsets, AISlotRotations);
			TArray<TSoftObjectPtr<UAnimationAsset>> AISlotAnimations;
			LoungeLayout->GetAISlotPreviewAnimations(AISlotAnimations);
			SmartObject->SetSlotAnimations(AISlotAnimations);
			SmartObject->DrawDebugSlots();
			UE_LOG(LogMajicEventAI, Display, TEXT("SmartObjectBootstrap actor=%s type=lounge slots=%d"),
				*LoungeLayout->GetName(),
				SmartObject->Slots.Num());
		}

		for (TActorIterator<ASectionalLayoutActor> It(World); It; ++It)
		{
			ASectionalLayoutActor* SectionalLayout = *It;
			if (!SectionalLayout || SectionalLayout->FindComponentByClass<UEventSmartObjectComponent>())
			{
				continue;
			}

			SectionalLayout->RebuildSectionalLayout();

			TArray<FVector> AISlotOffsets;
			SectionalLayout->GetAISlotPreviewLocalOffsets(AISlotOffsets);
			if (AISlotOffsets.Num() == 0)
			{
				continue;
			}
			TArray<FRotator> AISlotRotations;
			SectionalLayout->GetAISlotPreviewLocalRotations(AISlotRotations);

			TArray<FVector> AIEntryOffsets;
			SectionalLayout->GetAIEntryPreviewLocalOffsets(AIEntryOffsets);

			UEventSmartObjectComponent* SmartObject = NewObject<UEventSmartObjectComponent>(SectionalLayout, TEXT("AutoSectionalSmartObject"));
			if (!SmartObject)
			{
				continue;
			}

			SmartObject->InteractionTag = LoungeSitTag;
			SmartObject->MinimumTestSlotCount = 1;
			SmartObject->bLimitOccupancyByPercentage = true;
			SmartObject->MaxOccupancyRatio = 0.125f;
			SectionalLayout->AddInstanceComponent(SmartObject);
			SmartObject->RegisterComponent();
			SmartObject->SetSlotsFromLocalOffsetsAndRotations(AISlotOffsets, AISlotRotations);
			SmartObject->SetApproachLocalOffsets(AIEntryOffsets);
			TArray<TSoftObjectPtr<UAnimationAsset>> AISlotAnimations;
			SectionalLayout->GetAISlotPreviewAnimations(AISlotAnimations);
			SmartObject->SetSlotAnimations(AISlotAnimations);
			SmartObject->DrawDebugSlots();
			UE_LOG(LogMajicEventAI, Display, TEXT("SmartObjectBootstrap actor=%s type=sectional slots=%d entries=%d capacity=%d occupancy_limit=%.0f%%"),
				*SectionalLayout->GetName(),
				SmartObject->Slots.Num(),
				SmartObject->ApproachLocalOffsets.Num(),
				SmartObject->GetMaxClaimedSlotCount(),
				SmartObject->MaxOccupancyRatio * 100.0f);
		}
	}
}

AEventNPCActor::AEventNPCActor()
{
	PrimaryActorTick.bCanEverTick = true;

	AIControllerClass = AEventNPCController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// Default movement speed for a guest walking through an event venue
	GetCharacterMovement()->MaxWalkSpeed = 150.0f;
	GetCharacterMovement()->MaxFlySpeed = 150.0f;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->bUseRVOAvoidance = true;
	GetCharacterMovement()->AvoidanceConsiderationRadius = 500.0f;
	GetCharacterMovement()->AvoidanceWeight = 0.95f;
	bUseControllerRotationYaw = false;

	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetRelativeLocation(NPCMeshRelativeLocation);
	GetMesh()->SetRelativeRotation(NPCMeshRelativeRotation);
	GetMesh()->SetRelativeScale3D(NPCMeshRelativeScale);
	GetMesh()->SetHiddenInGame(true);
	GetMesh()->SetVisibility(false, true);

	DebugPlaceholderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DebugPlaceholderMesh"));
	DebugPlaceholderMesh->SetupAttachment(GetRootComponent());
	DebugPlaceholderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DebugPlaceholderMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 88.0f));
	DebugPlaceholderMesh->SetRelativeScale3D(FVector(0.35f, 0.35f, 1.6f));

	BlueprintVisualComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("BlueprintVisualComponent"));
	BlueprintVisualComponent->SetupAttachment(GetRootComponent());
	BlueprintVisualComponent->SetRelativeLocation(NPCMeshRelativeLocation);
	BlueprintVisualComponent->SetRelativeRotation(NPCMeshRelativeRotation);
	BlueprintVisualComponent->SetRelativeScale3D(NPCMeshRelativeScale);
	BlueprintVisualComponent->SetHiddenInGame(true);
	BlueprintVisualComponent->SetVisibility(false, true);

	DebugLabelComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("DebugNPCLabel"));
	DebugLabelComponent->SetupAttachment(GetRootComponent());
	DebugLabelComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 235.0f));
	DebugLabelComponent->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	DebugLabelComponent->SetHorizontalAlignment(EHTA_Center);
	DebugLabelComponent->SetVerticalAlignment(EVRTA_TextCenter);
	DebugLabelComponent->SetTextRenderColor(FColor::Yellow);
	DebugLabelComponent->SetWorldSize(18.0f);
	DebugLabelComponent->SetHiddenInGame(false);
	DebugLabelComponent->SetVisibility(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DebugMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (DebugMesh.Succeeded())
	{
		DebugPlaceholderMesh->SetStaticMesh(DebugMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UBehaviorTree> DefaultBehaviorTree(TEXT("/Game/EventAI/BT_EventNPC.BT_EventNPC"));
	if (DefaultBehaviorTree.Succeeded())
	{
		BehaviorTree = DefaultBehaviorTree.Object;
	}

	NPCSkeletalMesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(DefaultNPCSkeletalMeshPath));
	NPCAnimClass = TSoftClassPtr<UAnimInstance>(FSoftObjectPath(DefaultNPCAnimClassPath));
	NPCIdleAnimation = TSoftObjectPtr<UAnimationAsset>(FSoftObjectPath(DefaultNPCIdleAnimationPath));
	NPCWalkAnimation = TSoftObjectPtr<UAnimationAsset>(FSoftObjectPath(DefaultNPCWalkAnimationPath));
	NPCSitAnimation = TSoftObjectPtr<UAnimationAsset>(FSoftObjectPath(DefaultNPCSitAnimationPath));
	NPCBarIdleAnimation = TSoftObjectPtr<UAnimationAsset>(FSoftObjectPath(DefaultNPCBarIdleAnimationPath));
	NPCCafeIdleAnimation = TSoftObjectPtr<UAnimationAsset>(FSoftObjectPath(DefaultNPCCafeIdleAnimationPath));
	NPCTalkAnimation = TSoftObjectPtr<UAnimationAsset>(FSoftObjectPath(DefaultNPCTalkAnimationPath));
	NPCStageWatchAnimation = TSoftObjectPtr<UAnimationAsset>(FSoftObjectPath(DefaultNPCStageWatchAnimationPath));

	bUseRandomVisualVariants = true;
	bShowDebugPlaceholder = false;
	FEventNPCVisualVariant DefaultMetaHumanVariant;
	DefaultMetaHumanVariant.VariantName = TEXT("MH_Guest_01");
	DefaultMetaHumanVariant.VisualBlueprintClass = TSoftClassPtr<AActor>(FSoftObjectPath(DefaultMetaHumanVisualBlueprintPath));
	DefaultMetaHumanVariant.BarIdleAnimation = TSoftObjectPtr<UAnimationAsset>(FSoftObjectPath(DefaultMetaHumanBarIdleAnimationPath));
	DefaultMetaHumanVariant.CafeIdleAnimation = TSoftObjectPtr<UAnimationAsset>(FSoftObjectPath(DefaultMetaHumanBarIdleAnimationPath));
	DefaultMetaHumanVariant.SitAnimation = TSoftObjectPtr<UAnimationAsset>(FSoftObjectPath(DefaultMetaHumanBarIdleAnimationPath));
	DefaultMetaHumanVariant.MeshRelativeLocation = NPCMeshRelativeLocation;
	DefaultMetaHumanVariant.MeshRelativeRotation = NPCMeshRelativeRotation;
	DefaultMetaHumanVariant.MeshRelativeScale = NPCMeshRelativeScale;
	VisualVariants.Add(DefaultMetaHumanVariant);
}

void AEventNPCActor::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	BootstrapPlacedSeatingSmartObjects(World);
	if (World && LastNavigationBuildWorld.Get() != World)
	{
		LastNavigationBuildWorld = World;

		if (UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
		{
			ANavigationData* NavData = NavSystem->GetDefaultNavDataInstance(FNavigationSystem::Create);
			if (NavData)
			{
				UE_LOG(LogMajicEventAI, Display, TEXT("NavigationReady data=%s runtime_generation=%d"),
					*NavData->GetName(),
					static_cast<int32>(NavData->GetRuntimeGenerationMode()));
				NavSystem->Build();
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[EventAI] NavigationSystem exists but no default nav data could be created"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[EventAI] No NavigationSystem found in world"));
		}
	}

	if (DebugPlaceholderMesh)
	{
		DebugPlaceholderMesh->SetHiddenInGame(!bShowDebugPlaceholder);
		DebugPlaceholderMesh->SetVisibility(bShowDebugPlaceholder, true);
	}

	ConfigureNPCVisual();
	LastVisualAnimationLocation = GetActorLocation();

	GetCharacterMovement()->bUseRVOAvoidance = bUseCrowdAvoidance;
	GetCharacterMovement()->AvoidanceConsiderationRadius = CrowdAvoidanceConsiderationRadius;
	GetCharacterMovement()->AvoidanceWeight = CrowdAvoidanceWeight;
	GetCharacterMovement()->GravityScale = 1.0f;
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	if (bBypassVenueCollisionForPrototype)
	{
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetCharacterMovement()->GravityScale = 0.0f;
		GetCharacterMovement()->SetMovementMode(MOVE_Flying);
		UE_LOG(LogMajicEventAI, Warning, TEXT("NPCPrototypeCollisionBypass npc=%s warning=will_pass_through_obstacles"), *GetName());
	}

	if (!EventDirector)
	{
		EventDirector = Cast<AEventDirectorActor>(UGameplayStatics::GetActorOfClass(this, AEventDirectorActor::StaticClass()));
	}

	if (EventDirector)
	{
		EventDirector->RegisterNPC(this);
		OnEventPhaseChanged(EventDirector->GetCurrentPhase());
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("[EventAI] %s has no EventDirectorActor; using local phase %d"), *GetName(), static_cast<uint8>(CurrentPhase));
	}
}

void AEventNPCActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateNPCVisualAnimation(DeltaSeconds);
}

const FEventNPCVisualVariant* AEventNPCActor::SelectVisualVariant()
{
	if (!bUseRandomVisualVariants || VisualVariants.Num() == 0)
	{
		SelectedVisualVariantIndex = INDEX_NONE;
		return nullptr;
	}

	if (!VisualVariants.IsValidIndex(SelectedVisualVariantIndex))
	{
		SelectedVisualVariantIndex = FMath::RandRange(0, VisualVariants.Num() - 1);
	}

	return VisualVariants.IsValidIndex(SelectedVisualVariantIndex)
		? &VisualVariants[SelectedVisualVariantIndex]
		: nullptr;
}

void AEventNPCActor::ConfigureNPCVisual()
{
	bool bLoadedVisual = false;
	bUsingBlueprintVisual = false;
	const FEventNPCVisualVariant* VisualVariant = SelectVisualVariant();

	if (BlueprintVisualComponent)
	{
		BlueprintVisualComponent->SetChildActorClass(nullptr);
		BlueprintVisualComponent->SetHiddenInGame(true);
		BlueprintVisualComponent->SetVisibility(false, true);
	}

	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		CharacterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ResetNPCVisualCalibration();

		const bool bVariantUsesBlueprintVisual = VisualVariant && !VisualVariant->VisualBlueprintClass.IsNull();
		const bool bVariantUsesSkeletalMesh = VisualVariant && !VisualVariant->SkeletalMesh.IsNull();
		const FVector BaseMeshLocation = (bVariantUsesBlueprintVisual || bVariantUsesSkeletalMesh)
			? VisualVariant->MeshRelativeLocation
			: NPCMeshRelativeLocation;
		const FRotator BaseMeshRotation = (bVariantUsesBlueprintVisual || bVariantUsesSkeletalMesh)
			? VisualVariant->MeshRelativeRotation
			: NPCMeshRelativeRotation;
		const FVector BaseMeshScale = (bVariantUsesBlueprintVisual || bVariantUsesSkeletalMesh)
			? VisualVariant->MeshRelativeScale
			: NPCMeshRelativeScale;
		ActiveVisualMeshRelativeLocation = BaseMeshLocation;
		ActiveVisualMeshRelativeRotation = BaseMeshRotation;
		CharacterMesh->SetRelativeLocation(BaseMeshLocation);
		CharacterMesh->SetRelativeRotation(BaseMeshRotation);
		CharacterMesh->SetRelativeScale3D(BaseMeshScale);

		if (bVariantUsesBlueprintVisual && BlueprintVisualComponent)
		{
			if (UClass* LoadedBlueprintClass = VisualVariant->VisualBlueprintClass.LoadSynchronous())
			{
				BlueprintVisualComponent->SetRelativeLocation(BaseMeshLocation);
				BlueprintVisualComponent->SetRelativeRotation(BaseMeshRotation);
				BlueprintVisualComponent->SetRelativeScale3D(BaseMeshScale);
				BlueprintVisualComponent->SetChildActorClass(LoadedBlueprintClass);
				if (!BlueprintVisualComponent->GetChildActor())
				{
					BlueprintVisualComponent->CreateChildActor();
				}
				BlueprintVisualComponent->SetHiddenInGame(false);
				BlueprintVisualComponent->SetVisibility(true, true);

				CharacterMesh->SetHiddenInGame(true);
				CharacterMesh->SetVisibility(false, true);
				bLoadedVisual = true;
				bUsingBlueprintVisual = true;

				LoadedIdleAnimation = VisualVariant && !VisualVariant->IdleAnimation.IsNull()
					? VisualVariant->IdleAnimation.LoadSynchronous()
					: nullptr;
				LoadedWalkAnimation = VisualVariant && !VisualVariant->WalkAnimation.IsNull()
					? VisualVariant->WalkAnimation.LoadSynchronous()
					: nullptr;
				LoadedSitAnimation = VisualVariant && !VisualVariant->SitAnimation.IsNull()
					? VisualVariant->SitAnimation.LoadSynchronous()
					: nullptr;
				LoadedBarIdleAnimation = VisualVariant && !VisualVariant->BarIdleAnimation.IsNull()
					? VisualVariant->BarIdleAnimation.LoadSynchronous()
					: nullptr;
				LoadedCafeIdleAnimation = VisualVariant && !VisualVariant->CafeIdleAnimation.IsNull()
					? VisualVariant->CafeIdleAnimation.LoadSynchronous()
					: nullptr;
				LoadedTalkAnimation = VisualVariant && !VisualVariant->TalkAnimation.IsNull()
					? VisualVariant->TalkAnimation.LoadSynchronous()
					: nullptr;
				LoadedStageWatchAnimation = VisualVariant && !VisualVariant->StageWatchAnimation.IsNull()
					? VisualVariant->StageWatchAnimation.LoadSynchronous()
					: nullptr;

				UE_LOG(LogMajicEventAI, Display, TEXT("NPCVisualVariant npc=%s index=%d name=%s blueprint=%s"),
					*GetName(),
					SelectedVisualVariantIndex,
					*VisualVariant->VariantName.ToString(),
					*VisualVariant->VisualBlueprintClass.ToSoftObjectPath().ToString());
				LogBlueprintVisualComponents(TEXT("configured"));
			}
		}
		else if (bUseSkeletalNPCVisual)
		{
			USkeletalMesh* LoadedMesh = bVariantUsesSkeletalMesh
				? VisualVariant->SkeletalMesh.LoadSynchronous()
				: NPCSkeletalMesh.LoadSynchronous();
			if (LoadedMesh)
			{
				CharacterMesh->SetSkeletalMesh(LoadedMesh);

				LoadedIdleAnimation = VisualVariant && !VisualVariant->IdleAnimation.IsNull()
					? VisualVariant->IdleAnimation.LoadSynchronous()
					: NPCIdleAnimation.LoadSynchronous();
				LoadedWalkAnimation = VisualVariant && !VisualVariant->WalkAnimation.IsNull()
					? VisualVariant->WalkAnimation.LoadSynchronous()
					: NPCWalkAnimation.LoadSynchronous();
				LoadedSitAnimation = VisualVariant && !VisualVariant->SitAnimation.IsNull()
					? VisualVariant->SitAnimation.LoadSynchronous()
					: NPCSitAnimation.LoadSynchronous();
				LoadedBarIdleAnimation = VisualVariant && !VisualVariant->BarIdleAnimation.IsNull()
					? VisualVariant->BarIdleAnimation.LoadSynchronous()
					: NPCBarIdleAnimation.LoadSynchronous();
				LoadedCafeIdleAnimation = VisualVariant && !VisualVariant->CafeIdleAnimation.IsNull()
					? VisualVariant->CafeIdleAnimation.LoadSynchronous()
					: NPCCafeIdleAnimation.LoadSynchronous();
				LoadedTalkAnimation = VisualVariant && !VisualVariant->TalkAnimation.IsNull()
					? VisualVariant->TalkAnimation.LoadSynchronous()
					: NPCTalkAnimation.LoadSynchronous();
				LoadedStageWatchAnimation = VisualVariant && !VisualVariant->StageWatchAnimation.IsNull()
					? VisualVariant->StageWatchAnimation.LoadSynchronous()
					: NPCStageWatchAnimation.LoadSynchronous();

				if (bUseSimpleMovementAnimations && LoadedIdleAnimation)
				{
					PlaySimpleNPCAnimation(LoadedIdleAnimation, 1.0f, true);
					bNPCVisualPlayingWalk = false;
				}
				else if (UClass* LoadedAnimClass = VisualVariant && !VisualVariant->AnimClass.IsNull()
					? VisualVariant->AnimClass.LoadSynchronous()
					: NPCAnimClass.LoadSynchronous())
				{
					CharacterMesh->SetAnimInstanceClass(LoadedAnimClass);
				}

				CharacterMesh->SetHiddenInGame(false);
				CharacterMesh->SetVisibility(true, true);
				bLoadedVisual = true;

				if (VisualVariant)
				{
					UE_LOG(LogMajicEventAI, Display, TEXT("NPCVisualVariant npc=%s index=%d name=%s mesh=%s"),
						*GetName(),
						SelectedVisualVariantIndex,
						*VisualVariant->VariantName.ToString(),
						*VisualVariant->SkeletalMesh.ToSoftObjectPath().ToString());
				}
			}
			else
			{
				CharacterMesh->SetHiddenInGame(true);
				CharacterMesh->SetVisibility(false, true);
			}
		}
		else
		{
			CharacterMesh->SetHiddenInGame(true);
			CharacterMesh->SetVisibility(false, true);
		}
	}

	const bool bShowFallback = bShowDebugPlaceholder && (!bLoadedVisual || !bHideDebugPlaceholderWhenSkeletalVisualLoaded);
	if (DebugPlaceholderMesh)
	{
		DebugPlaceholderMesh->SetHiddenInGame(!bShowFallback);
		DebugPlaceholderMesh->SetVisibility(bShowFallback, true);
	}

	if (!bLoadedVisual && bUseSkeletalNPCVisual)
	{
		UE_LOG(LogMajicEventAI, Display, TEXT("NPCVisualFallback npc=%s reason=missing_skeletal_mesh path=%s"),
			*GetName(),
			*NPCSkeletalMesh.ToSoftObjectPath().ToString());
	}
}

void AEventNPCActor::PlaySimpleNPCAnimation(UAnimationAsset* Animation, float PlayRate, bool bLooping)
{
	if (!Animation)
	{
		return;
	}

	if (bUsingBlueprintVisual && BlueprintVisualComponent)
	{
		if (AActor* VisualActor = BlueprintVisualComponent->GetChildActor())
		{
			LogBlueprintVisualComponents(TEXT("before_play_loop"));
			USkeletalMeshComponent* FallbackMeshComponent = nullptr;
			if (USkeletalMeshComponent* MeshComponent = FindMetaHumanBodyComponent(VisualActor, FallbackMeshComponent))
			{
				PlaySingleNodeAnimation(MeshComponent, Animation, bLooping, PlayRate);
				if (bLogBlueprintVisualComponents)
				{
					UE_LOG(LogMajicEventAI, Display, TEXT("NPCBlueprintVisualPlay npc=%s component=%s animation=%s animation_path=%s looping=%s play_rate=%.2f skeleton=%s"),
						*GetName(),
						*MeshComponent->GetName(),
						*GetNameSafe(Animation),
						*Animation->GetPathName(),
						bLooping ? TEXT("true") : TEXT("false"),
						PlayRate,
						MeshComponent->GetSkeletalMeshAsset() && MeshComponent->GetSkeletalMeshAsset()->GetSkeleton()
							? *MeshComponent->GetSkeletalMeshAsset()->GetSkeleton()->GetName()
							: TEXT("none"));
				}
			}
			else if (FallbackMeshComponent)
			{
				PlaySingleNodeAnimation(FallbackMeshComponent, Animation, bLooping, PlayRate);
			}
			LogBlueprintVisualComponents(TEXT("after_play_loop"));
		}
		return;
	}

	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		if (CharacterMesh->GetSkeletalMeshAsset())
		{
			CharacterMesh->PlayAnimation(Animation, bLooping);
			CharacterMesh->SetPlayRate(PlayRate);
		}
	}
}

void AEventNPCActor::PlaySimpleNPCAnimationAtHoldFrame(UAnimationAsset* Animation, float NormalizedTime)
{
	if (!Animation)
	{
		return;
	}

	if (bUsingBlueprintVisual && BlueprintVisualComponent)
	{
		if (AActor* VisualActor = BlueprintVisualComponent->GetChildActor())
		{
			LogBlueprintVisualComponents(TEXT("before_play_hold"));
			USkeletalMeshComponent* FallbackMeshComponent = nullptr;
			if (USkeletalMeshComponent* MeshComponent = FindMetaHumanBodyComponent(VisualActor, FallbackMeshComponent))
			{
				const float AnimationLength = FMath::Max(Animation->GetPlayLength(), 0.0f);
				const float HoldPosition = AnimationLength * FMath::Clamp(NormalizedTime, 0.0f, 1.0f);
				HoldSingleNodeAnimation(MeshComponent, Animation, NormalizedTime);
				if (bLogBlueprintVisualComponents)
				{
					UE_LOG(LogMajicEventAI, Display, TEXT("NPCBlueprintVisualHold npc=%s component=%s animation=%s animation_path=%s normalized=%.2f time=%.2f skeleton=%s"),
						*GetName(),
						*MeshComponent->GetName(),
						*GetNameSafe(Animation),
						*Animation->GetPathName(),
						NormalizedTime,
						HoldPosition,
						MeshComponent->GetSkeletalMeshAsset() && MeshComponent->GetSkeletalMeshAsset()->GetSkeleton()
							? *MeshComponent->GetSkeletalMeshAsset()->GetSkeleton()->GetName()
							: TEXT("none"));
				}
			}
			else if (FallbackMeshComponent)
			{
				HoldSingleNodeAnimation(FallbackMeshComponent, Animation, NormalizedTime);
			}
			LogBlueprintVisualComponents(TEXT("after_play_hold"));
		}
		return;
	}

	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		if (CharacterMesh->GetSkeletalMeshAsset())
		{
			CharacterMesh->PlayAnimation(Animation, false);
			const float AnimationLength = FMath::Max(Animation->GetPlayLength(), 0.0f);
			const float HoldPosition = AnimationLength * FMath::Clamp(NormalizedTime, 0.0f, 1.0f);
			CharacterMesh->SetPosition(HoldPosition, false);
			CharacterMesh->SetPlayRate(0.0f);
		}
	}
}

void AEventNPCActor::LogBlueprintVisualComponents(const TCHAR* Context) const
{
	if (!bLogBlueprintVisualComponents || !BlueprintVisualComponent)
	{
		return;
	}

	const AActor* VisualActor = BlueprintVisualComponent->GetChildActor();
	if (!VisualActor)
	{
		UE_LOG(LogMajicEventAI, Display, TEXT("NPCBlueprintVisualComponents npc=%s context=%s child_actor=none"),
			*GetName(),
			Context ? Context : TEXT("unknown"));
		return;
	}

	TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
	VisualActor->GetComponents<USkeletalMeshComponent>(SkeletalMeshComponents);
	UE_LOG(LogMajicEventAI, Display, TEXT("NPCBlueprintVisualComponents npc=%s context=%s child_actor=%s skeletal_components=%d"),
		*GetName(),
		Context ? Context : TEXT("unknown"),
		*VisualActor->GetName(),
		SkeletalMeshComponents.Num());

	for (USkeletalMeshComponent* MeshComponent : SkeletalMeshComponents)
	{
		if (!MeshComponent)
		{
			continue;
		}

		const USkeletalMesh* SkeletalMesh = MeshComponent->GetSkeletalMeshAsset();
		const USkeleton* Skeleton = SkeletalMesh ? SkeletalMesh->GetSkeleton() : nullptr;
		const USkeletalMeshComponent* Leader = Cast<USkeletalMeshComponent>(MeshComponent->LeaderPoseComponent.Get());
		UE_LOG(LogMajicEventAI, Display, TEXT("NPCBlueprintVisualComponent npc=%s context=%s component=%s mesh=%s skeleton=%s anim_mode=%d anim_class=%s post_process_disabled=%s leader=%s hidden=%s visible=%s"),
			*GetName(),
			Context ? Context : TEXT("unknown"),
			*MeshComponent->GetName(),
			*GetNameSafe(SkeletalMesh),
			*GetNameSafe(Skeleton),
			static_cast<int32>(MeshComponent->GetAnimationMode()),
			*GetNameSafe(MeshComponent->GetAnimClass()),
			MeshComponent->GetDisablePostProcessBlueprint() ? TEXT("true") : TEXT("false"),
			*GetNameSafe(Leader),
			MeshComponent->bHiddenInGame ? TEXT("true") : TEXT("false"),
			MeshComponent->IsVisible() ? TEXT("true") : TEXT("false"));
	}
}

UAnimationAsset* AEventNPCActor::ResolveInteractionAnimation(const FGameplayTag& InteractionTag) const
{
	const FGameplayTag BarOrderTag = FGameplayTag::RequestGameplayTag(FName(TEXT("EventInteract.Bar.Order")), false);
	const FGameplayTag CafeSitTag = FGameplayTag::RequestGameplayTag(FName(TEXT("EventInteract.Cafe.Sit")), false);
	const FGameplayTag LoungeSitTag = FGameplayTag::RequestGameplayTag(FName(TEXT("EventInteract.Lounge.Sit")), false);
	const FGameplayTag StageWatchTag = FGameplayTag::RequestGameplayTag(FName(TEXT("EventInteract.Stage.Watch")), false);
	const FGameplayTag SocialTalkTag = FGameplayTag::RequestGameplayTag(FName(TEXT("EventInteract.Social.Talk")), false);

	if (InteractionTag == BarOrderTag)
	{
		return LoadedBarIdleAnimation ? LoadedBarIdleAnimation.Get() : LoadedIdleAnimation.Get();
	}

	if (InteractionTag == CafeSitTag)
	{
		if (LoadedCafeIdleAnimation)
		{
			return LoadedCafeIdleAnimation.Get();
		}
		return LoadedTalkAnimation ? LoadedTalkAnimation.Get() : LoadedIdleAnimation.Get();
	}

	if (InteractionTag == LoungeSitTag)
	{
		return LoadedSitAnimation ? LoadedSitAnimation.Get() : LoadedIdleAnimation.Get();
	}

	if (InteractionTag == StageWatchTag)
	{
		return LoadedStageWatchAnimation ? LoadedStageWatchAnimation.Get() : LoadedIdleAnimation.Get();
	}

	if (InteractionTag == SocialTalkTag)
	{
		return LoadedTalkAnimation ? LoadedTalkAnimation.Get() : LoadedIdleAnimation.Get();
	}

	return LoadedIdleAnimation.Get();
}

void AEventNPCActor::ApplyInteractionVisualCalibration(const FGameplayTag& InteractionTag)
{
	USceneComponent* ActiveVisualComponent = bUsingBlueprintVisual
		? Cast<USceneComponent>(BlueprintVisualComponent)
		: Cast<USceneComponent>(GetMesh());
	if (!ActiveVisualComponent)
	{
		return;
	}

	const FGameplayTag BarOrderTag = FGameplayTag::RequestGameplayTag(FName(TEXT("EventInteract.Bar.Order")), false);
	const FGameplayTag CafeSitTag = FGameplayTag::RequestGameplayTag(FName(TEXT("EventInteract.Cafe.Sit")), false);
	const FGameplayTag LoungeSitTag = FGameplayTag::RequestGameplayTag(FName(TEXT("EventInteract.Lounge.Sit")), false);
	const FGameplayTag StageWatchTag = FGameplayTag::RequestGameplayTag(FName(TEXT("EventInteract.Stage.Watch")), false);
	const FGameplayTag SocialTalkTag = FGameplayTag::RequestGameplayTag(FName(TEXT("EventInteract.Social.Talk")), false);

	FVector AdditionalOffset = FVector::ZeroVector;
	FRotator AdditionalRotation = FRotator::ZeroRotator;

	if (InteractionTag == BarOrderTag)
	{
		AdditionalOffset = NPCBarIdleAnimationMeshOffset;
		AdditionalRotation = NPCBarIdleAnimationMeshRotation;
	}
	else if (InteractionTag == CafeSitTag)
	{
		AdditionalOffset = NPCCafeIdleAnimationMeshOffset;
		AdditionalRotation = NPCCafeIdleAnimationMeshRotation;
	}
	else if (InteractionTag == LoungeSitTag)
	{
		AdditionalOffset = NPCSitAnimationMeshOffset;
		AdditionalRotation = NPCSitAnimationMeshRotation;
	}
	else if (InteractionTag == StageWatchTag)
	{
		AdditionalOffset = NPCStageWatchAnimationMeshOffset;
		AdditionalRotation = NPCStageWatchAnimationMeshRotation;
	}
	else if (InteractionTag == SocialTalkTag)
	{
		AdditionalOffset = NPCTalkAnimationMeshOffset;
		AdditionalRotation = NPCTalkAnimationMeshRotation;
	}

	ActiveVisualComponent->SetRelativeLocation(ActiveVisualMeshRelativeLocation + AdditionalOffset);
	ActiveVisualComponent->SetRelativeRotation(ActiveVisualMeshRelativeRotation + AdditionalRotation);
}

void AEventNPCActor::ResetNPCVisualCalibration()
{
	USceneComponent* ActiveVisualComponent = bUsingBlueprintVisual
		? Cast<USceneComponent>(BlueprintVisualComponent)
		: Cast<USceneComponent>(GetMesh());
	if (ActiveVisualComponent)
	{
		ActiveVisualComponent->SetRelativeLocation(ActiveVisualMeshRelativeLocation);
		ActiveVisualComponent->SetRelativeRotation(ActiveVisualMeshRelativeRotation);
	}
}

void AEventNPCActor::SetInteractionAnimationState(FGameplayTag InteractionTag)
{
	SetInteractionAnimationStateWithAnimation(InteractionTag, ResolveInteractionAnimation(InteractionTag));
}

void AEventNPCActor::SetInteractionAnimationStateWithAnimation(FGameplayTag InteractionTag, UAnimationAsset* InteractionAnimation)
{
	if (bUsingBlueprintVisual)
	{
		if (UAnimationAsset* BlueprintVisualAnimation = ResolveInteractionAnimation(InteractionTag))
		{
			InteractionAnimation = BlueprintVisualAnimation;
		}
	}

	if (!bUseSkeletalNPCVisual || !bUseSimpleMovementAnimations || !InteractionAnimation)
	{
		return;
	}

	ActiveInteractionAnimationTag = InteractionTag;
	bInteractionAnimationActive = true;
	bNPCVisualPlayingWalk = false;
	ApplyInteractionVisualCalibration(InteractionTag);
	if (bUsingBlueprintVisual && IsSeatedInteractionTag(InteractionTag))
	{
		PlaySimpleNPCAnimation(InteractionAnimation, 1.0f, true);
		return;
	}

	if (bFreezeSeatedInteractionAnimations && IsSeatedInteractionTag(InteractionTag))
	{
		const float HoldNormalizedTime = bUsingBlueprintVisual ? 0.5f : SeatedInteractionAnimationHoldNormalizedTime;
		PlaySimpleNPCAnimationAtHoldFrame(InteractionAnimation, HoldNormalizedTime);
	}
	else
	{
		PlaySimpleNPCAnimation(InteractionAnimation, 1.0f, !IsSeatedInteractionTag(InteractionTag));
	}
}

void AEventNPCActor::ClearInteractionAnimationState()
{
	ActiveInteractionAnimationTag = FGameplayTag();
	bInteractionAnimationActive = false;
	bNPCVisualPlayingWalk = false;
	ResetNPCVisualCalibration();

	if (LoadedIdleAnimation)
	{
		PlaySimpleNPCAnimation(LoadedIdleAnimation, 1.0f, true);
	}
}

void AEventNPCActor::StartLocomotionAnimationState()
{
	UnlockMovementFromInteractionSlot();
	ActiveInteractionAnimationTag = FGameplayTag();
	bInteractionAnimationActive = false;
	ResetNPCVisualCalibration();

	if (!bUsingBlueprintVisual && LoadedWalkAnimation)
	{
		bNPCVisualPlayingWalk = true;
		PlaySimpleNPCAnimation(LoadedWalkAnimation, WalkAnimationPlayRate, true);
	}
	else
	{
		bNPCVisualPlayingWalk = false;
		if (LoadedIdleAnimation)
		{
			PlaySimpleNPCAnimation(LoadedIdleAnimation, 1.0f, true);
		}
	}
}

void AEventNPCActor::LockMovementAtInteractionSlot()
{
	if (bInteractionSlotMovementLocked)
	{
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->DisableMovement();
	}
	bInteractionSlotMovementLocked = true;
	UE_LOG(LogMajicEventAI, Display, TEXT("NPCSeatMovementLocked npc=%s location=%s"),
		*GetName(),
		*GetActorLocation().ToCompactString());
}

void AEventNPCActor::UnlockMovementFromInteractionSlot()
{
	if (!bInteractionSlotMovementLocked)
	{
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->SetMovementMode(bBypassVenueCollisionForPrototype ? MOVE_Flying : MOVE_Walking);
	}
	bInteractionSlotMovementLocked = false;
	UE_LOG(LogMajicEventAI, Display, TEXT("NPCSeatMovementUnlocked npc=%s location=%s"),
		*GetName(),
		*GetActorLocation().ToCompactString());
}

void AEventNPCActor::UpdateNPCVisualAnimation(float DeltaSeconds)
{
	if (bUsingBlueprintVisual || !bUseSkeletalNPCVisual || !bUseSimpleMovementAnimations || !LoadedIdleAnimation || !LoadedWalkAnimation)
	{
		return;
	}

	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (!CharacterMesh || !CharacterMesh->GetSkeletalMeshAsset())
	{
		return;
	}

	const FVector CurrentLocation = GetActorLocation();
	const FVector ActualDisplacement = CurrentLocation - LastVisualAnimationLocation;
	const float ActualPlanarSpeed = DeltaSeconds > SMALL_NUMBER
		? FVector::Dist2D(CurrentLocation, LastVisualAnimationLocation) / DeltaSeconds
		: 0.0f;
	LastVisualAnimationLocation = CurrentLocation;

	if (bInteractionAnimationActive)
	{
		if (!bInteractionSlotMovementLocked)
		{
			UpdateMovementStallLogging(DeltaSeconds, ActualPlanarSpeed, ActualDisplacement);
		}
		return;
	}

	const bool bShouldPlayWalk = ActualPlanarSpeed > WalkingAnimationSpeedThreshold;
	if (bShouldPlayWalk == bNPCVisualPlayingWalk)
	{
		UpdateMovementStallLogging(DeltaSeconds, ActualPlanarSpeed, ActualDisplacement);
		return;
	}

	bNPCVisualPlayingWalk = bShouldPlayWalk;
	ResetNPCVisualCalibration();
	PlaySimpleNPCAnimation(bShouldPlayWalk ? LoadedWalkAnimation : LoadedIdleAnimation, bShouldPlayWalk ? WalkAnimationPlayRate : 1.0f, true);
	UpdateMovementStallLogging(DeltaSeconds, ActualPlanarSpeed, ActualDisplacement);
}

void AEventNPCActor::UpdateMovementStallLogging(float DeltaSeconds, float ActualPlanarSpeed, const FVector& ActualDisplacement)
{
	if ((!bLogMovementStalls && !bLogMovementDeviation) || DeltaSeconds <= SMALL_NUMBER)
	{
		return;
	}

	const AAIController* AIController = Cast<AAIController>(GetController());
	const bool bPathWantsMovement = AIController && AIController->GetMoveStatus() == EPathFollowingStatus::Moving;
	const bool bLooksStalled = bPathWantsMovement &&
		ActualPlanarSpeed <= MovementStallSpeedThreshold;

	if (bLooksStalled)
	{
		MovementStallElapsedTime += DeltaSeconds;
		MovementStallTotalTime += DeltaSeconds;

		int32 NearbyNPCCount = 0;
		FVector EscapeDirection = FVector::ZeroVector;
		const float NearbyRadiusSq = FMath::Square(MovementStallNearbyNPCRadius);
		if (UWorld* World = GetWorld())
		{
			for (TActorIterator<AEventNPCActor> It(World); It; ++It)
			{
				const AEventNPCActor* OtherNPC = *It;
				if (!OtherNPC || OtherNPC == this)
				{
					continue;
				}

				const FVector ToSelf = GetActorLocation() - OtherNPC->GetActorLocation();
				if (FVector::DistSquared2D(OtherNPC->GetActorLocation(), GetActorLocation()) <= NearbyRadiusSq)
				{
					++NearbyNPCCount;
					EscapeDirection += FVector(ToSelf.X, ToSelf.Y, 0.0f).GetSafeNormal();
				}
			}
		}

		if (ClaimedSmartObject && ClaimedSmartObject->GetOwner())
		{
			const FVector AwayFromClaimedObject = GetActorLocation() - ClaimedSmartObject->GetOwner()->GetActorLocation();
			EscapeDirection += FVector(AwayFromClaimedObject.X, AwayFromClaimedObject.Y, 0.0f).GetSafeNormal();
		}

		if (EscapeDirection.IsNearlyZero())
		{
			EscapeDirection = GetActorForwardVector().RotateAngleAxis(FMath::RandBool() ? 90.0f : -90.0f, FVector::UpVector);
		}

		if (!bMovementStallLoggedActive && MovementStallElapsedTime >= MovementStallLogAfterSeconds)
		{
			++MovementStallEventCount;
			bMovementStallLoggedActive = true;
			UE_LOG(LogMajicEventAI, Warning, TEXT("NPCMovementStallStart npc=%s current=%s actual_speed=%.1f move_status=%d nearby_npcs=%d radius=%.0f claimed=%s slot=%d total_stall_time=%.1f count=%d"),
				*GetName(),
				*GetActorLocation().ToCompactString(),
				ActualPlanarSpeed,
				AIController ? static_cast<int32>(AIController->GetMoveStatus()) : -1,
				NearbyNPCCount,
				MovementStallNearbyNPCRadius,
				ClaimedSmartObject && ClaimedSmartObject->GetOwner() ? *ClaimedSmartObject->GetOwner()->GetName() : TEXT("none"),
				ClaimedSlotIndex,
				MovementStallTotalTime,
				MovementStallEventCount);
		}

		if (!bMovementStallEscapeRequested && MovementStallElapsedTime >= MovementStallEscapeAfterSeconds)
		{
			bMovementStallEscapeRequested = true;
			RememberFailedMoveTarget(GetActorLocation());
			SetPreferredWanderDirection(EscapeDirection);

			if (AAIController* MutableAIController = Cast<AAIController>(GetController()))
			{
				MutableAIController->StopMovement();
			}

			UE_LOG(LogMajicEventAI, Warning, TEXT("NPCMovementStallEscape npc=%s current=%s stall_duration=%.1f nearby_npcs=%d direction=%s claimed=%s slot=%d action=stop_and_repath"),
				*GetName(),
				*GetActorLocation().ToCompactString(),
				MovementStallElapsedTime,
				NearbyNPCCount,
				*EscapeDirection.GetSafeNormal2D().ToCompactString(),
				ClaimedSmartObject && ClaimedSmartObject->GetOwner() ? *ClaimedSmartObject->GetOwner()->GetName() : TEXT("none"),
				ClaimedSlotIndex);
		}
		if (bMovementDeviationLoggedActive)
		{
			UE_LOG(LogMajicEventAI, Display, TEXT("NPCMovementDeviationEnd npc=%s reason=stalled current=%s duration=%.1f total_deviation_time=%.1f count=%d"),
				*GetName(),
				*GetActorLocation().ToCompactString(),
				MovementDeviationElapsedTime,
				MovementDeviationTotalTime,
				MovementDeviationEventCount);
		}
		MovementDeviationElapsedTime = 0.0f;
		bMovementDeviationLoggedActive = false;
		return;
	}

	if (bMovementStallLoggedActive)
	{
		UE_LOG(LogMajicEventAI, Display, TEXT("NPCMovementStallEnd npc=%s current=%s stall_duration=%.1f actual_speed=%.1f total_stall_time=%.1f count=%d"),
			*GetName(),
			*GetActorLocation().ToCompactString(),
			MovementStallElapsedTime,
			ActualPlanarSpeed,
			MovementStallTotalTime,
			MovementStallEventCount);
	}

	MovementStallElapsedTime = 0.0f;
	bMovementStallLoggedActive = false;
	bMovementStallEscapeRequested = false;

	const FVector PlanarActualDisplacement(ActualDisplacement.X, ActualDisplacement.Y, 0.0f);
	const bool bCanCheckDeviation = bLogMovementDeviation &&
		bPathWantsMovement &&
		bHasDebugMoveTarget &&
		ActualPlanarSpeed >= MovementDeviationSpeedThreshold &&
		!PlanarActualDisplacement.IsNearlyZero();

	if (bCanCheckDeviation)
	{
		FVector DesiredDirection = DebugMoveTargetLocation - GetActorLocation();
		DesiredDirection.Z = 0.0f;
		const FVector ActualDirection = PlanarActualDisplacement.GetSafeNormal();
		if (!DesiredDirection.IsNearlyZero())
		{
			DesiredDirection.Normalize();
			const float DirectionAlignment = FVector::DotProduct(DesiredDirection, ActualDirection);
			const float ClampedAlignment = FMath::Clamp(DirectionAlignment, -1.0f, 1.0f);
			const float DeviationAngleDegrees = FMath::RadiansToDegrees(FMath::Acos(ClampedAlignment));
			if (DeviationAngleDegrees >= MovementDeviationAngleThresholdDegrees)
			{
				MovementDeviationElapsedTime += DeltaSeconds;
				MovementDeviationTotalTime += DeltaSeconds;

				if (!bMovementDeviationLoggedActive && MovementDeviationElapsedTime >= MovementDeviationLogAfterSeconds)
				{
					++MovementDeviationEventCount;
					bMovementDeviationLoggedActive = true;
					UE_LOG(LogMajicEventAI, Warning, TEXT("NPCMovementDeviationStart npc=%s reason=%s current=%s target=%s actual_speed=%.1f angle=%.1f alignment=%.2f actual_dir=%s desired_dir=%s claimed=%s slot=%d total_deviation_time=%.1f count=%d"),
						*GetName(),
						*DebugMoveTargetReason.ToString(),
						*GetActorLocation().ToCompactString(),
						*DebugMoveTargetLocation.ToCompactString(),
						ActualPlanarSpeed,
						DeviationAngleDegrees,
						DirectionAlignment,
						*ActualDirection.ToCompactString(),
						*DesiredDirection.ToCompactString(),
						ClaimedSmartObject && ClaimedSmartObject->GetOwner() ? *ClaimedSmartObject->GetOwner()->GetName() : TEXT("none"),
						ClaimedSlotIndex,
						MovementDeviationTotalTime,
						MovementDeviationEventCount);
				}

				if (!bMovementDeviationEscapeRequested && MovementDeviationElapsedTime >= MovementDeviationEscapeAfterSeconds)
				{
					bMovementDeviationEscapeRequested = true;
					RememberFailedMoveTarget(DebugMoveTargetLocation);

					FVector EscapeDirection = ActualDirection;
					if (EscapeDirection.IsNearlyZero())
					{
						EscapeDirection = GetActorLocation() - DebugMoveTargetLocation;
						EscapeDirection.Z = 0.0f;
					}
					if (!EscapeDirection.IsNearlyZero())
					{
						SetPreferredWanderDirection(EscapeDirection);
					}

					if (AAIController* MutableAIController = Cast<AAIController>(GetController()))
					{
						MutableAIController->StopMovement();
					}

					UE_LOG(LogMajicEventAI, Warning, TEXT("NPCMovementDeviationEscape npc=%s reason=%s current=%s target=%s duration=%.1f angle=%.1f alignment=%.2f direction=%s action=stop_and_repath"),
						*GetName(),
						*DebugMoveTargetReason.ToString(),
						*GetActorLocation().ToCompactString(),
						*DebugMoveTargetLocation.ToCompactString(),
						MovementDeviationElapsedTime,
						DeviationAngleDegrees,
						DirectionAlignment,
						*EscapeDirection.GetSafeNormal2D().ToCompactString());
				}
				return;
			}
		}
	}

	if (bMovementDeviationLoggedActive)
	{
		UE_LOG(LogMajicEventAI, Display, TEXT("NPCMovementDeviationEnd npc=%s current=%s target=%s duration=%.1f actual_speed=%.1f total_deviation_time=%.1f count=%d"),
			*GetName(),
			*GetActorLocation().ToCompactString(),
			*DebugMoveTargetLocation.ToCompactString(),
			MovementDeviationElapsedTime,
			ActualPlanarSpeed,
			MovementDeviationTotalTime,
			MovementDeviationEventCount);
	}

	MovementDeviationElapsedTime = 0.0f;
	bMovementDeviationLoggedActive = false;
	bMovementDeviationEscapeRequested = false;
}

void AEventNPCActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bLogMovementStalls && (MovementStallEventCount > 0 || MovementStallTotalTime > 0.0f))
	{
		UE_LOG(LogMajicEventAI, Display, TEXT("NPCMovementStallSummary npc=%s total_stall_time=%.1f count=%d"),
			*GetName(),
			MovementStallTotalTime,
			MovementStallEventCount);
	}
	if (bLogMovementDeviation && (MovementDeviationEventCount > 0 || MovementDeviationTotalTime > 0.0f))
	{
		UE_LOG(LogMajicEventAI, Display, TEXT("NPCMovementDeviationSummary npc=%s total_deviation_time=%.1f count=%d"),
			*GetName(),
			MovementDeviationTotalTime,
			MovementDeviationEventCount);
	}

	ReleaseClaimedSlot();
	if (EventDirector)
	{
		EventDirector->UnregisterNPC(this);
	}

	Super::EndPlay(EndPlayReason);
}

void AEventNPCActor::PruneExpiredFailedMoveTargets()
{
	if (RecentFailedMoveTargets.Num() == 0)
	{
		return;
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	RecentFailedMoveTargets.RemoveAll([CurrentTime](const FNPCFailedMoveTarget& FailedTarget)
	{
		return FailedTarget.ExpireTimeSeconds <= CurrentTime;
	});
}

void AEventNPCActor::RememberFailedMoveTarget(const FVector& WorldLocation)
{
	if (FailedMoveTargetCooldownSeconds <= 0.0f || FailedMoveTargetAvoidRadius <= 0.0f)
	{
		return;
	}

	PruneExpiredFailedMoveTargets();

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	const float AvoidRadiusSq = FMath::Square(FailedMoveTargetAvoidRadius);
	for (FNPCFailedMoveTarget& FailedTarget : RecentFailedMoveTargets)
	{
		if (FVector::DistSquared2D(FailedTarget.Location, WorldLocation) <= AvoidRadiusSq)
		{
			FailedTarget.Location = WorldLocation;
			FailedTarget.ExpireTimeSeconds = CurrentTime + FailedMoveTargetCooldownSeconds;
			return;
		}
	}

	FNPCFailedMoveTarget FailedTarget;
	FailedTarget.Location = WorldLocation;
	FailedTarget.ExpireTimeSeconds = CurrentTime + FailedMoveTargetCooldownSeconds;
	RecentFailedMoveTargets.Add(FailedTarget);

	UE_LOG(LogMajicEventAI, Display, TEXT("NPCFailedMoveTargetRemember npc=%s target=%s avoid_radius=%.0f cooldown=%.1f count=%d"),
		*GetName(),
		*WorldLocation.ToCompactString(),
		FailedMoveTargetAvoidRadius,
		FailedMoveTargetCooldownSeconds,
		RecentFailedMoveTargets.Num());
}

bool AEventNPCActor::IsRecentFailedMoveTarget(const FVector& WorldLocation)
{
	if (FailedMoveTargetCooldownSeconds <= 0.0f || FailedMoveTargetAvoidRadius <= 0.0f)
	{
		return false;
	}

	PruneExpiredFailedMoveTargets();

	const float AvoidRadiusSq = FMath::Square(FailedMoveTargetAvoidRadius);
	for (const FNPCFailedMoveTarget& FailedTarget : RecentFailedMoveTargets)
	{
		if (FVector::DistSquared2D(FailedTarget.Location, WorldLocation) <= AvoidRadiusSq)
		{
			return true;
		}
	}

	return false;
}

void AEventNPCActor::SetDebugMoveTarget(const FVector& WorldLocation, FName MoveReason)
{
	DebugMoveTargetLocation = WorldLocation;
	DebugMoveTargetReason = MoveReason;
	bHasDebugMoveTarget = true;
}

void AEventNPCActor::ClearDebugMoveTarget()
{
	bHasDebugMoveTarget = false;
	DebugMoveTargetLocation = FVector::ZeroVector;
	DebugMoveTargetReason = NAME_None;
	MovementDeviationElapsedTime = 0.0f;
	bMovementDeviationLoggedActive = false;
	bMovementDeviationEscapeRequested = false;
}

void AEventNPCActor::SetPersonality(const FNPCPersonality& InPersonality)
{
	Personality = InPersonality;

	AEventNPCController* NPCController = Cast<AEventNPCController>(GetController());
	if (NPCController)
	{
		NPCController->PushPersonalityToBlackboard();
	}
}

void AEventNPCActor::SetEventDirector(AEventDirectorActor* Director)
{
	EventDirector = Director;
	if (Director)
	{
		Director->RegisterNPC(this);
	}
}

void AEventNPCActor::SetClaimedSlot(UEventSmartObjectComponent* SmartObject, int32 SlotIndex)
{
	ClearInteractionAnimationState();
	ClaimedSmartObject = SmartObject;
	ClaimedSlotIndex = SlotIndex;
	UpdateDebugLabel();
}

void AEventNPCActor::UpdateDebugLabel()
{
	if (!DebugLabelComponent)
	{
		return;
	}

	DebugLabelComponent->SetVisibility(bShowDebugNPCLabels);
	DebugLabelComponent->SetHiddenInGame(!bShowDebugNPCLabels);
	if (!bShowDebugNPCLabels)
	{
		return;
	}

	const AActor* SmartObjectOwner = ClaimedSmartObject ? ClaimedSmartObject->GetOwner() : nullptr;
	const FString SmartObjectName = SmartObjectOwner ? SmartObjectOwner->GetName() : FString(TEXT("no-slot"));
	const FString ShortSmartObjectName = SmartObjectName.Len() > 34 ? SmartObjectName.Right(34) : SmartObjectName;
	const FString LabelText = FString::Printf(TEXT("%s\n%s\nslot %d"), *GetName(), *ShortSmartObjectName, ClaimedSlotIndex);
	DebugLabelComponent->SetText(FText::FromString(LabelText));
}

void AEventNPCActor::ReleaseClaimedSlot(bool bClearInteractionAnimation)
{
	if (bClearInteractionAnimation)
	{
		ClearInteractionAnimationState();
	}
	if (ClaimedSmartObject)
	{
		const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
		const AActor* SmartObjectOwner = ClaimedSmartObject->GetOwner();
		const FString SmartObjectName = SmartObjectOwner ? SmartObjectOwner->GetName() : FString(TEXT("unknown"));
		RecentlyVisitedSmartObject = ClaimedSmartObject;
		RecentlyVisitedSmartObjectTimeSeconds = CurrentTime;
		RecentlyUsedInteractionTag = ClaimedSmartObject->InteractionTag;
		RecentlyUsedInteractionTagTimeSeconds = CurrentTime;
		if (SmartObjectOwner)
		{
			LastReleasedSmartObjectLocation = SmartObjectOwner->GetActorLocation();
			bHasLastReleasedSmartObjectLocation = true;

			FVector ExitDirection = FVector::ZeroVector;
			FTransform ClaimedSlotTransform;
			if (ClaimedSmartObject->GetSlotWorldTransform(ClaimedSlotIndex, ClaimedSlotTransform))
			{
				ExitDirection = -ClaimedSlotTransform.GetRotation().GetForwardVector();
			}
			if (ExitDirection.IsNearlyZero())
			{
				ExitDirection = GetActorLocation() - LastReleasedSmartObjectLocation;
			}
			ExitDirection.Z = 0.0f;
			if (ExitDirection.IsNearlyZero())
			{
				ExitDirection = GetActorForwardVector();
				ExitDirection.Z = 0.0f;
			}
			if (!ExitDirection.IsNearlyZero())
			{
				ExitDirection = ExitDirection.GetSafeNormal();
				if (bClearInteractionAnimation)
				{
					SetActorRotation(ExitDirection.Rotation());
				}
				SetPreferredWanderDirection(ExitDirection);
				UE_LOG(LogMajicEventAI, Display, TEXT("ForcedWanderFacing npc=%s after=%s direction=%s yaw=%.1f"),
					*GetName(),
					*SmartObjectName,
					*ExitDirection.ToCompactString(),
					ExitDirection.Rotation().Yaw);
			}
		}
		RequiredWanderStartTimeSeconds = CurrentTime;
		RequiredWanderAccumulatedDistance = 0.0f;
		LastWanderProgressLocation = GetActorLocation();
		bRequiresWanderBeforeNextSmartObject = RequiredWanderAfterSmartObjectSeconds > 0.0f || RequiredWanderAfterSmartObjectDistance > 0.0f;
		UE_LOG(LogMajicEventAI, Display, TEXT("ForcedWanderRequired npc=%s after=%s required_time=%.1f required_distance=%.0f"),
			*GetName(),
			*SmartObjectName,
			RequiredWanderAfterSmartObjectSeconds,
			RequiredWanderAfterSmartObjectDistance);
		ClaimedSmartObject->ReleaseSlot(this);
	}
	ClaimedSmartObject = nullptr;
	ClaimedSlotIndex = INDEX_NONE;
	UpdateDebugLabel();
}

void AEventNPCActor::MarkClaimedSlotReadyToLeave()
{
	if (!ClaimedSmartObject)
	{
		return;
	}

	if (ShouldHoldClaimedSocialSlots())
	{
		const AActor* SmartObjectOwner = ClaimedSmartObject->GetOwner();
		UE_LOG(LogMajicEventAI, Display, TEXT("SmartObjectHold npc=%s smart_object=%s reason=static_social_guest"),
			*GetName(),
			SmartObjectOwner ? *SmartObjectOwner->GetName() : TEXT("unknown"));
		return;
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	const AActor* SmartObjectOwner = ClaimedSmartObject->GetOwner();
	const FString SmartObjectName = SmartObjectOwner ? SmartObjectOwner->GetName() : FString(TEXT("unknown"));
	RecentlyVisitedSmartObject = ClaimedSmartObject;
	RecentlyVisitedSmartObjectTimeSeconds = CurrentTime;
	RecentlyUsedInteractionTag = ClaimedSmartObject->InteractionTag;
	RecentlyUsedInteractionTagTimeSeconds = CurrentTime;

	if (SmartObjectOwner)
	{
		LastReleasedSmartObjectLocation = SmartObjectOwner->GetActorLocation();
		bHasLastReleasedSmartObjectLocation = true;

		FVector ExitDirection = FVector::ZeroVector;
		FTransform ClaimedSlotTransform;
		if (ClaimedSmartObject->GetSlotWorldTransform(ClaimedSlotIndex, ClaimedSlotTransform))
		{
			ExitDirection = -ClaimedSlotTransform.GetRotation().GetForwardVector();
		}
		if (ExitDirection.IsNearlyZero())
		{
			ExitDirection = GetActorLocation() - LastReleasedSmartObjectLocation;
		}
		ExitDirection.Z = 0.0f;
		if (ExitDirection.IsNearlyZero())
		{
			ExitDirection = GetActorForwardVector();
			ExitDirection.Z = 0.0f;
		}
		if (!ExitDirection.IsNearlyZero())
		{
			ExitDirection = ExitDirection.GetSafeNormal();
			SetPreferredWanderDirection(ExitDirection);
			UE_LOG(LogMajicEventAI, Display, TEXT("ForcedWanderFacing npc=%s after=%s direction=%s yaw=%.1f pending_release=true"),
				*GetName(),
				*SmartObjectName,
				*ExitDirection.ToCompactString(),
				ExitDirection.Rotation().Yaw);
		}
	}

	RequiredWanderStartTimeSeconds = CurrentTime;
	RequiredWanderAccumulatedDistance = 0.0f;
	LastWanderProgressLocation = GetActorLocation();
	bRequiresWanderBeforeNextSmartObject = RequiredWanderAfterSmartObjectSeconds > 0.0f || RequiredWanderAfterSmartObjectDistance > 0.0f;
	UE_LOG(LogMajicEventAI, Display, TEXT("ForcedWanderRequired npc=%s after=%s required_time=%.1f required_distance=%.0f pending_release=true"),
		*GetName(),
		*SmartObjectName,
		RequiredWanderAfterSmartObjectSeconds,
		RequiredWanderAfterSmartObjectDistance);
}

bool AEventNPCActor::CanWander() const
{
	return bAllowWander;
}

bool AEventNPCActor::ShouldHoldClaimedSocialSlots() const
{
	return bHoldClaimedSocialSlots && !bAllowWander;
}

void AEventNPCActor::SetPreferredWanderDirection(const FVector& WorldDirection)
{
	FVector PlanarDirection = WorldDirection;
	PlanarDirection.Z = 0.0f;
	if (PlanarDirection.IsNearlyZero())
	{
		return;
	}

	PreferredWanderDirection = PlanarDirection.GetSafeNormal();
	bHasPreferredWanderDirection = true;
}

bool AEventNPCActor::ConsumePreferredWanderDirection(FVector& OutWorldDirection)
{
	if (!bHasPreferredWanderDirection || PreferredWanderDirection.IsNearlyZero())
	{
		return false;
	}

	OutWorldDirection = PreferredWanderDirection;
	bHasPreferredWanderDirection = false;
	PreferredWanderDirection = FVector::ZeroVector;
	return true;
}

bool AEventNPCActor::CanClaimSmartObject(const UEventSmartObjectComponent* SmartObject) const
{
	if (!SmartObject || RecentlyVisitedSmartObjectCooldown <= 0.0f)
	{
		if (!SmartObject || RecentlyUsedInteractionTypeCooldown <= 0.0f || !RecentlyUsedInteractionTag.IsValid())
		{
			return true;
		}
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	if (!HasCompletedRequiredWanderAfterSmartObject())
	{
		return false;
	}

	if (RecentlyVisitedSmartObject.Get() == SmartObject &&
		CurrentTime - RecentlyVisitedSmartObjectTimeSeconds < RecentlyVisitedSmartObjectCooldown)
	{
		return false;
	}

	if (RecentlyUsedInteractionTypeCooldown > 0.0f &&
		RecentlyUsedInteractionTag.IsValid() &&
		SmartObject->InteractionTag == RecentlyUsedInteractionTag &&
		CurrentTime - RecentlyUsedInteractionTagTimeSeconds < RecentlyUsedInteractionTypeCooldown)
	{
		return false;
	}

	return true;
}

bool AEventNPCActor::IsFarEnoughFromLastSmartObject(const UEventSmartObjectComponent* SmartObject) const
{
	if (!SmartObject || MinimumNextSmartObjectTravelDistance <= 0.0f || !bHasLastReleasedSmartObjectLocation)
	{
		return true;
	}

	const AActor* SmartObjectOwner = SmartObject->GetOwner();
	if (!SmartObjectOwner)
	{
		return true;
	}

	return FVector::DistSquared2D(SmartObjectOwner->GetActorLocation(), LastReleasedSmartObjectLocation) >=
		FMath::Square(MinimumNextSmartObjectTravelDistance);
}

bool AEventNPCActor::HasCompletedRequiredWanderAfterSmartObject() const
{
	if (!bRequiresWanderBeforeNextSmartObject)
	{
		return true;
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	const bool bMetTime = RequiredWanderAfterSmartObjectSeconds <= 0.0f ||
		CurrentTime - RequiredWanderStartTimeSeconds >= RequiredWanderAfterSmartObjectSeconds;
	const bool bMetDistance = RequiredWanderAfterSmartObjectDistance <= 0.0f ||
		RequiredWanderAccumulatedDistance >= RequiredWanderAfterSmartObjectDistance;

	return bMetTime && bMetDistance;
}

void AEventNPCActor::MarkWanderProgress()
{
	if (!bRequiresWanderBeforeNextSmartObject)
	{
		return;
	}

	const FVector CurrentLocation = GetActorLocation();
	if (LastWanderProgressLocation.IsNearlyZero())
	{
		LastWanderProgressLocation = CurrentLocation;
		return;
	}

	RequiredWanderAccumulatedDistance += FVector::Dist2D(CurrentLocation, LastWanderProgressLocation);
	LastWanderProgressLocation = CurrentLocation;

	if (HasCompletedRequiredWanderAfterSmartObject())
	{
		bRequiresWanderBeforeNextSmartObject = false;
		UE_LOG(LogMajicEventAI, Display, TEXT("ForcedWanderComplete npc=%s elapsed=%.1f distance=%.0f"),
			*GetName(),
			GetWorld() ? GetWorld()->GetTimeSeconds() - RequiredWanderStartTimeSeconds : 0.0f,
			RequiredWanderAccumulatedDistance);
	}
}

void AEventNPCActor::OnEventPhaseChanged(EEventPhase NewPhase)
{
	CurrentPhase = NewPhase;

	AEventNPCController* NPCController = Cast<AEventNPCController>(GetController());
	if (NPCController)
	{
		NPCController->SetBlackboardPhase(static_cast<uint8>(NewPhase));
	}
}
