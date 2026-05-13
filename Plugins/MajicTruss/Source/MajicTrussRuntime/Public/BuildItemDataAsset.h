#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DrapeRunActor.h"
#include "MBPWallActor.h"
#include "StageDeckBuildDefinition.h"
#include "TrussStructureActor.h"
#include "VideoPlacementActor.h"
#include "ProjectionScreenActor.h"
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
	ProjectionScreen UMETA(DisplayName = "Projection Screen")
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Build|Projection")
	FProjectionScreenBuildDefinition DefaultProjectionScreenDefinition;
};
