// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemToken.h"
#include "FaerieItemConsumableBase.generated.h"

// @todo bad forward declare! this is from a module that ItemGenerator doesn't include. restructure modules...
class UFaerieInventoryClient;

/**
 * Base class for tokens that represent a consumable e.g., contain logic for performing some action.
 * To achieve a "multi-use" consumable, pair with a UsesToken.
 * Children must implement either OnConsumed or OnConsumed_Mutable.
 * OnConsumed should be used if the item is not mutable, and thus, the token does not modify itself, or the owning item
 * when consumed. An item being destroyed by being consumed does not count.
 * OnConsumed_Mutable should be used if the item is mutable, and the act of consuming the item may change the item.
 * Make sure to override IsMutable if implementing OnConsumed_Mutable.
 */
UCLASS(Abstract)
class FAERIEITEMGENERATOR_API UFaerieItemConsumableBase : public UFaerieItemToken
{
	GENERATED_BODY()

public:
	// Consumable logic for tokens that have no effect on themselves.
	virtual void OnConsumed(const UFaerieInventoryClient* Client) const
		PURE_VIRTUAL(UFaerieItemConsumableBase::OnConsumed, );

	// Consumable logic for tokens that effect themselves.
	virtual void OnConsumed_Mutable(const UFaerieInventoryClient* Client)
	{
		OnConsumed(Client);
	}
};
