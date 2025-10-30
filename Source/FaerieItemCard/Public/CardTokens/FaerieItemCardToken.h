// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemCardTags.h"
#include "FaerieItemToken.h"
#include "FaerieItemCardToken.generated.h"

class UFaerieCardBase;

/**
 *
 */
UCLASS(DisplayName = "Token - Card Classes")
class FAERIEITEMCARD_API UFaerieItemCardToken : public UFaerieItemToken
{
	GENERATED_BODY()

public:
	TSoftClassPtr<UFaerieCardBase> GetCardClass(FFaerieItemCardType Tag) const;

protected:
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "ItemCardToken", meta = (ExposeOnSpawn, ForceInlineRow))
	TMap<FFaerieItemCardType, TSoftClassPtr<UFaerieCardBase>> CardClasses;
};