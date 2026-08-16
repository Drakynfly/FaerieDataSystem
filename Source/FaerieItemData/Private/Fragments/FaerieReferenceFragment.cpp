// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Fragments/FaerieReferenceFragment.h"
#include "FaerieItem.h"
#include "FaerieItemAsset.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieReferenceFragment)

const UFaerieItemAsset* FFaerieReferenceFragment::GetReferencedAsset(const FMassEntityManager* EntityManager, const Faerie::TValid<const FFaerieItemInstance&> Item, const FGameplayTag ReferenceTag, const bool MatchExact)
{
	auto ReferenceView = Faerie::ItemData::GetEntityFragmentOrDefault<FFaerieReferenceFragment>(EntityManager, Item);
	if (ReferenceView.IsValid())
	{
		return ReferenceView->GetReferencedAsset(ReferenceTag, MatchExact);
	}
	return nullptr;
}

const UFaerieItem* FFaerieReferenceFragment::GetReferencedItem(const FMassEntityManager* EntityManager, const Faerie::TValid<const FFaerieItemInstance&> Item, const FGameplayTag ReferenceTag, const bool MatchExact)
{
	auto ReferenceView = Faerie::ItemData::GetEntityFragmentOrDefault<FFaerieReferenceFragment>(EntityManager, Item);
	if (ReferenceView.IsValid())
	{
		return ReferenceView->GetReferencedItem(ReferenceTag, MatchExact);
	}
	return nullptr;
}

const UFaerieItemAsset* FFaerieReferenceFragment::GetReferencedAsset(const FGameplayTag ReferenceTag, const bool MatchExact) const
{
	for (auto&& Reference : References)
	{
		if (MatchExact ? Reference.Tag.MatchesTagExact(ReferenceTag) : Reference.Tag.MatchesTag(ReferenceTag))
		{
			if (const UFaerieItemAsset* Asset = Reference.Reference.Get())
			{
				return Asset;
			}
		}
	}
	return nullptr;
}

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