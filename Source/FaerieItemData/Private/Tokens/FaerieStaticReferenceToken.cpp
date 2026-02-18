// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Tokens/FaerieStaticReferenceToken.h"
#include "FaerieItem.h"
#include "FaerieItemAsset.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieStaticReferenceToken)

void UFaerieStaticReferenceToken::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ThisClass, References, COND_InitialOnly)
}

UFaerieStaticReferenceToken* UFaerieStaticReferenceToken::CreateInstance(const TConstArrayView<FFaerieTaggedStaticReference> InReferences)
{
	UFaerieStaticReferenceToken* NewToken = NewObject<UFaerieStaticReferenceToken>();
	NewToken->References = InReferences;
	return NewToken;
}

const UFaerieItem* UFaerieStaticReferenceToken::GetReferencedItem(const UFaerieItem& Item, const FGameplayTag ReferenceTag, const bool MatchExact)
{
	if (const UFaerieStaticReferenceToken* ReferenceToken = Item.GetOwnedToken<UFaerieStaticReferenceToken>())
	{
		return ReferenceToken->GetReferencedItem(ReferenceTag, MatchExact);
	}
	return nullptr;
}

const UFaerieItem* UFaerieStaticReferenceToken::GetReferencedItem(const FGameplayTag ReferenceTag, const bool MatchExact) const
{
	for (auto&& Reference : References)
	{
		if (MatchExact ? Reference.Tag.MatchesTagExact(ReferenceTag) : Reference.Tag.MatchesTag(ReferenceTag))
		{
			if (const UFaerieItemAsset* Asset = Reference.Reference.Get())
			{
				// Use Immutable to forcefully retrieve static instance.
				return Asset->GetItemInstance(EFaerieItemInstancingMutability::Immutable);
			}
		}
	}
	return nullptr;
}

namespace Faerie::Token
{
	TConstArrayView<TObjectPtr<UFaerieItemToken>> GetReferencedTokens(const UFaerieItem& Item, const FGameplayTag ReferenceTag, const bool MatchExact)
	{
		if (auto&& Reference = UFaerieStaticReferenceToken::GetReferencedItem(Item, ReferenceTag, MatchExact))
		{
			return Reference->GetOwnedTokens();
		}
		return {};
	}

	const UFaerieItemToken* GetReferencedTokenAtIndex(const UFaerieItem& Item, const int32 Index, const FGameplayTag ReferenceTag, const bool MatchExact)
	{
		if (auto&& Reference = UFaerieStaticReferenceToken::GetReferencedItem(Item, ReferenceTag, MatchExact))
		{
			return Reference->GetTokenAtIndex(Index);
		}
		return nullptr;
	}

	const UFaerieItemToken* GetReferencedToken(const UFaerieItem& Item, const TSubclassOf<UFaerieItemToken>& Class,
		const FGameplayTag ReferenceTag, const bool MatchExact)
	{
		if (auto&& Reference = UFaerieStaticReferenceToken::GetReferencedItem(Item, ReferenceTag, MatchExact))
		{
			return Reference->GetOwnedToken(Class);
		}
		return nullptr;
	}
}
