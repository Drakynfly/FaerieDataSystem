// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "TTypedTagStaticImpl2.h"
#include "TypedGameplayTags.h"

#include "FaerieItemCardTags.generated.h"

/**
 * Tag type to identify types of Item Card Widgets
 */
USTRUCT(BlueprintType, meta = (Categories = "Fae.CardType"))
struct FFaerieItemCardType : public FGameplayTag
{
	GENERATED_BODY()
	END_TAG_DECL2(FFaerieItemCardType, TEXT("Fae.CardType"))
};

namespace Faerie
{
	FAERIEITEMCARD_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieItemCardType, CardTypeBase)
	FAERIEITEMCARD_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieItemCardType, CardType_Full)
	FAERIEITEMCARD_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieItemCardType, CardType_Nameplate)
}

