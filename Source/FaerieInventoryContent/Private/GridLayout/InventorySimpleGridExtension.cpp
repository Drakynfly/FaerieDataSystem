// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieContainerEvent.h"

#include "GridLayout/InventorySimpleGridExtension.h"

#include "FaerieItemContainerBase.h"
#include "FaerieItemStorage.h"
#include "FaerieItemStorageIterators.h"
#include "ItemContainerEvent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventorySimpleGridExtension)

using namespace Faerie;

void UInventorySimpleGridExtension::InitializeGrid(const FFaerieContainerGridWriteContext& Context) const
{
	// Add all existing items to the grid on startup.
	// This is dumb, and just adds them in order, it doesn't space pack them. To do that, we would want to sort items by size, and add largest first.
	// This is also skipping possible serialization of grid data.
	// @todo handle serialization loading
	// @todo handle items that are too large to fit / too many items (log error?)
	Context.Unwrap().OccupiedCells.Reset(Context.GetGridSize());

	for (Container::FIterator_AllAddresses It(Context.GetStorage()); It; ++It)
	{
		const FFaerieItemInstance Instance = It.GetInstance();
		const FFaerieAddress Address = It.GetAddress();
		if (!AddItemToGrid(Context, Address, Instance))
		{
			// @todo Cannot add this item, ignore and try next for now...
		}
	}
}

EFaerieExtensionResponse UInventorySimpleGridExtension::AllowsAddition(const FFaerieContainerGridReadContext& Context,
																	  const Utils::TArrayAdapter<FFaerieItemProxy>& Proxies,
																	  const FFaerieExtensionAllowsAdditionArgs Args) const
{
	if (Context.GetEmptyCellCount() >= Proxies.Num())
	{
		return EFaerieExtensionResponse::Allowed;
	}
	return EFaerieExtensionResponse::Disallowed;
}

EFaerieExtensionResponse UInventorySimpleGridExtension::AllowsEdit(const FFaerieContainerGridReadContext& Context,
																  const TNotNull<const Container::IAddressView*> DataView,
																  const FFaerieInventoryTag EditType) const
{
	if (EditType == Inventory::Tags::Split)
	{
		if (Context.IsFull())
		{
			// If we are full, no open slots to split into.
			return EFaerieExtensionResponse::Disallowed;
		}
	}

	return EFaerieExtensionResponse::NoExplicitResponse;
}

void UInventorySimpleGridExtension::HandleEvent(const FFaerieContainerGridWriteContext& Context, const Container::FEvent& Event) const
{
	if (Event.IsAdditionEvent())
	{
		for (const FFaerieAddress Address : Event.AddressesTouched)
		{
			if (Context.IsInGrid(Address))
			{
				// Already in the grid...
				continue;
			}

			(void)AddItemToGrid(Context, Address, Event.Instance);
		}
	}
	else if (Event.IsRemovalEvent())
	{
		// Create a temporary array to store addresses that need to be removed
		TArray<FFaerieAddress, TInlineAllocator<8>> AddressesToRemove;

		for (const FFaerieAddress Address : Event.AddressesTouched)
        {
            if (Context.GetStorage()->ContainsAddress(Address))
            {
                PostStackChange(Context, { Address, Context.GetStackPlacementData(Address) });
            }
            else
            {
                AddressesToRemove.Add(Address);
            }
        }
        RemoveItemBatch(Context, AddressesToRemove, Event.Instance);
	}
	else
	{
		check(Event.IsEditEvent())

		// Create a temporary array to store addresses that need to be removed
		TArray<FFaerieAddress, TInlineAllocator<8>> AddressesToRemove;

        // get keys to remove
		for (const FFaerieAddress Address : Event.AddressesTouched)
        {
            if (!Context.GetStorage()->ContainsAddress(Address))
            {
                AddressesToRemove.Add(Address);
            }
            else
            {
            	if (Context.IsInGrid(Address))
                {
                    BroadcastEvent(Address, EFaerieGridEventType::ItemChanged);
                }
                else
                {
                    (void)AddItemToGrid(Context, Address, Event.Instance);
                }
            }
        }

        // remove the stored keys
        for (const FFaerieAddress& AddressToRemove : AddressesToRemove)
        {
        	if (auto Instance = Context.GetStorage()->ViewInstance(AddressToRemove);
        		Instance.IsSet())
        	{
        		RemoveItem(Context, AddressToRemove, Instance.GetValue());
        	}
            BroadcastEvent(AddressToRemove, EFaerieGridEventType::ItemRemoved);
        }
        Context.Unwrap().GridContent.MarkArrayDirty();
	}
}

void UInventorySimpleGridExtension::PreStackRemove_Client(const FFaerieContainerGridWriteContext& Context, const FFaerieGridKeyedStack& Stack) const
{
	// This is to account for removals through proxies that don't directly interface with the grid
	Context.Unwrap().OccupiedCells.UnmarkCell(Stack.Value.Origin);
	BroadcastEvent(Stack.Key, EFaerieGridEventType::ItemRemoved);
}

void UInventorySimpleGridExtension::PreStackRemove_Server(const FFaerieContainerGridWriteContext& Context, const FFaerieGridKeyedStack& Stack, const FFaerieItemInstance& Item) const
{
	// This is to account for removals through proxies that don't directly interface with the grid
	Context.Unwrap().OccupiedCells.UnmarkCell(Stack.Value.Origin);
	BroadcastEvent(Stack.Key, EFaerieGridEventType::ItemRemoved);
}

void UInventorySimpleGridExtension::PostStackAdd(const FFaerieContainerGridWriteContext& Context, const FFaerieGridKeyedStack& Stack) const
{
	BroadcastEvent(Stack.Key, EFaerieGridEventType::ItemAdded);
}

void UInventorySimpleGridExtension::PostStackChange(const FFaerieContainerGridWriteContext& Context, const FFaerieGridKeyedStack& Stack) const
{
	if (Context.GetStorage()->ContainsAddress(Stack.Key))
	{
		BroadcastEvent(Stack.Key, EFaerieGridEventType::ItemChanged);
	}
}

TOptional<FFaerieAddress> UInventorySimpleGridExtension::GetKeyAt(const FFaerieContainerGridReadContext& Context, const FIntPoint& Position) const
{
	if (const FFaerieGridKeyedStack* Stack = Context.GetGrid().Find(Position))
	{
		return Stack->Key;
	}
	return NullOpt;
}

bool UInventorySimpleGridExtension::CanAddAtLocation(const FFaerieContainerGridReadContext& Context, const TValid<const FFaerieItemProxy&> Proxy, const FIntPoint IntPoint) const
{
	return !Context.IsCellOccupied(IntPoint);
}

bool UInventorySimpleGridExtension::AddItemToGrid(const FFaerieContainerGridWriteContext& Context, const FFaerieAddress Address, const FFaerieItemInstance& Instance) const
{
	if (!Address.IsValid())
	{
		return false;
	}

	const FFaerieGridPlacement DesiredItemPlacement = FindFirstEmptyLocation(Context);

	if (DesiredItemPlacement.Origin == FIntPoint::NoneValue)
	{
		return false;
	}

	Context.Unwrap().GridContent.Insert(Address, DesiredItemPlacement);
	Context.Unwrap().OccupiedCells.MarkCell(DesiredItemPlacement.Origin);
	return true;
}

bool UInventorySimpleGridExtension::MoveItem(const FFaerieContainerGridWriteContext& Context, const FFaerieAddress Address, const FIntPoint& TargetPoint) const
{
	if (const TOptional<FFaerieAddress> OverlappingAddress = FindOverlappingItem(Context, TargetPoint, Address);
		OverlappingAddress.IsSet())
	{
		const TTuple<FFaerieEntryKey, FFaerieStackKey> Key = UFaerieItemStorage::BreakAddress(Address);
		const TTuple<FFaerieEntryKey, FFaerieStackKey> OverlappingKey = UFaerieItemStorage::BreakAddress(OverlappingAddress.GetValue());

		// If the Entry keys are identical, it gives us some other things to test before Swapping.
		if (Key.Get<0>() == OverlappingKey.Get<0>())
		{
			if (Key.Get<1>() == OverlappingKey.Get<1>())
			{
				// It's the same stack? No point in this!
				return false;
			}

			// Try merging them. This is known to be safe, since all stacks with the same key share immutability.
			if (Context.GetStorage()->MergeStacks(Key.Get<0>(), Key.Get<1>(), OverlappingKey.Get<1>()))
			{
				return true;
			}
		}

		const FFaerieGridContent::FScopedStackHandle HandleA = Context.Unwrap().GridContent.GetHandle(Address);
		const FFaerieGridContent::FScopedStackHandle HandleB = Context.Unwrap().GridContent.GetHandle(OverlappingAddress.GetValue());
		Swap(HandleA.Get().Origin, HandleB.Get().Origin);
		// No need to change cell marking, because swaps don't change any.
		return true;
	}

	const FFaerieGridContent::FScopedStackHandle Handle = Context.Unwrap().GridContent.GetHandle(Address);
	MoveSingleItem(Context, Handle.Get(), TargetPoint);
	return true;
}

bool UInventorySimpleGridExtension::RotateItem(const FFaerieContainerGridWriteContext& Context, const FFaerieAddress Address, const EFaerieSpatialItemRotation RotationToAdd) const
{
	const FFaerieGridContent::FScopedStackHandle Handle = Context.Unwrap().GridContent.GetHandle(Address);
	Handle->Rotation = Spatial::AddRotations(Handle->Rotation, RotationToAdd);
	return true;
}

void UInventorySimpleGridExtension::RemoveItem(const FFaerieContainerGridWriteContext& Context, const FFaerieAddress Address, const FFaerieItemInstance& Instance) const
{
	Context.Unwrap().GridContent.BSOA::Remove(Address,
		[Instance, Context, this](const FFaerieGridKeyedStack& Stack)
		{
			PreStackRemove_Server(Context, Stack, Instance);
		});
}

void UInventorySimpleGridExtension::RemoveItemBatch(const FFaerieContainerGridWriteContext& Context, const TConstArrayView<FFaerieAddress>& Keys, const FFaerieItemInstance& Instance) const
{
	for (const FFaerieAddress& KeyToRemove : Keys)
	{
		RemoveItem(Context, KeyToRemove, Instance);
		BroadcastEvent(KeyToRemove, EFaerieGridEventType::ItemRemoved);
	}
	Context.Unwrap().GridContent.MarkArrayDirty();
}

FFaerieGridPlacement UInventorySimpleGridExtension::FindFirstEmptyLocation(const FFaerieContainerGridReadContext& Context)
{
	const FIntVector2 GridSize = Context.GetGridSize();

	// For each cell in the grid
	FIntPoint TestPoint = FIntPoint::ZeroValue;
	for (TestPoint.Y = 0; TestPoint.Y < GridSize.Y; TestPoint.Y++)
	{
		for (TestPoint.X = 0; TestPoint.X < GridSize.X; TestPoint.X++)
		{
			// Skip if current cell is occupied
			if (Context.IsCellOccupied(TestPoint))
			{
				continue;
			}

			return FFaerieGridPlacement(TestPoint);
		}
	}
	// No valid placement found
	return FFaerieGridPlacement{FIntPoint::NoneValue};
}

TOptional<FFaerieAddress> UInventorySimpleGridExtension::FindOverlappingItem(const FFaerieContainerGridReadContext& Context, const FIntPoint& Position, const FFaerieAddress ExcludeAddress) const
{
	if (TOptional<FFaerieAddress> Address = GetKeyAt(Context, Position);
		Address.IsSet())
	{
		if (Address.GetValue() != ExcludeAddress)
		{
			return Address.GetValue();
		}
	}
	return FFaerieAddress();
}

void UInventorySimpleGridExtension::MoveSingleItem(const FFaerieContainerGridWriteContext& Context, FFaerieGridPlacement& Placement, const FIntPoint& NewPosition)
{
	// Clear old position first
	Context.Unwrap().OccupiedCells.UnmarkCell(Placement.Origin);

	// Then set new positions
	Context.Unwrap().OccupiedCells.MarkCell(NewPosition);

	Placement.Origin = NewPosition;
}