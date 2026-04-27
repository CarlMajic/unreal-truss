#include "UnrealTrussGameMode.h"

#include "UnrealTrussBuildPawn.h"

AUnrealTrussGameMode::AUnrealTrussGameMode()
{
	DefaultPawnClass = AUnrealTrussBuildPawn::StaticClass();
}
