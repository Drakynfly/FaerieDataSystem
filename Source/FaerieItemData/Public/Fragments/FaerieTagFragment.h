// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieMassFragment.h"
#include "GameplayTagContainer.h"
#include "FaerieTagFragment.generated.h"

USTRUCT(BlueprintType)
struct FFaerieTagFragment : public FFaerieMassFragment
{
	GENERATED_BODY()

	FFaerieTagFragment() = default;
	FFaerieTagFragment(const FGameplayTagContainer& Tags)
	  : Tags(Tags) {}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TagFragment")
	FGameplayTagContainer Tags;

	friend [[nodiscard]] UE_REWRITE uint32 GetTypeHash(const FFaerieTagFragment& Value)
	{
		return GetTypeHash(Value.Tags.GetGameplayTagArray());
	}
};

template <>
struct Faerie::ItemData::TMassFragmentTypeTraits<FFaerieTagFragment> : TMassFragmentTypeTraitsBase<FFaerieTagFragment>
{
	enum
	{
		PrimaryIdentifier = true
	};
};