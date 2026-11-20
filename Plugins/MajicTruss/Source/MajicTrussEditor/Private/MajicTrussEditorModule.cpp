#include "Modules/ModuleManager.h"

#include "AssetRegistry/AssetData.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Engine/Texture2D.h"
#include "IDetailCustomization.h"
#include "IDetailPropertyRow.h"
#include "LightFixtureActor.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyEditorModule.h"

class FLightFixtureActorDetails final : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShared<FLightFixtureActorDetails>();
	}

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override
	{
		const TSharedRef<IPropertyHandle> GoboHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(ALightFixtureActor, GoboTexture));
		IDetailCategoryBuilder& GoboCategory = DetailBuilder.EditCategory(TEXT("Light Fixture|Gobo"));
		GoboCategory.AddProperty(GoboHandle).CustomWidget()
		.NameContent()
		[
			GoboHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth(250.0f)
		[
			SNew(SObjectPropertyEntryBox)
			.PropertyHandle(GoboHandle)
			.AllowedClass(UTexture2D::StaticClass())
			.OnShouldFilterAsset_Static(&FLightFixtureActorDetails::ShouldFilterStockGobo)
		];
	}

private:
	static bool ShouldFilterStockGobo(const FAssetData& AssetData)
	{
		return AssetData.PackagePath != FName(TEXT("/Game/Majic_Gear/Lighting/Majic_Gobos"));
	}
};

class FMajicTrussEditorModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
		PropertyEditor.RegisterCustomClassLayout(ALightFixtureActor::StaticClass()->GetFName(),
			FOnGetDetailCustomizationInstance::CreateStatic(&FLightFixtureActorDetails::MakeInstance));
		PropertyEditor.NotifyCustomizationModuleChanged();
	}

	virtual void ShutdownModule() override
	{
		if (FModuleManager::Get().IsModuleLoaded(TEXT("PropertyEditor")))
		{
			FPropertyEditorModule& PropertyEditor = FModuleManager::GetModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
			PropertyEditor.UnregisterCustomClassLayout(ALightFixtureActor::StaticClass()->GetFName());
			PropertyEditor.NotifyCustomizationModuleChanged();
		}
	}
};

IMPLEMENT_MODULE(FMajicTrussEditorModule, MajicTrussEditor)
