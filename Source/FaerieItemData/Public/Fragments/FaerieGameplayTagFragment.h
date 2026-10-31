// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieMassFragment.h"
#include "FaerieMassTypeTraits.h"
#include "GameplayTagContainer.h"
#include "FaerieGameplayTagFragment.generated.h"

USTRUCT(BlueprintType)
struct FFaerieGameplayTagFragment : public FFaerieMassFragment
{
	GENERATED_BODY()

	FFaerieGameplayTagFragment() = default;
	FFaerieGameplayTagFragment(const FGameplayTagContainer& Tags)
	  : Tags(Tags) {}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TagFragment")
	FGameplayTagContainer Tags;

	[[nodiscard]] UE_REWRITE friend uint32 GetTypeHash(const FFaerieGameplayTagFragment& Value)
	{
		return GetTypeHash(Value.Tags.GetGameplayTagArray());
	}

____FAERIE_FRAGMENT_DECL(FFaerieGameplayTagFragment)
};

template<>
struct TMassFragmentTraits<FFaerieGameplayTagFragment> final
{
	enum
	{
		AuthorAcceptsItsNotTriviallyCopyable = true
	};
};

template <>
struct Faerie::ItemData::TMassTypeTraits<FFaerieGameplayTagFragment> : TMassTypeTraitsBase<FFaerieGameplayTagFragment>
{
	enum
	{
		PrimaryIdentifier = true
	};
};