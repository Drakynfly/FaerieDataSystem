// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Widgets/FaerieItemCardFragment.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemCardFragment)

TSoftClassPtr<UFaerieCardBase> FFaerieItemCardClassFragment::GetCardClass(const FFaerieItemCardType Tag) const
{
	for (FFaerieItemCardType Check = Tag;
		 Check.IsValid() && Check != FFaerieItemCardType::GetRootTag();
		 Check = FFaerieItemCardType::ConvertChecked(Check.RequestDirectParent()))
	{
		for (auto Class : Classes)
		{
			if (Class.CardType == Check)
			{
				return Class.CardClass;
			}
		}
	}

	return nullptr;
}