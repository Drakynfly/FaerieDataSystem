// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "GridLayout/GridExtensionClientActions.h"
#include "GridLayout/InventoryGridExtensionBase.h"
#include "FaerieItemStorage.h"
#include "Actions/FaerieInventoryClient.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GridExtensionClientActions)

using namespace Faerie;

bool FFaerieClientAction_MoveToGrid::IsValid(const TNotNull<const UFaerieInventoryClient*> Client) const
{
	return Position != FIntPoint::NoneValue &&
		::IsValid(Storage) &&
		Client->CanAccessContainer(Storage, StaticStruct());
}

bool FFaerieClientAction_MoveToGrid::CanMove(const FFaerieItemDataView& View) const
{
	// Fetch the Grid Extension and ensure it exists
	auto&& GridExtension = Extensions::Get<UInventoryGridExtensionBase>(Storage->GetExtensions(), true);
	if (!::IsValid(GridExtension))
	{
		return false;
	}

	return GridExtension->CanAddAtLocation(View, Position);
}

bool FFaerieClientAction_MoveToGrid::Possess(const FFaerieUnownedItemStack& Stack) const
{
	auto&& GridExtension = Extensions::Get<UInventoryGridExtensionBase>(Storage->GetExtensions(), true);
	check(GridExtension);

	// Must be a new stack, since we intend to manually place it in the grid.
	TValueOrError<Inventory::FEventData, FText> Result{MakeError(FText::GetEmpty())};
	Storage->AddItemStack(Stack, EFaerieStorageAddStackBehavior::OnlyNewStacks, Result);
	if (Result.HasError())
	{
		return false;
	}

	const FFaerieAddress TargetAddress = Result.GetValue().AddressesTouched.Last();

	// Finally, move item to the cell client requested.
	return GridExtension->MoveItem(TargetAddress, Position);
}

bool FFaerieClientAction_MoveToGrid::View(FFaerieItemDataView& View) const
{
	if (auto&& GridExtension = Extensions::Get<UInventoryGridExtensionBase>(Storage->GetExtensions(), true))
	{
		if (GridExtension->IsCellOccupied(Position))
		{
			View = GridExtension->ViewAt(Position);
			return true;
		}
	}
	return false;
}

bool FFaerieClientAction_MoveToGrid::Release(FFaerieUnownedItemStack& Stack) const
{
	auto&& GridExtension = Extensions::Get<UInventoryGridExtensionBase>(Storage->GetExtensions(), true);
	check(GridExtension);

	const FFaerieAddress Address = GridExtension->GetKeyAt(Position);
	return Storage->TakeStack(Address, Stack, Inventory::Tags::RemovalMoving, ItemData::EntireStack);
}

bool FFaerieClientAction_MoveToGrid::IsSwap() const
{
	if (auto&& GridExtension = Extensions::Get<UInventoryGridExtensionBase>(Storage->GetExtensions(), true))
	{
		return CanSwapSlots && GridExtension->IsCellOccupied(Position);
	}
	return false;
}

bool FFaerieClientAction_MoveItemOnGrid::Server_Execute(const TNotNull<const UFaerieInventoryClient*> Client) const
{
	if (!IsValid(Storage)) return false;
	if (!Client->CanAccessContainer(Storage, StaticStruct())) return false;

	if (auto&& GridExtension = Extensions::Get<UInventoryGridExtensionBase>(Storage->GetExtensions(), true))
	{
		return GridExtension->MoveItem(Address, DragEnd);
	}

	return false;
}

bool FFaerieClientAction_RotateGridEntry::Server_Execute(const TNotNull<const UFaerieInventoryClient*> Client) const
{
	if (!IsValid(Storage)) return false;
	if (!Client->CanAccessContainer(Storage, StaticStruct())) return false;

	// Don't bother with this.
	if (RotateBy == EFaerieSpatialItemRotation::None) return true;

	if (auto&& GridExtension = Extensions::Get<UInventoryGridExtensionBase>(Storage->GetExtensions(), true))
	{
		return GridExtension->RotateItem(Address, RotateBy);
	}

	return false;
}