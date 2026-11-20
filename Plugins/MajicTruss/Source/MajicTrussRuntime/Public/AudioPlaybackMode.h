#pragma once

#include "CoreMinimal.h"
#include "AudioPlaybackMode.generated.h"

UENUM(BlueprintType)
enum class EAudioPlaybackMode : uint8
{
	SoundAsset UMETA(DisplayName = "Sound Asset"),
	ProjectorVideo UMETA(DisplayName = "Projector Video"),
	NoAudio UMETA(DisplayName = "No Audio")
};
