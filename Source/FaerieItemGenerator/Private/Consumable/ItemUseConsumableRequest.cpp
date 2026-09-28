// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Consumable/ItemUseConsumableRequest.h"
#include "Consumable/FaerieConsumableFragment.h"

#include "Actions/FaerieInventoryClient.h"
#include "EntityManagerHelpers.h"
#include "FaerieItemContainerBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemUseConsumableRequest)

bool FFaerieClientAction_UseConsumable::Server_Execute(const TNotNull<const UFaerieInventoryClient*> Client) const
{
	if (!Client->CanAccessContainer(Handle.Container.Get(), StaticStruct())) return false;
	if (!IsValid(ConsumableType)) return false;

	const FFaerieItemProxy Proxy = Handle.Container->Proxy(Handle.Address);
	if (!Proxy.IsValid()) return false;

	FMassEntityManager& EntityManager = Faerie::ItemData::GetFaerieEntityManagerChecked(Client->GetWorld());
	return Faerie::Generation::TryConsume(EntityManager, Proxy, ConsumableType, Client->GetOwner(), 1);
}
