// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "CardTokens/FaerieItemCardToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemCardToken)

TSoftClassPtr<UFaerieCardBase> UFaerieItemCardToken::GetCardClass(const FFaerieItemCardType Tag) const
{
	// @todo tag loop
	if (auto Class = CardClasses.Find(Tag))
	{
		return *Class;
	}
	return nullptr;
}
