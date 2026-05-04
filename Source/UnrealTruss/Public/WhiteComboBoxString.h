#pragma once

#include "CoreMinimal.h"
#include "Components/ComboBoxString.h"
#include "WhiteComboBoxString.generated.h"

UCLASS()
class UNREALTRUSS_API UWhiteComboBoxString : public UComboBoxString
{
	GENERATED_BODY()

public:
	UWhiteComboBoxString(const FObjectInitializer& ObjectInitializer);
};
