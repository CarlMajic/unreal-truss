#include "WhiteComboBoxString.h"

UWhiteComboBoxString::UWhiteComboBoxString(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	const FSlateColor WhiteText(FLinearColor::White);
	InitForegroundColor(WhiteText);

	FTableRowStyle WhiteItemStyle = GetItemStyle();
	WhiteItemStyle.SetTextColor(WhiteText);
	WhiteItemStyle.SetSelectedTextColor(WhiteText);
	SetItemStyle(WhiteItemStyle);
}
