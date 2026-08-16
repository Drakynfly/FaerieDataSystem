// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Actions/FaerieStorageActions.h"
#include "FaerieItemStorage.h"
#include "ItemContainerEvent.h"
#include "Actions/FaerieInventoryClient.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieStorageActions)

bool FFaerieClientAction_MoveFromStorage::IsValid(const TNotNull<const UFaerieInventoryClient*> Client) const
{
	return ::IsValid(Storage) &&
		Client->CanAccessContainer(Storage, StaticStruct()) &&
		Faerie::ItemData::IsValidStackAmount(Amount) &&
		Storage->CanRemoveStack(Address, Faerie::Inventory::Tags::RemovalMoving);
}

bool FFaerieClientAction_MoveFromStorage::View(Faerie::ItemData::FScopeProxy& Proxy) const
{
	if (!Storage->ContainsAddress(Address))
	{
		return false;
	}

	Proxy = Storage->ViewAddress(Address);
	if (Amount > 0)
	{
		Proxy.SetCopies(FMath::Min(Proxy.Copies, Amount));
	}
	return true;
}

bool FFaerieClientAction_MoveFromStorage::CanMove(const Faerie::TValid<const FFaerieItemProxy&> Proxy) const
{
	// @todo we might need to parameterize the StackBehavior
	return Storage->CanAddStack(Proxy, EFaerieStorageAddStackBehavior::AddToAnyStack);
}

bool FFaerieClientAction_MoveFromStorage::Release(FFaerieUnownedItemStack& Stack) const
{
	return Storage->TakeStack(Address, Stack, Faerie::Inventory::Tags::RemovalMoving, Amount);
}

bool FFaerieClientAction_MoveFromStorage::Possess(const Faerie::TValid<const FFaerieUnownedItemStack&> Stack) const
{
	// @todo we might need to parameterize the StackBehavior
	return Storage->AddItemStack(Stack, EFaerieStorageAddStackBehavior::AddToAnyStack);
}

bool FFaerieClientAction_MoveToStorage::IsValid(const TNotNull<const UFaerieInventoryClient*> Client) const
{
	return ::IsValid(Storage) &&
		Client->CanAccessContainer(Storage, StaticStruct());
}

bool FFaerieClientAction_MoveToStorage::CanMove(const Faerie::TValid<const FFaerieItemProxy&> Proxy) const
{
	return Storage->CanAddStack(Proxy, AddStackBehavior);
}

bool FFaerieClientAction_MoveToStorage::Possess(const Faerie::TValid<const FFaerieUnownedItemStack&> Stack) const
{
	return Storage->AddItemStack(Stack, AddStackBehavior);
}

bool FFaerieClientAction_DeleteEntry::Server_Execute(const TNotNull<const UFaerieInventoryClient*> Client) const
{
	if (!IsValid(Storage)) return false;
	if (!Client->CanAccessContainer(Storage, StaticStruct())) return false;

	return Storage->RemoveStack(Address, Faerie::Inventory::Tags::RemovalDeletion, Amount);
}

bool FFaerieClientAction_RequestMoveEntry::Server_Execute(const TNotNull<const UFaerieInventoryClient*> Client) const
{
	if (!IsValid(Storage)) return false;
	if (!IsValid(ToStorage)) return false;
	if (!Client->CanAccessContainer(Storage, StaticStruct())) return false;
	if (!Client->CanAccessContainer(ToStorage, StaticStruct())) return false;

	return Storage->MoveStack(ToStorage, Address, Amount).IsValid();
}

bool FFaerieClientAction_MergeStacks::Server_Execute(const TNotNull<const UFaerieInventoryClient*> Client) const
{
	if (!IsValid(Storage)) return false;
	if (!Client->CanAccessContainer(Storage, StaticStruct())) return false;
	return Storage->MergeStacks(Entry, FromStack, ToStack, Amount);
}

bool FFaerieClientAction_SplitStack::Server_Execute(const TNotNull<const UFaerieInventoryClient*> Client) const
{
	if (!IsValid(Storage)) return false;
	if (!Client->CanAccessContainer(Storage, StaticStruct())) return false;
	return Storage->SplitStack(Address, Amount);
}