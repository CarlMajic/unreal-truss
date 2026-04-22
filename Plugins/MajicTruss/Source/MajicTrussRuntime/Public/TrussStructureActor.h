#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrussInventoryDataAsset.h"
#include "TrussStructureActor.generated.h"

class UBoxComponent;
class UInstancedStaticMeshComponent;

UCLASS(BlueprintType)
class MAJICTRUSSRUNTIME_API ATrussStructureActor : public AActor
{
	GENERATED_BODY()

public:
	ATrussStructureActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Truss")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Truss")
	TObjectPtr<UTrussInventoryDataAsset> Inventory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Truss")
	bool bUseMajicGearDefaultMeshes = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Truss", meta = (ClampMin = "0.0001"))
	float MeshScaleMultiplier = 0.0254f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Truss")
	bool bFlipShortMajicGearTrussSections = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Straight Run", meta = (ClampMin = "2.0", Units = "ft"))
	float LengthFt = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Straight Run")
	bool bBuildOnConstruction = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (ClampMin = "1.0", Units = "cm"))
	float DebugCrossSectionCm = 30.48f;

	UPROPERTY(BlueprintReadOnly, Category = "Truss", meta = (Units = "ft"))
	float LastBuiltLengthFt = 0.0f;

	UFUNCTION(BlueprintCallable, Category = "Truss")
	void BuildStraightRun();

	UFUNCTION(BlueprintCallable, Category = "Truss")
	void ClearGeneratedTruss();

	virtual void OnConstruction(const FTransform& Transform) override;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UActorComponent>> GeneratedComponents;

	UInstancedStaticMeshComponent* FindOrCreateMeshComponent(ETrussPieceType PieceType, UStaticMesh* StaticMesh);
	UStaticMesh* LoadMajicGearDefaultMesh(ETrussPieceType PieceType) const;
	FRotator GetMeshRotation(ETrussPieceType PieceType, bool bUsingMajicGearDefaultMesh) const;
	void AddDebugPiece(ETrussPieceType PieceType, float PieceLengthCm, float StartX);
};
