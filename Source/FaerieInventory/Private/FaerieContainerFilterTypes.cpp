// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieContainerFilterTypes.h"
#include "FaerieContainerIterator.h"
#include "FaerieItem.h"
#include "Tokens/FaerieInfoToken.h"
#include "Tokens/FaerieTagToken.h"

namespace Faerie::Container
{
	bool FMatchItemMutable::Exec(FIteratorPtr Iterator) const
	{
		return Iterator->ResolveItem()->CanMutate() == MutabilityToMatch;
	}

	bool FCompareName::Exec(FIteratorPtr Iterator) const
	{
		if (const UFaerieInfoToken* Info = Iterator->ResolveItem()->GetToken<UFaerieInfoToken>())
		{
			return Info->GetItemName().CompareTo(CompareText, ComparisonType) == 0;
		}
		return false;
	}

	bool FHasTag::Exec(FIteratorPtr Iterator) const
	{
		if (const UFaerieTagToken* Tags = Iterator->ResolveItem()->GetToken<UFaerieTagToken>())
		{
			if (HasTagExact)
			{
				return Tags->GetTags().HasTagExact(Tag);
			}
			return Tags->GetTags().HasTag(Tag);
		}
		return false;
	}

	bool FCallbackFilter::Exec(FIteratorPtr Iterator) const
	{
		return Callback.Execute(Iterator);
	}
}
