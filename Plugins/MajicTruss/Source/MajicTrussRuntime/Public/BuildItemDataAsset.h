#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DrapeRunActor.h"
#include "AudioGroundLineArrayActor.h"
#include "AudioGroundSpeakerActor.h"
#include "BarLayoutActor.h"
#include "BarSeatingActor.h"
#include "CafeTableActor.h"
#include "DiningTableActor.h"
#include "LoungeLayoutActor.h"
#include "MBPWallActor.h"
#include "StageDeckBuildDefinition.h"
#include "TrussStructureActor.h"
#include "VideoPlacementActor.h"
#include "VideoWallActor.h"
#include "ProjectionScreenActor.h"
#include "SectionalLayoutActor.h"
#include "BuildItemDataAsset.generated.h"

class UTexture2D;

UENUM(BlueprintType)
enum class EBuildItemType : uint8
{
	ActorClass UMETA(DisplayName = "Actor Class"),
	TrussStructure UMETA(DisplayName = "Truss Structure"),
	MBPWall UMETA(DisplayName = "MBP Wall"),
	StageDeck UMETA(DisplayName = "Stage Deck"),
	DrapeRun UMETA(DisplayName = "Drape Run"),
	VideoPlacement UMETA(DisplayName = "TV Placement"),
	VideoWall UMETA(DisplayName = "Video Wall"),
	ProjectionScreen UMETA(DisplayName = "Projection Screen"),
	AudioPlacement UMETA(DisplayName = "Audio Placement"),
	LoungeLayout UMETA(DisplayName = "Lounge Layout"),
	SectionalLayout UMETA(DisplayName = "Sectional Layout"),
	FurnitureSeating UMETA(DisplayName = "Furniture Seating"),
	BarLayout UMETA(DisplayName = "Bar Layout"),
	DiningTable UMETA(DisplayName = "Dining Table")
};

UENUM(BlueprintType)
enum class EAudioPlacementRuntimeType : uint8
{
	GroundSpeaker UMETA(DisplayName = "Ground Speaker"),
	GroundLineArray UMETA(DisplayName = "Ground Line Array")
};

USTRUCT(BlueprintType)
struct FAudioPlacementBuildDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	EAudioPlacementRuntimeType PlacementType = EAudioPlacementRuntimeType::GroundSpeaker;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	FAudioGroundSpeakerBuildDefinition GroundSpeakerDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	FAudioGroundLineArrayBuildDefinition GroundLineArrayDefinition;
};

UENUM(BlueprintType)
enum class EFurnitureSeatingRuntimeType : uint8
{
	BarSeating UMETA(DisplayName = "Bar Seating"),
	CafeTable UMETA(DisplayName = "Cafe Seating")
};

USTRUCT(BlueprintType)
struct FFurnitureSeatingBuildDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furniture Seating")
	EFurnitureSeatingRuntimeType SeatingType = EFurnitureSeatingRuntimeType::BarSeating;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furniture Seating")
	FBarSeatingBuildDefinition BarSeatingDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furniture Seating")
	FCafeTableBuildDefinition CafeTableDefinition;
};

UCLASS(BlueprintType)
class MAJICTRUSSRUNTIME_API UBuildItemDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build")
	FName ItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build")
	FName Category = TEXT("General");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build")
	EBuildItemType ItemType = EBuildItemType::ActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build")
	TSubclassOf<AActor> BuildActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build|Placement", meta = (ClampMin = "0.0", Units = "cm"))
	float GridSnapSizeCm = 30.48f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build|Placement", meta = (ClampMin = "0.0", Units = "deg"))
	float RotationStepDegrees = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build|Placement")
	bool bUseGridSnap = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build|Placement")
	bool bAlignToSurfaceNormal = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build|Truss")
	FTrussBuildDefinition DefaultTrussDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build|MBP")
	FMBPWallDefinition DefaultMBPWallDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build|Stage")
	FStageDeckBuildDefinition DefaultStageDeckDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build|Drape")
	FDrapeRunBuildDefinition DefaultDrapeRunDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build|Video")
	FVideoPlacementBuildDefinition DefaultVideoPlacementDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build|Video Wall")
	FVideoWallBuildDefinition DefaultVideoWallDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build|Projection")
	FProjectionScreenBuildDefinition DefaultProjectionScreenDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build|Audio")
	FAudioPlacementBuildDefinition DefaultAudioPlacementDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build|Lounge")
	FLoungeLayoutBuildDefinition DefaultLoungeLayoutDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build|Sectional")
	FSectionalLayoutBuildDefinition DefaultSectionalLayoutDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build|Furniture Seating")
	FFurnitureSeatingBuildDefinition DefaultFurnitureSeatingDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build|Bar")
	FBarLayoutBuildDefinition DefaultBarLayoutDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build|Dining")
	FDiningTableBuildDefinition DefaultDiningTableDefinition;
};
