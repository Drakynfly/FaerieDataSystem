// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemToken.h"
#include "FaerieItemConsumableBase.generated.h"

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
	// This function is const so that it can be called on const pointers. Mutation-safety is implemented by the function.
	// The owning item must be passed in, in case this token is immutable (and therefore cannot determine its own item)
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Consumable")
	bool TryConsume(const UFaerieItem* Item, AActor* Consumer) const;

	// Consumable logic for tokens that have no effect on themselves.
	virtual void OnConsumed(AActor* Consumer) const
		PURE_VIRTUAL(UFaerieItemConsumableBase::OnConsumed, );

	// Consumable logic for tokens that effect themselves.
	virtual void OnConsumed_Mutable(AActor* Consumer)
	{
		OnConsumed(Consumer);
	}
};
