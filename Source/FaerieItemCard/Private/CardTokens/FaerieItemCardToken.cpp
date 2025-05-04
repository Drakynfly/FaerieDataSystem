// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "CardTokens/FaerieItemCardToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemCardToken)

TSoftClassPtr<UFaerieCardBase> UFaerieItemCardToken::GetCardClass(const FFaerieItemCardType Tag) const
{
	for (FFaerieItemCardType Check = Tag;
		 Check.IsValid() && Check != FFaerieItemCardType::GetRootTag();
		 Check = FFaerieItemCardType::ConvertChecked(Check.RequestDirectParent()))
	{
		if (auto&& Class = CardClasses.Find(Check))
		{
			return *Class;
		}
	}

	return nullptr;
}
