#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MajicEventLayoutActor.generated.h"

// Place one of these in a level, then use its Import / Export buttons in the Details panel.
// Import builds every stage, truss, drape and MBP wall in a Majic Event Layout file at once.
// Export writes the ones in the level back, keeping every other item the file holds.
UCLASS(BlueprintType)
class MAJICTRUSSRUNTIME_API AMajicEventLayoutActor : public AActor
{
	GENERATED_BODY()

public:
	AMajicEventLayoutActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Event Layout")
	TObjectPtr<USceneComponent> SceneRoot;

	// The file used by the last import or export; the file pickers start here.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Layout", meta = (FilePathFilter = "json"))
	FString LayoutFile;

	// Builds the items in the file. Items whose id is already in the level are skipped.
	UFUNCTION(BlueprintCallable, Category = "Event Layout")
	bool ImportLayoutFile(const FString& FilePath, TArray<FString>& OutLines);

	// Writes the level's stages, trusses, drapes and MBP walls into the file.
	UFUNCTION(BlueprintCallable, Category = "Event Layout")
	bool ExportLayoutFile(const FString& FilePath, TArray<FString>& OutLines);

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Event Layout", meta = (DisplayName = "Import Event Layout"))
	void ImportEventLayout();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Event Layout", meta = (DisplayName = "Export Event Layout"))
	void ExportEventLayout();
};
