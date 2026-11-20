#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "EventSmartObjectComponent.generated.h"

class AEventNPCActor;
class UArrowComponent;
class UAnimationAsset;
class USphereComponent;

// ── Interaction type tags ─────────────────────────────────────────────────────
// These are defined as native tags in the module startup.
// Use them in BT decorators to filter which actors an NPC should seek out.
//
//   EventInteract.Bar.Order    — standing at bar waiting to order
//   EventInteract.Lounge.Sit  — seated in lounge furniture
//   EventInteract.Stage.Watch — standing/seated watching the stage
//   EventInteract.Social.Talk — paused to talk with another NPC

// ── A single interaction slot ─────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct FEventInteractionSlot
{
	GENERATED_BODY()

	// World-space offset from the actor root where the NPC should stand/sit
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
	FVector LocalOffset = FVector::ZeroVector;

	// World-space facing direction for the NPC when using this slot
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
	FRotator LocalRotation = FRotator::ZeroRotator;

	// Currently claimed by this NPC — null means available
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Slot")
	TObjectPtr<AEventNPCActor> ClaimedBy = nullptr;
};

// ── Component ─────────────────────────────────────────────────────────────────

UCLASS(ClassGroup = "MajicEventAI", meta = (BlueprintSpawnableComponent))
class MAJICEVENTAIRUNTIME_API UEventSmartObjectComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEventSmartObjectComponent();

	// ── Configuration ─────────────────────────────────────────────────────────

	// What kind of interaction this actor offers — used by NPC BT to find the right actors
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Object")
	FGameplayTag InteractionTag;

	// The slots NPCs can claim on this actor
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Object")
	TArray<FEventInteractionSlot> Slots;

	// Optional local-space approach points. NPCs path here first, then continue to the claimed slot.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Object")
	TArray<FVector> ApproachLocalOffsets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Object")
	TArray<TSoftObjectPtr<UAnimationAsset>> SlotAnimations;

	// How long (seconds) an NPC typically occupies a slot before releasing it
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Object", meta = (ClampMin = "1.0", Units = "s"))
	float BaseOccupancyDuration = 30.0f;

	// Random variance added to occupancy duration so not everyone leaves at the same time
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Object", meta = (ClampMin = "0.0", Units = "s"))
	float OccupancyVariance = 15.0f;

	// ── Auto-populate settings ────────────────────────────────────────────────
	// Set these in the Details panel — no Blueprint or code needed.

	// When true, slots are built automatically from the owner's mesh components on BeginPlay
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Object|Auto Populate")
	bool bAutoPopulateOnBeginPlay = false;

	// Mesh component name substrings to match for seating slots
	// Lounge: ["LoungeSofa", "LoungeChair"]   Stage: ["StageDeck"]
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Object|Auto Populate",
		meta = (EditCondition = "bAutoPopulateOnBeginPlay"))
	TArray<FString> AutoPopulateNameFilters;

	// Facing direction for auto-populated slots
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Object|Auto Populate",
		meta = (EditCondition = "bAutoPopulateOnBeginPlay"))
	FRotator AutoPopulateFacingRotation = FRotator::ZeroRotator;

	// Position offset applied to every auto-populated slot (cm, relative to actor)
	// Use this to nudge all slots up/forward/sideways to match actual seat positions
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Object|Auto Populate",
		meta = (EditCondition = "bAutoPopulateOnBeginPlay"))
	FVector AutoPopulatePositionOffset = FVector::ZeroVector;

	// Draw debug spheres on slots when Play starts
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Object|Auto Populate")
	bool bDrawDebugOnPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Object|Debug")
	bool bUseTestOccupancyTiming = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Object|Debug", meta = (ClampMin = "1.0", Units = "s", EditCondition = "bUseTestOccupancyTiming"))
	float TestOccupancyDuration = 24.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Object|Debug", meta = (ClampMin = "0.0", Units = "s", EditCondition = "bUseTestOccupancyTiming"))
	float TestOccupancyVariance = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Object|Debug")
	bool bEnsureMinimumTestSlots = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Object|Debug", meta = (ClampMin = "0", EditCondition = "bEnsureMinimumTestSlots"))
	int32 MinimumTestSlotCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Object|Debug", meta = (ClampMin = "50.0", Units = "cm", EditCondition = "bEnsureMinimumTestSlots"))
	float TestSlotClusterRadiusCm = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Object|Debug", meta = (Units = "deg"))
	float TestSlotClusterStartAngleDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Object|Facing")
	bool bFaceSlotsTowardLocalPoint = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Object|Facing", meta = (Units = "cm", EditCondition = "bFaceSlotsTowardLocalPoint"))
	FVector SlotFacingLocalPoint = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Object|Facing", meta = (Units = "deg"))
	float SlotFacingYawOffsetDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Object|Debug")
	bool bPreferDistantAvailableSlot = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Object|Capacity")
	bool bLimitOccupancyByPercentage = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Object|Capacity", meta = (ClampMin = "0.05", ClampMax = "1.0", EditCondition = "bLimitOccupancyByPercentage"))
	float MaxOccupancyRatio = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Object|Capacity", meta = (ClampMin = "0.0", Units = "s"))
	float FailedSlotCooldownSeconds = 45.0f;

	// ── Slot claiming ─────────────────────────────────────────────────────────

	// Returns true and fills OutSlotIndex + OutWorldTransform if a slot is available
	UFUNCTION(BlueprintCallable, Category = "Smart Object")
	bool TryClaimSlot(AEventNPCActor* NPC, int32& OutSlotIndex, FTransform& OutWorldTransform);

	UFUNCTION(BlueprintCallable, Category = "Smart Object")
	bool TryClaimSlotByIndex(AEventNPCActor* NPC, int32 SlotIndex, FTransform& OutWorldTransform);

	UFUNCTION(BlueprintPure, Category = "Smart Object")
	bool GetSlotWorldTransform(int32 SlotIndex, FTransform& OutWorldTransform) const;

	UFUNCTION(BlueprintPure, Category = "Smart Object")
	bool IsSlotAvailable(int32 SlotIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Smart Object")
	void MarkSlotFailed(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Smart Object")
	void SetApproachLocalOffsets(const TArray<FVector>& LocalOffsets);

	UFUNCTION(BlueprintCallable, Category = "Smart Object")
	void SetSlotAnimations(const TArray<TSoftObjectPtr<UAnimationAsset>>& Animations);

	UFUNCTION(BlueprintPure, Category = "Smart Object")
	UAnimationAsset* GetSlotAnimation(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category = "Smart Object")
	void GetApproachWorldLocations(TArray<FVector>& OutWorldLocations) const;

	UFUNCTION(BlueprintPure, Category = "Smart Object")
	bool HasApproachLocations() const { return ApproachLocalOffsets.Num() > 0; }

	// Release a previously claimed slot
	UFUNCTION(BlueprintCallable, Category = "Smart Object")
	void ReleaseSlot(AEventNPCActor* NPC);

	// Release all slots — called on phase change to clear the venue
	UFUNCTION(BlueprintCallable, Category = "Smart Object")
	void ReleaseAllSlots();

	// ── Queries ───────────────────────────────────────────────────────────────

	UFUNCTION(BlueprintPure, Category = "Smart Object")
	int32 GetAvailableSlotCount() const;

	UFUNCTION(BlueprintPure, Category = "Smart Object")
	bool HasAvailableSlot() const { return GetAvailableSlotCount() > 0; }

	UFUNCTION(BlueprintPure, Category = "Smart Object")
	int32 GetClaimedSlotCount() const;

	UFUNCTION(BlueprintPure, Category = "Smart Object")
	int32 GetMaxClaimedSlotCount() const;

	UFUNCTION(BlueprintPure, Category = "Smart Object")
	float GetOccupancyDuration() const;

	// ── Helpers ───────────────────────────────────────────────────────────────

	// Add evenly-spaced slots in a line — useful for bar queues
	UFUNCTION(BlueprintCallable, Category = "Smart Object")
	void BuildLineSlots(int32 Count, float SpacingCm, FVector StartLocalOffset, FRotator FacingRotation);

	// Add slots arranged in a small cluster — useful for lounge seating groups
	UFUNCTION(BlueprintCallable, Category = "Smart Object")
	void BuildClusterSlots(int32 Count, float RadiusCm, FRotator FacingRotation);

	// Auto-populate slots from the owner actor's UInstancedStaticMeshComponents.
	// Pass name substrings to filter which components count as seating.
	// Example for lounge: ComponentNameFilters = ["LoungeSofa", "LoungeChair"]
	// Call this in BeginPlay after the actor has built its layout.
	UFUNCTION(BlueprintCallable, Category = "Smart Object")
	void AutoPopulateSlotsFromMeshComponents(const TArray<FString>& ComponentNameFilters, FRotator SlotFacingRotation);

	UFUNCTION(BlueprintCallable, Category = "Smart Object")
	void AutoPopulateSlotsFromMeshComponentsFacingLocalPoint(const TArray<FString>& ComponentNameFilters, FVector FacingLocalPoint, float YawOffsetDegrees = 0.0f);

	UFUNCTION(BlueprintCallable, Category = "Smart Object")
	void SetSlotsFromLocalOffsets(const TArray<FVector>& LocalOffsets, FRotator SlotFacingRotation);

	UFUNCTION(BlueprintCallable, Category = "Smart Object")
	void SetSlotsFromLocalOffsetsAndRotations(const TArray<FVector>& LocalOffsets, const TArray<FRotator>& LocalRotations);

	UFUNCTION(BlueprintCallable, Category = "Smart Object")
	void SetSlotsFromLocalOffsetsFacingLocalPoint(const TArray<FVector>& LocalOffsets, FVector FacingLocalPoint, float YawOffsetDegrees = 0.0f);

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Smart Object|Debug")
	void RefreshEditorPreviewComponents();

	// ── Debug visualization ───────────────────────────────────────────────────

	// Show slot spheres in the viewport — call from Construction Script or BeginPlay
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Smart Object|Debug")
	void DrawDebugSlots();

	// Populate slots from mesh components AND draw them — one click in the editor to preview
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Smart Object|Debug")
	void PreviewSlots();

protected:
	virtual void BeginPlay() override;
	virtual void OnRegister() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

#if WITH_EDITORONLY_DATA
	// Draws slot visuals whenever the actor is moved or properties change in editor
	virtual void OnComponentCreated() override;
#endif

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Object|Debug")
	bool bDrawDebugInEditor = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smart Object|Debug", meta = (ClampMin = "0.02", Units = "s"))
	float EditorDebugDrawInterval = 0.1f;

private:
	float EditorDebugDrawElapsedTime = 0.0f;

	UPROPERTY(Transient)
	TMap<int32, float> SlotCooldownEndTimes;

#if WITH_EDITORONLY_DATA
	UPROPERTY(Transient)
	TArray<TObjectPtr<USphereComponent>> EditorSlotPreviewComponents;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UArrowComponent>> EditorSlotPreviewArrowComponents;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USphereComponent>> EditorApproachPreviewComponents;
#endif
};
