// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemInstance.h"
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
	// Note: this should be protected, not public, but I don't have a workaround for this yet.
	// Override to add logic when an item mutates while owned by the implementing object.
	virtual void OnItemDataChanged(const FFaerieItemInstance& Instance, TNotNull<const UScriptStruct*> FragmentType, FGameplayTag EditTag) = 0;
};