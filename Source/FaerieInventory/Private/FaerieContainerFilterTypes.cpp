// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieContainerFilterTypes.h"
#include "EntityManagerHelpers.h"
#include "FaerieContainerIterator.h"
#include "FaerieItem.h"

#include "Fragments/FaerieAssetInfo.h"
#include "Fragments/FaerieTagFragment.h"

namespace Faerie::Container
{
	bool FMatchItemMutable::Exec(const TNotNull<const UObject*> WorldContextObj, const ItemData::FValidatedDataView View) const
	{
		return View->GetInstance().IsMutable() == MutabilityToMatch;
	}

	bool FCompareName::Exec(const TNotNull<const UObject*> WorldContextObj, const ItemData::FValidatedDataView View) const
	{
		const ItemData::FOptionalEntityManager EntityManager(WorldContextObj);
		auto AssetInfo = Faerie::ItemData::GetEntityFragmentOrDefault<FFaerieAssetInfo>(EntityManager, View->GetInstance());
		if (AssetInfo.IsValid())
		{
			return AssetInfo->ObjectName.CompareTo(CompareText, ComparisonType) == 0;
		}
		return false;
	}

	bool FHasTag::Exec(const TNotNull<const UObject*> WorldContextObj, const ItemData::FValidatedDataView View) const
	{
		const ItemData::FOptionalEntityManager EntityManager(WorldContextObj);
		auto TagFragment = Faerie::ItemData::GetEntityFragmentOrDefault<FFaerieTagFragment>(EntityManager, View->GetInstance());
		if (TagFragment.IsValid())
		{
			if (HasTagExact)
			{
				return TagFragment->Tags.HasTagExact(Tag);
			}
			return TagFragment->Tags.HasTag(Tag);
		}
		return false;
	}

	bool FHasAnyTags::Exec(const TNotNull<const UObject*> WorldContextObj, const ItemData::FValidatedDataView View) const
	{
		const ItemData::FOptionalEntityManager EntityManager(WorldContextObj);
		auto TagFragment = Faerie::ItemData::GetEntityFragmentOrDefault<FFaerieTagFragment>(EntityManager, View->GetInstance());
		if (TagFragment.IsValid())
		{
			if (Exact)
			{
				return TagFragment->Tags.HasAnyExact(Tags);
			}
			return TagFragment->Tags.HasAny(Tags);
		}
		return false;
	}

	bool FHasAllTags::Exec(const TNotNull<const UObject*> WorldContextObj, const ItemData::FValidatedDataView View) const
	{
		const ItemData::FOptionalEntityManager EntityManager(WorldContextObj);
		auto TagFragment = Faerie::ItemData::GetEntityFragmentOrDefault<FFaerieTagFragment>(EntityManager, View->GetInstance());
		if (TagFragment.IsValid())
		{
			if (Exact)
			{
				return TagFragment->Tags.HasAllExact(Tags);
			}
			return TagFragment->Tags.HasAll(Tags);
		}
		return false;
	}

	bool FCallbackFilter::Exec(const TNotNull<const UObject*> WorldContextObj, const ItemData::FValidatedDataView View) const
	{
		return Callback.Execute(WorldContextObj, View);
	}
}
