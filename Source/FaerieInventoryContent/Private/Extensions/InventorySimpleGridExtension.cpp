// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Extensions/InventorySimpleGridExtension.h"

#include "FaerieItemContainerBase.h"
#include "FaerieItemStorage.h"
#include "ItemContainerEvent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventorySimpleGridExtension)

EEventExtensionResponse UInventorySimpleGridExtension::AllowsAddition(const UFaerieItemContainerBase* Container,
																	  const TConstArrayView<FFaerieItemStackView> Views,
																	  const FFaerieExtensionAllowsAdditionArgs Args) const
{
	if (OccupiedCells.GetNumUnmarked() > Views.Num())
	{
		return EEventExtensionResponse::Allowed;
	}
	return EEventExtensionResponse::Disallowed;
}

void UInventorySimpleGridExtension::PostAddition(const UFaerieItemContainerBase* Container,
												  const Faerie::Inventory::FEventLog& Event)
{
	// @todo don't add items for existing keys

	for (const FStackKey StackKey : Event.StackKeys)
	{
		AddItemToGrid(UFaerieItemStorage::MakeAddress(Event.EntryTouched, StackKey), Event.Item.Get());
	}
}

void UInventorySimpleGridExtension::PostRemoval(const UFaerieItemContainerBase* Container,
												 const Faerie::Inventory::FEventLog& Event)
{
	if (const UFaerieItemStorage* ItemStorage = Cast<UFaerieItemStorage>(Container))
	{
		// Create a temporary array to store keys that need to be removed
		TArray<FFaerieAddress> AddressesToRemove;

		for (const FStackKey StackKey : Event.StackKeys)
		{
			if (FFaerieAddress CurrentAddress = UFaerieItemStorage::MakeAddress(Event.EntryTouched, StackKey);
				ItemStorage->Contains(CurrentAddress))
			{
				PostStackChange({ CurrentAddress, GetStackPlacementData(CurrentAddress) });
			}
			else
			{
				AddressesToRemove.Add(CurrentAddress);
			}
		}
		RemoveItemBatch(AddressesToRemove, Event.Item.Get());
	}
}

EEventExtensionResponse UInventorySimpleGridExtension::AllowsEdit(const UFaerieItemContainerBase* Container,
																  const FEntryKey Key,
																  const FFaerieInventoryTag EditType) const
{
	if (EditType == Faerie::Inventory::Tags::Split)
	{
		if (OccupiedCells.IsFull())
		{
			return EEventExtensionResponse::Disallowed;
		}
	}

	return EEventExtensionResponse::NoExplicitResponse;
}

void UInventorySimpleGridExtension::PostEntryChanged(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event)
{
	// Create a temporary array to store keys that need to be removed
	TArray<FFaerieAddress> KeysToRemove;

	// @todo update logic to use Event
	// get keys to remove
	for (const auto& SpatialEntry : GridContent)
	{
		if (const UFaerieItemStorage* Storage = Cast<UFaerieItemStorage>(InitializedContainer);
			!Storage->Contains(SpatialEntry.Key))
		{
			KeysToRemove.Add(SpatialEntry.Key);
		}
		else
		{
			BroadcastEvent(SpatialEntry.Key, EFaerieGridEventType::ItemChanged);
		}
	}

	// remove the stored keys
	for (const FFaerieAddress& AddressToRemove : KeysToRemove)
	{
		RemoveItem(AddressToRemove, Container->ViewItem(AddressToRemove));
		BroadcastEvent(AddressToRemove, EFaerieGridEventType::ItemRemoved);
	}
	GridContent.MarkArrayDirty();
}


void UInventorySimpleGridExtension::PreStackRemove_Client(const FFaerieGridKeyedStack& Stack)
{
	// This is to account for removals through proxies that don't directly interface with the grid
	OccupiedCells.UnmarkCell(Stack.Value.Origin);
	BroadcastEvent(Stack.Key, EFaerieGridEventType::ItemRemoved);
}

void UInventorySimpleGridExtension::PreStackRemove_Server(const FFaerieGridKeyedStack& Stack, const UFaerieItem* Item)
{
	// This is to account for removals through proxies that don't directly interface with the grid
	OccupiedCells.UnmarkCell(Stack.Value.Origin);
	BroadcastEvent(Stack.Key, EFaerieGridEventType::ItemRemoved);
}

void UInventorySimpleGridExtension::PostStackAdd(const FFaerieGridKeyedStack& Stack)
{
	BroadcastEvent(Stack.Key, EFaerieGridEventType::ItemAdded);
}

void UInventorySimpleGridExtension::PostStackChange(const FFaerieGridKeyedStack& Stack)
{
	if (const UFaerieItemStorage* Storage = Cast<UFaerieItemStorage>(InitializedContainer); Storage->Contains(Stack.Key))
	{
		BroadcastEvent(Stack.Key, EFaerieGridEventType::ItemChanged);
	}
}

FFaerieAddress UInventorySimpleGridExtension::GetKeyAt(const FIntPoint& Position) const
{
	for (auto&& Element : GridContent)
	{
		if (Element.Value.Origin == Position)
		{
			return Element.Key;
		}
	}
	return FFaerieAddress();
}

bool UInventorySimpleGridExtension::CanAddAtLocation(const FFaerieItemStackView Stack, const FIntPoint IntPoint) const
{
	return !IsCellOccupied(IntPoint);
}

bool UInventorySimpleGridExtension::AddItemToGrid(const FFaerieAddress Address, const UFaerieItem* Item)
{
	if (!Address.IsValid())
	{
		return false;
	}

	if (GridContent.Contains(Address))
	{
		// Already in the grid...
		return true;
	}

	const FFaerieGridPlacement DesiredItemPlacement = FindFirstEmptyLocation();

	if (DesiredItemPlacement.Origin == FIntPoint::NoneValue)
	{
		return false;
	}

	GridContent.Insert(Address, DesiredItemPlacement);
	OccupiedCells.MarkCell(DesiredItemPlacement.Origin);
	return true;
}

bool UInventorySimpleGridExtension::MoveItem(const FFaerieAddress Address, const FIntPoint& TargetPoint)
{
	if (const FFaerieAddress OverlappingAddress = FindOverlappingItem(Address);
		OverlappingAddress.IsValid())
	{
		const TTuple<FEntryKey, FStackKey> Key = UFaerieItemStorage::BreakAddress(Address);
		const TTuple<FEntryKey, FStackKey> OverlappingKey = UFaerieItemStorage::BreakAddress(OverlappingAddress);

		// If the Entry keys are identical, it gives us some other things to test before Swapping.
		if (Key.Get<0>() == OverlappingKey.Get<0>())
		{
			if (Key.Get<1>() == OverlappingKey.Get<1>())
			{
				// It's the same stack? No point in this!
				return false;
			}

			// Try merging them. This is known to be safe, since all stacks with the same key share immutability.
			if (UFaerieItemStorage* Storage = Cast<UFaerieItemStorage>(InitializedContainer);
				Storage->MergeStacks(Key.Get<0>(), Key.Get<1>(), OverlappingKey.Get<1>()))
			{
				return true;
			}
		}

		const FFaerieGridContent::FScopedStackHandle HandleA = GridContent.GetHandle(Address);
		const FFaerieGridContent::FScopedStackHandle HandleB = GridContent.GetHandle(OverlappingAddress);
		SwapItems(HandleA.Get(), HandleB.Get());
		return true;
	}

	const FFaerieGridContent::FScopedStackHandle Handle = GridContent.GetHandle(Address);
	MoveSingleItem(Handle.Get(), TargetPoint);
	return true;
}

bool UInventorySimpleGridExtension::RotateItem(const FFaerieAddress Address)
{
	const FFaerieGridContent::FScopedStackHandle Handle = GridContent.GetHandle(Address);
	Handle->Rotation = GetNextRotation(Handle->Rotation);
	return true;
}

void UInventorySimpleGridExtension::RemoveItem(const FFaerieAddress Address, const UFaerieItem* Item)
{
	GridContent.BSOA::Remove(Address,
		[Item, this](const FFaerieGridKeyedStack& Stack)
		{
			PreStackRemove_Server(Stack, Item);
		});
}

void UInventorySimpleGridExtension::RemoveItemBatch(const TConstArrayView<FFaerieAddress>& Keys, const UFaerieItem* Item)
{
	for (const FFaerieAddress& KeyToRemove : Keys)
	{
		RemoveItem(KeyToRemove, Item);
		BroadcastEvent(KeyToRemove, EFaerieGridEventType::ItemRemoved);
	}
	GridContent.MarkArrayDirty();
}

FFaerieGridPlacement UInventorySimpleGridExtension::FindFirstEmptyLocation() const
{
	// Early exit if grid is empty or invalid
	if (GridSize.X <= 0 || GridSize.Y <= 0)
	{
		return FFaerieGridPlacement{FIntPoint::NoneValue};
	}

	// For each cell in the grid
	FIntPoint TestPoint = FIntPoint::ZeroValue;
	for (TestPoint.Y = 0; TestPoint.Y < GridSize.Y; TestPoint.Y++)
	{
		for (TestPoint.X = 0; TestPoint.X < GridSize.X; TestPoint.X++)
		{
			// Skip if current cell is occupied
			if (IsCellOccupied(TestPoint))
			{
				continue;
			}

			return FFaerieGridPlacement(TestPoint);
		}
	}
	// No valid placement found
	return FFaerieGridPlacement{FIntPoint::NoneValue};
}

FFaerieAddress UInventorySimpleGridExtension::FindOverlappingItem(const FFaerieAddress ExcludeAddress) const
{
	if (GridContent.Contains(ExcludeAddress))
	{
		return GridContent.GetElement(ExcludeAddress).Key;
	}
	return FFaerieAddress();
}

void UInventorySimpleGridExtension::SwapItems(FFaerieGridPlacement& PlacementA, FFaerieGridPlacement& PlacementB)
{
	Swap(PlacementA.Origin, PlacementB.Origin);
	// No need to change cell marking, because swaps don't change any.
}

void UInventorySimpleGridExtension::MoveSingleItem(FFaerieGridPlacement& Placement, const FIntPoint& NewPosition)
{
	// Clear old position first
	OccupiedCells.UnmarkCell(Placement.Origin);

	// Then set new positions
	OccupiedCells.MarkCell(NewPosition);

	Placement.Origin = NewPosition;
}