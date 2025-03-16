// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "BasicItemHashInstructions.h"
#include "FaerieHashStatics.h"
#include "FaerieItem.h"
#include "FaerieItemDataFilter.h"
#include "FaerieItemToken.h"
#include "Squirrel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BasicItemHashInstructions)

// WARNING: Changing any of these will invalidate all existing hashes generated with them.
// These are all random large primes, that will *hopefully* generate decent hashes with Squirrel

#define HASH_FAILURE 0

#define VALIDATED_TRUE 315883619
#define VALIDATED_FALSE 262158943

#define BOOLEAN_FILTER_TRUE 279557143
#define BOOLEAN_FILTER_FALSE 582595723
#define TOKEN_HASH_EMPTY 693300541

uint32 UFISHI_Literial::Hash(const FFaerieItemStackView StackView) const
{
	return Value.Hash;
}

uint32 UFISHI_IsValid::Hash(const FFaerieItemStackView StackView) const
{
	if (StackView.Item.IsValid() &&
		Faerie::ItemData::IsValidStack(StackView.Copies))
	{
		return VALIDATED_TRUE;
	}
	return VALIDATED_FALSE;
}

uint32 UFISHI_And::Hash(const FFaerieItemStackView StackView) const
{
	int32 Hash = 0;

	for (auto Instruction : Instructions)
	{
		Hash = Squirrel::HashCombine(Hash, ChildHash(Instruction, StackView));
	}

	return Hash;
}

uint32 UFISHI_Or::Hash(const FFaerieItemStackView StackView) const
{
	for (auto Instruction : Instructions)
	{
		if (const int32 Hash = ChildHash(Instruction, StackView);
			Hash != HASH_FAILURE)
		{
			return Hash;
		}
	}

	return HASH_FAILURE;
}

uint32 UFISHI_BooleanFilter::Hash(const FFaerieItemStackView StackView) const
{
	if (!ensure(IsValid(Pattern)))
	{
		return HASH_FAILURE;
	}

	if (Pattern->Exec(StackView))
	{
		return BOOLEAN_FILTER_TRUE;
	}

	return BOOLEAN_FILTER_FALSE;
}

uint32 UFISHI_BooleanSelect::Hash(const FFaerieItemStackView StackView) const
{
	if (!ensure(IsValid(Pattern)))
	{
		return HASH_FAILURE;
	}

	if (Pattern->Exec(StackView))
	{
		return ChildHash(True, StackView);
	}

	return ChildHash(False, StackView);
}

uint32 UFISHI_Tokens::Hash(const FFaerieItemStackView StackView) const
{
	uint32 Hash = 0;

	constexpr bool IncludeSuper = false;

	for (auto&& TokenClass : TokenClasses)
	{
		const TArray<const UFaerieItemToken*> Tokens = StackView.Item->GetTokens(TokenClass);

		if (Tokens.IsEmpty())
		{
			Hash = Squirrel::HashCombine(Hash, TOKEN_HASH_EMPTY);
		}
		else
		{
			for (auto&& Token : Tokens)
			{
				Hash = Squirrel::HashCombine(Hash, Faerie::Hash::HashObjectByProps(Token, IncludeSuper));
			}
		}
	}

	return Hash;
}