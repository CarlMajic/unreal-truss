#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TrussInventoryDataAsset.generated.h"

UENUM(BlueprintType)
enum class ETrussPieceType : uint8
{
	TenFoot UMETA(DisplayName = "10 ft Truss"),
	EightFoot UMETA(DisplayName = "8 ft Truss"),
	FiveFoot UMETA(DisplayName = "5 ft Truss"),
	FourFoot UMETA(DisplayName = "4 ft Truss"),
	TwoFoot UMETA(DisplayName = "2 ft Truss"),
	CornerBlock UMETA(DisplayName = "Corner Block"),
	Base UMETA(DisplayName = "Base"),
	Hinge UMETA(DisplayName = "Hinge")
};

USTRUCT(BlueprintType)
struct FTrussPieceDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Truss")
	ETrussPieceType PieceType = ETrussPieceType::TenFoot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Truss", meta = (ClampMin = "0.0", Units = "cm"))
	float LengthCm = 304.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Truss")
	TObjectPtr<UStaticMesh> StaticMesh = nullptr;
};

UCLASS(BlueprintType)
class MAJICTRUSSRUNTIME_API UTrussInventoryDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UTrussInventoryDataAsset();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Truss")
	TArray<FTrussPieceDefinition> Pieces;

	bool FindPiece(ETrussPieceType PieceType, FTrussPieceDefinition& OutPiece) const;
};

