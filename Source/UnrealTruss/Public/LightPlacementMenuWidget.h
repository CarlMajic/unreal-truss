#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TrussStructureActor.h"
#include "LightPlacementMenuWidget.generated.h"

class UButton;
class UComboBoxString;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLightPlacementMenuActionRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLightPlacementMenuCancelRequested);

UCLASS()
class UNREALTRUSS_API ULightPlacementMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Light Menu")
	FLightPlacementMenuActionRequested OnPlaceRequested;

	UPROPERTY(BlueprintAssignable, Category = "Light Menu")
	FLightPlacementMenuCancelRequested OnCancelRequested;

	UFUNCTION(BlueprintCallable, Category = "Light Menu")
	void SetFixtureOptions(const TArray<FString>& InDisplayNames, const TArray<TSubclassOf<AActor>>& InActorClasses);

	UFUNCTION(BlueprintCallable, Category = "Light Menu")
	void SetTargetTruss(ATrussStructureActor* InTargetTruss);

	UFUNCTION(BlueprintCallable, Category = "Light Menu")
	void SetPlacementReady(bool bInPlacementReady);

	UFUNCTION(BlueprintPure, Category = "Light Menu")
	TSubclassOf<AActor> GetSelectedFixtureClass() const;

	UFUNCTION(BlueprintPure, Category = "Light Menu")
	ETrussSlingType GetSelectedSlingType() const;

	UFUNCTION(BlueprintCallable, Category = "Light Menu")
	void RefreshMenu();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	UPROPERTY(Transient)
	TArray<FString> FixtureDisplayNames;

	UPROPERTY(Transient)
	TArray<TSubclassOf<AActor>> FixtureActorClasses;

	UPROPERTY(Transient)
	TObjectPtr<ATrussStructureActor> TargetTruss = nullptr;

	bool bPlacementReady = false;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HeaderText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FixtureLabelText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UComboBoxString> FixtureComboBox = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SlingLabelText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UComboBoxString> SlingComboBox = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UButton> PlaceButton = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PlaceButtonText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UButton> CancelButton = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CancelButtonText = nullptr;

	UWidget* GenerateComboItemWidget(FString Item);
	FText BuildDetailText() const;
	FText BuildActionText() const;

	UFUNCTION()
	void HandlePlaceClicked();

	UFUNCTION()
	void HandleCancelClicked();
};
