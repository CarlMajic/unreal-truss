#include "BuildMenuWidget.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AudioGroundLineArrayActor.h"
#include "AudioGroundSpeakerActor.h"
#include "BuildItemDataAsset.h"
#include "BuildManagerComponent.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/Slider.h"
#include "Components/Spacer.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "DiningTableActor.h"
#include "DMXAttribute.h"
#include "Engine/Texture2D.h"
#include "Game/DMXComponent.h"
#include "LightFixtureActor.h"
#include "Library/DMXEntityFixturePatch.h"
#include "Library/DMXEntityFixtureType.h"
#include "Library/DMXLibrary.h"
#include "StageDeckActor.h"
#include "TrussMathLibrary.h"
#include "TrussStructureActor.h"
#include "WhiteComboBoxString.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/UserWidget.h"
#include "Sound/SoundBase.h"

namespace
{
constexpr TCHAR NoAudioOption[] = TEXT("No Audio");
constexpr TCHAR SoundCheckAssetPath[] = TEXT("/Game/Majic_Gear/Audio/Sound_Check");
constexpr TCHAR LoungeSofaAssetPath[] = TEXT("/Game/Furniture/Sofas");
constexpr TCHAR LoungeChairAssetPath[] = TEXT("/Game/Furniture/Lounge_Chairs");
constexpr TCHAR LoungeCocktailTableAssetPath[] = TEXT("/Game/Furniture/Cocktail_Tables");
constexpr TCHAR LoungeEndTableAssetPath[] = TEXT("/Game/Furniture/End_Tables");
constexpr TCHAR LoungeLampAssetPath[] = TEXT("/Game/Furniture/Lamps");
constexpr TCHAR LoungeAccentAssetPath[] = TEXT("/Game/Furniture/Accents");
constexpr TCHAR SectionalArmlessAssetPath[] = TEXT("/Game/Furniture/Sectionals/Armless");
constexpr TCHAR SectionalCornerAssetPath[] = TEXT("/Game/Furniture/Sectionals/Corners");
constexpr TCHAR BarTableAssetPath[] = TEXT("/Game/Furniture/Bar_Tables");
constexpr TCHAR StoolAssetPath[] = TEXT("/Game/Furniture/Stools");
constexpr TCHAR CafeTableAssetPath[] = TEXT("/Game/Furniture/Cafe_Tables");
constexpr TCHAR DiningChairAssetPath[] = TEXT("/Game/Furniture/Dining_Seating");
constexpr TCHAR DiningTableAssetPath[] = TEXT("/Game/Furniture/Dining_Tables");
constexpr TCHAR DiningTableclothAssetPath[] = TEXT("/Game/Furniture/Dining_Seating/BBJ_Linens/Table_Cloths");
constexpr TCHAR MajicGoboAssetPath[] = TEXT("/Game/Majic_Gear/Lighting/Majic_Gobos");
constexpr TCHAR FrontBarAssetPath[] = TEXT("/Game/Furniture/Bars/Front_Bars");
constexpr TCHAR BackBarAssetPath[] = TEXT("/Game/Furniture/Bars/Back_Bars");

TArray<TPair<FString, UDMXEntityFixturePatch*>> GetLightingFixturePatchOptions()
{
	TArray<TPair<FString, UDMXEntityFixturePatch*>> Options;
	Options.Emplace(TEXT("Unpatched"), nullptr);

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> LibraryAssets;
	AssetRegistryModule.Get().GetAssetsByClass(UDMXLibrary::StaticClass()->GetClassPathName(), LibraryAssets, true);
	LibraryAssets.RemoveAll([](const FAssetData& Asset) { return !Asset.PackagePath.ToString().StartsWith(TEXT("/Game")); });
	LibraryAssets.Sort([](const FAssetData& A, const FAssetData& B) { return A.GetSoftObjectPath().ToString() < B.GetSoftObjectPath().ToString(); });
	const bool bShowLibraryName = LibraryAssets.Num() > 1;
	for (const FAssetData& LibraryAsset : LibraryAssets)
	{
		UDMXLibrary* Library = Cast<UDMXLibrary>(LibraryAsset.GetAsset());
		if (!Library)
		{
			continue;
		}

		const FString LibraryPrefix = bShowLibraryName ? Library->GetName() + TEXT(" / ") : FString();
		for (UDMXEntityFixturePatch* Patch : Library->GetEntitiesTypeCast<UDMXEntityFixturePatch>())
		{
			if (Patch)
			{
				Options.Emplace(FString::Printf(TEXT("%s%s (U%d.%d, %d ch)"),
					*LibraryPrefix, *Patch->Name, Patch->GetUniverseID(), Patch->GetStartingChannel(), Patch->GetChannelSpan()), Patch);
			}
		}
	}
	return Options;
}

TArray<const FDMXFixtureFunction*> GetLightingControlFunctions(const UDMXEntityFixturePatch* Patch)
{
	TArray<const FDMXFixtureFunction*> Result;
	const FDMXFixtureMode* Mode = Patch ? Patch->GetActiveMode() : nullptr;
	if (!Mode)
	{
		return Result;
	}

	static const FName PreferredAttributes[] = {
		TEXT("Dimmer"), TEXT("Pan"), TEXT("Tilt"),
		TEXT("Red"), TEXT("Green"), TEXT("Blue"),
		TEXT("Cyan"), TEXT("Magenta"), TEXT("Yellow"),
		TEXT("White"), TEXT("Amber"), TEXT("Color"), TEXT("Shutter")
	};
	for (const FName AttributeName : PreferredAttributes)
	{
		for (const FDMXFixtureFunction& Function : Mode->Functions)
		{
			if (Function.Attribute.Name == AttributeName && Function.GetNumChannels() <= 3)
			{
				Result.Add(&Function);
				break;
			}
		}
		if (Result.Num() == 7)
		{
			break;
		}
	}
	return Result;
}

FName LightingControlWidgetName(const TCHAR* Prefix, int32 Index)
{
	return FName(*FString::Printf(TEXT("%s%d"), Prefix, Index));
}

static EBuildItemType BuildTabForItemType(EBuildItemType ItemType)
{
	if (ItemType == EBuildItemType::SectionalLayout)
	{
		return EBuildItemType::LoungeLayout;
	}

	if (ItemType == EBuildItemType::MBPWall ||
		ItemType == EBuildItemType::StageDeck ||
		ItemType == EBuildItemType::DrapeRun ||
		ItemType == EBuildItemType::VideoPlacement ||
		ItemType == EBuildItemType::VideoWall ||
		ItemType == EBuildItemType::ProjectionScreen ||
		ItemType == EBuildItemType::AudioPlacement ||
		ItemType == EBuildItemType::LoungeLayout ||
		ItemType == EBuildItemType::FurnitureSeating ||
		ItemType == EBuildItemType::BarLayout ||
		ItemType == EBuildItemType::DiningTable)
	{
		return ItemType;
	}

	return EBuildItemType::TrussStructure;
}

static EBuildItemType BuildDesignerTabForItemType(EBuildItemType ItemType)
{
	if (ItemType == EBuildItemType::MBPWall ||
		ItemType == EBuildItemType::StageDeck ||
		ItemType == EBuildItemType::DrapeRun)
	{
		return EBuildItemType::StageDeck;
	}

	if (ItemType == EBuildItemType::VideoPlacement ||
		ItemType == EBuildItemType::VideoWall ||
		ItemType == EBuildItemType::ProjectionScreen ||
		ItemType == EBuildItemType::AudioPlacement)
	{
		return EBuildItemType::VideoPlacement;
	}

	if (ItemType == EBuildItemType::LoungeLayout ||
		ItemType == EBuildItemType::SectionalLayout ||
		ItemType == EBuildItemType::FurnitureSeating ||
		ItemType == EBuildItemType::BarLayout ||
		ItemType == EBuildItemType::DiningTable)
	{
		return EBuildItemType::LoungeLayout;
	}

	return BuildTabForItemType(ItemType);
}

static bool IsDiningTableContext(EBuildItemType ActiveMenuTab, const UBuildItemDataAsset* SelectedBuildItem)
{
	return ActiveMenuTab == EBuildItemType::DiningTable ||
		(SelectedBuildItem && SelectedBuildItem->ItemType == EBuildItemType::DiningTable);
}

static FString BuildDesignerCategoryLabel(EBuildItemType ItemType)
{
	switch (BuildDesignerTabForItemType(ItemType))
	{
	case EBuildItemType::StageDeck:
		return TEXT("Stage");
	case EBuildItemType::VideoPlacement:
		return TEXT("AV");
	case EBuildItemType::LoungeLayout:
		return TEXT("Furniture");
	case EBuildItemType::TrussStructure:
	default:
		return TEXT("Truss");
	}
}

static FString BuildItemTypeLabel(EBuildItemType ItemType)
{
	switch (ItemType)
	{
	case EBuildItemType::MBPWall:
		return TEXT("MBP Wall");
	case EBuildItemType::StageDeck:
		return TEXT("Stage");
	case EBuildItemType::DrapeRun:
		return TEXT("Drape");
	case EBuildItemType::VideoPlacement:
		return TEXT("TV");
	case EBuildItemType::VideoWall:
		return TEXT("Video Wall");
	case EBuildItemType::ProjectionScreen:
		return TEXT("Projection");
	case EBuildItemType::AudioPlacement:
		return TEXT("Audio");
	case EBuildItemType::LoungeLayout:
		return TEXT("Lounge");
	case EBuildItemType::SectionalLayout:
		return TEXT("Sectional");
	case EBuildItemType::FurnitureSeating:
		return TEXT("Seating");
	case EBuildItemType::BarLayout:
		return TEXT("Bars");
	case EBuildItemType::DiningTable:
		return TEXT("Dining");
	case EBuildItemType::TrussStructure:
	default:
		return TEXT("Truss");
	}
}

static FString FurnitureSeatingTypeToMenuLabel(EFurnitureSeatingRuntimeType SeatingType)
{
	return SeatingType == EFurnitureSeatingRuntimeType::CafeTable ? TEXT("Cafe Tables") : TEXT("High Seating");
}

static FString BuildDesignerSelectionLabel(const UBuildItemDataAsset* BuildItem, EBuildItemType ActiveMenuTab)
{
	const EBuildItemType ItemType = BuildItem ? BuildItem->ItemType : ActiveMenuTab;
	const FString CategoryLabel = BuildDesignerCategoryLabel(ItemType);
	const FString TypeLabel = BuildItemTypeLabel(ItemType);
	if (!BuildItem)
	{
		return FString::Printf(TEXT("%s > %s"), *CategoryLabel, *TypeLabel);
	}

	const FString DisplayName = BuildItem->DisplayName.IsEmpty()
		? BuildItem->ItemId.ToString()
		: BuildItem->DisplayName.ToString();
	return FString::Printf(TEXT("%s > %s"), *CategoryLabel, *DisplayName);
}

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

static UBuildItemDataAsset* FindMatchingBuildItemForAudioActor(const TArray<TObjectPtr<UBuildItemDataAsset>>& BuildItems, const AActor* AudioActor)
{
	if (!AudioActor)
	{
		return nullptr;
	}

	for (UBuildItemDataAsset* BuildItem : BuildItems)
	{
		if (BuildItem && BuildItem->ItemType == EBuildItemType::AudioPlacement)
		{
			return BuildItem;
		}
	}

	return nullptr;
}

static UBuildItemDataAsset* FindMatchingBuildItemForLoungeActor(const TArray<TObjectPtr<UBuildItemDataAsset>>& BuildItems, const ALoungeLayoutActor* LoungeActor)
{
	if (!LoungeActor)
	{
		return nullptr;
	}

	for (UBuildItemDataAsset* BuildItem : BuildItems)
	{
		if (BuildItem && BuildItem->ItemType == EBuildItemType::LoungeLayout)
		{
			return BuildItem;
		}
	}

	return nullptr;
}

static UBuildItemDataAsset* FindMatchingBuildItemForSectionalActor(const TArray<TObjectPtr<UBuildItemDataAsset>>& BuildItems, const ASectionalLayoutActor* SectionalActor)
{
	if (!SectionalActor)
	{
		return nullptr;
	}

	for (UBuildItemDataAsset* BuildItem : BuildItems)
	{
		if (BuildItem && BuildItem->ItemType == EBuildItemType::SectionalLayout)
		{
			return BuildItem;
		}
	}

	return nullptr;
}

static UBuildItemDataAsset* FindMatchingBuildItemForFurnitureSeatingActor(const TArray<TObjectPtr<UBuildItemDataAsset>>& BuildItems, const AActor* SeatingActor)
{
	if (!SeatingActor)
	{
		return nullptr;
	}

	for (UBuildItemDataAsset* BuildItem : BuildItems)
	{
		if (BuildItem && BuildItem->ItemType == EBuildItemType::FurnitureSeating)
		{
			return BuildItem;
		}
	}

	return nullptr;
}

static UBuildItemDataAsset* FindMatchingBuildItemForBarLayoutActor(const TArray<TObjectPtr<UBuildItemDataAsset>>& BuildItems, const ABarLayoutActor* BarLayoutActor)
{
	if (!BarLayoutActor)
	{
		return nullptr;
	}

	for (UBuildItemDataAsset* BuildItem : BuildItems)
	{
		if (BuildItem && BuildItem->ItemType == EBuildItemType::BarLayout)
		{
			return BuildItem;
		}
	}

	return nullptr;
}

static UBuildItemDataAsset* FindMatchingBuildItemForDiningTableActor(const TArray<TObjectPtr<UBuildItemDataAsset>>& BuildItems, const ADiningTableActor* DiningTableActor)
{
	if (!DiningTableActor)
	{
		return nullptr;
	}

	for (UBuildItemDataAsset* BuildItem : BuildItems)
	{
		if (BuildItem && BuildItem->ItemType == EBuildItemType::DiningTable)
		{
			return BuildItem;
		}
	}

	return nullptr;
}

static UBuildItemDataAsset* FindMatchingBuildItemForStageActor(const TArray<TObjectPtr<UBuildItemDataAsset>>& BuildItems, const AStageDeckActor* StageActor)
{
	if (!StageActor)
	{
		return nullptr;
	}

	for (UBuildItemDataAsset* BuildItem : BuildItems)
	{
		if (!BuildItem || BuildItem->ItemType != EBuildItemType::StageDeck)
		{
			continue;
		}

		if (BuildItem->BuildActorClass && StageActor->IsA(BuildItem->BuildActorClass))
		{
			return BuildItem;
		}
	}

	return nullptr;
}

static UBuildItemDataAsset* FindMatchingBuildItemForDrapeActor(const TArray<TObjectPtr<UBuildItemDataAsset>>& BuildItems, const ADrapeRunActor* DrapeActor)
{
	if (!DrapeActor)
	{
		return nullptr;
	}

	for (UBuildItemDataAsset* BuildItem : BuildItems)
	{
		if (!BuildItem || BuildItem->ItemType != EBuildItemType::DrapeRun)
		{
			continue;
		}

		if (BuildItem->BuildActorClass && DrapeActor->IsA(BuildItem->BuildActorClass))
		{
			return BuildItem;
		}
	}

	return nullptr;
}

static UBuildItemDataAsset* FindMatchingBuildItemForVideoActor(const TArray<TObjectPtr<UBuildItemDataAsset>>& BuildItems, const AVideoPlacementActor* VideoActor)
{
	if (!VideoActor)
	{
		return nullptr;
	}

	for (UBuildItemDataAsset* BuildItem : BuildItems)
	{
		if (!BuildItem || BuildItem->ItemType != EBuildItemType::VideoPlacement)
		{
			continue;
		}

		if (BuildItem->BuildActorClass && VideoActor->IsA(BuildItem->BuildActorClass))
		{
			return BuildItem;
		}
	}

	return nullptr;
}

static UBuildItemDataAsset* FindMatchingBuildItemForVideoWallActor(const TArray<TObjectPtr<UBuildItemDataAsset>>& BuildItems, const AVideoWallActor* VideoWallActor)
{
	if (!VideoWallActor)
	{
		return nullptr;
	}

	for (UBuildItemDataAsset* BuildItem : BuildItems)
	{
		if (!BuildItem || BuildItem->ItemType != EBuildItemType::VideoWall)
		{
			continue;
		}

		if (BuildItem->BuildActorClass && VideoWallActor->IsA(BuildItem->BuildActorClass))
		{
			return BuildItem;
		}
	}

	return nullptr;
}

static UBuildItemDataAsset* FindMatchingBuildItemForProjectionActor(const TArray<TObjectPtr<UBuildItemDataAsset>>& BuildItems, const AProjectionScreenActor* ProjectionActor)
{
	if (!ProjectionActor)
	{
		return nullptr;
	}

	for (UBuildItemDataAsset* BuildItem : BuildItems)
	{
		if (!BuildItem || BuildItem->ItemType != EBuildItemType::ProjectionScreen)
		{
			continue;
		}

		if (BuildItem->BuildActorClass && ProjectionActor->IsA(BuildItem->BuildActorClass))
		{
			return BuildItem;
		}
	}

	return nullptr;
}

static TArray<FString> GetLoungeItemOptionsFromFolder(const FString& CategoryFolder)
{
	TSet<FString> Options;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> Assets;
	AssetRegistryModule.Get().GetAssetsByPath(*CategoryFolder, Assets, true);

	for (const FAssetData& Asset : Assets)
	{
		if (Asset.AssetClassPath.GetAssetName() != TEXT("StaticMesh"))
		{
			continue;
		}

		FString PackagePath = Asset.PackagePath.ToString();
		PackagePath.RemoveFromStart(CategoryFolder);
		PackagePath.RemoveFromStart(TEXT("/"));

		FString ItemName;
		if (PackagePath.Split(TEXT("/"), &ItemName, nullptr))
		{
			Options.Add(ItemName);
		}
		else if (!PackagePath.IsEmpty())
		{
			Options.Add(PackagePath);
		}
		else
		{
			Options.Add(Asset.AssetName.ToString());
		}
	}

	TArray<FString> SortedOptions = Options.Array();
	SortedOptions.Sort();
	return SortedOptions;
}

static TArray<FString> GetFlatAssetOptionsFromFolder(const FString& AssetFolder)
{
	TArray<FString> Options;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> Assets;
	AssetRegistryModule.Get().GetAssetsByPath(*AssetFolder, Assets, false);

	for (const FAssetData& Asset : Assets)
	{
		const FName AssetClassName = Asset.AssetClassPath.GetAssetName();
		const bool bSupportedAsset =
			AssetClassName == TEXT("Texture2D") ||
			AssetClassName == TEXT("Material") ||
			AssetClassName == TEXT("MaterialInstance") ||
			AssetClassName == TEXT("MaterialInstanceConstant");
		if (!bSupportedAsset || Asset.AssetName == TEXT("UE_Placeholder"))
		{
			continue;
		}

		Options.Add(Asset.AssetName.ToString());
	}

	Options.Sort();
	return Options;
}

static FString EnsureLoungeOption(FString CurrentOption, const TArray<FString>& Options)
{
	if (!CurrentOption.IsEmpty() && Options.Contains(CurrentOption))
	{
		return CurrentOption;
	}

	return Options.Num() > 0 ? Options[0] : FString();
}
}

void UBuildMenuItemButtonProxy::Initialize(UBuildMenuWidget* InOwner, UBuildItemDataAsset* InBuildItem, bool bInHasFurnitureSeatingTypeOverride, EFurnitureSeatingRuntimeType InFurnitureSeatingType)
{
	Owner = InOwner;
	BuildItem = InBuildItem;
	bHasFurnitureSeatingTypeOverride = bInHasFurnitureSeatingTypeOverride;
	FurnitureSeatingType = InFurnitureSeatingType;
}

void UBuildMenuItemButtonProxy::HandleClicked()
{
	if (Owner.IsValid())
	{
		if (bHasFurnitureSeatingTypeOverride)
		{
			Owner->SetSelectedFurnitureSeatingBuildItem(BuildItem.Get(), FurnitureSeatingType);
		}
		else
		{
			Owner->SetSelectedBuildItem(BuildItem.Get());
		}
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
		CurrentStageDeckDefinition = SelectedBuildItem->DefaultStageDeckDefinition;
		CurrentDrapeRunDefinition = SelectedBuildItem->DefaultDrapeRunDefinition;
		CurrentVideoPlacementDefinition = SelectedBuildItem->DefaultVideoPlacementDefinition;
		CurrentVideoWallDefinition = SelectedBuildItem->DefaultVideoWallDefinition;
		CurrentProjectionScreenDefinition = SelectedBuildItem->DefaultProjectionScreenDefinition;
		CurrentAudioPlacementDefinition = SelectedBuildItem->DefaultAudioPlacementDefinition;
		CurrentLoungeLayoutDefinition = SelectedBuildItem->DefaultLoungeLayoutDefinition;
		CurrentSectionalLayoutDefinition = SelectedBuildItem->DefaultSectionalLayoutDefinition;
		CurrentFurnitureSeatingDefinition = SelectedBuildItem->DefaultFurnitureSeatingDefinition;
		CurrentBarLayoutDefinition = SelectedBuildItem->DefaultBarLayoutDefinition;
		CurrentDiningTableDefinition = SelectedBuildItem->DefaultDiningTableDefinition;
		ActiveMenuTab = BuildTabForItemType(SelectedBuildItem->ItemType);
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
		CurrentStageDeckDefinition = InSelectedItem->DefaultStageDeckDefinition;
		CurrentDrapeRunDefinition = InSelectedItem->DefaultDrapeRunDefinition;
		CurrentVideoPlacementDefinition = InSelectedItem->DefaultVideoPlacementDefinition;
		CurrentVideoWallDefinition = InSelectedItem->DefaultVideoWallDefinition;
		CurrentProjectionScreenDefinition = InSelectedItem->DefaultProjectionScreenDefinition;
		CurrentAudioPlacementDefinition = InSelectedItem->DefaultAudioPlacementDefinition;
		CurrentLoungeLayoutDefinition = InSelectedItem->DefaultLoungeLayoutDefinition;
		CurrentSectionalLayoutDefinition = InSelectedItem->DefaultSectionalLayoutDefinition;
		CurrentFurnitureSeatingDefinition = InSelectedItem->DefaultFurnitureSeatingDefinition;
		CurrentBarLayoutDefinition = InSelectedItem->DefaultBarLayoutDefinition;
		CurrentDiningTableDefinition = InSelectedItem->DefaultDiningTableDefinition;
		ActiveMenuTab = BuildTabForItemType(InSelectedItem->ItemType);
	}

	if (BuildManager && InSelectedItem)
	{
		BuildManager->SetSelectedBuildItem(InSelectedItem);
		ApplyTrussDefinitionToBuildManager();
		ApplyMBPDefinitionToBuildManager();
		ApplyStageDefinitionToBuildManager();
		ApplyDrapeDefinitionToBuildManager();
		ApplyVideoDefinitionToBuildManager();
		ApplyVideoWallDefinitionToBuildManager();
		ApplyProjectionDefinitionToBuildManager();
		ApplyAudioDefinitionToBuildManager();
		ApplyLoungeDefinitionToBuildManager();
		ApplySectionalDefinitionToBuildManager();
		ApplyFurnitureSeatingDefinitionToBuildManager();
		ApplyBarLayoutDefinitionToBuildManager();
		ApplyDiningTableDefinitionToBuildManager();
	}

	RefreshMenu();
}

void UBuildMenuWidget::SetSelectedFurnitureSeatingBuildItem(UBuildItemDataAsset* InSelectedItem, EFurnitureSeatingRuntimeType SeatingType)
{
	if (!InSelectedItem || InSelectedItem->ItemType != EBuildItemType::FurnitureSeating)
	{
		SetSelectedBuildItem(InSelectedItem);
		return;
	}

	SetSelectedBuildItem(InSelectedItem);
	CurrentFurnitureSeatingDefinition.SeatingType = SeatingType;
	if (BuildManager)
	{
		ApplyFurnitureSeatingDefinitionToBuildManager();
	}
	RefreshMenu();
}

void UBuildMenuWidget::SetBuildManager(UBuildManagerComponent* InBuildManager)
{
	BuildManager = InBuildManager;
}

void UBuildMenuWidget::RefreshMenu()
{
	BindDesignerWidgetTree();
	if (UWidget* LightingPanel = WidgetTree ? WidgetTree->FindWidget(TEXT("LightingControlPanel")) : nullptr)
	{
		LightingPanel->SetVisibility(EditingLightTarget ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (DesignerRotationSlider)
	{
		DesignerRotationSlider->SetVisibility(EditingLightTarget ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	if (!EditingLightTarget)
	{
		for (UWidget* StageLabel : { StageFrontRailingLabelText.Get(), StageBackRailingLabelText.Get(), StageLeftRailingLabelText.Get(), StageRightRailingLabelText.Get(), StageLeftStepLabelText.Get(), StageRightStepLabelText.Get(), StageAutomaticSkirtLabelText.Get(), StageCellEnabledLabelText.Get() })
		{
			if (StageLabel && StageLabel->GetParent())
			{
				StageLabel->GetParent()->SetVisibility(ESlateVisibility::Visible);
			}
		}
	}

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
	RefreshEditActorActionButtons();

	if (EditingLightTarget)
	{
		RefreshLightingControls();
	}
	else switch (ActiveMenuTab)
	{
	case EBuildItemType::MBPWall:
		RefreshMBPControls();
		break;
	case EBuildItemType::StageDeck:
		if (SelectedBuildItem && SelectedBuildItem->ItemType == EBuildItemType::MBPWall)
		{
			RefreshMBPControls();
		}
		else if (SelectedBuildItem && SelectedBuildItem->ItemType == EBuildItemType::DrapeRun)
		{
			RefreshDrapeControls();
		}
		else
		{
			RefreshStageControls();
		}
		break;
	case EBuildItemType::DrapeRun:
		RefreshDrapeControls();
		break;
	case EBuildItemType::VideoPlacement:
		if (SelectedBuildItem && SelectedBuildItem->ItemType == EBuildItemType::VideoWall)
		{
			RefreshVideoWallControls();
		}
		else if (SelectedBuildItem && SelectedBuildItem->ItemType == EBuildItemType::ProjectionScreen)
		{
			RefreshProjectionControls();
		}
		else if (SelectedBuildItem && SelectedBuildItem->ItemType == EBuildItemType::AudioPlacement)
		{
			RefreshAudioControls();
		}
		else
		{
			RefreshVideoControls();
		}
		break;
	case EBuildItemType::VideoWall:
		RefreshVideoWallControls();
		break;
	case EBuildItemType::ProjectionScreen:
		RefreshProjectionControls();
		break;
	case EBuildItemType::AudioPlacement:
		RefreshAudioControls();
		break;
	case EBuildItemType::LoungeLayout:
		if (SelectedBuildItem && SelectedBuildItem->ItemType == EBuildItemType::SectionalLayout)
		{
			RefreshSectionalControls();
		}
		else if (SelectedBuildItem && SelectedBuildItem->ItemType == EBuildItemType::FurnitureSeating)
		{
			RefreshFurnitureSeatingControls();
		}
		else if (SelectedBuildItem && SelectedBuildItem->ItemType == EBuildItemType::BarLayout)
		{
			RefreshBarLayoutControls();
		}
		else if (SelectedBuildItem && SelectedBuildItem->ItemType == EBuildItemType::DiningTable)
		{
			RefreshDiningTableControls();
		}
		else
		{
			RefreshLoungeControls();
		}
		break;
	case EBuildItemType::FurnitureSeating:
		RefreshFurnitureSeatingControls();
		break;
	case EBuildItemType::BarLayout:
		RefreshBarLayoutControls();
		break;
	case EBuildItemType::DiningTable:
		RefreshDiningTableControls();
		break;
	case EBuildItemType::TrussStructure:
	default:
		RefreshTrussControls();
		break;
	}
	RefreshTabButtons();
	RebuildItemButtons();
	RefreshDesignerSummary();
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

FStageDeckBuildDefinition UBuildMenuWidget::GetCurrentStageDeckDefinition() const
{
	return CurrentStageDeckDefinition;
}

FDrapeRunBuildDefinition UBuildMenuWidget::GetCurrentDrapeRunDefinition() const
{
	return CurrentDrapeRunDefinition;
}

FVideoPlacementBuildDefinition UBuildMenuWidget::GetCurrentVideoPlacementDefinition() const
{
	return CurrentVideoPlacementDefinition;
}

FVideoWallBuildDefinition UBuildMenuWidget::GetCurrentVideoWallDefinition() const
{
	return CurrentVideoWallDefinition;
}

FProjectionScreenBuildDefinition UBuildMenuWidget::GetCurrentProjectionScreenDefinition() const
{
	return CurrentProjectionScreenDefinition;
}

FAudioPlacementBuildDefinition UBuildMenuWidget::GetCurrentAudioPlacementDefinition() const
{
	return CurrentAudioPlacementDefinition;
}

FLoungeLayoutBuildDefinition UBuildMenuWidget::GetCurrentLoungeLayoutDefinition() const
{
	return CurrentLoungeLayoutDefinition;
}

FSectionalLayoutBuildDefinition UBuildMenuWidget::GetCurrentSectionalLayoutDefinition() const
{
	return CurrentSectionalLayoutDefinition;
}

FFurnitureSeatingBuildDefinition UBuildMenuWidget::GetCurrentFurnitureSeatingDefinition() const
{
	return CurrentFurnitureSeatingDefinition;
}

FBarLayoutBuildDefinition UBuildMenuWidget::GetCurrentBarLayoutDefinition() const
{
	return CurrentBarLayoutDefinition;
}

FDiningTableBuildDefinition UBuildMenuWidget::GetCurrentDiningTableDefinition() const
{
	return CurrentDiningTableDefinition;
}

void UBuildMenuWidget::SetEditingTarget(ATrussStructureActor* InEditingTarget)
{
	EditingTarget = InEditingTarget;
	EditingLightTarget = nullptr;
	EditingMBPTarget = nullptr;
	EditingStageTarget = nullptr;
	EditingDrapeTarget = nullptr;
	EditingVideoTarget = nullptr;
	EditingVideoWallTarget = nullptr;
	EditingProjectionTarget = nullptr;
	EditingAudioGroundSpeakerTarget = nullptr;
	EditingAudioGroundLineArrayTarget = nullptr;
	EditingLoungeTarget = nullptr;
	EditingSectionalTarget = nullptr;
	EditingBarSeatingTarget = nullptr;
	EditingCafeTableTarget = nullptr;
	EditingBarLayoutTarget = nullptr;
	EditingDiningTableTarget = nullptr;

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

void UBuildMenuWidget::SetEditingMBPTarget(AMBPWallActor* InEditingTarget, int32 InTargetRow, int32 InTargetColumn, bool bStartWithPanel)
{
	EditingTarget = nullptr;
	EditingMBPTarget = InEditingTarget;
	EditingStageTarget = nullptr;
	EditingDrapeTarget = nullptr;
	EditingVideoTarget = nullptr;
	EditingVideoWallTarget = nullptr;
	EditingProjectionTarget = nullptr;
	EditingAudioGroundSpeakerTarget = nullptr;
	EditingAudioGroundLineArrayTarget = nullptr;
	EditingLoungeTarget = nullptr;
	EditingSectionalTarget = nullptr;
	EditingBarSeatingTarget = nullptr;
	EditingCafeTableTarget = nullptr;
	EditingBarLayoutTarget = nullptr;
	EditingDiningTableTarget = nullptr;

	if (EditingMBPTarget)
	{
		ActiveMenuTab = EBuildItemType::MBPWall;
		CurrentMBPWallDefinition = EditingMBPTarget->GetWallDefinition();
		CurrentMBPEditTargetRow = FMath::Clamp(InTargetRow, 0, FMath::Max(EditingMBPTarget->Rows - 1, 0));
		CurrentMBPEditTargetColumn = FMath::Clamp(InTargetColumn, 0, FMath::Max(EditingMBPTarget->Columns - 1, 0));
		CurrentMBPEditScope = bStartWithPanel ? EMBPRuntimeEditScope::Panel : EMBPRuntimeEditScope::WholeWall;

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

void UBuildMenuWidget::SetEditingStageTarget(AStageDeckActor* InEditingTarget, int32 InTargetRow, int32 InTargetColumn, bool bStartWithCell)
{
	EditingTarget = nullptr;
	EditingMBPTarget = nullptr;
	EditingStageTarget = InEditingTarget;
	EditingDrapeTarget = nullptr;
	EditingVideoTarget = nullptr;
	EditingVideoWallTarget = nullptr;
	EditingProjectionTarget = nullptr;
	EditingAudioGroundSpeakerTarget = nullptr;
	EditingAudioGroundLineArrayTarget = nullptr;
	EditingLoungeTarget = nullptr;
	EditingSectionalTarget = nullptr;
	EditingBarSeatingTarget = nullptr;
	EditingCafeTableTarget = nullptr;
	EditingBarLayoutTarget = nullptr;
	EditingDiningTableTarget = nullptr;

	if (EditingStageTarget)
	{
		ActiveMenuTab = EBuildItemType::StageDeck;
		CurrentStageDeckDefinition.Columns = EditingStageTarget->Columns;
		CurrentStageDeckDefinition.Rows = EditingStageTarget->Rows;
		CurrentStageDeckDefinition.DefaultHeightPreset = EditingStageTarget->DefaultHeightPreset;
		CurrentStageDeckDefinition.DefaultSurfaceStyle = EditingStageTarget->DefaultSurfaceStyle;
		CurrentStageDeckDefinition.bEnableFrontRailing = EditingStageTarget->bEnableFrontRailing;
		CurrentStageDeckDefinition.bEnableBackRailing = EditingStageTarget->bEnableBackRailing;
		CurrentStageDeckDefinition.bEnableLeftRailing = EditingStageTarget->bEnableLeftRailing;
		CurrentStageDeckDefinition.bEnableRightRailing = EditingStageTarget->bEnableRightRailing;
		CurrentStageDeckDefinition.bEnableLeftStep = EditingStageTarget->bEnableLeftStep;
		CurrentStageDeckDefinition.bEnableRightStep = EditingStageTarget->bEnableRightStep;
		CurrentStageDeckDefinition.bEnableAutomaticSkirt = EditingStageTarget->bEnableAutomaticSkirt;
		CurrentStageEditScope = bStartWithCell ? EStageRuntimeEditScope::Cell : EStageRuntimeEditScope::WholeStage;
		CurrentStageEditTargetRow = FMath::Clamp(InTargetRow, 0, FMath::Max(EditingStageTarget->Rows - 1, 0));
		CurrentStageEditTargetColumn = FMath::Clamp(InTargetColumn, 0, FMath::Max(EditingStageTarget->Columns - 1, 0));
		SyncCurrentStageCellFromTarget();

		if (UBuildItemDataAsset* MatchingItem = FindMatchingBuildItemForStageActor(BuildItems, EditingStageTarget))
		{
			SelectedBuildItem = MatchingItem;
		}
	}

	RefreshMenu();
}

AStageDeckActor* UBuildMenuWidget::GetEditingStageTarget() const
{
	return EditingStageTarget;
}

void UBuildMenuWidget::SetEditingDrapeTarget(ADrapeRunActor* InEditingTarget)
{
	EditingTarget = nullptr;
	EditingMBPTarget = nullptr;
	EditingStageTarget = nullptr;
	EditingDrapeTarget = InEditingTarget;
	EditingVideoTarget = nullptr;
	EditingVideoWallTarget = nullptr;
	EditingProjectionTarget = nullptr;
	EditingAudioGroundSpeakerTarget = nullptr;
	EditingAudioGroundLineArrayTarget = nullptr;
	EditingLoungeTarget = nullptr;
	EditingSectionalTarget = nullptr;
	EditingBarSeatingTarget = nullptr;
	EditingCafeTableTarget = nullptr;
	EditingBarLayoutTarget = nullptr;
	EditingDiningTableTarget = nullptr;

	if (EditingDrapeTarget)
	{
		ActiveMenuTab = EBuildItemType::DrapeRun;
		CurrentDrapeRunDefinition = EditingDrapeTarget->GetBuildDefinition();
		if (UBuildItemDataAsset* MatchingItem = FindMatchingBuildItemForDrapeActor(BuildItems, EditingDrapeTarget))
		{
			SelectedBuildItem = MatchingItem;
		}
	}

	RefreshMenu();
}

ADrapeRunActor* UBuildMenuWidget::GetEditingDrapeTarget() const
{
	return EditingDrapeTarget;
}

void UBuildMenuWidget::SetEditingVideoTarget(AVideoPlacementActor* InEditingTarget)
{
	EditingTarget = nullptr;
	EditingMBPTarget = nullptr;
	EditingStageTarget = nullptr;
	EditingDrapeTarget = nullptr;
	EditingVideoTarget = InEditingTarget;
	EditingVideoWallTarget = nullptr;
	EditingProjectionTarget = nullptr;
	EditingAudioGroundSpeakerTarget = nullptr;
	EditingAudioGroundLineArrayTarget = nullptr;
	EditingLoungeTarget = nullptr;
	EditingSectionalTarget = nullptr;
	EditingBarSeatingTarget = nullptr;
	EditingCafeTableTarget = nullptr;
	EditingBarLayoutTarget = nullptr;
	EditingDiningTableTarget = nullptr;

	if (EditingVideoTarget)
	{
		ActiveMenuTab = EBuildItemType::VideoPlacement;
		CurrentVideoPlacementDefinition = EditingVideoTarget->GetBuildDefinition();
		if (UBuildItemDataAsset* MatchingItem = FindMatchingBuildItemForVideoActor(BuildItems, EditingVideoTarget))
		{
			SelectedBuildItem = MatchingItem;
		}
	}

	RefreshMenu();
}

AVideoPlacementActor* UBuildMenuWidget::GetEditingVideoTarget() const
{
	return EditingVideoTarget;
}

void UBuildMenuWidget::SetEditingVideoWallTarget(AVideoWallActor* InEditingTarget)
{
	EditingTarget = nullptr;
	EditingMBPTarget = nullptr;
	EditingStageTarget = nullptr;
	EditingDrapeTarget = nullptr;
	EditingVideoTarget = nullptr;
	EditingVideoWallTarget = InEditingTarget;
	EditingProjectionTarget = nullptr;
	EditingAudioGroundSpeakerTarget = nullptr;
	EditingAudioGroundLineArrayTarget = nullptr;
	EditingLoungeTarget = nullptr;
	EditingSectionalTarget = nullptr;
	EditingBarSeatingTarget = nullptr;
	EditingCafeTableTarget = nullptr;
	EditingBarLayoutTarget = nullptr;
	EditingDiningTableTarget = nullptr;

	if (EditingVideoWallTarget)
	{
		ActiveMenuTab = EBuildItemType::VideoWall;
		CurrentVideoWallDefinition = EditingVideoWallTarget->GetBuildDefinition();
		if (UBuildItemDataAsset* MatchingItem = FindMatchingBuildItemForVideoWallActor(BuildItems, EditingVideoWallTarget))
		{
			SelectedBuildItem = MatchingItem;
		}
	}

	RefreshMenu();
}

AVideoWallActor* UBuildMenuWidget::GetEditingVideoWallTarget() const
{
	return EditingVideoWallTarget;
}

void UBuildMenuWidget::SetEditingProjectionTarget(AProjectionScreenActor* InEditingTarget)
{
	EditingTarget = nullptr;
	EditingMBPTarget = nullptr;
	EditingStageTarget = nullptr;
	EditingDrapeTarget = nullptr;
	EditingVideoTarget = nullptr;
	EditingVideoWallTarget = nullptr;
	EditingProjectionTarget = InEditingTarget;
	EditingAudioGroundSpeakerTarget = nullptr;
	EditingAudioGroundLineArrayTarget = nullptr;
	EditingLoungeTarget = nullptr;
	EditingSectionalTarget = nullptr;
	EditingBarSeatingTarget = nullptr;
	EditingCafeTableTarget = nullptr;
	EditingBarLayoutTarget = nullptr;
	EditingDiningTableTarget = nullptr;

	if (EditingProjectionTarget)
	{
		ActiveMenuTab = EBuildItemType::ProjectionScreen;
		CurrentProjectionScreenDefinition = EditingProjectionTarget->GetBuildDefinition();
		if (UBuildItemDataAsset* MatchingItem = FindMatchingBuildItemForProjectionActor(BuildItems, EditingProjectionTarget))
		{
			SelectedBuildItem = MatchingItem;
		}
	}

	RefreshMenu();
}

AProjectionScreenActor* UBuildMenuWidget::GetEditingProjectionTarget() const
{
	return EditingProjectionTarget;
}

void UBuildMenuWidget::SetEditingAudioTarget(AActor* InEditingTarget)
{
	EditingTarget = nullptr;
	EditingMBPTarget = nullptr;
	EditingStageTarget = nullptr;
	EditingDrapeTarget = nullptr;
	EditingVideoTarget = nullptr;
	EditingVideoWallTarget = nullptr;
	EditingProjectionTarget = nullptr;
	EditingLoungeTarget = nullptr;
	EditingSectionalTarget = nullptr;
	EditingAudioGroundSpeakerTarget = Cast<AAudioGroundSpeakerActor>(InEditingTarget);
	EditingAudioGroundLineArrayTarget = Cast<AAudioGroundLineArrayActor>(InEditingTarget);
	EditingBarSeatingTarget = nullptr;
	EditingCafeTableTarget = nullptr;
	EditingBarLayoutTarget = nullptr;
	EditingDiningTableTarget = nullptr;

	if (EditingAudioGroundSpeakerTarget)
	{
		ActiveMenuTab = EBuildItemType::AudioPlacement;
		CurrentAudioPlacementDefinition.PlacementType = EAudioPlacementRuntimeType::GroundSpeaker;
		CurrentAudioPlacementDefinition.GroundSpeakerDefinition = EditingAudioGroundSpeakerTarget->GetBuildDefinition();
		CurrentAudioSource = EditingAudioGroundSpeakerTarget->AudioSource;
	}
	else if (EditingAudioGroundLineArrayTarget)
	{
		ActiveMenuTab = EBuildItemType::AudioPlacement;
		CurrentAudioPlacementDefinition.PlacementType = EAudioPlacementRuntimeType::GroundLineArray;
		CurrentAudioPlacementDefinition.GroundLineArrayDefinition = EditingAudioGroundLineArrayTarget->GetBuildDefinition();
		CurrentAudioSource = EditingAudioGroundLineArrayTarget->AudioSource;
	}
	else
	{
		CurrentAudioSource = nullptr;
	}

	if (IsEditingAudio())
	{
		if (UBuildItemDataAsset* MatchingItem = FindMatchingBuildItemForAudioActor(BuildItems, InEditingTarget))
		{
			SelectedBuildItem = MatchingItem;
		}
	}

	RefreshMenu();
}

AActor* UBuildMenuWidget::GetEditingAudioTarget() const
{
	if (EditingAudioGroundSpeakerTarget)
	{
		return EditingAudioGroundSpeakerTarget;
	}

	return EditingAudioGroundLineArrayTarget;
}

void UBuildMenuWidget::SetEditingLoungeTarget(ALoungeLayoutActor* InEditingTarget)
{
	EditingTarget = nullptr;
	EditingMBPTarget = nullptr;
	EditingStageTarget = nullptr;
	EditingDrapeTarget = nullptr;
	EditingVideoTarget = nullptr;
	EditingVideoWallTarget = nullptr;
	EditingProjectionTarget = nullptr;
	EditingAudioGroundSpeakerTarget = nullptr;
	EditingAudioGroundLineArrayTarget = nullptr;
	EditingLoungeTarget = InEditingTarget;
	EditingSectionalTarget = nullptr;
	EditingBarSeatingTarget = nullptr;
	EditingCafeTableTarget = nullptr;
	EditingBarLayoutTarget = nullptr;
	EditingDiningTableTarget = nullptr;

	if (EditingLoungeTarget)
	{
		ActiveMenuTab = EBuildItemType::LoungeLayout;
		CurrentLoungeLayoutDefinition = EditingLoungeTarget->GetBuildDefinition();
		if (UBuildItemDataAsset* MatchingItem = FindMatchingBuildItemForLoungeActor(BuildItems, EditingLoungeTarget))
		{
			SelectedBuildItem = MatchingItem;
		}
	}

	RefreshMenu();
}

ALoungeLayoutActor* UBuildMenuWidget::GetEditingLoungeTarget() const
{
	return EditingLoungeTarget;
}

void UBuildMenuWidget::SetEditingSectionalTarget(ASectionalLayoutActor* InEditingTarget)
{
	EditingTarget = nullptr;
	EditingMBPTarget = nullptr;
	EditingStageTarget = nullptr;
	EditingDrapeTarget = nullptr;
	EditingVideoTarget = nullptr;
	EditingVideoWallTarget = nullptr;
	EditingProjectionTarget = nullptr;
	EditingAudioGroundSpeakerTarget = nullptr;
	EditingAudioGroundLineArrayTarget = nullptr;
	EditingLoungeTarget = nullptr;
	EditingSectionalTarget = InEditingTarget;
	EditingBarSeatingTarget = nullptr;
	EditingCafeTableTarget = nullptr;
	EditingBarLayoutTarget = nullptr;
	EditingDiningTableTarget = nullptr;

	if (EditingSectionalTarget)
	{
		ActiveMenuTab = EBuildItemType::LoungeLayout;
		CurrentSectionalLayoutDefinition = EditingSectionalTarget->GetBuildDefinition();
		if (UBuildItemDataAsset* MatchingItem = FindMatchingBuildItemForSectionalActor(BuildItems, EditingSectionalTarget))
		{
			SelectedBuildItem = MatchingItem;
		}
	}

	RefreshMenu();
}

ASectionalLayoutActor* UBuildMenuWidget::GetEditingSectionalTarget() const
{
	return EditingSectionalTarget;
}

void UBuildMenuWidget::SetEditingFurnitureSeatingTarget(AActor* InEditingTarget)
{
	EditingTarget = nullptr;
	EditingMBPTarget = nullptr;
	EditingStageTarget = nullptr;
	EditingDrapeTarget = nullptr;
	EditingVideoTarget = nullptr;
	EditingVideoWallTarget = nullptr;
	EditingProjectionTarget = nullptr;
	EditingAudioGroundSpeakerTarget = nullptr;
	EditingAudioGroundLineArrayTarget = nullptr;
	EditingLoungeTarget = nullptr;
	EditingSectionalTarget = nullptr;
	EditingBarSeatingTarget = Cast<ABarSeatingActor>(InEditingTarget);
	EditingCafeTableTarget = Cast<ACafeTableActor>(InEditingTarget);
	EditingBarLayoutTarget = nullptr;
	EditingDiningTableTarget = nullptr;

	if (EditingBarSeatingTarget)
	{
		ActiveMenuTab = EBuildItemType::FurnitureSeating;
		CurrentFurnitureSeatingDefinition.SeatingType = EFurnitureSeatingRuntimeType::BarSeating;
		CurrentFurnitureSeatingDefinition.BarSeatingDefinition = EditingBarSeatingTarget->GetBuildDefinition();
	}
	else if (EditingCafeTableTarget)
	{
		ActiveMenuTab = EBuildItemType::FurnitureSeating;
		CurrentFurnitureSeatingDefinition.SeatingType = EFurnitureSeatingRuntimeType::CafeTable;
		CurrentFurnitureSeatingDefinition.CafeTableDefinition = EditingCafeTableTarget->GetBuildDefinition();
	}

	if (IsEditingFurnitureSeating())
	{
		if (UBuildItemDataAsset* MatchingItem = FindMatchingBuildItemForFurnitureSeatingActor(BuildItems, InEditingTarget))
		{
			SelectedBuildItem = MatchingItem;
		}
	}

	RefreshMenu();
}

AActor* UBuildMenuWidget::GetEditingFurnitureSeatingTarget() const
{
	if (EditingBarSeatingTarget)
	{
		return EditingBarSeatingTarget;
	}

	return EditingCafeTableTarget;
}

void UBuildMenuWidget::SetEditingBarLayoutTarget(ABarLayoutActor* InEditingTarget)
{
	EditingTarget = nullptr;
	EditingMBPTarget = nullptr;
	EditingStageTarget = nullptr;
	EditingDrapeTarget = nullptr;
	EditingVideoTarget = nullptr;
	EditingVideoWallTarget = nullptr;
	EditingProjectionTarget = nullptr;
	EditingAudioGroundSpeakerTarget = nullptr;
	EditingAudioGroundLineArrayTarget = nullptr;
	EditingLoungeTarget = nullptr;
	EditingSectionalTarget = nullptr;
	EditingBarSeatingTarget = nullptr;
	EditingCafeTableTarget = nullptr;
	EditingBarLayoutTarget = InEditingTarget;
	EditingDiningTableTarget = nullptr;

	if (EditingBarLayoutTarget)
	{
		ActiveMenuTab = EBuildItemType::BarLayout;
		CurrentBarLayoutDefinition = EditingBarLayoutTarget->GetBuildDefinition();
		if (UBuildItemDataAsset* MatchingItem = FindMatchingBuildItemForBarLayoutActor(BuildItems, EditingBarLayoutTarget))
		{
			SelectedBuildItem = MatchingItem;
		}
	}

	RefreshMenu();
}

ABarLayoutActor* UBuildMenuWidget::GetEditingBarLayoutTarget() const
{
	return EditingBarLayoutTarget;
}

void UBuildMenuWidget::SetEditingDiningTableTarget(ADiningTableActor* InEditingTarget)
{
	EditingTarget = nullptr;
	EditingMBPTarget = nullptr;
	EditingStageTarget = nullptr;
	EditingDrapeTarget = nullptr;
	EditingVideoTarget = nullptr;
	EditingVideoWallTarget = nullptr;
	EditingProjectionTarget = nullptr;
	EditingAudioGroundSpeakerTarget = nullptr;
	EditingAudioGroundLineArrayTarget = nullptr;
	EditingLoungeTarget = nullptr;
	EditingSectionalTarget = nullptr;
	EditingBarSeatingTarget = nullptr;
	EditingCafeTableTarget = nullptr;
	EditingBarLayoutTarget = nullptr;
	EditingDiningTableTarget = InEditingTarget;

	if (EditingDiningTableTarget)
	{
		ActiveMenuTab = EBuildItemType::DiningTable;
		CurrentDiningTableDefinition = EditingDiningTableTarget->GetBuildDefinition();
		if (UBuildItemDataAsset* MatchingItem = FindMatchingBuildItemForDiningTableActor(BuildItems, EditingDiningTableTarget))
		{
			SelectedBuildItem = MatchingItem;
		}
	}

	RefreshMenu();
}

ADiningTableActor* UBuildMenuWidget::GetEditingDiningTableTarget() const
{
	return EditingDiningTableTarget;
}

void UBuildMenuWidget::SetEditingLightTarget(ALightFixtureActor* InEditingTarget)
{
	if (InEditingTarget)
	{
		SetEditingTarget(nullptr);
	}
	EditingLightTarget = InEditingTarget;
	RefreshMenu();
}

ALightFixtureActor* UBuildMenuWidget::GetEditingLightTarget() const
{
	return EditingLightTarget;
}

void UBuildMenuWidget::SetLightingFixtureOptions(const TArray<FString>& InNames, const TArray<TSubclassOf<AActor>>& InClasses)
{
	LightingFixtureNames = InNames;
	LightingFixtureClasses = InClasses;
	if (EditingLightTarget)
	{
		RefreshMenu();
	}
}

AActor* UBuildMenuWidget::GetEditingActor() const
{
	if (EditingLightTarget)
	{
		return EditingLightTarget;
	}
	if (EditingTarget)
	{
		return EditingTarget;
	}
	if (EditingMBPTarget)
	{
		return EditingMBPTarget;
	}
	if (EditingStageTarget)
	{
		return EditingStageTarget;
	}
	if (EditingDrapeTarget)
	{
		return EditingDrapeTarget;
	}
	if (EditingVideoTarget)
	{
		return EditingVideoTarget;
	}
	if (EditingVideoWallTarget)
	{
		return EditingVideoWallTarget;
	}
	if (EditingProjectionTarget)
	{
		return EditingProjectionTarget;
	}
	if (EditingAudioGroundSpeakerTarget)
	{
		return EditingAudioGroundSpeakerTarget;
	}
	if (EditingAudioGroundLineArrayTarget)
	{
		return EditingAudioGroundLineArrayTarget;
	}
	if (EditingLoungeTarget)
	{
		return EditingLoungeTarget;
	}
	if (EditingSectionalTarget)
	{
		return EditingSectionalTarget;
	}
	if (EditingBarSeatingTarget)
	{
		return EditingBarSeatingTarget;
	}
	if (EditingCafeTableTarget)
	{
		return EditingCafeTableTarget;
	}
	if (EditingBarLayoutTarget)
	{
		return EditingBarLayoutTarget;
	}
	if (EditingDiningTableTarget)
	{
		return EditingDiningTableTarget;
	}
	return nullptr;
}

bool UBuildMenuWidget::ShouldShowPointerForCurrentEdit() const
{
	if (IsEditingMBP())
	{
		return CurrentMBPEditScope != EMBPRuntimeEditScope::WholeWall;
	}

	if (IsEditingStage())
	{
		return CurrentStageEditScope == EStageRuntimeEditScope::Cell;
	}

	return false;
}

void UBuildMenuWidget::SetEditingStageCellTarget(int32 InTargetRow, int32 InTargetColumn)
{
	if (!EditingStageTarget)
	{
		return;
	}

	const int32 ClampedRow = FMath::Clamp(InTargetRow, 0, FMath::Max(EditingStageTarget->Rows - 1, 0));
	const int32 ClampedColumn = FMath::Clamp(InTargetColumn, 0, FMath::Max(EditingStageTarget->Columns - 1, 0));
	if (CurrentStageEditTargetRow == ClampedRow && CurrentStageEditTargetColumn == ClampedColumn)
	{
		return;
	}

	CurrentStageEditTargetRow = ClampedRow;
	CurrentStageEditTargetColumn = ClampedColumn;
	SyncCurrentStageCellFromTarget();
	RefreshMenu();
}

TSharedRef<SWidget> UBuildMenuWidget::RebuildWidget()
{
	if (GetClass() != UBuildMenuWidget::StaticClass())
	{
		bUsesDesignerWidgetTree = true;
		return Super::RebuildWidget();
	}

	bUsesDesignerWidgetTree = false;
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
	if (UHorizontalBoxSlot* MBPTabSlot = TabButtonBox->AddChildToHorizontalBox(MBPTabButton))
	{
		MBPTabSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
	}

	StageTabButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("StageTabButton"));
	StageTabButton->OnClicked.AddDynamic(this, &UBuildMenuWidget::HandleStageTabClicked);
	UTextBlock* StageTabText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StageTabText"));
	StageTabText->SetText(FText::FromString(TEXT("Stage")));
	StageTabText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	StageTabButton->AddChild(StageTabText);
	if (UHorizontalBoxSlot* StageTabSlot = TabButtonBox->AddChildToHorizontalBox(StageTabButton))
	{
		StageTabSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
	}

	DrapeTabButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DrapeTabButton"));
	DrapeTabButton->OnClicked.AddDynamic(this, &UBuildMenuWidget::HandleDrapeTabClicked);
	UTextBlock* DrapeTabText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DrapeTabText"));
	DrapeTabText->SetText(FText::FromString(TEXT("Drape")));
	DrapeTabText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	DrapeTabButton->AddChild(DrapeTabText);
	if (UHorizontalBoxSlot* DrapeTabSlot = TabButtonBox->AddChildToHorizontalBox(DrapeTabButton))
	{
		DrapeTabSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
	}

	VideoTabButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("VideoTabButton"));
	VideoTabButton->OnClicked.AddDynamic(this, &UBuildMenuWidget::HandleVideoTabClicked);
	UTextBlock* VideoTabText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("VideoTabText"));
	VideoTabText->SetText(FText::FromString(TEXT("TV")));
	VideoTabText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	VideoTabButton->AddChild(VideoTabText);
	if (UHorizontalBoxSlot* VideoTabSlot = TabButtonBox->AddChildToHorizontalBox(VideoTabButton))
	{
		VideoTabSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
	}

	VideoWallTabButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("VideoWallTabButton"));
	VideoWallTabButton->OnClicked.AddDynamic(this, &UBuildMenuWidget::HandleVideoWallTabClicked);
	UTextBlock* VideoWallTabText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("VideoWallTabText"));
	VideoWallTabText->SetText(FText::FromString(TEXT("Video Wall")));
	VideoWallTabText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	VideoWallTabButton->AddChild(VideoWallTabText);
	if (UHorizontalBoxSlot* VideoWallTabSlot = TabButtonBox->AddChildToHorizontalBox(VideoWallTabButton))
	{
		VideoWallTabSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
	}

	ProjectionTabButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ProjectionTabButton"));
	ProjectionTabButton->OnClicked.AddDynamic(this, &UBuildMenuWidget::HandleProjectionTabClicked);
	UTextBlock* ProjectionTabText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ProjectionTabText"));
	ProjectionTabText->SetText(FText::FromString(TEXT("Projection")));
	ProjectionTabText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	ProjectionTabButton->AddChild(ProjectionTabText);
	if (UHorizontalBoxSlot* ProjectionTabSlot = TabButtonBox->AddChildToHorizontalBox(ProjectionTabButton))
	{
		ProjectionTabSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
	}

	AudioTabButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("AudioTabButton"));
	AudioTabButton->OnClicked.AddDynamic(this, &UBuildMenuWidget::HandleAudioTabClicked);
	UTextBlock* AudioTabText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AudioTabText"));
	AudioTabText->SetText(FText::FromString(TEXT("Audio")));
	AudioTabText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	AudioTabButton->AddChild(AudioTabText);
	if (UHorizontalBoxSlot* AudioTabSlot = TabButtonBox->AddChildToHorizontalBox(AudioTabButton))
	{
		AudioTabSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
	}

	LoungeTabButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("LoungeTabButton"));
	LoungeTabButton->OnClicked.AddDynamic(this, &UBuildMenuWidget::HandleLoungeTabClicked);
	UTextBlock* LoungeTabText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LoungeTabText"));
	LoungeTabText->SetText(FText::FromString(TEXT("Lounge")));
	LoungeTabText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	LoungeTabButton->AddChild(LoungeTabText);
	TabButtonBox->AddChildToHorizontalBox(LoungeTabButton);

	SeatingTabButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SeatingTabButton"));
	SeatingTabButton->OnClicked.AddDynamic(this, &UBuildMenuWidget::HandleSeatingTabClicked);
	UTextBlock* SeatingTabText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SeatingTabText"));
	SeatingTabText->SetText(FText::FromString(TEXT("Seating")));
	SeatingTabText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	SeatingTabButton->AddChild(SeatingTabText);
	TabButtonBox->AddChildToHorizontalBox(SeatingTabButton);

	BarTabButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BarTabButton"));
	BarTabButton->OnClicked.AddDynamic(this, &UBuildMenuWidget::HandleBarTabClicked);
	UTextBlock* BarTabText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BarTabText"));
	BarTabText->SetText(FText::FromString(TEXT("Bars")));
	BarTabText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	BarTabButton->AddChild(BarTabText);
	TabButtonBox->AddChildToHorizontalBox(BarTabButton);

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
	ModeComboBox->AddOption(BuildModeToOption(ETrussBuildMode::Tower));
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
	CreateLightingControlPanel(RootBox);

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

	StageHeightLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StageHeightLabelText"));
	StageHeightLabelText->SetText(FText::FromString(TEXT("Deck Height")));
	StageHeightLabelText->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.90f, 0.90f)));
	if (UVerticalBoxSlot* StageHeightLabelSlot = RootBox->AddChildToVerticalBox(StageHeightLabelText))
	{
		StageHeightLabelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
	}

	StageHeightComboBox = WidgetTree->ConstructWidget<UWhiteComboBoxString>(UWhiteComboBoxString::StaticClass(), TEXT("StageHeightComboBox"));
	StageHeightComboBox->OnGenerateWidgetEvent.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UBuildMenuWidget, GenerateComboItemWidget));
	StageHeightComboBox->AddOption(StageHeightPresetToOption(EStageDeckHeightPreset::In8));
	StageHeightComboBox->AddOption(StageHeightPresetToOption(EStageDeckHeightPreset::In12));
	StageHeightComboBox->AddOption(StageHeightPresetToOption(EStageDeckHeightPreset::In24));
	StageHeightComboBox->AddOption(StageHeightPresetToOption(EStageDeckHeightPreset::In27));
	StageHeightComboBox->OnSelectionChanged.AddDynamic(this, &UBuildMenuWidget::HandleStageHeightChanged);
	if (UVerticalBoxSlot* StageHeightComboSlot = RootBox->AddChildToVerticalBox(StageHeightComboBox))
	{
		StageHeightComboSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	StageSurfaceLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StageSurfaceLabelText"));
	StageSurfaceLabelText->SetText(FText::FromString(TEXT("Surface Style")));
	StageSurfaceLabelText->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.90f, 0.90f)));
	if (UVerticalBoxSlot* StageSurfaceLabelSlot = RootBox->AddChildToVerticalBox(StageSurfaceLabelText))
	{
		StageSurfaceLabelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
	}

	StageSurfaceComboBox = WidgetTree->ConstructWidget<UWhiteComboBoxString>(UWhiteComboBoxString::StaticClass(), TEXT("StageSurfaceComboBox"));
	StageSurfaceComboBox->OnGenerateWidgetEvent.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UBuildMenuWidget, GenerateComboItemWidget));
	StageSurfaceComboBox->AddOption(StageSurfaceStyleToOption(EStageDeckSurfaceStyle::BlackTop));
	StageSurfaceComboBox->AddOption(StageSurfaceStyleToOption(EStageDeckSurfaceStyle::GrayCarpet));
	StageSurfaceComboBox->OnSelectionChanged.AddDynamic(this, &UBuildMenuWidget::HandleStageSurfaceChanged);
	if (UVerticalBoxSlot* StageSurfaceComboSlot = RootBox->AddChildToVerticalBox(StageSurfaceComboBox))
	{
		StageSurfaceComboSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	auto AddStageCheckBoxRow = [this, RootBox](const TCHAR* LabelName, const TCHAR* CheckBoxName, const TCHAR* LabelText, TObjectPtr<UTextBlock>& OutLabel, TObjectPtr<UCheckBox>& OutCheckBox)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		OutLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), LabelName);
		OutLabel->SetText(FText::FromString(LabelText));
		OutLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.90f, 0.90f)));
		OutCheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), CheckBoxName);

		if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(OutLabel))
		{
			LabelSlot->SetHorizontalAlignment(HAlign_Left);
			LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		if (UHorizontalBoxSlot* CheckSlot = Row->AddChildToHorizontalBox(OutCheckBox))
		{
			CheckSlot->SetHorizontalAlignment(HAlign_Right);
		}

		if (UVerticalBoxSlot* RowSlot = RootBox->AddChildToVerticalBox(Row))
		{
			RowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		}
	};

	AddStageCheckBoxRow(TEXT("StageFrontRailingLabelText"), TEXT("StageFrontRailingCheckBox"), TEXT("Front Railing"), StageFrontRailingLabelText, StageFrontRailingCheckBox);
	StageFrontRailingCheckBox->OnCheckStateChanged.AddDynamic(this, &UBuildMenuWidget::HandleStageFrontRailingChanged);
	AddStageCheckBoxRow(TEXT("StageBackRailingLabelText"), TEXT("StageBackRailingCheckBox"), TEXT("Back Railing"), StageBackRailingLabelText, StageBackRailingCheckBox);
	StageBackRailingCheckBox->OnCheckStateChanged.AddDynamic(this, &UBuildMenuWidget::HandleStageBackRailingChanged);
	AddStageCheckBoxRow(TEXT("StageLeftRailingLabelText"), TEXT("StageLeftRailingCheckBox"), TEXT("Left Railing"), StageLeftRailingLabelText, StageLeftRailingCheckBox);
	StageLeftRailingCheckBox->OnCheckStateChanged.AddDynamic(this, &UBuildMenuWidget::HandleStageLeftRailingChanged);
	AddStageCheckBoxRow(TEXT("StageRightRailingLabelText"), TEXT("StageRightRailingCheckBox"), TEXT("Right Railing"), StageRightRailingLabelText, StageRightRailingCheckBox);
	StageRightRailingCheckBox->OnCheckStateChanged.AddDynamic(this, &UBuildMenuWidget::HandleStageRightRailingChanged);
	AddStageCheckBoxRow(TEXT("StageLeftStepLabelText"), TEXT("StageLeftStepCheckBox"), TEXT("Left Step"), StageLeftStepLabelText, StageLeftStepCheckBox);
	StageLeftStepCheckBox->OnCheckStateChanged.AddDynamic(this, &UBuildMenuWidget::HandleStageLeftStepChanged);
	AddStageCheckBoxRow(TEXT("StageRightStepLabelText"), TEXT("StageRightStepCheckBox"), TEXT("Right Step"), StageRightStepLabelText, StageRightStepCheckBox);
	StageRightStepCheckBox->OnCheckStateChanged.AddDynamic(this, &UBuildMenuWidget::HandleStageRightStepChanged);
	AddStageCheckBoxRow(TEXT("StageAutomaticSkirtLabelText"), TEXT("StageAutomaticSkirtCheckBox"), TEXT("Automatic Skirt"), StageAutomaticSkirtLabelText, StageAutomaticSkirtCheckBox);
	StageAutomaticSkirtCheckBox->OnCheckStateChanged.AddDynamic(this, &UBuildMenuWidget::HandleStageAutomaticSkirtChanged);
	AddStageCheckBoxRow(TEXT("StageCellEnabledLabelText"), TEXT("StageCellEnabledCheckBox"), TEXT("Cell Enabled"), StageCellEnabledLabelText, StageCellEnabledCheckBox);
	StageCellEnabledCheckBox->OnCheckStateChanged.AddDynamic(this, &UBuildMenuWidget::HandleStageCellEnabledChanged);

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

	EditActorActionBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("EditActorActionBox"));
	MoveActorButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("MoveActorButton"));
	MoveActorButton->SetBackgroundColor(FLinearColor(0.18f, 0.45f, 0.70f, 1.0f));
	MoveActorButton->OnClicked.AddDynamic(this, &UBuildMenuWidget::HandleMoveActorButtonClicked);
	UTextBlock* MoveActorButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MoveActorButtonText"));
	MoveActorButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	MoveActorButtonText->SetJustification(ETextJustify::Center);
	MoveActorButtonText->SetText(FText::FromString(TEXT("Move Actor")));
	MoveActorButton->AddChild(MoveActorButtonText);

	RotateActorButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RotateActorButton"));
	RotateActorButton->SetBackgroundColor(FLinearColor(0.70f, 0.45f, 0.18f, 1.0f));
	RotateActorButton->OnClicked.AddDynamic(this, &UBuildMenuWidget::HandleRotateActorButtonClicked);
	UTextBlock* RotateActorButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RotateActorButtonText"));
	RotateActorButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	RotateActorButtonText->SetJustification(ETextJustify::Center);
	RotateActorButtonText->SetText(FText::FromString(TEXT("Rotate Actor")));
	RotateActorButton->AddChild(RotateActorButtonText);

	DeleteActorButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DeleteActorButton"));
	DeleteActorButton->SetBackgroundColor(FLinearColor(0.70f, 0.18f, 0.18f, 1.0f));
	DeleteActorButton->OnClicked.AddDynamic(this, &UBuildMenuWidget::HandleDeleteActorButtonClicked);
	UTextBlock* DeleteActorButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DeleteActorButtonText"));
	DeleteActorButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	DeleteActorButtonText->SetJustification(ETextJustify::Center);
	DeleteActorButtonText->SetText(FText::FromString(TEXT("Delete Actor")));
	DeleteActorButton->AddChild(DeleteActorButtonText);

	if (UHorizontalBoxSlot* MoveButtonSlot = EditActorActionBox->AddChildToHorizontalBox(MoveActorButton))
	{
		MoveButtonSlot->SetPadding(FMargin(0.0f, 0.0f, 6.0f, 0.0f));
		MoveButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	if (UHorizontalBoxSlot* RotateButtonSlot = EditActorActionBox->AddChildToHorizontalBox(RotateActorButton))
	{
		RotateButtonSlot->SetPadding(FMargin(6.0f, 0.0f, 6.0f, 0.0f));
		RotateButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	if (UHorizontalBoxSlot* DeleteButtonSlot = EditActorActionBox->AddChildToHorizontalBox(DeleteActorButton))
	{
		DeleteButtonSlot->SetPadding(FMargin(6.0f, 0.0f, 0.0f, 0.0f));
		DeleteButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	if (UVerticalBoxSlot* EditActionSlot = RootBox->AddChildToVerticalBox(EditActorActionBox))
	{
		EditActionSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	ItemListBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ItemListBox"));
	RootBox->AddChildToVerticalBox(ItemListBox);

	RefreshMenu();
	return Super::RebuildWidget();
}

void UBuildMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindDesignerWidgetTree();
	RefreshMenu();
}

void UBuildMenuWidget::RebuildItemButtons()
{
	if (!ItemListBox && !DesignerBuildGrid)
	{
		return;
	}

	ButtonProxies.Reset();
	if (ItemListBox)
	{
		ItemListBox->ClearChildren();
	}
	if (DesignerBuildGrid)
	{
		DesignerBuildGrid->ClearChildren();
	}

	const bool bShowBuildItemButtons = !EditingLightTarget && !EditingTarget && !IsEditingStage() && !IsEditingDrape() && !IsEditingVideo() && !IsEditingVideoWall() && !IsEditingProjection() && !IsEditingAudio() && !IsEditingLounge() && !IsEditingSectional() && !IsEditingFurnitureSeating() && !IsEditingBarLayout() && !IsEditingDiningTable() && BuildItems.Num() > 1;
	int32 VisibleItemCount = 0;
	for (UBuildItemDataAsset* BuildItem : BuildItems)
	{
		if (BuildItem && ItemBelongsToActiveTab(BuildItem))
		{
			VisibleItemCount += BuildItem->ItemType == EBuildItemType::FurnitureSeating ? 2 : 1;
		}
	}

	if (ItemListBox)
	{
		ItemListBox->SetVisibility(bShowBuildItemButtons && VisibleItemCount > 1 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (DesignerBuildGridScroll)
	{
		DesignerBuildGridScroll->SetVisibility(bShowBuildItemButtons && VisibleItemCount > 1 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (!bShowBuildItemButtons)
	{
		return;
	}

	auto AddBuildItemButton = [this](UBuildItemDataAsset* BuildItem, const FText& LabelText, int32 VisibleIndex, bool bHasFurnitureSeatingTypeOverride = false, EFurnitureSeatingRuntimeType FurnitureSeatingType = EFurnitureSeatingRuntimeType::BarSeating)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		const bool bIsSelected = BuildItem == SelectedBuildItem &&
			(!bHasFurnitureSeatingTypeOverride || CurrentFurnitureSeatingDefinition.SeatingType == FurnitureSeatingType);
		Button->SetBackgroundColor(bIsSelected
			? FLinearColor(0.18f, 0.45f, 0.70f, 1.0f)
			: FLinearColor(0.10f, 0.10f, 0.12f, 1.0f));

		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(LabelText);
		Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Label->SetJustification(ETextJustify::Center);
		Label->SetAutoWrapText(true);
		FSlateFontInfo LabelFont = Label->GetFont();
		LabelFont.Size = bUsesDesignerWidgetTree ? 9 : LabelFont.Size;
		Label->SetFont(LabelFont);
		Button->AddChild(Label);

		UBuildMenuItemButtonProxy* Proxy = NewObject<UBuildMenuItemButtonProxy>(this);
		Proxy->Initialize(this, BuildItem, bHasFurnitureSeatingTypeOverride, FurnitureSeatingType);
		Button->OnClicked.AddDynamic(Proxy, &UBuildMenuItemButtonProxy::HandleClicked);
		ButtonProxies.Add(Proxy);

		if (DesignerBuildGrid)
		{
			if (UUniformGridSlot* GridSlot = DesignerBuildGrid->AddChildToUniformGrid(Button, VisibleIndex / 3, VisibleIndex % 3))
			{
				GridSlot->SetHorizontalAlignment(HAlign_Fill);
				GridSlot->SetVerticalAlignment(VAlign_Fill);
			}
		}
		else if (ItemListBox)
		{
			if (UVerticalBoxSlot* ButtonSlot = ItemListBox->AddChildToVerticalBox(Button))
			{
				ButtonSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
			}
		}
	};

	int32 VisibleIndex = 0;
	for (UBuildItemDataAsset* BuildItem : BuildItems)
	{
		if (!BuildItem || !ItemBelongsToActiveTab(BuildItem))
		{
			continue;
		}

		if (BuildItem->ItemType == EBuildItemType::FurnitureSeating)
		{
			AddBuildItemButton(BuildItem, FText::FromString(FurnitureSeatingTypeToMenuLabel(EFurnitureSeatingRuntimeType::CafeTable)), VisibleIndex++, true, EFurnitureSeatingRuntimeType::CafeTable);
			AddBuildItemButton(BuildItem, FText::FromString(FurnitureSeatingTypeToMenuLabel(EFurnitureSeatingRuntimeType::BarSeating)), VisibleIndex++, true, EFurnitureSeatingRuntimeType::BarSeating);
			continue;
		}

		AddBuildItemButton(BuildItem, BuildItem->DisplayName.IsEmpty() ? FText::FromName(BuildItem->ItemId) : BuildItem->DisplayName, VisibleIndex);
		++VisibleIndex;
	}
}

void UBuildMenuWidget::CreateLightingControlPanel(UVerticalBox* Parent)
{
	if (!Parent || !WidgetTree)
	{
		return;
	}

	UVerticalBox* Panel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LightingControlPanel"));
	Panel->SetVisibility(ESlateVisibility::Collapsed);
	if (UVerticalBoxSlot* PanelSlot = Parent->AddChildToVerticalBox(Panel))
	{
		PanelSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 12.0f));
	}

	UTextBlock* Heading = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LightingControlHeading"));
	Heading->SetText(FText::FromString(TEXT("Control")));
	Heading->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	Panel->AddChildToVerticalBox(Heading);

	UTextBlock* Status = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LightingControlStatus"));
	Status->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.72f, 0.75f)));
	Panel->AddChildToVerticalBox(Status);

	for (int32 Index = 0; Index < 7; ++Index)
	{
		UVerticalBox* Row = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), LightingControlWidgetName(TEXT("LightingControlRow"), Index));
		Row->SetVisibility(ESlateVisibility::Collapsed);
		if (UVerticalBoxSlot* RowSlot = Panel->AddChildToVerticalBox(Row))
		{
			RowSlot->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 2.0f));
		}

		UHorizontalBox* Caption = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Row->AddChildToVerticalBox(Caption);
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), LightingControlWidgetName(TEXT("LightingControlLabel"), Index));
		Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.9f, 0.9f)));
		if (UHorizontalBoxSlot* LabelSlot = Caption->AddChildToHorizontalBox(Label))
		{
			LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		UTextBlock* Value = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), LightingControlWidgetName(TEXT("LightingControlValue"), Index));
		Value->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.82f, 0.9f)));
		Caption->AddChildToHorizontalBox(Value);

		USlider* Slider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), LightingControlWidgetName(TEXT("LightingControlSlider"), Index));
		Slider->SetStepSize(0.01f);
		switch (Index)
		{
		case 0: Slider->OnValueChanged.AddDynamic(this, &UBuildMenuWidget::HandlePrimaryValueChanged); break;
		case 1: Slider->OnValueChanged.AddDynamic(this, &UBuildMenuWidget::HandleSecondaryValueChanged); break;
		case 2: Slider->OnValueChanged.AddDynamic(this, &UBuildMenuWidget::HandleTertiaryValueChanged); break;
		case 3: Slider->OnValueChanged.AddDynamic(this, &UBuildMenuWidget::HandleQuaternaryValueChanged); break;
		case 4: Slider->OnValueChanged.AddDynamic(this, &UBuildMenuWidget::HandleMBPRowsChanged); break;
		case 5: Slider->OnValueChanged.AddDynamic(this, &UBuildMenuWidget::HandleMBPColumnsChanged); break;
		case 6: Slider->OnValueChanged.AddDynamic(this, &UBuildMenuWidget::HandleDesignerRotationChanged); break;
		}
		Row->AddChildToVerticalBox(Slider);
	}

	UBorder* Swatch = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LightingColorSwatch"));
	Swatch->SetVisibility(ESlateVisibility::Collapsed);
	USpacer* SwatchSize = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	SwatchSize->SetSize(FVector2D(1.0f, 16.0f));
	Swatch->AddChild(SwatchSize);
	if (UVerticalBoxSlot* SwatchSlot = Panel->AddChildToVerticalBox(Swatch))
	{
		SwatchSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
	}

	UVerticalBox* GoboPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LightingGoboPanel"));
	GoboPanel->SetVisibility(ESlateVisibility::Collapsed);
	if (UVerticalBoxSlot* GoboSlot = Panel->AddChildToVerticalBox(GoboPanel))
	{
		GoboSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
	}
	UTextBlock* GoboHeading = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	GoboHeading->SetText(FText::FromString(TEXT("Gobo")));
	GoboHeading->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	GoboPanel->AddChildToVerticalBox(GoboHeading);

	UComboBoxString* GoboComboBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("LightingGoboComboBox"));
	GoboComboBox->SetMaxListHeight(360.0f);
	GoboComboBox->OnSelectionChanged.AddDynamic(this, &UBuildMenuWidget::HandleGoboChanged);
	GoboComboBox->OnGenerateWidgetEvent.BindDynamic(this, &UBuildMenuWidget::CreateGoboOptionWidget);
	GoboPanel->AddChildToVerticalBox(GoboComboBox);

	UImage* GoboPreview = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("LightingGoboPreview"));
	GoboPreview->SetDesiredSizeOverride(FVector2D(96.0f, 96.0f));
	GoboPreview->SetVisibility(ESlateVisibility::Collapsed);
	if (UVerticalBoxSlot* PreviewSlot = GoboPanel->AddChildToVerticalBox(GoboPreview))
	{
		PreviewSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
	}
}

void UBuildMenuWidget::CreateDesignerControlPanel(UVerticalBox* PanelBody)
{
	if (!PanelBody || !WidgetTree || ModeComboBox)
	{
		return;
	}

	int32 ControlPanelInsertIndex = 3;
	if (!EditActorActionBox && (!MoveActorButton || !RotateActorButton || !DeleteActorButton))
	{
		EditActorActionBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("DesignerEditActorActionBox"));
		MoveActorButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DesignerMoveActorButton"));
		MoveActorButton->SetBackgroundColor(FLinearColor(0.18f, 0.45f, 0.70f, 1.0f));

		UTextBlock* MoveActorButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DesignerMoveActorButtonText"));
		MoveActorButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		MoveActorButtonText->SetJustification(ETextJustify::Center);
		MoveActorButtonText->SetText(FText::FromString(TEXT("Move")));
		MoveActorButton->AddChild(MoveActorButtonText);

		RotateActorButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DesignerRotateActorButton"));
		RotateActorButton->SetBackgroundColor(FLinearColor(0.70f, 0.45f, 0.18f, 1.0f));

		UTextBlock* RotateActorButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DesignerRotateActorButtonText"));
		RotateActorButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		RotateActorButtonText->SetJustification(ETextJustify::Center);
		RotateActorButtonText->SetText(FText::FromString(TEXT("Rotate")));
		RotateActorButton->AddChild(RotateActorButtonText);

		DeleteActorButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DesignerDeleteActorButton"));
		DeleteActorButton->SetBackgroundColor(FLinearColor(0.70f, 0.18f, 0.18f, 1.0f));

		UTextBlock* DeleteActorButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DesignerDeleteActorButtonText"));
		DeleteActorButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		DeleteActorButtonText->SetJustification(ETextJustify::Center);
		DeleteActorButtonText->SetText(FText::FromString(TEXT("Delete")));
		DeleteActorButton->AddChild(DeleteActorButtonText);

		if (UHorizontalBoxSlot* MoveButtonSlot = EditActorActionBox->AddChildToHorizontalBox(MoveActorButton))
		{
			MoveButtonSlot->SetPadding(FMargin(0.0f, 0.0f, 6.0f, 0.0f));
			MoveButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		if (UHorizontalBoxSlot* RotateButtonSlot = EditActorActionBox->AddChildToHorizontalBox(RotateActorButton))
		{
			RotateButtonSlot->SetPadding(FMargin(6.0f, 0.0f, 6.0f, 0.0f));
			RotateButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		if (UHorizontalBoxSlot* DeleteButtonSlot = EditActorActionBox->AddChildToHorizontalBox(DeleteActorButton))
		{
			DeleteButtonSlot->SetPadding(FMargin(6.0f, 0.0f, 0.0f, 0.0f));
			DeleteButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		if (UVerticalBoxSlot* EditActionSlot = Cast<UVerticalBoxSlot>(PanelBody->InsertChildAt(ControlPanelInsertIndex, EditActorActionBox)))
		{
			EditActionSlot->SetPadding(FMargin(4.0f, 4.0f, 4.0f, 8.0f));
			EditActionSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
		++ControlPanelInsertIndex;
	}

	UScrollBox* ControlPanelScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("DesignerGeneratedControlScroll"));
	if (UVerticalBoxSlot* ControlPanelScrollSlot = Cast<UVerticalBoxSlot>(PanelBody->InsertChildAt(ControlPanelInsertIndex, ControlPanelScroll)))
	{
		ControlPanelScrollSlot->SetPadding(FMargin(4.0f, 4.0f, 4.0f, 0.0f));
		ControlPanelScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UVerticalBox* ControlPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DesignerGeneratedControlPanel"));
	ControlPanelScroll->AddChild(ControlPanel);

	auto AddLabel = [this, ControlPanel](TObjectPtr<UTextBlock>& OutLabel, const TCHAR* Name, const TCHAR* Text)
	{
		OutLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		OutLabel->SetText(FText::FromString(Text));
		OutLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.90f, 0.90f)));
		if (UVerticalBoxSlot* LabelSlot = ControlPanel->AddChildToVerticalBox(OutLabel))
		{
			LabelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
		}
	};

	auto AddCombo = [this, ControlPanel](TObjectPtr<UWhiteComboBoxString>& OutCombo, const TCHAR* Name)
	{
		OutCombo = WidgetTree->ConstructWidget<UWhiteComboBoxString>(UWhiteComboBoxString::StaticClass(), Name);
		OutCombo->OnGenerateWidgetEvent.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UBuildMenuWidget, GenerateComboItemWidget));
		if (UVerticalBoxSlot* ComboSlot = ControlPanel->AddChildToVerticalBox(OutCombo))
		{
			ComboSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
		}
	};

	auto AddSpinBox = [this, ControlPanel](TObjectPtr<USpinBox>& OutSpinBox, const TCHAR* Name)
	{
		OutSpinBox = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), Name);
		OutSpinBox->SetDelta(1.0f);
		OutSpinBox->SetAlwaysUsesDeltaSnap(true);
		if (UVerticalBoxSlot* SpinSlot = ControlPanel->AddChildToVerticalBox(OutSpinBox))
		{
			SpinSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
		}
	};

	DetailText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DesignerDetailText"));
	DetailText->SetColorAndOpacity(FSlateColor(FLinearColor(0.75f, 0.78f, 0.82f)));
	DetailText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* DetailSlot = ControlPanel->AddChildToVerticalBox(DetailText))
	{
		DetailSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	AddLabel(ModeLabelText, TEXT("DesignerModeLabelText"), TEXT("Truss Mode"));
	AddCombo(ModeComboBox, TEXT("DesignerModeComboBox"));
	ModeComboBox->AddOption(BuildModeToOption(ETrussBuildMode::StraightRun));
	ModeComboBox->AddOption(BuildModeToOption(ETrussBuildMode::Rectangle));
	ModeComboBox->AddOption(BuildModeToOption(ETrussBuildMode::Arch));
	ModeComboBox->AddOption(BuildModeToOption(ETrussBuildMode::Tower));
	ModeComboBox->AddOption(BuildModeToOption(ETrussBuildMode::Cube));
	ModeComboBox->AddOption(BuildModeToOption(ETrussBuildMode::CubeArch));
	ModeComboBox->OnSelectionChanged.AddDynamic(this, &UBuildMenuWidget::HandleModeChanged);

	AddLabel(PrimaryValueLabelText, TEXT("DesignerPrimaryValueLabelText"), TEXT(""));
	AddSpinBox(PrimaryValueSpinBox, TEXT("DesignerPrimaryValueSpinBox"));
	PrimaryValueSpinBox->OnValueChanged.AddDynamic(this, &UBuildMenuWidget::HandlePrimaryValueChanged);

	AddLabel(SecondaryValueLabelText, TEXT("DesignerSecondaryValueLabelText"), TEXT(""));
	AddSpinBox(SecondaryValueSpinBox, TEXT("DesignerSecondaryValueSpinBox"));
	SecondaryValueSpinBox->OnValueChanged.AddDynamic(this, &UBuildMenuWidget::HandleSecondaryValueChanged);

	AddLabel(TertiaryValueLabelText, TEXT("DesignerTertiaryValueLabelText"), TEXT(""));
	AddSpinBox(TertiaryValueSpinBox, TEXT("DesignerTertiaryValueSpinBox"));
	TertiaryValueSpinBox->OnValueChanged.AddDynamic(this, &UBuildMenuWidget::HandleTertiaryValueChanged);

	AddLabel(QuaternaryValueLabelText, TEXT("DesignerQuaternaryValueLabelText"), TEXT(""));
	AddSpinBox(QuaternaryValueSpinBox, TEXT("DesignerQuaternaryValueSpinBox"));
	QuaternaryValueSpinBox->OnValueChanged.AddDynamic(this, &UBuildMenuWidget::HandleQuaternaryValueChanged);

	AddLabel(SidePieceLabelText, TEXT("DesignerSidePieceLabelText"), TEXT("Side Spacer Piece"));
	AddCombo(SidePieceComboBox, TEXT("DesignerSidePieceComboBox"));
	SidePieceComboBox->AddOption(PieceTypeToOption(ETrussPieceType::TwoFoot));
	SidePieceComboBox->AddOption(PieceTypeToOption(ETrussPieceType::FourFoot));
	SidePieceComboBox->AddOption(PieceTypeToOption(ETrussPieceType::FiveFoot));
	SidePieceComboBox->AddOption(PieceTypeToOption(ETrussPieceType::EightFoot));
	SidePieceComboBox->AddOption(PieceTypeToOption(ETrussPieceType::TenFoot));
	SidePieceComboBox->OnSelectionChanged.AddDynamic(this, &UBuildMenuWidget::HandleSidePieceChanged);
	CreateLightingControlPanel(ControlPanel);

	AddLabel(DepthPieceLabelText, TEXT("DesignerDepthPieceLabelText"), TEXT("Depth Spacer Piece"));
	AddCombo(DepthPieceComboBox, TEXT("DesignerDepthPieceComboBox"));
	DepthPieceComboBox->AddOption(PieceTypeToOption(ETrussPieceType::TwoFoot));
	DepthPieceComboBox->AddOption(PieceTypeToOption(ETrussPieceType::FourFoot));
	DepthPieceComboBox->AddOption(PieceTypeToOption(ETrussPieceType::FiveFoot));
	DepthPieceComboBox->AddOption(PieceTypeToOption(ETrussPieceType::EightFoot));
	DepthPieceComboBox->AddOption(PieceTypeToOption(ETrussPieceType::TenFoot));
	DepthPieceComboBox->OnSelectionChanged.AddDynamic(this, &UBuildMenuWidget::HandleDepthPieceChanged);

	AddLabel(MBPRowsLabelText, TEXT("DesignerMBPRowsLabelText"), TEXT("Rows"));
	AddSpinBox(MBPRowsSpinBox, TEXT("DesignerMBPRowsSpinBox"));
	MBPRowsSpinBox->OnValueChanged.AddDynamic(this, &UBuildMenuWidget::HandleMBPRowsChanged);

	AddLabel(MBPColumnsLabelText, TEXT("DesignerMBPColumnsLabelText"), TEXT("Columns"));
	AddSpinBox(MBPColumnsSpinBox, TEXT("DesignerMBPColumnsSpinBox"));
	MBPColumnsSpinBox->OnValueChanged.AddDynamic(this, &UBuildMenuWidget::HandleMBPColumnsChanged);

	AddLabel(MBPStyleLabelText, TEXT("DesignerMBPStyleLabelText"), TEXT("Default Style"));
	AddCombo(MBPStyleComboBox, TEXT("DesignerMBPStyleComboBox"));
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

	AddLabel(StageHeightLabelText, TEXT("DesignerStageHeightLabelText"), TEXT("Deck Height"));
	AddCombo(StageHeightComboBox, TEXT("DesignerStageHeightComboBox"));
	StageHeightComboBox->AddOption(StageHeightPresetToOption(EStageDeckHeightPreset::In8));
	StageHeightComboBox->AddOption(StageHeightPresetToOption(EStageDeckHeightPreset::In12));
	StageHeightComboBox->AddOption(StageHeightPresetToOption(EStageDeckHeightPreset::In24));
	StageHeightComboBox->AddOption(StageHeightPresetToOption(EStageDeckHeightPreset::In27));
	StageHeightComboBox->OnSelectionChanged.AddDynamic(this, &UBuildMenuWidget::HandleStageHeightChanged);

	AddLabel(StageSurfaceLabelText, TEXT("DesignerStageSurfaceLabelText"), TEXT("Surface Style"));
	AddCombo(StageSurfaceComboBox, TEXT("DesignerStageSurfaceComboBox"));
	StageSurfaceComboBox->AddOption(StageSurfaceStyleToOption(EStageDeckSurfaceStyle::BlackTop));
	StageSurfaceComboBox->AddOption(StageSurfaceStyleToOption(EStageDeckSurfaceStyle::GrayCarpet));
	StageSurfaceComboBox->OnSelectionChanged.AddDynamic(this, &UBuildMenuWidget::HandleStageSurfaceChanged);

	auto AddCheckBoxRow = [this, ControlPanel](const TCHAR* LabelName, const TCHAR* CheckBoxName, const TCHAR* LabelText, TObjectPtr<UTextBlock>& OutLabel, TObjectPtr<UCheckBox>& OutCheckBox)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		OutLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), LabelName);
		OutLabel->SetText(FText::FromString(LabelText));
		OutLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.90f, 0.90f)));
		OutCheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), CheckBoxName);

		if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(OutLabel))
		{
			LabelSlot->SetHorizontalAlignment(HAlign_Left);
			LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		if (UHorizontalBoxSlot* CheckSlot = Row->AddChildToHorizontalBox(OutCheckBox))
		{
			CheckSlot->SetHorizontalAlignment(HAlign_Right);
		}
		if (UVerticalBoxSlot* RowSlot = ControlPanel->AddChildToVerticalBox(Row))
		{
			RowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		}
	};

	AddCheckBoxRow(TEXT("DesignerStageFrontRailingLabelText"), TEXT("DesignerStageFrontRailingCheckBox"), TEXT("Front Railing"), StageFrontRailingLabelText, StageFrontRailingCheckBox);
	StageFrontRailingCheckBox->OnCheckStateChanged.AddDynamic(this, &UBuildMenuWidget::HandleStageFrontRailingChanged);
	AddCheckBoxRow(TEXT("DesignerStageBackRailingLabelText"), TEXT("DesignerStageBackRailingCheckBox"), TEXT("Back Railing"), StageBackRailingLabelText, StageBackRailingCheckBox);
	StageBackRailingCheckBox->OnCheckStateChanged.AddDynamic(this, &UBuildMenuWidget::HandleStageBackRailingChanged);
	AddCheckBoxRow(TEXT("DesignerStageLeftRailingLabelText"), TEXT("DesignerStageLeftRailingCheckBox"), TEXT("Left Railing"), StageLeftRailingLabelText, StageLeftRailingCheckBox);
	StageLeftRailingCheckBox->OnCheckStateChanged.AddDynamic(this, &UBuildMenuWidget::HandleStageLeftRailingChanged);
	AddCheckBoxRow(TEXT("DesignerStageRightRailingLabelText"), TEXT("DesignerStageRightRailingCheckBox"), TEXT("Right Railing"), StageRightRailingLabelText, StageRightRailingCheckBox);
	StageRightRailingCheckBox->OnCheckStateChanged.AddDynamic(this, &UBuildMenuWidget::HandleStageRightRailingChanged);
	AddCheckBoxRow(TEXT("DesignerStageLeftStepLabelText"), TEXT("DesignerStageLeftStepCheckBox"), TEXT("Left Step"), StageLeftStepLabelText, StageLeftStepCheckBox);
	StageLeftStepCheckBox->OnCheckStateChanged.AddDynamic(this, &UBuildMenuWidget::HandleStageLeftStepChanged);
	AddCheckBoxRow(TEXT("DesignerStageRightStepLabelText"), TEXT("DesignerStageRightStepCheckBox"), TEXT("Right Step"), StageRightStepLabelText, StageRightStepCheckBox);
	StageRightStepCheckBox->OnCheckStateChanged.AddDynamic(this, &UBuildMenuWidget::HandleStageRightStepChanged);
	AddCheckBoxRow(TEXT("DesignerStageAutomaticSkirtLabelText"), TEXT("DesignerStageAutomaticSkirtCheckBox"), TEXT("Automatic Skirt"), StageAutomaticSkirtLabelText, StageAutomaticSkirtCheckBox);
	StageAutomaticSkirtCheckBox->OnCheckStateChanged.AddDynamic(this, &UBuildMenuWidget::HandleStageAutomaticSkirtChanged);
	AddCheckBoxRow(TEXT("DesignerStageCellEnabledLabelText"), TEXT("DesignerStageCellEnabledCheckBox"), TEXT("Cell Enabled"), StageCellEnabledLabelText, StageCellEnabledCheckBox);
	StageCellEnabledCheckBox->OnCheckStateChanged.AddDynamic(this, &UBuildMenuWidget::HandleStageCellEnabledChanged);
}

void UBuildMenuWidget::BindDesignerWidgetTree()
{
	if (!bUsesDesignerWidgetTree || !WidgetTree)
	{
		return;
	}

	DesignerBuildGridScroll = Cast<UScrollBox>(WidgetTree->FindWidget(TEXT("BuildGridScroll")));
	DesignerBuildGrid = Cast<UUniformGridPanel>(WidgetTree->FindWidget(TEXT("BuildGrid")));
	DesignerCategoryTitleText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("CategoryTitle")));
	DesignerSelectedValueText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("Value_Selected")));
	DesignerRotationSlider = Cast<USlider>(WidgetTree->FindWidget(TEXT("RotationSlider")));
	DesignerConfirmButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("Btn_Confirm")));
	DesignerCancelButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("Btn_Cancel")));
	MoveActorButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("Btn_Move")));
	if (!MoveActorButton)
	{
		MoveActorButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("Btn_MoveActor")));
	}
	if (!MoveActorButton)
	{
		MoveActorButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("Btn_Move_Actor")));
	}
	if (!MoveActorButton)
	{
		MoveActorButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("DesignerMoveActorButton")));
	}
	RotateActorButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("Btn_Rotate")));
	if (!RotateActorButton)
	{
		RotateActorButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("Btn_RotateActor")));
	}
	if (!RotateActorButton)
	{
		RotateActorButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("Btn_Rotate_Actor")));
	}
	if (!RotateActorButton)
	{
		RotateActorButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("DesignerRotateActorButton")));
	}
	DeleteActorButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("Btn_Delete")));
	if (!DeleteActorButton)
	{
		DeleteActorButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("Btn_DeleteActor")));
	}
	if (!DeleteActorButton)
	{
		DeleteActorButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("Btn_Delete_Actor")));
	}
	if (!DeleteActorButton)
	{
		DeleteActorButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("DesignerDeleteActorButton")));
	}
	EditActorActionBox = Cast<UHorizontalBox>(WidgetTree->FindWidget(TEXT("EditActionBar")));
	if (!EditActorActionBox)
	{
		EditActorActionBox = Cast<UHorizontalBox>(WidgetTree->FindWidget(TEXT("DesignerEditActorActionBox")));
	}
	if (DesignerBuildGridScroll)
	{
		if (UVerticalBoxSlot* BuildGridSlot = Cast<UVerticalBoxSlot>(DesignerBuildGridScroll->Slot))
		{
			BuildGridSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
	}
	CreateDesignerControlPanel(Cast<UVerticalBox>(WidgetTree->FindWidget(TEXT("PanelBody"))));

	if (DesignerConfirmButton)
	{
		DesignerConfirmButton->OnClicked.RemoveDynamic(this, &UBuildMenuWidget::HandleActionButtonClicked);
		DesignerConfirmButton->OnClicked.AddDynamic(this, &UBuildMenuWidget::HandleActionButtonClicked);
	}

	if (DesignerCancelButton)
	{
		DesignerCancelButton->OnClicked.RemoveDynamic(this, &UBuildMenuWidget::HandleDesignerCancelClicked);
		DesignerCancelButton->OnClicked.AddDynamic(this, &UBuildMenuWidget::HandleDesignerCancelClicked);
	}

	if (MoveActorButton)
	{
		MoveActorButton->OnClicked.RemoveDynamic(this, &UBuildMenuWidget::HandleMoveActorButtonClicked);
		MoveActorButton->OnClicked.AddDynamic(this, &UBuildMenuWidget::HandleMoveActorButtonClicked);
	}

	if (RotateActorButton)
	{
		RotateActorButton->OnClicked.RemoveDynamic(this, &UBuildMenuWidget::HandleRotateActorButtonClicked);
		RotateActorButton->OnClicked.AddDynamic(this, &UBuildMenuWidget::HandleRotateActorButtonClicked);
	}

	if (DeleteActorButton)
	{
		DeleteActorButton->OnClicked.RemoveDynamic(this, &UBuildMenuWidget::HandleDeleteActorButtonClicked);
		DeleteActorButton->OnClicked.AddDynamic(this, &UBuildMenuWidget::HandleDeleteActorButtonClicked);
	}

	if (DesignerRotationSlider)
	{
		DesignerRotationSlider->OnValueChanged.RemoveDynamic(this, &UBuildMenuWidget::HandleDesignerRotationChanged);
		DesignerRotationSlider->OnValueChanged.AddDynamic(this, &UBuildMenuWidget::HandleDesignerRotationChanged);
	}

	if (UButton* TabButton = FindDesignerTabInnerButton(TEXT("Tab_Truss")))
	{
		SetDesignerTabLabel(TEXT("Tab_Truss"), FText::FromString(TEXT("Truss")));
		TabButton->OnClicked.RemoveDynamic(this, &UBuildMenuWidget::HandleTrussTabClicked);
		TabButton->OnClicked.AddDynamic(this, &UBuildMenuWidget::HandleTrussTabClicked);
	}
	if (UButton* TabButton = FindDesignerTabInnerButton(TEXT("Tab_Stage")))
	{
		SetDesignerTabLabel(TEXT("Tab_Stage"), FText::FromString(TEXT("Stage")));
		TabButton->OnClicked.RemoveDynamic(this, &UBuildMenuWidget::HandleStageTabClicked);
		TabButton->OnClicked.AddDynamic(this, &UBuildMenuWidget::HandleStageTabClicked);
	}
	if (UButton* TabButton = FindDesignerTabInnerButton(TEXT("Tab_AV")))
	{
		SetDesignerTabLabel(TEXT("Tab_AV"), FText::FromString(TEXT("AV")));
		TabButton->OnClicked.RemoveDynamic(this, &UBuildMenuWidget::HandleVideoTabClicked);
		TabButton->OnClicked.AddDynamic(this, &UBuildMenuWidget::HandleVideoTabClicked);
	}
	if (UButton* TabButton = FindDesignerTabInnerButton(TEXT("Tab_Audio")))
	{
		SetDesignerTabLabel(TEXT("Tab_Audio"), FText::FromString(TEXT("Audio")));
		TabButton->OnClicked.RemoveDynamic(this, &UBuildMenuWidget::HandleAudioTabClicked);
		TabButton->OnClicked.AddDynamic(this, &UBuildMenuWidget::HandleAudioTabClicked);
	}
	if (UWidget* AudioTabWidget = WidgetTree->FindWidget(TEXT("Tab_Audio")))
	{
		AudioTabWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (UButton* TabButton = FindDesignerTabInnerButton(TEXT("Tab_Furniture")))
	{
		SetDesignerTabLabel(TEXT("Tab_Furniture"), FText::FromString(TEXT("Furniture")));
		TabButton->OnClicked.RemoveDynamic(this, &UBuildMenuWidget::HandleLoungeTabClicked);
		TabButton->OnClicked.AddDynamic(this, &UBuildMenuWidget::HandleLoungeTabClicked);
	}
}

UButton* UBuildMenuWidget::FindDesignerTabInnerButton(const FName& WidgetName) const
{
	const UUserWidget* TabWidget = WidgetTree ? Cast<UUserWidget>(WidgetTree->FindWidget(WidgetName)) : nullptr;
	return TabWidget && TabWidget->WidgetTree ? Cast<UButton>(TabWidget->WidgetTree->FindWidget(TEXT("TabButton"))) : nullptr;
}

void UBuildMenuWidget::SetDesignerTabLabel(const FName& WidgetName, const FText& LabelText) const
{
	const UUserWidget* TabWidget = WidgetTree ? Cast<UUserWidget>(WidgetTree->FindWidget(WidgetName)) : nullptr;
	if (!TabWidget || !TabWidget->WidgetTree)
	{
		return;
	}

	if (UTextBlock* TabLabel = Cast<UTextBlock>(TabWidget->WidgetTree->FindWidget(TEXT("TabLabel"))))
	{
		TabLabel->SetText(LabelText);
	}
}

void UBuildMenuWidget::RefreshDesignerSummary()
{
	if (!bUsesDesignerWidgetTree)
	{
		return;
	}

	if (DesignerCategoryTitleText)
	{
		if (EditingLightTarget || EditingTarget || IsEditingMBP() || IsEditingStage() || IsEditingDrape() || IsEditingVideo() || IsEditingVideoWall() || IsEditingProjection() || IsEditingAudio() || IsEditingLounge() || IsEditingSectional() || IsEditingFurnitureSeating() || IsEditingBarLayout() || IsEditingDiningTable())
		{
			DesignerCategoryTitleText->SetText(BuildHeaderText());
		}
		else
		{
			const EBuildItemType ItemType = SelectedBuildItem ? SelectedBuildItem->ItemType : ActiveMenuTab;
			DesignerCategoryTitleText->SetText(FText::FromString(BuildDesignerCategoryLabel(ItemType)));
		}
	}

	if (DesignerSelectedValueText)
	{
		if (EditingLightTarget)
		{
			FString FixtureName = EditingLightTarget->FixtureClass.ToSoftObjectPath().GetAssetName();
			FixtureName.RemoveFromStart(TEXT("BP_"));
			FixtureName.RemoveFromEnd(TEXT("_C"));
			DesignerSelectedValueText->SetText(FText::FromString(FString::Printf(TEXT("Lighting > %s"), FixtureName.IsEmpty() ? TEXT("Fixture") : *FixtureName)));
		}
		else if (SelectedBuildItem && SelectedBuildItem->ItemType == EBuildItemType::FurnitureSeating)
		{
			DesignerSelectedValueText->SetText(FText::FromString(FString::Printf(TEXT("Furniture > %s"), *FurnitureSeatingTypeToMenuLabel(CurrentFurnitureSeatingDefinition.SeatingType))));
		}
		else
		{
			DesignerSelectedValueText->SetText(FText::FromString(BuildDesignerSelectionLabel(SelectedBuildItem, ActiveMenuTab)));
		}
	}
}

FText UBuildMenuWidget::BuildDetailText() const
{
	if (EditingLightTarget)
	{
		FString FixtureName = EditingLightTarget->FixtureClass.ToSoftObjectPath().GetAssetName();
		FixtureName.RemoveFromStart(TEXT("BP_"));
		FixtureName.RemoveFromEnd(TEXT("_C"));
		const UDMXEntityFixturePatch* Patch = EditingLightTarget->DMX ? EditingLightTarget->DMX->GetFixturePatch() : nullptr;
		const FString PatchDescription = Patch
			? FString::Printf(TEXT("%s (U%d.%d, %d ch)"), *Patch->Name, Patch->GetUniverseID(), Patch->GetStartingChannel(), Patch->GetChannelSpan())
			: TEXT("Unpatched");
		return FText::FromString(FString::Printf(TEXT("Fixture: %s\nPatch: %s\nTruss: %s"),
			FixtureName.IsEmpty() ? TEXT("None") : *FixtureName,
			*PatchDescription,
			EditingLightTarget->bAttachedToTruss ? TEXT("Attached") : TEXT("Not attached")));
	}

	if (!SelectedBuildItem && ActiveMenuTab != EBuildItemType::MBPWall && ActiveMenuTab != EBuildItemType::DrapeRun && ActiveMenuTab != EBuildItemType::VideoPlacement && ActiveMenuTab != EBuildItemType::VideoWall && ActiveMenuTab != EBuildItemType::ProjectionScreen && ActiveMenuTab != EBuildItemType::AudioPlacement && ActiveMenuTab != EBuildItemType::LoungeLayout && ActiveMenuTab != EBuildItemType::FurnitureSeating && ActiveMenuTab != EBuildItemType::BarLayout && ActiveMenuTab != EBuildItemType::DiningTable)
	{
		return FText::FromString(TEXT("No build item selected."));
	}

	const FText Name = SelectedBuildItem
		? (SelectedBuildItem->ItemType == EBuildItemType::FurnitureSeating
			? FText::FromString(FurnitureSeatingTypeToMenuLabel(CurrentFurnitureSeatingDefinition.SeatingType))
			: (SelectedBuildItem->DisplayName.IsEmpty() ? FText::FromName(SelectedBuildItem->ItemId) : SelectedBuildItem->DisplayName))
		: (ActiveMenuTab == EBuildItemType::AudioPlacement
			? FText::FromString(TEXT("Audio"))
			: (ActiveMenuTab == EBuildItemType::LoungeLayout
			? FText::FromString(TEXT("Lounge"))
			: (ActiveMenuTab == EBuildItemType::DiningTable
			? FText::FromString(TEXT("Dining"))
			: (ActiveMenuTab == EBuildItemType::BarLayout
			? FText::FromString(TEXT("Bars"))
			: (ActiveMenuTab == EBuildItemType::ProjectionScreen
			? FText::FromString(TEXT("Projection"))
			: (ActiveMenuTab == EBuildItemType::VideoWall
				? FText::FromString(TEXT("Video Wall"))
				: (ActiveMenuTab == EBuildItemType::VideoPlacement
					? FText::FromString(TEXT("Video"))
					: (ActiveMenuTab == EBuildItemType::DrapeRun ? FText::FromString(TEXT("Drape")) : FText::FromString(TEXT("MBP Wall"))))))))));

	const TCHAR* TypeLabel = TEXT("MBP Wall");
	if (ActiveMenuTab == EBuildItemType::TrussStructure)
	{
		TypeLabel = TEXT("Truss Structure");
	}
	else if (ActiveMenuTab == EBuildItemType::StageDeck)
	{
		TypeLabel = TEXT("Stage Deck");
	}
	else if (ActiveMenuTab == EBuildItemType::DrapeRun)
	{
		TypeLabel = TEXT("Drape Run");
	}
	else if (ActiveMenuTab == EBuildItemType::VideoPlacement)
	{
		TypeLabel = TEXT("TV Placement");
	}
	else if (ActiveMenuTab == EBuildItemType::VideoWall)
	{
		TypeLabel = TEXT("Video Wall");
	}
	else if (ActiveMenuTab == EBuildItemType::ProjectionScreen)
	{
		TypeLabel = TEXT("Projection Screen");
	}
	else if (ActiveMenuTab == EBuildItemType::AudioPlacement)
	{
		TypeLabel = TEXT("Audio Placement");
	}
	else if (ActiveMenuTab == EBuildItemType::LoungeLayout && (!SelectedBuildItem || SelectedBuildItem->ItemType != EBuildItemType::DiningTable))
	{
		TypeLabel = SelectedBuildItem && SelectedBuildItem->ItemType == EBuildItemType::SectionalLayout ? TEXT("Sectional Layout") : TEXT("Lounge Layout");
	}
	else if (ActiveMenuTab == EBuildItemType::FurnitureSeating)
	{
		TypeLabel = CurrentFurnitureSeatingDefinition.SeatingType == EFurnitureSeatingRuntimeType::CafeTable ? TEXT("Cafe Tables") : TEXT("High Seating");
	}
	else if (ActiveMenuTab == EBuildItemType::BarLayout)
	{
		TypeLabel = TEXT("Bar Layout");
	}
	else if (ActiveMenuTab == EBuildItemType::DiningTable || (SelectedBuildItem && SelectedBuildItem->ItemType == EBuildItemType::DiningTable))
	{
		TypeLabel = TEXT("Dining Table");
	}

	FString Detail = FString::Printf(
		TEXT("Category: %s\nType: %s\nGrid Snap: %.2f cm\nRotation Step: %.1f deg"),
		SelectedBuildItem ? *SelectedBuildItem->Category.ToString() : TEXT("Backdrop"),
		TypeLabel,
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
		case ETrussBuildMode::Tower:
			Detail += FString::Printf(TEXT("\nMode: Tower\nHeight: %.1f ft"), Definition.TowerHeightFt);
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
	else if (ActiveMenuTab == EBuildItemType::StageDeck)
	{
		const FStageDeckBuildDefinition& Definition = CurrentStageDeckDefinition;
		Detail += FString::Printf(
			TEXT("\nRows: %d\nColumns: %d\nHeight: %s\nSurface: %s\nRailings: F:%s B:%s L:%s R:%s\nSteps: Left:%s Right:%s\nSkirt: %s"),
			Definition.Rows,
			Definition.Columns,
			*StageHeightPresetToOption(Definition.DefaultHeightPreset),
			*StageSurfaceStyleToOption(Definition.DefaultSurfaceStyle),
			Definition.bEnableFrontRailing ? TEXT("On") : TEXT("Off"),
			Definition.bEnableBackRailing ? TEXT("On") : TEXT("Off"),
			Definition.bEnableLeftRailing ? TEXT("On") : TEXT("Off"),
			Definition.bEnableRightRailing ? TEXT("On") : TEXT("Off"),
			Definition.bEnableLeftStep ? TEXT("On") : TEXT("Off"),
			Definition.bEnableRightStep ? TEXT("On") : TEXT("Off"),
			Definition.bEnableAutomaticSkirt ? TEXT("On") : TEXT("Off"));

		if (IsEditingStage())
		{
			Detail += FString::Printf(
				TEXT("\nEdit Scope: %s\nTarget Row: %d\nTarget Column: %d\nCell Enabled: %s"),
				*StageEditScopeToOption(CurrentStageEditScope),
				CurrentStageEditTargetRow + 1,
				CurrentStageEditTargetColumn + 1,
				bCurrentStageCellEnabled ? TEXT("Yes") : TEXT("No"));
		}
	}
	else if (ActiveMenuTab == EBuildItemType::DrapeRun)
	{
		const FDrapeRunBuildDefinition& Definition = CurrentDrapeRunDefinition;
		Detail += FString::Printf(
			TEXT("\nLength: %.1f ft\nHeight: %.1f ft"),
			Definition.LengthFt,
			Definition.HeightFt);
	}
	else if (ActiveMenuTab == EBuildItemType::VideoPlacement)
	{
		const FVideoPlacementBuildDefinition& Definition = CurrentVideoPlacementDefinition;
		const EVideoTVSupportMode SupportMode = AVideoPlacementActor::GetSupportModeForTVModel(Definition.TVModel);
		const FString SupportModeText = SupportMode == EVideoTVSupportMode::TrussTower
			? TEXT("Truss Tower")
			: TEXT("Altman Base + Black Pipe");
		Detail += FString::Printf(
			TEXT("\nTV Support: %s\nTV: %s\nTV Size: %d in\nTV Center Height: %.1f ft\nSupport Height: %.1f ft"),
			*SupportModeText,
			*TVModelToOption(Definition.TVModel),
			AVideoPlacementActor::GetTVSizeInches(Definition.TVModel),
			Definition.TVCenterHeightFt,
			Definition.TowerHeightFt);
	}
	else if (ActiveMenuTab == EBuildItemType::VideoWall)
	{
		const FVideoWallBuildDefinition& Definition = CurrentVideoWallDefinition;
		Detail += FString::Printf(
			TEXT("\nRows: %d\nColumns: %d\nSupport: %s\nSupport Spacing: %s"),
			Definition.Rows,
			Definition.Columns,
			*VideoWallSupportModeToOption(Definition.SupportMode),
			*VideoWallSupportSpacingToOption(Definition.SupportSpacing));
	}
	else if (ActiveMenuTab == EBuildItemType::ProjectionScreen)
	{
		const FProjectionScreenBuildDefinition& Definition = CurrentProjectionScreenDefinition;
		Detail += FString::Printf(
			TEXT("\nScreen: %s\nProjector: %s\nLens: %s\nHorizontal Offset: %.1f ft\nHeight From Center: %.1f ft\nCenter Offset: X %.1f cm, Z %.1f cm"),
			*ProjectionScreenSizeToOption(Definition.ScreenKitSize),
			*ProjectionProjectorToOption(Definition.ProjectorType),
			*ProjectionLensToOption(Definition.LensType),
			Definition.ProjectorHorizontalOffsetFt,
			Definition.TowerHeightFt,
			Definition.ScreenCenterOffsetCm.X,
			Definition.ScreenCenterOffsetCm.Z);
	}
	else if (ActiveMenuTab == EBuildItemType::AudioPlacement)
	{
		const bool bLineArray = CurrentAudioPlacementDefinition.PlacementType == EAudioPlacementRuntimeType::GroundLineArray;
		Detail += bLineArray
			? FString::Printf(
				TEXT("\nAudio Type: Ground Line Array\nArray: %s\nSpeakers: %d\nPole Extension: %.1f in\nSub Only: %s\nSound: %s"),
				*AudioLineArrayModelToOption(CurrentAudioPlacementDefinition.GroundLineArrayDefinition.LineArrayModel),
				CurrentAudioPlacementDefinition.GroundLineArrayDefinition.SpeakerCount,
				CurrentAudioPlacementDefinition.GroundLineArrayDefinition.PoleExtensionInches,
				CurrentAudioPlacementDefinition.GroundLineArrayDefinition.bSubOnly ? TEXT("Yes") : TEXT("No"),
				*AudioSourceToOption(CurrentAudioSource))
			: FString::Printf(
				TEXT("\nAudio Type: Ground Speaker\nSpeaker: %s\nStand Extension: %.1f ft\nSound: %s"),
				*AudioSpeakerModelToOption(CurrentAudioPlacementDefinition.GroundSpeakerDefinition.SpeakerModel),
				CurrentAudioPlacementDefinition.GroundSpeakerDefinition.StandExtensionFt,
				*AudioSourceToOption(CurrentAudioSource));
	}
	else if (ActiveMenuTab == EBuildItemType::LoungeLayout && (!SelectedBuildItem || SelectedBuildItem->ItemType != EBuildItemType::DiningTable))
	{
		if (SelectedBuildItem && SelectedBuildItem->ItemType == EBuildItemType::SectionalLayout)
		{
			const FSectionalLayoutBuildDefinition& Definition = CurrentSectionalLayoutDefinition;
			Detail += FString::Printf(
				TEXT("\nArmless: %s\nCorner: %s\nShape: %s\nWidth Sections: %d\nDepth Sections: %d\nSpacing: %.1f cm\nAuto Spacing: %s"),
				Definition.ArmlessItem.IsEmpty() ? TEXT("Auto") : *Definition.ArmlessItem,
				Definition.CornerItem.IsEmpty() ? TEXT("Auto") : *Definition.CornerItem,
				*SectionalShapeToOption(Definition.Shape),
				Definition.WidthSections,
				Definition.DepthSections,
				Definition.SectionSpacingCm,
				Definition.bUseArmlessItemDefaultSpacing ? TEXT("Yes") : TEXT("No"));
		}
		else
		{
			const FLoungeLayoutBuildDefinition& Definition = CurrentLoungeLayoutDefinition;
			Detail += FString::Printf(
				TEXT("\nSofa: %s\nChair: %s\nCocktail Table: %s\nEnd Table: %s\nLamp: %s\nAccent: %s\nSofas: %d\nChairs: %d\nEnd Tables: %d\nLamps: %d\nAccents: %d\nTwo Sofas: %s\nFour Chairs: %s\nEnd Table X Offset: %.1f cm\nMirrored Chair Yaw: %.1f deg"),
				Definition.SofaItem.IsEmpty() ? TEXT("Auto") : *Definition.SofaItem,
				Definition.ChairItem.IsEmpty() ? TEXT("Auto") : *Definition.ChairItem,
				Definition.CocktailTableItem.IsEmpty() ? TEXT("Auto") : *Definition.CocktailTableItem,
				Definition.EndTableItem.IsEmpty() ? TEXT("Auto") : *Definition.EndTableItem,
				Definition.LampItem.IsEmpty() ? TEXT("Auto") : *Definition.LampItem,
				Definition.AccentItem.IsEmpty() ? TEXT("Auto") : *Definition.AccentItem,
				Definition.SofaCount,
				Definition.ChairCount,
				Definition.EndTableCount,
				Definition.LampCount,
				Definition.AccentCount,
				Definition.bUseTwoSofas ? TEXT("Yes") : TEXT("No"),
				Definition.bUseFourChairs ? TEXT("Yes") : TEXT("No"),
				Definition.EndTableOffsetXCm,
				Definition.MirroredChairYawOffsetDegrees);
		}
	}
	else if (ActiveMenuTab == EBuildItemType::FurnitureSeating)
	{
		const bool bCafe = CurrentFurnitureSeatingDefinition.SeatingType == EFurnitureSeatingRuntimeType::CafeTable;
		const FBarSeatingBuildDefinition& BarDefinition = CurrentFurnitureSeatingDefinition.BarSeatingDefinition;
		const FCafeTableBuildDefinition& CafeDefinition = CurrentFurnitureSeatingDefinition.CafeTableDefinition;
		Detail += FString::Printf(
			TEXT("\nSeating Type: %s\nTable: %s\nSeat: %s\nLayout: %s\nSeat Y Offset: %.1f cm\nPair X Offset: %.1f cm\nClassroom Style: %s"),
			*FurnitureSeatingTypeToOption(CurrentFurnitureSeatingDefinition.SeatingType),
			bCafe ? (CafeDefinition.CafeTableItem.IsEmpty() ? TEXT("Auto") : *CafeDefinition.CafeTableItem) : (BarDefinition.BarTableItem.IsEmpty() ? TEXT("Auto") : *BarDefinition.BarTableItem),
			bCafe ? (CafeDefinition.DiningChairItem.IsEmpty() ? TEXT("Auto") : *CafeDefinition.DiningChairItem) : (BarDefinition.StoolItem.IsEmpty() ? TEXT("Auto") : *BarDefinition.StoolItem),
			bCafe ? *CafeTableLayoutToOption(CafeDefinition.ChairLayout) : *BarSeatingLayoutToOption(BarDefinition.StoolLayout),
			bCafe ? CafeDefinition.ChairOffsetYCm : BarDefinition.StoolOffsetYCm,
			bCafe ? CafeDefinition.RectangleChairPairOffsetXCm : BarDefinition.RectangleStoolPairOffsetXCm,
			(bCafe ? CafeDefinition.bClassroomStyle : BarDefinition.bClassroomStyle) ? TEXT("Yes") : TEXT("No"));
	}
	else if (ActiveMenuTab == EBuildItemType::BarLayout)
	{
		const FBarLayoutBuildDefinition& Definition = CurrentBarLayoutDefinition;
		Detail += FString::Printf(
			TEXT("\nShape: %s\nFront: %s\nBack: %s\nUse Front: %s\nUse Back: %s\nWall Count: %d\nWall Spacing: %.1f cm\nCenter Offsets: 2 %.1f cm, 4 %.1f cm, 6 %.1f cm"),
			*BarLayoutShapeToOption(Definition.LayoutShape),
			Definition.FrontBarItem.IsEmpty() ? TEXT("Auto") : *Definition.FrontBarItem,
			Definition.BackBarItem.IsEmpty() ? TEXT("Auto") : *Definition.BackBarItem,
			Definition.bUseFrontBar ? TEXT("Yes") : TEXT("No"),
			Definition.bUseBackBar ? TEXT("Yes") : TEXT("No"),
			Definition.WallBarCount,
			Definition.WallBarSpacingCm,
			Definition.CenterTwoSidedOffsetCm,
			Definition.CenterFourSidedOffsetCm,
			Definition.CenterSixSidedOffsetCm);
	}
	else if (ActiveMenuTab == EBuildItemType::DiningTable || (SelectedBuildItem && SelectedBuildItem->ItemType == EBuildItemType::DiningTable))
	{
		const FDiningTableBuildDefinition& Definition = CurrentDiningTableDefinition;
		Detail += FString::Printf(
			TEXT("\nTable: %s\nChair: %s\nChairs: %s\nArrangement: %s\nGap: %.1f cm\nTablecloth: %s\nLinen: %s\nOverhang: %.1f cm\nDrop: %.1f cm"),
			Definition.DiningTableItem.IsEmpty() ? TEXT("Auto") : *Definition.DiningTableItem,
			Definition.DiningChairItem.IsEmpty() ? TEXT("Auto") : *Definition.DiningChairItem,
			*DiningChairCountToOption(Definition.ChairCount),
			*DiningChairArrangementToOption(Definition.ChairArrangement),
			Definition.ChairGapCm,
			Definition.bUseTablecloth ? TEXT("Yes") : TEXT("No"),
			Definition.TableclothItem.IsEmpty() ? TEXT("Auto") : *Definition.TableclothItem,
			Definition.TableclothOverhangCm,
			Definition.TableclothDropCm);
	}

	return FText::Format(FText::FromString(TEXT("{0}\n\n{1}")), Name, FText::FromString(Detail));
}

FText UBuildMenuWidget::BuildHeaderText() const
{
	if (EditingLightTarget)
	{
		return FText::FromString(TEXT("Edit Lighting"));
	}

	if (EditingTarget)
	{
		return FText::FromString(TEXT("Edit Truss"));
	}

	if (IsEditingMBP())
	{
		return FText::FromString(TEXT("Edit MBP"));
	}

	if (IsEditingStage())
	{
		return FText::FromString(TEXT("Edit Stage"));
	}

	if (IsEditingDrape())
	{
		return FText::FromString(TEXT("Edit Drape"));
	}

	if (IsEditingVideo())
	{
		return FText::FromString(TEXT("Edit TV"));
	}

	if (IsEditingVideoWall())
	{
		return FText::FromString(TEXT("Edit Video Wall"));
	}

	if (IsEditingProjection())
	{
		return FText::FromString(TEXT("Edit Projection"));
	}

	if (IsEditingAudio())
	{
		return FText::FromString(TEXT("Edit Audio"));
	}

	if (IsEditingLounge())
	{
		return FText::FromString(TEXT("Edit Lounge"));
	}

	if (IsEditingSectional())
	{
		return FText::FromString(TEXT("Edit Sectional"));
	}

	if (IsEditingFurnitureSeating())
	{
		return FText::FromString(TEXT("Edit Seating"));
	}

	if (IsEditingBarLayout())
	{
		return FText::FromString(TEXT("Edit Bar"));
	}

	if (IsEditingDiningTable())
	{
		return FText::FromString(TEXT("Edit Dining"));
	}

	return FText::FromString(TEXT("Build Menu"));
}

FText UBuildMenuWidget::BuildActionButtonText() const
{
	return IsEditingAnyActor()
		? FText::FromString(TEXT("Apply"))
		: FText::FromString(TEXT("Create"));
}

bool UBuildMenuWidget::IsEditingMBP() const
{
	return EditingMBPTarget != nullptr;
}

bool UBuildMenuWidget::IsEditingAnyActor() const
{
	return GetEditingActor() != nullptr;
}

void UBuildMenuWidget::RefreshEditActorActionButtons()
{
	const ESlateVisibility EditActionVisibility = IsEditingAnyActor()
		? ESlateVisibility::Visible
		: ESlateVisibility::Collapsed;

	if (EditActorActionBox)
	{
		EditActorActionBox->SetVisibility(EditActionVisibility);
	}
	if (MoveActorButton)
	{
		MoveActorButton->SetVisibility(EditActionVisibility);
	}
	if (RotateActorButton)
	{
		RotateActorButton->SetVisibility(EditActionVisibility);
	}
	if (DeleteActorButton)
	{
		DeleteActorButton->SetVisibility(EditActionVisibility);
	}
}

bool UBuildMenuWidget::IsEditingStage() const
{
	return EditingStageTarget != nullptr;
}

bool UBuildMenuWidget::IsEditingDrape() const
{
	return EditingDrapeTarget != nullptr;
}

bool UBuildMenuWidget::IsEditingVideo() const
{
	return EditingVideoTarget != nullptr;
}

bool UBuildMenuWidget::IsEditingVideoWall() const
{
	return EditingVideoWallTarget != nullptr;
}

bool UBuildMenuWidget::IsEditingProjection() const
{
	return EditingProjectionTarget != nullptr;
}

bool UBuildMenuWidget::IsEditingAudio() const
{
	return EditingAudioGroundSpeakerTarget != nullptr || EditingAudioGroundLineArrayTarget != nullptr;
}

bool UBuildMenuWidget::IsEditingLounge() const
{
	return EditingLoungeTarget != nullptr;
}

bool UBuildMenuWidget::IsEditingSectional() const
{
	return EditingSectionalTarget != nullptr;
}

bool UBuildMenuWidget::IsEditingFurnitureSeating() const
{
	return EditingBarSeatingTarget != nullptr || EditingCafeTableTarget != nullptr;
}

bool UBuildMenuWidget::IsEditingBarLayout() const
{
	return EditingBarLayoutTarget != nullptr;
}

bool UBuildMenuWidget::IsEditingDiningTable() const
{
	return EditingDiningTableTarget != nullptr;
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
			ModeComboBox->ClearOptions();
			ModeComboBox->AddOption(BuildModeToOption(ETrussBuildMode::StraightRun));
			ModeComboBox->AddOption(BuildModeToOption(ETrussBuildMode::Rectangle));
			ModeComboBox->AddOption(BuildModeToOption(ETrussBuildMode::Arch));
			ModeComboBox->AddOption(BuildModeToOption(ETrussBuildMode::Tower));
			ModeComboBox->AddOption(BuildModeToOption(ETrussBuildMode::Cube));
			ModeComboBox->AddOption(BuildModeToOption(ETrussBuildMode::CubeArch));
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
	case ETrussBuildMode::Tower:
		SetNumericControl(PrimaryValueLabelText, PrimaryValueSpinBox, TEXT("Tower Height (ft)"), CurrentTrussDefinition.TowerHeightFt, 2.0f, 100.0f, true);
		SetNumericControl(SecondaryValueLabelText, SecondaryValueSpinBox, TEXT(""), 0.0f, 0.0f, 0.0f, false);
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

void UBuildMenuWidget::RefreshLightingControls()
{
	bRefreshingControls = true;
	for (UWidget* Control : { PrimaryValueLabelText.Get(), SecondaryValueLabelText.Get(), TertiaryValueLabelText.Get(), QuaternaryValueLabelText.Get(), SidePieceLabelText.Get(), DepthPieceLabelText.Get(), MBPRowsLabelText.Get(), MBPColumnsLabelText.Get(), MBPStyleLabelText.Get(), StageHeightLabelText.Get(), StageSurfaceLabelText.Get() })
	{
		if (Control) Control->SetVisibility(ESlateVisibility::Collapsed);
	}
	for (UWidget* Control : { PrimaryValueSpinBox.Get(), SecondaryValueSpinBox.Get(), TertiaryValueSpinBox.Get(), QuaternaryValueSpinBox.Get(), MBPRowsSpinBox.Get(), MBPColumnsSpinBox.Get() })
	{
		if (Control) Control->SetVisibility(ESlateVisibility::Collapsed);
	}
	for (UWidget* Control : { SidePieceComboBox.Get(), DepthPieceComboBox.Get(), MBPStyleComboBox.Get(), StageHeightComboBox.Get(), StageSurfaceComboBox.Get() })
	{
		if (Control) Control->SetVisibility(ESlateVisibility::Collapsed);
	}
	for (UWidget* StageLabel : { StageFrontRailingLabelText.Get(), StageBackRailingLabelText.Get(), StageLeftRailingLabelText.Get(), StageRightRailingLabelText.Get(), StageLeftStepLabelText.Get(), StageRightStepLabelText.Get(), StageAutomaticSkirtLabelText.Get(), StageCellEnabledLabelText.Get() })
	{
		if (StageLabel && StageLabel->GetParent())
		{
			StageLabel->GetParent()->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	for (UWidget* Control : { StageFrontRailingLabelText.Get(), StageBackRailingLabelText.Get(), StageLeftRailingLabelText.Get(), StageRightRailingLabelText.Get(), StageLeftStepLabelText.Get(), StageRightStepLabelText.Get(), StageAutomaticSkirtLabelText.Get(), StageCellEnabledLabelText.Get() })
	{
		if (Control) Control->SetVisibility(ESlateVisibility::Collapsed);
	}
	for (UWidget* Control : { StageFrontRailingCheckBox.Get(), StageBackRailingCheckBox.Get(), StageLeftRailingCheckBox.Get(), StageRightRailingCheckBox.Get(), StageLeftStepCheckBox.Get(), StageRightStepCheckBox.Get(), StageAutomaticSkirtCheckBox.Get(), StageCellEnabledCheckBox.Get() })
	{
		if (Control) Control->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (ModeLabelText)
	{
		ModeLabelText->SetText(FText::FromString(TEXT("Fixture")));
		ModeLabelText->SetVisibility(ESlateVisibility::Visible);
	}
	if (ModeComboBox)
	{
		ModeComboBox->SetVisibility(ESlateVisibility::Visible);
		ModeComboBox->ClearOptions();
		FString SelectedName;
		const UClass* CurrentClass = EditingLightTarget ? EditingLightTarget->FixtureClass.LoadSynchronous() : nullptr;
		for (int32 Index = 0; Index < LightingFixtureClasses.Num() && Index < LightingFixtureNames.Num(); ++Index)
		{
			ModeComboBox->AddOption(LightingFixtureNames[Index]);
			if (LightingFixtureClasses[Index].Get() == CurrentClass)
			{
				SelectedName = LightingFixtureNames[Index];
			}
		}
		if (SelectedName.IsEmpty())
		{
			SelectedName = CurrentClass ? CurrentClass->GetName() : TEXT("No Fixture");
			ModeComboBox->AddOption(SelectedName);
		}
		ModeComboBox->SetSelectedOption(SelectedName);
	}

	UDMXEntityFixturePatch* CurrentPatch = EditingLightTarget && EditingLightTarget->DMX
		? EditingLightTarget->DMX->GetFixturePatch() : nullptr;
	FString SelectedPatchOption = TEXT("Unpatched");
	const TArray<TPair<FString, UDMXEntityFixturePatch*>> PatchOptions = GetLightingFixturePatchOptions();
	for (const TPair<FString, UDMXEntityFixturePatch*>& PatchOption : PatchOptions)
	{
		if (PatchOption.Value == CurrentPatch)
		{
			SelectedPatchOption = PatchOption.Key;
			break;
		}
	}

	if (SidePieceLabelText)
	{
		SidePieceLabelText->SetText(FText::FromString(TEXT("Fixture Patch")));
		SidePieceLabelText->SetVisibility(ESlateVisibility::Visible);
	}
	if (SidePieceComboBox)
	{
		SidePieceComboBox->SetVisibility(ESlateVisibility::Visible);
		SidePieceComboBox->ClearOptions();
		for (const TPair<FString, UDMXEntityFixturePatch*>& PatchOption : PatchOptions)
		{
			SidePieceComboBox->AddOption(PatchOption.Key);
		}
		SidePieceComboBox->SetSelectedOption(SelectedPatchOption);
	}

	const TArray<const FDMXFixtureFunction*> ControlFunctions = GetLightingControlFunctions(CurrentPatch);
	TMap<FDMXAttributeName, int32> CurrentValues;
	if (CurrentPatch)
	{
		CurrentPatch->GetAttributeValues(CurrentValues);
	}
	if (UTextBlock* Status = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("LightingControlStatus"))))
	{
		Status->SetText(FText::FromString(CurrentPatch ? TEXT("No intensity, position, or color controls in this patch mode.") : TEXT("Choose a fixture patch to control this light.")));
		Status->SetVisibility(ControlFunctions.IsEmpty() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	for (int32 Index = 0; Index < 7; ++Index)
	{
		UWidget* Row = WidgetTree->FindWidget(LightingControlWidgetName(TEXT("LightingControlRow"), Index));
		if (!Row)
		{
			continue;
		}
		Row->SetVisibility(ControlFunctions.IsValidIndex(Index) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (!ControlFunctions.IsValidIndex(Index))
		{
			continue;
		}

		const FDMXFixtureFunction& Function = *ControlFunctions[Index];
		const int64 MaxValue = (int64(1) << (8 * Function.GetNumChannels())) - 1;
		const int32* ReceivedValue = CurrentValues.Find(Function.Attribute);
		const float NormalizedValue = static_cast<float>(FMath::Clamp<int64>(ReceivedValue ? *ReceivedValue : Function.DefaultValue, 0, MaxValue)) / static_cast<float>(MaxValue);
		if (UTextBlock* Label = Cast<UTextBlock>(WidgetTree->FindWidget(LightingControlWidgetName(TEXT("LightingControlLabel"), Index))))
		{
			Label->SetText(FText::FromName(Function.Attribute.Name));
		}
		if (UTextBlock* Value = Cast<UTextBlock>(WidgetTree->FindWidget(LightingControlWidgetName(TEXT("LightingControlValue"), Index))))
		{
			Value->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(NormalizedValue * 100.0f))));
		}
		if (USlider* Slider = Cast<USlider>(WidgetTree->FindWidget(LightingControlWidgetName(TEXT("LightingControlSlider"), Index))))
		{
			Slider->SetValue(NormalizedValue);
		}
	}
	UpdateLightingColorSwatch();

	if (UWidget* GoboPanel = WidgetTree->FindWidget(TEXT("LightingGoboPanel")))
	{
		const bool bShowGobos = EditingLightTarget && EditingLightTarget->IsLekoFixture();
		GoboPanel->SetVisibility(bShowGobos ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		GoboTexturePathsByOption.Reset();
		if (bShowGobos)
		{
			if (UComboBoxString* ComboBox = Cast<UComboBoxString>(WidgetTree->FindWidget(TEXT("LightingGoboComboBox"))))
			{
				ComboBox->ClearOptions();
				ComboBox->AddOption(TEXT("Open"));
				FString SelectedOption = TEXT("Open");
				const FSoftObjectPath SelectedPath = EditingLightTarget->GoboTexture.ToSoftObjectPath();

				FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
				TArray<FAssetData> GoboAssets;
				AssetRegistry.Get().GetAssetsByPath(FName(MajicGoboAssetPath), GoboAssets, false);
				GoboAssets.Sort([](const FAssetData& A, const FAssetData& B) { return A.AssetName.LexicalLess(B.AssetName); });
				for (const FAssetData& Asset : GoboAssets)
				{
					if (Asset.AssetClassPath != UTexture2D::StaticClass()->GetClassPathName())
					{
						continue;
					}
					const FString Name = Asset.AssetName.ToString();
					const FSoftObjectPath Path = Asset.GetSoftObjectPath();
					GoboTexturePathsByOption.Add(Name, Path);
					ComboBox->AddOption(Name);
					if (Path == SelectedPath)
					{
						SelectedOption = Name;
					}
				}

				FSoftObjectPath CustomPath = EditingLightTarget->CustomGoboTexture.ToSoftObjectPath();
				if (CustomPath.IsNull() && !SelectedPath.IsNull() && SelectedOption == TEXT("Open"))
				{
					CustomPath = SelectedPath;
				}
				if (!CustomPath.IsNull())
				{
					GoboTexturePathsByOption.Add(TEXT("Custom"), CustomPath);
					ComboBox->AddOption(TEXT("Custom"));
					if (CustomPath == SelectedPath)
					{
						SelectedOption = TEXT("Custom");
					}
				}
				ComboBox->SetSelectedOption(SelectedOption);
				RefreshGoboPreview(SelectedOption);
			}
		}
	}
	bRefreshingControls = false;
}

void UBuildMenuWidget::ApplyLightingControlValue(int32 SlotIndex, float NormalizedValue)
{
	UDMXEntityFixturePatch* Patch = EditingLightTarget && EditingLightTarget->DMX
		? EditingLightTarget->DMX->GetFixturePatch() : nullptr;
	const TArray<const FDMXFixtureFunction*> ControlFunctions = GetLightingControlFunctions(Patch);
	if (!ControlFunctions.IsValidIndex(SlotIndex))
	{
		return;
	}

	const FDMXFixtureFunction& Function = *ControlFunctions[SlotIndex];
	const int64 MaxValue = (int64(1) << (8 * Function.GetNumChannels())) - 1;
	const int32 RawValue = static_cast<int32>(FMath::RoundToInt64(FMath::Clamp(NormalizedValue, 0.0f, 1.0f) * static_cast<double>(MaxValue)));
	TMap<FDMXAttributeName, int32> Values;
	Values.Add(Function.Attribute, RawValue);
	Patch->SendDMX(Values);

	if (UTextBlock* Value = Cast<UTextBlock>(WidgetTree->FindWidget(LightingControlWidgetName(TEXT("LightingControlValue"), SlotIndex))))
	{
		Value->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(FMath::Clamp(NormalizedValue, 0.0f, 1.0f) * 100.0f))));
	}
	UpdateLightingColorSwatch();
}

void UBuildMenuWidget::UpdateLightingColorSwatch() const
{
	UBorder* Swatch = WidgetTree ? Cast<UBorder>(WidgetTree->FindWidget(TEXT("LightingColorSwatch"))) : nullptr;
	if (!Swatch)
	{
		return;
	}

	const UDMXEntityFixturePatch* Patch = EditingLightTarget && EditingLightTarget->DMX
		? EditingLightTarget->DMX->GetFixturePatch() : nullptr;
	const TArray<const FDMXFixtureFunction*> ControlFunctions = GetLightingControlFunctions(Patch);
	float Red = -1.0f, Green = -1.0f, Blue = -1.0f;
	float Cyan = -1.0f, Magenta = -1.0f, Yellow = -1.0f;
	for (int32 Index = 0; Index < ControlFunctions.Num(); ++Index)
	{
		const USlider* Slider = Cast<USlider>(WidgetTree->FindWidget(LightingControlWidgetName(TEXT("LightingControlSlider"), Index)));
		if (!Slider)
		{
			continue;
		}
		const FName Attribute = ControlFunctions[Index]->Attribute.Name;
		if (Attribute == TEXT("Red")) Red = Slider->GetValue();
		else if (Attribute == TEXT("Green")) Green = Slider->GetValue();
		else if (Attribute == TEXT("Blue")) Blue = Slider->GetValue();
		else if (Attribute == TEXT("Cyan")) Cyan = Slider->GetValue();
		else if (Attribute == TEXT("Magenta")) Magenta = Slider->GetValue();
		else if (Attribute == TEXT("Yellow")) Yellow = Slider->GetValue();
	}

	if (Red >= 0.0f && Green >= 0.0f && Blue >= 0.0f)
	{
		Swatch->SetBrushColor(FLinearColor(Red, Green, Blue));
		Swatch->SetVisibility(ESlateVisibility::Visible);
	}
	else if (Cyan >= 0.0f && Magenta >= 0.0f && Yellow >= 0.0f)
	{
		Swatch->SetBrushColor(FLinearColor(1.0f - Cyan, 1.0f - Magenta, 1.0f - Yellow));
		Swatch->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		Swatch->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UBuildMenuWidget::RefreshGoboPreview(const FString& Option) const
{
	UImage* Preview = WidgetTree ? Cast<UImage>(WidgetTree->FindWidget(TEXT("LightingGoboPreview"))) : nullptr;
	if (!Preview)
	{
		return;
	}
	const FSoftObjectPath* Path = GoboTexturePathsByOption.Find(Option);
	UTexture2D* Texture = Path ? Cast<UTexture2D>(Path->TryLoad()) : nullptr;
	Preview->SetVisibility(Texture ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (Texture)
	{
		Preview->SetBrushFromTexture(Texture);
	}
}

UWidget* UBuildMenuWidget::CreateGoboOptionWidget(FString Item)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (const FSoftObjectPath* Path = GoboTexturePathsByOption.Find(Item))
	{
		if (UTexture2D* Texture = Cast<UTexture2D>(Path->TryLoad()))
		{
			UImage* Thumbnail = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			Thumbnail->SetBrushFromTexture(Texture);
			Thumbnail->SetDesiredSizeOverride(FVector2D(44.0f, 44.0f));
			if (UHorizontalBoxSlot* ImageSlot = Row->AddChildToHorizontalBox(Thumbnail))
			{
				ImageSlot->SetPadding(FMargin(4.0f));
			}
		}
	}
	UTextBlock* Name = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Name->SetText(FText::FromString(Item));
	Name->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	if (UHorizontalBoxSlot* NameSlot = Row->AddChildToHorizontalBox(Name))
	{
		NameSlot->SetPadding(FMargin(4.0f, 10.0f, 4.0f, 4.0f));
	}
	return Row;
}

void UBuildMenuWidget::HandleGoboChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (bRefreshingControls || !EditingLightTarget || !EditingLightTarget->IsLekoFixture())
	{
		return;
	}
	const FSoftObjectPath* Path = GoboTexturePathsByOption.Find(SelectedItem);
	EditingLightTarget->SetGoboTexture(Path ? Cast<UTexture2D>(Path->TryLoad()) : nullptr);
	RefreshGoboPreview(SelectedItem);
}

void UBuildMenuWidget::RefreshTabButtons()
{
	auto GetButtonColor = [this](EBuildItemType TabType)
	{
		if (EditingLightTarget)
		{
			return FLinearColor(0.10f, 0.10f, 0.12f, 1.0f);
		}
		const EBuildItemType VisibleTab = bUsesDesignerWidgetTree
			? BuildDesignerTabForItemType(ActiveMenuTab)
			: ActiveMenuTab;
		return VisibleTab == TabType
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

	if (StageTabButton)
	{
		StageTabButton->SetBackgroundColor(GetButtonColor(EBuildItemType::StageDeck));
	}

	if (DrapeTabButton)
	{
		DrapeTabButton->SetBackgroundColor(GetButtonColor(EBuildItemType::DrapeRun));
	}

	if (VideoTabButton)
	{
		VideoTabButton->SetBackgroundColor(GetButtonColor(EBuildItemType::VideoPlacement));
	}

	if (VideoWallTabButton)
	{
		VideoWallTabButton->SetBackgroundColor(GetButtonColor(EBuildItemType::VideoWall));
	}

	if (ProjectionTabButton)
	{
		ProjectionTabButton->SetBackgroundColor(GetButtonColor(EBuildItemType::ProjectionScreen));
	}

	if (AudioTabButton)
	{
		AudioTabButton->SetBackgroundColor(GetButtonColor(EBuildItemType::AudioPlacement));
	}

	if (LoungeTabButton)
	{
		LoungeTabButton->SetBackgroundColor(GetButtonColor(EBuildItemType::LoungeLayout));
	}

	if (SeatingTabButton)
	{
		SeatingTabButton->SetBackgroundColor(GetButtonColor(EBuildItemType::FurnitureSeating));
	}

	if (BarTabButton)
	{
		BarTabButton->SetBackgroundColor(GetButtonColor(EBuildItemType::BarLayout));
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
			ModeComboBox->AddOption(MBPEditScopeToOption(EMBPRuntimeEditScope::WholeWall));
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
		const bool bWholeWall = CurrentMBPEditScope == EMBPRuntimeEditScope::WholeWall;
		SetEditNumericControl(PrimaryValueLabelText, PrimaryValueSpinBox, bWholeWall ? TEXT("Rows") : TEXT("Target Row"), bWholeWall ? CurrentMBPWallDefinition.Rows : CurrentMBPEditTargetRow + 1, 1.0f, bWholeWall ? 100.0f : FMath::Max(CurrentMBPWallDefinition.Rows, 1), true);
		SetEditNumericControl(SecondaryValueLabelText, SecondaryValueSpinBox, bWholeWall ? TEXT("Columns") : TEXT("Target Column"), bWholeWall ? CurrentMBPWallDefinition.Columns : CurrentMBPEditTargetColumn + 1, 1.0f, bWholeWall ? 100.0f : FMath::Max(CurrentMBPWallDefinition.Columns, 1), bWholeWall || CurrentMBPEditScope == EMBPRuntimeEditScope::Panel);
		SetEditNumericControl(TertiaryValueLabelText, TertiaryValueSpinBox, TEXT("Depth Offset (cm)"), CurrentMBPEditDepthOffsetCm, -304.8f, 304.8f, !bWholeWall);
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

void UBuildMenuWidget::RefreshStageControls()
{
	bRefreshingControls = true;

	const bool bIsStageItem = ActiveMenuTab == EBuildItemType::StageDeck;
	const bool bEditingStage = IsEditingStage();
	const bool bEditingWholeStage = bEditingStage && CurrentStageEditScope == EStageRuntimeEditScope::WholeStage;
	const bool bEditingCell = bEditingStage && CurrentStageEditScope == EStageRuntimeEditScope::Cell;
	const ESlateVisibility VisibleState = bIsStageItem ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;

	if (ModeLabelText)
	{
		ModeLabelText->SetVisibility(bEditingStage ? VisibleState : ESlateVisibility::Collapsed);
		if (bEditingStage)
		{
			ModeLabelText->SetText(FText::FromString(TEXT("Edit Scope")));
		}
	}

	if (ModeComboBox)
	{
		ModeComboBox->SetVisibility(bEditingStage ? VisibleState : ESlateVisibility::Collapsed);
		if (bEditingStage)
		{
			ModeComboBox->ClearOptions();
			ModeComboBox->AddOption(StageEditScopeToOption(EStageRuntimeEditScope::WholeStage));
			ModeComboBox->AddOption(StageEditScopeToOption(EStageRuntimeEditScope::Cell));
			ModeComboBox->SetSelectedOption(StageEditScopeToOption(CurrentStageEditScope));
		}
	}

	auto SetNumericControl = [VisibleState](UTextBlock* Label, USpinBox* SpinBox, const TCHAR* LabelText, float Value, float MinValue, float MaxValue, bool bShow)
	{
		const ESlateVisibility ControlVisibility = bShow && VisibleState == ESlateVisibility::Visible
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed;

		if (Label)
		{
			Label->SetText(FText::FromString(LabelText));
			Label->SetVisibility(ControlVisibility);
		}

		if (SpinBox)
		{
			SpinBox->SetVisibility(ControlVisibility);
			SpinBox->SetMinValue(MinValue);
			SpinBox->SetMaxValue(MaxValue);
			SpinBox->SetMinSliderValue(MinValue);
			SpinBox->SetMaxSliderValue(MaxValue);
			if (ControlVisibility == ESlateVisibility::Visible)
			{
				SpinBox->SetValue(Value);
			}
		}
	};

	if (bEditingCell)
	{
		SetNumericControl(PrimaryValueLabelText, PrimaryValueSpinBox, TEXT("Target Row"), CurrentStageEditTargetRow + 1, 1.0f, FMath::Max(CurrentStageDeckDefinition.Rows, 1), true);
		SetNumericControl(SecondaryValueLabelText, SecondaryValueSpinBox, TEXT("Target Column"), CurrentStageEditTargetColumn + 1, 1.0f, FMath::Max(CurrentStageDeckDefinition.Columns, 1), true);
	}
	else
	{
		SetNumericControl(PrimaryValueLabelText, PrimaryValueSpinBox, TEXT("Columns"), CurrentStageDeckDefinition.Columns, 1.0f, 24.0f, bIsStageItem);
		SetNumericControl(SecondaryValueLabelText, SecondaryValueSpinBox, TEXT("Rows"), CurrentStageDeckDefinition.Rows, 1.0f, 24.0f, bIsStageItem);
	}
	SetNumericControl(TertiaryValueLabelText, TertiaryValueSpinBox, TEXT(""), 0.0f, 0.0f, 0.0f, false);
	SetNumericControl(QuaternaryValueLabelText, QuaternaryValueSpinBox, TEXT(""), 0.0f, 0.0f, 0.0f, false);

	if (SidePieceLabelText) SidePieceLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (SidePieceComboBox) SidePieceComboBox->SetVisibility(ESlateVisibility::Collapsed);
	if (DepthPieceLabelText) DepthPieceLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (DepthPieceComboBox) DepthPieceComboBox->SetVisibility(ESlateVisibility::Collapsed);

	auto SetTextVisibility = [VisibleState](UTextBlock* Label)
	{
		if (Label)
		{
			Label->SetVisibility(VisibleState);
		}
	};

	auto SetCheckBoxValue = [VisibleState](UCheckBox* CheckBox, bool bValue)
	{
		if (CheckBox)
		{
			CheckBox->SetVisibility(VisibleState);
			if (VisibleState == ESlateVisibility::Visible)
			{
				CheckBox->SetIsChecked(bValue);
			}
		}
	};

	SetTextVisibility(StageHeightLabelText);
	if (StageHeightComboBox)
	{
		StageHeightComboBox->SetVisibility(VisibleState);
		if (bIsStageItem)
		{
			StageHeightComboBox->SetSelectedOption(StageHeightPresetToOption(
				bEditingCell ? CurrentStageCellHeightPreset : CurrentStageDeckDefinition.DefaultHeightPreset));
		}
	}

	SetTextVisibility(StageSurfaceLabelText);
	if (StageSurfaceComboBox)
	{
		StageSurfaceComboBox->SetVisibility(VisibleState);
		if (bIsStageItem)
		{
			StageSurfaceComboBox->SetSelectedOption(StageSurfaceStyleToOption(
				bEditingCell ? CurrentStageCellSurfaceStyle : CurrentStageDeckDefinition.DefaultSurfaceStyle));
		}
	}

	const ESlateVisibility WholeStageVisibility = (bIsStageItem && !bEditingCell) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	auto SetWholeStageLabelVisibility = [WholeStageVisibility](UTextBlock* Label)
	{
		if (Label)
		{
			Label->SetVisibility(WholeStageVisibility);
		}
	};
	auto SetWholeStageCheckBoxValue = [WholeStageVisibility](UCheckBox* CheckBox, bool bValue)
	{
		if (CheckBox)
		{
			CheckBox->SetVisibility(WholeStageVisibility);
			if (WholeStageVisibility == ESlateVisibility::Visible)
			{
				CheckBox->SetIsChecked(bValue);
			}
		}
	};

	SetWholeStageLabelVisibility(StageFrontRailingLabelText);
	SetWholeStageCheckBoxValue(StageFrontRailingCheckBox, CurrentStageDeckDefinition.bEnableFrontRailing);
	SetWholeStageLabelVisibility(StageBackRailingLabelText);
	SetWholeStageCheckBoxValue(StageBackRailingCheckBox, CurrentStageDeckDefinition.bEnableBackRailing);
	SetWholeStageLabelVisibility(StageLeftRailingLabelText);
	SetWholeStageCheckBoxValue(StageLeftRailingCheckBox, CurrentStageDeckDefinition.bEnableLeftRailing);
	SetWholeStageLabelVisibility(StageRightRailingLabelText);
	SetWholeStageCheckBoxValue(StageRightRailingCheckBox, CurrentStageDeckDefinition.bEnableRightRailing);
	SetWholeStageLabelVisibility(StageLeftStepLabelText);
	SetWholeStageCheckBoxValue(StageLeftStepCheckBox, CurrentStageDeckDefinition.bEnableLeftStep);
	SetWholeStageLabelVisibility(StageRightStepLabelText);
	SetWholeStageCheckBoxValue(StageRightStepCheckBox, CurrentStageDeckDefinition.bEnableRightStep);
	SetWholeStageLabelVisibility(StageAutomaticSkirtLabelText);
	SetWholeStageCheckBoxValue(StageAutomaticSkirtCheckBox, CurrentStageDeckDefinition.bEnableAutomaticSkirt);

	SetTextVisibility(StageCellEnabledLabelText);
	if (StageCellEnabledLabelText)
	{
		StageCellEnabledLabelText->SetVisibility(bEditingCell ? VisibleState : ESlateVisibility::Collapsed);
	}
	if (StageCellEnabledCheckBox)
	{
		StageCellEnabledCheckBox->SetVisibility(bEditingCell ? VisibleState : ESlateVisibility::Collapsed);
		if (bEditingCell)
		{
			StageCellEnabledCheckBox->SetIsChecked(bCurrentStageCellEnabled);
		}
	}

	bRefreshingControls = false;
}

void UBuildMenuWidget::RefreshDrapeControls()
{
	if (ActiveMenuTab != EBuildItemType::DrapeRun)
	{
		return;
	}

	bRefreshingControls = true;

	if (ModeLabelText) ModeLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (ModeComboBox) ModeComboBox->SetVisibility(ESlateVisibility::Collapsed);

	auto SetNumericControl = [](UTextBlock* Label, USpinBox* SpinBox, const TCHAR* LabelText, float Value, float MinValue, float MaxValue, float SliderMax)
	{
		if (Label)
		{
			Label->SetText(FText::FromString(LabelText));
			Label->SetVisibility(ESlateVisibility::Visible);
		}

		if (SpinBox)
		{
			SpinBox->SetVisibility(ESlateVisibility::Visible);
			SpinBox->SetMinValue(MinValue);
			SpinBox->SetMaxValue(MaxValue);
			SpinBox->SetMinSliderValue(MinValue);
			SpinBox->SetMaxSliderValue(SliderMax);
			SpinBox->SetValue(Value);
		}
	};

	SetNumericControl(PrimaryValueLabelText, PrimaryValueSpinBox, TEXT("Length (ft)"), CurrentDrapeRunDefinition.LengthFt, 1.0f, 1000.0f, 120.0f);
	SetNumericControl(SecondaryValueLabelText, SecondaryValueSpinBox, TEXT("Height (ft)"), CurrentDrapeRunDefinition.HeightFt, 1.0f, 24.0f, 20.0f);

	if (TertiaryValueLabelText) TertiaryValueLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (TertiaryValueSpinBox) TertiaryValueSpinBox->SetVisibility(ESlateVisibility::Collapsed);
	if (QuaternaryValueLabelText) QuaternaryValueLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (QuaternaryValueSpinBox) QuaternaryValueSpinBox->SetVisibility(ESlateVisibility::Collapsed);
	if (SidePieceLabelText) SidePieceLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (SidePieceComboBox) SidePieceComboBox->SetVisibility(ESlateVisibility::Collapsed);
	if (DepthPieceLabelText) DepthPieceLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (DepthPieceComboBox) DepthPieceComboBox->SetVisibility(ESlateVisibility::Collapsed);

	bRefreshingControls = false;
}

void UBuildMenuWidget::RefreshVideoControls()
{
	if (ActiveMenuTab != EBuildItemType::VideoPlacement)
	{
		return;
	}

	bRefreshingControls = true;

	if (ModeLabelText)
	{
		ModeLabelText->SetText(FText::FromString(TEXT("TV Model")));
		ModeLabelText->SetVisibility(ESlateVisibility::Visible);
	}

	if (ModeComboBox)
	{
		ModeComboBox->SetVisibility(ESlateVisibility::Visible);
		ModeComboBox->ClearOptions();
		for (EVideoTVModel TVModel : {
			EVideoTVModel::Hisense58,
			EVideoTVModel::Insignia43,
			EVideoTVModel::Philips46,
			EVideoTVModel::Samsung22,
			EVideoTVModel::Samsung55Outdoor,
			EVideoTVModel::Samsung58,
			EVideoTVModel::Samsung60,
			EVideoTVModel::Samsung82Crystal,
			EVideoTVModel::Samsung82Smart,
			EVideoTVModel::Sharp55,
			EVideoTVModel::Sharp60,
			EVideoTVModel::Sharp80,
			EVideoTVModel::Sharp90,
			EVideoTVModel::Vizio70,
			EVideoTVModel::Benq25Preview})
		{
			ModeComboBox->AddOption(TVModelToOption(TVModel));
		}
		ModeComboBox->SetSelectedOption(TVModelToOption(CurrentVideoPlacementDefinition.TVModel));
	}

	auto SetNumericControl = [](UTextBlock* Label, USpinBox* SpinBox, const TCHAR* LabelText, float Value, float MinValue, float MaxValue, float SliderMax)
	{
		if (Label)
		{
			Label->SetText(FText::FromString(LabelText));
			Label->SetVisibility(ESlateVisibility::Visible);
		}

		if (SpinBox)
		{
			SpinBox->SetVisibility(ESlateVisibility::Visible);
			SpinBox->SetMinValue(MinValue);
			SpinBox->SetMaxValue(MaxValue);
			SpinBox->SetMinSliderValue(MinValue);
			SpinBox->SetMaxSliderValue(SliderMax);
			SpinBox->SetValue(Value);
		}
	};

	SetNumericControl(PrimaryValueLabelText, PrimaryValueSpinBox, TEXT("TV Center Height (ft)"), CurrentVideoPlacementDefinition.TVCenterHeightFt, 1.0f, 30.0f, 16.0f);
	SetNumericControl(SecondaryValueLabelText, SecondaryValueSpinBox, TEXT("Support Height (ft)"), CurrentVideoPlacementDefinition.TowerHeightFt, 2.0f, 30.0f, 20.0f);

	if (TertiaryValueLabelText) TertiaryValueLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (TertiaryValueSpinBox) TertiaryValueSpinBox->SetVisibility(ESlateVisibility::Collapsed);
	if (QuaternaryValueLabelText) QuaternaryValueLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (QuaternaryValueSpinBox) QuaternaryValueSpinBox->SetVisibility(ESlateVisibility::Collapsed);
	if (SidePieceLabelText) SidePieceLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (SidePieceComboBox) SidePieceComboBox->SetVisibility(ESlateVisibility::Collapsed);
	if (DepthPieceLabelText) DepthPieceLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (DepthPieceComboBox) DepthPieceComboBox->SetVisibility(ESlateVisibility::Collapsed);

	bRefreshingControls = false;
}

void UBuildMenuWidget::RefreshVideoWallControls()
{
	if (ActiveMenuTab != EBuildItemType::VideoWall)
	{
		return;
	}

	bRefreshingControls = true;

	if (ModeLabelText)
	{
		ModeLabelText->SetText(FText::FromString(TEXT("Support")));
		ModeLabelText->SetVisibility(ESlateVisibility::Visible);
	}

	if (ModeComboBox)
	{
		ModeComboBox->SetVisibility(ESlateVisibility::Visible);
		ModeComboBox->ClearOptions();
		ModeComboBox->AddOption(VideoWallSupportModeToOption(EVideoWallSupportMode::GroundStacked));
		ModeComboBox->AddOption(VideoWallSupportModeToOption(EVideoWallSupportMode::Flown));
		ModeComboBox->SetSelectedOption(VideoWallSupportModeToOption(CurrentVideoWallDefinition.SupportMode));
	}

	auto SetNumericControl = [](UTextBlock* Label, USpinBox* SpinBox, const TCHAR* LabelText, float Value, float MinValue, float MaxValue, float SliderMax)
	{
		if (Label)
		{
			Label->SetText(FText::FromString(LabelText));
			Label->SetVisibility(ESlateVisibility::Visible);
		}

		if (SpinBox)
		{
			SpinBox->SetVisibility(ESlateVisibility::Visible);
			SpinBox->SetMinValue(MinValue);
			SpinBox->SetMaxValue(MaxValue);
			SpinBox->SetMinSliderValue(MinValue);
			SpinBox->SetMaxSliderValue(SliderMax);
			SpinBox->SetValue(Value);
		}
	};

	SetNumericControl(PrimaryValueLabelText, PrimaryValueSpinBox, TEXT("Rows"), CurrentVideoWallDefinition.Rows, 1.0f, 100.0f, 20.0f);
	SetNumericControl(SecondaryValueLabelText, SecondaryValueSpinBox, TEXT("Columns"), CurrentVideoWallDefinition.Columns, 1.0f, 100.0f, 20.0f);

	if (TertiaryValueLabelText) TertiaryValueLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (TertiaryValueSpinBox) TertiaryValueSpinBox->SetVisibility(ESlateVisibility::Collapsed);
	if (QuaternaryValueLabelText) QuaternaryValueLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (QuaternaryValueSpinBox) QuaternaryValueSpinBox->SetVisibility(ESlateVisibility::Collapsed);

	if (SidePieceLabelText)
	{
		SidePieceLabelText->SetText(FText::FromString(TEXT("Support Spacing")));
		SidePieceLabelText->SetVisibility(ESlateVisibility::Visible);
	}

	if (SidePieceComboBox)
	{
		SidePieceComboBox->SetVisibility(ESlateVisibility::Visible);
		SidePieceComboBox->ClearOptions();
		SidePieceComboBox->AddOption(VideoWallSupportSpacingToOption(EVideoWallSupportSpacing::OneMeter));
		SidePieceComboBox->AddOption(VideoWallSupportSpacingToOption(EVideoWallSupportSpacing::OnePointFiveMeters));
		SidePieceComboBox->SetSelectedOption(VideoWallSupportSpacingToOption(CurrentVideoWallDefinition.SupportSpacing));
	}

	if (DepthPieceLabelText) DepthPieceLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (DepthPieceComboBox) DepthPieceComboBox->SetVisibility(ESlateVisibility::Collapsed);

	if (IsEditingVideoWall())
	{
		ApplyVideoWallEditToTarget();
	}
	else
	{
		ApplyVideoWallDefinitionToBuildManager();
	}
	bRefreshingControls = false;
}

void UBuildMenuWidget::RefreshProjectionControls()
{
	if (ActiveMenuTab != EBuildItemType::ProjectionScreen)
	{
		return;
	}

	bRefreshingControls = true;

	if (ModeLabelText)
	{
		ModeLabelText->SetText(FText::FromString(TEXT("Screen")));
		ModeLabelText->SetVisibility(ESlateVisibility::Visible);
	}

	if (ModeComboBox)
	{
		ModeComboBox->SetVisibility(ESlateVisibility::Visible);
		ModeComboBox->ClearOptions();
		for (EProjectionScreenKitSize ScreenSize : {
			EProjectionScreenKitSize::Screen6x12,
			EProjectionScreenKitSize::Screen8x14,
			EProjectionScreenKitSize::Screen9x16,
			EProjectionScreenKitSize::Screen13x24})
		{
			ModeComboBox->AddOption(ProjectionScreenSizeToOption(ScreenSize));
		}
		ModeComboBox->SetSelectedOption(ProjectionScreenSizeToOption(CurrentProjectionScreenDefinition.ScreenKitSize));
	}

	if (SidePieceLabelText)
	{
		SidePieceLabelText->SetText(FText::FromString(TEXT("Projector")));
		SidePieceLabelText->SetVisibility(ESlateVisibility::Visible);
	}

	if (SidePieceComboBox)
	{
		SidePieceComboBox->SetVisibility(ESlateVisibility::Visible);
		SidePieceComboBox->ClearOptions();
		SidePieceComboBox->AddOption(ProjectionProjectorToOption(EProjectionProjectorType::ChristieM4K25RGB));
		SidePieceComboBox->SetSelectedOption(ProjectionProjectorToOption(CurrentProjectionScreenDefinition.ProjectorType));
	}

	if (DepthPieceLabelText)
	{
		DepthPieceLabelText->SetText(FText::FromString(TEXT("Lens")));
		DepthPieceLabelText->SetVisibility(ESlateVisibility::Visible);
	}

	if (DepthPieceComboBox)
	{
		DepthPieceComboBox->SetVisibility(ESlateVisibility::Visible);
		DepthPieceComboBox->ClearOptions();
		for (EProjectionLensType LensType : {
			EProjectionLensType::ILS067HD,
			EProjectionLensType::ILS116149HD,
			EProjectionLensType::ILS4169HD})
		{
			DepthPieceComboBox->AddOption(ProjectionLensToOption(LensType));
		}
		DepthPieceComboBox->SetSelectedOption(ProjectionLensToOption(CurrentProjectionScreenDefinition.LensType));
	}

	float ScreenWidthFt = 0.0f;
	float ScreenHeightFt = 0.0f;
	GetProjectionScreenDimensionsFt(CurrentProjectionScreenDefinition.ScreenKitSize, ScreenWidthFt, ScreenHeightFt);

	float VerticalMinPercent = 0.0f;
	float VerticalMaxPercent = 0.0f;
	float HorizontalMinPercent = 0.0f;
	float HorizontalMaxPercent = 0.0f;
	bool bHasShiftData = false;
	GetProjectionLensShiftPercent(CurrentProjectionScreenDefinition.LensType, VerticalMinPercent, VerticalMaxPercent, HorizontalMinPercent, HorizontalMaxPercent, bHasShiftData);

	const float HorizontalMinFt = bHasShiftData ? ScreenWidthFt * (HorizontalMinPercent / 100.0f) : 0.0f;
	const float HorizontalMaxFt = bHasShiftData ? ScreenWidthFt * (HorizontalMaxPercent / 100.0f) : 0.0f;
	const float VerticalMinFt = bHasShiftData ? ScreenHeightFt * (VerticalMinPercent / 100.0f) : 0.0f;
	const float VerticalMaxFt = bHasShiftData ? ScreenHeightFt * (VerticalMaxPercent / 100.0f) : 0.0f;

	CurrentProjectionScreenDefinition.ProjectorHorizontalOffsetFt = FMath::Clamp(CurrentProjectionScreenDefinition.ProjectorHorizontalOffsetFt, HorizontalMinFt, HorizontalMaxFt);
	CurrentProjectionScreenDefinition.TowerHeightFt = FMath::Clamp(CurrentProjectionScreenDefinition.TowerHeightFt, VerticalMinFt, VerticalMaxFt);

	auto SetNumericControl = [](UTextBlock* Label, USpinBox* SpinBox, const TCHAR* LabelText, float Value, float MinValue, float MaxValue)
	{
		if (Label)
		{
			Label->SetText(FText::FromString(LabelText));
			Label->SetVisibility(ESlateVisibility::Visible);
		}

		if (SpinBox)
		{
			SpinBox->SetVisibility(ESlateVisibility::Visible);
			SpinBox->SetMinValue(MinValue);
			SpinBox->SetMaxValue(MaxValue);
			SpinBox->SetMinSliderValue(MinValue);
			SpinBox->SetMaxSliderValue(MaxValue);
			SpinBox->SetValue(Value);
		}
	};

	SetNumericControl(PrimaryValueLabelText, PrimaryValueSpinBox, TEXT("Projector Left / Right (ft)"), CurrentProjectionScreenDefinition.ProjectorHorizontalOffsetFt, HorizontalMinFt, HorizontalMaxFt);
	SetNumericControl(SecondaryValueLabelText, SecondaryValueSpinBox, TEXT("Projector Up / Down (ft)"), CurrentProjectionScreenDefinition.TowerHeightFt, VerticalMinFt, VerticalMaxFt);

	if (IsEditingProjection())
	{
		SetNumericControl(TertiaryValueLabelText, TertiaryValueSpinBox, TEXT("Center Left / Right (cm)"), CurrentProjectionScreenDefinition.ScreenCenterOffsetCm.X, -500.0f, 500.0f);
		SetNumericControl(QuaternaryValueLabelText, QuaternaryValueSpinBox, TEXT("Center Up / Down (cm)"), CurrentProjectionScreenDefinition.ScreenCenterOffsetCm.Z, -500.0f, 500.0f);
	}
	else
	{
		if (TertiaryValueLabelText) TertiaryValueLabelText->SetVisibility(ESlateVisibility::Collapsed);
		if (TertiaryValueSpinBox) TertiaryValueSpinBox->SetVisibility(ESlateVisibility::Collapsed);
		if (QuaternaryValueLabelText) QuaternaryValueLabelText->SetVisibility(ESlateVisibility::Collapsed);
		if (QuaternaryValueSpinBox) QuaternaryValueSpinBox->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (IsEditingProjection())
	{
		ApplyProjectionEditToTarget();
	}
	else
	{
		ApplyProjectionDefinitionToBuildManager();
	}
	bRefreshingControls = false;
}

void UBuildMenuWidget::RefreshAudioControls()
{
	if (ActiveMenuTab != EBuildItemType::AudioPlacement)
	{
		return;
	}

	bRefreshingControls = true;

	const bool bLineArray = CurrentAudioPlacementDefinition.PlacementType == EAudioPlacementRuntimeType::GroundLineArray;
	RefreshAudioSourceOptions();

	if (ModeLabelText)
	{
		ModeLabelText->SetText(FText::FromString(TEXT("Audio Type")));
		ModeLabelText->SetVisibility(IsEditingAudio() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}

	if (ModeComboBox)
	{
		ModeComboBox->SetVisibility(IsEditingAudio() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
		ModeComboBox->ClearOptions();
		ModeComboBox->AddOption(AudioPlacementTypeToOption(EAudioPlacementRuntimeType::GroundSpeaker));
		ModeComboBox->AddOption(AudioPlacementTypeToOption(EAudioPlacementRuntimeType::GroundLineArray));
		ModeComboBox->SetSelectedOption(AudioPlacementTypeToOption(CurrentAudioPlacementDefinition.PlacementType));
	}

	if (SidePieceLabelText)
	{
		SidePieceLabelText->SetText(FText::FromString(bLineArray ? TEXT("Line Array") : TEXT("Speaker Model")));
		SidePieceLabelText->SetVisibility(ESlateVisibility::Visible);
	}

	if (SidePieceComboBox)
	{
		SidePieceComboBox->SetVisibility(ESlateVisibility::Visible);
		SidePieceComboBox->ClearOptions();
		if (bLineArray)
		{
			SidePieceComboBox->AddOption(AudioLineArrayModelToOption(EAudioGroundLineArrayModel::MLA_Mini));
			SidePieceComboBox->AddOption(AudioLineArrayModelToOption(EAudioGroundLineArrayModel::MLA_Compact));
			SidePieceComboBox->SetSelectedOption(AudioLineArrayModelToOption(CurrentAudioPlacementDefinition.GroundLineArrayDefinition.LineArrayModel));
		}
		else
		{
			for (EAudioGroundSpeakerModel SpeakerModel : {
				EAudioGroundSpeakerModel::QSC_K8,
				EAudioGroundSpeakerModel::QSC_K10,
				EAudioGroundSpeakerModel::QSC_K12,
				EAudioGroundSpeakerModel::QSC_KW153,
				EAudioGroundSpeakerModel::CDD_LIVE_15})
			{
				SidePieceComboBox->AddOption(AudioSpeakerModelToOption(SpeakerModel));
			}
			SidePieceComboBox->SetSelectedOption(AudioSpeakerModelToOption(CurrentAudioPlacementDefinition.GroundSpeakerDefinition.SpeakerModel));
		}
	}

	auto SetNumericControl = [](UTextBlock* Label, USpinBox* SpinBox, const TCHAR* LabelText, float Value, float MinValue, float MaxValue, bool bShow)
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

	SetNumericControl(
		PrimaryValueLabelText,
		PrimaryValueSpinBox,
		bLineArray ? TEXT("Speaker Count") : TEXT("Stand Extension (ft)"),
		bLineArray ? CurrentAudioPlacementDefinition.GroundLineArrayDefinition.SpeakerCount : CurrentAudioPlacementDefinition.GroundSpeakerDefinition.StandExtensionFt,
		bLineArray ? 1.0f : 0.0f,
		bLineArray ? (CurrentAudioPlacementDefinition.GroundLineArrayDefinition.LineArrayModel == EAudioGroundLineArrayModel::MLA_Compact ? 6.0f : 12.0f) : 3.0f,
		true);

	SetNumericControl(
		SecondaryValueLabelText,
		SecondaryValueSpinBox,
		TEXT("Pole Extension (in)"),
		CurrentAudioPlacementDefinition.GroundLineArrayDefinition.PoleExtensionInches,
		0.0f,
		17.0f,
		bLineArray);

	SetNumericControl(TertiaryValueLabelText, TertiaryValueSpinBox, TEXT(""), 0.0f, 0.0f, 0.0f, false);
	SetNumericControl(QuaternaryValueLabelText, QuaternaryValueSpinBox, TEXT(""), 0.0f, 0.0f, 0.0f, false);

	if (DepthPieceLabelText)
	{
		DepthPieceLabelText->SetText(FText::FromString(TEXT("Sound")));
		DepthPieceLabelText->SetVisibility(IsEditingAudio() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (DepthPieceComboBox)
	{
		DepthPieceComboBox->SetVisibility(IsEditingAudio() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		DepthPieceComboBox->ClearOptions();
		for (const FString& Option : AudioSourceOptions)
		{
			DepthPieceComboBox->AddOption(Option);
		}
		DepthPieceComboBox->SetSelectedOption(AudioSourceToOption(CurrentAudioSource));
	}

	if (StageAutomaticSkirtLabelText)
	{
		StageAutomaticSkirtLabelText->SetText(FText::FromString(TEXT("Sub Only")));
		StageAutomaticSkirtLabelText->SetVisibility(bLineArray ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (StageAutomaticSkirtCheckBox)
	{
		StageAutomaticSkirtCheckBox->SetVisibility(bLineArray ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (bLineArray)
		{
			StageAutomaticSkirtCheckBox->SetIsChecked(CurrentAudioPlacementDefinition.GroundLineArrayDefinition.bSubOnly);
		}
	}

	if (!IsEditingAudio())
	{
		ApplyAudioDefinitionToBuildManager();
	}
	bRefreshingControls = false;
}

void UBuildMenuWidget::RefreshLoungeControls()
{
	if (ActiveMenuTab != EBuildItemType::LoungeLayout || (SelectedBuildItem && SelectedBuildItem->ItemType == EBuildItemType::SectionalLayout))
	{
		return;
	}

	bRefreshingControls = true;

	const TArray<FString> SofaOptions = GetLoungeItemOptionsFromFolder(LoungeSofaAssetPath);
	const TArray<FString> ChairOptions = GetLoungeItemOptionsFromFolder(LoungeChairAssetPath);
	const TArray<FString> CocktailOptions = GetLoungeItemOptionsFromFolder(LoungeCocktailTableAssetPath);
	const TArray<FString> EndTableOptions = GetLoungeItemOptionsFromFolder(LoungeEndTableAssetPath);
	const TArray<FString> LampOptions = GetLoungeItemOptionsFromFolder(LoungeLampAssetPath);
	const TArray<FString> AccentOptions = GetLoungeItemOptionsFromFolder(LoungeAccentAssetPath);

	CurrentLoungeLayoutDefinition.SofaItem = EnsureLoungeOption(CurrentLoungeLayoutDefinition.SofaItem, SofaOptions);
	CurrentLoungeLayoutDefinition.ChairItem = EnsureLoungeOption(CurrentLoungeLayoutDefinition.ChairItem, ChairOptions);
	CurrentLoungeLayoutDefinition.CocktailTableItem = EnsureLoungeOption(CurrentLoungeLayoutDefinition.CocktailTableItem, CocktailOptions);
	CurrentLoungeLayoutDefinition.EndTableItem = EnsureLoungeOption(CurrentLoungeLayoutDefinition.EndTableItem, EndTableOptions);
	CurrentLoungeLayoutDefinition.LampItem = EnsureLoungeOption(CurrentLoungeLayoutDefinition.LampItem, LampOptions);
	CurrentLoungeLayoutDefinition.AccentItem = EnsureLoungeOption(CurrentLoungeLayoutDefinition.AccentItem, AccentOptions);

	auto SetComboControl = [](UTextBlock* Label, UWhiteComboBoxString* ComboBox, const TCHAR* LabelText, const TArray<FString>& Options, const FString& SelectedOption)
	{
		if (Label)
		{
			Label->SetText(FText::FromString(LabelText));
			Label->SetVisibility(ESlateVisibility::Visible);
		}

		if (ComboBox)
		{
			ComboBox->SetVisibility(ESlateVisibility::Visible);
			ComboBox->ClearOptions();
			for (const FString& Option : Options)
			{
				ComboBox->AddOption(Option);
			}
			if (!SelectedOption.IsEmpty())
			{
				ComboBox->SetSelectedOption(SelectedOption);
			}
		}
	};

	SetComboControl(ModeLabelText, ModeComboBox, TEXT("Sofa"), SofaOptions, CurrentLoungeLayoutDefinition.SofaItem);
	SetComboControl(SidePieceLabelText, SidePieceComboBox, TEXT("Chair"), ChairOptions, CurrentLoungeLayoutDefinition.ChairItem);
	SetComboControl(DepthPieceLabelText, DepthPieceComboBox, TEXT("Cocktail Table"), CocktailOptions, CurrentLoungeLayoutDefinition.CocktailTableItem);
	SetComboControl(MBPStyleLabelText, MBPStyleComboBox, TEXT("End Table"), EndTableOptions, CurrentLoungeLayoutDefinition.EndTableItem);
	SetComboControl(StageHeightLabelText, StageHeightComboBox, TEXT("Lamp"), LampOptions, CurrentLoungeLayoutDefinition.LampItem);
	SetComboControl(StageSurfaceLabelText, StageSurfaceComboBox, TEXT("Accent"), AccentOptions, CurrentLoungeLayoutDefinition.AccentItem);

	auto SetNumericControl = [](UTextBlock* Label, USpinBox* SpinBox, const TCHAR* LabelText, float Value, float MinValue, float MaxValue, float SliderMax)
	{
		if (Label)
		{
			Label->SetText(FText::FromString(LabelText));
			Label->SetVisibility(ESlateVisibility::Visible);
		}

		if (SpinBox)
		{
			SpinBox->SetVisibility(ESlateVisibility::Visible);
			SpinBox->SetMinValue(MinValue);
			SpinBox->SetMaxValue(MaxValue);
			SpinBox->SetMinSliderValue(MinValue);
			SpinBox->SetMaxSliderValue(SliderMax);
			SpinBox->SetValue(Value);
		}
	};

	SetNumericControl(PrimaryValueLabelText, PrimaryValueSpinBox, TEXT("Sofa Count"), CurrentLoungeLayoutDefinition.SofaCount, 0.0f, 8.0f, 4.0f);
	SetNumericControl(SecondaryValueLabelText, SecondaryValueSpinBox, TEXT("Chair Count"), CurrentLoungeLayoutDefinition.ChairCount, 0.0f, 8.0f, 4.0f);
	SetNumericControl(TertiaryValueLabelText, TertiaryValueSpinBox, TEXT("End Table X Offset (cm)"), CurrentLoungeLayoutDefinition.EndTableOffsetXCm, -500.0f, 500.0f, 250.0f);
	SetNumericControl(QuaternaryValueLabelText, QuaternaryValueSpinBox, TEXT("Mirrored Chair Yaw (deg)"), CurrentLoungeLayoutDefinition.MirroredChairYawOffsetDegrees, -90.0f, 90.0f, 45.0f);

	if (MBPRowsLabelText)
	{
		MBPRowsLabelText->SetText(FText::FromString(TEXT("End Table Count")));
		MBPRowsLabelText->SetVisibility(ESlateVisibility::Visible);
	}
	if (MBPRowsSpinBox)
	{
		MBPRowsSpinBox->SetVisibility(ESlateVisibility::Visible);
		MBPRowsSpinBox->SetMinValue(0.0f);
		MBPRowsSpinBox->SetMaxValue(8.0f);
		MBPRowsSpinBox->SetMinSliderValue(0.0f);
		MBPRowsSpinBox->SetMaxSliderValue(4.0f);
		MBPRowsSpinBox->SetValue(CurrentLoungeLayoutDefinition.EndTableCount);
	}
	if (MBPColumnsLabelText)
	{
		MBPColumnsLabelText->SetText(FText::FromString(TEXT("Lamp Count")));
		MBPColumnsLabelText->SetVisibility(ESlateVisibility::Visible);
	}
	if (MBPColumnsSpinBox)
	{
		MBPColumnsSpinBox->SetVisibility(ESlateVisibility::Visible);
		MBPColumnsSpinBox->SetMinValue(0.0f);
		MBPColumnsSpinBox->SetMaxValue(8.0f);
		MBPColumnsSpinBox->SetMinSliderValue(0.0f);
		MBPColumnsSpinBox->SetMaxSliderValue(4.0f);
		MBPColumnsSpinBox->SetValue(CurrentLoungeLayoutDefinition.LampCount);
	}

	if (StageFrontRailingLabelText)
	{
		StageFrontRailingLabelText->SetText(FText::FromString(TEXT("Two Sofas")));
		StageFrontRailingLabelText->SetVisibility(ESlateVisibility::Visible);
	}
	if (StageFrontRailingCheckBox)
	{
		StageFrontRailingCheckBox->SetVisibility(ESlateVisibility::Visible);
		StageFrontRailingCheckBox->SetIsChecked(CurrentLoungeLayoutDefinition.bUseTwoSofas);
	}
	if (StageBackRailingLabelText)
	{
		StageBackRailingLabelText->SetText(FText::FromString(TEXT("Four Chairs")));
		StageBackRailingLabelText->SetVisibility(ESlateVisibility::Visible);
	}
	if (StageBackRailingCheckBox)
	{
		StageBackRailingCheckBox->SetVisibility(ESlateVisibility::Visible);
		StageBackRailingCheckBox->SetIsChecked(CurrentLoungeLayoutDefinition.bUseFourChairs);
	}

	if (StageLeftRailingLabelText) StageLeftRailingLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageLeftRailingCheckBox) StageLeftRailingCheckBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageRightRailingLabelText) StageRightRailingLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageRightRailingCheckBox) StageRightRailingCheckBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageLeftStepLabelText) StageLeftStepLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageLeftStepCheckBox) StageLeftStepCheckBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageRightStepLabelText) StageRightStepLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageRightStepCheckBox) StageRightStepCheckBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageAutomaticSkirtLabelText) StageAutomaticSkirtLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageAutomaticSkirtCheckBox) StageAutomaticSkirtCheckBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageCellEnabledLabelText) StageCellEnabledLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageCellEnabledCheckBox) StageCellEnabledCheckBox->SetVisibility(ESlateVisibility::Collapsed);

	if (IsEditingLounge())
	{
		ApplyLoungeEditToTarget();
	}
	else
	{
		ApplyLoungeDefinitionToBuildManager();
	}
	bRefreshingControls = false;
}

void UBuildMenuWidget::RefreshSectionalControls()
{
	if (ActiveMenuTab != EBuildItemType::LoungeLayout || !SelectedBuildItem || SelectedBuildItem->ItemType != EBuildItemType::SectionalLayout)
	{
		return;
	}

	bRefreshingControls = true;

	const TArray<FString> ArmlessOptions = GetLoungeItemOptionsFromFolder(SectionalArmlessAssetPath);
	const TArray<FString> CornerOptions = GetLoungeItemOptionsFromFolder(SectionalCornerAssetPath);
	CurrentSectionalLayoutDefinition.ArmlessItem = EnsureLoungeOption(CurrentSectionalLayoutDefinition.ArmlessItem, ArmlessOptions);
	CurrentSectionalLayoutDefinition.CornerItem = EnsureLoungeOption(CurrentSectionalLayoutDefinition.CornerItem, CornerOptions);

	auto SetComboControl = [](UTextBlock* Label, UWhiteComboBoxString* ComboBox, const TCHAR* LabelText, const TArray<FString>& Options, const FString& SelectedOption)
	{
		if (Label)
		{
			Label->SetText(FText::FromString(LabelText));
			Label->SetVisibility(ESlateVisibility::Visible);
		}

		if (ComboBox)
		{
			ComboBox->SetVisibility(ESlateVisibility::Visible);
			ComboBox->ClearOptions();
			for (const FString& Option : Options)
			{
				ComboBox->AddOption(Option);
			}
			if (!SelectedOption.IsEmpty())
			{
				ComboBox->SetSelectedOption(SelectedOption);
			}
		}
	};

	TArray<FString> ShapeOptions;
	ShapeOptions.Add(SectionalShapeToOption(ESectionalLayoutShape::OneSided));
	ShapeOptions.Add(SectionalShapeToOption(ESectionalLayoutShape::TwoSidedL));
	ShapeOptions.Add(SectionalShapeToOption(ESectionalLayoutShape::ThreeSidedU));
	ShapeOptions.Add(SectionalShapeToOption(ESectionalLayoutShape::FourSidedBox));

	SetComboControl(ModeLabelText, ModeComboBox, TEXT("Shape"), ShapeOptions, SectionalShapeToOption(CurrentSectionalLayoutDefinition.Shape));
	SetComboControl(SidePieceLabelText, SidePieceComboBox, TEXT("Armless"), ArmlessOptions, CurrentSectionalLayoutDefinition.ArmlessItem);
	SetComboControl(DepthPieceLabelText, DepthPieceComboBox, TEXT("Corner"), CornerOptions, CurrentSectionalLayoutDefinition.CornerItem);

	auto SetNumericControl = [](UTextBlock* Label, USpinBox* SpinBox, const TCHAR* LabelText, float Value, float MinValue, float MaxValue, float SliderMin, float SliderMax)
	{
		if (Label)
		{
			Label->SetText(FText::FromString(LabelText));
			Label->SetVisibility(ESlateVisibility::Visible);
		}

		if (SpinBox)
		{
			SpinBox->SetVisibility(ESlateVisibility::Visible);
			SpinBox->SetMinValue(MinValue);
			SpinBox->SetMaxValue(MaxValue);
			SpinBox->SetMinSliderValue(SliderMin);
			SpinBox->SetMaxSliderValue(SliderMax);
			SpinBox->SetValue(Value);
		}
	};

	SetNumericControl(PrimaryValueLabelText, PrimaryValueSpinBox, TEXT("Width Sections"), CurrentSectionalLayoutDefinition.WidthSections, 1.0f, 20.0f, 1.0f, 10.0f);
	SetNumericControl(SecondaryValueLabelText, SecondaryValueSpinBox, TEXT("Depth Sections"), CurrentSectionalLayoutDefinition.DepthSections, 1.0f, 20.0f, 1.0f, 10.0f);
	SetNumericControl(TertiaryValueLabelText, TertiaryValueSpinBox, TEXT("Section Spacing (cm)"), CurrentSectionalLayoutDefinition.SectionSpacingCm, -500.0f, 500.0f, -150.0f, 150.0f);

	if (StageFrontRailingLabelText)
	{
		StageFrontRailingLabelText->SetText(FText::FromString(TEXT("Use Item Spacing")));
		StageFrontRailingLabelText->SetVisibility(ESlateVisibility::Visible);
	}
	if (StageFrontRailingCheckBox)
	{
		StageFrontRailingCheckBox->SetVisibility(ESlateVisibility::Visible);
		StageFrontRailingCheckBox->SetIsChecked(CurrentSectionalLayoutDefinition.bUseArmlessItemDefaultSpacing);
	}

	if (QuaternaryValueLabelText) QuaternaryValueLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (QuaternaryValueSpinBox) QuaternaryValueSpinBox->SetVisibility(ESlateVisibility::Collapsed);
	if (MBPRowsLabelText) MBPRowsLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (MBPRowsSpinBox) MBPRowsSpinBox->SetVisibility(ESlateVisibility::Collapsed);
	if (MBPColumnsLabelText) MBPColumnsLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (MBPColumnsSpinBox) MBPColumnsSpinBox->SetVisibility(ESlateVisibility::Collapsed);
	if (MBPStyleLabelText) MBPStyleLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (MBPStyleComboBox) MBPStyleComboBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageHeightLabelText) StageHeightLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageHeightComboBox) StageHeightComboBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageSurfaceLabelText) StageSurfaceLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageSurfaceComboBox) StageSurfaceComboBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageBackRailingLabelText) StageBackRailingLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageBackRailingCheckBox) StageBackRailingCheckBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageLeftRailingLabelText) StageLeftRailingLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageLeftRailingCheckBox) StageLeftRailingCheckBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageRightRailingLabelText) StageRightRailingLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageRightRailingCheckBox) StageRightRailingCheckBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageLeftStepLabelText) StageLeftStepLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageLeftStepCheckBox) StageLeftStepCheckBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageRightStepLabelText) StageRightStepLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageRightStepCheckBox) StageRightStepCheckBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageAutomaticSkirtLabelText) StageAutomaticSkirtLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageAutomaticSkirtCheckBox) StageAutomaticSkirtCheckBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageCellEnabledLabelText) StageCellEnabledLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageCellEnabledCheckBox) StageCellEnabledCheckBox->SetVisibility(ESlateVisibility::Collapsed);

	ApplySectionalDefinitionToBuildManager();
	bRefreshingControls = false;
}

void UBuildMenuWidget::RefreshFurnitureSeatingControls()
{
	if (ActiveMenuTab != EBuildItemType::FurnitureSeating)
	{
		return;
	}

	bRefreshingControls = true;

	const bool bCafe = CurrentFurnitureSeatingDefinition.SeatingType == EFurnitureSeatingRuntimeType::CafeTable;
	TArray<FString> TypeOptions;
	TypeOptions.Add(FurnitureSeatingTypeToOption(EFurnitureSeatingRuntimeType::BarSeating));
	TypeOptions.Add(FurnitureSeatingTypeToOption(EFurnitureSeatingRuntimeType::CafeTable));

	const TArray<FString> TableOptions = GetLoungeItemOptionsFromFolder(bCafe ? CafeTableAssetPath : BarTableAssetPath);
	const TArray<FString> SeatOptions = GetLoungeItemOptionsFromFolder(bCafe ? DiningChairAssetPath : StoolAssetPath);

	if (bCafe)
	{
		CurrentFurnitureSeatingDefinition.CafeTableDefinition.CafeTableItem = EnsureLoungeOption(CurrentFurnitureSeatingDefinition.CafeTableDefinition.CafeTableItem, TableOptions);
		CurrentFurnitureSeatingDefinition.CafeTableDefinition.DiningChairItem = EnsureLoungeOption(CurrentFurnitureSeatingDefinition.CafeTableDefinition.DiningChairItem, SeatOptions);
	}
	else
	{
		CurrentFurnitureSeatingDefinition.BarSeatingDefinition.BarTableItem = EnsureLoungeOption(CurrentFurnitureSeatingDefinition.BarSeatingDefinition.BarTableItem, TableOptions);
		CurrentFurnitureSeatingDefinition.BarSeatingDefinition.StoolItem = EnsureLoungeOption(CurrentFurnitureSeatingDefinition.BarSeatingDefinition.StoolItem, SeatOptions);
	}

	auto SetComboControl = [](UTextBlock* Label, UWhiteComboBoxString* ComboBox, const TCHAR* LabelText, const TArray<FString>& Options, const FString& SelectedOption)
	{
		if (Label)
		{
			Label->SetText(FText::FromString(LabelText));
			Label->SetVisibility(ESlateVisibility::Visible);
		}

		if (ComboBox)
		{
			ComboBox->SetVisibility(ESlateVisibility::Visible);
			ComboBox->ClearOptions();
			for (const FString& Option : Options)
			{
				ComboBox->AddOption(Option);
			}
			if (!SelectedOption.IsEmpty())
			{
				ComboBox->SetSelectedOption(SelectedOption);
			}
		}
	};

	TArray<FString> LayoutOptions;
	if (bCafe)
	{
		LayoutOptions.Add(CafeTableLayoutToOption(ECafeTableChairLayout::NoChairs));
		LayoutOptions.Add(CafeTableLayoutToOption(ECafeTableChairLayout::TwoTop));
		LayoutOptions.Add(CafeTableLayoutToOption(ECafeTableChairLayout::FourTop));
		LayoutOptions.Add(CafeTableLayoutToOption(ECafeTableChairLayout::RectangleFourTop));
		LayoutOptions.Add(CafeTableLayoutToOption(ECafeTableChairLayout::RectangleSixTop));
	}
	else
	{
		LayoutOptions.Add(BarSeatingLayoutToOption(EBarSeatingStoolLayout::NoStools));
		LayoutOptions.Add(BarSeatingLayoutToOption(EBarSeatingStoolLayout::TwoTop));
		LayoutOptions.Add(BarSeatingLayoutToOption(EBarSeatingStoolLayout::FourTop));
		LayoutOptions.Add(BarSeatingLayoutToOption(EBarSeatingStoolLayout::RectangleFourTop));
		LayoutOptions.Add(BarSeatingLayoutToOption(EBarSeatingStoolLayout::RectangleSixTop));
	}

	SetComboControl(ModeLabelText, ModeComboBox, TEXT("Tool"), TypeOptions, FurnitureSeatingTypeToOption(CurrentFurnitureSeatingDefinition.SeatingType));
	SetComboControl(SidePieceLabelText, SidePieceComboBox, bCafe ? TEXT("Cafe Table") : TEXT("Bar Table"), TableOptions, bCafe ? CurrentFurnitureSeatingDefinition.CafeTableDefinition.CafeTableItem : CurrentFurnitureSeatingDefinition.BarSeatingDefinition.BarTableItem);
	SetComboControl(DepthPieceLabelText, DepthPieceComboBox, bCafe ? TEXT("Dining Chair") : TEXT("Stool"), SeatOptions, bCafe ? CurrentFurnitureSeatingDefinition.CafeTableDefinition.DiningChairItem : CurrentFurnitureSeatingDefinition.BarSeatingDefinition.StoolItem);
	SetComboControl(MBPStyleLabelText, MBPStyleComboBox, TEXT("Chair Layout"), LayoutOptions, bCafe ? CafeTableLayoutToOption(CurrentFurnitureSeatingDefinition.CafeTableDefinition.ChairLayout) : BarSeatingLayoutToOption(CurrentFurnitureSeatingDefinition.BarSeatingDefinition.StoolLayout));

	auto SetNumericControl = [](UTextBlock* Label, USpinBox* SpinBox, const TCHAR* LabelText, float Value, float MinValue, float MaxValue, float SliderMin, float SliderMax)
	{
		if (Label)
		{
			Label->SetText(FText::FromString(LabelText));
			Label->SetVisibility(ESlateVisibility::Visible);
		}

		if (SpinBox)
		{
			SpinBox->SetVisibility(ESlateVisibility::Visible);
			SpinBox->SetMinValue(MinValue);
			SpinBox->SetMaxValue(MaxValue);
			SpinBox->SetMinSliderValue(SliderMin);
			SpinBox->SetMaxSliderValue(SliderMax);
			SpinBox->SetValue(Value);
		}
	};

	SetNumericControl(PrimaryValueLabelText, PrimaryValueSpinBox, bCafe ? TEXT("Chair Y Offset (cm)") : TEXT("Stool Y Offset (cm)"), bCafe ? CurrentFurnitureSeatingDefinition.CafeTableDefinition.ChairOffsetYCm : CurrentFurnitureSeatingDefinition.BarSeatingDefinition.StoolOffsetYCm, bCafe ? -500.0f : 0.0f, 500.0f, bCafe ? -100.0f : 0.0f, 100.0f);
	SetNumericControl(SecondaryValueLabelText, SecondaryValueSpinBox, bCafe ? TEXT("Rectangle Chair Pair X Offset (cm)") : TEXT("Rectangle Stool Pair X Offset (cm)"), bCafe ? CurrentFurnitureSeatingDefinition.CafeTableDefinition.RectangleChairPairOffsetXCm : CurrentFurnitureSeatingDefinition.BarSeatingDefinition.RectangleStoolPairOffsetXCm, 0.0f, 1000.0f, 0.0f, 300.0f);

	if (StageFrontRailingLabelText)
	{
		StageFrontRailingLabelText->SetText(FText::FromString(TEXT("Classroom Style")));
		StageFrontRailingLabelText->SetVisibility(ESlateVisibility::Visible);
	}
	if (StageFrontRailingCheckBox)
	{
		StageFrontRailingCheckBox->SetVisibility(ESlateVisibility::Visible);
		StageFrontRailingCheckBox->SetIsChecked(bCafe ? CurrentFurnitureSeatingDefinition.CafeTableDefinition.bClassroomStyle : CurrentFurnitureSeatingDefinition.BarSeatingDefinition.bClassroomStyle);
	}

	if (TertiaryValueLabelText) TertiaryValueLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (TertiaryValueSpinBox) TertiaryValueSpinBox->SetVisibility(ESlateVisibility::Collapsed);
	if (QuaternaryValueLabelText) QuaternaryValueLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (QuaternaryValueSpinBox) QuaternaryValueSpinBox->SetVisibility(ESlateVisibility::Collapsed);
	if (MBPRowsLabelText) MBPRowsLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (MBPRowsSpinBox) MBPRowsSpinBox->SetVisibility(ESlateVisibility::Collapsed);
	if (MBPColumnsLabelText) MBPColumnsLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (MBPColumnsSpinBox) MBPColumnsSpinBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageHeightLabelText) StageHeightLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageHeightComboBox) StageHeightComboBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageSurfaceLabelText) StageSurfaceLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageSurfaceComboBox) StageSurfaceComboBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageBackRailingLabelText) StageBackRailingLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageBackRailingCheckBox) StageBackRailingCheckBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageLeftRailingLabelText) StageLeftRailingLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageLeftRailingCheckBox) StageLeftRailingCheckBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageRightRailingLabelText) StageRightRailingLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageRightRailingCheckBox) StageRightRailingCheckBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageLeftStepLabelText) StageLeftStepLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageLeftStepCheckBox) StageLeftStepCheckBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageRightStepLabelText) StageRightStepLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageRightStepCheckBox) StageRightStepCheckBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageAutomaticSkirtLabelText) StageAutomaticSkirtLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageAutomaticSkirtCheckBox) StageAutomaticSkirtCheckBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageCellEnabledLabelText) StageCellEnabledLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageCellEnabledCheckBox) StageCellEnabledCheckBox->SetVisibility(ESlateVisibility::Collapsed);

	if (IsEditingFurnitureSeating())
	{
		ApplyFurnitureSeatingEditToTarget();
	}
	else
	{
		ApplyFurnitureSeatingDefinitionToBuildManager();
	}
	bRefreshingControls = false;
}

void UBuildMenuWidget::RefreshDiningTableControls()
{
	if (ActiveMenuTab != EBuildItemType::DiningTable && (!SelectedBuildItem || SelectedBuildItem->ItemType != EBuildItemType::DiningTable))
	{
		return;
	}

	bRefreshingControls = true;

	const TArray<FString> TableOptions = GetLoungeItemOptionsFromFolder(DiningTableAssetPath);
	const TArray<FString> ChairOptions = GetLoungeItemOptionsFromFolder(DiningChairAssetPath);
	const TArray<FString> TableclothOptions = GetFlatAssetOptionsFromFolder(DiningTableclothAssetPath);
	CurrentDiningTableDefinition.DiningTableItem = EnsureLoungeOption(CurrentDiningTableDefinition.DiningTableItem, TableOptions);
	CurrentDiningTableDefinition.DiningChairItem = EnsureLoungeOption(CurrentDiningTableDefinition.DiningChairItem, ChairOptions);
	CurrentDiningTableDefinition.TableclothItem = EnsureLoungeOption(CurrentDiningTableDefinition.TableclothItem, TableclothOptions);
	CurrentDiningTableDefinition.ChairGapCm = FMath::Clamp(CurrentDiningTableDefinition.ChairGapCm, -200.0f, 80.0f);
	CurrentDiningTableDefinition.TableclothOverhangCm = FMath::Clamp(CurrentDiningTableDefinition.TableclothOverhangCm, 0.0f, 80.0f);
	CurrentDiningTableDefinition.TableclothDropCm = FMath::Clamp(CurrentDiningTableDefinition.TableclothDropCm, 0.0f, 120.0f);

	auto SetComboControl = [](UTextBlock* Label, UWhiteComboBoxString* ComboBox, const TCHAR* LabelText, const TArray<FString>& Options, const FString& SelectedOption)
	{
		if (Label)
		{
			Label->SetText(FText::FromString(LabelText));
			Label->SetVisibility(ESlateVisibility::Visible);
		}

		if (ComboBox)
		{
			ComboBox->SetVisibility(ESlateVisibility::Visible);
			ComboBox->ClearOptions();
			for (const FString& Option : Options)
			{
				ComboBox->AddOption(Option);
			}
			if (!SelectedOption.IsEmpty())
			{
				ComboBox->SetSelectedOption(SelectedOption);
			}
		}
	};

	TArray<FString> CountOptions;
	CountOptions.Add(DiningChairCountToOption(EDiningChairCount::Two));
	CountOptions.Add(DiningChairCountToOption(EDiningChairCount::Four));
	CountOptions.Add(DiningChairCountToOption(EDiningChairCount::Six));
	CountOptions.Add(DiningChairCountToOption(EDiningChairCount::Eight));

	TArray<FString> ArrangementOptions;
	ArrangementOptions.Add(DiningChairArrangementToOption(EDiningChairArrangement::Rectangular));
	ArrangementOptions.Add(DiningChairArrangementToOption(EDiningChairArrangement::Radial));

	SetComboControl(ModeLabelText, ModeComboBox, TEXT("Chairs"), CountOptions, DiningChairCountToOption(CurrentDiningTableDefinition.ChairCount));
	SetComboControl(SidePieceLabelText, SidePieceComboBox, TEXT("Dining Table"), TableOptions, CurrentDiningTableDefinition.DiningTableItem);
	SetComboControl(DepthPieceLabelText, DepthPieceComboBox, TEXT("Dining Chair"), ChairOptions, CurrentDiningTableDefinition.DiningChairItem);
	SetComboControl(MBPStyleLabelText, MBPStyleComboBox, TEXT("Arrangement"), ArrangementOptions, DiningChairArrangementToOption(CurrentDiningTableDefinition.ChairArrangement));
	SetComboControl(StageHeightLabelText, StageHeightComboBox, TEXT("Tablecloth"), TableclothOptions, CurrentDiningTableDefinition.TableclothItem);

	if (PrimaryValueLabelText)
	{
		PrimaryValueLabelText->SetText(FText::FromString(TEXT("Chair Gap (cm)")));
		PrimaryValueLabelText->SetVisibility(ESlateVisibility::Visible);
	}
	if (PrimaryValueSpinBox)
	{
		PrimaryValueSpinBox->SetVisibility(ESlateVisibility::Visible);
		PrimaryValueSpinBox->SetMinValue(-200.0f);
		PrimaryValueSpinBox->SetMaxValue(80.0f);
		PrimaryValueSpinBox->SetMinSliderValue(-200.0f);
		PrimaryValueSpinBox->SetMaxSliderValue(80.0f);
		PrimaryValueSpinBox->SetValue(CurrentDiningTableDefinition.ChairGapCm);
	}

	if (SecondaryValueLabelText)
	{
		SecondaryValueLabelText->SetText(FText::FromString(TEXT("Cloth Overhang (cm)")));
		SecondaryValueLabelText->SetVisibility(ESlateVisibility::Visible);
	}
	if (SecondaryValueSpinBox)
	{
		SecondaryValueSpinBox->SetVisibility(ESlateVisibility::Visible);
		SecondaryValueSpinBox->SetMinValue(0.0f);
		SecondaryValueSpinBox->SetMaxValue(80.0f);
		SecondaryValueSpinBox->SetMinSliderValue(0.0f);
		SecondaryValueSpinBox->SetMaxSliderValue(80.0f);
		SecondaryValueSpinBox->SetValue(CurrentDiningTableDefinition.TableclothOverhangCm);
	}
	if (TertiaryValueLabelText)
	{
		TertiaryValueLabelText->SetText(FText::FromString(TEXT("Cloth Drop (cm)")));
		TertiaryValueLabelText->SetVisibility(ESlateVisibility::Visible);
	}
	if (TertiaryValueSpinBox)
	{
		TertiaryValueSpinBox->SetVisibility(ESlateVisibility::Visible);
		TertiaryValueSpinBox->SetMinValue(0.0f);
		TertiaryValueSpinBox->SetMaxValue(120.0f);
		TertiaryValueSpinBox->SetMinSliderValue(0.0f);
		TertiaryValueSpinBox->SetMaxSliderValue(120.0f);
		TertiaryValueSpinBox->SetValue(CurrentDiningTableDefinition.TableclothDropCm);
	}
	if (QuaternaryValueLabelText) QuaternaryValueLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (QuaternaryValueSpinBox) QuaternaryValueSpinBox->SetVisibility(ESlateVisibility::Collapsed);
	if (MBPRowsLabelText) MBPRowsLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (MBPRowsSpinBox) MBPRowsSpinBox->SetVisibility(ESlateVisibility::Collapsed);
	if (MBPColumnsLabelText) MBPColumnsLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (MBPColumnsSpinBox) MBPColumnsSpinBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageSurfaceLabelText) StageSurfaceLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageSurfaceComboBox) StageSurfaceComboBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageFrontRailingLabelText)
	{
		StageFrontRailingLabelText->SetText(FText::FromString(TEXT("Use Tablecloth")));
		StageFrontRailingLabelText->SetVisibility(ESlateVisibility::Visible);
	}
	if (StageFrontRailingCheckBox)
	{
		StageFrontRailingCheckBox->SetVisibility(ESlateVisibility::Visible);
		StageFrontRailingCheckBox->SetIsChecked(CurrentDiningTableDefinition.bUseTablecloth);
	}
	if (StageBackRailingLabelText) StageBackRailingLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageBackRailingCheckBox) StageBackRailingCheckBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageLeftRailingLabelText) StageLeftRailingLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageLeftRailingCheckBox) StageLeftRailingCheckBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageRightRailingLabelText) StageRightRailingLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageRightRailingCheckBox) StageRightRailingCheckBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageLeftStepLabelText) StageLeftStepLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageLeftStepCheckBox) StageLeftStepCheckBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageRightStepLabelText) StageRightStepLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageRightStepCheckBox) StageRightStepCheckBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageAutomaticSkirtLabelText) StageAutomaticSkirtLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageAutomaticSkirtCheckBox) StageAutomaticSkirtCheckBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageCellEnabledLabelText) StageCellEnabledLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageCellEnabledCheckBox) StageCellEnabledCheckBox->SetVisibility(ESlateVisibility::Collapsed);

	if (IsEditingDiningTable())
	{
		ApplyDiningTableEditToTarget();
	}
	else
	{
		ApplyDiningTableDefinitionToBuildManager();
	}
	bRefreshingControls = false;
}

void UBuildMenuWidget::RefreshBarLayoutControls()
{
	if (ActiveMenuTab != EBuildItemType::BarLayout)
	{
		return;
	}

	bRefreshingControls = true;

	const TArray<FString> FrontOptions = GetLoungeItemOptionsFromFolder(FrontBarAssetPath);
	const TArray<FString> BackOptions = GetLoungeItemOptionsFromFolder(BackBarAssetPath);
	CurrentBarLayoutDefinition.FrontBarItem = EnsureLoungeOption(CurrentBarLayoutDefinition.FrontBarItem, FrontOptions);
	CurrentBarLayoutDefinition.BackBarItem = EnsureLoungeOption(CurrentBarLayoutDefinition.BackBarItem, BackOptions);
	CurrentBarLayoutDefinition.WallBarCount = FMath::Clamp(CurrentBarLayoutDefinition.WallBarCount, 1, 24);

	TArray<FString> ShapeOptions;
	ShapeOptions.Add(BarLayoutShapeToOption(EBarLayoutShape::Single));
	ShapeOptions.Add(BarLayoutShapeToOption(EBarLayoutShape::WallLine));
	ShapeOptions.Add(BarLayoutShapeToOption(EBarLayoutShape::CenterTwoSided));
	ShapeOptions.Add(BarLayoutShapeToOption(EBarLayoutShape::CenterFourSided));
	ShapeOptions.Add(BarLayoutShapeToOption(EBarLayoutShape::CenterSixSided));

	auto SetComboControl = [this](UTextBlock* Label, UWhiteComboBoxString* ComboBox, const TCHAR* LabelText, const TArray<FString>& Options, const FString& SelectedOption)
	{
		if (Label)
		{
			Label->SetText(FText::FromString(LabelText));
			Label->SetVisibility(ESlateVisibility::Visible);
		}
		if (ComboBox)
		{
			ComboBox->ClearOptions();
			for (const FString& Option : Options)
			{
				ComboBox->AddOption(Option);
			}
			ComboBox->SetSelectedOption(SelectedOption);
			ComboBox->SetVisibility(ESlateVisibility::Visible);
		}
	};

	SetComboControl(ModeLabelText, ModeComboBox, TEXT("Bar Layout"), ShapeOptions, BarLayoutShapeToOption(CurrentBarLayoutDefinition.LayoutShape));
	SetComboControl(SidePieceLabelText, SidePieceComboBox, TEXT("Front Bar"), FrontOptions, CurrentBarLayoutDefinition.FrontBarItem);
	SetComboControl(DepthPieceLabelText, DepthPieceComboBox, TEXT("Back Bar"), BackOptions, CurrentBarLayoutDefinition.BackBarItem);

	if (MBPStyleLabelText) MBPStyleLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (MBPStyleComboBox) MBPStyleComboBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageHeightLabelText) StageHeightLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageHeightComboBox) StageHeightComboBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageSurfaceLabelText) StageSurfaceLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageSurfaceComboBox) StageSurfaceComboBox->SetVisibility(ESlateVisibility::Collapsed);

	auto SetNumericControl = [](UTextBlock* Label, USpinBox* SpinBox, const TCHAR* LabelText, float Value, float MinValue, float MaxValue, float SliderMin, float SliderMax, bool bVisible = true)
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
			SpinBox->SetMinSliderValue(SliderMin);
			SpinBox->SetMaxSliderValue(SliderMax);
			SpinBox->SetValue(Value);
		}
	};

	SetNumericControl(PrimaryValueLabelText, PrimaryValueSpinBox, TEXT("Wall Bar Count"), CurrentBarLayoutDefinition.WallBarCount, 1.0f, 24.0f, 1.0f, 12.0f);
	SetNumericControl(SecondaryValueLabelText, SecondaryValueSpinBox, TEXT("Wall Bar Spacing (cm)"), CurrentBarLayoutDefinition.WallBarSpacingCm, -1000.0f, 1000.0f, -500.0f, 500.0f);

	float CenterOffsetCm = 0.0f;
	if (CurrentBarLayoutDefinition.LayoutShape == EBarLayoutShape::CenterTwoSided)
	{
		CenterOffsetCm = CurrentBarLayoutDefinition.CenterTwoSidedOffsetCm;
	}
	else if (CurrentBarLayoutDefinition.LayoutShape == EBarLayoutShape::CenterFourSided)
	{
		CenterOffsetCm = CurrentBarLayoutDefinition.CenterFourSidedOffsetCm;
	}
	else if (CurrentBarLayoutDefinition.LayoutShape == EBarLayoutShape::CenterSixSided)
	{
		CenterOffsetCm = CurrentBarLayoutDefinition.CenterSixSidedOffsetCm;
	}
	const bool bShowCenterOffset = CurrentBarLayoutDefinition.LayoutShape == EBarLayoutShape::CenterTwoSided ||
		CurrentBarLayoutDefinition.LayoutShape == EBarLayoutShape::CenterFourSided ||
		CurrentBarLayoutDefinition.LayoutShape == EBarLayoutShape::CenterSixSided;
	SetNumericControl(TertiaryValueLabelText, TertiaryValueSpinBox, TEXT("Center Offset (cm)"), CenterOffsetCm, -1000.0f, 1000.0f, -500.0f, 500.0f, bShowCenterOffset);

	if (QuaternaryValueLabelText) QuaternaryValueLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (QuaternaryValueSpinBox) QuaternaryValueSpinBox->SetVisibility(ESlateVisibility::Collapsed);
	if (MBPRowsLabelText) MBPRowsLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (MBPRowsSpinBox) MBPRowsSpinBox->SetVisibility(ESlateVisibility::Collapsed);
	if (MBPColumnsLabelText) MBPColumnsLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (MBPColumnsSpinBox) MBPColumnsSpinBox->SetVisibility(ESlateVisibility::Collapsed);

	if (StageFrontRailingLabelText)
	{
		StageFrontRailingLabelText->SetText(FText::FromString(TEXT("Use Front Bar")));
		StageFrontRailingLabelText->SetVisibility(ESlateVisibility::Visible);
	}
	if (StageFrontRailingCheckBox)
	{
		StageFrontRailingCheckBox->SetVisibility(ESlateVisibility::Visible);
		StageFrontRailingCheckBox->SetIsChecked(CurrentBarLayoutDefinition.bUseFrontBar);
	}
	if (StageBackRailingLabelText)
	{
		StageBackRailingLabelText->SetText(FText::FromString(TEXT("Use Back Bar")));
		StageBackRailingLabelText->SetVisibility(ESlateVisibility::Visible);
	}
	if (StageBackRailingCheckBox)
	{
		StageBackRailingCheckBox->SetVisibility(ESlateVisibility::Visible);
		StageBackRailingCheckBox->SetIsChecked(CurrentBarLayoutDefinition.bUseBackBar);
	}
	if (StageLeftRailingLabelText) StageLeftRailingLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageLeftRailingCheckBox) StageLeftRailingCheckBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageRightRailingLabelText) StageRightRailingLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageRightRailingCheckBox) StageRightRailingCheckBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageLeftStepLabelText) StageLeftStepLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageLeftStepCheckBox) StageLeftStepCheckBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageRightStepLabelText) StageRightStepLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageRightStepCheckBox) StageRightStepCheckBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageAutomaticSkirtLabelText) StageAutomaticSkirtLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageAutomaticSkirtCheckBox) StageAutomaticSkirtCheckBox->SetVisibility(ESlateVisibility::Collapsed);
	if (StageCellEnabledLabelText) StageCellEnabledLabelText->SetVisibility(ESlateVisibility::Collapsed);
	if (StageCellEnabledCheckBox) StageCellEnabledCheckBox->SetVisibility(ESlateVisibility::Collapsed);

	bRefreshingControls = false;
	ApplyBarLayoutDefinitionToBuildManager();
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

void UBuildMenuWidget::ApplyStageDefinitionToBuildManager()
{
	if (!BuildManager || !SelectedBuildItem || SelectedBuildItem->ItemType != EBuildItemType::StageDeck)
	{
		return;
	}

	BuildManager->SetActiveStageDeckDefinition(CurrentStageDeckDefinition);
}

void UBuildMenuWidget::ApplyDrapeDefinitionToBuildManager()
{
	if (!BuildManager || !SelectedBuildItem || SelectedBuildItem->ItemType != EBuildItemType::DrapeRun)
	{
		return;
	}

	BuildManager->SetActiveDrapeRunDefinition(CurrentDrapeRunDefinition);
}

void UBuildMenuWidget::ApplyVideoDefinitionToBuildManager()
{
	if (!BuildManager || !SelectedBuildItem || SelectedBuildItem->ItemType != EBuildItemType::VideoPlacement)
	{
		return;
	}

	BuildManager->SetActiveVideoPlacementDefinition(CurrentVideoPlacementDefinition);
}

void UBuildMenuWidget::ApplyVideoWallDefinitionToBuildManager()
{
	if (!BuildManager || !SelectedBuildItem || SelectedBuildItem->ItemType != EBuildItemType::VideoWall)
	{
		return;
	}

	BuildManager->SetActiveVideoWallDefinition(CurrentVideoWallDefinition);
}

void UBuildMenuWidget::ApplyProjectionDefinitionToBuildManager()
{
	if (!BuildManager || !SelectedBuildItem || SelectedBuildItem->ItemType != EBuildItemType::ProjectionScreen)
	{
		return;
	}

	BuildManager->SetActiveProjectionScreenDefinition(CurrentProjectionScreenDefinition);
}

void UBuildMenuWidget::ApplyAudioDefinitionToBuildManager()
{
	if (!BuildManager || !SelectedBuildItem || SelectedBuildItem->ItemType != EBuildItemType::AudioPlacement)
	{
		return;
	}

	BuildManager->SetActiveAudioPlacementDefinition(CurrentAudioPlacementDefinition);
}

void UBuildMenuWidget::ApplyLoungeDefinitionToBuildManager()
{
	if (IsEditingLounge())
	{
		ApplyLoungeEditToTarget();
		return;
	}

	if (!BuildManager || !SelectedBuildItem || SelectedBuildItem->ItemType != EBuildItemType::LoungeLayout)
	{
		return;
	}

	BuildManager->SetActiveLoungeLayoutDefinition(CurrentLoungeLayoutDefinition);
}

void UBuildMenuWidget::ApplySectionalDefinitionToBuildManager()
{
	if (IsEditingSectional())
	{
		ApplySectionalEditToTarget();
		return;
	}

	if (!BuildManager || !SelectedBuildItem || SelectedBuildItem->ItemType != EBuildItemType::SectionalLayout)
	{
		return;
	}

	BuildManager->SetActiveSectionalLayoutDefinition(CurrentSectionalLayoutDefinition);
}

void UBuildMenuWidget::ApplyFurnitureSeatingDefinitionToBuildManager()
{
	if (IsEditingFurnitureSeating())
	{
		ApplyFurnitureSeatingEditToTarget();
		return;
	}

	if (!BuildManager || !SelectedBuildItem || SelectedBuildItem->ItemType != EBuildItemType::FurnitureSeating)
	{
		return;
	}

	BuildManager->SetActiveFurnitureSeatingDefinition(CurrentFurnitureSeatingDefinition);
}

void UBuildMenuWidget::ApplyBarLayoutDefinitionToBuildManager()
{
	if (IsEditingBarLayout())
	{
		ApplyBarLayoutEditToTarget();
		return;
	}

	if (!BuildManager || !SelectedBuildItem || SelectedBuildItem->ItemType != EBuildItemType::BarLayout)
	{
		return;
	}

	BuildManager->SetActiveBarLayoutDefinition(CurrentBarLayoutDefinition);
}

void UBuildMenuWidget::ApplyDiningTableDefinitionToBuildManager()
{
	if (IsEditingDiningTable())
	{
		ApplyDiningTableEditToTarget();
		return;
	}

	if (!BuildManager || !SelectedBuildItem || SelectedBuildItem->ItemType != EBuildItemType::DiningTable)
	{
		return;
	}

	BuildManager->SetActiveDiningTableDefinition(CurrentDiningTableDefinition);
}

void UBuildMenuWidget::ApplyFurnitureSeatingEditToTarget()
{
	if (EditingBarSeatingTarget)
	{
		EditingBarSeatingTarget->ApplyBuildDefinition(CurrentFurnitureSeatingDefinition.BarSeatingDefinition, true);
	}
	else if (EditingCafeTableTarget)
	{
		EditingCafeTableTarget->ApplyBuildDefinition(CurrentFurnitureSeatingDefinition.CafeTableDefinition, true);
	}
}

void UBuildMenuWidget::ApplyLoungeEditToTarget()
{
	if (EditingLoungeTarget)
	{
		EditingLoungeTarget->ApplyBuildDefinition(CurrentLoungeLayoutDefinition, true);
	}
}

void UBuildMenuWidget::ApplySectionalEditToTarget()
{
	if (EditingSectionalTarget)
	{
		EditingSectionalTarget->ApplyBuildDefinition(CurrentSectionalLayoutDefinition, true);
	}
}

void UBuildMenuWidget::ApplyBarLayoutEditToTarget()
{
	if (EditingBarLayoutTarget)
	{
		EditingBarLayoutTarget->ApplyBuildDefinition(CurrentBarLayoutDefinition, true);
	}
}

void UBuildMenuWidget::ApplyDiningTableEditToTarget()
{
	if (EditingDiningTableTarget)
	{
		EditingDiningTableTarget->ApplyBuildDefinition(CurrentDiningTableDefinition, true);
	}
}

void UBuildMenuWidget::ApplyAudioEditToTarget()
{
	if (EditingAudioGroundSpeakerTarget)
	{
		EditingAudioGroundSpeakerTarget->ApplyBuildDefinition(CurrentAudioPlacementDefinition.GroundSpeakerDefinition, true);
		EditingAudioGroundSpeakerTarget->SetAudioSource(CurrentAudioSource, true);
	}
	else if (EditingAudioGroundLineArrayTarget)
	{
		EditingAudioGroundLineArrayTarget->ApplyBuildDefinition(CurrentAudioPlacementDefinition.GroundLineArrayDefinition, true);
		EditingAudioGroundLineArrayTarget->SetAudioSource(CurrentAudioSource, true);
	}
}

void UBuildMenuWidget::RefreshAudioSourceOptions()
{
	AudioSourceOptions.Reset();
	AudioSourcePathsByOption.Reset();
	AudioSourceOptions.Add(NoAudioOption);

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> Assets;
	AssetRegistryModule.Get().GetAssetsByPath(*FString(SoundCheckAssetPath), Assets, false);
	Assets.Sort([](const FAssetData& Left, const FAssetData& Right)
	{
		return Left.AssetName.LexicalLess(Right.AssetName);
	});

	for (const FAssetData& Asset : Assets)
	{
		if (!Cast<USoundBase>(Asset.GetAsset()))
		{
			continue;
		}

		const FString Option = Asset.AssetName.ToString();
		AudioSourceOptions.Add(Option);
		AudioSourcePathsByOption.Add(Option, Asset.ToSoftObjectPath());
	}
}

FString UBuildMenuWidget::AudioSourceToOption(const TSoftObjectPtr<USoundBase>& AudioSource) const
{
	if (AudioSource.IsNull())
	{
		return NoAudioOption;
	}

	const FSoftObjectPath SourcePath = AudioSource.ToSoftObjectPath();
	for (const TPair<FString, FSoftObjectPath>& Pair : AudioSourcePathsByOption)
	{
		if (Pair.Value == SourcePath)
		{
			return Pair.Key;
		}
	}

	return AudioSource.GetAssetName();
}

TSoftObjectPtr<USoundBase> UBuildMenuWidget::OptionToAudioSource(const FString& Option) const
{
	if (Option == NoAudioOption)
	{
		return nullptr;
	}

	if (const FSoftObjectPath* AssetPath = AudioSourcePathsByOption.Find(Option))
	{
		return TSoftObjectPtr<USoundBase>(*AssetPath);
	}

	return nullptr;
}

void UBuildMenuWidget::ApplyDrapeEditToTarget()
{
	if (EditingDrapeTarget)
	{
		EditingDrapeTarget->ApplyBuildDefinition(CurrentDrapeRunDefinition, true);
	}
}

void UBuildMenuWidget::ApplyVideoEditToTarget()
{
	if (EditingVideoTarget)
	{
		EditingVideoTarget->ApplyBuildDefinition(CurrentVideoPlacementDefinition, true);
	}
}

void UBuildMenuWidget::ApplyVideoWallEditToTarget()
{
	if (EditingVideoWallTarget)
	{
		EditingVideoWallTarget->ApplyBuildDefinition(CurrentVideoWallDefinition, true);
	}
}

void UBuildMenuWidget::ApplyProjectionEditToTarget()
{
	if (EditingProjectionTarget)
	{
		EditingProjectionTarget->ApplyBuildDefinition(CurrentProjectionScreenDefinition, true);
	}
}

void UBuildMenuWidget::ApplyMBPEditToTarget()
{
	if (!EditingMBPTarget)
	{
		return;
	}

	switch (CurrentMBPEditScope)
	{
	case EMBPRuntimeEditScope::WholeWall:
		EditingMBPTarget->ApplyWallDefinition(CurrentMBPWallDefinition, true);
		break;
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

void UBuildMenuWidget::ApplyStageEditToTarget()
{
	if (!EditingStageTarget)
	{
		return;
	}

	if (CurrentStageEditScope == EStageRuntimeEditScope::WholeStage)
	{
		EditingStageTarget->ApplyBuildDefinition(CurrentStageDeckDefinition, true);
		CurrentStageEditTargetRow = FMath::Clamp(CurrentStageEditTargetRow, 0, FMath::Max(EditingStageTarget->Rows - 1, 0));
		CurrentStageEditTargetColumn = FMath::Clamp(CurrentStageEditTargetColumn, 0, FMath::Max(EditingStageTarget->Columns - 1, 0));
		SyncCurrentStageCellFromTarget();
		return;
	}

	FStageDeckCell CellDefinition;
	CellDefinition.bEnabled = bCurrentStageCellEnabled;
	CellDefinition.HeightPreset = CurrentStageCellHeightPreset;
	CellDefinition.SurfaceStyle = CurrentStageCellSurfaceStyle;
	EditingStageTarget->ApplyCellDefinition(CurrentStageEditTargetRow, CurrentStageEditTargetColumn, CellDefinition, true);
}

void UBuildMenuWidget::SyncCurrentStageCellFromTarget()
{
	if (!EditingStageTarget)
	{
		return;
	}

	FStageDeckCell CellDefinition;
	if (EditingStageTarget->GetCellDefinition(CurrentStageEditTargetRow, CurrentStageEditTargetColumn, CellDefinition))
	{
		bCurrentStageCellEnabled = CellDefinition.bEnabled;
		CurrentStageCellHeightPreset = CellDefinition.HeightPreset;
		CurrentStageCellSurfaceStyle = CellDefinition.SurfaceStyle;
	}
}

bool UBuildMenuWidget::ItemBelongsToActiveTab(const UBuildItemDataAsset* BuildItem) const
{
	if (!BuildItem)
	{
		return false;
	}

	if (bUsesDesignerWidgetTree)
	{
		if (ActiveMenuTab == EBuildItemType::StageDeck)
		{
			return BuildItem->ItemType == EBuildItemType::StageDeck ||
				BuildItem->ItemType == EBuildItemType::MBPWall ||
				BuildItem->ItemType == EBuildItemType::DrapeRun;
		}

		if (ActiveMenuTab == EBuildItemType::VideoPlacement)
		{
			return BuildItem->ItemType == EBuildItemType::VideoPlacement ||
				BuildItem->ItemType == EBuildItemType::VideoWall ||
				BuildItem->ItemType == EBuildItemType::ProjectionScreen ||
				BuildItem->ItemType == EBuildItemType::AudioPlacement;
		}

		if (ActiveMenuTab == EBuildItemType::LoungeLayout)
		{
			return BuildItem->ItemType == EBuildItemType::LoungeLayout ||
				BuildItem->ItemType == EBuildItemType::SectionalLayout ||
				BuildItem->ItemType == EBuildItemType::FurnitureSeating ||
				BuildItem->ItemType == EBuildItemType::BarLayout ||
				BuildItem->ItemType == EBuildItemType::DiningTable;
		}
	}

	if (ActiveMenuTab == EBuildItemType::MBPWall)
	{
		return BuildItem->ItemType == EBuildItemType::MBPWall;
	}

	if (ActiveMenuTab == EBuildItemType::StageDeck)
	{
		return BuildItem->ItemType == EBuildItemType::StageDeck;
	}

	if (ActiveMenuTab == EBuildItemType::DrapeRun)
	{
		return BuildItem->ItemType == EBuildItemType::DrapeRun;
	}

	if (ActiveMenuTab == EBuildItemType::VideoPlacement)
	{
		return BuildItem->ItemType == EBuildItemType::VideoPlacement;
	}

	if (ActiveMenuTab == EBuildItemType::VideoWall)
	{
		return BuildItem->ItemType == EBuildItemType::VideoWall;
	}

	if (ActiveMenuTab == EBuildItemType::ProjectionScreen)
	{
		return BuildItem->ItemType == EBuildItemType::ProjectionScreen;
	}

	if (ActiveMenuTab == EBuildItemType::AudioPlacement)
	{
		return BuildItem->ItemType == EBuildItemType::AudioPlacement;
	}

	if (ActiveMenuTab == EBuildItemType::LoungeLayout)
	{
		return BuildItem->ItemType == EBuildItemType::LoungeLayout ||
			BuildItem->ItemType == EBuildItemType::SectionalLayout ||
			BuildItem->ItemType == EBuildItemType::DiningTable;
	}

	if (ActiveMenuTab == EBuildItemType::FurnitureSeating)
	{
		return BuildItem->ItemType == EBuildItemType::FurnitureSeating;
	}

	if (ActiveMenuTab == EBuildItemType::BarLayout)
	{
		return BuildItem->ItemType == EBuildItemType::BarLayout;
	}

	if (ActiveMenuTab == EBuildItemType::DiningTable)
	{
		return BuildItem->ItemType == EBuildItemType::DiningTable;
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
	case ETrussBuildMode::Tower:
		return TEXT("Tower");
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
	if (Option == TEXT("Tower"))
	{
		return ETrussBuildMode::Tower;
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

FString UBuildMenuWidget::StageHeightPresetToOption(EStageDeckHeightPreset HeightPreset)
{
	switch (HeightPreset)
	{
	case EStageDeckHeightPreset::In8:
		return TEXT("8 Inch");
	case EStageDeckHeightPreset::In12:
		return TEXT("12 Inch");
	case EStageDeckHeightPreset::In27:
		return TEXT("27 Inch");
	case EStageDeckHeightPreset::In24:
	default:
		return TEXT("24 Inch");
	}
}

EStageDeckHeightPreset UBuildMenuWidget::OptionToStageHeightPreset(const FString& Option)
{
	if (Option == TEXT("8 Inch"))
	{
		return EStageDeckHeightPreset::In8;
	}
	if (Option == TEXT("12 Inch"))
	{
		return EStageDeckHeightPreset::In12;
	}
	if (Option == TEXT("27 Inch"))
	{
		return EStageDeckHeightPreset::In27;
	}
	return EStageDeckHeightPreset::In24;
}

FString UBuildMenuWidget::StageSurfaceStyleToOption(EStageDeckSurfaceStyle SurfaceStyle)
{
	switch (SurfaceStyle)
	{
	case EStageDeckSurfaceStyle::GrayCarpet:
		return TEXT("Gray Carpet");
	case EStageDeckSurfaceStyle::BlackTop:
	default:
		return TEXT("Black Top");
	}
}

EStageDeckSurfaceStyle UBuildMenuWidget::OptionToStageSurfaceStyle(const FString& Option)
{
	if (Option == TEXT("Gray Carpet"))
	{
		return EStageDeckSurfaceStyle::GrayCarpet;
	}
	return EStageDeckSurfaceStyle::BlackTop;
}

FString UBuildMenuWidget::MBPEditScopeToOption(EMBPRuntimeEditScope Scope)
{
	switch (Scope)
	{
	case EMBPRuntimeEditScope::WholeWall:
		return TEXT("Whole Wall");
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
	if (Option == TEXT("Whole Wall"))
	{
		return EMBPRuntimeEditScope::WholeWall;
	}
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

FString UBuildMenuWidget::StageEditScopeToOption(EStageRuntimeEditScope Scope)
{
	switch (Scope)
	{
	case EStageRuntimeEditScope::WholeStage:
		return TEXT("Whole Stage");
	case EStageRuntimeEditScope::Cell:
	default:
		return TEXT("Cell");
	}
}

EStageRuntimeEditScope UBuildMenuWidget::OptionToStageEditScope(const FString& Option)
{
	if (Option == TEXT("Whole Stage"))
	{
		return EStageRuntimeEditScope::WholeStage;
	}

	return EStageRuntimeEditScope::Cell;
}

FString UBuildMenuWidget::TVModelToOption(EVideoTVModel TVModel)
{
	switch (TVModel)
	{
	case EVideoTVModel::Hisense58:
		return TEXT("Hisense 58 4K");
	case EVideoTVModel::Insignia43:
		return TEXT("Insignia 43 1080p");
	case EVideoTVModel::Philips46:
		return TEXT("Philips 46 Monitor");
	case EVideoTVModel::Samsung22:
		return TEXT("Samsung 22 Monitor");
	case EVideoTVModel::Samsung55Outdoor:
		return TEXT("Samsung 55 Outdoor");
	case EVideoTVModel::Samsung60:
		return TEXT("Samsung 60 HD");
	case EVideoTVModel::Samsung82Crystal:
		return TEXT("Samsung 82 Crystal");
	case EVideoTVModel::Samsung82Smart:
		return TEXT("Samsung 82 Smart");
	case EVideoTVModel::Sharp55:
		return TEXT("Sharp 55 4K");
	case EVideoTVModel::Sharp60:
		return TEXT("Sharp 60 4K");
	case EVideoTVModel::Sharp80:
		return TEXT("Sharp 80 LED");
	case EVideoTVModel::Sharp90:
		return TEXT("Sharp 90 LED");
	case EVideoTVModel::Vizio70:
		return TEXT("Vizio 70 LED");
	case EVideoTVModel::Benq25Preview:
		return TEXT("BenQ 25 Preview");
	case EVideoTVModel::Samsung58:
	default:
		return TEXT("Samsung 58 UHD");
	}
}

EVideoTVModel UBuildMenuWidget::OptionToTVModel(const FString& Option)
{
	if (Option == TEXT("Hisense 58 4K")) return EVideoTVModel::Hisense58;
	if (Option == TEXT("Insignia 43 1080p")) return EVideoTVModel::Insignia43;
	if (Option == TEXT("Philips 46 Monitor")) return EVideoTVModel::Philips46;
	if (Option == TEXT("Samsung 22 Monitor")) return EVideoTVModel::Samsung22;
	if (Option == TEXT("Samsung 55 Outdoor")) return EVideoTVModel::Samsung55Outdoor;
	if (Option == TEXT("Samsung 60 HD")) return EVideoTVModel::Samsung60;
	if (Option == TEXT("Samsung 82 Crystal")) return EVideoTVModel::Samsung82Crystal;
	if (Option == TEXT("Samsung 82 Smart")) return EVideoTVModel::Samsung82Smart;
	if (Option == TEXT("Sharp 55 4K")) return EVideoTVModel::Sharp55;
	if (Option == TEXT("Sharp 60 4K")) return EVideoTVModel::Sharp60;
	if (Option == TEXT("Sharp 80 LED")) return EVideoTVModel::Sharp80;
	if (Option == TEXT("Sharp 90 LED")) return EVideoTVModel::Sharp90;
	if (Option == TEXT("Vizio 70 LED")) return EVideoTVModel::Vizio70;
	if (Option == TEXT("BenQ 25 Preview")) return EVideoTVModel::Benq25Preview;
	return EVideoTVModel::Samsung58;
}

FString UBuildMenuWidget::VideoWallSupportModeToOption(EVideoWallSupportMode SupportMode)
{
	return SupportMode == EVideoWallSupportMode::Flown ? TEXT("Flown") : TEXT("Ground Support");
}

EVideoWallSupportMode UBuildMenuWidget::OptionToVideoWallSupportMode(const FString& Option)
{
	return Option == TEXT("Flown") ? EVideoWallSupportMode::Flown : EVideoWallSupportMode::GroundStacked;
}

FString UBuildMenuWidget::VideoWallSupportSpacingToOption(EVideoWallSupportSpacing SupportSpacing)
{
	return SupportSpacing == EVideoWallSupportSpacing::OnePointFiveMeters ? TEXT("1500 mm") : TEXT("1000 mm");
}

EVideoWallSupportSpacing UBuildMenuWidget::OptionToVideoWallSupportSpacing(const FString& Option)
{
	return Option == TEXT("1500 mm") ? EVideoWallSupportSpacing::OnePointFiveMeters : EVideoWallSupportSpacing::OneMeter;
}

FString UBuildMenuWidget::ProjectionScreenSizeToOption(EProjectionScreenKitSize ScreenKitSize)
{
	switch (ScreenKitSize)
	{
	case EProjectionScreenKitSize::Screen6x12:
		return TEXT("6 x 12");
	case EProjectionScreenKitSize::Screen8x14:
		return TEXT("8 x 14");
	case EProjectionScreenKitSize::Screen13x24:
		return TEXT("13 x 24");
	case EProjectionScreenKitSize::Screen9x16:
	default:
		return TEXT("9 x 16");
	}
}

EProjectionScreenKitSize UBuildMenuWidget::OptionToProjectionScreenSize(const FString& Option)
{
	if (Option == TEXT("6 x 12")) return EProjectionScreenKitSize::Screen6x12;
	if (Option == TEXT("8 x 14")) return EProjectionScreenKitSize::Screen8x14;
	if (Option == TEXT("13 x 24")) return EProjectionScreenKitSize::Screen13x24;
	return EProjectionScreenKitSize::Screen9x16;
}

FString UBuildMenuWidget::ProjectionProjectorToOption(EProjectionProjectorType ProjectorType)
{
	switch (ProjectorType)
	{
	case EProjectionProjectorType::ChristieM4K25RGB:
	default:
		return TEXT("Christie M 4K25 RGB");
	}
}

EProjectionProjectorType UBuildMenuWidget::OptionToProjectionProjector(const FString& Option)
{
	return EProjectionProjectorType::ChristieM4K25RGB;
}

FString UBuildMenuWidget::ProjectionLensToOption(EProjectionLensType LensType)
{
	switch (LensType)
	{
	case EProjectionLensType::ILS067HD:
		return TEXT("ILS 0.67 HD");
	case EProjectionLensType::ILS4169HD:
		return TEXT("ILS 4.1-6.9 HD");
	case EProjectionLensType::ILS116149HD:
	default:
		return TEXT("ILS 1.16-1.49 HD");
	}
}

EProjectionLensType UBuildMenuWidget::OptionToProjectionLens(const FString& Option)
{
	if (Option == TEXT("ILS 0.67 HD")) return EProjectionLensType::ILS067HD;
	if (Option == TEXT("ILS 4.1-6.9 HD")) return EProjectionLensType::ILS4169HD;
	return EProjectionLensType::ILS116149HD;
}

FString UBuildMenuWidget::AudioPlacementTypeToOption(EAudioPlacementRuntimeType PlacementType)
{
	return PlacementType == EAudioPlacementRuntimeType::GroundLineArray ? TEXT("Ground Line Array") : TEXT("Ground Speaker");
}

EAudioPlacementRuntimeType UBuildMenuWidget::OptionToAudioPlacementType(const FString& Option)
{
	return Option == TEXT("Ground Line Array") ? EAudioPlacementRuntimeType::GroundLineArray : EAudioPlacementRuntimeType::GroundSpeaker;
}

FString UBuildMenuWidget::SectionalShapeToOption(ESectionalLayoutShape Shape)
{
	switch (Shape)
	{
	case ESectionalLayoutShape::OneSided:
		return TEXT("One Sided");
	case ESectionalLayoutShape::ThreeSidedU:
		return TEXT("Three Sided U");
	case ESectionalLayoutShape::FourSidedBox:
		return TEXT("Four Sided Box");
	case ESectionalLayoutShape::TwoSidedL:
	default:
		return TEXT("Two Sided L");
	}
}

ESectionalLayoutShape UBuildMenuWidget::OptionToSectionalShape(const FString& Option)
{
	if (Option == TEXT("One Sided")) return ESectionalLayoutShape::OneSided;
	if (Option == TEXT("Three Sided U")) return ESectionalLayoutShape::ThreeSidedU;
	if (Option == TEXT("Four Sided Box")) return ESectionalLayoutShape::FourSidedBox;
	return ESectionalLayoutShape::TwoSidedL;
}

FString UBuildMenuWidget::FurnitureSeatingTypeToOption(EFurnitureSeatingRuntimeType SeatingType)
{
	return SeatingType == EFurnitureSeatingRuntimeType::CafeTable ? TEXT("Cafe Tables") : TEXT("High Seating");
}

EFurnitureSeatingRuntimeType UBuildMenuWidget::OptionToFurnitureSeatingType(const FString& Option)
{
	return Option == TEXT("Cafe Tables") || Option == TEXT("Cafe Seating") ? EFurnitureSeatingRuntimeType::CafeTable : EFurnitureSeatingRuntimeType::BarSeating;
}

FString UBuildMenuWidget::BarLayoutShapeToOption(EBarLayoutShape Shape)
{
	switch (Shape)
	{
	case EBarLayoutShape::WallLine:
		return TEXT("Wall Line");
	case EBarLayoutShape::CenterTwoSided:
		return TEXT("Center Two Sided");
	case EBarLayoutShape::CenterFourSided:
		return TEXT("Center Four Sided");
	case EBarLayoutShape::CenterSixSided:
		return TEXT("Center Six Sided");
	case EBarLayoutShape::Single:
	default:
		return TEXT("Single");
	}
}

EBarLayoutShape UBuildMenuWidget::OptionToBarLayoutShape(const FString& Option)
{
	if (Option == TEXT("Wall Line")) return EBarLayoutShape::WallLine;
	if (Option == TEXT("Center Two Sided")) return EBarLayoutShape::CenterTwoSided;
	if (Option == TEXT("Center Four Sided")) return EBarLayoutShape::CenterFourSided;
	if (Option == TEXT("Center Six Sided")) return EBarLayoutShape::CenterSixSided;
	return EBarLayoutShape::Single;
}

FString UBuildMenuWidget::BarSeatingLayoutToOption(EBarSeatingStoolLayout Layout)
{
	switch (Layout)
	{
	case EBarSeatingStoolLayout::NoStools:
		return TEXT("No Stools");
	case EBarSeatingStoolLayout::TwoTop:
		return TEXT("Square/Round Two Top");
	case EBarSeatingStoolLayout::RectangleFourTop:
		return TEXT("Rectangle Four Top");
	case EBarSeatingStoolLayout::RectangleSixTop:
		return TEXT("Rectangle Six Top");
	case EBarSeatingStoolLayout::FourTop:
	default:
		return TEXT("Square/Round Four Top");
	}
}

EBarSeatingStoolLayout UBuildMenuWidget::OptionToBarSeatingLayout(const FString& Option)
{
	if (Option == TEXT("No Stools")) return EBarSeatingStoolLayout::NoStools;
	if (Option == TEXT("Square/Round Two Top")) return EBarSeatingStoolLayout::TwoTop;
	if (Option == TEXT("Rectangle Four Top")) return EBarSeatingStoolLayout::RectangleFourTop;
	if (Option == TEXT("Rectangle Six Top")) return EBarSeatingStoolLayout::RectangleSixTop;
	return EBarSeatingStoolLayout::FourTop;
}

FString UBuildMenuWidget::CafeTableLayoutToOption(ECafeTableChairLayout Layout)
{
	switch (Layout)
	{
	case ECafeTableChairLayout::NoChairs:
		return TEXT("No Chairs");
	case ECafeTableChairLayout::TwoTop:
		return TEXT("Square/Round Two Top");
	case ECafeTableChairLayout::RectangleFourTop:
		return TEXT("Rectangle Four Top");
	case ECafeTableChairLayout::RectangleSixTop:
		return TEXT("Rectangle Six Top");
	case ECafeTableChairLayout::FourTop:
	default:
		return TEXT("Square/Round Four Top");
	}
}

ECafeTableChairLayout UBuildMenuWidget::OptionToCafeTableLayout(const FString& Option)
{
	if (Option == TEXT("No Chairs")) return ECafeTableChairLayout::NoChairs;
	if (Option == TEXT("Square/Round Two Top")) return ECafeTableChairLayout::TwoTop;
	if (Option == TEXT("Rectangle Four Top")) return ECafeTableChairLayout::RectangleFourTop;
	if (Option == TEXT("Rectangle Six Top")) return ECafeTableChairLayout::RectangleSixTop;
	return ECafeTableChairLayout::FourTop;
}

FString UBuildMenuWidget::DiningChairCountToOption(EDiningChairCount ChairCount)
{
	switch (ChairCount)
	{
	case EDiningChairCount::Two:
		return TEXT("2-Top");
	case EDiningChairCount::Six:
		return TEXT("6-Top");
	case EDiningChairCount::Eight:
		return TEXT("8-Top");
	case EDiningChairCount::Four:
	default:
		return TEXT("4-Top");
	}
}

EDiningChairCount UBuildMenuWidget::OptionToDiningChairCount(const FString& Option)
{
	if (Option == TEXT("2-Top")) return EDiningChairCount::Two;
	if (Option == TEXT("6-Top")) return EDiningChairCount::Six;
	if (Option == TEXT("8-Top")) return EDiningChairCount::Eight;
	return EDiningChairCount::Four;
}

FString UBuildMenuWidget::DiningChairArrangementToOption(EDiningChairArrangement Arrangement)
{
	return Arrangement == EDiningChairArrangement::Radial ? TEXT("Radial") : TEXT("Rect");
}

EDiningChairArrangement UBuildMenuWidget::OptionToDiningChairArrangement(const FString& Option)
{
	return Option == TEXT("Radial") ? EDiningChairArrangement::Radial : EDiningChairArrangement::Rectangular;
}

FString UBuildMenuWidget::AudioSpeakerModelToOption(EAudioGroundSpeakerModel SpeakerModel)
{
	switch (SpeakerModel)
	{
	case EAudioGroundSpeakerModel::QSC_K10:
		return TEXT("QSC K10");
	case EAudioGroundSpeakerModel::QSC_K12:
		return TEXT("QSC K12");
	case EAudioGroundSpeakerModel::QSC_KW153:
		return TEXT("QSC KW153");
	case EAudioGroundSpeakerModel::CDD_LIVE_15:
		return TEXT("CDD-LIVE 15");
	case EAudioGroundSpeakerModel::QSC_K8:
	default:
		return TEXT("QSC K8");
	}
}

EAudioGroundSpeakerModel UBuildMenuWidget::OptionToAudioSpeakerModel(const FString& Option)
{
	if (Option == TEXT("QSC K10")) return EAudioGroundSpeakerModel::QSC_K10;
	if (Option == TEXT("QSC K12")) return EAudioGroundSpeakerModel::QSC_K12;
	if (Option == TEXT("QSC KW153")) return EAudioGroundSpeakerModel::QSC_KW153;
	if (Option == TEXT("CDD-LIVE 15")) return EAudioGroundSpeakerModel::CDD_LIVE_15;
	return EAudioGroundSpeakerModel::QSC_K8;
}

FString UBuildMenuWidget::AudioLineArrayModelToOption(EAudioGroundLineArrayModel LineArrayModel)
{
	return LineArrayModel == EAudioGroundLineArrayModel::MLA_Compact ? TEXT("MLA Compact") : TEXT("MLA Mini");
}

EAudioGroundLineArrayModel UBuildMenuWidget::OptionToAudioLineArrayModel(const FString& Option)
{
	return Option == TEXT("MLA Compact") ? EAudioGroundLineArrayModel::MLA_Compact : EAudioGroundLineArrayModel::MLA_Mini;
}

void UBuildMenuWidget::GetProjectionScreenDimensionsFt(EProjectionScreenKitSize ScreenKitSize, float& OutWidthFt, float& OutHeightFt)
{
	switch (ScreenKitSize)
	{
	case EProjectionScreenKitSize::Screen6x12:
		OutWidthFt = 12.0f;
		OutHeightFt = 6.0f;
		return;
	case EProjectionScreenKitSize::Screen8x14:
		OutWidthFt = 14.0f;
		OutHeightFt = 8.0f;
		return;
	case EProjectionScreenKitSize::Screen13x24:
		OutWidthFt = 24.0f;
		OutHeightFt = 13.0f;
		return;
	case EProjectionScreenKitSize::Screen9x16:
	default:
		OutWidthFt = 16.0f;
		OutHeightFt = 9.0f;
		return;
	}
}

void UBuildMenuWidget::GetProjectionLensShiftPercent(EProjectionLensType LensType, float& OutVerticalMinPercent, float& OutVerticalMaxPercent, float& OutHorizontalMinPercent, float& OutHorizontalMaxPercent, bool& bOutHasData)
{
	bOutHasData = true;
	switch (LensType)
	{
	case EProjectionLensType::ILS067HD:
		OutVerticalMinPercent = -32.0f;
		OutVerticalMaxPercent = 48.0f;
		OutHorizontalMinPercent = -8.0f;
		OutHorizontalMaxPercent = 19.0f;
		return;
	case EProjectionLensType::ILS116149HD:
		OutVerticalMinPercent = -128.0f;
		OutVerticalMaxPercent = 133.0f;
		OutHorizontalMinPercent = -73.0f;
		OutHorizontalMaxPercent = 73.0f;
		return;
	case EProjectionLensType::ILS4169HD:
	default:
		OutVerticalMinPercent = 0.0f;
		OutVerticalMaxPercent = 0.0f;
		OutHorizontalMinPercent = 0.0f;
		OutHorizontalMaxPercent = 0.0f;
		bOutHasData = false;
		return;
	}
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

void UBuildMenuWidget::HandleStageTabClicked()
{
	ActiveMenuTab = EBuildItemType::StageDeck;

	if (bUsesDesignerWidgetTree)
	{
		for (UBuildItemDataAsset* BuildItem : BuildItems)
		{
			if (BuildItem && BuildItem->ItemType == EBuildItemType::StageDeck)
			{
				SetSelectedBuildItem(BuildItem);
				return;
			}
		}
	}

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

void UBuildMenuWidget::HandleDrapeTabClicked()
{
	ActiveMenuTab = EBuildItemType::DrapeRun;

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

void UBuildMenuWidget::HandleVideoTabClicked()
{
	ActiveMenuTab = EBuildItemType::VideoPlacement;

	if (bUsesDesignerWidgetTree)
	{
		for (UBuildItemDataAsset* BuildItem : BuildItems)
		{
			if (BuildItem && BuildItem->ItemType == EBuildItemType::VideoPlacement)
			{
				SetSelectedBuildItem(BuildItem);
				return;
			}
		}
	}

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

void UBuildMenuWidget::HandleVideoWallTabClicked()
{
	ActiveMenuTab = EBuildItemType::VideoWall;

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

void UBuildMenuWidget::HandleProjectionTabClicked()
{
	ActiveMenuTab = EBuildItemType::ProjectionScreen;

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

void UBuildMenuWidget::HandleAudioTabClicked()
{
	ActiveMenuTab = EBuildItemType::AudioPlacement;

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

void UBuildMenuWidget::HandleLoungeTabClicked()
{
	ActiveMenuTab = EBuildItemType::LoungeLayout;

	if (bUsesDesignerWidgetTree)
	{
		for (UBuildItemDataAsset* BuildItem : BuildItems)
		{
			if (BuildItem && BuildItem->ItemType == EBuildItemType::LoungeLayout)
			{
				SetSelectedBuildItem(BuildItem);
				return;
			}
		}
	}

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

void UBuildMenuWidget::HandleSeatingTabClicked()
{
	ActiveMenuTab = EBuildItemType::FurnitureSeating;

	if (!SelectedBuildItem || !ItemBelongsToActiveTab(SelectedBuildItem))
	{
		for (UBuildItemDataAsset* BuildItem : BuildItems)
		{
			if (ItemBelongsToActiveTab(BuildItem))
			{
				SetSelectedBuildItem(BuildItem);
				OnBuildItemSelected.Broadcast(BuildItem);
				return;
			}
		}
	}

	RefreshMenu();
}

void UBuildMenuWidget::HandleBarTabClicked()
{
	ActiveMenuTab = EBuildItemType::BarLayout;
	if (!SelectedBuildItem || !ItemBelongsToActiveTab(SelectedBuildItem))
	{
		for (UBuildItemDataAsset* BuildItem : BuildItems)
		{
			if (BuildItem && ItemBelongsToActiveTab(BuildItem))
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
	if (EditingLightTarget)
	{
		const int32 FixtureIndex = LightingFixtureNames.IndexOfByKey(SelectedItemOption);
		if (LightingFixtureClasses.IsValidIndex(FixtureIndex) && LightingFixtureClasses[FixtureIndex])
		{
			EditingLightTarget->FixtureClass = LightingFixtureClasses[FixtureIndex].Get();
			EditingLightTarget->MountDefinition.FixtureClass = LightingFixtureClasses[FixtureIndex];
			EditingLightTarget->RebuildFixture();
			RefreshMenu();
		}
		return;
	}

	if (IsEditingMBP())
	{
		CurrentMBPEditScope = OptionToMBPEditScope(SelectedItemOption);
		if (CurrentMBPEditScope == EMBPRuntimeEditScope::WholeWall)
		{
			CurrentMBPWallDefinition = EditingMBPTarget ? EditingMBPTarget->GetWallDefinition() : CurrentMBPWallDefinition;
		}
		ApplyMBPEditToTarget();
		RefreshMenu();
		return;
	}

	if (IsEditingStage())
	{
		CurrentStageEditScope = OptionToStageEditScope(SelectedItemOption);
		if (CurrentStageEditScope == EStageRuntimeEditScope::Cell)
		{
			SyncCurrentStageCellFromTarget();
		}
		RefreshMenu();
		return;
	}

	if (ActiveMenuTab == EBuildItemType::VideoPlacement)
	{
		CurrentVideoPlacementDefinition.TVModel = OptionToTVModel(SelectedItemOption);
		if (IsEditingVideo())
		{
			ApplyVideoEditToTarget();
		}
		else
		{
			ApplyVideoDefinitionToBuildManager();
		}
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::VideoWall)
	{
		CurrentVideoWallDefinition.SupportMode = OptionToVideoWallSupportMode(SelectedItemOption);
		if (IsEditingVideoWall())
		{
			ApplyVideoWallEditToTarget();
		}
		else
		{
			ApplyVideoWallDefinitionToBuildManager();
		}
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::ProjectionScreen)
	{
		CurrentProjectionScreenDefinition.ScreenKitSize = OptionToProjectionScreenSize(SelectedItemOption);
		if (IsEditingProjection())
		{
			ApplyProjectionEditToTarget();
		}
		else
		{
			ApplyProjectionDefinitionToBuildManager();
		}
		RefreshMenu();
		return;
	}

	if (ActiveMenuTab == EBuildItemType::AudioPlacement)
	{
		CurrentAudioPlacementDefinition.PlacementType = OptionToAudioPlacementType(SelectedItemOption);
		if (IsEditingAudio())
		{
			ApplyAudioEditToTarget();
		}
		else
		{
			ApplyAudioDefinitionToBuildManager();
		}
		RefreshMenu();
		return;
	}

	if (ActiveMenuTab == EBuildItemType::LoungeLayout && (!SelectedBuildItem || SelectedBuildItem->ItemType != EBuildItemType::DiningTable))
	{
		if (SelectedBuildItem && SelectedBuildItem->ItemType == EBuildItemType::SectionalLayout)
		{
			CurrentSectionalLayoutDefinition.Shape = OptionToSectionalShape(SelectedItemOption);
			ApplySectionalDefinitionToBuildManager();
			if (DetailText)
			{
				DetailText->SetText(BuildDetailText());
			}
			return;
		}

		CurrentLoungeLayoutDefinition.SofaItem = SelectedItemOption;
		ApplyLoungeDefinitionToBuildManager();
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::FurnitureSeating)
	{
		if (IsEditingFurnitureSeating())
		{
			CurrentFurnitureSeatingDefinition.SeatingType = EditingCafeTableTarget
				? EFurnitureSeatingRuntimeType::CafeTable
				: EFurnitureSeatingRuntimeType::BarSeating;
			RefreshMenu();
			return;
		}

		CurrentFurnitureSeatingDefinition.SeatingType = OptionToFurnitureSeatingType(SelectedItemOption);
		ApplyFurnitureSeatingDefinitionToBuildManager();
		RefreshMenu();
		return;
	}

	if (ActiveMenuTab == EBuildItemType::BarLayout)
	{
		CurrentBarLayoutDefinition.LayoutShape = OptionToBarLayoutShape(SelectedItemOption);
		ApplyBarLayoutDefinitionToBuildManager();
		RefreshMenu();
		return;
	}

	if (ActiveMenuTab == EBuildItemType::DiningTable || (SelectedBuildItem && SelectedBuildItem->ItemType == EBuildItemType::DiningTable))
	{
		CurrentDiningTableDefinition.ChairCount = OptionToDiningChairCount(SelectedItemOption);
		ApplyDiningTableDefinitionToBuildManager();
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
	if (EditingLightTarget)
	{
		ApplyLightingControlValue(0, NewValue);
		return;
	}

	if (IsEditingMBP())
	{
		if (CurrentMBPEditScope == EMBPRuntimeEditScope::WholeWall)
		{
			CurrentMBPWallDefinition.Rows = FMath::Max(1, FMath::RoundToInt(NewValue));
		}
		else
		{
			CurrentMBPEditTargetRow = FMath::Max(0, FMath::RoundToInt(NewValue) - 1);
		}
		ApplyMBPEditToTarget();
		if (CurrentMBPEditScope == EMBPRuntimeEditScope::WholeWall)
		{
			RefreshMenu();
			return;
		}
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::StageDeck)
	{
		if (IsEditingStage() && CurrentStageEditScope == EStageRuntimeEditScope::Cell)
		{
			CurrentStageEditTargetRow = FMath::Max(0, FMath::RoundToInt(NewValue) - 1);
			SyncCurrentStageCellFromTarget();
			RefreshMenu();
			return;
		}
		else
		{
			CurrentStageDeckDefinition.Columns = FMath::Max(1, FMath::RoundToInt(NewValue));
			if (IsEditingStage())
			{
				ApplyStageEditToTarget();
			}
			else
			{
				ApplyStageDefinitionToBuildManager();
			}
		}
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::DrapeRun)
	{
		CurrentDrapeRunDefinition.LengthFt = FMath::Max(NewValue, 1.0f);
		if (IsEditingDrape())
		{
			ApplyDrapeEditToTarget();
		}
		else
		{
			ApplyDrapeDefinitionToBuildManager();
		}
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::VideoPlacement)
	{
		CurrentVideoPlacementDefinition.TVCenterHeightFt = FMath::Max(NewValue, 1.0f);
		if (IsEditingVideo())
		{
			ApplyVideoEditToTarget();
		}
		else
		{
			ApplyVideoDefinitionToBuildManager();
		}
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::VideoWall)
	{
		CurrentVideoWallDefinition.Rows = FMath::Max(1, FMath::RoundToInt(NewValue));
		if (IsEditingVideoWall())
		{
			ApplyVideoWallEditToTarget();
		}
		else
		{
			ApplyVideoWallDefinitionToBuildManager();
		}
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::ProjectionScreen)
	{
		float ScreenWidthFt = 0.0f;
		float ScreenHeightFt = 0.0f;
		GetProjectionScreenDimensionsFt(CurrentProjectionScreenDefinition.ScreenKitSize, ScreenWidthFt, ScreenHeightFt);

		float VerticalMinPercent = 0.0f;
		float VerticalMaxPercent = 0.0f;
		float HorizontalMinPercent = 0.0f;
		float HorizontalMaxPercent = 0.0f;
		bool bHasShiftData = false;
		GetProjectionLensShiftPercent(CurrentProjectionScreenDefinition.LensType, VerticalMinPercent, VerticalMaxPercent, HorizontalMinPercent, HorizontalMaxPercent, bHasShiftData);

		const float MinOffsetFt = bHasShiftData ? ScreenWidthFt * (HorizontalMinPercent / 100.0f) : 0.0f;
		const float MaxOffsetFt = bHasShiftData ? ScreenWidthFt * (HorizontalMaxPercent / 100.0f) : 0.0f;
		CurrentProjectionScreenDefinition.ProjectorHorizontalOffsetFt = FMath::Clamp(NewValue, MinOffsetFt, MaxOffsetFt);
		if (IsEditingProjection())
		{
			ApplyProjectionEditToTarget();
		}
		else
		{
			ApplyProjectionDefinitionToBuildManager();
		}
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::AudioPlacement)
	{
		if (CurrentAudioPlacementDefinition.PlacementType == EAudioPlacementRuntimeType::GroundLineArray)
		{
			const int32 MaxSpeakers = CurrentAudioPlacementDefinition.GroundLineArrayDefinition.LineArrayModel == EAudioGroundLineArrayModel::MLA_Compact ? 6 : 12;
			CurrentAudioPlacementDefinition.GroundLineArrayDefinition.SpeakerCount = FMath::Clamp(FMath::RoundToInt(NewValue), 1, MaxSpeakers);
		}
		else
		{
			CurrentAudioPlacementDefinition.GroundSpeakerDefinition.StandExtensionFt = FMath::Clamp(NewValue, 0.0f, 3.0f);
		}
		if (IsEditingAudio())
		{
			ApplyAudioEditToTarget();
		}
		else
		{
			ApplyAudioDefinitionToBuildManager();
		}
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::LoungeLayout && (!SelectedBuildItem || SelectedBuildItem->ItemType != EBuildItemType::DiningTable))
	{
		if (SelectedBuildItem && SelectedBuildItem->ItemType == EBuildItemType::SectionalLayout)
		{
			CurrentSectionalLayoutDefinition.WidthSections = FMath::Clamp(FMath::RoundToInt(NewValue), 1, 20);
			ApplySectionalDefinitionToBuildManager();
			if (DetailText)
			{
				DetailText->SetText(BuildDetailText());
			}
			return;
		}

		CurrentLoungeLayoutDefinition.SofaCount = FMath::Clamp(FMath::RoundToInt(NewValue), 0, 8);
		ApplyLoungeDefinitionToBuildManager();
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::FurnitureSeating)
	{
		if (CurrentFurnitureSeatingDefinition.SeatingType == EFurnitureSeatingRuntimeType::CafeTable)
		{
			CurrentFurnitureSeatingDefinition.CafeTableDefinition.ChairOffsetYCm = FMath::Clamp(NewValue, -500.0f, 500.0f);
		}
		else
		{
			CurrentFurnitureSeatingDefinition.BarSeatingDefinition.StoolOffsetYCm = FMath::Clamp(NewValue, 0.0f, 500.0f);
		}
		ApplyFurnitureSeatingDefinitionToBuildManager();
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::BarLayout)
	{
		CurrentBarLayoutDefinition.WallBarCount = FMath::Clamp(FMath::RoundToInt(NewValue), 1, 24);
		ApplyBarLayoutDefinitionToBuildManager();
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::DiningTable || (SelectedBuildItem && SelectedBuildItem->ItemType == EBuildItemType::DiningTable))
	{
		CurrentDiningTableDefinition.ChairGapCm = FMath::Clamp(NewValue, -200.0f, 80.0f);
		ApplyDiningTableDefinitionToBuildManager();
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
	case ETrussBuildMode::Tower:
		CurrentTrussDefinition.TowerHeightFt = FMath::Max(NewValue, 2.0f);
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
	if (EditingLightTarget)
	{
		ApplyLightingControlValue(1, NewValue);
		return;
	}

	if (IsDiningTableContext(ActiveMenuTab, SelectedBuildItem))
	{
		CurrentDiningTableDefinition.TableclothOverhangCm = FMath::Clamp(NewValue, 0.0f, 80.0f);
		ApplyDiningTableDefinitionToBuildManager();
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (IsEditingMBP())
	{
		if (CurrentMBPEditScope == EMBPRuntimeEditScope::WholeWall)
		{
			CurrentMBPWallDefinition.Columns = FMath::Max(1, FMath::RoundToInt(NewValue));
		}
		else
		{
			CurrentMBPEditTargetColumn = FMath::Max(0, FMath::RoundToInt(NewValue) - 1);
		}
		ApplyMBPEditToTarget();
		if (CurrentMBPEditScope == EMBPRuntimeEditScope::WholeWall)
		{
			RefreshMenu();
			return;
		}
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::StageDeck)
	{
		if (IsEditingStage() && CurrentStageEditScope == EStageRuntimeEditScope::Cell)
		{
			CurrentStageEditTargetColumn = FMath::Max(0, FMath::RoundToInt(NewValue) - 1);
			SyncCurrentStageCellFromTarget();
			RefreshMenu();
			return;
		}
		else
		{
			CurrentStageDeckDefinition.Rows = FMath::Max(1, FMath::RoundToInt(NewValue));
			if (IsEditingStage())
			{
				ApplyStageEditToTarget();
			}
			else
			{
				ApplyStageDefinitionToBuildManager();
			}
		}
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::DrapeRun)
	{
		CurrentDrapeRunDefinition.HeightFt = FMath::Max(NewValue, 1.0f);
		if (IsEditingDrape())
		{
			ApplyDrapeEditToTarget();
		}
		else
		{
			ApplyDrapeDefinitionToBuildManager();
		}
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::VideoPlacement)
	{
		CurrentVideoPlacementDefinition.TowerHeightFt = FMath::Max(NewValue, 2.0f);
		if (IsEditingVideo())
		{
			ApplyVideoEditToTarget();
		}
		else
		{
			ApplyVideoDefinitionToBuildManager();
		}
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::VideoWall)
	{
		CurrentVideoWallDefinition.Columns = FMath::Max(1, FMath::RoundToInt(NewValue));
		if (IsEditingVideoWall())
		{
			ApplyVideoWallEditToTarget();
		}
		else
		{
			ApplyVideoWallDefinitionToBuildManager();
		}
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::ProjectionScreen)
	{
		float ScreenWidthFt = 0.0f;
		float ScreenHeightFt = 0.0f;
		GetProjectionScreenDimensionsFt(CurrentProjectionScreenDefinition.ScreenKitSize, ScreenWidthFt, ScreenHeightFt);

		float VerticalMinPercent = 0.0f;
		float VerticalMaxPercent = 0.0f;
		float HorizontalMinPercent = 0.0f;
		float HorizontalMaxPercent = 0.0f;
		bool bHasShiftData = false;
		GetProjectionLensShiftPercent(CurrentProjectionScreenDefinition.LensType, VerticalMinPercent, VerticalMaxPercent, HorizontalMinPercent, HorizontalMaxPercent, bHasShiftData);

		const float MinOffsetFt = bHasShiftData ? ScreenHeightFt * (VerticalMinPercent / 100.0f) : 0.0f;
		const float MaxOffsetFt = bHasShiftData ? ScreenHeightFt * (VerticalMaxPercent / 100.0f) : 0.0f;
		CurrentProjectionScreenDefinition.TowerHeightFt = FMath::Clamp(NewValue, MinOffsetFt, MaxOffsetFt);
		if (IsEditingProjection())
		{
			ApplyProjectionEditToTarget();
		}
		else
		{
			ApplyProjectionDefinitionToBuildManager();
		}
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::AudioPlacement)
	{
		if (CurrentAudioPlacementDefinition.PlacementType == EAudioPlacementRuntimeType::GroundLineArray)
		{
			CurrentAudioPlacementDefinition.GroundLineArrayDefinition.PoleExtensionInches = FMath::Clamp(NewValue, 0.0f, 17.0f);
			if (IsEditingAudio())
			{
				ApplyAudioEditToTarget();
			}
			else
			{
				ApplyAudioDefinitionToBuildManager();
			}
			if (DetailText)
			{
				DetailText->SetText(BuildDetailText());
			}
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::LoungeLayout && (!SelectedBuildItem || SelectedBuildItem->ItemType != EBuildItemType::DiningTable))
	{
		if (SelectedBuildItem && SelectedBuildItem->ItemType == EBuildItemType::SectionalLayout)
		{
			CurrentSectionalLayoutDefinition.DepthSections = FMath::Clamp(FMath::RoundToInt(NewValue), 1, 20);
			ApplySectionalDefinitionToBuildManager();
			if (DetailText)
			{
				DetailText->SetText(BuildDetailText());
			}
			return;
		}

		CurrentLoungeLayoutDefinition.ChairCount = FMath::Clamp(FMath::RoundToInt(NewValue), 0, 8);
		ApplyLoungeDefinitionToBuildManager();
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::FurnitureSeating)
	{
		if (CurrentFurnitureSeatingDefinition.SeatingType == EFurnitureSeatingRuntimeType::CafeTable)
		{
			CurrentFurnitureSeatingDefinition.CafeTableDefinition.RectangleChairPairOffsetXCm = FMath::Clamp(NewValue, 0.0f, 1000.0f);
		}
		else
		{
			CurrentFurnitureSeatingDefinition.BarSeatingDefinition.RectangleStoolPairOffsetXCm = FMath::Clamp(NewValue, 0.0f, 1000.0f);
		}
		ApplyFurnitureSeatingDefinitionToBuildManager();
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::BarLayout)
	{
		CurrentBarLayoutDefinition.WallBarSpacingCm = FMath::Clamp(NewValue, -1000.0f, 1000.0f);
		ApplyBarLayoutDefinitionToBuildManager();
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
	if (EditingLightTarget)
	{
		ApplyLightingControlValue(2, NewValue);
		return;
	}

	if (IsDiningTableContext(ActiveMenuTab, SelectedBuildItem))
	{
		CurrentDiningTableDefinition.TableclothDropCm = FMath::Clamp(NewValue, 0.0f, 120.0f);
		ApplyDiningTableDefinitionToBuildManager();
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (IsEditingMBP())
	{
		if (CurrentMBPEditScope == EMBPRuntimeEditScope::WholeWall)
		{
			return;
		}
		CurrentMBPEditDepthOffsetCm = NewValue;
		ApplyMBPEditToTarget();
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::ProjectionScreen)
	{
		CurrentProjectionScreenDefinition.ScreenCenterOffsetCm.X = FMath::Clamp(NewValue, -500.0f, 500.0f);
		if (IsEditingProjection())
		{
			ApplyProjectionEditToTarget();
		}
		else
		{
			ApplyProjectionDefinitionToBuildManager();
		}
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::LoungeLayout && (!SelectedBuildItem || SelectedBuildItem->ItemType != EBuildItemType::DiningTable))
	{
		if (SelectedBuildItem && SelectedBuildItem->ItemType == EBuildItemType::SectionalLayout)
		{
			CurrentSectionalLayoutDefinition.SectionSpacingCm = FMath::Clamp(NewValue, -500.0f, 500.0f);
			CurrentSectionalLayoutDefinition.bUseArmlessItemDefaultSpacing = false;
			ApplySectionalDefinitionToBuildManager();
			if (DetailText)
			{
				DetailText->SetText(BuildDetailText());
			}
			return;
		}

		CurrentLoungeLayoutDefinition.EndTableOffsetXCm = FMath::Clamp(NewValue, -500.0f, 500.0f);
		ApplyLoungeDefinitionToBuildManager();
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::BarLayout)
	{
		if (CurrentBarLayoutDefinition.LayoutShape == EBarLayoutShape::CenterTwoSided)
		{
			CurrentBarLayoutDefinition.CenterTwoSidedOffsetCm = FMath::Clamp(NewValue, -1000.0f, 1000.0f);
		}
		else if (CurrentBarLayoutDefinition.LayoutShape == EBarLayoutShape::CenterFourSided)
		{
			CurrentBarLayoutDefinition.CenterFourSidedOffsetCm = FMath::Clamp(NewValue, -1000.0f, 1000.0f);
		}
		else if (CurrentBarLayoutDefinition.LayoutShape == EBarLayoutShape::CenterSixSided)
		{
			CurrentBarLayoutDefinition.CenterSixSidedOffsetCm = FMath::Clamp(NewValue, -1000.0f, 1000.0f);
		}
		ApplyBarLayoutDefinitionToBuildManager();
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
	if (EditingLightTarget)
	{
		ApplyLightingControlValue(3, NewValue);
		return;
	}

	if (ActiveMenuTab == EBuildItemType::ProjectionScreen)
	{
		CurrentProjectionScreenDefinition.ScreenCenterOffsetCm.Z = FMath::Clamp(NewValue, -500.0f, 500.0f);
		if (IsEditingProjection())
		{
			ApplyProjectionEditToTarget();
		}
		else
		{
			ApplyProjectionDefinitionToBuildManager();
		}
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::LoungeLayout)
	{
		CurrentLoungeLayoutDefinition.MirroredChairYawOffsetDegrees = FMath::Clamp(NewValue, -90.0f, 90.0f);
		ApplyLoungeDefinitionToBuildManager();
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}
}

void UBuildMenuWidget::HandleSidePieceChanged(FString SelectedItemOption, ESelectInfo::Type SelectionType)
{
	if (bRefreshingControls)
	{
		return;
	}
	if (EditingLightTarget)
	{
		if (EditingLightTarget->DMX)
		{
			for (const TPair<FString, UDMXEntityFixturePatch*>& PatchOption : GetLightingFixturePatchOptions())
			{
				if (PatchOption.Key == SelectedItemOption)
				{
					UDMXComponent* DMXComponent = EditingLightTarget->DMX;
					UDMXLibrary* CurrentLibrary = DMXComponent->FixturePatchRef.DMXLibrary.Get();
					DMXComponent->SetFixturePatch(PatchOption.Value);
					if (!PatchOption.Value)
					{
						DMXComponent->FixturePatchRef.DMXLibrary = CurrentLibrary;
					}
					EditingLightTarget->RefreshDMXBinding();
					RefreshMenu();
					break;
				}
			}
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::ProjectionScreen)
	{
		CurrentProjectionScreenDefinition.ProjectorType = OptionToProjectionProjector(SelectedItemOption);
		if (IsEditingProjection())
		{
			ApplyProjectionEditToTarget();
		}
		else
		{
			ApplyProjectionDefinitionToBuildManager();
		}
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::VideoWall)
	{
		CurrentVideoWallDefinition.SupportSpacing = OptionToVideoWallSupportSpacing(SelectedItemOption);
		if (IsEditingVideoWall())
		{
			ApplyVideoWallEditToTarget();
		}
		else
		{
			ApplyVideoWallDefinitionToBuildManager();
		}
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::AudioPlacement)
	{
		if (CurrentAudioPlacementDefinition.PlacementType == EAudioPlacementRuntimeType::GroundLineArray)
		{
			CurrentAudioPlacementDefinition.GroundLineArrayDefinition.LineArrayModel = OptionToAudioLineArrayModel(SelectedItemOption);
			const int32 MaxSpeakers = CurrentAudioPlacementDefinition.GroundLineArrayDefinition.LineArrayModel == EAudioGroundLineArrayModel::MLA_Compact ? 6 : 12;
			CurrentAudioPlacementDefinition.GroundLineArrayDefinition.SpeakerCount = FMath::Clamp(CurrentAudioPlacementDefinition.GroundLineArrayDefinition.SpeakerCount, 1, MaxSpeakers);
		}
		else
		{
			CurrentAudioPlacementDefinition.GroundSpeakerDefinition.SpeakerModel = OptionToAudioSpeakerModel(SelectedItemOption);
		}
		if (IsEditingAudio())
		{
			ApplyAudioEditToTarget();
		}
		else
		{
			ApplyAudioDefinitionToBuildManager();
		}
		RefreshMenu();
		return;
	}

	if (ActiveMenuTab == EBuildItemType::LoungeLayout && (!SelectedBuildItem || SelectedBuildItem->ItemType != EBuildItemType::DiningTable))
	{
		if (SelectedBuildItem && SelectedBuildItem->ItemType == EBuildItemType::SectionalLayout)
		{
			CurrentSectionalLayoutDefinition.ArmlessItem = SelectedItemOption;
			ApplySectionalDefinitionToBuildManager();
			RefreshMenu();
			return;
		}

		CurrentLoungeLayoutDefinition.ChairItem = SelectedItemOption;
		ApplyLoungeDefinitionToBuildManager();
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::FurnitureSeating)
	{
		if (CurrentFurnitureSeatingDefinition.SeatingType == EFurnitureSeatingRuntimeType::CafeTable)
		{
			CurrentFurnitureSeatingDefinition.CafeTableDefinition.CafeTableItem = SelectedItemOption;
		}
		else
		{
			CurrentFurnitureSeatingDefinition.BarSeatingDefinition.BarTableItem = SelectedItemOption;
		}
		ApplyFurnitureSeatingDefinitionToBuildManager();
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::BarLayout)
	{
		CurrentBarLayoutDefinition.FrontBarItem = SelectedItemOption;
		ApplyBarLayoutDefinitionToBuildManager();
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::DiningTable || (SelectedBuildItem && SelectedBuildItem->ItemType == EBuildItemType::DiningTable))
	{
		CurrentDiningTableDefinition.DiningTableItem = SelectedItemOption;
		ApplyDiningTableDefinitionToBuildManager();
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (CurrentTrussDefinition.BuildMode != ETrussBuildMode::CubeArch)
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
	if (bRefreshingControls)
	{
		return;
	}

	if (ActiveMenuTab == EBuildItemType::ProjectionScreen)
	{
		CurrentProjectionScreenDefinition.LensType = OptionToProjectionLens(SelectedItemOption);
		if (IsEditingProjection())
		{
			ApplyProjectionEditToTarget();
		}
		else
		{
			ApplyProjectionDefinitionToBuildManager();
		}
		RefreshMenu();
		return;
	}

	if (ActiveMenuTab == EBuildItemType::AudioPlacement)
	{
		CurrentAudioSource = OptionToAudioSource(SelectedItemOption);
		if (IsEditingAudio())
		{
			ApplyAudioEditToTarget();
		}
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::LoungeLayout && (!SelectedBuildItem || SelectedBuildItem->ItemType != EBuildItemType::DiningTable))
	{
		if (SelectedBuildItem && SelectedBuildItem->ItemType == EBuildItemType::SectionalLayout)
		{
			CurrentSectionalLayoutDefinition.CornerItem = SelectedItemOption;
			ApplySectionalDefinitionToBuildManager();
			if (DetailText)
			{
				DetailText->SetText(BuildDetailText());
			}
			return;
		}

		CurrentLoungeLayoutDefinition.CocktailTableItem = SelectedItemOption;
		ApplyLoungeDefinitionToBuildManager();
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::FurnitureSeating)
	{
		if (CurrentFurnitureSeatingDefinition.SeatingType == EFurnitureSeatingRuntimeType::CafeTable)
		{
			CurrentFurnitureSeatingDefinition.CafeTableDefinition.DiningChairItem = SelectedItemOption;
		}
		else
		{
			CurrentFurnitureSeatingDefinition.BarSeatingDefinition.StoolItem = SelectedItemOption;
		}
		ApplyFurnitureSeatingDefinitionToBuildManager();
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::BarLayout)
	{
		CurrentBarLayoutDefinition.BackBarItem = SelectedItemOption;
		ApplyBarLayoutDefinitionToBuildManager();
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::DiningTable || (SelectedBuildItem && SelectedBuildItem->ItemType == EBuildItemType::DiningTable))
	{
		CurrentDiningTableDefinition.DiningChairItem = SelectedItemOption;
		ApplyDiningTableDefinitionToBuildManager();
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (CurrentTrussDefinition.BuildMode != ETrussBuildMode::CubeArch)
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
	if (bRefreshingControls)
	{
		return;
	}
	if (EditingLightTarget)
	{
		ApplyLightingControlValue(4, NewValue);
		return;
	}

	if (ActiveMenuTab == EBuildItemType::LoungeLayout)
	{
		CurrentLoungeLayoutDefinition.EndTableCount = FMath::Clamp(FMath::RoundToInt(NewValue), 0, 8);
		ApplyLoungeDefinitionToBuildManager();
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

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
	if (bRefreshingControls)
	{
		return;
	}
	if (EditingLightTarget)
	{
		ApplyLightingControlValue(5, NewValue);
		return;
	}

	if (ActiveMenuTab == EBuildItemType::LoungeLayout)
	{
		CurrentLoungeLayoutDefinition.LampCount = FMath::Clamp(FMath::RoundToInt(NewValue), 0, 8);
		ApplyLoungeDefinitionToBuildManager();
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

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
	if (bRefreshingControls)
	{
		return;
	}

	if (ActiveMenuTab == EBuildItemType::LoungeLayout && (!SelectedBuildItem || SelectedBuildItem->ItemType != EBuildItemType::DiningTable))
	{
		CurrentLoungeLayoutDefinition.EndTableItem = SelectedItemOption;
		ApplyLoungeDefinitionToBuildManager();
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::FurnitureSeating)
	{
		if (CurrentFurnitureSeatingDefinition.SeatingType == EFurnitureSeatingRuntimeType::CafeTable)
		{
			CurrentFurnitureSeatingDefinition.CafeTableDefinition.ChairLayout = OptionToCafeTableLayout(SelectedItemOption);
		}
		else
		{
			CurrentFurnitureSeatingDefinition.BarSeatingDefinition.StoolLayout = OptionToBarSeatingLayout(SelectedItemOption);
		}
		ApplyFurnitureSeatingDefinitionToBuildManager();
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::DiningTable || (SelectedBuildItem && SelectedBuildItem->ItemType == EBuildItemType::DiningTable))
	{
		CurrentDiningTableDefinition.ChairArrangement = OptionToDiningChairArrangement(SelectedItemOption);
		ApplyDiningTableDefinitionToBuildManager();
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

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

void UBuildMenuWidget::HandleStageHeightChanged(FString SelectedItemOption, ESelectInfo::Type SelectionType)
{
	if (bRefreshingControls)
	{
		return;
	}

	if (IsDiningTableContext(ActiveMenuTab, SelectedBuildItem))
	{
		CurrentDiningTableDefinition.TableclothItem = SelectedItemOption;
		ApplyDiningTableDefinitionToBuildManager();
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::LoungeLayout)
	{
		CurrentLoungeLayoutDefinition.LampItem = SelectedItemOption;
		ApplyLoungeDefinitionToBuildManager();
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (bRefreshingControls || !SelectedBuildItem || SelectedBuildItem->ItemType != EBuildItemType::StageDeck)
	{
		return;
	}

	if (IsEditingStage() && CurrentStageEditScope == EStageRuntimeEditScope::Cell)
	{
		CurrentStageCellHeightPreset = OptionToStageHeightPreset(SelectedItemOption);
		ApplyStageEditToTarget();
	}
	else
	{
		CurrentStageDeckDefinition.DefaultHeightPreset = OptionToStageHeightPreset(SelectedItemOption);
		if (IsEditingStage())
		{
			ApplyStageEditToTarget();
		}
		else
		{
			ApplyStageDefinitionToBuildManager();
		}
	}
	if (DetailText)
	{
		DetailText->SetText(BuildDetailText());
	}
}

void UBuildMenuWidget::HandleStageSurfaceChanged(FString SelectedItemOption, ESelectInfo::Type SelectionType)
{
	if (bRefreshingControls)
	{
		return;
	}

	if (ActiveMenuTab == EBuildItemType::LoungeLayout)
	{
		CurrentLoungeLayoutDefinition.AccentItem = SelectedItemOption;
		ApplyLoungeDefinitionToBuildManager();
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (bRefreshingControls || !SelectedBuildItem || SelectedBuildItem->ItemType != EBuildItemType::StageDeck)
	{
		return;
	}

	if (IsEditingStage() && CurrentStageEditScope == EStageRuntimeEditScope::Cell)
	{
		CurrentStageCellSurfaceStyle = OptionToStageSurfaceStyle(SelectedItemOption);
		ApplyStageEditToTarget();
	}
	else
	{
		CurrentStageDeckDefinition.DefaultSurfaceStyle = OptionToStageSurfaceStyle(SelectedItemOption);
		if (IsEditingStage())
		{
			ApplyStageEditToTarget();
		}
		else
		{
			ApplyStageDefinitionToBuildManager();
		}
	}
	if (DetailText)
	{
		DetailText->SetText(BuildDetailText());
	}
}

void UBuildMenuWidget::HandleStageFrontRailingChanged(bool bIsChecked)
{
	if (bRefreshingControls)
	{
		return;
	}

	if (IsDiningTableContext(ActiveMenuTab, SelectedBuildItem))
	{
		CurrentDiningTableDefinition.bUseTablecloth = bIsChecked;
		ApplyDiningTableDefinitionToBuildManager();
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::LoungeLayout)
	{
		if (SelectedBuildItem && SelectedBuildItem->ItemType == EBuildItemType::SectionalLayout)
		{
			CurrentSectionalLayoutDefinition.bUseArmlessItemDefaultSpacing = bIsChecked;
			ApplySectionalDefinitionToBuildManager();
			RefreshMenu();
			return;
		}

		CurrentLoungeLayoutDefinition.bUseTwoSofas = bIsChecked;
		ApplyLoungeDefinitionToBuildManager();
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::FurnitureSeating)
	{
		if (CurrentFurnitureSeatingDefinition.SeatingType == EFurnitureSeatingRuntimeType::CafeTable)
		{
			CurrentFurnitureSeatingDefinition.CafeTableDefinition.bClassroomStyle = bIsChecked;
		}
		else
		{
			CurrentFurnitureSeatingDefinition.BarSeatingDefinition.bClassroomStyle = bIsChecked;
		}
		ApplyFurnitureSeatingDefinitionToBuildManager();
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::BarLayout)
	{
		CurrentBarLayoutDefinition.bUseFrontBar = bIsChecked;
		ApplyBarLayoutDefinitionToBuildManager();
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (bRefreshingControls || ActiveMenuTab != EBuildItemType::StageDeck)
	{
		return;
	}

	CurrentStageDeckDefinition.bEnableFrontRailing = bIsChecked;
	if (IsEditingStage())
	{
		ApplyStageEditToTarget();
	}
	else
	{
		ApplyStageDefinitionToBuildManager();
	}
	if (DetailText)
	{
		DetailText->SetText(BuildDetailText());
	}
}

void UBuildMenuWidget::HandleStageBackRailingChanged(bool bIsChecked)
{
	if (bRefreshingControls)
	{
		return;
	}

	if (ActiveMenuTab == EBuildItemType::LoungeLayout)
	{
		CurrentLoungeLayoutDefinition.bUseFourChairs = bIsChecked;
		ApplyLoungeDefinitionToBuildManager();
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab == EBuildItemType::BarLayout)
	{
		CurrentBarLayoutDefinition.bUseBackBar = bIsChecked;
		ApplyBarLayoutDefinitionToBuildManager();
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (bRefreshingControls || ActiveMenuTab != EBuildItemType::StageDeck)
	{
		return;
	}

	CurrentStageDeckDefinition.bEnableBackRailing = bIsChecked;
	if (IsEditingStage())
	{
		ApplyStageEditToTarget();
	}
	else
	{
		ApplyStageDefinitionToBuildManager();
	}
	if (DetailText)
	{
		DetailText->SetText(BuildDetailText());
	}
}

void UBuildMenuWidget::HandleStageLeftRailingChanged(bool bIsChecked)
{
	if (bRefreshingControls || ActiveMenuTab != EBuildItemType::StageDeck)
	{
		return;
	}

	CurrentStageDeckDefinition.bEnableLeftRailing = bIsChecked;
	if (IsEditingStage())
	{
		ApplyStageEditToTarget();
	}
	else
	{
		ApplyStageDefinitionToBuildManager();
	}
	if (DetailText)
	{
		DetailText->SetText(BuildDetailText());
	}
}

void UBuildMenuWidget::HandleStageRightRailingChanged(bool bIsChecked)
{
	if (bRefreshingControls || ActiveMenuTab != EBuildItemType::StageDeck)
	{
		return;
	}

	CurrentStageDeckDefinition.bEnableRightRailing = bIsChecked;
	if (IsEditingStage())
	{
		ApplyStageEditToTarget();
	}
	else
	{
		ApplyStageDefinitionToBuildManager();
	}
	if (DetailText)
	{
		DetailText->SetText(BuildDetailText());
	}
}

void UBuildMenuWidget::HandleStageLeftStepChanged(bool bIsChecked)
{
	if (bRefreshingControls || ActiveMenuTab != EBuildItemType::StageDeck)
	{
		return;
	}

	CurrentStageDeckDefinition.bEnableLeftStep = bIsChecked;
	if (IsEditingStage())
	{
		ApplyStageEditToTarget();
	}
	else
	{
		ApplyStageDefinitionToBuildManager();
	}
	if (DetailText)
	{
		DetailText->SetText(BuildDetailText());
	}
}

void UBuildMenuWidget::HandleStageRightStepChanged(bool bIsChecked)
{
	if (bRefreshingControls || ActiveMenuTab != EBuildItemType::StageDeck)
	{
		return;
	}

	CurrentStageDeckDefinition.bEnableRightStep = bIsChecked;
	if (IsEditingStage())
	{
		ApplyStageEditToTarget();
	}
	else
	{
		ApplyStageDefinitionToBuildManager();
	}
	if (DetailText)
	{
		DetailText->SetText(BuildDetailText());
	}
}

void UBuildMenuWidget::HandleStageAutomaticSkirtChanged(bool bIsChecked)
{
	if (bRefreshingControls)
	{
		return;
	}

	if (ActiveMenuTab == EBuildItemType::AudioPlacement)
	{
		CurrentAudioPlacementDefinition.GroundLineArrayDefinition.bSubOnly = bIsChecked;
		if (IsEditingAudio())
		{
			ApplyAudioEditToTarget();
		}
		else
		{
			ApplyAudioDefinitionToBuildManager();
		}
		if (DetailText)
		{
			DetailText->SetText(BuildDetailText());
		}
		return;
	}

	if (ActiveMenuTab != EBuildItemType::StageDeck)
	{
		return;
	}

	CurrentStageDeckDefinition.bEnableAutomaticSkirt = bIsChecked;
	if (IsEditingStage())
	{
		ApplyStageEditToTarget();
	}
	else
	{
		ApplyStageDefinitionToBuildManager();
	}
	if (DetailText)
	{
		DetailText->SetText(BuildDetailText());
	}
}

void UBuildMenuWidget::HandleStageCellEnabledChanged(bool bIsChecked)
{
	if (bRefreshingControls || !IsEditingStage() || CurrentStageEditScope != EStageRuntimeEditScope::Cell)
	{
		return;
	}

	bCurrentStageCellEnabled = bIsChecked;
	ApplyStageEditToTarget();
	if (DetailText)
	{
		DetailText->SetText(BuildDetailText());
	}
}

void UBuildMenuWidget::HandleActionButtonClicked()
{
	OnActionRequested.Broadcast();
}

void UBuildMenuWidget::HandleMoveActorButtonClicked()
{
	OnMoveRequested.Broadcast();
}

void UBuildMenuWidget::HandleRotateActorButtonClicked()
{
	OnRotateRequested.Broadcast();
}

void UBuildMenuWidget::HandleDeleteActorButtonClicked()
{
	OnDeleteRequested.Broadcast();
}

void UBuildMenuWidget::HandleDesignerCancelClicked()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UBuildMenuWidget::HandleDesignerRotationChanged(float NewValue)
{
	if (bRefreshingControls)
	{
		return;
	}
	if (EditingLightTarget)
	{
		ApplyLightingControlValue(6, NewValue);
		return;
	}
	if (!BuildManager)
	{
		DesignerLastRotationSliderValue = NewValue;
		return;
	}

	const float DeltaDegrees = NewValue - DesignerLastRotationSliderValue;
	DesignerLastRotationSliderValue = NewValue;
	if (!FMath::IsNearlyZero(DeltaDegrees))
	{
		BuildManager->RotatePreviewYaw(DeltaDegrees);
	}
}
