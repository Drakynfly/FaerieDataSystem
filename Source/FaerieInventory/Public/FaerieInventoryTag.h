// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "TTypedTagStaticImpl2.h"
#include "FaerieInventoryTag.generated.h"

/**
 * The key used to flag entries with custom data.
 */
USTRUCT(BlueprintType, meta = (Categories = "Fae.Inventory"))
struct FFaerieInventoryTag : public FGameplayTag
{
	GENERATED_BODY()
	END_TAG_DECL2(FFaerieInventoryTag, TEXT("Fae.Inventory"))
};