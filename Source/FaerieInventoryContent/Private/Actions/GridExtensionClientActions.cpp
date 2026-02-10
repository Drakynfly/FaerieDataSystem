// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Actions/GridExtensionClientActions.h"
#include "FaerieItemStorage.h"
#include "Actions/FaerieInventoryClient.h"
#include "Extensions/InventoryGridExtensionBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GridExtensionClientActions)

bool FFaerieClientAction_MoveToGrid::IsValid(const UFaerieInventoryClient* Client) const
{
	return Position != FIntPoint::NoneValue &&
		::IsValid(Storage) &&
		Client->CanAccessContainer(Storage, StaticStruct());
}

bool FFaerieClientAction_MoveToGrid::CanMove(const FFaerieItemStackView& View) const
{
	// Fetch the Grid Extension and ensure it exists
	auto&& GridExtension = GetExtension<UInventoryGridExtensionBase>(Storage, true);
	if (!::IsValid(GridExtension))
	{
		return false;
	}

	return GridExtension->CanAddAtLocation(View, Position);
}

bool FFaerieClientAction_MoveToGrid::Possess(const FFaerieItemStack& Stack) const
{
	auto&& GridExtension = GetExtension<UInventoryGridExtensionBase>(Storage, true);
	check(GridExtension);

	// Must be a new stack, since we intend to manually place it in the grid.
	Faerie::Inventory::FEventLog Log;
	Storage->AddItemStack(Stack, EFaerieStorageAddStackBehavior::OnlyNewStacks, Log);
	if (!Log.Success)
	{
		return false;
	}

	const FFaerieAddress TargetAddress = Log.AddressesTouched.Last();

	// Finally, move item to the cell client requested.
	return GridExtension->MoveItem(TargetAddress, Position);
}

bool FFaerieClientAction_MoveToGrid::View(FFaerieItemStackView& View) const
{
	if (auto&& GridExtension = GetExtension<UInventoryGridExtensionBase>(Storage, true))
	{
		if (GridExtension->IsCellOccupied(Position))
		{
			View = GridExtension->ViewAt(Position);
			return true;
		}
	}
	return false;
}

bool FFaerieClientAction_MoveToGrid::Release(FFaerieItemStack& Stack) const
{
	auto&& GridExtension = GetExtension<UInventoryGridExtensionBase>(Storage, true);
	check(GridExtension);

	const FFaerieAddress Address = GridExtension->GetKeyAt(Position);
	return Storage->TakeStack(Address, Stack, Faerie::Inventory::Tags::RemovalMoving, Faerie::ItemData::EntireStack);
}

bool FFaerieClientAction_MoveToGrid::IsSwap() const
{
	if (auto&& GridExtension = GetExtension<UInventoryGridExtensionBase>(Storage, true))
	{
		return CanSwapSlots && GridExtension->IsCellOccupied(Position);
	}
	return false;
}

bool FFaerieClientAction_MoveItemOnGrid::Server_Execute(const UFaerieInventoryClient* Client) const
{
	if (!IsValid(Storage)) return false;
	if (!Client->CanAccessContainer(Storage, StaticStruct())) return false;

	if (auto&& GridExtension = GetExtension<UInventoryGridExtensionBase>(Storage, true))
	{
		return GridExtension->MoveItem(Address, DragEnd);
	}

	return false;
}

bool FFaerieClientAction_RotateGridEntry::Server_Execute(const UFaerieInventoryClient* Client) const
{
	if (!IsValid(Storage)) return false;
	if (!Client->CanAccessContainer(Storage, StaticStruct())) return false;

	if (auto&& GridExtension = GetExtension<UInventoryGridExtensionBase>(Storage, true))
	{
		return GridExtension->RotateItem(Address);
	}

	return false;
}