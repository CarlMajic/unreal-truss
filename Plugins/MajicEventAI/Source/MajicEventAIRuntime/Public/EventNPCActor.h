#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "EventDirectorActor.h"
#include "EventNPCActor.generated.h"

class AEventDirectorActor;
class AEventNPCController;
class UBehaviorTree;
class UEventSmartObjectComponent;
class UAnimInstance;
class UAnimationAsset;
class USkeletalMesh;
class UChildActorComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

// ── Personality ───────────────────────────────────────────────────────────────
// Assigned at spawn by the entrance actor. Drives Behavior Tree task weights.

USTRUCT(BlueprintType)
struct FNPCPersonality
{
	GENERATED_BODY()

	// 0-1 draw toward the bar area
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BarAffinity = 0.5f;

	// 0-1 tendency to seek out and talk with other NPCs
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Sociability = 0.5f;

	// 0-1 how quickly this NPC responds to the seating announcement
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Punctuality = 0.5f;

	// 0-1 preference for being close to the stage
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Personality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StageProximity = 0.5f;
};

USTRUCT(BlueprintType)
struct FNPCFailedMoveTarget
{
	GENERATED_BODY()

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "NPC|Movement")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "NPC|Movement")
	float ExpireTimeSeconds = 0.0f;
};

// ── Actor ─────────────────────────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct FEventNPCVisualVariant
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual")
	FName VariantName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual")
	TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual")
	TSoftClassPtr<AActor> VisualBlueprintClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual")
	TSoftClassPtr<UAnimInstance> AnimClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual")
	TSoftObjectPtr<UAnimationAsset> IdleAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual")
	TSoftObjectPtr<UAnimationAsset> WalkAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual|Interactions")
	TSoftObjectPtr<UAnimationAsset> SitAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual|Interactions")
	TSoftObjectPtr<UAnimationAsset> BarIdleAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual|Interactions")
	TSoftObjectPtr<UAnimationAsset> CafeIdleAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual|Interactions")
	TSoftObjectPtr<UAnimationAsset> TalkAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual|Interactions")
	TSoftObjectPtr<UAnimationAsset> StageWatchAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual", meta = (Units = "cm"))
	FVector MeshRelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual")
	FRotator MeshRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual")
	FVector MeshRelativeScale = FVector::OneVector;
};

UCLASS(BlueprintType)
class MAJICEVENTAIRUNTIME_API AEventNPCActor : public ACharacter
{
	GENERATED_BODY()

public:
	AEventNPCActor();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

public:
	// ── Setup — called by EventEntranceActor after spawn ──────────────────────

	UFUNCTION(BlueprintCallable, Category = "NPC")
	void SetPersonality(const FNPCPersonality& InPersonality);

	UFUNCTION(BlueprintCallable, Category = "NPC")
	void SetEventDirector(AEventDirectorActor* Director);

	// ── Phase response — called by EventDirectorActor ─────────────────────────

	UFUNCTION(BlueprintCallable, Category = "NPC")
	void OnEventPhaseChanged(EEventPhase NewPhase);

	// ── Accessors ─────────────────────────────────────────────────────────────

	UFUNCTION(BlueprintPure, Category = "NPC")
	FNPCPersonality GetPersonality() const { return Personality; }

	UFUNCTION(BlueprintPure, Category = "NPC")
	EEventPhase GetCurrentPhase() const { return CurrentPhase; }

	UFUNCTION(BlueprintPure, Category = "NPC")
	AEventDirectorActor* GetEventDirector() const { return EventDirector.Get(); }

	// ── Configuration ─────────────────────────────────────────────────────────

	// Assign a Behavior Tree asset in the Blueprint subclass Details panel
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	TObjectPtr<UBehaviorTree> BehaviorTree;

	// Visible test body for C++-only NPCs. Hide this when a proper character mesh is assigned.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Debug")
	bool bShowDebugPlaceholder = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Debug")
	TObjectPtr<UStaticMeshComponent> DebugPlaceholderMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Visual")
	TObjectPtr<UChildActorComponent> BlueprintVisualComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Debug")
	TObjectPtr<UTextRenderComponent> DebugLabelComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual")
	bool bUseSkeletalNPCVisual = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual")
	TSoftObjectPtr<USkeletalMesh> NPCSkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual")
	TSoftClassPtr<UAnimInstance> NPCAnimClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual|Variants")
	bool bUseRandomVisualVariants = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual|Variants", meta = (EditCondition = "bUseRandomVisualVariants"))
	TArray<FEventNPCVisualVariant> VisualVariants;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "NPC|Visual|Variants")
	int32 SelectedVisualVariantIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual")
	bool bUseSimpleMovementAnimations = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual")
	TSoftObjectPtr<UAnimationAsset> NPCIdleAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual")
	TSoftObjectPtr<UAnimationAsset> NPCWalkAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual|Interactions")
	TSoftObjectPtr<UAnimationAsset> NPCSitAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual|Interactions")
	TSoftObjectPtr<UAnimationAsset> NPCBarIdleAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual|Interactions")
	TSoftObjectPtr<UAnimationAsset> NPCCafeIdleAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual|Interactions")
	TSoftObjectPtr<UAnimationAsset> NPCTalkAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual|Interactions")
	TSoftObjectPtr<UAnimationAsset> NPCStageWatchAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual|Interaction Calibration", meta = (Units = "cm"))
	FVector NPCSitAnimationMeshOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual|Interaction Calibration")
	FRotator NPCSitAnimationMeshRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual|Interaction Calibration", meta = (Units = "cm"))
	FVector NPCBarIdleAnimationMeshOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual|Interaction Calibration")
	FRotator NPCBarIdleAnimationMeshRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual|Interaction Calibration", meta = (Units = "cm"))
	FVector NPCCafeIdleAnimationMeshOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual|Interaction Calibration")
	FRotator NPCCafeIdleAnimationMeshRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual|Interaction Calibration", meta = (Units = "cm"))
	FVector NPCTalkAnimationMeshOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual|Interaction Calibration")
	FRotator NPCTalkAnimationMeshRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual|Interaction Calibration", meta = (Units = "cm"))
	FVector NPCStageWatchAnimationMeshOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual|Interaction Calibration")
	FRotator NPCStageWatchAnimationMeshRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual", meta = (ClampMin = "0.0", Units = "cm/s"))
	float WalkingAnimationSpeedThreshold = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual", meta = (ClampMin = "0.05"))
	float WalkAnimationPlayRate = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual|Interactions")
	bool bFreezeSeatedInteractionAnimations = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual|Interactions", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bFreezeSeatedInteractionAnimations"))
	float SeatedInteractionAnimationHoldNormalizedTime = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual", meta = (Units = "cm"))
	FVector NPCMeshRelativeLocation = FVector(0.0f, 0.0f, -90.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual")
	FRotator NPCMeshRelativeRotation = FRotator(0.0f, -90.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual")
	FVector NPCMeshRelativeScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual")
	bool bHideDebugPlaceholderWhenSkeletalVisualLoaded = true;

	// Prototype bypass for venue shells with oversized collision. Disable when venue collision is cleaned up.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Debug")
	bool bBypassVenueCollisionForPrototype = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Movement")
	bool bUseCrowdAvoidance = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Movement", meta = (ClampMin = "0.0", Units = "cm"))
	float CrowdAvoidanceConsiderationRadius = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Movement", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CrowdAvoidanceWeight = 0.95f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Movement", meta = (ClampMin = "0.0", Units = "cm"))
	float FailedMoveTargetAvoidRadius = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Movement", meta = (ClampMin = "0.0", Units = "s"))
	float FailedMoveTargetCooldownSeconds = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Movement|Debug")
	bool bLogMovementStalls = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Movement|Debug", meta = (ClampMin = "0.0", Units = "cm/s", EditCondition = "bLogMovementStalls"))
	float MovementStallSpeedThreshold = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Movement|Debug", meta = (ClampMin = "0.1", Units = "s", EditCondition = "bLogMovementStalls"))
	float MovementStallLogAfterSeconds = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Movement|Debug", meta = (ClampMin = "0.1", Units = "s", EditCondition = "bLogMovementStalls"))
	float MovementStallEscapeAfterSeconds = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Movement|Debug", meta = (ClampMin = "0.0", Units = "cm", EditCondition = "bLogMovementStalls"))
	float MovementStallNearbyNPCRadius = 160.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Movement|Debug")
	bool bLogMovementDeviation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual|Debug")
	bool bLogBlueprintVisualComponents = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Visual|Debug")
	bool bShowDebugNPCLabels = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Movement|Debug", meta = (ClampMin = "0.0", Units = "cm/s", EditCondition = "bLogMovementDeviation"))
	float MovementDeviationSpeedThreshold = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Movement|Debug", meta = (ClampMin = "0.0", ClampMax = "180.0", Units = "deg", EditCondition = "bLogMovementDeviation"))
	float MovementDeviationAngleThresholdDegrees = 55.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Movement|Debug", meta = (ClampMin = "0.1", Units = "s", EditCondition = "bLogMovementDeviation"))
	float MovementDeviationLogAfterSeconds = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Movement|Debug", meta = (ClampMin = "0.1", Units = "s", EditCondition = "bLogMovementDeviation"))
	float MovementDeviationEscapeAfterSeconds = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Movement")
	bool bAllowWander = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Behavior", meta = (ClampMin = "0.0", Units = "s"))
	float RecentlyVisitedSmartObjectCooldown = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Behavior", meta = (ClampMin = "0.0", Units = "s"))
	float RecentlyUsedInteractionTypeCooldown = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Behavior", meta = (ClampMin = "0.0", Units = "cm"))
	float MinimumNextSmartObjectTravelDistance = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Behavior", meta = (ClampMin = "0.0", Units = "s"))
	float RequiredWanderAfterSmartObjectSeconds = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Behavior", meta = (ClampMin = "0.0", Units = "cm"))
	float RequiredWanderAfterSmartObjectDistance = 915.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Behavior")
	bool bHoldClaimedSocialSlots = true;

	// ── Smart Object slot tracking — written by BT tasks ─────────────────────

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "NPC")
	TObjectPtr<UEventSmartObjectComponent> ClaimedSmartObject;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "NPC")
	int32 ClaimedSlotIndex = INDEX_NONE;

	UFUNCTION(BlueprintCallable, Category = "NPC")
	void SetClaimedSlot(UEventSmartObjectComponent* SmartObject, int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "NPC")
	void ReleaseClaimedSlot(bool bClearInteractionAnimation = true);

	UFUNCTION(BlueprintCallable, Category = "NPC")
	void MarkClaimedSlotReadyToLeave();

	UFUNCTION(BlueprintPure, Category = "NPC")
	bool CanWander() const;

	UFUNCTION(BlueprintPure, Category = "NPC")
	bool ShouldHoldClaimedSocialSlots() const;

	UFUNCTION(BlueprintCallable, Category = "NPC|Visual")
	void SetInteractionAnimationState(FGameplayTag InteractionTag);

	UFUNCTION(BlueprintCallable, Category = "NPC|Visual")
	void SetInteractionAnimationStateWithAnimation(FGameplayTag InteractionTag, UAnimationAsset* InteractionAnimation);

	UFUNCTION(BlueprintCallable, Category = "NPC|Visual")
	void ClearInteractionAnimationState();

	UFUNCTION(BlueprintCallable, Category = "NPC|Visual")
	void StartLocomotionAnimationState();

	UFUNCTION(BlueprintCallable, Category = "NPC|Movement")
	void LockMovementAtInteractionSlot();

	UFUNCTION(BlueprintCallable, Category = "NPC|Movement")
	void UnlockMovementFromInteractionSlot();

	UFUNCTION(BlueprintPure, Category = "NPC")
	bool HasClaimedSlot() const { return ClaimedSmartObject != nullptr && ClaimedSlotIndex != INDEX_NONE; }

	UFUNCTION(BlueprintPure, Category = "NPC")
	UEventSmartObjectComponent* GetClaimedSmartObject() const { return ClaimedSmartObject.Get(); }

	UFUNCTION(BlueprintPure, Category = "NPC")
	int32 GetClaimedSlotIndex() const { return ClaimedSlotIndex; }

	UFUNCTION(BlueprintPure, Category = "NPC")
	bool CanClaimSmartObject(const UEventSmartObjectComponent* SmartObject) const;

	UFUNCTION(BlueprintPure, Category = "NPC")
	bool IsFarEnoughFromLastSmartObject(const UEventSmartObjectComponent* SmartObject) const;

	UFUNCTION(BlueprintPure, Category = "NPC")
	bool HasCompletedRequiredWanderAfterSmartObject() const;

	UFUNCTION(BlueprintCallable, Category = "NPC")
	void MarkWanderProgress();

	UFUNCTION(BlueprintCallable, Category = "NPC")
	void SetPreferredWanderDirection(const FVector& WorldDirection);

	UFUNCTION(BlueprintCallable, Category = "NPC")
	bool ConsumePreferredWanderDirection(FVector& OutWorldDirection);

	UFUNCTION(BlueprintCallable, Category = "NPC")
	void RememberFailedMoveTarget(const FVector& WorldLocation);

	UFUNCTION(BlueprintCallable, Category = "NPC")
	bool IsRecentFailedMoveTarget(const FVector& WorldLocation);

	UFUNCTION(BlueprintCallable, Category = "NPC")
	void SetDebugMoveTarget(const FVector& WorldLocation, FName MoveReason);

	UFUNCTION(BlueprintCallable, Category = "NPC")
	void ClearDebugMoveTarget();

protected:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "NPC")
	FNPCPersonality Personality;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "NPC")
	EEventPhase CurrentPhase = EEventPhase::DoorsClosed;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "NPC")
	TObjectPtr<AEventDirectorActor> EventDirector;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "NPC|Behavior")
	TObjectPtr<UEventSmartObjectComponent> RecentlyVisitedSmartObject;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "NPC|Behavior")
	float RecentlyVisitedSmartObjectTimeSeconds = -FLT_MAX;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "NPC|Behavior")
	FGameplayTag RecentlyUsedInteractionTag;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "NPC|Behavior")
	float RecentlyUsedInteractionTagTimeSeconds = -FLT_MAX;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "NPC|Behavior")
	FVector LastReleasedSmartObjectLocation = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "NPC|Behavior")
	bool bHasLastReleasedSmartObjectLocation = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "NPC|Behavior")
	float RequiredWanderStartTimeSeconds = -FLT_MAX;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "NPC|Behavior")
	float RequiredWanderAccumulatedDistance = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "NPC|Behavior")
	FVector LastWanderProgressLocation = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "NPC|Behavior")
	bool bRequiresWanderBeforeNextSmartObject = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "NPC|Behavior")
	FVector PreferredWanderDirection = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "NPC|Behavior")
	bool bHasPreferredWanderDirection = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "NPC|Movement")
	TArray<FNPCFailedMoveTarget> RecentFailedMoveTargets;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "NPC|Movement")
	FVector DebugMoveTargetLocation = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "NPC|Movement")
	FName DebugMoveTargetReason = NAME_None;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "NPC|Movement")
	bool bHasDebugMoveTarget = false;

	void ConfigureNPCVisual();
	const FEventNPCVisualVariant* SelectVisualVariant();
	void UpdateNPCVisualAnimation(float DeltaSeconds);
	UAnimationAsset* ResolveInteractionAnimation(const FGameplayTag& InteractionTag) const;
	void ApplyInteractionVisualCalibration(const FGameplayTag& InteractionTag);
	void ResetNPCVisualCalibration();
	void PlaySimpleNPCAnimation(UAnimationAsset* Animation, float PlayRate, bool bLooping);
	void PlaySimpleNPCAnimationAtHoldFrame(UAnimationAsset* Animation, float NormalizedTime);
	void LogBlueprintVisualComponents(const TCHAR* Context) const;
	void UpdateDebugLabel();
	void UpdateMovementStallLogging(float DeltaSeconds, float ActualPlanarSpeed, const FVector& ActualDisplacement);
	void PruneExpiredFailedMoveTargets();

	UPROPERTY(Transient)
	TObjectPtr<UAnimationAsset> LoadedIdleAnimation;

	UPROPERTY(Transient)
	TObjectPtr<UAnimationAsset> LoadedWalkAnimation;

	UPROPERTY(Transient)
	TObjectPtr<UAnimationAsset> LoadedSitAnimation;

	UPROPERTY(Transient)
	TObjectPtr<UAnimationAsset> LoadedBarIdleAnimation;

	UPROPERTY(Transient)
	TObjectPtr<UAnimationAsset> LoadedCafeIdleAnimation;

	UPROPERTY(Transient)
	TObjectPtr<UAnimationAsset> LoadedTalkAnimation;

	UPROPERTY(Transient)
	TObjectPtr<UAnimationAsset> LoadedStageWatchAnimation;

	UPROPERTY(Transient)
	FVector LastVisualAnimationLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	FVector ActiveVisualMeshRelativeLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	FRotator ActiveVisualMeshRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(Transient)
	bool bUsingBlueprintVisual = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "NPC|Visual")
	FGameplayTag ActiveInteractionAnimationTag;

	bool bNPCVisualPlayingWalk = false;
	bool bInteractionAnimationActive = false;
	bool bInteractionSlotMovementLocked = false;

	float MovementStallElapsedTime = 0.0f;
	float MovementStallTotalTime = 0.0f;
	int32 MovementStallEventCount = 0;
	bool bMovementStallLoggedActive = false;
	bool bMovementStallEscapeRequested = false;
	float MovementDeviationElapsedTime = 0.0f;
	float MovementDeviationTotalTime = 0.0f;
	int32 MovementDeviationEventCount = 0;
	bool bMovementDeviationLoggedActive = false;
	bool bMovementDeviationEscapeRequested = false;
};
