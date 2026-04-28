#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "UnrealTrussBuildPawn.generated.h"

class UBuildItemDataAsset;
class UBuildManagerComponent;
class UBuildMenuWidget;
class ATrussStructureActor;
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
	TObjectPtr<ATrussStructureActor> HoveredTrussActor;

	UPROPERTY(EditAnywhere, Category = "Build")
	TSubclassOf<UBuildMenuWidget> BuildMenuWidgetClass;

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
	void RotateBuildPositive();
	void RotateBuildNegative();
	void ShowControlsMessage() const;
	UBuildItemDataAsset* FindDefaultBuildItem() const;
	void GatherBuildItems();
	void EnsureBuildMenuWidget();
	void SetBuildMenuVisible(bool bVisible);
	void SetHoveredTrussActor(ATrussStructureActor* NewHoveredActor);
	UFUNCTION()
	void HandleBuildItemSelected(UBuildItemDataAsset* SelectedItem);
	UFUNCTION()
	void HandleBuildMenuActionRequested();
	ATrussStructureActor* TraceForTrussActor() const;
};
