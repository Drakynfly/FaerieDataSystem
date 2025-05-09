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

	FInventoryKey NewKey;
	NewKey.EntryKey = Event.EntryTouched;

	for (const FStackKey StackKey : Event.StackKeys)
	{
		NewKey.StackKey = StackKey;
		AddItemToGrid(NewKey, Event.Item.Get());
	}
}

void UInventorySimpleGridExtension::PostRemoval(const UFaerieItemContainerBase* Container,
												 const Faerie::Inventory::FEventLog& Event)
{
	if (const UFaerieItemStorage* ItemStorage = Cast<UFaerieItemStorage>(Container))
	{
		// Create a temporary array to store keys that need to be removed
		TArray<FInventoryKey> KeysToRemove;

		for (auto&& StackKey : Event.StackKeys)
		{
			if (FInventoryKey CurrentKey{Event.EntryTouched, StackKey};
				ItemStorage->IsValidKey(CurrentKey))
			{
				PostStackChange({ CurrentKey, GetStackPlacementData(CurrentKey) });
			}
			else
			{
				KeysToRemove.Add(CurrentKey);
			}
		}
		RemoveItemBatch(KeysToRemove, Event.Item.Get());
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
	TArray<FInventoryKey> KeysToRemove;

	// @todo update logic to use Event
	// get keys to remove
	for (const auto& SpatialEntry : GridContent)
	{
		if (const UFaerieItemStorage* Storage = Cast<UFaerieItemStorage>(InitializedContainer);
			!Storage->IsValidKey(SpatialEntry.Key))
		{
			KeysToRemove.Add(SpatialEntry.Key);
		}
		else
		{
			BroadcastEvent(SpatialEntry.Key, EFaerieGridEventType::ItemChanged);
		}
	}

	// remove the stored keys
	for (const FInventoryKey& KeyToRemove : KeysToRemove)
	{
		RemoveItem(KeyToRemove, Container->View(KeyToRemove.EntryKey).Item.Get());
		BroadcastEvent(KeyToRemove, EFaerieGridEventType::ItemRemoved);
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
	if (const UFaerieItemStorage* Storage = Cast<UFaerieItemStorage>(InitializedContainer); Storage->IsValidKey(Stack.Key))
	{
		BroadcastEvent(Stack.Key, EFaerieGridEventType::ItemChanged);
	}
}

FInventoryKey UInventorySimpleGridExtension::GetKeyAt(const FIntPoint& Position) const
{
	for (auto&& Element : GridContent)
	{
		if (Element.Value.Origin == Position)
		{
			return Element.Key;
		}
	}
	return FInventoryKey();
}

bool UInventorySimpleGridExtension::CanAddAtLocation(const FFaerieItemStackView Stack, const FIntPoint IntPoint) const
{
	return !IsCellOccupied(IntPoint);
}

bool UInventorySimpleGridExtension::AddItemToGrid(const FInventoryKey& Key, const UFaerieItem* Item)
{
	if (!Key.IsValid())
	{
		return false;
	}

	if (GridContent.Find(Key) != nullptr)
	{
		return true;
	}

	const FFaerieGridPlacement DesiredItemPlacement = FindFirstEmptyLocation();

	if (DesiredItemPlacement.Origin == FIntPoint::NoneValue)
	{
		return false;
	}

	GridContent.Insert(Key, DesiredItemPlacement);
	OccupiedCells.MarkCell(DesiredItemPlacement.Origin);
	return true;
}

bool UInventorySimpleGridExtension::MoveItem(const FInventoryKey& Key, const FIntPoint& TargetPoint)
{
	if (const FInventoryKey OverlappingKey = FindOverlappingItem(Key);
		OverlappingKey.IsValid())
	{
		// If the Entry keys are identical, it gives us some other things to test before Swapping.
		if (Key.EntryKey == OverlappingKey.EntryKey)
		{
			if (Key.StackKey == OverlappingKey.StackKey)
			{
				// It's the same stack? No point in this!
				return false;
			}

			// Try merging them. This is known to be safe, since all stacks with the same key share immutability.
			if (UFaerieItemStorage* Storage = Cast<UFaerieItemStorage>(InitializedContainer);
				Storage->MergeStacks(Key.EntryKey, Key.StackKey, OverlappingKey.StackKey))
			{
				return true;
			}
		}

		const FFaerieGridContent::FScopedStackHandle HandleA = GridContent.GetHandle(Key);
		const FFaerieGridContent::FScopedStackHandle HandleB = GridContent.GetHandle(OverlappingKey);
		SwapItems(HandleA.Get(), HandleB.Get());
		return true;
	}

	const FFaerieGridContent::FScopedStackHandle Handle = GridContent.GetHandle(Key);
	MoveSingleItem(Handle.Get(), TargetPoint);
	return true;
}

bool UInventorySimpleGridExtension::RotateItem(const FInventoryKey& Key)
{
	const FFaerieGridContent::FScopedStackHandle Handle = GridContent.GetHandle(Key);
	Handle->Rotation = GetNextRotation(Handle->Rotation);
	return true;
}

void UInventorySimpleGridExtension::RemoveItem(const FInventoryKey& Key, const UFaerieItem* Item)
{
	GridContent.BSOA::Remove(Key,
		[Item, this](const FFaerieGridKeyedStack& Stack)
		{
			PreStackRemove_Server(Stack, Item);
		});
}

void UInventorySimpleGridExtension::RemoveItemBatch(const TConstArrayView<FInventoryKey>& Keys, const UFaerieItem* Item)
{
	for (const FInventoryKey& KeyToRemove : Keys)
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

FInventoryKey UInventorySimpleGridExtension::FindOverlappingItem(const FInventoryKey& ExcludeKey) const
{
	if (GridContent.Contains(ExcludeKey))
	{
		return GridContent.GetElement(ExcludeKey).Key;
	}
	return FInventoryKey();
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