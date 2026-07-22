// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Actions/ItemUseConsumableRequest.h"
#include "Actions/FaerieInventoryClient.h"
#include "FaerieItemContainerBase.h"

#include "Consumable/FaerieConsumableFragment.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemUseConsumableRequest)

bool FFaerieClientAction_UseConsumable::Server_Execute(const TNotNull<const UFaerieInventoryClient*> Client) const
{
	if (!Client->CanAccessContainer(Handle.Container.Get(), StaticStruct())) return false;
	if (!IsValid(ConsumableType)) return false;

	const FFaerieItemProxy Proxy = Handle.Container->Proxy(Handle.Address);
	if (!Proxy.IsValid()) return false;

	return Faerie::Generation::TryConsume(Proxy, ConsumableType, Client->GetOwner(), 1);
}
