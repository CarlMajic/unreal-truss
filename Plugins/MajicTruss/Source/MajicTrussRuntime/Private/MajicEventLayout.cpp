#include "MajicEventLayout.h"

#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
constexpr double CmPerFoot = 30.48;
const TCHAR* FormatName = TEXT("majic-event-layout");

double Round4(double Value)
{
	return FMath::RoundToDouble(Value * 10000.0) / 10000.0 + 0.0;
}

TSharedPtr<FJsonObject> ItemToJson(const FMajicLayoutItem& Item)
{
	// Items of types we do not build are written exactly as they were read.
	if (Item.Raw.IsValid())
	{
		return Item.Raw;
	}

	const TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();
	Out->SetStringField(TEXT("id"), Item.Id);
	Out->SetStringField(TEXT("type"), Item.Type);
	Out->SetStringField(TEXT("name"), Item.Name.IsEmpty() ? Item.Id : Item.Name);

	const TSharedRef<FJsonObject> Position = MakeShared<FJsonObject>();
	Position->SetNumberField(TEXT("x_ft"), Round4(Item.XFt));
	Position->SetNumberField(TEXT("y_ft"), Round4(Item.YFt));
	Position->SetNumberField(TEXT("rotation_deg"), Round4(Item.RotationDeg));
	Out->SetObjectField(TEXT("position"), Position);

	Out->SetObjectField(TEXT("params"), Item.Params.IsValid() ? Item.Params : MakeShared<FJsonObject>());
	return Out;
}
}

namespace MajicEventLayout
{
bool Parse(const FString& JsonText, FMajicLayoutDoc& OutDoc, FString& OutError)
{
	OutDoc = FMajicLayoutDoc();
	OutError.Reset();

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		OutError = TEXT("The file is not valid JSON.");
		return false;
	}

	FString Format;
	if (!Root->TryGetStringField(TEXT("format"), Format) || Format != FormatName)
	{
		OutError = TEXT("This is not a Majic Event Layout file (format must be \"majic-event-layout\").");
		return false;
	}

	double VersionNumber = 0.0;
	if (!Root->TryGetNumberField(TEXT("version"), VersionNumber) || !FMath::IsNearlyEqual(VersionNumber, FMath::RoundToDouble(VersionNumber)))
	{
		OutError = TEXT("The layout version must be an integer.");
		return false;
	}
	if (FMath::RoundToInt(VersionNumber) != 1)
	{
		OutError = FString::Printf(TEXT("Unsupported layout version %d. This plugin reads version 1."), FMath::RoundToInt(VersionNumber));
		return false;
	}

	const TSharedPtr<FJsonObject>* EventObject = nullptr;
	if (Root->TryGetObjectField(TEXT("event"), EventObject))
	{
		OutDoc.Event = *EventObject;
	}

	const TArray<TSharedPtr<FJsonValue>>* ItemValues = nullptr;
	if (!Root->TryGetArrayField(TEXT("items"), ItemValues))
	{
		OutError = TEXT("The layout has no \"items\" list.");
		return false;
	}

	TSet<FString> SeenIds;
	for (const TSharedPtr<FJsonValue>& Value : *ItemValues)
	{
		const TSharedPtr<FJsonObject> Object = Value.IsValid() ? Value->AsObject() : nullptr;
		if (!Object.IsValid())
		{
			OutError = TEXT("Every item must be a JSON object.");
			return false;
		}

		FMajicLayoutItem Item;
		if (!Object->TryGetStringField(TEXT("id"), Item.Id) || Item.Id.IsEmpty())
		{
			OutError = TEXT("Every item needs a non-empty \"id\".");
			return false;
		}
		if (SeenIds.Contains(Item.Id))
		{
			OutError = FString::Printf(TEXT("Duplicate item id \"%s\"."), *Item.Id);
			return false;
		}
		SeenIds.Add(Item.Id);

		if (!Object->TryGetStringField(TEXT("type"), Item.Type) || Item.Type.IsEmpty())
		{
			OutError = FString::Printf(TEXT("Item \"%s\" has no \"type\"."), *Item.Id);
			return false;
		}
		if (!Object->TryGetStringField(TEXT("name"), Item.Name) || Item.Name.IsEmpty())
		{
			Item.Name = Item.Id;
		}

		const TSharedPtr<FJsonObject>* Position = nullptr;
		if (Object->TryGetObjectField(TEXT("position"), Position))
		{
			(*Position)->TryGetNumberField(TEXT("x_ft"), Item.XFt);
			(*Position)->TryGetNumberField(TEXT("y_ft"), Item.YFt);
			(*Position)->TryGetNumberField(TEXT("rotation_deg"), Item.RotationDeg);
		}

		const TSharedPtr<FJsonObject>* Params = nullptr;
		if (!Object->TryGetObjectField(TEXT("params"), Params))
		{
			OutError = FString::Printf(TEXT("Item \"%s\" has no \"params\" object."), *Item.Id);
			return false;
		}
		Item.Params = *Params;
		Item.Raw = Object;
		OutDoc.Items.Add(MoveTemp(Item));
	}
	return true;
}

FString Serialize(const FMajicLayoutDoc& Doc)
{
	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("format"), FormatName);
	Root->SetNumberField(TEXT("version"), 1);
	if (Doc.Event.IsValid())
	{
		Root->SetObjectField(TEXT("event"), Doc.Event);
	}

	TArray<TSharedPtr<FJsonValue>> Items;
	for (const FMajicLayoutItem& Item : Doc.Items)
	{
		Items.Add(MakeShared<FJsonValueObject>(ItemToJson(Item)));
	}
	Root->SetArrayField(TEXT("items"), Items);

	FString Text;
	const TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&Text);
	FJsonSerializer::Serialize(Root, Writer);
	return Text;
}

bool LoadFile(const FString& FilePath, FMajicLayoutDoc& OutDoc, FString& OutError)
{
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *FilePath))
	{
		OutError = FString::Printf(TEXT("Could not read %s."), *FilePath);
		return false;
	}
	return Parse(Text, OutDoc, OutError);
}

bool SaveFile(const FString& FilePath, const FMajicLayoutDoc& Doc, FString& OutError)
{
	if (!FFileHelper::SaveStringToFile(Serialize(Doc), *FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		OutError = FString::Printf(TEXT("Could not write %s."), *FilePath);
		return false;
	}
	return true;
}

void MergeItems(
	FMajicLayoutDoc& Doc,
	const TArray<FMajicLayoutItem>& NewItems,
	const TSet<FString>& ReplaceTypes,
	TFunction<bool(const FMajicLayoutItem&)> KeepExisting)
{
	TArray<FMajicLayoutItem> Result;
	for (const FMajicLayoutItem& Existing : Doc.Items)
	{
		if (!ReplaceTypes.Contains(Existing.Type) || (KeepExisting && KeepExisting(Existing)))
		{
			Result.Add(Existing);
		}
	}
	for (const FMajicLayoutItem& Item : NewItems)
	{
		// An id the file still holds (for example a kept control drape) is not written twice.
		const bool bTaken = Result.ContainsByPredicate([&Item](const FMajicLayoutItem& Other) { return Other.Id == Item.Id; });
		if (!bTaken)
		{
			Result.Add(Item);
		}
	}
	Doc.Items = MoveTemp(Result);
}

FTransform ActorTransformForOrigin(double XFt, double YFt, double RotationDeg, const FVector& LocalCorner)
{
	const FRotator Rotation(0.0, 180.0 - RotationDeg, 0.0);
	const FVector Origin(XFt * CmPerFoot, -YFt * CmPerFoot, 0.0);
	return FTransform(Rotation, Origin - Rotation.RotateVector(LocalCorner));
}

void ReadOriginFromTransform(const FTransform& Transform, const FVector& LocalCorner, double& OutXFt, double& OutYFt, double& OutRotationDeg)
{
	const FVector World = Transform.TransformPosition(LocalCorner);
	OutXFt = Round4(World.X / CmPerFoot);
	OutYFt = Round4(-World.Y / CmPerFoot);
	OutRotationDeg = Round4(FRotator::NormalizeAxis(180.0 - Transform.Rotator().Yaw));
}
}
