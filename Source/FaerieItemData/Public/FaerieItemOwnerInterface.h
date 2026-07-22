// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataFwd.h"
#include "FaerieItemDataDefines.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "FaerieItemOwnerInterface.generated.h"

UINTERFACE(NotBlueprintable, MinimalAPI)
class UFaerieItemOwnerInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * This interface exists to connect the FaerieItemData module and FaerieInventory.
 * The only implementer of this interface should be UFaerieItemContainerBase.
 */
class FAERIEITEMDATA_API IFaerieItemOwnerInterface
{
	GENERATED_BODY()

public:
	// Call this function to destroy a item instance via a proxy.
	virtual void DestroyStack(const FFaerieItemProxy& Proxy, int32 Copies = Faerie::ItemData::EntireStack) = 0;

	// Call this function to grant ownership of a UFaerieItem stack. Returns true if ownership was accepted.
	// It is implied, and is the responsibility of the implementing class, to either accept ownership of the whole stack,
	// or none. Partial possession is not allowed.
	[[nodiscard]] virtual bool Possess(const FFaerieUnownedItemStack& DataView) = 0;

	virtual void OnItemDataChanged(const Faerie::ItemData::FMutableReference& Instance, TNotNull<const UScriptStruct*> Struct, FGameplayTag EditTag) = 0;
};