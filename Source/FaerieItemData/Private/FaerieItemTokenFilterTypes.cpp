// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemTokenFilterTypes.h"
#include "FaerieItemToken.h"

namespace Faerie::Token
{
	bool FMatchMutability::Exec(const TNotNull<const UFaerieItemToken*> Token) const
	{
		return Token->IsMutable() == MutabilityToMatch;
	}

	bool FIsOwned::Exec(const TNotNull<const UFaerieItemToken*> Token) const
	{
		return Token->GetOuterItem() == Item;
	}

	bool FIsClass::Exec(const TNotNull<const UFaerieItemToken*> Token) const
	{
		return Token->IsA(Class);
	}

	bool FHasInterface::Exec(const TNotNull<const UFaerieItemToken*> Token) const
	{
		return Token->IsA(Class);
	}

	bool FIsClassExact::Exec(const TNotNull<const UFaerieItemToken*> Token) const
	{
		return Token->GetClass() == Class;
	}

	bool FIsAnyClass::Exec(const TNotNull<const UFaerieItemToken*> Token) const
	{
		for (auto&& Element : Classes)
		{
			if (Token->IsA(Element))
			{
				return true;
			}
		}
		return false;
	}

	bool FIsAnyClassExact::Exec(const TNotNull<const UFaerieItemToken*> Token) const
	{
		return Classes.Contains(Token->GetClass());
	}

	bool FTagFilter::Exec(const TNotNull<const UFaerieItemToken*> Token) const
	{
		if (Exact)
		{
			return Token->GetClassTags().HasTagExact(Tag);
		}
		return Token->GetClassTags().HasTag(Tag);
	}

	bool FTagsFilter::Exec(const TNotNull<const UFaerieItemToken*> Token) const
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

	bool FTagQueryFilter::Exec(const TNotNull<const UFaerieItemToken*> Token) const
	{
		return Token->GetClassTags().MatchesQuery(Query);
	}
}
