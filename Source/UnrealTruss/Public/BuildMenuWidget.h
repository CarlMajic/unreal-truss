#pragma once

#include "BuildItemDataAsset.h"
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MBPWallActor.h"
#include "TrussStructureActor.h"
#include "BuildMenuWidget.generated.h"

class UBuildItemDataAsset;
class UBuildManagerComponent;
class UButton;
class UHorizontalBox;
class UWhiteComboBoxString;
class USpinBox;
class UScrollBox;
class UTextBlock;
class UVerticalBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBuildMenuSelectionChanged, UBuildItemDataAsset*, SelectedItem);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBuildMenuActionRequested);

enum class EMBPRuntimeEditScope : uint8
{
	Panel,
	Row,
	Column
};

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

	UFUNCTION(BlueprintPure, Category = "Build Menu")
	FMBPWallDefinition GetCurrentMBPWallDefinition() const;

	UFUNCTION(BlueprintCallable, Category = "Build Menu")
	void SetEditingTarget(class ATrussStructureActor* InEditingTarget);

	UFUNCTION(BlueprintPure, Category = "Build Menu")
	class ATrussStructureActor* GetEditingTarget() const;

	UFUNCTION(BlueprintCallable, Category = "Build Menu")
	void SetEditingMBPTarget(class AMBPWallActor* InEditingTarget, int32 InTargetRow = 0, int32 InTargetColumn = 0);

	UFUNCTION(BlueprintPure, Category = "Build Menu")
	class AMBPWallActor* GetEditingMBPTarget() const;

	UFUNCTION(BlueprintCallable, Category = "Build Menu")
	void SetEditingMBPPanelTarget(int32 InTargetRow, int32 InTargetColumn);

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
	FMBPWallDefinition CurrentMBPWallDefinition;

	UPROPERTY(Transient)
	TObjectPtr<UBuildManagerComponent> BuildManager = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> ItemListBox = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UScrollBox> RootScrollBox = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> TabButtonBox = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UButton> TrussTabButton = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UButton> MBPTabButton = nullptr;

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
	TObjectPtr<UWhiteComboBoxString> ModeComboBox = nullptr;

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
	TObjectPtr<UWhiteComboBoxString> SidePieceComboBox = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DepthPieceLabelText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UWhiteComboBoxString> DepthPieceComboBox = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MBPRowsLabelText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USpinBox> MBPRowsSpinBox = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MBPColumnsLabelText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USpinBox> MBPColumnsSpinBox = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MBPStyleLabelText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UWhiteComboBoxString> MBPStyleComboBox = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBuildMenuItemButtonProxy>> ButtonProxies;

	UPROPERTY(Transient)
	TObjectPtr<ATrussStructureActor> EditingTarget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AMBPWallActor> EditingMBPTarget = nullptr;

	bool bRefreshingControls = false;
	EBuildItemType ActiveMenuTab = EBuildItemType::TrussStructure;
	EMBPRuntimeEditScope CurrentMBPEditScope = EMBPRuntimeEditScope::Panel;
	int32 CurrentMBPEditTargetRow = 0;
	int32 CurrentMBPEditTargetColumn = 0;
	float CurrentMBPEditDepthOffsetCm = 0.0f;

	void RebuildItemButtons();
	FText BuildDetailText() const;
	FText BuildHeaderText() const;
	FText BuildActionButtonText() const;
	bool IsEditingMBP() const;
	void RefreshTabButtons();
	void RefreshTrussControls();
	void RefreshMBPControls();
	void ApplyTrussDefinitionToBuildManager();
	void ApplyMBPDefinitionToBuildManager();
	void ApplyMBPEditToTarget();
	bool ItemBelongsToActiveTab(const UBuildItemDataAsset* BuildItem) const;
	static FString BuildModeToOption(ETrussBuildMode BuildMode);
	static ETrussBuildMode OptionToBuildMode(const FString& Option);
	static FString PieceTypeToOption(ETrussPieceType PieceType);
	static ETrussPieceType OptionToPieceType(const FString& Option);
	static FString MBPStyleToOption(EMBPPanelStyle Style);
	static EMBPPanelStyle OptionToMBPStyle(const FString& Option);
	static FString MBPEditScopeToOption(EMBPRuntimeEditScope Scope);
	static EMBPRuntimeEditScope OptionToMBPEditScope(const FString& Option);
	UWidget* GenerateComboItemWidget(FString Item);

	UFUNCTION()
	void HandleTrussTabClicked();

	UFUNCTION()
	void HandleMBPTabClicked();

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
	void HandleMBPRowsChanged(float NewValue);

	UFUNCTION()
	void HandleMBPColumnsChanged(float NewValue);

	UFUNCTION()
	void HandleMBPStyleChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleActionButtonClicked();
};
