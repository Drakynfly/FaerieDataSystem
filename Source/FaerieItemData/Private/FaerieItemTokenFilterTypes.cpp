// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemTokenFilterTypes.h"
#include "FaerieItemToken.h"

namespace Faerie::Token
{
	bool FMutableFilter::StaticPasses(const UFaerieItemToken* Token)
	{
		return Token->IsMutable();
	}

	bool FImmutableFilter::StaticPasses(const UFaerieItemToken* Token)
	{
		return !Token->IsMutable();
	}

	bool FTagFilter::Passes(const UFaerieItemToken* Token)
	{
		if (Exact)
		{
			return Token->GetClassTags().HasTagExact(Tag);
		}
		return Token->GetClassTags().HasTag(Tag);
	}

	bool FTagsFilter::Passes(const UFaerieItemToken* Token)
	{
		if (Exact)
		{
			if (All)
			{
				return Token->GetClassTags().HasAllExact(Tags);
			}
			return Token->GetClassTags().HasAnyExact(Tags);
		}

		if (All)
		{
			return Token->GetClassTags().HasAll(Tags);
		}
		return Token->GetClassTags().HasAny(Tags);
	}

	bool FTagQueryFilter::Passes(const UFaerieItemToken* Token)
	{
		return Token->GetClassTags().MatchesQuery(Query);
	}
}
