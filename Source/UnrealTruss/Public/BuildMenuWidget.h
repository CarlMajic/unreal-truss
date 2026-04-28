#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TrussStructureActor.h"
#include "BuildMenuWidget.generated.h"

class UBuildItemDataAsset;
class UBuildManagerComponent;
class UButton;
class UComboBoxString;
class USpinBox;
class UTextBlock;
class UVerticalBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBuildMenuSelectionChanged, UBuildItemDataAsset*, SelectedItem);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBuildMenuActionRequested);

UCLASS()
class UNREALTRUSS_API UBuildMenuItemButtonProxy : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(class UBuildMenuWidget* InOwner, UBuildItemDataAsset* InBuildItem);

	UFUNCTION()
	void HandleClicked();

private:
	TWeakObjectPtr<class UBuildMenuWidget> Owner;
	TWeakObjectPtr<UBuildItemDataAsset> BuildItem;
};

UCLASS()
class UNREALTRUSS_API UBuildMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Build Menu")
	FBuildMenuSelectionChanged OnBuildItemSelected;

	UPROPERTY(BlueprintAssignable, Category = "Build Menu")
	FBuildMenuActionRequested OnActionRequested;

	UFUNCTION(BlueprintCallable, Category = "Build Menu")
	void SetBuildItems(const TArray<UBuildItemDataAsset*>& InBuildItems);

	UFUNCTION(BlueprintCallable, Category = "Build Menu")
	void SetSelectedBuildItem(UBuildItemDataAsset* InSelectedItem);

	UFUNCTION(BlueprintCallable, Category = "Build Menu")
	void SetBuildManager(UBuildManagerComponent* InBuildManager);

	UFUNCTION(BlueprintCallable, Category = "Build Menu")
	void RefreshMenu();

	UFUNCTION(BlueprintPure, Category = "Build Menu")
	UBuildItemDataAsset* GetSelectedBuildItem() const;

	UFUNCTION(BlueprintPure, Category = "Build Menu")
	FTrussBuildDefinition GetCurrentTrussDefinition() const;

	UFUNCTION(BlueprintCallable, Category = "Build Menu")
	void SetEditingTarget(class ATrussStructureActor* InEditingTarget);

	UFUNCTION(BlueprintPure, Category = "Build Menu")
	class ATrussStructureActor* GetEditingTarget() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UBuildItemDataAsset>> BuildItems;

	UPROPERTY(Transient)
	TObjectPtr<UBuildItemDataAsset> SelectedBuildItem = nullptr;

	UPROPERTY(Transient)
	FTrussBuildDefinition CurrentTrussDefinition;

	UPROPERTY(Transient)
	TObjectPtr<UBuildManagerComponent> BuildManager = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> ItemListBox = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HeaderText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ActionButton = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ActionButtonText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ModeLabelText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UComboBoxString> ModeComboBox = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PrimaryValueLabelText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USpinBox> PrimaryValueSpinBox = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SecondaryValueLabelText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USpinBox> SecondaryValueSpinBox = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TertiaryValueLabelText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USpinBox> TertiaryValueSpinBox = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> QuaternaryValueLabelText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USpinBox> QuaternaryValueSpinBox = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SidePieceLabelText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UComboBoxString> SidePieceComboBox = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DepthPieceLabelText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UComboBoxString> DepthPieceComboBox = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBuildMenuItemButtonProxy>> ButtonProxies;

	UPROPERTY(Transient)
	TObjectPtr<ATrussStructureActor> EditingTarget = nullptr;

	bool bRefreshingControls = false;

	void RebuildItemButtons();
	FText BuildDetailText() const;
	FText BuildHeaderText() const;
	FText BuildActionButtonText() const;
	void RefreshTrussControls();
	void ApplyTrussDefinitionToBuildManager();
	static FString BuildModeToOption(ETrussBuildMode BuildMode);
	static ETrussBuildMode OptionToBuildMode(const FString& Option);
	static FString PieceTypeToOption(ETrussPieceType PieceType);
	static ETrussPieceType OptionToPieceType(const FString& Option);
	UWidget* GenerateComboItemWidget(FString Item);

	UFUNCTION()
	void HandleModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandlePrimaryValueChanged(float NewValue);

	UFUNCTION()
	void HandleSecondaryValueChanged(float NewValue);

	UFUNCTION()
	void HandleTertiaryValueChanged(float NewValue);

	UFUNCTION()
	void HandleQuaternaryValueChanged(float NewValue);

	UFUNCTION()
	void HandleSidePieceChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleDepthPieceChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleActionButtonClicked();
};
