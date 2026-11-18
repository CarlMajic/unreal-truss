#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

// Majic Event Layout: the JSON file shared by the Blender, Fusion and Unreal plugins
// (format spec: majic-layout-format/docs/FORMAT.md). This header is the file model: reading,
// writing and merging. It knows nothing about actors.

struct MAJICTRUSSRUNTIME_API FMajicLayoutItem
{
	FString Id;
	FString Type;
	FString Name;
	double XFt = 0.0;
	double YFt = 0.0;
	double RotationDeg = 0.0;

	// Type-specific inputs. Never null for a parsed or created item.
	TSharedPtr<FJsonObject> Params;

	// The item exactly as read, kept so that item types this plugin does not build are written
	// back untouched. Null for items created here.
	TSharedPtr<FJsonObject> Raw;
};

struct MAJICTRUSSRUNTIME_API FMajicLayoutDoc
{
	TSharedPtr<FJsonObject> Event;
	TArray<FMajicLayoutItem> Items;
};

namespace MajicEventLayout
{
	MAJICTRUSSRUNTIME_API bool Parse(const FString& JsonText, FMajicLayoutDoc& OutDoc, FString& OutError);
	MAJICTRUSSRUNTIME_API FString Serialize(const FMajicLayoutDoc& Doc);
	MAJICTRUSSRUNTIME_API bool LoadFile(const FString& FilePath, FMajicLayoutDoc& OutDoc, FString& OutError);
	MAJICTRUSSRUNTIME_API bool SaveFile(const FString& FilePath, const FMajicLayoutDoc& Doc, FString& OutError);

	// Replace every item whose type is in ReplaceTypes with NewItems, keeping all other items.
	// Existing items of a replaced type for which KeepExisting returns true are kept too.
	MAJICTRUSSRUNTIME_API void MergeItems(
		FMajicLayoutDoc& Doc,
		const TArray<FMajicLayoutItem>& NewItems,
		const TSet<FString>& ReplaceTypes,
		TFunction<bool(const FMajicLayoutItem&)> KeepExisting = nullptr);

	// ---- Placement -------------------------------------------------------------------
	// File frame: x_ft to house right, y_ft away from the audience, rotation counter-clockwise
	// from above, zero facing the audience. The Majic Unreal actors are authored with their front
	// toward -Y, so the world places the audience at +Y: world X = x, world Y = -y, and an actor
	// is turned 180 degrees plus the file rotation (yaw = 180 - rotation).
	// LocalCorner is the item's origin point in the actor's own frame (cm).
	MAJICTRUSSRUNTIME_API FTransform ActorTransformForOrigin(double XFt, double YFt, double RotationDeg, const FVector& LocalCorner);
	MAJICTRUSSRUNTIME_API void ReadOriginFromTransform(const FTransform& Transform, const FVector& LocalCorner, double& OutXFt, double& OutYFt, double& OutRotationDeg);
}
