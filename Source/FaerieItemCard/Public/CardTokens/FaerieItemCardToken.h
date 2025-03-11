// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

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
	TSoftClassPtr<UFaerieCardBase> GetCardClass(FGameplayTag Tag) const;

	UE_DEPRECATED(5.5, "Use overload that takes an FGameplayTag")
	TSoftClassPtr<UFaerieCardBase> GetCardClass() const { return CardClass; }

protected:
	UPROPERTY(EditInstanceOnly, Category = "ItemCardToken")
	TMap<FGameplayTag, TSoftClassPtr<UFaerieCardBase>> CardClasses;

	UE_DEPRECATED(5.5, "Replaced with Tag-based map, CardClasses")
	UPROPERTY(EditInstanceOnly)
	TSoftClassPtr<UFaerieCardBase> CardClass;
};

UCLASS(DisplayName = "Token - Card Class: Info (DEPRECATED)")
class UE_DEPRECATED(5.5, "This class is deprecated, use FaerieItemCardToken")
FAERIEITEMCARD_API UCustomInfoCard : public UFaerieItemCardToken
{
	GENERATED_BODY()
};

UCLASS(DisplayName = "Token - Card Class: Palette (DEPRECATED)")
class UE_DEPRECATED(5.5, "This class is deprecated, use FaerieItemCardToken")
FAERIEITEMCARD_API UCustomPaletteCard : public UFaerieItemCardToken
{
	GENERATED_BODY()
};