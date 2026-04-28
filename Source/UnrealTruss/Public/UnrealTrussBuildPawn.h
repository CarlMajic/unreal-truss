#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "TrussStructureActor.h"
#include "UnrealTrussBuildPawn.generated.h"

class UBuildItemDataAsset;
class UBuildManagerComponent;
class UBuildMenuWidget;
class ULightPlacementMenuWidget;
class UTargetingPointerComponent;
class ABuildPreviewActor;
class UCameraComponent;
class UFloatingPawnMovement;
class USphereComponent;
class USpringArmComponent;

UCLASS()
class UNREALTRUSS_API AUnrealTrussBuildPawn : public APawn
{
	GENERATED_BODY()

public:
	AUnrealTrussBuildPawn();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USpringArmComponent> SpringArmComponent;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UCameraComponent> CameraComponent;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UFloatingPawnMovement> MovementComponent;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UBuildManagerComponent> BuildManagerComponent;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UTargetingPointerComponent> TargetingPointerComponent;

	UPROPERTY(EditAnywhere, Category = "Build")
	FName PreferredBuildItemId = TEXT("TrussStraight");

	UPROPERTY(EditAnywhere, Category = "Build")
	bool bAutoSelectFirstBuildItem = true;

	UPROPERTY(Transient)
	TObjectPtr<UBuildItemDataAsset> DefaultBuildItem;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBuildItemDataAsset>> AvailableBuildItems;

	UPROPERTY(Transient)
	TObjectPtr<UBuildMenuWidget> BuildMenuWidget;

	UPROPERTY(Transient)
	TObjectPtr<ULightPlacementMenuWidget> LightPlacementMenuWidget;

	UPROPERTY(Transient)
	TObjectPtr<ATrussStructureActor> HoveredTrussActor;

	UPROPERTY(Transient)
	FVector HoveredTrussHitLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	TObjectPtr<ATrussStructureActor> PendingLightTargetTruss;

	UPROPERTY(Transient)
	FVector PendingLightTargetWorldLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	TSubclassOf<AActor> ActiveLightFixtureClass;

	UPROPERTY(Transient)
	ETrussSlingType ActiveLightSlingType = ETrussSlingType::OverSlung;

	UPROPERTY(Transient)
	TObjectPtr<ABuildPreviewActor> LightPreviewActor;

	UPROPERTY(EditAnywhere, Category = "Build")
	TSubclassOf<UBuildMenuWidget> BuildMenuWidgetClass;

	UPROPERTY(EditAnywhere, Category = "Lighting")
	TSubclassOf<ULightPlacementMenuWidget> LightPlacementMenuWidgetClass;

	UPROPERTY(EditAnywhere, Category = "Lighting")
	FString LightingBlueprintFolder = TEXT("/Game/Majic_Gear/Lighting");

	UPROPERTY(Transient)
	TArray<FString> AvailableLightingNames;

	UPROPERTY(Transient)
	TArray<TSubclassOf<AActor>> AvailableLightingClasses;

	bool bLightPlacementModeActive = false;
	float MoveForwardValue = 0.0f;
	float MoveRightValue = 0.0f;
	float MoveUpValue = 0.0f;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void MoveUp(float Value);
	void TurnYaw(float Value);
	void LookUp(float Value);
	void ToggleBuildMode();
	void ToggleBuildMenu();
	void ConfirmBuildPlacement();
	void CancelBuildMode();
	void EditLookedAtTruss();
	void ToggleLightPlacementMode();
	void RotateBuildPositive();
	void RotateBuildNegative();
	void ShowControlsMessage() const;
	UBuildItemDataAsset* FindDefaultBuildItem() const;
	void GatherBuildItems();
	void GatherLightingBlueprints();
	void EnsureBuildMenuWidget();
	void EnsureLightPlacementMenuWidget();
	bool EnsureLightPreviewActor();
	void DestroyLightPreviewActor();
	void SetBuildMenuVisible(bool bVisible);
	void SetLightPlacementMenuVisible(bool bVisible);
	void SetHoveredTrussActor(ATrussStructureActor* NewHoveredActor);
	bool TraceForTrussHit(FHitResult& OutHitResult, ATrussStructureActor*& OutActor) const;
	void BeginLightPlacementSelection();
	void UpdateLightPreview();
	UFUNCTION()
	void HandleBuildItemSelected(UBuildItemDataAsset* SelectedItem);
	UFUNCTION()
	void HandleBuildMenuActionRequested();
	UFUNCTION()
	void HandleLightPlacementActionRequested();
	UFUNCTION()
	void HandleLightPlacementCanceled();
	ATrussStructureActor* TraceForTrussActor() const;
};
