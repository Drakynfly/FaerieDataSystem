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
			// If we don't have any uses, we cannot run the consume logic
			if (!Uses->HasUses(1))
			{
				return false;
			}
		}

		// Run consumable logic
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
			// Remove a usage.
			Uses->RemoveUses(1);
		}

		return true;
	}

	// If that fails, attempt by const access.
	OnConsumed(Item, Consumer);
	return true;
}
