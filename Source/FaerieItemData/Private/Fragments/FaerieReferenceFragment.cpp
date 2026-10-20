// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Fragments/FaerieReferenceFragment.h"
#include "FaerieItem.h"
#include "FaerieItemAsset.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieReferenceFragment)

const UFaerieItem* FFaerieReferenceFragment::GetReferencedItem(const FGameplayTag ReferenceTag, const bool MatchExact) const
{
	for (auto&& Reference : References)
	{
		if (MatchExact ? Reference.Tag.MatchesTagExact(ReferenceTag) : Reference.Tag.MatchesTag(ReferenceTag))
		{
			if (const UFaerieItemAsset* Asset = Reference.Reference.Get())
			{
				return Asset->GetAssetTemplateItem();
			}
		}
	}
	return nullptr;
}