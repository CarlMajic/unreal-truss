#include "MBPWallActor.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"
#include "Misc/Paths.h"
#include "UObject/UnrealType.h"
#include "Materials/MaterialInterface.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#if WITH_EDITOR
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#endif

namespace
{
constexpr float DefaultShimmerFaceOffsetXCm = -1.902981f;
constexpr float DefaultShimmerFaceOffsetYCm = -0.104f;
constexpr float DefaultShimmerFaceOffsetZCm = -4.650027f;

FString GetStyleAssetFolder(EMBPPanelStyle Style)
{
	switch (Style)
	{
	case EMBPPanelStyle::Empty:
	case EMBPPanelStyle::Custom:
		return FString();
	case EMBPPanelStyle::Blank:
		return TEXT("/Game/Majic_Gear/MBP/Extra_Frame/StaticMeshes");
	case EMBPPanelStyle::Acrylic:
		return TEXT("/Game/Majic_Gear/MBP/Acrylic_Segment/StaticMeshes");
	case EMBPPanelStyle::Boxwood:
		return TEXT("/Game/Majic_Gear/MBP/Boxwood_Segment/StaticMeshes");
	case EMBPPanelStyle::Drift:
		return TEXT("/Game/Majic_Gear/MBP/Drift_Segment/StaticMeshes");
	case EMBPPanelStyle::Geo:
		return TEXT("/Game/Majic_Gear/MBP/Geo_Segment/StaticMeshes");
	case EMBPPanelStyle::Shimmer:
		return TEXT("/Game/Majic_Gear/MBP/Shimmer_Segment/StaticMeshes");
	case EMBPPanelStyle::Hive:
		return TEXT("/Game/Majic_Gear/MBP/Hive_Segment/StaticMeshes");
	case EMBPPanelStyle::Platinum:
		return TEXT("/Game/Majic_Gear/MBP/Platinum_Segment/StaticMeshes");
	default:
		return FString();
	}
}

FString GetShimmerMaterialPath(EMBPShimmerVariant Variant)
{
	switch (Variant)
	{
	case EMBPShimmerVariant::Red:
		return TEXT("/Game/Majic_Gear/MBP/Shimmer_Segment/Materials/RedShimmerSquare.RedShimmerSquare");
	case EMBPShimmerVariant::Fuchsia:
		return TEXT("/Game/Majic_Gear/MBP/Shimmer_Segment/Materials/FushaShimmerSquare.FushaShimmerSquare");
	case EMBPShimmerVariant::Holographic:
		return TEXT("/Game/Majic_Gear/MBP/Shimmer_Segment/Materials/HolographicSquare_Mat1.HolographicSquare_Mat1");
	case EMBPShimmerVariant::Gold:
	default:
		return TEXT("/Game/Majic_Gear/MBP/Shimmer_Segment/Materials/GoldShimmerSquare_Mat.GoldShimmerSquare_Mat");
	}
}

bool IsMBPStaticMeshAsset(const FAssetData& AssetData)
{
	return AssetData.AssetClassPath == UStaticMesh::StaticClass()->GetClassPathName();
}

FString GetExtraFrameAssetFolder()
{
	return TEXT("/Game/Majic_Gear/MBP/Extra_Frame/StaticMeshes");
}

constexpr int32 ShimmerPlaneColumns = 30;
constexpr int32 ShimmerPlaneRows = 30;

bool ReadRequiredPositiveNumber(const TSharedPtr<FJsonObject>& Object, const TCHAR* FieldName, double& OutValue, FString& OutError)
{
	if (!Object->TryGetNumberField(FieldName, OutValue) || OutValue <= 0.0)
	{
		OutError = FString::Printf(TEXT("%s must be a number greater than zero."), FieldName);
		return false;
	}
	return true;
}

FString NormalizeWebStyle(FString Style)
{
	Style.TrimStartAndEndInline();
	Style.ToLowerInline();
	Style.ReplaceInline(TEXT("-"), TEXT("_"));
	Style.ReplaceInline(TEXT(" "), TEXT(""));
	return Style;
}

bool ResolveWebStyle(const FString& RawStyle, EMBPPanelStyle& OutStyle, EMBPShimmerVariant& OutShimmerVariant)
{
	const FString Style = NormalizeWebStyle(RawStyle.IsEmpty() ? TEXT("empty") : RawStyle);
	OutShimmerVariant = EMBPShimmerVariant::Gold;

	if (Style == TEXT("empty")) OutStyle = EMBPPanelStyle::Empty;
	else if (Style == TEXT("blank")) OutStyle = EMBPPanelStyle::Blank;
	else if (Style == TEXT("acrylic")) OutStyle = EMBPPanelStyle::Acrylic;
	else if (Style == TEXT("platinum")) OutStyle = EMBPPanelStyle::Platinum;
	else if (Style == TEXT("hive")) OutStyle = EMBPPanelStyle::Hive;
	else if (Style == TEXT("drift")) OutStyle = EMBPPanelStyle::Drift;
	else if (Style == TEXT("diamond") || Style == TEXT("geowall") || Style == TEXT("geo")) OutStyle = EMBPPanelStyle::Geo;
	else if (Style == TEXT("greenwall")) OutStyle = EMBPPanelStyle::Boxwood;
	else if (Style == TEXT("shimmer") || Style == TEXT("goldshimmer")) OutStyle = EMBPPanelStyle::Shimmer;
	else if (Style == TEXT("redshimmer"))
	{
		OutStyle = EMBPPanelStyle::Shimmer;
		OutShimmerVariant = EMBPShimmerVariant::Red;
	}
	else if (Style == TEXT("fushashimmer") || Style == TEXT("fuchsiashimmer"))
	{
		OutStyle = EMBPPanelStyle::Shimmer;
		OutShimmerVariant = EMBPShimmerVariant::Fuchsia;
	}
	else if (Style == TEXT("holographicshimmer"))
	{
		OutStyle = EMBPPanelStyle::Shimmer;
		OutShimmerVariant = EMBPShimmerVariant::Holographic;
	}
	else return false;

	return true;
}

bool ResolveSupportInterval(const TSharedPtr<FJsonValue>& Value, int32& OutInterval)
{
	if (!Value.IsValid())
	{
		OutInterval = 2;
		return true;
	}
	if (Value->Type == EJson::Number)
	{
		const double Number = Value->AsNumber();
		OutInterval = FMath::RoundToInt(Number);
		return FMath::IsNearlyEqual(Number, static_cast<double>(OutInterval)) && OutInterval >= 1;
	}

	FString Key = Value->AsString();
	Key.TrimStartAndEndInline();
	Key.ToLowerInline();
	if (Key == TEXT("every") || Key == TEXT("every_column")) OutInterval = 1;
	else if (Key == TEXT("every_other") || Key == TEXT("every_2nd")) OutInterval = 2;
	else if (Key == TEXT("every_third") || Key == TEXT("every_3rd")) OutInterval = 3;
	else if (Key == TEXT("ends") || Key == TEXT("ends_only")) OutInterval = 99;
	else return false;
	return true;
}
}

AMBPWallActor::AMBPWallActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	SelectionBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("SelectionBounds"));
	SelectionBounds->SetupAttachment(SceneRoot);
	SelectionBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SelectionBounds->SetCollisionObjectType(ECC_WorldDynamic);
	SelectionBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	SelectionBounds->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SelectionBounds->SetGenerateOverlapEvents(false);
	SelectionBounds->SetHiddenInGame(true);
	SelectionBounds->SetLineThickness(2.0f);
	SelectionBounds->ShapeColor = FColor::Yellow;
}

void AMBPWallActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (FMath::IsNearlyZero(ShimmerFaceOffsetXCm) &&
		FMath::IsNearlyZero(ShimmerFaceOffsetYCm) &&
		FMath::IsNearlyZero(ShimmerFaceOffsetZCm))
	{
		ShimmerFaceOffsetXCm = DefaultShimmerFaceOffsetXCm;
		ShimmerFaceOffsetYCm = DefaultShimmerFaceOffsetYCm;
		ShimmerFaceOffsetZCm = DefaultShimmerFaceOffsetZCm;
	}

	EnsureSlotCount(false);
	SyncSlotsToDefaultStyleIfNeeded();
	if (bBuildOnConstruction)
	{
		RebuildWall();
	}
}

#if WITH_EDITOR
void AMBPWallActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AMBPWallActor, ImportedJsonFile) && !ImportedJsonFile.IsEmpty())
	{
		FString Error;
		if (!ImportWebLayoutFromJsonFile(ImportedJsonFile, Error))
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Error), FText::FromString(TEXT("MBP Import Failed")));
		}
		return;
	}
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AMBPWallActor, BatchTargetIndex) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(AMBPWallActor, BatchEditAxis) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(AMBPWallActor, BatchStyle) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(AMBPWallActor, BatchShimmerVariant) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(AMBPWallActor, BatchShimmerMaterial) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(AMBPWallActor, BatchDepthOffsetCm))
	{
		ApplyBatchEdit();
	}
}
#endif

void AMBPWallActor::RebuildWall()
{
	EnsureSlotCount(false);
	ClearGeneratedComponents();

	const float StepX = PanelWidthCm + HorizontalSpacingCm;
	const float StepZ = PanelHeightCm + VerticalSpacingCm;
	const float OriginX = bCenterOnActor ? (-0.5f * (Columns - 1) * StepX) : 0.0f;
	const float OriginZ = bCenterOnActor ? (-0.5f * (Rows - 1) * StepZ) : 0.0f;
	const FRotator PanelRotation(0.0f, 90.0f, 0.0f);

	TMap<FString, UInstancedStaticMeshComponent*> BucketMap;
	FBox Bounds(EForceInit::ForceInit);

	for (int32 RowIndex = 0; RowIndex < Rows; ++RowIndex)
	{
		for (int32 ColumnIndex = 0; ColumnIndex < Columns; ++ColumnIndex)
		{
			const int32 SlotIndex = (RowIndex * Columns) + ColumnIndex;
			if (!PanelSlots.IsValidIndex(SlotIndex))
			{
				continue;
			}

			const FMBPPanelSlot& Slot = PanelSlots[SlotIndex];
			const float SnappedDepthOffsetCm = GetSnappedDepthOffsetCm(Slot.DepthOffsetCm);
			const FVector SlotLocation(
				OriginX + (ColumnIndex * StepX),
				SnappedDepthOffsetCm,
				OriginZ + (RowIndex * StepZ));

			for (const FSoftObjectPath& MeshPath : GetMeshPathsForSlot(Slot))
			{
				UInstancedStaticMeshComponent* MeshComponent = FindOrCreateComponentBucket(MeshPath, Slot, BucketMap);
				if (!MeshComponent)
				{
					continue;
				}

				const FTransform InstanceTransform(PanelRotation, SlotLocation, FVector::OneVector);
				MeshComponent->AddInstance(InstanceTransform);

				if (const UStaticMesh* StaticMesh = MeshComponent->GetStaticMesh())
				{
					const FBoxSphereBounds MeshBounds = StaticMesh->GetBounds();
					const FVector Extent = MeshBounds.BoxExtent;
					Bounds += FBox(SlotLocation - Extent, SlotLocation + Extent);
				}
			}

			if (Slot.Style == EMBPPanelStyle::Shimmer)
			{
				const FSoftObjectPath PlaneMeshPath = GetShimmerPlaneMeshPath();
				if (UInstancedStaticMeshComponent* PlaneComponent = FindOrCreateComponentBucket(PlaneMeshPath, Slot, BucketMap))
				{
					if (const UStaticMesh* PlaneMesh = PlaneComponent->GetStaticMesh())
					{
						const FVector NativePlaneSize = PlaneMesh->GetBounds().BoxExtent * 2.0f;
						const float NativeTileSizeCm = FMath::Max3(NativePlaneSize.X, NativePlaneSize.Y, NativePlaneSize.Z);
						if (NativeTileSizeCm <= KINDA_SMALL_NUMBER)
						{
							continue;
						}

						const float TileWidthCm = PanelWidthCm / static_cast<float>(ShimmerPlaneColumns);
						const float TileHeightCm = PanelHeightCm / static_cast<float>(ShimmerPlaneRows);
						const float TileSizeCm = FMath::Min(TileWidthCm, TileHeightCm);
						const float TileScale = TileSizeCm / NativeTileSizeCm;
						const FVector ScaledPlaneExtent = PlaneMesh->GetBounds().BoxExtent * TileScale;
						const float PlaneStartX = SlotLocation.X + (0.5f * TileWidthCm);
						const float PlaneStartZ = SlotLocation.Z - (0.5f * TileHeightCm);

						for (int32 PlaneRow = 0; PlaneRow < ShimmerPlaneRows; ++PlaneRow)
						{
							for (int32 PlaneColumn = 0; PlaneColumn < ShimmerPlaneColumns; ++PlaneColumn)
							{
								const FVector PlaneLocation(
									PlaneStartX + (PlaneColumn * TileWidthCm) + ShimmerFaceOffsetXCm,
									SlotLocation.Y + ShimmerFaceOffsetYCm,
									PlaneStartZ - (PlaneRow * TileHeightCm) + ShimmerFaceOffsetZCm);
								const FTransform PlaneTransform(PanelRotation, PlaneLocation, FVector(TileScale));
								PlaneComponent->AddInstance(PlaneTransform);
								Bounds += FBox(PlaneLocation - ScaledPlaneExtent, PlaneLocation + ScaledPlaneExtent);
							}
						}
					}
				}
			}

			if (Slot.Style != EMBPPanelStyle::Blank && !FMath::IsNearlyZero(SnappedDepthOffsetCm))
			{
				for (const FSoftObjectPath& MeshPath : GetExtraFrameMeshPaths())
				{
					UInstancedStaticMeshComponent* MeshComponent = FindOrCreateComponentBucket(MeshPath, Slot, BucketMap);
					if (!MeshComponent)
					{
						continue;
					}

					const FTransform InstanceTransform(PanelRotation, SlotLocation, FVector::OneVector);
					MeshComponent->AddInstance(InstanceTransform);

					if (const UStaticMesh* StaticMesh = MeshComponent->GetStaticMesh())
					{
						const FBoxSphereBounds MeshBounds = StaticMesh->GetBounds();
						const FVector Extent = MeshBounds.BoxExtent;
						Bounds += FBox(SlotLocation - Extent, SlotLocation + Extent);
					}
				}
			}
		}
	}

	UpdateSelectionBounds(Bounds);
}

void AMBPWallActor::ApplyWallDefinition(const FMBPWallDefinition& Definition, bool bResetSlotsToDefault)
{
	Columns = FMath::Max(1, Definition.Columns);
	Rows = FMath::Max(1, Definition.Rows);
	DefaultStyle = Definition.DefaultStyle;
	DefaultShimmerVariant = Definition.DefaultShimmerVariant;
	DefaultShimmerMaterial = Definition.DefaultShimmerMaterial;
	ShimmerFaceOffsetXCm = Definition.ShimmerFaceOffsetXCm;
	ShimmerFaceOffsetYCm = Definition.ShimmerFaceOffsetYCm;
	ShimmerFaceOffsetZCm = Definition.ShimmerFaceOffsetZCm;
	PanelWidthCm = Definition.PanelWidthCm;
	PanelHeightCm = Definition.PanelHeightCm;
	HorizontalSpacingCm = Definition.HorizontalSpacingCm;
	VerticalSpacingCm = Definition.VerticalSpacingCm;
	DepthOffsetStepCm = Definition.DepthOffsetStepCm;
	bCenterOnActor = Definition.bCenterOnActor;
	SupportInterval = FMath::Max(1, Definition.SupportInterval);
	bLeftCap = Definition.bLeftCap;
	bRightCap = Definition.bRightCap;

	if (bResetSlotsToDefault)
	{
		ResetSlotsToDefault();
	}
	else
	{
		EnsureSlotCount(false);
		RebuildWall();
	}
}

FMBPWallDefinition AMBPWallActor::GetWallDefinition() const
{
	FMBPWallDefinition Definition;
	Definition.Columns = Columns;
	Definition.Rows = Rows;
	Definition.DefaultStyle = DefaultStyle;
	Definition.DefaultShimmerVariant = DefaultShimmerVariant;
	Definition.DefaultShimmerMaterial = DefaultShimmerMaterial;
	Definition.ShimmerFaceOffsetXCm = ShimmerFaceOffsetXCm;
	Definition.ShimmerFaceOffsetYCm = ShimmerFaceOffsetYCm;
	Definition.ShimmerFaceOffsetZCm = ShimmerFaceOffsetZCm;
	Definition.PanelWidthCm = PanelWidthCm;
	Definition.PanelHeightCm = PanelHeightCm;
	Definition.HorizontalSpacingCm = HorizontalSpacingCm;
	Definition.VerticalSpacingCm = VerticalSpacingCm;
	Definition.DepthOffsetStepCm = DepthOffsetStepCm;
	Definition.bCenterOnActor = bCenterOnActor;
	Definition.SupportInterval = SupportInterval;
	Definition.bLeftCap = bLeftCap;
	Definition.bRightCap = bRightCap;
	return Definition;
}

bool AMBPWallActor::ImportWebLayoutFromJson(const FString& JsonText, FString& OutError)
{
	OutError.Reset();
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		OutError = TEXT("The selected file is not valid JSON.");
		return false;
	}

	double VersionNumber = 1.0;
	Root->TryGetNumberField(TEXT("version"), VersionNumber);
	const int32 Version = FMath::RoundToInt(VersionNumber);
	if (!FMath::IsNearlyEqual(VersionNumber, static_cast<double>(Version)) || (Version != 1 && Version != 2))
	{
		OutError = FString::Printf(TEXT("Unsupported MBP layout version: %g."), VersionNumber);
		return false;
	}

	double WidthFt = 12.0;
	double HeightFt = 9.0;
	if (Root->HasField(TEXT("width_ft")) && !ReadRequiredPositiveNumber(Root, TEXT("width_ft"), WidthFt, OutError)) return false;
	if (Root->HasField(TEXT("height_ft")) && !ReadRequiredPositiveNumber(Root, TEXT("height_ft"), HeightFt, OutError)) return false;

	int32 ImportedSupportInterval = 2;
	if (!ResolveSupportInterval(Root->TryGetField(TEXT("support_spacing")), ImportedSupportInterval))
	{
		OutError = TEXT("Unsupported support spacing. Use every, every_other, every_third, or ends.");
		return false;
	}

	const int32 ImportedColumns = FMath::Max(1, FMath::RoundToInt(WidthFt / 3.0));
	const int32 ImportedRows = FMath::Max(1, FMath::FloorToInt(HeightFt / 3.0));
	TArray<FMBPPanelSlot> ImportedSlots;
	ImportedSlots.SetNum(ImportedColumns * ImportedRows);
	for (FMBPPanelSlot& Slot : ImportedSlots)
	{
		Slot.Style = EMBPPanelStyle::Empty;
		Slot.ShimmerVariant = EMBPShimmerVariant::Gold;
		Slot.DepthOffsetCm = 0.0f;
	}

	const TArray<TSharedPtr<FJsonValue>>* Cells = nullptr;
	if (Root->TryGetArrayField(TEXT("cells"), Cells))
	{
		for (const TSharedPtr<FJsonValue>& CellValue : *Cells)
		{
			const TSharedPtr<FJsonObject> Cell = CellValue.IsValid() ? CellValue->AsObject() : nullptr;
			if (!Cell.IsValid())
			{
				OutError = TEXT("Each layout cell must be a JSON object.");
				return false;
			}

			double RowNumber = 0.0;
			double ColumnNumber = 0.0;
			if (!Cell->TryGetNumberField(TEXT("row"), RowNumber) || !Cell->TryGetNumberField(TEXT("col"), ColumnNumber))
			{
				OutError = TEXT("Each layout cell must have integer row and col values.");
				return false;
			}
			const int32 Row = FMath::RoundToInt(RowNumber);
			const int32 Column = FMath::RoundToInt(ColumnNumber);
			if (!FMath::IsNearlyEqual(RowNumber, static_cast<double>(Row)) ||
				!FMath::IsNearlyEqual(ColumnNumber, static_cast<double>(Column)) || Row < 0 || Column < 0)
			{
				OutError = TEXT("Cell row and column must be non-negative integers.");
				return false;
			}

			FString WebStyle = TEXT("empty");
			Cell->TryGetStringField(TEXT("style"), WebStyle);
			EMBPPanelStyle PanelStyle = EMBPPanelStyle::Empty;
			EMBPShimmerVariant ShimmerVariant = EMBPShimmerVariant::Gold;
			if (!ResolveWebStyle(WebStyle, PanelStyle, ShimmerVariant))
			{
				OutError = FString::Printf(TEXT("Unsupported MBP panel style: \"%s\"."), *WebStyle);
				return false;
			}

			double DepthNumber = 0.0;
			if (Cell->HasField(TEXT("depth_in")) && !Cell->TryGetNumberField(TEXT("depth_in"), DepthNumber))
			{
				OutError = TEXT("Cell depth_in must be an integer.");
				return false;
			}
			const int32 DepthIn = FMath::RoundToInt(DepthNumber);
			if (!FMath::IsNearlyEqual(DepthNumber, static_cast<double>(DepthIn)) ||
				(DepthIn != 0 && DepthIn != 12 && DepthIn != 24 && DepthIn != 36))
			{
				OutError = TEXT("Cell depth must be 0, 12, 24, or 36 inches.");
				return false;
			}

			if (Row < ImportedRows && Column < ImportedColumns)
			{
				FMBPPanelSlot& Slot = ImportedSlots[(Row * ImportedColumns) + Column];
				Slot.Style = PanelStyle;
				Slot.ShimmerVariant = ShimmerVariant;
				Slot.DepthOffsetCm = static_cast<float>(DepthIn) * 2.54f;
			}
		}
	}
	else if (Root->HasField(TEXT("cells")))
	{
		OutError = TEXT("Layout cells must be a list.");
		return false;
	}

	Modify();
	Columns = ImportedColumns;
	Rows = ImportedRows;
	PanelWidthCm = 91.44f;
	PanelHeightCm = 91.44f;
	DepthOffsetStepCm = 30.48f;
	SupportInterval = ImportedSupportInterval;
	bLeftCap = false;
	bRightCap = false;
	Root->TryGetBoolField(TEXT("left_cap"), bLeftCap);
	Root->TryGetBoolField(TEXT("right_cap"), bRightCap);
	ImportedLayoutVersion = Version;
	PanelSlots = MoveTemp(ImportedSlots);
	bHasCachedDefaultStyle = true;
	CachedDefaultStyle = DefaultStyle;
	CachedDefaultShimmerVariant = DefaultShimmerVariant;
	CachedDefaultShimmerMaterial = DefaultShimmerMaterial;
	RebuildWall();
	MarkPackageDirty();
	return true;
}

bool AMBPWallActor::ImportWebLayoutFromJsonFile(const FString& FilePath, FString& OutError)
{
	FString JsonText;
	if (!FFileHelper::LoadFileToString(JsonText, *FilePath))
	{
		OutError = FString::Printf(TEXT("Could not read JSON file: %s"), *FilePath);
		return false;
	}
	if (!ImportWebLayoutFromJson(JsonText, OutError))
	{
		return false;
	}
	ImportedJsonFile = FPaths::ConvertRelativePathToFull(FilePath);
	return true;
}

void AMBPWallActor::ImportWebLayoutWithFilePicker()
{
#if WITH_EDITOR
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("The file picker is unavailable.")));
		return;
	}

	TArray<FString> SelectedFiles;
	const FString StartDirectory = ImportedJsonFile.IsEmpty() ? FPaths::ProjectDir() : FPaths::GetPath(ImportedJsonFile);
	if (!DesktopPlatform->OpenFileDialog(nullptr, TEXT("Import MBP Web Layout"), StartDirectory, TEXT(""),
		TEXT("MBP Layout JSON (*.json)|*.json"), EFileDialogFlags::None, SelectedFiles) || SelectedFiles.IsEmpty())
	{
		return;
	}

	FString Error;
	if (!ImportWebLayoutFromJsonFile(SelectedFiles[0], Error))
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Error), FText::FromString(TEXT("MBP Import Failed")));
	}
#endif
}

bool AMBPWallActor::GetSlotIndicesFromWorldLocation(const FVector& WorldLocation, int32& OutRow, int32& OutColumn) const
{
	const float StepX = PanelWidthCm + HorizontalSpacingCm;
	const float StepZ = PanelHeightCm + VerticalSpacingCm;
	if (StepX <= KINDA_SMALL_NUMBER || StepZ <= KINDA_SMALL_NUMBER || Columns <= 0 || Rows <= 0)
	{
		return false;
	}

	const float OriginX = bCenterOnActor ? (-0.5f * (Columns - 1) * StepX) : 0.0f;
	const float OriginZ = bCenterOnActor ? (-0.5f * (Rows - 1) * StepZ) : 0.0f;
	const FVector LocalHitLocation = GetActorTransform().InverseTransformPosition(WorldLocation);

	OutColumn = FMath::Clamp(FMath::RoundToInt((LocalHitLocation.X - OriginX) / StepX), 0, Columns - 1);
	OutRow = FMath::Clamp(FMath::RoundToInt((LocalHitLocation.Z - OriginZ) / StepZ), 0, Rows - 1);
	return IsValidSlotIndexPair(OutRow, OutColumn);
}

bool AMBPWallActor::GetPanelSlot(int32 RowIndex, int32 ColumnIndex, FMBPPanelSlot& OutSlot) const
{
	if (!IsValidSlotIndexPair(RowIndex, ColumnIndex))
	{
		return false;
	}

	OutSlot = PanelSlots[GetSlotLinearIndex(RowIndex, ColumnIndex)];
	return true;
}

bool AMBPWallActor::ApplyPanelEdit(int32 RowIndex, int32 ColumnIndex, EMBPPanelStyle Style, float DepthOffsetCm)
{
	if (!IsValidSlotIndexPair(RowIndex, ColumnIndex))
	{
		return false;
	}

	return ApplyEditToSlotIndices({ GetSlotLinearIndex(RowIndex, ColumnIndex) }, Style, DepthOffsetCm);
}

bool AMBPWallActor::ApplyRowEditByIndex(int32 RowIndex, EMBPPanelStyle Style, float DepthOffsetCm)
{
	if (RowIndex < 0 || RowIndex >= Rows)
	{
		return false;
	}

	TArray<int32> SlotIndices;
	for (int32 ColumnIndex = 0; ColumnIndex < Columns; ++ColumnIndex)
	{
		SlotIndices.Add(GetSlotLinearIndex(RowIndex, ColumnIndex));
	}

	return ApplyEditToSlotIndices(SlotIndices, Style, DepthOffsetCm);
}

bool AMBPWallActor::ApplyColumnEditByIndex(int32 ColumnIndex, EMBPPanelStyle Style, float DepthOffsetCm)
{
	if (ColumnIndex < 0 || ColumnIndex >= Columns)
	{
		return false;
	}

	TArray<int32> SlotIndices;
	for (int32 RowIndex = 0; RowIndex < Rows; ++RowIndex)
	{
		SlotIndices.Add(GetSlotLinearIndex(RowIndex, ColumnIndex));
	}

	return ApplyEditToSlotIndices(SlotIndices, Style, DepthOffsetCm);
}

void AMBPWallActor::SetSelectionHighlighted(bool bHighlighted)
{
	if (SelectionBounds)
	{
		SelectionBounds->SetHiddenInGame(!bHighlighted);
	}
}

void AMBPWallActor::ResetSlotsToDefault()
{
	EnsureSlotCount(true);
	RebuildWall();
}

void AMBPWallActor::ResizeSlotsToGrid()
{
	EnsureSlotCount(false);
	RebuildWall();
}

void AMBPWallActor::ApplyBatchEdit()
{
	for (const int32 SlotIndex : GetBatchSlotIndices())
	{
		if (!PanelSlots.IsValidIndex(SlotIndex))
		{
			continue;
		}

		FMBPPanelSlot& Slot = PanelSlots[SlotIndex];
		Slot.Style = BatchStyle;
		Slot.ShimmerVariant = BatchShimmerVariant;
		Slot.ShimmerMaterial = BatchShimmerMaterial;
		Slot.DepthOffsetCm = BatchDepthOffsetCm;
		if (BatchStyle != EMBPPanelStyle::Custom)
		{
			Slot.CustomStaticMeshes.Reset();
		}
	}

	CachedDefaultStyle = DefaultStyle;
	CachedDefaultShimmerVariant = DefaultShimmerVariant;
	CachedDefaultShimmerMaterial = DefaultShimmerMaterial;
	bHasCachedDefaultStyle = true;
	RebuildWall();
}

void AMBPWallActor::EnsureSlotCount(bool bResetNewSlotsToDefault)
{
	const int32 TargetCount = FMath::Max(1, Columns) * FMath::Max(1, Rows);
	const int32 PreviousCount = PanelSlots.Num();
	PanelSlots.SetNum(TargetCount);

	for (int32 Index = 0; Index < TargetCount; ++Index)
	{
		if (bResetNewSlotsToDefault || Index >= PreviousCount)
		{
			PanelSlots[Index] = MakeDefaultSlot();
		}
	}
}

FMBPPanelSlot AMBPWallActor::MakeDefaultSlot() const
{
	FMBPPanelSlot Slot;
	Slot.Style = DefaultStyle;
	Slot.ShimmerVariant = DefaultShimmerVariant;
	Slot.ShimmerMaterial = DefaultShimmerMaterial;
	return Slot;
}

void AMBPWallActor::SyncSlotsToDefaultStyleIfNeeded()
{
	if (!bHasCachedDefaultStyle)
	{
		bHasCachedDefaultStyle = true;
		CachedDefaultStyle = DefaultStyle;
		CachedDefaultShimmerVariant = DefaultShimmerVariant;
		CachedDefaultShimmerMaterial = DefaultShimmerMaterial;
		return;
	}

	if (CachedDefaultStyle == DefaultStyle &&
		CachedDefaultShimmerVariant == DefaultShimmerVariant &&
		CachedDefaultShimmerMaterial == DefaultShimmerMaterial)
	{
		return;
	}

	bool bSlotsStillMatchCachedDefaults = true;
	for (const FMBPPanelSlot& Slot : PanelSlots)
	{
		if (Slot.Style != CachedDefaultStyle)
		{
			bSlotsStillMatchCachedDefaults = false;
			break;
		}

		if (CachedDefaultStyle == EMBPPanelStyle::Shimmer && Slot.ShimmerVariant != CachedDefaultShimmerVariant)
		{
			bSlotsStillMatchCachedDefaults = false;
			break;
		}

		if (CachedDefaultStyle == EMBPPanelStyle::Shimmer && Slot.ShimmerMaterial != CachedDefaultShimmerMaterial)
		{
			bSlotsStillMatchCachedDefaults = false;
			break;
		}
	}

	if (bSlotsStillMatchCachedDefaults)
	{
		ApplyDefaultStyleToAllSlots();
	}

	CachedDefaultStyle = DefaultStyle;
	CachedDefaultShimmerVariant = DefaultShimmerVariant;
	CachedDefaultShimmerMaterial = DefaultShimmerMaterial;
}

void AMBPWallActor::ApplyDefaultStyleToAllSlots()
{
	for (FMBPPanelSlot& Slot : PanelSlots)
	{
		Slot.Style = DefaultStyle;
		Slot.ShimmerVariant = DefaultShimmerVariant;
		Slot.ShimmerMaterial = DefaultShimmerMaterial;
	}
}

TArray<int32> AMBPWallActor::GetBatchSlotIndices() const
{
	TArray<int32> SlotIndices;

	if (BatchEditAxis == EMBPBatchEditAxis::Row)
	{
		if (BatchTargetIndex < 0 || BatchTargetIndex >= Rows)
		{
			return SlotIndices;
		}

		for (int32 ColumnIndex = 0; ColumnIndex < Columns; ++ColumnIndex)
		{
			SlotIndices.Add((BatchTargetIndex * Columns) + ColumnIndex);
		}

		return SlotIndices;
	}

	if (BatchTargetIndex < 0 || BatchTargetIndex >= Columns)
	{
		return SlotIndices;
	}

	for (int32 RowIndex = 0; RowIndex < Rows; ++RowIndex)
	{
		SlotIndices.Add((RowIndex * Columns) + BatchTargetIndex);
	}

	return SlotIndices;
}

bool AMBPWallActor::IsValidSlotIndexPair(int32 RowIndex, int32 ColumnIndex) const
{
	return RowIndex >= 0 && RowIndex < Rows && ColumnIndex >= 0 && ColumnIndex < Columns && PanelSlots.IsValidIndex(GetSlotLinearIndex(RowIndex, ColumnIndex));
}

int32 AMBPWallActor::GetSlotLinearIndex(int32 RowIndex, int32 ColumnIndex) const
{
	return (RowIndex * Columns) + ColumnIndex;
}

void AMBPWallActor::ApplyStyleAndDepthToSlot(FMBPPanelSlot& Slot, EMBPPanelStyle Style, float DepthOffsetCm)
{
	Slot.Style = Style;
	Slot.DepthOffsetCm = DepthOffsetCm;

	if (Style == EMBPPanelStyle::Shimmer)
	{
		Slot.ShimmerVariant = DefaultShimmerVariant;
		if (Slot.ShimmerMaterial.IsNull())
		{
			Slot.ShimmerMaterial = DefaultShimmerMaterial;
		}
	}

	if (Style != EMBPPanelStyle::Custom)
	{
		Slot.CustomStaticMeshes.Reset();
	}
}

bool AMBPWallActor::ApplyEditToSlotIndices(const TArray<int32>& SlotIndices, EMBPPanelStyle Style, float DepthOffsetCm)
{
	bool bChanged = false;
	for (const int32 SlotIndex : SlotIndices)
	{
		if (!PanelSlots.IsValidIndex(SlotIndex))
		{
			continue;
		}

		ApplyStyleAndDepthToSlot(PanelSlots[SlotIndex], Style, DepthOffsetCm);
		bChanged = true;
	}

	if (bChanged)
	{
		RebuildWall();
	}

	return bChanged;
}

float AMBPWallActor::GetSnappedDepthOffsetCm(float RawDepthOffsetCm) const
{
	if (DepthOffsetStepCm <= KINDA_SMALL_NUMBER)
	{
		return RawDepthOffsetCm;
	}

	return FMath::RoundToFloat(RawDepthOffsetCm / DepthOffsetStepCm) * DepthOffsetStepCm;
}

TArray<FSoftObjectPath> AMBPWallActor::GetMeshPathsForFolder(const FString& AssetFolderPath) const
{
	TArray<FSoftObjectPath> MeshPaths;

	if (AssetFolderPath.IsEmpty())
	{
		return MeshPaths;
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> Assets;
	AssetRegistryModule.Get().GetAssetsByPath(*AssetFolderPath, Assets, false);

	Assets.StableSort([](const FAssetData& A, const FAssetData& B)
	{
		return A.AssetName.LexicalLess(B.AssetName);
	});

	for (const FAssetData& AssetData : Assets)
	{
		if (IsMBPStaticMeshAsset(AssetData))
		{
			MeshPaths.Add(AssetData.ToSoftObjectPath());
		}
	}

	return MeshPaths;
}

TArray<FSoftObjectPath> AMBPWallActor::GetMeshPathsForStyle(EMBPPanelStyle Style) const
{
	return GetMeshPathsForFolder(GetStyleAssetFolder(Style));
}

TArray<FSoftObjectPath> AMBPWallActor::GetMeshPathsForSlot(const FMBPPanelSlot& Slot) const
{
	if (Slot.Style == EMBPPanelStyle::Empty)
	{
		return {};
	}

	if (Slot.Style == EMBPPanelStyle::Custom)
	{
		TArray<FSoftObjectPath> MeshPaths;
		for (const TSoftObjectPtr<UStaticMesh>& Mesh : Slot.CustomStaticMeshes)
		{
			if (!Mesh.IsNull())
			{
				MeshPaths.Add(Mesh.ToSoftObjectPath());
			}
		}

		return MeshPaths;
	}

	if (Slot.Style == EMBPPanelStyle::Shimmer)
	{
		return GetShimmerFrameMeshPaths();
	}

	return GetMeshPathsForStyle(Slot.Style);
}

TArray<FSoftObjectPath> AMBPWallActor::GetShimmerFrameMeshPaths() const
{
	TArray<FSoftObjectPath> MeshPaths;
	for (const FSoftObjectPath& MeshPath : GetMeshPathsForStyle(EMBPPanelStyle::Shimmer))
	{
		const FString AssetName = MeshPath.GetAssetName();
		if (AssetName.Equals(TEXT("Plane")) || AssetName.StartsWith(TEXT("Shimmer__")))
		{
			continue;
		}

		MeshPaths.Add(MeshPath);
	}

	return MeshPaths;
}

FSoftObjectPath AMBPWallActor::GetShimmerPlaneMeshPath() const
{
	for (const FSoftObjectPath& MeshPath : GetMeshPathsForStyle(EMBPPanelStyle::Shimmer))
	{
		if (MeshPath.GetAssetName().Equals(TEXT("Plane")))
		{
			return MeshPath;
		}
	}

	return FSoftObjectPath();
}

TArray<FSoftObjectPath> AMBPWallActor::GetExtraFrameMeshPaths() const
{
	return GetMeshPathsForFolder(GetExtraFrameAssetFolder());
}

UMaterialInterface* AMBPWallActor::LoadShimmerMaterialOverride(EMBPShimmerVariant Variant) const
{
	const FString AssetPath = GetShimmerMaterialPath(Variant);
	return AssetPath.IsEmpty() ? nullptr : LoadObject<UMaterialInterface>(nullptr, *AssetPath);
}

UMaterialInterface* AMBPWallActor::ResolveShimmerMaterial(const FMBPPanelSlot& Slot) const
{
	if (!Slot.ShimmerMaterial.IsNull())
	{
		return Slot.ShimmerMaterial.LoadSynchronous();
	}

	if (!DefaultShimmerMaterial.IsNull())
	{
		return DefaultShimmerMaterial.LoadSynchronous();
	}

	return LoadShimmerMaterialOverride(Slot.ShimmerVariant);
}

UInstancedStaticMeshComponent* AMBPWallActor::FindOrCreateComponentBucket(
	const FSoftObjectPath& MeshPath,
	const FMBPPanelSlot& Slot,
	TMap<FString, UInstancedStaticMeshComponent*>& BucketMap)
{
	const FString VariantSuffix = Slot.Style == EMBPPanelStyle::Shimmer
		? FString::Printf(
			TEXT("_%s"),
			*(
				!Slot.ShimmerMaterial.IsNull()
					? Slot.ShimmerMaterial.ToSoftObjectPath().GetAssetName()
					: (!DefaultShimmerMaterial.IsNull()
						? DefaultShimmerMaterial.ToSoftObjectPath().GetAssetName()
						: FString::FromInt(static_cast<int32>(Slot.ShimmerVariant)))))
		: FString();
	const FString BucketKey = MeshPath.ToString() + VariantSuffix;

	if (UInstancedStaticMeshComponent** ExistingComponent = BucketMap.Find(BucketKey))
	{
		return *ExistingComponent;
	}

	UStaticMesh* StaticMesh = Cast<UStaticMesh>(MeshPath.TryLoad());
	if (!StaticMesh)
	{
		return nullptr;
	}

	const FName ComponentName(*FString::Printf(TEXT("MBP_%s%s"), *StaticMesh->GetName(), *VariantSuffix));
	UInstancedStaticMeshComponent* MeshComponent = FindGeneratedComponentByName(ComponentName);
	if (!MeshComponent)
	{
		MeshComponent = NewObject<UInstancedStaticMeshComponent>(this, ComponentName);
		if (!MeshComponent)
		{
			return nullptr;
		}

		MeshComponent->SetupAttachment(SceneRoot);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComponent->RegisterComponent();
		AddInstanceComponent(MeshComponent);
		GeneratedInstanceComponents.Add(MeshComponent);
	}

	MeshComponent->SetStaticMesh(StaticMesh);
	MeshComponent->SetHiddenInGame(false);
	MeshComponent->SetVisibility(true);

	if (Slot.Style == EMBPPanelStyle::Shimmer && StaticMesh->GetName().Equals(TEXT("Plane")))
	{
		if (UMaterialInterface* OverrideMaterial = ResolveShimmerMaterial(Slot))
		{
			MeshComponent->SetMaterial(0, OverrideMaterial);
		}
	}

	BucketMap.Add(BucketKey, MeshComponent);

	return MeshComponent;
}

void AMBPWallActor::ClearGeneratedComponents()
{
	TInlineComponentArray<UInstancedStaticMeshComponent*> TrackedInstanceComponents(this);
	for (UInstancedStaticMeshComponent* MeshComponent : TrackedInstanceComponents)
	{
		if (!MeshComponent)
		{
			continue;
		}

		if (MeshComponent->GetName().StartsWith(TEXT("MBP_")) && !GeneratedInstanceComponents.Contains(MeshComponent))
		{
			GeneratedInstanceComponents.Add(MeshComponent);
		}
	}

	for (UInstancedStaticMeshComponent* MeshComponent : GeneratedInstanceComponents)
	{
		if (!MeshComponent)
		{
			continue;
		}

		MeshComponent->ClearInstances();
		MeshComponent->SetVisibility(false);
		MeshComponent->SetHiddenInGame(true);
	}
}

UInstancedStaticMeshComponent* AMBPWallActor::FindGeneratedComponentByName(const FName& ComponentName) const
{
	for (UInstancedStaticMeshComponent* MeshComponent : GeneratedInstanceComponents)
	{
		if (MeshComponent && MeshComponent->GetFName() == ComponentName)
		{
			return MeshComponent;
		}
	}

	return FindObjectFast<UInstancedStaticMeshComponent>(const_cast<AMBPWallActor*>(this), ComponentName);
}

void AMBPWallActor::UpdateSelectionBounds(const FBox& Bounds)
{
	if (!SelectionBounds)
	{
		return;
	}

	if (!Bounds.IsValid)
	{
		SelectionBounds->SetBoxExtent(FVector(50.0f, 50.0f, 50.0f));
		SelectionBounds->SetRelativeLocation(FVector::ZeroVector);
		return;
	}

	SelectionBounds->SetBoxExtent(Bounds.GetExtent() + FVector(5.0f));
	SelectionBounds->SetRelativeLocation(Bounds.GetCenter());
}
