// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "GridLayout/GridExtensionClientActions.h"
#include "GridLayout/InventoryGridExtensionBase.h"
#include "FaerieItemStorage.h"
#include "Actions/FaerieInventoryClient.h"
#include "FaerieContainerEvent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GridExtensionClientActions)

using namespace Faerie;

bool FFaerieClientAction_MoveToGrid::IsValid(const TNotNull<const UFaerieInventoryClient*> Client) const
{
	return Position != FIntPoint::NoneValue &&
		::IsValid(Storage) &&
		Client->CanAccessContainer(Storage, StaticStruct());
}

bool FFaerieClientAction_MoveToGrid::View(ItemData::FScopeProxy& Proxy) const
{
	if (const FFaerieContainerGridData* GridData = Storage->ReadContainerData<FFaerieContainerGridData>(false))
	{
		const FFaerieContainerGridReadContext Context = GridData->GetReadContext();

		if (Context.IsCellOccupied(Position))
		{
			Proxy = Context.ViewAt_Native(Position);
			return true;
		}
	}
	return false;
}

bool FFaerieClientAction_MoveToGrid::CanMove(const TValid<const FFaerieItemProxy&> Proxy) const
{
	if (const FFaerieContainerGridData* GridData = Storage->ReadContainerData<FFaerieContainerGridData>(false))
	{
		const FFaerieContainerGridReadContext Context = GridData->GetReadContext();
		return Context.CanAddAtLocation(Proxy, Position);
	}
	return false;
}

bool FFaerieClientAction_MoveToGrid::Possess(const TValid<const FFaerieUnownedItemStack&> Stack) const
{
	// Must be a new stack, since we intend to manually place it in the grid.
	TValueOrError<Container::FEvent, FText> Result{MakeError(FText::GetEmpty())};
	Storage->AddItemStack(Stack, EFaerieStorageAddStackBehavior::OnlyNewStacks, Result);
	if (!Result.HasValue())
	{
		return false;
	}

	const FFaerieAddress TargetAddress = Result.GetValue().AddressesTouched.Last();

	bool Out = false;
	Storage->WriteContainerData(FFaerieContainerGridData::StaticStruct(),
		[this, &Out, TargetAddress](const FStructView Element)
		{
			Out = Element.Get<FFaerieContainerGridData>().GetWriteContext().MoveItem(TargetAddress, Position);
		}, false);

	return Out;
}

bool FFaerieClientAction_MoveToGrid::Release(FFaerieUnownedItemStack& Stack) const
{
	bool Out = false;
	Storage->WriteContainerData(FFaerieContainerGridData::StaticStruct(),
		[this, &Out, &Stack](const FStructView Element)
		{
			auto Context = Element.Get<FFaerieContainerGridData>().GetWriteContext();
			const TOptional<FFaerieAddress> Address = Context.FindAddress(Position);
			if (Address.IsSet())
			{
				Out = Context.GetStorage()->TakeStack(Address.GetValue(), Stack, Inventory::Tags::RemovalMoving, ItemData::EntireStack);
			}
		}, false);
	return Out;
}

bool FFaerieClientAction_MoveToGrid::IsSwap() const
{
	if (const FFaerieContainerGridData* GridData = Storage->ReadContainerData<FFaerieContainerGridData>(false))
	{
		const FFaerieContainerGridReadContext Context = GridData->GetReadContext();
		return Context.IsCellOccupied(Position);
	}
	return false;
}

bool FFaerieClientAction_MoveItemOnGrid::Server_Execute(const TNotNull<const UFaerieInventoryClient*> Client) const
{
	if (!IsValid(Storage)) return false;
	if (!Client->CanAccessContainer(Storage, StaticStruct())) return false;

	Storage->WriteContainerData(FFaerieContainerGridData::StaticStruct(),
		[this](const FStructView Element)
		{
			const FFaerieContainerGridWriteContext Context = Element.Get<FFaerieContainerGridData>().GetWriteContext();
			(void)Context.MoveItem(Address, Position);
		}, false);

	return false;
}

bool FFaerieClientAction_RotateGridEntry::Server_Execute(const TNotNull<const UFaerieInventoryClient*> Client) const
{
	if (!IsValid(Storage)) return false;
	if (!Client->CanAccessContainer(Storage, StaticStruct())) return false;

	// Don't bother with this.
	if (RotateBy == EFaerieSpatialItemRotation::None) return true;

	Storage->WriteContainerData(FFaerieContainerGridData::StaticStruct(),
		[this](const FStructView Element)
		{
			const FFaerieContainerGridWriteContext Context = Element.Get<FFaerieContainerGridData>().GetWriteContext();
			(void)Context.RotateItem(Address, RotateBy);
		}, false);

	return false;
}