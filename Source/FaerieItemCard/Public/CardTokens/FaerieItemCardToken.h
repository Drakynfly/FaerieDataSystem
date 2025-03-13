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

	UE_DEPRECATED(5.5, "Use overload that takes an FGameplayTag")
PRAGMA_DISABLE_DEPRECATION_WARNINGS
	TSoftClassPtr<UFaerieCardBase> GetCardClass() const { return CardClass; }
PRAGMA_ENABLE_DEPRECATION_WARNINGS

protected:
	UPROPERTY(EditInstanceOnly, Category = "ItemCardToken")
	TMap<FFaerieItemCardType, TSoftClassPtr<UFaerieCardBase>> CardClasses;

	UE_DEPRECATED(5.5, "Replaced with Tag-based map, CardClasses")
	UPROPERTY(EditInstanceOnly)
	TSoftClassPtr<UFaerieCardBase> CardClass;
};

class UE_DEPRECATED(5.5, "This class is deprecated, use FaerieItemCardToken") UCustomInfoCard;
UCLASS(DisplayName = "Token - Card Class: Info (DEPRECATED)")
class FAERIEITEMCARD_API UCustomInfoCard : public UFaerieItemCardToken
{
	GENERATED_BODY()
};

class UE_DEPRECATED(5.5, "This class is deprecated, use FaerieItemCardToken") UCustomPaletteCard;
UCLASS(DisplayName = "Token - Card Class: Palette (DEPRECATED)")
class FAERIEITEMCARD_API UCustomPaletteCard : public UFaerieItemCardToken
{
	GENERATED_BODY()
};