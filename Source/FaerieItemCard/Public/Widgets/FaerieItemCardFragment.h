// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemCardTags.h"
#include "FaerieMassFragment.h"
#include "FaerieItemCardFragment.generated.h"

class UFaerieCardBase;

USTRUCT(BlueprintType)
struct FFaerieItemCardElement
{
	GENERATED_BODY()

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "ItemCardElement")
	FFaerieItemCardType CardType;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "ItemCardElement")
	TSoftClassPtr<UFaerieCardBase> CardClass;
};

USTRUCT()
struct FAERIEITEMCARD_API FFaerieItemCardClassFragment : public FFaerieMassFragment
{
	GENERATED_BODY()

	/*
	 * Children slots of an item. There can currently be up to five of these.
	 */
	UPROPERTY(EditInstanceOnly, Category = "ItemCardClassFragment")
	FFaerieItemCardElement Classes[5];

	TSoftClassPtr<UFaerieCardBase> GetCardClass(FFaerieItemCardType Tag) const;
};