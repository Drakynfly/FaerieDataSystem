// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Actions/ItemUseConsumableRequest.h"
#include "FaerieItem.h"
#include "FaerieItemContainerBase.h"
#include "Actions/FaerieInventoryClient.h"
#include "Tokens/FaerieItemConsumableBase.h"
#include "Tokens/FaerieItemUsesToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemUseConsumableRequest)

bool FFaerieClientAction_UseConsumable::Server_Execute(const UFaerieInventoryClient* Client) const
{
	if (!Client->CanAccessContainer(Handle.Container.Get(), StaticStruct())) return false;
	if (!Handle.Container->Contains(Handle.Address)) return false;

	const UFaerieItem* Item = Handle.Container->ViewItem(Handle.Address);
	if (!IsValid(Item)) return false;

	if (const UFaerieItemConsumableBase* Consumable = Item->GetToken<UFaerieItemConsumableBase>();
		IsValid(Consumable))
	{
		Consumable->TryConsume(Item, Client->GetOwner());
		return true;
	}

	return false;
}
