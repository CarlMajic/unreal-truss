#pragma once

#include "BuildItemDataAsset.h"
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DrapeRunActor.h"
#include "LoungeLayoutActor.h"
#include "MBPWallActor.h"
#include "StageDeckActor.h"
#include "StageDeckBuildDefinition.h"
#include "TrussStructureActor.h"
#include "VideoPlacementActor.h"
#include "VideoWallActor.h"
#include "ProjectionScreenActor.h"
#include "BuildMenuWidget.generated.h"

class UBuildItemDataAsset;
class UBuildManagerComponent;
class UButton;
class UCheckBox;
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
	WholeWall,
	Panel,
	Row,
	Column
};

enum class EStageRuntimeEditScope : uint8
{
	WholeStage,
	Cell
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

	UFUNCTION(BlueprintPure, Category = "Build Menu")
	FStageDeckBuildDefinition GetCurrentStageDeckDefinition() const;

	UFUNCTION(BlueprintPure, Category = "Build Menu")
	FDrapeRunBuildDefinition GetCurrentDrapeRunDefinition() const;

	UFUNCTION(BlueprintPure, Category = "Build Menu")
	FVideoPlacementBuildDefinition GetCurrentVideoPlacementDefinition() const;

	UFUNCTION(BlueprintPure, Category = "Build Menu")
	FVideoWallBuildDefinition GetCurrentVideoWallDefinition() const;

	UFUNCTION(BlueprintPure, Category = "Build Menu")
	FProjectionScreenBuildDefinition GetCurrentProjectionScreenDefinition() const;

	UFUNCTION(BlueprintPure, Category = "Build Menu")
	FAudioPlacementBuildDefinition GetCurrentAudioPlacementDefinition() const;

	UFUNCTION(BlueprintPure, Category = "Build Menu")
	FLoungeLayoutBuildDefinition GetCurrentLoungeLayoutDefinition() const;

	UFUNCTION(BlueprintCallable, Category = "Build Menu")
	void SetEditingTarget(class ATrussStructureActor* InEditingTarget);

	UFUNCTION(BlueprintPure, Category = "Build Menu")
	class ATrussStructureActor* GetEditingTarget() const;

	UFUNCTION(BlueprintCallable, Category = "Build Menu")
	void SetEditingMBPTarget(class AMBPWallActor* InEditingTarget, int32 InTargetRow = 0, int32 InTargetColumn = 0, bool bStartWithPanel = false);

	UFUNCTION(BlueprintPure, Category = "Build Menu")
	class AMBPWallActor* GetEditingMBPTarget() const;

	UFUNCTION(BlueprintCallable, Category = "Build Menu")
	void SetEditingMBPPanelTarget(int32 InTargetRow, int32 InTargetColumn);

	UFUNCTION(BlueprintCallable, Category = "Build Menu")
	void SetEditingStageTarget(class AStageDeckActor* InEditingTarget, int32 InTargetRow = 0, int32 InTargetColumn = 0, bool bStartWithCell = false);

	UFUNCTION(BlueprintPure, Category = "Build Menu")
	class AStageDeckActor* GetEditingStageTarget() const;

	UFUNCTION(BlueprintCallable, Category = "Build Menu")
	void SetEditingStageCellTarget(int32 InTargetRow, int32 InTargetColumn);

	UFUNCTION(BlueprintCallable, Category = "Build Menu")
	void SetEditingDrapeTarget(class ADrapeRunActor* InEditingTarget);

	UFUNCTION(BlueprintPure, Category = "Build Menu")
	class ADrapeRunActor* GetEditingDrapeTarget() const;

	UFUNCTION(BlueprintCallable, Category = "Build Menu")
	void SetEditingVideoTarget(class AVideoPlacementActor* InEditingTarget);

	UFUNCTION(BlueprintPure, Category = "Build Menu")
	class AVideoPlacementActor* GetEditingVideoTarget() const;

	UFUNCTION(BlueprintCallable, Category = "Build Menu")
	void SetEditingVideoWallTarget(class AVideoWallActor* InEditingTarget);

	UFUNCTION(BlueprintPure, Category = "Build Menu")
	class AVideoWallActor* GetEditingVideoWallTarget() const;

	UFUNCTION(BlueprintCallable, Category = "Build Menu")
	void SetEditingProjectionTarget(class AProjectionScreenActor* InEditingTarget);

	UFUNCTION(BlueprintPure, Category = "Build Menu")
	class AProjectionScreenActor* GetEditingProjectionTarget() const;

	UFUNCTION(BlueprintCallable, Category = "Build Menu")
	void SetEditingAudioTarget(class AActor* InEditingTarget);

	UFUNCTION(BlueprintPure, Category = "Build Menu")
	class AActor* GetEditingAudioTarget() const;

	UFUNCTION(BlueprintCallable, Category = "Build Menu")
	void SetEditingLoungeTarget(class ALoungeLayoutActor* InEditingTarget);

	UFUNCTION(BlueprintPure, Category = "Build Menu")
	class ALoungeLayoutActor* GetEditingLoungeTarget() const;

	UFUNCTION(BlueprintPure, Category = "Build Menu")
	bool ShouldShowPointerForCurrentEdit() const;

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
	FStageDeckBuildDefinition CurrentStageDeckDefinition;

	UPROPERTY(Transient)
	FDrapeRunBuildDefinition CurrentDrapeRunDefinition;

	UPROPERTY(Transient)
	FVideoPlacementBuildDefinition CurrentVideoPlacementDefinition;

	UPROPERTY(Transient)
	FVideoWallBuildDefinition CurrentVideoWallDefinition;

	UPROPERTY(Transient)
	FProjectionScreenBuildDefinition CurrentProjectionScreenDefinition;

	UPROPERTY(Transient)
	FAudioPlacementBuildDefinition CurrentAudioPlacementDefinition;

	UPROPERTY(Transient)
	FLoungeLayoutBuildDefinition CurrentLoungeLayoutDefinition;

	UPROPERTY(Transient)
	TSoftObjectPtr<class USoundBase> CurrentAudioSource;

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
	TObjectPtr<UButton> StageTabButton = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UButton> DrapeTabButton = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UButton> VideoTabButton = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UButton> VideoWallTabButton = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ProjectionTabButton = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UButton> AudioTabButton = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UButton> LoungeTabButton = nullptr;

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
	TObjectPtr<UTextBlock> StageHeightLabelText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UWhiteComboBoxString> StageHeightComboBox = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StageSurfaceLabelText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UWhiteComboBoxString> StageSurfaceComboBox = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StageFrontRailingLabelText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UCheckBox> StageFrontRailingCheckBox = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StageBackRailingLabelText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UCheckBox> StageBackRailingCheckBox = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StageLeftRailingLabelText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UCheckBox> StageLeftRailingCheckBox = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StageRightRailingLabelText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UCheckBox> StageRightRailingCheckBox = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StageLeftStepLabelText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UCheckBox> StageLeftStepCheckBox = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StageRightStepLabelText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UCheckBox> StageRightStepCheckBox = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StageAutomaticSkirtLabelText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UCheckBox> StageAutomaticSkirtCheckBox = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StageCellEnabledLabelText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UCheckBox> StageCellEnabledCheckBox = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBuildMenuItemButtonProxy>> ButtonProxies;

	UPROPERTY(Transient)
	TObjectPtr<ATrussStructureActor> EditingTarget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AMBPWallActor> EditingMBPTarget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AStageDeckActor> EditingStageTarget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ADrapeRunActor> EditingDrapeTarget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AVideoPlacementActor> EditingVideoTarget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AVideoWallActor> EditingVideoWallTarget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AProjectionScreenActor> EditingProjectionTarget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<class AAudioGroundSpeakerActor> EditingAudioGroundSpeakerTarget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<class AAudioGroundLineArrayActor> EditingAudioGroundLineArrayTarget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ALoungeLayoutActor> EditingLoungeTarget = nullptr;

	TArray<FString> AudioSourceOptions;
	TMap<FString, FSoftObjectPath> AudioSourcePathsByOption;

	bool bRefreshingControls = false;
	EBuildItemType ActiveMenuTab = EBuildItemType::TrussStructure;
	EMBPRuntimeEditScope CurrentMBPEditScope = EMBPRuntimeEditScope::Panel;
	int32 CurrentMBPEditTargetRow = 0;
	int32 CurrentMBPEditTargetColumn = 0;
	float CurrentMBPEditDepthOffsetCm = 0.0f;
	EStageRuntimeEditScope CurrentStageEditScope = EStageRuntimeEditScope::Cell;
	int32 CurrentStageEditTargetRow = 0;
	int32 CurrentStageEditTargetColumn = 0;
	bool bCurrentStageCellEnabled = true;
	EStageDeckHeightPreset CurrentStageCellHeightPreset = EStageDeckHeightPreset::In24;
	EStageDeckSurfaceStyle CurrentStageCellSurfaceStyle = EStageDeckSurfaceStyle::BlackTop;

	void RebuildItemButtons();
	FText BuildDetailText() const;
	FText BuildHeaderText() const;
	FText BuildActionButtonText() const;
	bool IsEditingMBP() const;
	bool IsEditingStage() const;
	bool IsEditingDrape() const;
	bool IsEditingVideo() const;
	bool IsEditingVideoWall() const;
	bool IsEditingProjection() const;
	bool IsEditingAudio() const;
	bool IsEditingLounge() const;
	void RefreshTabButtons();
	void RefreshTrussControls();
	void RefreshMBPControls();
	void RefreshStageControls();
	void RefreshDrapeControls();
	void RefreshVideoControls();
	void RefreshVideoWallControls();
	void RefreshProjectionControls();
	void RefreshAudioControls();
	void RefreshLoungeControls();
	void ApplyTrussDefinitionToBuildManager();
	void ApplyMBPDefinitionToBuildManager();
	void ApplyStageDefinitionToBuildManager();
	void ApplyDrapeDefinitionToBuildManager();
	void ApplyVideoDefinitionToBuildManager();
	void ApplyVideoWallDefinitionToBuildManager();
	void ApplyProjectionDefinitionToBuildManager();
	void ApplyAudioDefinitionToBuildManager();
	void ApplyLoungeDefinitionToBuildManager();
	void ApplyMBPEditToTarget();
	void ApplyStageEditToTarget();
	void ApplyDrapeEditToTarget();
	void ApplyVideoEditToTarget();
	void ApplyVideoWallEditToTarget();
	void ApplyProjectionEditToTarget();
	void ApplyAudioEditToTarget();
	void ApplyLoungeEditToTarget();
	void SyncCurrentStageCellFromTarget();
	void RefreshAudioSourceOptions();
	FString AudioSourceToOption(const TSoftObjectPtr<USoundBase>& AudioSource) const;
	TSoftObjectPtr<USoundBase> OptionToAudioSource(const FString& Option) const;
	bool ItemBelongsToActiveTab(const UBuildItemDataAsset* BuildItem) const;
	static FString BuildModeToOption(ETrussBuildMode BuildMode);
	static ETrussBuildMode OptionToBuildMode(const FString& Option);
	static FString PieceTypeToOption(ETrussPieceType PieceType);
	static ETrussPieceType OptionToPieceType(const FString& Option);
	static FString MBPStyleToOption(EMBPPanelStyle Style);
	static EMBPPanelStyle OptionToMBPStyle(const FString& Option);
	static FString StageHeightPresetToOption(EStageDeckHeightPreset HeightPreset);
	static EStageDeckHeightPreset OptionToStageHeightPreset(const FString& Option);
	static FString StageSurfaceStyleToOption(EStageDeckSurfaceStyle SurfaceStyle);
	static EStageDeckSurfaceStyle OptionToStageSurfaceStyle(const FString& Option);
	static FString MBPEditScopeToOption(EMBPRuntimeEditScope Scope);
	static EMBPRuntimeEditScope OptionToMBPEditScope(const FString& Option);
	static FString StageEditScopeToOption(EStageRuntimeEditScope Scope);
	static EStageRuntimeEditScope OptionToStageEditScope(const FString& Option);
	static FString TVModelToOption(EVideoTVModel TVModel);
	static EVideoTVModel OptionToTVModel(const FString& Option);
	static FString VideoWallSupportModeToOption(EVideoWallSupportMode SupportMode);
	static EVideoWallSupportMode OptionToVideoWallSupportMode(const FString& Option);
	static FString VideoWallSupportSpacingToOption(EVideoWallSupportSpacing SupportSpacing);
	static EVideoWallSupportSpacing OptionToVideoWallSupportSpacing(const FString& Option);
	static FString ProjectionScreenSizeToOption(EProjectionScreenKitSize ScreenKitSize);
	static EProjectionScreenKitSize OptionToProjectionScreenSize(const FString& Option);
	static FString ProjectionProjectorToOption(EProjectionProjectorType ProjectorType);
	static EProjectionProjectorType OptionToProjectionProjector(const FString& Option);
	static FString ProjectionLensToOption(EProjectionLensType LensType);
	static EProjectionLensType OptionToProjectionLens(const FString& Option);
	static FString AudioPlacementTypeToOption(EAudioPlacementRuntimeType PlacementType);
	static EAudioPlacementRuntimeType OptionToAudioPlacementType(const FString& Option);
	static FString AudioSpeakerModelToOption(EAudioGroundSpeakerModel SpeakerModel);
	static EAudioGroundSpeakerModel OptionToAudioSpeakerModel(const FString& Option);
	static FString AudioLineArrayModelToOption(EAudioGroundLineArrayModel LineArrayModel);
	static EAudioGroundLineArrayModel OptionToAudioLineArrayModel(const FString& Option);
	static void GetProjectionScreenDimensionsFt(EProjectionScreenKitSize ScreenKitSize, float& OutWidthFt, float& OutHeightFt);
	static void GetProjectionLensShiftPercent(EProjectionLensType LensType, float& OutVerticalMinPercent, float& OutVerticalMaxPercent, float& OutHorizontalMinPercent, float& OutHorizontalMaxPercent, bool& bOutHasData);
	UWidget* GenerateComboItemWidget(FString Item);

	UFUNCTION()
	void HandleTrussTabClicked();

	UFUNCTION()
	void HandleMBPTabClicked();

	UFUNCTION()
	void HandleStageTabClicked();

	UFUNCTION()
	void HandleDrapeTabClicked();

	UFUNCTION()
	void HandleVideoTabClicked();

	UFUNCTION()
	void HandleVideoWallTabClicked();

	UFUNCTION()
	void HandleProjectionTabClicked();

	UFUNCTION()
	void HandleAudioTabClicked();

	UFUNCTION()
	void HandleLoungeTabClicked();

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
	void HandleStageHeightChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleStageSurfaceChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleStageFrontRailingChanged(bool bIsChecked);

	UFUNCTION()
	void HandleStageBackRailingChanged(bool bIsChecked);

	UFUNCTION()
	void HandleStageLeftRailingChanged(bool bIsChecked);

	UFUNCTION()
	void HandleStageRightRailingChanged(bool bIsChecked);

	UFUNCTION()
	void HandleStageLeftStepChanged(bool bIsChecked);

	UFUNCTION()
	void HandleStageRightStepChanged(bool bIsChecked);

	UFUNCTION()
	void HandleStageAutomaticSkirtChanged(bool bIsChecked);

	UFUNCTION()
	void HandleStageCellEnabledChanged(bool bIsChecked);

	UFUNCTION()
	void HandleActionButtonClicked();
};
