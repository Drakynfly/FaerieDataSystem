// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Actions/FaerieStorageActions.h"
#include "FaerieItemStorage.h"
#include "ItemContainerEvent.h"
#include "Actions/FaerieInventoryClient.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieStorageActions)

bool FFaerieClientAction_MoveFromStorage::IsValid(const UFaerieInventoryClient* Client) const
{
	if (!::IsValid(Storage) ||
		!Client->CanAccessContainer(Storage, FFaerieClientAction_MoveHandlerBase::StaticStruct()) ||
		!Storage->Contains(Address) ||
		!Faerie::ItemData::IsValidStackAmount(Amount) ||
		!Storage->CanRemoveStack(Address, Faerie::Inventory::Tags::RemovalMoving))
	{
		return false;
	}
	return true;
}

bool FFaerieClientAction_MoveFromStorage::View(FFaerieItemStackView& View) const
{
	View = Storage->ViewStack(Address);
	if (Amount > 0)
	{
		View.Copies = FMath::Min(View.Copies, Amount);
	}
	return View.Item.IsValid();
}

bool FFaerieClientAction_MoveFromStorage::CanMove(const FFaerieItemStackView& View) const
{
	// @todo we might need to parameterize the StackBehavior
	return Storage->CanAddStack(View, EFaerieStorageAddStackBehavior::AddToAnyStack);
}

bool FFaerieClientAction_MoveFromStorage::Release(FFaerieItemStack& Stack) const
{
	return Storage->TakeStack(Address, Stack, Faerie::Inventory::Tags::RemovalMoving, Amount);
}

bool FFaerieClientAction_MoveFromStorage::Possess(const FFaerieItemStack& Stack) const
{
	// @todo we might need to parameterize the StackBehavior
	return Storage->AddItemStack(Stack, EFaerieStorageAddStackBehavior::AddToAnyStack);
}

bool FFaerieClientAction_MoveToStorage::IsValid(const UFaerieInventoryClient* Client) const
{
	return ::IsValid(Storage) &&
		Client->CanAccessContainer(Storage, FFaerieClientAction_MoveHandlerBase::StaticStruct());
}

bool FFaerieClientAction_MoveToStorage::CanMove(const FFaerieItemStackView& View) const
{
	return Storage->CanAddStack(View, AddStackBehavior);
}

bool FFaerieClientAction_MoveToStorage::Possess(const FFaerieItemStack& Stack) const
{
	return Storage->AddItemStack(Stack, AddStackBehavior);
}

bool FFaerieClientAction_DeleteEntry::Server_Execute(const UFaerieInventoryClient* Client) const
{
	if (!IsValid(Storage)) return false;
	if (!Client->CanAccessContainer(Storage, StaticStruct())) return false;

	return Storage->RemoveStack(Address, Faerie::Inventory::Tags::RemovalDeletion, Amount);
}

bool FFaerieClientAction_RequestMoveEntry::Server_Execute(const UFaerieInventoryClient* Client) const
{
	if (!IsValid(Storage)) return false;
	if (!IsValid(ToStorage)) return false;
	if (!Client->CanAccessContainer(Storage, StaticStruct())) return false;
	if (!Client->CanAccessContainer(ToStorage, StaticStruct())) return false;

	return Storage->MoveStack(ToStorage, Address, Amount).IsValid();
}

bool FFaerieClientAction_MergeStacks::Server_Execute(const UFaerieInventoryClient* Client) const
{
	if (!IsValid(Storage)) return false;
	if (!Client->CanAccessContainer(Storage, StaticStruct())) return false;
	return Storage->MergeStacks(Entry, FromStack, ToStack, Amount);
}

bool FFaerieClientAction_SplitStack::Server_Execute(const UFaerieInventoryClient* Client) const
{
	if (!IsValid(Storage)) return false;
	if (!Client->CanAccessContainer(Storage, StaticStruct())) return false;
	return Storage->SplitStack(Address, Amount);
}