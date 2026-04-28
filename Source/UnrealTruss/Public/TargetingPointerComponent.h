#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TargetingPointerComponent.generated.h"

class APlayerController;
class ATrussStructureActor;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ETargetingPointerMode : uint8
{
	Disabled,
	WorldPlacement,
	TrussSelection
};

UCLASS(ClassGroup = (UnrealTruss), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALTRUSS_API UTargetingPointerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTargetingPointerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pointer", meta = (ClampMin = "100.0", Units = "cm"))
	float TraceDistanceCm = 50000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pointer", meta = (ClampMin = "0.1", Units = "cm"))
	float BeamThicknessCm = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pointer", meta = (ClampMin = "0.1", Units = "cm"))
	float MarkerDiameterCm = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pointer", meta = (Units = "cm"))
	FVector VisualOriginOffset = FVector(30.0f, 0.0f, -18.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Pointer")
	bool bHasValidHit = false;

	UPROPERTY(BlueprintReadOnly, Category = "Pointer")
	FHitResult CurrentHitResult;

	UPROPERTY(BlueprintReadOnly, Category = "Pointer")
	TObjectPtr<ATrussStructureActor> CurrentTrussActor = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Pointer")
	void UpdatePointer(
		APlayerController* PlayerController,
		ETargetingPointerMode Mode,
		AActor* PrimaryIgnoredActor = nullptr,
		AActor* SecondaryIgnoredActor = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Pointer")
	void HidePointer();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> BeamMeshComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> MarkerMeshComponent = nullptr;

	void EnsureVisualComponents();
	void UpdateVisuals(const FVector& Start, const FVector& End, bool bShowMarker);
	void DrawPointerDebug(const FVector& Start, const FVector& End, bool bHitTarget) const;
	bool TraceWorldPlacement(APlayerController* PlayerController, AActor* PrimaryIgnoredActor, AActor* SecondaryIgnoredActor, FHitResult& OutHitResult);
	bool TraceTrussSelection(APlayerController* PlayerController, AActor* PrimaryIgnoredActor, AActor* SecondaryIgnoredActor, FHitResult& OutHitResult, ATrussStructureActor*& OutTrussActor);
	void ConfigureMeshAppearance(UStaticMeshComponent* MeshComponent, const FLinearColor& Color) const;
};
