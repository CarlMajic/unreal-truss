#include "WhiteComboBoxString.h"

UWhiteComboBoxString::UWhiteComboBoxString(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InitForegroundColor(FSlateColor(FLinearColor::White));
}
