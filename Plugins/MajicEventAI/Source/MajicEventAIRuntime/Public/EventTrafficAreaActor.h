#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NavAreas/NavArea.h"
#include "EventTrafficAreaActor.generated.h"

class UBoxComponent;
class UNavModifierComponent;

UCLASS()
class MAJICEVENTAIRUNTIME_API UEventNavArea_PreferredPathway : public UNavArea
{
	GENERATED_BODY()

public:
	UEventNavArea_PreferredPathway(const FObjectInitializer& ObjectInitializer);
};

UCLASS()
class MAJICEVENTAIRUNTIME_API UEventNavArea_CrowdedSlowZone : public UNavArea
{
	GENERATED_BODY()

public:
	UEventNavArea_CrowdedSlowZone(const FObjectInitializer& ObjectInitializer);
};

UENUM(BlueprintType)
enum class EEventTrafficAreaType : uint8
{
	PreferredPathway UMETA(DisplayName = "Preferred Pathway"),
	CrowdedSlowZone UMETA(DisplayName = "Crowded Slow Zone"),
	NoThroughZone UMETA(DisplayName = "No Through Zone")
};

struct FEventTrafficRouteResult
{
	FVector Waypoint = FVector::ZeroVector;
	FString AreaName;
	float DirectPathLength = 0.0f;
	float RouteLength = 0.0f;
	float DetourRatio = 0.0f;
	int32 ConsideredAreas = 0;
	int32 RejectedProjectionCount = 0;
	int32 RejectedPathCount = 0;
	int32 RejectedNoThroughCount = 0;
	int32 RejectedDetourCount = 0;
};

UCLASS(BlueprintType, meta = (DisplayName = "Event Traffic Area"))
class MAJICEVENTAIRUNTIME_API AEventTrafficAreaActor : public AActor
{
	GENERATED_BODY()

public:
	AEventTrafficAreaActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic")
	TObjectPtr<UBoxComponent> TrafficBounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traffic|Navigation")
	TObjectPtr<UNavModifierComponent> NavModifier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic")
	EEventTrafficAreaType AreaType = EEventTrafficAreaType::PreferredPathway;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic", meta = (ClampMin = "0.0"))
	float Priority = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic|Navigation")
	bool bAffectNavigationMesh = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic|Navigation")
	TSubclassOf<UNavArea> PreferredPathwayNavAreaClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic|Navigation")
	TSubclassOf<UNavArea> CrowdedSlowZoneNavAreaClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic|Navigation")
	TSubclassOf<UNavArea> NoThroughZoneNavAreaClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic|Debug")
	bool bDrawDebugInEditor = true;

	UFUNCTION(BlueprintPure, Category = "Traffic")
	bool ContainsWorldLocation(const FVector& WorldLocation) const;

	UFUNCTION(BlueprintPure, Category = "Traffic")
	FVector GetClosestWorldLocation(const FVector& WorldLocation) const;

	static float CalculateTrafficAreaScore(UWorld* World, const FVector& WorldLocation);
	static float CalculateTrafficPathScore(UWorld* World, const FVector& FromLocation, const FVector& ToLocation, FString* OutAreaNames = nullptr, FString* OutNoThroughAreaNames = nullptr);
	static bool IsRouteBlockedByNoThroughZone(UWorld* World, const FVector& FromLocation, const FVector& ToLocation, FString* OutAreaNames = nullptr);
	static bool FindPreferredTrafficWaypoint(UWorld* World, const FVector& FromLocation, const FVector& ToLocation, float MinRouteDistance, float MaxDetourRatio, FVector& OutWaypoint, FEventTrafficRouteResult* OutResult = nullptr);

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	void UpdateNavigationModifier();
};
