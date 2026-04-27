#include "BuildMenuWidget.h"

#include "BuildItemDataAsset.h"
#include "BuildManagerComponent.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "TrussMathLibrary.h"
#include "TrussStructureActor.h"
#include "Blueprint/WidgetTree.h"

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
	}

	if (BuildManager && InSelectedItem)
	{
		BuildManager->SetSelectedBuildItem(InSelectedItem);
		ApplyTrussDefinitionToBuildManager();
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

	RefreshTrussControls();
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

TSharedRef<SWidget> UBuildMenuWidget::RebuildWidget()
{
	WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));

	UBorder* RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RootBorder"));
	RootBorder->SetPadding(FMargin(20.0f));
	RootBorder->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.03f, 0.92f));
	WidgetTree->RootWidget = RootBorder;

	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootBox"));
	RootBorder->SetContent(RootBox);

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

	ModeComboBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("ModeComboBox"));
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

	SidePieceComboBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("SidePieceComboBox"));
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

	DepthPieceComboBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("DepthPieceComboBox"));
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

	for (UBuildItemDataAsset* BuildItem : BuildItems)
	{
		if (!BuildItem)
		{
			continue;
		}

		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		Button->SetBackgroundColor(BuildItem == SelectedBuildItem
			? FLinearColor(0.18f, 0.45f, 0.70f, 1.0f)
			: FLinearColor(0.10f, 0.10f, 0.12f, 1.0f));

		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(FText::FromString(TEXT("Create")));
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
	if (!SelectedBuildItem)
	{
		return FText::FromString(TEXT("No build item selected."));
	}

	const FText Name = SelectedBuildItem->DisplayName.IsEmpty()
		? FText::FromName(SelectedBuildItem->ItemId)
		: SelectedBuildItem->DisplayName;

	FString Detail = FString::Printf(
		TEXT("Category: %s\nType: %s\nGrid Snap: %.2f cm\nRotation Step: %.1f deg"),
		*SelectedBuildItem->Category.ToString(),
		SelectedBuildItem->ItemType == EBuildItemType::TrussStructure ? TEXT("Truss Structure") : TEXT("Actor Class"),
		SelectedBuildItem->GridSnapSizeCm,
		SelectedBuildItem->RotationStepDegrees
	);

	if (SelectedBuildItem->ItemType == EBuildItemType::TrussStructure)
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

	return FText::Format(FText::FromString(TEXT("{0}\n\n{1}")), Name, FText::FromString(Detail));
}

FText UBuildMenuWidget::BuildHeaderText() const
{
	return FText::FromString(TEXT("Build Menu"));
}

void UBuildMenuWidget::RefreshTrussControls()
{
	bRefreshingControls = true;

	const bool bIsTrussItem = SelectedBuildItem && SelectedBuildItem->ItemType == EBuildItemType::TrussStructure;
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

void UBuildMenuWidget::ApplyTrussDefinitionToBuildManager()
{
	if (!BuildManager || !SelectedBuildItem || SelectedBuildItem->ItemType != EBuildItemType::TrussStructure)
	{
		return;
	}

	BuildManager->SetActiveTrussDefinition(CurrentTrussDefinition);
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

void UBuildMenuWidget::HandleModeChanged(FString SelectedItemOption, ESelectInfo::Type SelectionType)
{
	if (bRefreshingControls)
	{
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
