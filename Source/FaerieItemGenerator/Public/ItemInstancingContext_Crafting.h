// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemSlotInterface.h"
#include "FaerieItemSource.h"
#include "ItemInstancingContext_Crafting.generated.h"

class USquirrel;

USTRUCT()
struct FAERIEITEMGENERATOR_API FFaerieItemInstancingContext_Crafting : public FFaerieItemInstancingContext
{
	GENERATED_BODY()

public:
	// Used to fill Required & Optional Slots (via inputs)
	UPROPERTY()
	FFaerieCraftingFilledSlots InputEntryData;

	// Used to fill Required & Optional Slots (generated instances)
	UPROPERTY()
	TMap<FFaerieItemSlotHandle, FFaerieItemStack> GeneratedChildren;

	// When valid, this context can generate seeded output, otherwise implementations may choose to either fail or
	// resolve to non-seeded output.
	UPROPERTY()
	TObjectPtr<USquirrel> Squirrel = nullptr;

	virtual const UScriptStruct* GetScriptStruct() const override
	{
		return FFaerieItemInstancingContext_Crafting::StaticStruct();
	}
};