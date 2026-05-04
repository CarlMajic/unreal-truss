#include "BuildMenuWidget.h"

#include "BuildItemDataAsset.h"
#include "BuildManagerComponent.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "TrussMathLibrary.h"
#include "TrussStructureActor.h"
#include "WhiteComboBoxString.h"
#include "Blueprint/WidgetTree.h"

namespace
{
static UBuildItemDataAsset* FindMatchingBuildItemForActor(const TArray<TObjectPtr<UBuildItemDataAsset>>& BuildItems, const ATrussStructureActor* TrussActor)
{
	if (!TrussActor)
	{
		return nullptr;
	}

	for (UBuildItemDataAsset* BuildItem : BuildItems)
	{
		if (!BuildItem || BuildItem->ItemType != EBuildItemType::TrussStructure)
		{
			continue;
		}

		if (BuildItem->BuildActorClass && TrussActor->IsA(BuildItem->BuildActorClass))
		{
			return BuildItem;
		}
	}

	return nullptr;
}
}

void UBuildMenuItemButtonProxy::Initialize(UBuildMenuWidget* InOwner, UBuildItemDataAsset* InBuildItem)
{
	Owner = InOwner;
	BuildItem = InBuildItem;
}

void UBuildMenuItemButtonProxy::HandleClicked()
{
	if (Owner.IsValid())
	{
		Owner->SetSelectedBuildItem(BuildItem.Get());
		Owner->OnBuildItemSelected.Broadcast(BuildItem.Get());
	}
}

void UBuildMenuWidget::SetBuildItems(const TArray<UBuildItemDataAsset*>& InBuildItems)
{
	BuildItems.Reset();
	for (UBuildItemDataAsset* BuildItem : InBuildItems)
	{
		BuildItems.Add(BuildItem);
	}

	if (!SelectedBuildItem && BuildItems.Num() > 0)
	{
		SelectedBuildItem = BuildItems[0];
	}

	if (SelectedBuildItem)
	{
		CurrentTrussDefinition = SelectedBuildItem->DefaultTrussDefinition;
		CurrentMBPWallDefinition = SelectedBuildItem->DefaultMBPWallDefinition;
		ActiveMenuTab = SelectedBuildItem->ItemType == EBuildItemType::MBPWall
			? EBuildItemType::MBPWall
			: EBuildItemType::TrussStructure;
	}

	RefreshMenu();
}

void UBuildMenuWidget::SetSelectedBuildItem(UBuildItemDataAsset* InSelectedItem)
{
	const bool bSelectionChanged = SelectedBuildItem != InSelectedItem;
	SelectedBuildItem = InSelectedItem;
	if (InSelectedItem && bSelectionChanged)
	{
		CurrentTrussDefinition = InSelectedItem->DefaultTrussDefinition;
		CurrentMBPWallDefinition = InSelectedItem->DefaultMBPWallDefinition;
		ActiveMenuTab = InSelectedItem->ItemType == EBuildItemType::MBPWall
			? EBuildItemType::MBPWall
			: EBuildItemType::TrussStructure;
	}

	if (BuildManager && InSelectedItem)
	{
		BuildManager->SetSelectedBuildItem(InSelectedItem);
		ApplyTrussDefinitionToBuildManager();
		ApplyMBPDefinitionToBuildManager();
	}

	RefreshMenu();
}

void UBuildMenuWidget::SetBuildManager(UBuildManagerComponent* InBuildManager)
{
	BuildManager = InBuildManager;
}

void UBuildMenuWidget::RefreshMenu()
{
	if (HeaderText)
	{
		HeaderText->SetText(BuildHeaderText());
	}

	if (DetailText)
	{
		DetailText->SetText(BuildDetailText());
	}

	if (ActionButtonText)
	{
		ActionButtonText->SetText(BuildActionButtonText());
	}

	RefreshTrussControls();
	RefreshMBPControls();
	RefreshTabButtons();
	RebuildItemButtons();
}

UBuildItemDataAsset* UBuildMenuWidget::GetSelectedBuildItem() const
{
	return SelectedBuildItem;
}

FTrussBuildDefinition UBuildMenuWidget::GetCurrentTrussDefinition() const
{
	return CurrentTrussDefinition;
}

FMBPWallDefinition UBuildMenuWidget::GetCurrentMBPWallDefinition() const
{
	return CurrentMBPWallDefinition;
}

void UBuildMenuWidget::SetEditingTarget(ATrussStructureActor* InEditingTarget)
{
	EditingTarget = InEditingTarget;
	EditingMBPTarget = nullptr;

	if (EditingTarget)
	{
		CurrentTrussDefinition = EditingTarget->GetBuildDefinition();
		if (UBuildItemDataAsset* MatchingItem = FindMatchingBuildItemForActor(BuildItems, EditingTarget))
		{
			SelectedBuildItem = MatchingItem;
		}
	}

	RefreshMenu();
}

ATrussStructureActor* UBuildMenuWidget::GetEditingTarget() const
{
	return EditingTarget;
}

void UBuildMenuWidget::SetEditingMBPTarget(AMBPWallActor* InEditingTarget, int32 InTargetRow, int32 InTargetColumn)
{
	EditingTarget = nullptr;
	EditingMBPTarget = InEditingTarget;

	if (EditingMBPTarget)
	{
		ActiveMenuTab = EBuildItemType::MBPWall;
		CurrentMBPWallDefinition = EditingMBPTarget->GetWallDefinition();
		CurrentMBPEditTargetRow = FMath::Clamp(InTargetRow, 0, FMath::Max(EditingMBPTarget->Rows - 1, 0));
		CurrentMBPEditTargetColumn = FMath::Clamp(InTargetColumn, 0, FMath::Max(EditingMBPTarget->Columns - 1, 0));
		CurrentMBPEditScope = EMBPRuntimeEditScope::Panel;

		FMBPPanelSlot TargetSlot;
		if (EditingMBPTarget->GetPanelSlot(CurrentMBPEditTargetRow, CurrentMBPEditTargetColumn, TargetSlot))
		{
			CurrentMBPWallDefinition.DefaultStyle = TargetSlot.Style;
			CurrentMBPEditDepthOffsetCm = TargetSlot.DepthOffsetCm;
		}

		for (UBuildItemDataAsset* BuildItem : BuildItems)
		{
			if (BuildItem && BuildItem->ItemType == EBuildItemType::MBPWall)
			{
				SelectedBuildItem = BuildItem;
				break;
			}
		}
	}

	RefreshMenu();
}

AMBPWallActor* UBuildMenuWidget::GetEditingMBPTarget() const
{
	return EditingMBPTarget;
}

void UBuildMenuWidget::SetEditingMBPPanelTarget(int32 InTargetRow, int32 InTargetColumn)
{
	if (!EditingMBPTarget)
	{
		return;
	}

	const int32 ClampedRow = FMath::Clamp(InTargetRow, 0, FMath::Max(EditingMBPTarget->Rows - 1, 0));
	const int32 ClampedColumn = FMath::Clamp(InTargetColumn, 0, FMath::Max(EditingMBPTarget->Columns - 1, 0));
	if (CurrentMBPEditTargetRow == ClampedRow && CurrentMBPEditTargetColumn == ClampedColumn)
	{
		return;
	}

	CurrentMBPEditTargetRow = ClampedRow;
	CurrentMBPEditTargetColumn = ClampedColumn;

	FMBPPanelSlot TargetSlot;
	if (EditingMBPTarget->GetPanelSlot(CurrentMBPEditTargetRow, CurrentMBPEditTargetColumn, TargetSlot))
	{
		CurrentMBPWallDefinition.DefaultStyle = TargetSlot.Style;
		CurrentMBPEditDepthOffsetCm = TargetSlot.DepthOffsetCm;
	}

	RefreshMenu();
}

TSharedRef<SWidget> UBuildMenuWidget::RebuildWidget()
{
	WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));

	UBorder* RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RootBorder"));
	RootBorder->SetPadding(FMargin(20.0f));
	RootBorder->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.03f, 0.92f));
	WidgetTree->RootWidget = RootBorder;

	RootScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RootScrollBox"));
	RootBorder->SetContent(RootScrollBox);

	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootBox"));
	RootScrollBox->AddChild(RootBox);

	TabButtonBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TabButtonBox"));
	if (UVerticalBoxSlot* TabSlot = RootBox->AddChildToVerticalBox(TabButtonBox))
	{
		TabSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	TrussTabButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("TrussTabButton"));
	TrussTabButton->OnClicked.AddDynamic(this, &UBuildMenuWidget::HandleTrussTabClicked);
	UTextBlock* TrussTabText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TrussTabText"));
	TrussTabText->SetText(FText::FromString(TEXT("Truss")));
	TrussTabText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	TrussTabButton->AddChild(TrussTabText);
	if (UHorizontalBoxSlot* TrussTabSlot = TabButtonBox->AddChildToHorizontalBox(TrussTabButton))
	{
		TrussTabSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
	}

	MBPTabButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("MBPTabButton"));
	MBPTabButton->OnClicked.AddDynamic(this, &UBuildMenuWidget::HandleMBPTabClicked);
	UTextBlock* MBPTabText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MBPTabText"));
	MBPTabText->SetText(FText::FromString(TEXT("MBP")));
	MBPTabText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	MBPTabButton->AddChild(MBPTabText);
	TabButtonBox->AddChildToHorizontalBox(MBPTabButton);

	HeaderText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HeaderText"));
	HeaderText->SetText(FText::FromString(TEXT("Build Menu")));
	HeaderText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.95f, 0.95f)));
	RootBox->AddChildToVerticalBox(HeaderText);

	DetailText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DetailText"));
	DetailText->SetColorAndOpacity(FSlateColor(FLinearColor(0.75f, 0.78f, 0.82f)));
	DetailText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* DetailSlot = RootBox->AddChildToVerticalBox(DetailText))
	{
		DetailSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 12.0f));
	}

	ModeLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ModeLabelText"));
	ModeLabelText->SetText(FText::FromString(TEXT("Truss Mode")));
	ModeLabelText->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.90f, 0.90f)));
	if (UVerticalBoxSlot* ModeLabelSlot = RootBox->AddChildToVerticalBox(ModeLabelText))
	{
		ModeLabelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
	}

	ModeComboBox = WidgetTree->ConstructWidget<UWhiteComboBoxString>(UWhiteComboBoxString::StaticClass(), TEXT("ModeComboBox"));
	ModeComboBox->OnGenerateWidgetEvent.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UBuildMenuWidget, GenerateComboItemWidget));
	ModeComboBox->AddOption(BuildModeToOption(ETrussBuildMode::StraightRun));
	ModeComboBox->AddOption(BuildModeToOption(ETrussBuildMode::Rectangle));
	ModeComboBox->AddOption(BuildModeToOption(ETrussBuildMode::Arch));
	ModeComboBox->AddOption(BuildModeToOption(ETrussBuildMode::Cube));
	ModeComboBox->AddOption(BuildModeToOption(ETrussBuildMode::CubeArch));
	ModeComboBox->OnSelectionChanged.AddDynamic(this, &UBuildMenuWidget::HandleModeChanged);
	if (UVerticalBoxSlot* ModeComboBoxSlot = RootBox->AddChildToVerticalBox(ModeComboBox))
	{
		ModeComboBoxSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	PrimaryValueLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PrimaryValueLabelText"));
	PrimaryValueLabelText->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.90f, 0.90f)));
	if (UVerticalBoxSlot* PrimaryLabelSlot = RootBox->AddChildToVerticalBox(PrimaryValueLabelText))
	{
		PrimaryLabelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
	}

	PrimaryValueSpinBox = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), TEXT("PrimaryValueSpinBox"));
	PrimaryValueSpinBox->SetDelta(1.0f);
	PrimaryValueSpinBox->SetAlwaysUsesDeltaSnap(true);
	PrimaryValueSpinBox->OnValueChanged.AddDynamic(this, &UBuildMenuWidget::HandlePrimaryValueChanged);
	if (UVerticalBoxSlot* PrimarySpinSlot = RootBox->AddChildToVerticalBox(PrimaryValueSpinBox))
	{
		PrimarySpinSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	SecondaryValueLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SecondaryValueLabelText"));
	SecondaryValueLabelText->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.90f, 0.90f)));
	if (UVerticalBoxSlot* SecondaryLabelSlot = RootBox->AddChildToVerticalBox(SecondaryValueLabelText))
	{
		SecondaryLabelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
	}

	SecondaryValueSpinBox = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), TEXT("SecondaryValueSpinBox"));
	SecondaryValueSpinBox->SetDelta(1.0f);
	SecondaryValueSpinBox->SetAlwaysUsesDeltaSnap(true);
	SecondaryValueSpinBox->OnValueChanged.AddDynamic(this, &UBuildMenuWidget::HandleSecondaryValueChanged);
	if (UVerticalBoxSlot* SecondarySpinSlot = RootBox->AddChildToVerticalBox(SecondaryValueSpinBox))
	{
		SecondarySpinSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	TertiaryValueLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TertiaryValueLabelText"));
	TertiaryValueLabelText->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.90f, 0.90f)));
	if (UVerticalBoxSlot* TertiaryLabelSlot = RootBox->AddChildToVerticalBox(TertiaryValueLabelText))
	{
		TertiaryLabelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
	}

	TertiaryValueSpinBox = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), TEXT("TertiaryValueSpinBox"));
	TertiaryValueSpinBox->SetDelta(1.0f);
	TertiaryValueSpinBox->SetAlwaysUsesDeltaSnap(true);
	TertiaryValueSpinBox->OnValueChanged.AddDynamic(this, &UBuildMenuWidget::HandleTertiaryValueChanged);
	if (UVerticalBoxSlot* TertiarySpinSlot = RootBox->AddChildToVerticalBox(TertiaryValueSpinBox))
	{
		TertiarySpinSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	QuaternaryValueLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuaternaryValueLabelText"));
	QuaternaryValueLabelText->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.90f, 0.90f)));
	if (UVerticalBoxSlot* QuaternaryLabelSlot = RootBox->AddChildToVerticalBox(QuaternaryValueLabelText))
	{
		QuaternaryLabelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
	}

	QuaternaryValueSpinBox = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), TEXT("QuaternaryValueSpinBox"));
	QuaternaryValueSpinBox->SetDelta(1.0f);
	QuaternaryValueSpinBox->SetAlwaysUsesDeltaSnap(true);
	QuaternaryValueSpinBox->OnValueChanged.AddDynamic(this, &UBuildMenuWidget::HandleQuaternaryValueChanged);
	if (UVerticalBoxSlot* QuaternarySpinSlot = RootBox->AddChildToVerticalBox(QuaternaryValueSpinBox))
	{
		QuaternarySpinSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	SidePieceLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SidePieceLabelText"));
	SidePieceLabelText->SetText(FText::FromString(TEXT("Side Spacer Piece")));
	SidePieceLabelText->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.90f, 0.90f)));
	if (UVerticalBoxSlot* SidePieceLabelSlot = RootBox->AddChildToVerticalBox(SidePieceLabelText))
	{
		SidePieceLabelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
	}

	SidePieceComboBox = WidgetTree->ConstructWidget<UWhiteComboBoxString>(UWhiteComboBoxString::StaticClass(), TEXT("SidePieceComboBox"));
	SidePieceComboBox->OnGenerateWidgetEvent.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UBuildMenuWidget, GenerateComboItemWidget));
	SidePieceComboBox->AddOption(PieceTypeToOption(ETrussPieceType::TwoFoot));
	SidePieceComboBox->AddOption(PieceTypeToOption(ETrussPieceType::FourFoot));
	SidePieceComboBox->AddOption(PieceTypeToOption(ETrussPieceType::FiveFoot));
	SidePieceComboBox->AddOption(PieceTypeToOption(ETrussPieceType::EightFoot));
	SidePieceComboBox->AddOption(PieceTypeToOption(ETrussPieceType::TenFoot));
	SidePieceComboBox->OnSelectionChanged.AddDynamic(this, &UBuildMenuWidget::HandleSidePieceChanged);
	if (UVerticalBoxSlot* SidePieceComboSlot = RootBox->AddChildToVerticalBox(SidePieceComboBox))
	{
		SidePieceComboSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	DepthPieceLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DepthPieceLabelText"));
	DepthPieceLabelText->SetText(FText::FromString(TEXT("Depth Spacer Piece")));
	DepthPieceLabelText->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.90f, 0.90f)));
	if (UVerticalBoxSlot* DepthPieceLabelSlot = RootBox->AddChildToVerticalBox(DepthPieceLabelText))
	{
		DepthPieceLabelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
	}

	DepthPieceComboBox = WidgetTree->ConstructWidget<UWhiteComboBoxString>(UWhiteComboBoxString::StaticClass(), TEXT("DepthPieceComboBox"));
	DepthPieceComboBox->OnGenerateWidgetEvent.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UBuildMenuWidget, GenerateComboItemWidget));
	DepthPieceComboBox->AddOption(PieceTypeToOption(ETrussPieceType::TwoFoot));
	DepthPieceComboBox->AddOption(PieceTypeToOption(ETrussPieceType::FourFoot));
	DepthPieceComboBox->AddOption(PieceTypeToOption(ETrussPieceType::FiveFoot));
	DepthPieceComboBox->AddOption(PieceTypeToOption(ETrussPieceType::EightFoot));
	DepthPieceComboBox->AddOption(PieceTypeToOption(ETrussPieceType::TenFoot));
	DepthPieceComboBox->OnSelectionChanged.AddDynamic(this, &UBuildMenuWidget::HandleDepthPieceChanged);
	if (UVerticalBoxSlot* DepthPieceComboSlot = RootBox->AddChildToVerticalBox(DepthPieceComboBox))
	{
		DepthPieceComboSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	MBPRowsLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MBPRowsLabelText"));
	MBPRowsLabelText->SetText(FText::FromString(TEXT("Rows")));
	MBPRowsLabelText->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.90f, 0.90f)));
	if (UVerticalBoxSlot* MBPRowsLabelSlot = RootBox->AddChildToVerticalBox(MBPRowsLabelText))
	{
		MBPRowsLabelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
	}

	MBPRowsSpinBox = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), TEXT("MBPRowsSpinBox"));
	MBPRowsSpinBox->SetDelta(1.0f);
	MBPRowsSpinBox->SetAlwaysUsesDeltaSnap(true);
	MBPRowsSpinBox->OnValueChanged.AddDynamic(this, &UBuildMenuWidget::HandleMBPRowsChanged);
	if (UVerticalBoxSlot* MBPRowsSpinSlot = RootBox->AddChildToVerticalBox(MBPRowsSpinBox))
	{
		MBPRowsSpinSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	MBPColumnsLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MBPColumnsLabelText"));
	MBPColumnsLabelText->SetText(FText::FromString(TEXT("Columns")));
	MBPColumnsLabelText->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.90f, 0.90f)));
	if (UVerticalBoxSlot* MBPColumnsLabelSlot = RootBox->AddChildToVerticalBox(MBPColumnsLabelText))
	{
		MBPColumnsLabelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
	}

	MBPColumnsSpinBox = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), TEXT("MBPColumnsSpinBox"));
	MBPColumnsSpinBox->SetDelta(1.0f);
	MBPColumnsSpinBox->SetAlwaysUsesDeltaSnap(true);
	MBPColumnsSpinBox->OnValueChanged.AddDynamic(this, &UBuildMenuWidget::HandleMBPColumnsChanged);
	if (UVerticalBoxSlot* MBPColumnsSpinSlot = RootBox->AddChildToVerticalBox(MBPColumnsSpinBox))
	{
		MBPColumnsSpinSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	MBPStyleLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MBPStyleLabelText"));
	MBPStyleLabelText->SetText(FText::FromString(TEXT("Default Style")));
	MBPStyleLabelText->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.90f, 0.90f)));
	if (UVerticalBoxSlot* MBPStyleLabelSlot = RootBox->AddChildToVerticalBox(MBPStyleLabelText))
	{
		MBPStyleLabelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
	}

	MBPStyleComboBox = WidgetTree->ConstructWidget<UWhiteComboBoxString>(UWhiteComboBoxString::StaticClass(), TEXT("MBPStyleComboBox"));
	MBPStyleComboBox->OnGenerateWidgetEvent.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UBuildMenuWidget, GenerateComboItemWidget));
	MBPStyleComboBox->AddOption(MBPStyleToOption(EMBPPanelStyle::Empty));
	MBPStyleComboBox->AddOption(MBPStyleToOption(EMBPPanelStyle::Acrylic));
	MBPStyleComboBox->AddOption(MBPStyleToOption(EMBPPanelStyle::Boxwood));
	MBPStyleComboBox->AddOption(MBPStyleToOption(EMBPPanelStyle::Drift));
	MBPStyleComboBox->AddOption(MBPStyleToOption(EMBPPanelStyle::Geo));
	MBPStyleComboBox->AddOption(MBPStyleToOption(EMBPPanelStyle::Shimmer));
	MBPStyleComboBox->AddOption(MBPStyleToOption(EMBPPanelStyle::Hive));
	MBPStyleComboBox->AddOption(MBPStyleToOption(EMBPPanelStyle::Platinum));
	MBPStyleComboBox->AddOption(MBPStyleToOption(EMBPPanelStyle::Custom));
	MBPStyleComboBox->OnSelectionChanged.AddDynamic(this, &UBuildMenuWidget::HandleMBPStyleChanged);
	if (UVerticalBoxSlot* MBPStyleComboSlot = RootBox->AddChildToVerticalBox(MBPStyleComboBox))
	{
		MBPStyleComboSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	ActionButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ActionButton"));
	ActionButton->SetBackgroundColor(FLinearColor(0.18f, 0.45f, 0.70f, 1.0f));
	ActionButton->OnClicked.AddDynamic(this, &UBuildMenuWidget::HandleActionButtonClicked);

	ActionButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ActionButtonText"));
	ActionButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	ActionButtonText->SetText(BuildActionButtonText());
	ActionButton->AddChild(ActionButtonText);
	if (UVerticalBoxSlot* ActionButtonSlot = RootBox->AddChildToVerticalBox(ActionButton))
	{
		ActionButtonSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	ItemListBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ItemListBox"));
	RootBox->AddChildToVerticalBox(ItemListBox);

	RefreshMenu();
	return Super::RebuildWidget();
}

void UBuildMenuWidget::RebuildItemButtons()
{
	if (!ItemListBox || !WidgetTree)
	{
		return;
	}

	ButtonProxies.Reset();
	ItemListBox->ClearChildren();

	const bool bShowBuildItemButtons = !EditingTarget && BuildItems.Num() > 1;
	int32 VisibleItemCount = 0;
	for (UBuildItemDataAsset* BuildItem : BuildItems)
	{
		if (BuildItem && ItemBelongsToActiveTab(BuildItem))
		{
			++VisibleItemCount;
		}
	}

	ItemListBox->SetVisibility(bShowBuildItemButtons && VisibleItemCount > 1 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (!bShowBuildItemButtons)
	{
		return;
	}

	for (UBuildItemDataAsset* BuildItem : BuildItems)
	{
		if (!BuildItem || !ItemBelongsToActiveTab(BuildItem))
		{
			continue;
		}

		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		Button->SetBackgroundColor(BuildItem == SelectedBuildItem
			? FLinearColor(0.18f, 0.45f, 0.70f, 1.0f)
			: FLinearColor(0.10f, 0.10f, 0.12f, 1.0f));

		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(BuildItem->DisplayName.IsEmpty() ? FText::FromName(BuildItem->ItemId) : BuildItem->DisplayName);
		Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Button->AddChild(Label);

		UBuildMenuItemButtonProxy* Proxy = NewObject<UBuildMenuItemButtonProxy>(this);
		Proxy->Initialize(this, BuildItem);
		Button->OnClicked.AddDynamic(Proxy, &UBuildMenuItemButtonProxy::HandleClicked);
		ButtonProxies.Add(Proxy);

		if (UVerticalBoxSlot* ButtonSlot = ItemListBox->AddChildToVerticalBox(Button))
		{
			ButtonSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		}
	}
}

FText UBuildMenuWidget::BuildDetailText() const
{
	if (!SelectedBuildItem && ActiveMenuTab != EBuildItemType::MBPWall)
	{
		return FText::FromString(TEXT("No build item selected."));
	}

	const FText Name = SelectedBuildItem
		? (SelectedBuildItem->DisplayName.IsEmpty() ? FText::FromName(SelectedBuildItem->ItemId) : SelectedBuildItem->DisplayName)
		: FText::FromString(TEXT("MBP Wall"));

	FString Detail = FString::Printf(
		TEXT("Category: %s\nType: %s\nGrid Snap: %.2f cm\nRotation Step: %.1f deg"),
		SelectedBuildItem ? *SelectedBuildItem->Category.ToString() : TEXT("Backdrop"),
		ActiveMenuTab == EBuildItemType::TrussStructure
			? TEXT("Truss Structure")
			: TEXT("MBP Wall"),
		SelectedBuildItem ? SelectedBuildItem->GridSnapSizeCm : 30.48f,
		SelectedBuildItem ? SelectedBuildItem->RotationStepDegrees : 15.0f
	);

	if (ActiveMenuTab == EBuildItemType::TrussStructure && SelectedBuildItem && SelectedBuildItem->ItemType == EBuildItemType::TrussStructure)
	{
		const FTrussBuildDefinition& Definition = CurrentTrussDefinition;
		switch (Definition.BuildMode)
		{
		case ETrussBuildMode::Rectangle:
			Detail += FString::Printf(TEXT("\nMode: Rectangle\nSize: %.1f ft x %.1f ft\nHeight: %.1f ft"), Definition.RectangleLengthFt, Definition.RectangleWidthFt, Definition.RectangleHeightFt);
			break;
		case ETrussBuildMode::Arch:
			Detail += FString::Printf(TEXT("\nMode: Arch\nSize: %.1f ft x %.1f ft"), Definition.ArchWidthFt, Definition.ArchHeightFt);
			break;
		case ETrussBuildMode::Cube:
			Detail += FString::Printf(TEXT("\nMode: Cube\nSize: %.1f x %.1f x %.1f ft"), Definition.CubeLengthFt, Definition.CubeWidthFt, Definition.CubeHeightFt);
			break;
		case ETrussBuildMode::CubeArch:
			Detail += FString::Printf(TEXT("\nMode: Cube Arch\nSize: %.1f ft x %.1f ft\nSide Spacer: %s\nDepth Spacer: %s"), Definition.CubeArchWidthFt, Definition.CubeArchHeightFt, *UTrussMathLibrary::PieceTypeToLabel(Definition.CubeArchSideSpacingPiece), *UTrussMathLibrary::PieceTypeToLabel(Definition.CubeArchDepthSpacingPiece));
			break;
		case ETrussBuildMode::StraightRun:
		default:
			Detail += FString::Printf(TEXT("\nMode: Straight Run\nLength: %.1f ft\nHeight: %.1f ft"), Definition.LengthFt, Definition.StraightRunHeightFt);
			break;
		}
	}
	else if (ActiveMenuTab == EBuildItemType::MBPWall)
	{
		const FMBPWallDefinition& Definition = CurrentMBPWallDefinition;
		Detail += FString::Printf(
			TEXT("\nRows: %d\nColumns: %d\nDefault Style: %s\nPanel Size: %.1f cm x %.1f cm"),
			Definition.Rows,
			Definition.Columns,
			*MBPStyleToOption(Definition.DefaultStyle),
			Definition.PanelWidthCm,
			Definition.PanelHeightCm
		);

		if (IsEditingMBP())
		{
			Detail += FString::Printf(
				TEXT("\nEdit Scope: %s\nTarget Row: %d\nTarget Column: %d\nDepth Offset: %.2f cm"),
				*MBPEditScopeToOption(CurrentMBPEditScope),
				CurrentMBPEditTargetRow + 1,
				CurrentMBPEditTargetColumn + 1,
				CurrentMBPEditDepthOffsetCm);
		}
	}

	return FText::Format(FText::FromString(TEXT("{0}\n\n{1}")), Name, FText::FromString(Detail));
}

FText UBuildMenuWidget::BuildHeaderText() const
{
	if (EditingTarget)
	{
		return FText::FromString(TEXT("Edit Truss"));
	}

	if (IsEditingMBP())
	{
		return FText::FromString(TEXT("Edit MBP"));
	}

	return FText::FromString(TEXT("Build Menu"));
}

FText UBuildMenuWidget::BuildActionButtonText() const
{
	return (EditingTarget || IsEditingMBP())
		? FText::FromString(TEXT("Apply"))
		: FText::FromString(TEXT("Create"));
}

bool UBuildMenuWidget::IsEditingMBP() const
{
	return EditingMBPTarget != nullptr;
}

void UBuildMenuWidget::RefreshTrussControls()
{
	bRefreshingControls = true;

	const bool bIsTrussItem = ActiveMenuTab == EBuildItemType::TrussStructure;
	const ESlateVisibility VisibleState = bIsTrussItem ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;

	if (ModeLabelText)
	{
		ModeLabelText->SetVisibility(VisibleState);
	}

		if (ModeComboBox)
		{
			ModeComboBox->SetVisibility(VisibleState);
			if (bIsTrussItem)
			{
				ModeComboBox->SetSelectedOption(BuildModeToOption(CurrentTrussDefinition.BuildMode));
			}
		}

	if (!bIsTrussItem)
	{
		if (PrimaryValueLabelText) PrimaryValueLabelText->SetVisibility(ESlateVisibility::Collapsed);
		if (PrimaryValueSpinBox) PrimaryValueSpinBox->SetVisibility(ESlateVisibility::Collapsed);
		if (SecondaryValueLabelText) SecondaryValueLabelText->SetVisibility(ESlateVisibility::Collapsed);
		if (SecondaryValueSpinBox) SecondaryValueSpinBox->SetVisibility(ESlateVisibility::Collapsed);
		if (TertiaryValueLabelText) TertiaryValueLabelText->SetVisibility(ESlateVisibility::Collapsed);
		if (TertiaryValueSpinBox) TertiaryValueSpinBox->SetVisibility(ESlateVisibility::Collapsed);
		if (QuaternaryValueLabelText) QuaternaryValueLabelText->SetVisibility(ESlateVisibility::Collapsed);
		if (QuaternaryValueSpinBox) QuaternaryValueSpinBox->SetVisibility(ESlateVisibility::Collapsed);
		if (SidePieceLabelText) SidePieceLabelText->SetVisibility(ESlateVisibility::Collapsed);
		if (SidePieceComboBox) SidePieceComboBox->SetVisibility(ESlateVisibility::Collapsed);
		if (DepthPieceLabelText) DepthPieceLabelText->SetVisibility(ESlateVisibility::Collapsed);
		if (DepthPieceComboBox) DepthPieceComboBox->SetVisibility(ESlateVisibility::Collapsed);
		bRefreshingControls = false;
		return;
	}

	auto SetNumericControl = [this](UTextBlock* Label, USpinBox* SpinBox, const FString& LabelText, float Value, float MinValue, float MaxValue, bool bVisible)
	{
		if (Label)
		{
			Label->SetText(FText::FromString(LabelText));
			Label->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}

		if (SpinBox)
		{
			SpinBox->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			SpinBox->SetMinValue(MinValue);
			SpinBox->SetMaxValue(MaxValue);
			SpinBox->SetMinSliderValue(MinValue);
			SpinBox->SetMaxSliderValue(MaxValue);
			if (bVisible)
			{
				SpinBox->SetValue(Value);
			}
		}
	};

	auto SetPieceControl = [](UTextBlock* Label, UComboBoxString* ComboBox, const FString& LabelText, const FString& SelectedOption, bool bVisible)
	{
		if (Label)
		{
			Label->SetText(FText::FromString(LabelText));
			Label->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}

		if (ComboBox)
		{
			ComboBox->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			if (bVisible)
			{
				ComboBox->SetSelectedOption(SelectedOption);
			}
		}
	};

	switch (CurrentTrussDefinition.BuildMode)
	{
	case ETrussBuildMode::Rectangle:
		SetNumericControl(PrimaryValueLabelText, PrimaryValueSpinBox, TEXT("Rectangle Length (ft)"), CurrentTrussDefinition.RectangleLengthFt, 2.0f, 200.0f, true);
		SetNumericControl(SecondaryValueLabelText, SecondaryValueSpinBox, TEXT("Rectangle Width (ft)"), CurrentTrussDefinition.RectangleWidthFt, 2.0f, 200.0f, true);
		SetNumericControl(TertiaryValueLabelText, TertiaryValueSpinBox, TEXT("Rectangle Height (ft)"), CurrentTrussDefinition.RectangleHeightFt, 0.0f, 100.0f, true);
		SetNumericControl(QuaternaryValueLabelText, QuaternaryValueSpinBox, TEXT(""), 0.0f, 0.0f, 0.0f, false);
		SetPieceControl(SidePieceLabelText, SidePieceComboBox, TEXT(""), TEXT(""), false);
		SetPieceControl(DepthPieceLabelText, DepthPieceComboBox, TEXT(""), TEXT(""), false);
		break;
	case ETrussBuildMode::Arch:
		SetNumericControl(PrimaryValueLabelText, PrimaryValueSpinBox, TEXT("Arch Width (ft)"), CurrentTrussDefinition.ArchWidthFt, 4.0f, 200.0f, true);
		SetNumericControl(SecondaryValueLabelText, SecondaryValueSpinBox, TEXT("Arch Height (ft)"), CurrentTrussDefinition.ArchHeightFt, 4.0f, 200.0f, true);
		SetNumericControl(TertiaryValueLabelText, TertiaryValueSpinBox, TEXT(""), 0.0f, 0.0f, 0.0f, false);
		SetNumericControl(QuaternaryValueLabelText, QuaternaryValueSpinBox, TEXT(""), 0.0f, 0.0f, 0.0f, false);
		SetPieceControl(SidePieceLabelText, SidePieceComboBox, TEXT(""), TEXT(""), false);
		SetPieceControl(DepthPieceLabelText, DepthPieceComboBox, TEXT(""), TEXT(""), false);
		break;
	case ETrussBuildMode::Cube:
		SetNumericControl(PrimaryValueLabelText, PrimaryValueSpinBox, TEXT("Cube Length (ft)"), CurrentTrussDefinition.CubeLengthFt, 4.0f, 200.0f, true);
		SetNumericControl(SecondaryValueLabelText, SecondaryValueSpinBox, TEXT("Cube Width (ft)"), CurrentTrussDefinition.CubeWidthFt, 4.0f, 200.0f, true);
		SetNumericControl(TertiaryValueLabelText, TertiaryValueSpinBox, TEXT("Cube Height (ft)"), CurrentTrussDefinition.CubeHeightFt, 4.0f, 200.0f, true);
		SetNumericControl(QuaternaryValueLabelText, QuaternaryValueSpinBox, TEXT(""), 0.0f, 0.0f, 0.0f, false);
		SetPieceControl(SidePieceLabelText, SidePieceComboBox, TEXT(""), TEXT(""), false);
		SetPieceControl(DepthPieceLabelText, DepthPieceComboBox, TEXT(""), TEXT(""), false);
		break;
	case ETrussBuildMode::CubeArch:
		SetNumericControl(PrimaryValueLabelText, PrimaryValueSpinBox, TEXT("Cube Arch Width (ft)"), CurrentTrussDefinition.CubeArchWidthFt, 8.0f, 200.0f, true);
		SetNumericControl(SecondaryValueLabelText, SecondaryValueSpinBox, TEXT("Cube Arch Height (ft)"), CurrentTrussDefinition.CubeArchHeightFt, 4.0f, 200.0f, true);
		SetNumericControl(TertiaryValueLabelText, TertiaryValueSpinBox, TEXT(""), 0.0f, 0.0f, 0.0f, false);
		SetNumericControl(QuaternaryValueLabelText, QuaternaryValueSpinBox, TEXT(""), 0.0f, 0.0f, 0.0f, false);
		SetPieceControl(SidePieceLabelText, SidePieceComboBox, TEXT("Side Spacer Piece"), PieceTypeToOption(CurrentTrussDefinition.CubeArchSideSpacingPiece), true);
		SetPieceControl(DepthPieceLabelText, DepthPieceComboBox, TEXT("Depth Spacer Piece"), PieceTypeToOption(CurrentTrussDefinition.CubeArchDepthSpacingPiece), true);
		break;
	case ETrussBuildMode::StraightRun:
	default:
		SetNumericControl(PrimaryValueLabelText, PrimaryValueSpinBox, TEXT("Straight Run Length (ft)"), CurrentTrussDefinition.LengthFt, 2.0f, 200.0f, true);
		SetNumericControl(SecondaryValueLabelText, SecondaryValueSpinBox, TEXT("Straight Run Height (ft)"), CurrentTrussDefinition.StraightRunHeightFt, 0.0f, 100.0f, true);
		SetNumericControl(TertiaryValueLabelText, TertiaryValueSpinBox, TEXT(""), 0.0f, 0.0f, 0.0f, false);
		SetNumericControl(QuaternaryValueLabelText, QuaternaryValueSpinBox, TEXT(""), 0.0f, 0.0f, 0.0f, false);
		SetPieceControl(SidePieceLabelText, SidePieceComboBox, TEXT(""), TEXT(""), false);
		SetPieceControl(DepthPieceLabelText, DepthPieceComboBox, TEXT(""), TEXT(""), false);
		break;
	}

	bRefreshingControls = false;
}

void UBuildMenuWidget::RefreshTabButtons()
{
	auto GetButtonColor = [this](EBuildItemType TabType)
	{
		return ActiveMenuTab == TabType
			? FLinearColor(0.18f, 0.45f, 0.70f, 1.0f)
			: FLinearColor(0.10f, 0.10f, 0.12f, 1.0f);
	};

	if (TrussTabButton)
	{
		TrussTabButton->SetBackgroundColor(GetButtonColor(EBuildItemType::TrussStructure));
	}

	if (MBPTabButton)
	{
		MBPTabButton->SetBackgroundColor(GetButtonColor(EBuildItemType::MBPWall));
	}
}

void UBuildMenuWidget::RefreshMBPControls()
{
	bRefreshingControls = true;

	const bool bIsMBPItem = ActiveMenuTab == EBuildItemType::MBPWall;
	const ESlateVisibility VisibleState = bIsMBPItem ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	const bool bEditingMBP = IsEditingMBP();

	if (ModeLabelText)
	{
		ModeLabelText->SetVisibility(bEditingMBP ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (bEditingMBP)
		{
			ModeLabelText->SetText(FText::FromString(TEXT("Edit Scope")));
		}
	}

	if (ModeComboBox)
	{
		ModeComboBox->SetVisibility(bEditingMBP ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (bEditingMBP)
		{
			ModeComboBox->ClearOptions();
			ModeComboBox->AddOption(MBPEditScopeToOption(EMBPRuntimeEditScope::Panel));
			ModeComboBox->AddOption(MBPEditScopeToOption(EMBPRuntimeEditScope::Row));
			ModeComboBox->AddOption(MBPEditScopeToOption(EMBPRuntimeEditScope::Column));
			ModeComboBox->SetSelectedOption(MBPEditScopeToOption(CurrentMBPEditScope));
		}
	}

	auto SetEditNumericControl = [](UTextBlock* Label, USpinBox* SpinBox, const TCHAR* LabelText, float Value, float MinValue, float MaxValue, bool bShow)
	{
		if (Label)
		{
			Label->SetText(FText::FromString(LabelText));
			Label->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}

		if (SpinBox)
		{
			SpinBox->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			SpinBox->SetMinValue(MinValue);
			SpinBox->SetMaxValue(MaxValue);
			SpinBox->SetMinSliderValue(MinValue);
			SpinBox->SetMaxSliderValue(MaxValue);
			if (bShow)
			{
				SpinBox->SetValue(Value);
			}
		}
	};

	if (bEditingMBP)
	{
		SetEditNumericControl(PrimaryValueLabelText, PrimaryValueSpinBox, TEXT("Target Row"), CurrentMBPEditTargetRow + 1, 1.0f, FMath::Max(CurrentMBPWallDefinition.Rows, 1), true);
		SetEditNumericControl(
			SecondaryValueLabelText,
			SecondaryValueSpinBox,
			TEXT("Target Column"),
			CurrentMBPEditTargetColumn + 1,
			1.0f,
			FMath::Max(CurrentMBPWallDefinition.Columns, 1),
			CurrentMBPEditScope == EMBPRuntimeEditScope::Panel);
		SetEditNumericControl(TertiaryValueLabelText, TertiaryValueSpinBox, TEXT("Depth Offset (cm)"), CurrentMBPEditDepthOffsetCm, -304.8f, 304.8f, true);
		SetEditNumericControl(QuaternaryValueLabelText, QuaternaryValueSpinBox, TEXT(""), 0.0f, 0.0f, 0.0f, false);
		if (SidePieceLabelText) SidePieceLabelText->SetVisibility(ESlateVisibility::Collapsed);
		if (SidePieceComboBox) SidePieceComboBox->SetVisibility(ESlateVisibility::Collapsed);
		if (DepthPieceLabelText) DepthPieceLabelText->SetVisibility(ESlateVisibility::Collapsed);
		if (DepthPieceComboBox) DepthPieceComboBox->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (MBPRowsLabelText)
	{
		MBPRowsLabelText->SetVisibility(bEditingMBP ? ESlateVisibility::Collapsed : VisibleState);
	}

	if (MBPRowsSpinBox)
	{
		MBPRowsSpinBox->SetVisibility(bEditingMBP ? ESlateVisibility::Collapsed : VisibleState);
		MBPRowsSpinBox->SetMinValue(1.0f);
		MBPRowsSpinBox->SetMaxValue(100.0f);
		MBPRowsSpinBox->SetMinSliderValue(1.0f);
		MBPRowsSpinBox->SetMaxSliderValue(24.0f);
		if (bIsMBPItem)
		{
			MBPRowsSpinBox->SetValue(CurrentMBPWallDefinition.Rows);
		}
	}

	if (MBPColumnsLabelText)
	{
		MBPColumnsLabelText->SetVisibility(bEditingMBP ? ESlateVisibility::Collapsed : VisibleState);
	}

	if (MBPColumnsSpinBox)
	{
		MBPColumnsSpinBox->SetVisibility(bEditingMBP ? ESlateVisibility::Collapsed : VisibleState);
		MBPColumnsSpinBox->SetMinValue(1.0f);
		MBPColumnsSpinBox->SetMaxValue(100.0f);
		MBPColumnsSpinBox->SetMinSliderValue(1.0f);
		MBPColumnsSpinBox->SetMaxSliderValue(24.0f);
		if (bIsMBPItem)
		{
			MBPColumnsSpinBox->SetValue(CurrentMBPWallDefinition.Columns);
		}
	}

	if (MBPStyleLabelText)
	{
		MBPStyleLabelText->SetVisibility(VisibleState);
		if (bEditingMBP)
		{
			MBPStyleLabelText->SetText(FText::FromString(TEXT("Edit Style")));
		}
		else
		{
			MBPStyleLabelText->SetText(FText::FromString(TEXT("Default Style")));
		}
	}

	if (MBPStyleComboBox)
	{
		MBPStyleComboBox->SetVisibility(VisibleState);
		if (bIsMBPItem)
		{
			MBPStyleComboBox->SetSelectedOption(MBPStyleToOption(CurrentMBPWallDefinition.DefaultStyle));
		}
	}

	bRefreshingControls = false;
}

void UBuildMenuWidget::ApplyTrussDefinitionToBuildManager()
{
	if (!BuildManager || !SelectedBuildItem || SelectedBuildItem->ItemType != EBuildItemType::TrussStructure)
	{
		return;
	}

	BuildManager->SetActiveTrussDefinition(CurrentTrussDefinition);
}

void UBuildMenuWidget::ApplyMBPDefinitionToBuildManager()
{
	if (!BuildManager || !SelectedBuildItem || SelectedBuildItem->ItemType != EBuildItemType::MBPWall)
	{
		return;
	}

	BuildManager->SetActiveMBPWallDefinition(CurrentMBPWallDefinition);
}

void UBuildMenuWidget::ApplyMBPEditToTarget()
{
	if (!EditingMBPTarget)
	{
		return;
	}

	switch (CurrentMBPEditScope)
	{
	case EMBPRuntimeEditScope::Row:
		EditingMBPTarget->ApplyRowEditByIndex(CurrentMBPEditTargetRow, CurrentMBPWallDefinition.DefaultStyle, CurrentMBPEditDepthOffsetCm);
		break;
	case EMBPRuntimeEditScope::Column:
		EditingMBPTarget->ApplyColumnEditByIndex(CurrentMBPEditTargetColumn, CurrentMBPWallDefinition.DefaultStyle, CurrentMBPEditDepthOffsetCm);
		break;
	case EMBPRuntimeEditScope::Panel:
	default:
		EditingMBPTarget->ApplyPanelEdit(CurrentMBPEditTargetRow, CurrentMBPEditTargetColumn, CurrentMBPWallDefinition.DefaultStyle, CurrentMBPEditDepthOffsetCm);
		break;
	}
}

bool UBuildMenuWidget::ItemBelongsToActiveTab(const UBuildItemDataAsset* BuildItem) const
{
	if (!BuildItem)
	{
		return false;
	}

	if (ActiveMenuTab == EBuildItemType::MBPWall)
	{
		return BuildItem->ItemType == EBuildItemType::MBPWall;
	}

	return BuildItem->ItemType == EBuildItemType::TrussStructure || BuildItem->ItemType == EBuildItemType::ActorClass;
}

FString UBuildMenuWidget::BuildModeToOption(ETrussBuildMode BuildMode)
{
	switch (BuildMode)
	{
	case ETrussBuildMode::Rectangle:
		return TEXT("Rectangle");
	case ETrussBuildMode::Arch:
		return TEXT("Arch");
	case ETrussBuildMode::Cube:
		return TEXT("Cube");
	case ETrussBuildMode::CubeArch:
		return TEXT("Cube Arch");
	case ETrussBuildMode::StraightRun:
	default:
		return TEXT("Straight Run");
	}
}

ETrussBuildMode UBuildMenuWidget::OptionToBuildMode(const FString& Option)
{
	if (Option == TEXT("Rectangle"))
	{
		return ETrussBuildMode::Rectangle;
	}
	if (Option == TEXT("Arch"))
	{
		return ETrussBuildMode::Arch;
	}
	if (Option == TEXT("Cube"))
	{
		return ETrussBuildMode::Cube;
	}
	if (Option == TEXT("Cube Arch"))
	{
		return ETrussBuildMode::CubeArch;
	}
	return ETrussBuildMode::StraightRun;
}

FString UBuildMenuWidget::PieceTypeToOption(ETrussPieceType PieceType)
{
	switch (PieceType)
	{
	case ETrussPieceType::TwoFoot:
		return TEXT("2 ft");
	case ETrussPieceType::FourFoot:
		return TEXT("4 ft");
	case ETrussPieceType::FiveFoot:
		return TEXT("5 ft");
	case ETrussPieceType::EightFoot:
		return TEXT("8 ft");
	case ETrussPieceType::TenFoot:
		return TEXT("10 ft");
	default:
		return TEXT("4 ft");
	}
}

ETrussPieceType UBuildMenuWidget::OptionToPieceType(const FString& Option)
{
	if (Option == TEXT("2 ft"))
	{
		return ETrussPieceType::TwoFoot;
	}
	if (Option == TEXT("5 ft"))
	{
		return ETrussPieceType::FiveFoot;
	}
	if (Option == TEXT("8 ft"))
	{
		return ETrussPieceType::EightFoot;
	}
	if (Option == TEXT("10 ft"))
	{
		return ETrussPieceType::TenFoot;
	}
	return ETrussPieceType::FourFoot;
}

FString UBuildMenuWidget::MBPStyleToOption(EMBPPanelStyle Style)
{
	switch (Style)
	{
	case EMBPPanelStyle::Empty:
		return TEXT("Empty");
	case EMBPPanelStyle::Acrylic:
		return TEXT("Acrylic");
	case EMBPPanelStyle::Boxwood:
		return TEXT("Boxwood");
	case EMBPPanelStyle::Drift:
		return TEXT("Drift");
	case EMBPPanelStyle::Geo:
		return TEXT("Geo");
	case EMBPPanelStyle::Shimmer:
		return TEXT("Shimmer");
	case EMBPPanelStyle::Hive:
		return TEXT("Hive");
	case EMBPPanelStyle::Platinum:
		return TEXT("Platinum");
	case EMBPPanelStyle::Custom:
		return TEXT("Custom");
	default:
		return TEXT("Geo");
	}
}

EMBPPanelStyle UBuildMenuWidget::OptionToMBPStyle(const FString& Option)
{
	if (Option == TEXT("Empty"))
	{
		return EMBPPanelStyle::Empty;
	}
	if (Option == TEXT("Acrylic"))
	{
		return EMBPPanelStyle::Acrylic;
	}
	if (Option == TEXT("Boxwood"))
	{
		return EMBPPanelStyle::Boxwood;
	}
	if (Option == TEXT("Drift"))
	{
		return EMBPPanelStyle::Drift;
	}
	if (Option == TEXT("Shimmer"))
	{
		return EMBPPanelStyle::Shimmer;
	}
	if (Option == TEXT("Hive"))
	{
		return EMBPPanelStyle::Hive;
	}
	if (Option == TEXT("Platinum"))
	{
		return EMBPPanelStyle::Platinum;
	}
	if (Option == TEXT("Custom"))
	{
		return EMBPPanelStyle::Custom;
	}
	return EMBPPanelStyle::Geo;
}

FString UBuildMenuWidget::MBPEditScopeToOption(EMBPRuntimeEditScope Scope)
{
	switch (Scope)
	{
	case EMBPRuntimeEditScope::Row:
		return TEXT("Row");
	case EMBPRuntimeEditScope::Column:
		return TEXT("Column");
	case EMBPRuntimeEditScope::Panel:
	default:
		return TEXT("Panel");
	}
}

EMBPRuntimeEditScope UBuildMenuWidget::OptionToMBPEditScope(const FString& Option)
{
	if (Option == TEXT("Row"))
	{
		return EMBPRuntimeEditScope::Row;
	}
	if (Option == TEXT("Column"))
	{
		return EMBPRuntimeEditScope::Column;
	}
	return EMBPRuntimeEditScope::Panel;
}

UWidget* UBuildMenuWidget::GenerateComboItemWidget(FString Item)
{
	UTextBlock* ItemText = WidgetTree ? WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()) : NewObject<UTextBlock>(this);
	if (ItemText)
	{
		ItemText->SetText(FText::FromString(Item));
		ItemText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	}

	return ItemText;
}

void UBuildMenuWidget::HandleTrussTabClicked()
{
	ActiveMenuTab = EBuildItemType::TrussStructure;

	if (!SelectedBuildItem || !ItemBelongsToActiveTab(SelectedBuildItem))
	{
		for (UBuildItemDataAsset* BuildItem : BuildItems)
		{
			if (ItemBelongsToActiveTab(BuildItem))
			{
				SetSelectedBuildItem(BuildItem);
				return;
			}
		}
	}

	RefreshMenu();
}

void UBuildMenuWidget::HandleMBPTabClicked()
{
	ActiveMenuTab = EBuildItemType::MBPWall;

	if (!SelectedBuildItem || !ItemBelongsToActiveTab(SelectedBuildItem))
	{
		for (UBuildItemDataAsset* BuildItem : BuildItems)
		{
			if (ItemBelongsToActiveTab(BuildItem))
			{
				SetSelectedBuildItem(BuildItem);
				return;
			}
		}
	}

	RefreshMenu();
}

void UBuildMenuWidget::HandleModeChanged(FString SelectedItemOption, ESelectInfo::Type SelectionType)
{
	if (bRefreshingControls)
	{
		return;
	}

	if (IsEditingMBP())
	{
		CurrentMBPEditScope = OptionToMBPEditScope(SelectedItemOption);
		ApplyMBPEditToTarget();
		RefreshMenu();
		return;
	}

	if (!SelectedBuildItem || SelectedBuildItem->ItemType != EBuildItemType::TrussStructure)
	{
		return;
	}

	CurrentTrussDefinition.BuildMode = OptionToBuildMode(SelectedItemOption);
	ApplyTrussDefinitionToBuildManager();
	RefreshMenu();
}

void UBuildMenuWidget::HandlePrimaryValueChanged(float NewValue)
{
	if (bRefreshingControls)
	{
		return;
	}

	if (IsEditingMBP())
	{
		CurrentMBPEditTargetRow = FMath::Max(0, FMath::RoundToInt(NewValue) - 1);
		ApplyMBPEditToTarget();
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (!SelectedBuildItem || SelectedBuildItem->ItemType != EBuildItemType::TrussStructure)
	{
		return;
	}

	switch (CurrentTrussDefinition.BuildMode)
	{
	case ETrussBuildMode::Rectangle:
		CurrentTrussDefinition.RectangleLengthFt = FMath::Max(NewValue, 2.0f);
		break;
	case ETrussBuildMode::Arch:
		CurrentTrussDefinition.ArchWidthFt = FMath::Max(NewValue, 4.0f);
		break;
	case ETrussBuildMode::Cube:
		CurrentTrussDefinition.CubeLengthFt = FMath::Max(NewValue, 4.0f);
		break;
	case ETrussBuildMode::CubeArch:
		CurrentTrussDefinition.CubeArchWidthFt = FMath::Max(NewValue, 8.0f);
		break;
	case ETrussBuildMode::StraightRun:
	default:
		CurrentTrussDefinition.LengthFt = FMath::Max(NewValue, 2.0f);
		break;
	}

	ApplyTrussDefinitionToBuildManager();
	if (DetailText)
	{
		DetailText->SetText(BuildDetailText());
	}
}

void UBuildMenuWidget::HandleSecondaryValueChanged(float NewValue)
{
	if (bRefreshingControls)
	{
		return;
	}

	if (IsEditingMBP())
	{
		CurrentMBPEditTargetColumn = FMath::Max(0, FMath::RoundToInt(NewValue) - 1);
		ApplyMBPEditToTarget();
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (!SelectedBuildItem || SelectedBuildItem->ItemType != EBuildItemType::TrussStructure)
	{
		return;
	}

	switch (CurrentTrussDefinition.BuildMode)
	{
	case ETrussBuildMode::Rectangle:
		CurrentTrussDefinition.RectangleWidthFt = FMath::Max(NewValue, 2.0f);
		break;
	case ETrussBuildMode::StraightRun:
		CurrentTrussDefinition.StraightRunHeightFt = FMath::Max(NewValue, 0.0f);
		break;
	case ETrussBuildMode::Arch:
		CurrentTrussDefinition.ArchHeightFt = FMath::Max(NewValue, 4.0f);
		break;
	case ETrussBuildMode::Cube:
		CurrentTrussDefinition.CubeWidthFt = FMath::Max(NewValue, 4.0f);
		break;
	case ETrussBuildMode::CubeArch:
		CurrentTrussDefinition.CubeArchHeightFt = FMath::Max(NewValue, 4.0f);
		break;
	default:
		break;
	}

	ApplyTrussDefinitionToBuildManager();
	if (DetailText)
	{
		DetailText->SetText(BuildDetailText());
	}
}

void UBuildMenuWidget::HandleTertiaryValueChanged(float NewValue)
{
	if (bRefreshingControls)
	{
		return;
	}

	if (IsEditingMBP())
	{
		CurrentMBPEditDepthOffsetCm = NewValue;
		ApplyMBPEditToTarget();
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (!SelectedBuildItem || SelectedBuildItem->ItemType != EBuildItemType::TrussStructure)
	{
		return;
	}

	if (CurrentTrussDefinition.BuildMode == ETrussBuildMode::Cube)
	{
		CurrentTrussDefinition.CubeHeightFt = FMath::Max(NewValue, 4.0f);
	}
	else if (CurrentTrussDefinition.BuildMode == ETrussBuildMode::Rectangle)
	{
		CurrentTrussDefinition.RectangleHeightFt = FMath::Max(NewValue, 0.0f);
	}

	ApplyTrussDefinitionToBuildManager();
	if (DetailText)
	{
		DetailText->SetText(BuildDetailText());
	}
}

void UBuildMenuWidget::HandleQuaternaryValueChanged(float NewValue)
{
	if (bRefreshingControls)
	{
		return;
	}
}

void UBuildMenuWidget::HandleSidePieceChanged(FString SelectedItemOption, ESelectInfo::Type SelectionType)
{
	if (bRefreshingControls || CurrentTrussDefinition.BuildMode != ETrussBuildMode::CubeArch)
	{
		return;
	}

	CurrentTrussDefinition.CubeArchSideSpacingPiece = OptionToPieceType(SelectedItemOption);
	ApplyTrussDefinitionToBuildManager();
	if (DetailText)
	{
		DetailText->SetText(BuildDetailText());
	}
}

void UBuildMenuWidget::HandleDepthPieceChanged(FString SelectedItemOption, ESelectInfo::Type SelectionType)
{
	if (bRefreshingControls || CurrentTrussDefinition.BuildMode != ETrussBuildMode::CubeArch)
	{
		return;
	}

	CurrentTrussDefinition.CubeArchDepthSpacingPiece = OptionToPieceType(SelectedItemOption);
	ApplyTrussDefinitionToBuildManager();
	if (DetailText)
	{
		DetailText->SetText(BuildDetailText());
	}
}

void UBuildMenuWidget::HandleMBPRowsChanged(float NewValue)
{
	if (bRefreshingControls || !SelectedBuildItem || SelectedBuildItem->ItemType != EBuildItemType::MBPWall)
	{
		return;
	}

	CurrentMBPWallDefinition.Rows = FMath::Max(1, FMath::RoundToInt(NewValue));
	ApplyMBPDefinitionToBuildManager();
	if (DetailText)
	{
		DetailText->SetText(BuildDetailText());
	}
}

void UBuildMenuWidget::HandleMBPColumnsChanged(float NewValue)
{
	if (bRefreshingControls || !SelectedBuildItem || SelectedBuildItem->ItemType != EBuildItemType::MBPWall)
	{
		return;
	}

	CurrentMBPWallDefinition.Columns = FMath::Max(1, FMath::RoundToInt(NewValue));
	ApplyMBPDefinitionToBuildManager();
	if (DetailText)
	{
		DetailText->SetText(BuildDetailText());
	}
}

void UBuildMenuWidget::HandleMBPStyleChanged(FString SelectedItemOption, ESelectInfo::Type SelectionType)
{
	if (bRefreshingControls || !SelectedBuildItem || SelectedBuildItem->ItemType != EBuildItemType::MBPWall)
	{
		return;
	}

	CurrentMBPWallDefinition.DefaultStyle = OptionToMBPStyle(SelectedItemOption);
	if (IsEditingMBP())
	{
		ApplyMBPEditToTarget();
	}
	else
	{
		ApplyMBPDefinitionToBuildManager();
	}
	if (DetailText)
	{
		DetailText->SetText(BuildDetailText());
	}
}

void UBuildMenuWidget::HandleActionButtonClicked()
{
	OnActionRequested.Broadcast();
}
