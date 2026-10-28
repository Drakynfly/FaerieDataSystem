// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Extensions/UserFavoriteFragment.h"
#include "FaerieItemContainerBase.h"
#include "MassEntityManager.h"

#include "Actions/FaerieInventoryClient.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UserFavoriteFragment)

using namespace Faerie;

bool FFaerieClientAction_FavoriteItem::Server_Execute(const TNotNull<const UFaerieInventoryClient*> Client, FMassEntityManager& EntityManager) const
{
	auto&& Container = Handle.Container.Get();
	if (!IsValid(Container)) return false;
	if (!Client->CanAccessContainer(EntityManager, Container, StaticStruct())) return false;

	const FMassEntityHandle Item = Container->GetItemHandle(Handle.Address);

	if (EntityManager.IsEntityValid(Item))
	{
		(void)EntityManager.AddSparseElementToEntity(Item, FFaerieUserFavoriteFragment::StaticStruct());
		// @Todo we need to broadcast this... UI needs to update on server and client when this runs
	}
	else
	{
		// @TODO do we lazy init entries that don't have entities, or will storage automatically make them all
	}

	return true;
}

bool FFaerieClientAction_UnfavoriteItem::Server_Execute(const TNotNull<const UFaerieInventoryClient*> Client, FMassEntityManager& EntityManager) const
{
	auto&& Container = Handle.Container.Get();
	if (!IsValid(Container)) return false;
	if (!Client->CanAccessContainer(EntityManager, Container, StaticStruct())) return false;

	const FMassEntityHandle Item = Container->GetItemHandle(Handle.Address);

	if (EntityManager.IsEntityValid(Item))
	{
		EntityManager.RemoveSparseElementFromEntity(Item, FFaerieUserFavoriteFragment::StaticStruct());
		// @Todo we need to broadcast this... UI needs to update on server and client when this runs
	}

	return true;
}