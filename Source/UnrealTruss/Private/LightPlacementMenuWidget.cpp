#include "LightPlacementMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void ULightPlacementMenuWidget::SetFixtureOptions(const TArray<FString>& InDisplayNames, const TArray<TSubclassOf<AActor>>& InActorClasses)
{
	FixtureDisplayNames = InDisplayNames;
	FixtureActorClasses = InActorClasses;
	RefreshMenu();
}

void ULightPlacementMenuWidget::SetTargetTruss(ATrussStructureActor* InTargetTruss)
{
	TargetTruss = InTargetTruss;
	RefreshMenu();
}

void ULightPlacementMenuWidget::SetPlacementReady(bool bInPlacementReady)
{
	bPlacementReady = bInPlacementReady;
	RefreshMenu();
}

TSubclassOf<AActor> ULightPlacementMenuWidget::GetSelectedFixtureClass() const
{
	if (!FixtureComboBox)
	{
		return nullptr;
	}

	const FString SelectedName = FixtureComboBox->GetSelectedOption();
	const int32 SelectedIndex = FixtureDisplayNames.IndexOfByKey(SelectedName);
	return FixtureActorClasses.IsValidIndex(SelectedIndex) ? FixtureActorClasses[SelectedIndex] : nullptr;
}

ETrussSlingType ULightPlacementMenuWidget::GetSelectedSlingType() const
{
	if (SlingComboBox && SlingComboBox->GetSelectedOption().Contains(TEXT("Under")))
	{
		return ETrussSlingType::UnderSlung;
	}

	return ETrussSlingType::OverSlung;
}

void ULightPlacementMenuWidget::RefreshMenu()
{
	if (HeaderText)
	{
		HeaderText->SetText(FText::FromString(TEXT("Place Light")));
	}

	if (DetailText)
	{
		DetailText->SetText(BuildDetailText());
	}

	if (PlaceButtonText)
	{
		PlaceButtonText->SetText(BuildActionText());
	}

	if (FixtureComboBox)
	{
		const FString CurrentSelection = FixtureComboBox->GetSelectedOption();
		FixtureComboBox->ClearOptions();
		for (const FString& DisplayName : FixtureDisplayNames)
		{
			FixtureComboBox->AddOption(DisplayName);
		}

		if (!CurrentSelection.IsEmpty() && FixtureDisplayNames.Contains(CurrentSelection))
		{
			FixtureComboBox->SetSelectedOption(CurrentSelection);
		}
		else if (FixtureDisplayNames.Num() > 0)
		{
			FixtureComboBox->SetSelectedIndex(0);
		}
	}
}

TSharedRef<SWidget> ULightPlacementMenuWidget::RebuildWidget()
{
	WidgetTree = NewObject<UWidgetTree>(this, TEXT("LightPlacementMenuTree"));

	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootBox"));
	WidgetTree->RootWidget = RootBox;

	auto AddBlock = [&](const TCHAR* Name, const FLinearColor& Color, int32 Size, const FString& Text) -> UTextBlock*
	{
		UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		TextBlock->SetColorAndOpacity(FSlateColor(Color));
		TextBlock->SetText(FText::FromString(Text));
		TextBlock->SetAutoWrapText(true);
		if (UVerticalBoxSlot* Slot = RootBox->AddChildToVerticalBox(TextBlock))
		{
			Slot->SetPadding(FMargin(12.0f, 8.0f));
		}
		return TextBlock;
	};

	HeaderText = AddBlock(TEXT("HeaderText"), FLinearColor::White, 18, TEXT("Place Light"));
	DetailText = AddBlock(TEXT("DetailText"), FLinearColor(0.8f, 0.86f, 0.95f), 12, TEXT("Choose a fixture and sling direction."));

	FixtureLabelText = AddBlock(TEXT("FixtureLabelText"), FLinearColor::White, 12, TEXT("Fixture"));
	FixtureComboBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("FixtureComboBox"));
	FixtureComboBox->OnGenerateWidgetEvent.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ULightPlacementMenuWidget, GenerateComboItemWidget));
	if (UVerticalBoxSlot* VerticalSlot = RootBox->AddChildToVerticalBox(FixtureComboBox))
	{
		VerticalSlot->SetPadding(FMargin(12.0f, 0.0f, 12.0f, 8.0f));
	}

	SlingLabelText = AddBlock(TEXT("SlingLabelText"), FLinearColor::White, 12, TEXT("Sling"));
	SlingComboBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("SlingComboBox"));
	SlingComboBox->OnGenerateWidgetEvent.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ULightPlacementMenuWidget, GenerateComboItemWidget));
	SlingComboBox->AddOption(TEXT("Over Slung"));
	SlingComboBox->AddOption(TEXT("Under Slung"));
	SlingComboBox->SetSelectedIndex(0);
	if (UVerticalBoxSlot* VerticalSlot = RootBox->AddChildToVerticalBox(SlingComboBox))
	{
		VerticalSlot->SetPadding(FMargin(12.0f, 0.0f, 12.0f, 12.0f));
	}

	UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ButtonRow"));
	if (UVerticalBoxSlot* VerticalSlot = RootBox->AddChildToVerticalBox(ButtonRow))
	{
		VerticalSlot->SetPadding(FMargin(12.0f, 8.0f, 12.0f, 12.0f));
	}

	PlaceButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("PlaceButton"));
	PlaceButton->OnClicked.AddDynamic(this, &ULightPlacementMenuWidget::HandlePlaceClicked);
	if (UHorizontalBoxSlot* HorizontalSlot = ButtonRow->AddChildToHorizontalBox(PlaceButton))
	{
		HorizontalSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
		HorizontalSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	PlaceButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PlaceButtonText"));
	PlaceButtonText->SetText(FText::FromString(TEXT("Place Light")));
	PlaceButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	PlaceButton->AddChild(PlaceButtonText);

	CancelButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CancelButton"));
	CancelButton->OnClicked.AddDynamic(this, &ULightPlacementMenuWidget::HandleCancelClicked);
	if (UHorizontalBoxSlot* HorizontalSlot = ButtonRow->AddChildToHorizontalBox(CancelButton))
	{
		HorizontalSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	CancelButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CancelButtonText"));
	CancelButtonText->SetText(FText::FromString(TEXT("Cancel")));
	CancelButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	CancelButton->AddChild(CancelButtonText);

	RefreshMenu();
	return Super::RebuildWidget();
}

UWidget* ULightPlacementMenuWidget::GenerateComboItemWidget(FString Item)
{
	UTextBlock* ItemText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ItemText->SetText(FText::FromString(Item));
	ItemText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	return ItemText;
}

FText ULightPlacementMenuWidget::BuildDetailText() const
{
	if (!bPlacementReady)
	{
		return FText::FromString(TEXT("Choose a light and sling direction, then click Place to begin pointing at the truss."));
	}

	if (!TargetTruss)
	{
		return FText::FromString(TEXT("Point at a truss rail and left click to place the selected light. The same light stays active for repeated placement."));
	}

	return FText::FromString(FString::Printf(TEXT("Ready to place on %s. Left click to place, or change the light and press Place again to update the active preview."), *TargetTruss->GetName()));
}

FText ULightPlacementMenuWidget::BuildActionText() const
{
	return bPlacementReady
		? FText::FromString(TEXT("Update Preview"))
		: FText::FromString(TEXT("Place"));
}

void ULightPlacementMenuWidget::HandlePlaceClicked()
{
	OnPlaceRequested.Broadcast();
}

void ULightPlacementMenuWidget::HandleCancelClicked()
{
	OnCancelRequested.Broadcast();
}
