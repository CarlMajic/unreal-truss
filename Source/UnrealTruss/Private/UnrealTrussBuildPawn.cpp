#include "UnrealTrussBuildPawn.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "BuildItemDataAsset.h"
#include "BuildManagerComponent.h"
#include "BuildMenuWidget.h"
#include "BuildPreviewActor.h"
#include "LightPlacementMenuWidget.h"
#include "MBPWallActor.h"
#include "StageDeckActor.h"
#include "TargetingPointerComponent.h"
#include "InputCoreTypes.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/Blueprint.h"
#include "Engine/Engine.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "TrussStructureActor.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

namespace
{
static UBuildItemDataAsset* CreateFallbackMBPBuildItem(UObject* Outer)
{
	UBuildItemDataAsset* BuildItem = NewObject<UBuildItemDataAsset>(Outer, NAME_None, RF_Transient);
	if (!BuildItem)
	{
		return nullptr;
	}

	BuildItem->ItemId = TEXT("MBPWallDefault");
	BuildItem->DisplayName = FText::FromString(TEXT("MBP Wall"));
	BuildItem->Description = FText::FromString(TEXT("Runtime MBP wall builder."));
	BuildItem->Category = TEXT("Backdrop");
	BuildItem->ItemType = EBuildItemType::MBPWall;
	BuildItem->BuildActorClass = AMBPWallActor::StaticClass();
	BuildItem->GridSnapSizeCm = 30.48f;
	BuildItem->RotationStepDegrees = 15.0f;
	BuildItem->bUseGridSnap = true;
	BuildItem->bAlignToSurfaceNormal = false;
	return BuildItem;
}

static UBuildItemDataAsset* CreateFallbackStageBuildItem(UObject* Outer)
{
	UBuildItemDataAsset* BuildItem = NewObject<UBuildItemDataAsset>(Outer, NAME_None, RF_Transient);
	if (!BuildItem)
	{
		return nullptr;
	}

	BuildItem->ItemId = TEXT("StageDeckDefault");
	BuildItem->DisplayName = FText::FromString(TEXT("Stage"));
	BuildItem->Description = FText::FromString(TEXT("Runtime stage builder."));
	BuildItem->Category = TEXT("Stage");
	BuildItem->ItemType = EBuildItemType::StageDeck;
	BuildItem->BuildActorClass = AStageDeckActor::StaticClass();
	BuildItem->GridSnapSizeCm = 121.92f;
	BuildItem->RotationStepDegrees = 90.0f;
	BuildItem->bUseGridSnap = true;
	BuildItem->bAlignToSurfaceNormal = false;
	return BuildItem;
}

static bool HasBuildItemType(const TArray<TObjectPtr<UBuildItemDataAsset>>& BuildItems, EBuildItemType ItemType)
{
	for (const UBuildItemDataAsset* BuildItem : BuildItems)
	{
		if (BuildItem && BuildItem->ItemType == ItemType)
		{
			return true;
		}
	}

	return false;
}
}

AUnrealTrussBuildPawn::AUnrealTrussBuildPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitSphereRadius(34.0f);
	CollisionComponent->SetCollisionProfileName(TEXT("Pawn"));
	SetRootComponent(CollisionComponent);

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArmComponent->SetupAttachment(CollisionComponent);
	SpringArmComponent->TargetArmLength = 0.0f;
	SpringArmComponent->bUsePawnControlRotation = true;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(SpringArmComponent);
	CameraComponent->bUsePawnControlRotation = false;

	MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComponent"));
	MovementComponent->MaxSpeed = 2400.0f;
	MovementComponent->Acceleration = 8000.0f;
	MovementComponent->Deceleration = 12000.0f;

	BuildManagerComponent = CreateDefaultSubobject<UBuildManagerComponent>(TEXT("BuildManagerComponent"));
	TargetingPointerComponent = CreateDefaultSubobject<UTargetingPointerComponent>(TEXT("TargetingPointerComponent"));

	AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void AUnrealTrussBuildPawn::BeginPlay()
{
	Super::BeginPlay();

	GatherBuildItems();
	GatherLightingBlueprints();
	DefaultBuildItem = FindDefaultBuildItem();
	EnsureBuildMenuWidget();
	EnsureLightPlacementMenuWidget();
	ShowControlsMessage();
}

void AUnrealTrussBuildPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const FRotator ControlRotation = Controller ? Controller->GetControlRotation() : GetActorRotation();
	const FVector Forward = FRotationMatrix(FRotator(0.0f, ControlRotation.Yaw, 0.0f)).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(FRotator(0.0f, ControlRotation.Yaw, 0.0f)).GetUnitAxis(EAxis::Y);
	const FVector Up = FVector::UpVector;

	if (!FMath::IsNearlyZero(MoveForwardValue))
	{
		AddMovementInput(Forward, MoveForwardValue);
	}

	if (!FMath::IsNearlyZero(MoveRightValue))
	{
		AddMovementInput(Right, MoveRightValue);
	}

	if (!FMath::IsNearlyZero(MoveUpValue))
	{
		AddMovementInput(Up, MoveUpValue);
	}

	if (BuildManagerComponent && BuildManagerComponent->bBuildModeActive)
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
		{
			BuildManagerComponent->UpdatePreviewFromPlayerView(PlayerController);
		}
	}

	if (bLightPlacementModeActive)
	{
		UpdateLightPreview();
	}

	const bool bBuildMenuVisible = BuildMenuWidget && BuildMenuWidget->GetVisibility() == ESlateVisibility::Visible;
	const bool bLightMenuVisible = LightPlacementMenuWidget && LightPlacementMenuWidget->GetVisibility() == ESlateVisibility::Visible;
	APlayerController* PlayerController = Cast<APlayerController>(GetController());

	if (TargetingPointerComponent && PlayerController)
	{
		AActor* PreviewActorToIgnore = BuildManagerComponent ? BuildManagerComponent->ActivePreviewActor.Get() : nullptr;
		AActor* PreviewChildToIgnore = BuildManagerComponent && BuildManagerComponent->ActivePreviewActor
			? BuildManagerComponent->ActivePreviewActor->GetPreviewActor()
			: nullptr;
		const bool bEditingMBP = BuildMenuWidget && BuildMenuWidget->GetEditingMBPTarget() != nullptr;

		if (!bBuildMenuVisible && !bLightMenuVisible && BuildManagerComponent && BuildManagerComponent->bBuildModeActive)
		{
			TargetingPointerComponent->UpdatePointer(PlayerController, ETargetingPointerMode::WorldPlacement, PreviewActorToIgnore, PreviewChildToIgnore);
		}
		else if (!bBuildMenuVisible && !bLightMenuVisible && bLightPlacementModeActive)
		{
			TargetingPointerComponent->UpdatePointer(PlayerController, ETargetingPointerMode::TrussSelection, PreviewActorToIgnore, PreviewChildToIgnore);
		}
		else if (!bBuildMenuVisible && !bLightMenuVisible && bMBPEditSelectionModeActive)
		{
			TargetingPointerComponent->UpdatePointer(PlayerController, ETargetingPointerMode::WorldPlacement, PreviewActorToIgnore, PreviewChildToIgnore);
		}
		else if (!bBuildMenuVisible && !bLightMenuVisible && bStageEditSelectionModeActive)
		{
			TargetingPointerComponent->UpdatePointer(PlayerController, ETargetingPointerMode::WorldPlacement, PreviewActorToIgnore, PreviewChildToIgnore);
		}
		else if (bBuildMenuVisible && bEditingMBP)
		{
			TargetingPointerComponent->UpdatePointer(PlayerController, ETargetingPointerMode::WorldPlacement, PreviewActorToIgnore, PreviewChildToIgnore);
		}
		else if (bBuildMenuVisible && BuildMenuWidget && BuildMenuWidget->GetEditingStageTarget())
		{
			TargetingPointerComponent->UpdatePointer(PlayerController, ETargetingPointerMode::WorldPlacement, PreviewActorToIgnore, PreviewChildToIgnore);
		}
		else
		{
			TargetingPointerComponent->HidePointer();
		}
	}

	if (bBuildMenuVisible && BuildMenuWidget)
	{
		if (AMBPWallActor* EditingMBPWall = BuildMenuWidget->GetEditingMBPTarget())
		{
			FHitResult HitResult;
			ATrussStructureActor* HitTrussActor = nullptr;
			AMBPWallActor* HitMBPWallActor = nullptr;
			AStageDeckActor* HitStageDeckActor = nullptr;
			if (TraceForEditableActorHit(HitResult, HitTrussActor, HitMBPWallActor, HitStageDeckActor) && HitMBPWallActor == EditingMBPWall)
			{
				int32 TargetRow = 0;
				int32 TargetColumn = 0;
				if (EditingMBPWall->GetSlotIndicesFromWorldLocation(HitResult.ImpactPoint, TargetRow, TargetColumn))
				{
					BuildMenuWidget->SetEditingMBPPanelTarget(TargetRow, TargetColumn);
				}
			}
		}
		else if (AStageDeckActor* EditingStageDeck = BuildMenuWidget->GetEditingStageTarget())
		{
			FHitResult HitResult;
			ATrussStructureActor* HitTrussActor = nullptr;
			AMBPWallActor* HitMBPWallActor = nullptr;
			AStageDeckActor* HitStageDeckActor = nullptr;
			if (TraceForEditableActorHit(HitResult, HitTrussActor, HitMBPWallActor, HitStageDeckActor) && HitStageDeckActor == EditingStageDeck)
			{
				int32 TargetRow = 0;
				int32 TargetColumn = 0;
				if (EditingStageDeck->GetCellIndicesFromWorldLocation(HitResult.ImpactPoint, TargetRow, TargetColumn))
				{
					BuildMenuWidget->SetEditingStageCellTarget(TargetRow, TargetColumn);
				}
			}
		}
	}

	if (!bBuildMenuVisible && !bLightMenuVisible)
	{
		if (TargetingPointerComponent && TargetingPointerComponent->bHasValidHit && TargetingPointerComponent->CurrentTrussActor)
		{
			HoveredTrussHitLocation = TargetingPointerComponent->CurrentHitResult.ImpactPoint;
			SetHoveredTrussActor(TargetingPointerComponent->CurrentTrussActor);
		}
		else
		{
			FHitResult TrussHitResult;
			ATrussStructureActor* TrussActor = nullptr;
			if (TraceForTrussHit(TrussHitResult, TrussActor))
			{
				HoveredTrussHitLocation = TrussHitResult.ImpactPoint;
			}
			SetHoveredTrussActor(TrussActor);
		}
	}
	else
	{
		HoveredTrussHitLocation = FVector::ZeroVector;
		SetHoveredTrussActor(nullptr);
	}
}

void AUnrealTrussBuildPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AUnrealTrussBuildPawn::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AUnrealTrussBuildPawn::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("MoveUp"), this, &AUnrealTrussBuildPawn::MoveUp);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AUnrealTrussBuildPawn::TurnYaw);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AUnrealTrussBuildPawn::LookUp);

	PlayerInputComponent->BindAction(TEXT("ToggleBuildMode"), IE_Pressed, this, &AUnrealTrussBuildPawn::ToggleBuildMode);
	PlayerInputComponent->BindAction(TEXT("ToggleBuildMenu"), IE_Pressed, this, &AUnrealTrussBuildPawn::ToggleBuildMenu);
	PlayerInputComponent->BindAction(TEXT("ConfirmBuild"), IE_Pressed, this, &AUnrealTrussBuildPawn::ConfirmBuildPlacement);
	PlayerInputComponent->BindAction(TEXT("CancelBuild"), IE_Pressed, this, &AUnrealTrussBuildPawn::CancelBuildMode);
	PlayerInputComponent->BindAction(TEXT("EditLookedAtTruss"), IE_Pressed, this, &AUnrealTrussBuildPawn::EditLookedAtTruss);
	PlayerInputComponent->BindAction(TEXT("ToggleLightPlacementMode"), IE_Pressed, this, &AUnrealTrussBuildPawn::ToggleLightPlacementMode);
	PlayerInputComponent->BindAction(TEXT("RotateBuildPositive"), IE_Pressed, this, &AUnrealTrussBuildPawn::RotateBuildPositive);
	PlayerInputComponent->BindAction(TEXT("RotateBuildNegative"), IE_Pressed, this, &AUnrealTrussBuildPawn::RotateBuildNegative);
}

void AUnrealTrussBuildPawn::MoveForward(float Value)
{
	MoveForwardValue = Value;
}

void AUnrealTrussBuildPawn::MoveRight(float Value)
{
	MoveRightValue = Value;
}

void AUnrealTrussBuildPawn::MoveUp(float Value)
{
	MoveUpValue = Value;
}

void AUnrealTrussBuildPawn::TurnYaw(float Value)
{
	AddControllerYawInput(Value);
}

void AUnrealTrussBuildPawn::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void AUnrealTrussBuildPawn::ToggleBuildMode()
{
	if (!BuildManagerComponent)
	{
		return;
	}

	if (BuildManagerComponent->bBuildModeActive)
	{
		BuildManagerComponent->ExitBuildMode();
		return;
	}

	if (!DefaultBuildItem)
	{
		DefaultBuildItem = FindDefaultBuildItem();
	}

	BuildManagerComponent->ClearEditingTrussActor();
	if (BuildMenuWidget)
	{
		if (AMBPWallActor* EditingMBPWall = BuildMenuWidget->GetEditingMBPTarget())
		{
			EditingMBPWall->SetSelectionHighlighted(false);
		}
		if (AStageDeckActor* EditingStageDeck = BuildMenuWidget->GetEditingStageTarget())
		{
			EditingStageDeck->SetSelectionHighlighted(false);
		}
		BuildMenuWidget->SetEditingTarget(nullptr);
		BuildMenuWidget->SetEditingMBPTarget(nullptr);
		BuildMenuWidget->SetEditingStageTarget(nullptr);
	}
	bMBPEditSelectionModeActive = false;
	bStageEditSelectionModeActive = false;
	if (PendingMBPEditWall)
	{
		PendingMBPEditWall->SetSelectionHighlighted(false);
		PendingMBPEditWall = nullptr;
	}
	if (PendingStageEditActor)
	{
		PendingStageEditActor->SetSelectionHighlighted(false);
		PendingStageEditActor = nullptr;
	}

	EnsureBuildMenuWidget();
	if (BuildMenuWidget && BuildMenuWidget->GetSelectedBuildItem())
	{
		DefaultBuildItem = BuildMenuWidget->GetSelectedBuildItem();
	}

	if (!DefaultBuildItem)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("No BuildItemDataAsset found. Create one in Content Browser first."));
		}
		return;
	}

	BuildManagerComponent->SetSelectedBuildItem(DefaultBuildItem);
	if (BuildMenuWidget && DefaultBuildItem->ItemType == EBuildItemType::TrussStructure)
	{
		BuildManagerComponent->SetActiveTrussDefinition(BuildMenuWidget->GetCurrentTrussDefinition());
	}
	else if (BuildMenuWidget && DefaultBuildItem->ItemType == EBuildItemType::MBPWall)
	{
		BuildManagerComponent->SetActiveMBPWallDefinition(BuildMenuWidget->GetCurrentMBPWallDefinition());
	}
	else if (BuildMenuWidget && DefaultBuildItem->ItemType == EBuildItemType::StageDeck)
	{
		BuildManagerComponent->SetActiveStageDeckDefinition(BuildMenuWidget->GetCurrentStageDeckDefinition());
	}
	BuildManagerComponent->EnterBuildMode();
}

void AUnrealTrussBuildPawn::ToggleBuildMenu()
{
	EnsureBuildMenuWidget();
	if (!BuildMenuWidget)
	{
		return;
	}

	const bool bIsVisible = BuildMenuWidget->IsInViewport() && BuildMenuWidget->GetVisibility() == ESlateVisibility::Visible;
	SetBuildMenuVisible(!bIsVisible);
}

void AUnrealTrussBuildPawn::ConfirmBuildPlacement()
{
	if (bLightPlacementModeActive)
	{
		HandleLightPlacementActionRequested();
		return;
	}

	if (bMBPEditSelectionModeActive)
	{
		FHitResult HitResult;
		ATrussStructureActor* HitTrussActor = nullptr;
		AMBPWallActor* HitMBPWallActor = nullptr;
		AStageDeckActor* HitStageDeckActor = nullptr;
		if (PendingMBPEditWall &&
			TraceForEditableActorHit(HitResult, HitTrussActor, HitMBPWallActor, HitStageDeckActor) &&
			HitMBPWallActor == PendingMBPEditWall)
		{
			int32 TargetRow = 0;
			int32 TargetColumn = 0;
			HitMBPWallActor->GetSlotIndicesFromWorldLocation(HitResult.ImpactPoint, TargetRow, TargetColumn);
			EnsureBuildMenuWidget();
			if (BuildMenuWidget)
			{
				BuildMenuWidget->SetEditingMBPTarget(HitMBPWallActor, TargetRow, TargetColumn);
			}
			SetBuildMenuVisible(true);
		}

		bMBPEditSelectionModeActive = false;
		return;
	}

	if (bStageEditSelectionModeActive)
	{
		FHitResult HitResult;
		ATrussStructureActor* HitTrussActor = nullptr;
		AMBPWallActor* HitMBPWallActor = nullptr;
		AStageDeckActor* HitStageDeckActor = nullptr;
		if (PendingStageEditActor &&
			TraceForEditableActorHit(HitResult, HitTrussActor, HitMBPWallActor, HitStageDeckActor) &&
			HitStageDeckActor == PendingStageEditActor)
		{
			int32 TargetRow = 0;
			int32 TargetColumn = 0;
			PendingStageEditActor->GetCellIndicesFromWorldLocation(HitResult.ImpactPoint, TargetRow, TargetColumn);
			EnsureBuildMenuWidget();
			if (BuildMenuWidget)
			{
				BuildMenuWidget->SetEditingStageTarget(HitStageDeckActor, TargetRow, TargetColumn);
			}
			SetBuildMenuVisible(true);
		}

		bStageEditSelectionModeActive = false;
		return;
	}

	if (!BuildManagerComponent || !BuildManagerComponent->bBuildModeActive)
	{
		return;
	}

	const FBuildPlacementResult PlacementResult = BuildManagerComponent->ConfirmPlacement();
	if (GEngine)
	{
		const FString Message = PlacementResult.bSuccess
			? TEXT("Placed build actor.")
			: TEXT("Placement failed. Aim at valid world geometry first.");
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, PlacementResult.bSuccess ? FColor::Green : FColor::Yellow, Message);
	}
}

void AUnrealTrussBuildPawn::CancelBuildMode()
{
	if (BuildManagerComponent)
	{
		BuildManagerComponent->ExitBuildMode();
		BuildManagerComponent->ClearEditingTrussActor();
	}

	if (BuildMenuWidget)
	{
		if (AMBPWallActor* EditingMBPWall = BuildMenuWidget->GetEditingMBPTarget())
		{
			EditingMBPWall->SetSelectionHighlighted(false);
		}
		if (AStageDeckActor* EditingStageDeck = BuildMenuWidget->GetEditingStageTarget())
		{
			EditingStageDeck->SetSelectionHighlighted(false);
		}
		BuildMenuWidget->SetEditingTarget(nullptr);
		BuildMenuWidget->SetEditingMBPTarget(nullptr);
		BuildMenuWidget->SetEditingStageTarget(nullptr);
	}
	bMBPEditSelectionModeActive = false;
	bStageEditSelectionModeActive = false;
	PendingMBPEditWall = nullptr;
	PendingStageEditActor = nullptr;

	bLightPlacementModeActive = false;
	ActiveLightFixtureClass = nullptr;
	PendingLightTargetTruss = nullptr;
	PendingLightTargetWorldLocation = FVector::ZeroVector;
	DestroyLightPreviewActor();
	SetLightPlacementMenuVisible(false);
	SetHoveredTrussActor(nullptr);
}

void AUnrealTrussBuildPawn::EditLookedAtTruss()
{
	FHitResult HitResult;
	ATrussStructureActor* TrussActor = nullptr;
	AMBPWallActor* MBPWallActor = nullptr;
	AStageDeckActor* StageDeckActor = nullptr;
	if (!TraceForEditableActorHit(HitResult, TrussActor, MBPWallActor, StageDeckActor) || !BuildManagerComponent)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("No editable truss, MBP wall, or stage found under the view."));
		}
		return;
	}

	EnsureBuildMenuWidget();
	if (TrussActor)
	{
		BuildManagerComponent->SetEditingTrussActor(TrussActor);
		BuildManagerComponent->EnterBuildMode();

		if (BuildMenuWidget)
		{
			BuildMenuWidget->SetEditingTarget(TrussActor);
		}
	}
	else if (MBPWallActor)
	{
		BuildManagerComponent->ExitBuildMode();
		BuildManagerComponent->ClearEditingTrussActor();
		MBPWallActor->SetSelectionHighlighted(true);
		PendingMBPEditWall = MBPWallActor;
		bMBPEditSelectionModeActive = true;
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Cyan, TEXT("Aim at an MBP panel and left click to open edit controls."));
		}
		return;
	}
	else if (StageDeckActor)
	{
		BuildManagerComponent->ExitBuildMode();
		BuildManagerComponent->ClearEditingTrussActor();
		StageDeckActor->SetSelectionHighlighted(true);
		PendingStageEditActor = StageDeckActor;
		bStageEditSelectionModeActive = true;
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Cyan, TEXT("Aim at a stage cell and left click to open stage edit controls."));
		}
		return;
	}

	SetBuildMenuVisible(true);
}

void AUnrealTrussBuildPawn::ToggleLightPlacementMode()
{
	const bool bMenuVisible = LightPlacementMenuWidget && LightPlacementMenuWidget->GetVisibility() == ESlateVisibility::Visible;
	if (bLightPlacementModeActive || bMenuVisible)
	{
		bLightPlacementModeActive = false;
		ActiveLightFixtureClass = nullptr;
		PendingLightTargetTruss = nullptr;
		PendingLightTargetWorldLocation = FVector::ZeroVector;
		DestroyLightPreviewActor();
		SetLightPlacementMenuVisible(false);
		if (LightPlacementMenuWidget)
		{
			LightPlacementMenuWidget->SetPlacementReady(false);
			LightPlacementMenuWidget->SetTargetTruss(nullptr);
		}
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("Light placement canceled."));
		}
		return;
	}

	if (BuildManagerComponent)
	{
		BuildManagerComponent->ExitBuildMode();
		BuildManagerComponent->ClearEditingTrussActor();
	}

	if (BuildMenuWidget)
	{
		if (AMBPWallActor* EditingMBPWall = BuildMenuWidget->GetEditingMBPTarget())
		{
			EditingMBPWall->SetSelectionHighlighted(false);
		}
		if (AStageDeckActor* EditingStageDeck = BuildMenuWidget->GetEditingStageTarget())
		{
			EditingStageDeck->SetSelectionHighlighted(false);
		}
		BuildMenuWidget->SetEditingTarget(nullptr);
		BuildMenuWidget->SetEditingMBPTarget(nullptr);
		BuildMenuWidget->SetEditingStageTarget(nullptr);
	}
	PendingMBPEditWall = nullptr;
	PendingStageEditActor = nullptr;
	bMBPEditSelectionModeActive = false;
	bStageEditSelectionModeActive = false;

	SetBuildMenuVisible(false);
	EnsureLightPlacementMenuWidget();
	if (LightPlacementMenuWidget)
	{
		LightPlacementMenuWidget->SetPlacementReady(false);
		LightPlacementMenuWidget->SetTargetTruss(nullptr);
	}
	SetLightPlacementMenuVisible(true);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, TEXT("Choose a light, then press Place to begin repeated truss light placement."));
	}
}

void AUnrealTrussBuildPawn::RotateBuildPositive()
{
	if (BuildManagerComponent)
	{
		BuildManagerComponent->RotatePreviewYaw(15.0f);
	}
}

void AUnrealTrussBuildPawn::RotateBuildNegative()
{
	if (BuildManagerComponent)
	{
		BuildManagerComponent->RotatePreviewYaw(-15.0f);
	}
}

void AUnrealTrussBuildPawn::ShowControlsMessage() const
{
	if (!GEngine)
	{
		return;
	}

	GEngine->AddOnScreenDebugMessage(
		-1,
		10.0f,
		FColor::Cyan,
		TEXT("Controls: WASD move, Space/Ctrl up-down, Mouse look, Tab build menu, B create mode, E edit looked-at truss/MBP/stage, L light mode, Left Mouse place/click rail/select panel or stage cell, R/F rotate, Q cancel")
	);
}

UBuildItemDataAsset* AUnrealTrussBuildPawn::FindDefaultBuildItem() const
{
	UBuildItemDataAsset* FirstItem = nullptr;

	for (UBuildItemDataAsset* BuildItem : AvailableBuildItems)
	{
		if (!BuildItem)
		{
			continue;
		}

		if (!FirstItem)
		{
			FirstItem = BuildItem;
		}

		if (BuildItem->ItemId == PreferredBuildItemId)
		{
			return BuildItem;
		}
	}

	return bAutoSelectFirstBuildItem ? FirstItem : nullptr;
}

void AUnrealTrussBuildPawn::GatherBuildItems()
{
	AvailableBuildItems.Reset();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> AssetDataList;
	AssetRegistryModule.Get().GetAssetsByClass(UBuildItemDataAsset::StaticClass()->GetClassPathName(), AssetDataList, true);

	for (const FAssetData& AssetData : AssetDataList)
	{
		if (UBuildItemDataAsset* BuildItem = Cast<UBuildItemDataAsset>(AssetData.GetAsset()))
		{
			AvailableBuildItems.Add(BuildItem);
		}
	}

	if (!HasBuildItemType(AvailableBuildItems, EBuildItemType::MBPWall))
	{
		if (UBuildItemDataAsset* FallbackMBPItem = CreateFallbackMBPBuildItem(this))
		{
			AvailableBuildItems.Add(FallbackMBPItem);
		}
	}

	if (!HasBuildItemType(AvailableBuildItems, EBuildItemType::StageDeck))
	{
		if (UBuildItemDataAsset* FallbackStageItem = CreateFallbackStageBuildItem(this))
		{
			AvailableBuildItems.Add(FallbackStageItem);
		}
	}
}

void AUnrealTrussBuildPawn::GatherLightingBlueprints()
{
	AvailableLightingNames.Reset();
	AvailableLightingClasses.Reset();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> AssetDataList;
	AssetRegistryModule.Get().GetAssetsByPath(FName(*LightingBlueprintFolder), AssetDataList, true, false);

	for (const FAssetData& AssetData : AssetDataList)
	{
		if (AssetData.AssetClassPath != FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("Blueprint")))
		{
			continue;
		}

		const FString AssetName = AssetData.AssetName.ToString();
		if (!AssetName.StartsWith(TEXT("BP_")))
		{
			continue;
		}

		if (UBlueprint* BlueprintAsset = Cast<UBlueprint>(AssetData.GetAsset()))
		{
			if (UClass* GeneratedClass = BlueprintAsset->GeneratedClass)
			{
				if (GeneratedClass->IsChildOf(AActor::StaticClass()))
				{
					AvailableLightingNames.Add(AssetName.RightChop(3));
					AvailableLightingClasses.Add(GeneratedClass);
				}
			}
		}
	}
}

void AUnrealTrussBuildPawn::EnsureBuildMenuWidget()
{
	if (BuildMenuWidget)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}

	TSubclassOf<UBuildMenuWidget> MenuClass = BuildMenuWidgetClass;
	if (!MenuClass)
	{
		MenuClass = UBuildMenuWidget::StaticClass();
	}
	BuildMenuWidget = CreateWidget<UBuildMenuWidget>(PlayerController, MenuClass);
	if (!BuildMenuWidget)
	{
		return;
	}

	TArray<UBuildItemDataAsset*> BuildItemArray;
	for (UBuildItemDataAsset* BuildItem : AvailableBuildItems)
	{
		BuildItemArray.Add(BuildItem);
	}

	BuildMenuWidget->SetBuildManager(BuildManagerComponent);
	BuildMenuWidget->SetBuildItems(BuildItemArray);
	if (DefaultBuildItem)
	{
		BuildMenuWidget->SetSelectedBuildItem(DefaultBuildItem);
	}
	BuildMenuWidget->OnBuildItemSelected.AddDynamic(this, &AUnrealTrussBuildPawn::HandleBuildItemSelected);
	BuildMenuWidget->OnActionRequested.AddDynamic(this, &AUnrealTrussBuildPawn::HandleBuildMenuActionRequested);
	BuildMenuWidget->AddToViewport(10);
	BuildMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
}

void AUnrealTrussBuildPawn::EnsureLightPlacementMenuWidget()
{
	if (LightPlacementMenuWidget)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}

	TSubclassOf<ULightPlacementMenuWidget> MenuClass = LightPlacementMenuWidgetClass;
	if (!MenuClass)
	{
		MenuClass = ULightPlacementMenuWidget::StaticClass();
	}

	LightPlacementMenuWidget = CreateWidget<ULightPlacementMenuWidget>(PlayerController, MenuClass);
	if (!LightPlacementMenuWidget)
	{
		return;
	}

	LightPlacementMenuWidget->SetFixtureOptions(AvailableLightingNames, AvailableLightingClasses);
	LightPlacementMenuWidget->SetPlacementReady(false);
	LightPlacementMenuWidget->OnPlaceRequested.AddDynamic(this, &AUnrealTrussBuildPawn::HandleLightPlacementActionRequested);
	LightPlacementMenuWidget->OnCancelRequested.AddDynamic(this, &AUnrealTrussBuildPawn::HandleLightPlacementCanceled);
	LightPlacementMenuWidget->AddToViewport(11);
	LightPlacementMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
}

bool AUnrealTrussBuildPawn::EnsureLightPreviewActor()
{
	if (LightPreviewActor)
	{
		return true;
	}

	if (!ActiveLightFixtureClass)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	LightPreviewActor = World->SpawnActor<ABuildPreviewActor>(ABuildPreviewActor::StaticClass(), FTransform::Identity, SpawnParameters);
	if (!LightPreviewActor)
	{
		return false;
	}

	LightPreviewActor->SetPreviewActorClass(ActiveLightFixtureClass);
	LightPreviewActor->SetActorHiddenInGame(true);
	return true;
}

void AUnrealTrussBuildPawn::DestroyLightPreviewActor()
{
	if (LightPreviewActor)
	{
		LightPreviewActor->Destroy();
		LightPreviewActor = nullptr;
	}
}

void AUnrealTrussBuildPawn::SetBuildMenuVisible(bool bVisible)
{
	if (!BuildMenuWidget)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}

	BuildMenuWidget->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	PlayerController->bShowMouseCursor = bVisible;

	if (bVisible)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(BuildMenuWidget->TakeWidget());
		InputMode.SetHideCursorDuringCapture(false);
		PlayerController->SetInputMode(InputMode);
	}
	else
	{
		PlayerController->SetInputMode(FInputModeGameOnly());
	}
}

void AUnrealTrussBuildPawn::SetLightPlacementMenuVisible(bool bVisible)
{
	if (!LightPlacementMenuWidget)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}

	LightPlacementMenuWidget->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	PlayerController->bShowMouseCursor = bVisible;

	if (bVisible)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(LightPlacementMenuWidget->TakeWidget());
		InputMode.SetHideCursorDuringCapture(false);
		PlayerController->SetInputMode(InputMode);
	}
	else
	{
		PlayerController->SetInputMode(FInputModeGameOnly());
	}
}

void AUnrealTrussBuildPawn::SetHoveredTrussActor(ATrussStructureActor* NewHoveredActor)
{
	if (HoveredTrussActor == NewHoveredActor)
	{
		return;
	}

	if (HoveredTrussActor)
	{
		HoveredTrussActor->SetSelectionHighlighted(false);
	}

	HoveredTrussActor = NewHoveredActor;

	if (HoveredTrussActor)
	{
		HoveredTrussActor->SetSelectionHighlighted(true);
	}
}

void AUnrealTrussBuildPawn::HandleBuildItemSelected(UBuildItemDataAsset* SelectedItem)
{
	if (!SelectedItem || !BuildManagerComponent)
	{
		return;
	}

	DefaultBuildItem = SelectedItem;
	BuildManagerComponent->SetSelectedBuildItem(SelectedItem);
	if (BuildMenuWidget && SelectedItem->ItemType == EBuildItemType::TrussStructure)
	{
		BuildManagerComponent->SetActiveTrussDefinition(BuildMenuWidget->GetCurrentTrussDefinition());
	}
	else if (BuildMenuWidget && SelectedItem->ItemType == EBuildItemType::MBPWall)
	{
		BuildManagerComponent->SetActiveMBPWallDefinition(BuildMenuWidget->GetCurrentMBPWallDefinition());
	}
	else if (BuildMenuWidget && SelectedItem->ItemType == EBuildItemType::StageDeck)
	{
		BuildManagerComponent->SetActiveStageDeckDefinition(BuildMenuWidget->GetCurrentStageDeckDefinition());
	}
	else
	{
		BuildManagerComponent->SetActiveTrussDefinition(SelectedItem->DefaultTrussDefinition);
	}
	BuildManagerComponent->EnterBuildMode();
}

void AUnrealTrussBuildPawn::HandleBuildMenuActionRequested()
{
	if (BuildMenuWidget)
	{
		if (AMBPWallActor* EditingMBPWall = BuildMenuWidget->GetEditingMBPTarget())
		{
			EditingMBPWall->SetSelectionHighlighted(false);
			BuildMenuWidget->SetEditingMBPTarget(nullptr);
			PendingMBPEditWall = nullptr;
			SetBuildMenuVisible(false);
			return;
		}
		if (AStageDeckActor* EditingStageDeck = BuildMenuWidget->GetEditingStageTarget())
		{
			EditingStageDeck->SetSelectionHighlighted(false);
			BuildMenuWidget->SetEditingStageTarget(nullptr);
			PendingStageEditActor = nullptr;
			SetBuildMenuVisible(false);
			return;
		}
	}

	ConfirmBuildPlacement();
	SetBuildMenuVisible(false);
}

void AUnrealTrussBuildPawn::HandleLightPlacementActionRequested()
{
	if (!LightPlacementMenuWidget)
	{
		return;
	}

	if (!bLightPlacementModeActive)
	{
		UClass* FixtureClass = LightPlacementMenuWidget->GetSelectedFixtureClass();
		if (!FixtureClass || !FixtureClass->IsChildOf(AActor::StaticClass()))
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Red, TEXT("Failed to resolve selected lighting actor class."));
			}
			return;
		}

		ActiveLightFixtureClass = FixtureClass;
		ActiveLightSlingType = LightPlacementMenuWidget->GetSelectedSlingType();
		bLightPlacementModeActive = true;
		PendingLightTargetTruss = nullptr;
		PendingLightTargetWorldLocation = FVector::ZeroVector;
		DestroyLightPreviewActor();
		EnsureLightPreviewActor();
		LightPlacementMenuWidget->SetPlacementReady(true);
		SetLightPlacementMenuVisible(false);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("Light selected. Aim at a truss rail and left click to place copies."));
		}
		return;
	}

	if (!PendingLightTargetTruss || !ActiveLightFixtureClass)
	{
		return;
	}

	FTransform MountTransform;
	if (!PendingLightTargetTruss->GetFixtureMountTransform(PendingLightTargetWorldLocation, ActiveLightSlingType, MountTransform))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (PendingLightTargetTruss->AddMountedFixtureDefinition(ActiveLightFixtureClass, ActiveLightSlingType, PendingLightTargetWorldLocation, true))
	{
		PendingLightTargetTruss->Modify();
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT("Placed light on truss."));
	}
}

void AUnrealTrussBuildPawn::HandleLightPlacementCanceled()
{
	SetLightPlacementMenuVisible(false);
	if (LightPlacementMenuWidget)
	{
		LightPlacementMenuWidget->SetPlacementReady(false);
		LightPlacementMenuWidget->SetTargetTruss(nullptr);
	}
	PendingLightTargetTruss = nullptr;
	PendingLightTargetWorldLocation = FVector::ZeroVector;
	bLightPlacementModeActive = false;
	ActiveLightFixtureClass = nullptr;
	DestroyLightPreviewActor();
}

bool AUnrealTrussBuildPawn::TraceForTrussHit(FHitResult& OutHitResult, ATrussStructureActor*& OutActor) const
{
	if (TargetingPointerComponent && TargetingPointerComponent->bHasValidHit && TargetingPointerComponent->CurrentTrussActor)
	{
		OutHitResult = TargetingPointerComponent->CurrentHitResult;
		OutActor = TargetingPointerComponent->CurrentTrussActor;
		return true;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	UWorld* World = GetWorld();
	if (!PlayerController || !World)
	{
		return false;
	}

	FVector ViewLocation = FVector::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

	const FVector TraceEnd = ViewLocation + (ViewRotation.Vector() * 50000.0f);
	TArray<FHitResult> HitResults;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EditTrussTrace), true, this);
	if (BuildManagerComponent && BuildManagerComponent->ActivePreviewActor)
	{
		QueryParams.AddIgnoredActor(BuildManagerComponent->ActivePreviewActor);
		if (AActor* PreviewChildActor = BuildManagerComponent->ActivePreviewActor->GetPreviewActor())
		{
			QueryParams.AddIgnoredActor(PreviewChildActor);
		}
	}
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	if (!World->LineTraceMultiByObjectType(HitResults, ViewLocation, TraceEnd, ObjectQueryParams, QueryParams))
	{
		return false;
	}

	for (const FHitResult& HitResult : HitResults)
	{
		if (ATrussStructureActor* TrussActor = Cast<ATrussStructureActor>(HitResult.GetActor()))
		{
			OutHitResult = HitResult;
			OutActor = TrussActor;
			return true;
		}

		if (const UActorComponent* HitComponent = HitResult.GetComponent())
		{
			if (ATrussStructureActor* OwnerTrussActor = Cast<ATrussStructureActor>(HitComponent->GetOwner()))
			{
				OutHitResult = HitResult;
				OutActor = OwnerTrussActor;
				return true;
			}
		}
	}

	return false;
}

void AUnrealTrussBuildPawn::UpdateLightPreview()
{
	if (!bLightPlacementModeActive || !ActiveLightFixtureClass)
	{
		DestroyLightPreviewActor();
		return;
	}

	if (!EnsureLightPreviewActor())
	{
		return;
	}

	if (LightPlacementMenuWidget)
	{
		LightPlacementMenuWidget->SetTargetTruss(HoveredTrussActor.Get());
	}

	if (!HoveredTrussActor)
	{
		LightPreviewActor->SetActorHiddenInGame(true);
		PendingLightTargetTruss = nullptr;
		return;
	}

	FTransform MountTransform;
	if (!HoveredTrussActor->GetFixtureMountTransform(HoveredTrussHitLocation, ActiveLightSlingType, MountTransform))
	{
		LightPreviewActor->SetActorHiddenInGame(true);
		PendingLightTargetTruss = nullptr;
		return;
	}

	PendingLightTargetTruss = HoveredTrussActor.Get();
	PendingLightTargetWorldLocation = HoveredTrussHitLocation;
	LightPreviewActor->SetActorHiddenInGame(false);
	LightPreviewActor->SetActorTransform(MountTransform);
	LightPreviewActor->SetPlacementValid(true);
}

void AUnrealTrussBuildPawn::BeginLightPlacementSelection()
{
	if (!bLightPlacementModeActive)
	{
		return;
	}

	ATrussStructureActor* TargetTruss = HoveredTrussActor.Get();
	if (!TargetTruss)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("Look at a truss before choosing a light mount point."));
		}
		return;
	}

	EnsureLightPlacementMenuWidget();
	if (!LightPlacementMenuWidget)
	{
		return;
	}

	PendingLightTargetTruss = TargetTruss;
	PendingLightTargetWorldLocation = HoveredTrussHitLocation;
	LightPlacementMenuWidget->SetTargetTruss(TargetTruss);
	SetLightPlacementMenuVisible(true);
}

ATrussStructureActor* AUnrealTrussBuildPawn::TraceForTrussActor() const
{
	FHitResult HitResult;
	ATrussStructureActor* TrussActor = nullptr;
	return TraceForTrussHit(HitResult, TrussActor) ? TrussActor : nullptr;
}

bool AUnrealTrussBuildPawn::TraceForEditableActorHit(FHitResult& OutHitResult, ATrussStructureActor*& OutTrussActor, AMBPWallActor*& OutMBPWallActor, AStageDeckActor*& OutStageDeckActor) const
{
	OutHitResult = FHitResult();
	OutTrussActor = nullptr;
	OutMBPWallActor = nullptr;
	OutStageDeckActor = nullptr;

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	UWorld* World = GetWorld();
	if (!PlayerController || !World)
	{
		return false;
	}

	FVector ViewLocation = FVector::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

	const FVector TraceEnd = ViewLocation + (ViewRotation.Vector() * 50000.0f);
	TArray<FHitResult> HitResults;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EditBuildActorTrace), true, this);
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	if (!World->LineTraceMultiByObjectType(HitResults, ViewLocation, TraceEnd, ObjectQueryParams, QueryParams))
	{
		return false;
	}

	for (const FHitResult& HitResult : HitResults)
	{
		if (ATrussStructureActor* HitTrussActor = Cast<ATrussStructureActor>(HitResult.GetActor()))
		{
			OutHitResult = HitResult;
			OutTrussActor = HitTrussActor;
			return true;
		}

		if (AMBPWallActor* HitMBPWallActor = Cast<AMBPWallActor>(HitResult.GetActor()))
		{
			OutHitResult = HitResult;
			OutMBPWallActor = HitMBPWallActor;
			return true;
		}

		if (AStageDeckActor* HitStageDeckActor = Cast<AStageDeckActor>(HitResult.GetActor()))
		{
			OutHitResult = HitResult;
			OutStageDeckActor = HitStageDeckActor;
			return true;
		}

		if (const UActorComponent* HitComponent = HitResult.GetComponent())
		{
			if (ATrussStructureActor* OwnerTrussActor = Cast<ATrussStructureActor>(HitComponent->GetOwner()))
			{
				OutHitResult = HitResult;
				OutTrussActor = OwnerTrussActor;
				return true;
			}

			if (AMBPWallActor* OwnerMBPWallActor = Cast<AMBPWallActor>(HitComponent->GetOwner()))
			{
				OutHitResult = HitResult;
				OutMBPWallActor = OwnerMBPWallActor;
				return true;
			}

			if (AStageDeckActor* OwnerStageDeckActor = Cast<AStageDeckActor>(HitComponent->GetOwner()))
			{
				OutHitResult = HitResult;
				OutStageDeckActor = OwnerStageDeckActor;
				return true;
			}
		}
	}

	return false;
}
