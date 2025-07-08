// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Actions/ItemUseConsumableRequest.h"
#include "FaerieItemContainerBase.h"
#include "Tokens/FaerieItemConsumableBase.h"
#include "Tokens/FaerieItemUsesToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemUseConsumableRequest)

bool FFaerieClientAction_UseConsumable::Server_Execute(const UFaerieInventoryClient* Client) const
{
	if (!Client->CanAccessContainer(Handle.Container.Get(), StaticStruct())) return false;
	if (!Handle.Container->Contains(Handle.Address)) return false;

	const FFaerieItemStackView View = Handle.Container->ViewStack(Handle.Address);
	if (!View.IsValid()) return false;

	// Try getting the Consumable logic token, first by Mutable access.
	if (UFaerieItem* Mutable = View.Item->MutateCast();
		IsValid(Mutable))
	{
		UFaerieItemConsumableBase* MutableConsumable = Mutable->GetEditableToken<UFaerieItemConsumableBase>();

		// If that failed, the item is not a Consumable.
		if (!IsValid(MutableConsumable)) return false;

		MutableConsumable->OnConsumed_Mutable(Client);

		// While we have confirmed the Item is mutable, check for Uses, and try to take one.
		auto Uses = Mutable->GetEditableToken<UFaerieItemUsesToken>();

		if (IsValid(Uses) &&
			Uses->HasUses(1))
		{
			Uses->RemoveUses(1);
		}

		if (Uses->GetDestroyItemOnLastUse() && Uses->GetUsesRemaining() == 0)
		{
			(void)Handle.Container->Release(Handle.Address, 1);
		}

		return true;
	}

	// If that fails, attempt by const access.
	if (const UFaerieItemConsumableBase* Consumable = View.Item->GetToken<UFaerieItemConsumableBase>();
		IsValid(Consumable))
	{
		Consumable->OnConsumed(Client);
		return true;
	}

	return false;
}
