#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TrussInventoryDataAsset.h"
#include "TrussMathLibrary.generated.h"

USTRUCT(BlueprintType)
struct FTrussCombinationResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Truss")
	TArray<ETrussPieceType> Pieces;

	UPROPERTY(BlueprintReadOnly, Category = "Truss", meta = (Units = "cm"))
	float ActualLengthCm = 0.0f;
};

UCLASS()
class MAJICTRUSSRUNTIME_API UTrussMathLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Truss|Math")
	static float FeetToCentimeters(float Feet);

	UFUNCTION(BlueprintPure, Category = "Truss|Math")
	static float CentimetersToFeet(float Centimeters);

	UFUNCTION(BlueprintCallable, Category = "Truss|Math")
	static FTrussCombinationResult FindBestTrussCombination(float TargetLengthCm, float ToleranceCm = 0.1f);

	UFUNCTION(BlueprintPure, Category = "Truss|Math")
	static FString PieceTypeToLabel(ETrussPieceType PieceType);

	UFUNCTION(BlueprintPure, Category = "Truss|Math")
	static float GetDefaultPieceLengthCm(ETrussPieceType PieceType);

private:
	static bool FindExact(float TargetLengthCm, const TArray<ETrussPieceType>& CandidatePieces, float ToleranceCm, TArray<ETrussPieceType>& OutPieces);
};
