// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemTokenFilter.h"
#include "FaerieItem.h"
#include "FaerieItemToken.h"
#include "TypeCastingUtils.h"

namespace Faerie::Token
{
	namespace Private
	{
		void FIteratorAccess::AddWriteLock(const UFaerieItem* Item)
		{
			Item->WriteLock++;
		}

		void FIteratorAccess::RemoveWriteLock(const UFaerieItem* Item)
		{
			Item->WriteLock--;
		}

		UFaerieItemToken* FIteratorAccess::ResolveToken(const UFaerieItem* Item, const int32 Index)
		{
			return Item->GetTokenAtIndex(Index)->MutateCast();
		}

		const UFaerieItemToken* FIteratorAccess::ConstResolveToken(const UFaerieItem* Item, const int32 Index)
		{
			return Item->GetTokenAtIndex(Index);
		}
	}

	ITokenFilter::ITokenFilter(const UFaerieItem* Item)
	  : Item(Item)
	{
		check(IsValid(Item))

		// Initialize TokensBits with all tokens enabled.
		TokenBits.Init(true, Item->GetTokens().Num());
	}

	template <typename Pred>
	void FilterByPredicate(TBitArray<>& TokenBits, const UFaerieItem* Item, Pred&& Func)
	{
		for (TConstSetBitIterator<> It(TokenBits); It; ++It)
		{
			const UFaerieItemToken* Token = Item->GetTokenAtIndex(It.GetIndex());
			if (!ensureAlways(IsValid(Token))) continue;
			if (!Func(Token))
			{
				TokenBits.AccessCorrespondingBit(It) = false;
			}
		}
	}

	ITokenFilter& ITokenFilter::ByClass_Impl(const TSubclassOf<UFaerieItemToken>& Class)
	{
		if (!IsValid(Class) ||
			Class == UFaerieItemToken::StaticClass()) return *this;

		FilterByPredicate(TokenBits, Item, [Class](const UFaerieItemToken* Token)
			{
				return Token->IsA(Class);
			});

		return *this;
	}

	ITokenFilter& ITokenFilter::ByVirtual_Impl(ITokenFilterType& Type)
	{
		FilterByPredicate(TokenBits, Item, [&](const UFaerieItemToken* Token)
			{
				return Type.Passes(Token);
			});
		return *this;
	}

	bool ITokenFilter::CompareTokens(const ITokenFilter& OtherFilter) const
	{
		// This already indicates they are not equal.
		if (Num() != OtherFilter.Num())
		{
			return false;
		}

		// Copy bits so we can remove from them.
		TBitArray<> OtherBits = OtherFilter.TokenBits;

		for (TConstSetBitIterator<> ItA(TokenBits); ItA; ++ItA)
		{
			const UFaerieItemToken* TokenA = Item->GetTokenAtIndex(ItA.GetIndex());
			if (!ensureAlways(IsValid(TokenA))) continue;

			for (TConstSetBitIterator<> ItB(OtherBits); ItB; ++ItB)
			{
				const UFaerieItemToken* TokenB = Item->GetTokenAtIndex(ItB.GetIndex());
				if (!ensureAlways(IsValid(TokenB))) continue;

				if (TokenA->CompareWith(TokenB))
				{
					// The token is a match! Remove token from B to prevent re-match, and continue to the next token in ItA.
					OtherBits.AccessCorrespondingBit(ItB) = false;
					break;
				}
			}

			// No match was found for a token in ItA, exit as failure.
			return false;
		}

		// All tokens in Other should habe been matched. Check and exit as a success.
		check(OtherBits.IsEmpty())
		return true;
	}

	TArray<const UFaerieItemToken*> ITokenFilter::Emit() const
	{
		TArray<const UFaerieItemToken*> Tokens;
		Tokens.Reserve(TokenBits.CountSetBits());
		for (TConstSetBitIterator<> It(TokenBits); It; ++It)
		{
			const UFaerieItemToken* Token = Item->GetTokenAtIndex(It.GetIndex());
			if (!ensureAlways(IsValid(Token))) continue;
			Tokens.Add(Token);
		}
		return Tokens;
	}

	TArray<TObjectPtr<UFaerieItemToken>> ITokenFilter::BlueprintOnlyAccess() const
	{
		return Type::Cast<TArray<TObjectPtr<UFaerieItemToken>>>(Emit());
	}

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
