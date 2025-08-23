// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Tokens/FaerieItemConsumableBase.h"
#include "FaerieItem.h"
#include "FaerieItemOwnerInterface.h"
#include "Tokens/FaerieItemUsesToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemConsumableBase)

bool UFaerieItemConsumableBase::CanConsume(const UFaerieItem* Item, const AActor* Consumer) const
{
	const UFaerieItemUsesToken* Uses = Item->GetToken<UFaerieItemUsesToken>();
	if (IsValid(Uses))
	{
		return Uses->HasUses(1);
	}

	return true;
}

bool UFaerieItemConsumableBase::TryConsume(const UFaerieItem* Item, AActor* Consumer) const
{
	// Try getting the Consumable logic token, first by Mutable access.
	if (UFaerieItem* Mutable = Item->MutateCast();
		IsValid(Mutable))
	{
		// While we have confirmed the Item is mutable, check for Uses, and try to take one.
		UFaerieItemUsesToken* Uses = Mutable->GetEditableToken<UFaerieItemUsesToken>();
		if (IsValid(Uses))
		{
			// If we have a usage left, remove 1.
			if (Uses->HasUses(1))
			{
				Uses->RemoveUses(1);
			}
			// If we don't, we cannot run the consume logic
			else
			{
				return false;
			}
		}

		if (ThisClass* MutableThis = MutateCast<ThisClass>())
		{
			MutableThis->OnConsumed_Mutable(Mutable, Consumer);
		}
		else
		{
			OnConsumed(Mutable, Consumer);
		}

		if (IsValid(Uses))
		{
			// Destroy the item if we ran out of uses.
			if (Uses->GetUsesRemaining() == 0 && Uses->GetDestroyItemOnLastUse())
			{
				IFaerieItemOwnerInterface* Container = GetImplementingOuter<IFaerieItemOwnerInterface>();
				if (Container)
				{
					(void)Container->Release({Item, 1});
				}
			}
		}

		return true;
	}

	// If that fails, attempt by const access.
	OnConsumed(Item, Consumer);
	return true;
}
