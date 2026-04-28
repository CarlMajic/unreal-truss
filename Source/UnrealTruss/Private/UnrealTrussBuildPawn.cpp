#include "UnrealTrussBuildPawn.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "BuildItemDataAsset.h"
#include "BuildManagerComponent.h"
#include "BuildMenuWidget.h"
#include "BuildPreviewActor.h"
#include "InputCoreTypes.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "TrussStructureActor.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

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

	AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void AUnrealTrussBuildPawn::BeginPlay()
{
	Super::BeginPlay();

	GatherBuildItems();
	DefaultBuildItem = FindDefaultBuildItem();
	EnsureBuildMenuWidget();
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

	const bool bMenuVisible = BuildMenuWidget && BuildMenuWidget->GetVisibility() == ESlateVisibility::Visible;
	if (!bMenuVisible)
	{
		SetHoveredTrussActor(TraceForTrussActor());
	}
	else
	{
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
	PlayerInputComponent->BindAction(TEXT("RotateBuildPositive"), IE_Pressed, this, &AUnrealTrussBuildPawn::RotateBuildPositive);
	PlayerInputComponent->BindAction(TEXT("RotateBuildNegative"), IE_Pressed, this, &AUnrealTrussBuildPawn::RotateBuildNegative);
	PlayerInputComponent->BindKey(EKeys::E, IE_Pressed, this, &AUnrealTrussBuildPawn::EditLookedAtTruss);
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
		BuildMenuWidget->SetEditingTarget(nullptr);
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
		BuildMenuWidget->SetEditingTarget(nullptr);
	}

	SetHoveredTrussActor(nullptr);
}

void AUnrealTrussBuildPawn::EditLookedAtTruss()
{
	ATrussStructureActor* TrussActor = HoveredTrussActor.Get();
	if (!TrussActor)
	{
		TrussActor = TraceForTrussActor();
	}
	if (!TrussActor || !BuildManagerComponent)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("No truss actor found under the view."));
		}
		return;
	}

	EnsureBuildMenuWidget();
	BuildManagerComponent->SetEditingTrussActor(TrussActor);
	BuildManagerComponent->EnterBuildMode();

	if (BuildMenuWidget)
	{
		BuildMenuWidget->SetEditingTarget(TrussActor);
	}

	SetBuildMenuVisible(true);
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
		TEXT("Controls: WASD move, Space/Ctrl up-down, Mouse look, Tab menu, B create mode, E edit looked-at truss, Left Mouse place/update, R/F rotate, Q cancel")
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
	else
	{
		BuildManagerComponent->SetActiveTrussDefinition(SelectedItem->DefaultTrussDefinition);
	}
	BuildManagerComponent->EnterBuildMode();
}

void AUnrealTrussBuildPawn::HandleBuildMenuActionRequested()
{
	ConfirmBuildPlacement();
	SetBuildMenuVisible(false);
}

ATrussStructureActor* AUnrealTrussBuildPawn::TraceForTrussActor() const
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	UWorld* World = GetWorld();
	if (!PlayerController || !World)
	{
		return nullptr;
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
		return nullptr;
	}

	for (const FHitResult& HitResult : HitResults)
	{
		if (ATrussStructureActor* TrussActor = Cast<ATrussStructureActor>(HitResult.GetActor()))
		{
			return TrussActor;
		}

		if (const UActorComponent* HitComponent = HitResult.GetComponent())
		{
			if (ATrussStructureActor* OwnerTrussActor = Cast<ATrussStructureActor>(HitComponent->GetOwner()))
			{
				return OwnerTrussActor;
			}
		}
	}

	return nullptr;
}
