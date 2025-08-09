// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "ItemSlotHandle.h"
#include "FaerieItemProxy.h"
#include "FaerieCraftingRequestSlot.generated.h"

// Maps crafting slots to the item that the client wants to use.
USTRUCT(BlueprintType)
struct FAERIEITEMGENERATOR_API FFaerieCraftingRequestSlot
{
	GENERATED_BODY()

	// The resource slot this request slot is filling.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Request Slot")
	FFaerieItemSlotHandle SlotID;

	// The item that is being used to fill the slot.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Request Slot")
	FFaerieItemProxy ItemProxy;
};