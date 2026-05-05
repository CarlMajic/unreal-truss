#pragma once

#include "CoreMinimal.h"
#include "StageDeckBuildDefinition.generated.h"

UENUM(BlueprintType)
enum class EStageDeckHeightPreset : uint8
{
	In8 UMETA(DisplayName = "8 Inch"),
	In12 UMETA(DisplayName = "12 Inch"),
	In24 UMETA(DisplayName = "24 Inch"),
	In27 UMETA(DisplayName = "27 Inch")
};

UENUM(BlueprintType)
enum class EStageDeckSurfaceStyle : uint8
{
	BlackTop UMETA(DisplayName = "Black Top"),
	GrayCarpet UMETA(DisplayName = "Gray Carpet")
};

USTRUCT(BlueprintType)
struct MAJICTRUSSRUNTIME_API FStageDeckBuildDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage", meta = (ClampMin = "1"))
	int32 Columns = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage", meta = (ClampMin = "1"))
	int32 Rows = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
	EStageDeckHeightPreset DefaultHeightPreset = EStageDeckHeightPreset::In24;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage")
	EStageDeckSurfaceStyle DefaultSurfaceStyle = EStageDeckSurfaceStyle::BlackTop;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Railing")
	bool bEnableFrontRailing = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Railing")
	bool bEnableBackRailing = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Railing")
	bool bEnableLeftRailing = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Railing")
	bool bEnableRightRailing = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Steps")
	bool bEnableLeftStep = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Steps")
	bool bEnableRightStep = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage|Skirt")
	bool bEnableAutomaticSkirt = false;
};
