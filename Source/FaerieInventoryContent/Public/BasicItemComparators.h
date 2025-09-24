// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataComparator.h"
#include "BasicItemComparators.generated.h"

/**
 * Compares two items by their name token.
 */
UCLASS(meta = (DisplayName = "Alphabetical"))
class FAERIEINVENTORYCONTENT_API UFaerieLexicographicNameComparator : public UFaerieItemDataComparator
{
	GENERATED_BODY()

public:
	virtual bool Exec(const FFaerieItemSnapshot& A, const FFaerieItemSnapshot& B) const override;
};

/**
 * Compares two items by their last modified date.
 */
UCLASS(meta = (DisplayName = "Timestamp"))
class FAERIEINVENTORYCONTENT_API UFaerieDateModifiedComparator : public UFaerieItemDataComparator
{
	GENERATED_BODY()

public:
	virtual bool Exec(const FFaerieItemSnapshot& A, const FFaerieItemSnapshot& B) const override;
};