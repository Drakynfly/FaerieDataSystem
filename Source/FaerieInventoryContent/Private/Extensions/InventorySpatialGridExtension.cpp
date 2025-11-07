// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Extensions/InventorySpatialGridExtension.h"
#include "FaerieInventoryContentLog.h"

#include "FaerieItemContainerBase.h"
#include "FaerieItemStorage.h"
#include "ItemContainerEvent.h"
#include "Tokens/FaerieShapeToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventorySpatialGridExtension)

DECLARE_STATS_GROUP(TEXT("InventorySpatialGridExtension"), STATGROUP_FaerieSpatialGrid, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Client OccupiedCells rebuild"), STAT_Client_CellRebuild, STATGROUP_FaerieSpatialGrid);

namespace Faerie
{
	FFaerieGridPlacement FindFirstEmptyLocation(const FCellGrid& Grid, const FFaerieGridShapeConstView& Shape)
	{
		const FIntPoint GridSize = Grid.GetDimensions();

		// Early exit if grid is empty or invalid
		if (GridSize.X <= 0 || GridSize.Y <= 0)
		{
			return FFaerieGridPlacement{FIntPoint::NoneValue};
		}

		// Determine which rotations to check
		TArray<ESpatialItemRotation, TInlineAllocator<4>> RotationRange;
		if (Shape.IsSymmetrical())
		{
			RotationRange.Add(ESpatialItemRotation::None);
		}
		else
		{
			for (const ESpatialItemRotation Rotation : TEnumRange<ESpatialItemRotation>())
			{
				RotationRange.Add(Rotation);
			}
		}

		// Find top left most point
		FIntPoint FirstPoint = FIntPoint(TNumericLimits<int32>::Max());
		for (const FIntPoint& Point : Shape.Points)
		{
			if (Point.Y < FirstPoint.Y || (Point.Y == FirstPoint.Y && Point.X < FirstPoint.X))
			{
				FirstPoint = Point;
			}
		}

		FFaerieGridPlacement TestPlacement;

		// For each cell in the grid
		FIntPoint TestPoint = FIntPoint::ZeroValue;
		for (TestPoint.Y = 0; TestPoint.Y < GridSize.Y; TestPoint.Y++)
		{
			for (TestPoint.X = 0; TestPoint.X < GridSize.X; TestPoint.X++)
			{
				// Skip if current cell is occupied
				if (Grid.GetCell(TestPoint))
				{
					continue;
				}

				// Calculate the origin offset by the first point
				TestPlacement.Origin = TestPoint - FirstPoint;

				for (const ESpatialItemRotation Rotation : RotationRange)
				{
					TestPlacement.Rotation = Rotation;
					const FFaerieGridShape Translated = ApplyPlacement(Shape, TestPlacement); // @todo this is *way* too many array allocations. optimize this!
					if (FitsInGrid(Grid, Translated, {}))
					{
						return TestPlacement;
					}
				}
			}
		}
		// No valid placement found
		return FFaerieGridPlacement{FIntPoint::NoneValue};
	}

	FFaerieGridShape ApplyPlacement(const FFaerieGridShapeConstView& Shape, const FFaerieGridPlacement& Placement, const bool bNormalize, const bool Reset)
	{
		if (bNormalize)
		{
			return Shape.Copy().Rotate(Placement.Rotation, Reset).Normalize().Translate(Placement.Origin);
		}
		return Shape.Copy().Rotate(Placement.Rotation, Reset).Translate(Placement.Origin);
	}

	void ApplyPlacementInline(FFaerieGridShape& Shape, const FFaerieGridPlacement& Placement, const bool bNormalize)
	{
		Shape.RotateInline(Placement.Rotation);
		if (bNormalize)
		{
			Shape.NormalizeInline();
		}
		Shape.TranslateInline(Placement.Origin);
	}

	bool FitsInGrid(const FCellGrid& Grid, const FFaerieGridShapeConstView& TranslatedShape, const FExclusionSet& ExclusionSet)
	{
		const FIntPoint GridSize = Grid.GetDimensions();

		// Calculate shape bounds
		const FIntRect Bounds = TranslatedShape.GetBounds();

		// Early exit if shape is obviously too large
		if (Bounds.Max.X > GridSize.X || Bounds.Max.Y > GridSize.Y)
		{
			UE_LOG(LogFaerieInventoryContent, Warning, TEXT("Item Too Big"));
			return false;
		}

		// Check if all points in the shape fit within the grid and don't overlap with occupied cells
		for (const FIntPoint& Point : TranslatedShape.Points)
		{
			// Check if point is within grid bounds
			if (Point.X < 0 || Point.X >= GridSize.X ||
				Point.Y < 0 || Point.Y >= GridSize.Y)
			{
				UE_LOG(LogFaerieInventoryContent, Warning, TEXT("Item Falls Outside Grid"));
				return false;
			}

			// If this index is not in the excluded list, check if it's occupied
			if (!ExclusionSet.Contains(Point) && Grid.GetCell(Point))
			{
				UE_LOG(LogFaerieInventoryContent, Warning, TEXT("Cell Is Occupied"));
				return false;
			}
		}

		return true;
	}

	void MarkShapeCells(FCellGrid& Grid, const FFaerieGridShapeConstView TranslatedShape)
	{
		for (auto& Point : TranslatedShape.Points)
		{
			Grid.MarkCell(Point);
		}
	}

	void UnmarkShapeCells(FCellGrid& Grid, const FFaerieGridShapeConstView& TranslatedShape)
	{
		for (auto& Point : TranslatedShape.Points)
		{
			Grid.UnmarkCell(Point);
		}
	}
}

EEventExtensionResponse UInventorySpatialGridExtension::AllowsAddition(const UFaerieItemContainerBase* Container,
																	   const TConstArrayView<FFaerieItemStackView> Views,
																	   const FFaerieExtensionAllowsAdditionArgs Args) const
{
	// @todo add boolean in config to allow items without a shape

	if (Views.Num() == 1)
	{
		if (!CanAddItemToGrid(GetItemShape_Impl(Views[0].Item.Get())))
		{
			return EEventExtensionResponse::Disallowed;
		}
	}

	switch (Args.TestType)
	{
	case EFaerieStorageAddStackTestMultiType::IndividualTests:
		{
			for (auto View : Views)
			{
				if (!CanAddItemToGrid(GetItemShape_Impl(View.Item.Get())))
				{
					return EEventExtensionResponse::Disallowed;
				}
			}

			return EEventExtensionResponse::Allowed;
		}

	case EFaerieStorageAddStackTestMultiType::GroupTest:
		{
			TArray<FFaerieGridShapeConstView> Shapes;
			for (auto&& View : Views)
			{
				Shapes.Add(GetItemShape_Impl(View.Item.Get()));
			}

			if (!CanAddItemsToGrid(Shapes))
			{
				return EEventExtensionResponse::Disallowed;
			}
			return EEventExtensionResponse::Allowed;
		}
	}

	// Should not reach this;
	return EEventExtensionResponse::NoExplicitResponse;
}

void UInventorySpatialGridExtension::PostAddition(const UFaerieItemContainerBase* Container,
												const Faerie::Inventory::FEventLog& Event)
{
	// @todo don't add items for existing keys

	for (const FStackKey StackKey : Event.StackKeys)
	{
		AddItemToGrid(UFaerieItemStorage::MakeAddress(Event.EntryTouched, StackKey), Event.Item.Get());
	}
}

void UInventorySpatialGridExtension::PostRemoval(const UFaerieItemContainerBase* Container,
												 const Faerie::Inventory::FEventLog& Event)
{
	if (const UFaerieItemStorage* ItemStorage = Cast<UFaerieItemStorage>(Container))
	{
		// Create a temporary array to store keys that need to be removed
		TArray<FFaerieAddress> AddressesToRemove;

		for (const FStackKey StackKey : Event.StackKeys)
		{
			if (const FFaerieAddress Address = UFaerieItemStorage::MakeAddress(Event.EntryTouched, StackKey);
				ItemStorage->Contains(Address))
			{
				PostStackChange({ Address, GetStackPlacementData(Address) });
			}
			else
			{
				AddressesToRemove.Add(Address);
			}
		}
		RemoveItemBatch(AddressesToRemove, Event.Item.Get());
	}
}

EEventExtensionResponse UInventorySpatialGridExtension::AllowsEdit(const UFaerieItemContainerBase* Container,
																	const FEntryKey Key, const FFaerieInventoryTag EditType) const
{
	if (EditType == Faerie::Inventory::Tags::Split)
	{
		if (!CanAddItemToGrid(GetItemShape_Impl(Container->View(Key).Item.Get())))
		{
			return EEventExtensionResponse::Disallowed;
		}
	}

	return EEventExtensionResponse::NoExplicitResponse;
}

void UInventorySpatialGridExtension::PostEntryChanged(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event)
{
	// Create a temporary array to store keys that need to be removed
	TArray<FFaerieAddress> AddressesToRemove;

	// get addresses to remove
	for (const FStackKey AffectedKey : Event.StackKeys)
	{
		const FFaerieAddress CurrentAddress = UFaerieItemStorage::MakeAddress(Event.EntryTouched, AffectedKey);
		if (const UFaerieItemStorage* Storage = Cast<UFaerieItemStorage>(InitializedContainer);
			!Storage->Contains(CurrentAddress))
		{
			AddressesToRemove.Add(CurrentAddress);
		}
		else
		{
			if (GridContent.Contains(CurrentAddress))
			{
				BroadcastEvent(CurrentAddress, EFaerieGridEventType::ItemChanged);
			}
			else
			{
				AddItemToGrid(CurrentAddress, Event.Item.Get());
			}
		}
	}

	// remove the stored keys
	RemoveItemBatch(AddressesToRemove, Event.Item.Get());
}

void UInventorySpatialGridExtension::PreStackRemove_Client(const FFaerieGridKeyedStack& Stack)
{
	RebuildOccupiedCells();

	BroadcastEvent(Stack.Key, EFaerieGridEventType::ItemRemoved);
}

void UInventorySpatialGridExtension::PreStackRemove_Server(const FFaerieGridKeyedStack& Stack, const UFaerieItem* Item)
{
	// This is to account for removals through proxies that don't directly interface with the grid
	const FFaerieGridShape Translated = Faerie::ApplyPlacement(GetItemShape_Impl(Item), Stack.Value);
	UnmarkShapeCells(OccupiedCells, Translated);

	BroadcastEvent(Stack.Key, EFaerieGridEventType::ItemRemoved);
}

void UInventorySpatialGridExtension::PostStackAdd(const FFaerieGridKeyedStack& Stack)
{
	BroadcastEvent(Stack.Key, EFaerieGridEventType::ItemAdded);
}

void UInventorySpatialGridExtension::PostStackChange(const FFaerieGridKeyedStack& Stack)
{
	if (const UFaerieItemStorage* Storage = Cast<UFaerieItemStorage>(InitializedContainer);
		Storage->Contains(Stack.Key))
	{
		BroadcastEvent(Stack.Key, EFaerieGridEventType::ItemChanged);
	}
}

FFaerieAddress UInventorySpatialGridExtension::GetKeyAt(const FIntPoint& Position) const
{
	for (auto&& Element : GridContent)
	{
		// Easy check first
		if (Element.Value.Origin == Position) return Element.Key;

		FFaerieGridShape Shape = GetItemShape_Impl(Element.Key).Copy();
		Faerie::ApplyPlacementInline(Shape, Element.Value);
		if (Shape.Contains(Position))
		{
			return Element.Key;
		}
	}

	return FFaerieAddress();
}

bool UInventorySpatialGridExtension::CanAddAtLocation(const FFaerieItemStackView Stack, const FIntPoint IntPoint) const
{
	const FFaerieGridShapeConstView Shape = GetItemShape_Impl(Stack.Item.Get());
	return CanAddAtLocation(Shape, IntPoint);
}

bool UInventorySpatialGridExtension::AddItemToGrid(const FFaerieAddress Address, const UFaerieItem* Item)
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

	FFaerieGridShape Shape = GetItemShape_Impl(Item).Copy();

	const FFaerieGridPlacement DesiredItemPlacement = FindFirstEmptyLocation(OccupiedCells, Shape);

	if (DesiredItemPlacement.Origin == FIntPoint::NoneValue)
	{
		return false;
	}

	GridContent.Insert(Address, DesiredItemPlacement);

	Faerie::ApplyPlacementInline(Shape, DesiredItemPlacement);
	MarkShapeCells(OccupiedCells, Shape);

	return true;
}

bool UInventorySpatialGridExtension::MoveItem(const FFaerieAddress Address, const FIntPoint& TargetPoint)
{
	const FFaerieGridShapeConstView ItemShape = GetItemShape_Impl(Address);

	// Create placement at target point with current rotation
	const FFaerieGridPlacement NewPlacement(TargetPoint, GetStackPlacementData(Address).Rotation);

	// Get the rotated shape based on current stack rotation so we can correctly get items that would overlap
	const FFaerieGridShape NewShape = Faerie::ApplyPlacement(ItemShape, NewPlacement, true);

	// If this new position overlaps an existing item
	if (const FFaerieAddress OverlappingAddress = FindOverlappingItem(NewShape, Address);
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

		const FFaerieGridContent::FScopedStackHandle StackHandleA = GridContent.GetHandle(Address);
		const FFaerieGridContent::FScopedStackHandle StackHandleB = GridContent.GetHandle(OverlappingAddress);

		return TrySwapItems(
			Address, StackHandleA.Get(),
			OverlappingAddress, StackHandleB.Get());
	}

	// Copied logic from MoveSingleItem, but optimized to use existing variables.
	{
		const Faerie::FExclusionSet ExclusionSet = MakeExclusionSet(Address);
		if (!FitsInGrid(OccupiedCells, NewShape, ExclusionSet))
		{
			return false;
		}

		const FFaerieGridContent::FScopedStackHandle StackHandle = GridContent.GetHandle(Address);

		const FFaerieGridShape OldShape = Faerie::ApplyPlacement(ItemShape, StackHandle.Get(), true);
		UnmarkShapeCells(OccupiedCells, OldShape);
		StackHandle->Origin = TargetPoint;
		MarkShapeCells(OccupiedCells, NewShape);
	}

	return true;
}

bool UInventorySpatialGridExtension::RotateItem(const FFaerieAddress Address)
{
	const FFaerieGridShapeConstView ItemShape = GetItemShape_Impl(Address);

	// No Point in Trying to Rotate
	if (ItemShape.IsSymmetrical()) return false;

	const FFaerieGridContent::FScopedStackHandle Handle = GridContent.GetHandle(Address);

	// Store old points before transformations so we can clear them from the bit grid
	const FFaerieGridShape OldShape = Faerie::ApplyPlacement(ItemShape, Handle.Get(), true);

	FFaerieGridPlacement NewPlacement = GetStackPlacementData(Address);
	NewPlacement.Rotation = GetNextRotation(NewPlacement.Rotation);
	const FFaerieGridShape NewShape = Faerie::ApplyPlacement(ItemShape, NewPlacement, false, NewPlacement.Rotation == ESpatialItemRotation::None);

	const Faerie::FExclusionSet ExclusionSet = MakeExclusionSet(Address);
	if (!FitsInGrid(OccupiedCells, NewShape, ExclusionSet))
	{
		return false;
	}

	const FIntRect OldBounds = OldShape.GetBounds();
	const FIntRect NewBounds = NewShape.GetBounds();

	// Clear old occupied cells
	UnmarkShapeCells(OccupiedCells, OldShape);

	Handle->Rotation = NewPlacement.Rotation;
	if (OldBounds != NewBounds)
	{
		Handle->Origin = NewBounds.Min;
	}
	// Set new occupied cells taking into account rotation
	MarkShapeCells(OccupiedCells, NewShape);

	return true;
}

void UInventorySpatialGridExtension::RemoveItem(const FFaerieAddress Address, const UFaerieItem* Item)
{
	GridContent.BSOA::Remove(Address,
		[Item, this](const FFaerieGridKeyedStack& Stack)
		{
			PreStackRemove_Server(Stack, Item);
		});
}

void UInventorySpatialGridExtension::RemoveItemBatch(const TConstArrayView<FFaerieAddress>& Addresses, const UFaerieItem* Item)
{
	for (const FFaerieAddress AddressToRemove : Addresses)
	{
		RemoveItem(AddressToRemove, Item);
		BroadcastEvent(AddressToRemove, EFaerieGridEventType::ItemRemoved);
	}
	GridContent.MarkArrayDirty();
}

void UInventorySpatialGridExtension::RebuildOccupiedCells()
{
	SCOPE_CYCLE_COUNTER(STAT_Client_CellRebuild);

	OccupiedCells.Reset(GridSize);

	for (const auto& SpatialEntry : GridContent)
	{
		if (!InitializedContainer->Contains(SpatialEntry.Key))
		{
			continue;
		}

		if (const FFaerieGridShapeConstView Shape = GetItemShape_Impl(InitializedContainer->ViewItem(SpatialEntry.Key));
			Shape.IsValid())
		{
			const FFaerieGridShape Translated = Faerie::ApplyPlacement(Shape, SpatialEntry.Value);
			MarkShapeCells(OccupiedCells, Translated);
		}
	}
}

FFaerieGridShapeConstView UInventorySpatialGridExtension::GetItemShape_Impl(const UFaerieItem* Item) const
{
	if (IsValid(Item))
	{
		if (const UFaerieShapeToken* ShapeToken = Item->GetToken<UFaerieShapeToken>())
		{
			return ShapeToken->GetShape();
		}
		return FFaerieGridShape::Square1;
	}
	return FFaerieGridShapeConstView();
}

FFaerieGridShapeConstView UInventorySpatialGridExtension::GetItemShape_Impl(const FFaerieAddress Address) const
{
	if (IsValid(InitializedContainer))
	{
		const UFaerieItem* Item = InitializedContainer->ViewItem(Address);
		return GetItemShape_Impl(Item);
	}

	return FFaerieGridShapeConstView();
}

bool UInventorySpatialGridExtension::CanAddItemToGrid(const FFaerieGridShapeConstView& Shape) const
{
	const FFaerieGridPlacement TestPlacement = FindFirstEmptyLocation(OccupiedCells, Shape);
	return TestPlacement.Origin != FIntPoint::NoneValue;
}

bool UInventorySpatialGridExtension::CanAddItemsToGrid(const TArray<FFaerieGridShapeConstView>& Shapes) const
{
	// @todo obviously this is not very ideal. It just throws each item into the grid first place it goes. A proper shape-packing algo would be nice.

	// Copy occupied cells so we can test if each shape can fit in it.
	Faerie::FCellGrid CellsCopy = OccupiedCells;
	for (auto&& Shape : Shapes)
	{
		const FFaerieGridPlacement Location = FindFirstEmptyLocation(CellsCopy, Shape);
		if (Location.Origin != FIntPoint::NoneValue)
		{
			MarkShapeCells(CellsCopy, Shape);
		}
		else
		{
			return false;
		}
	}
	return true;
}

FFaerieGridShape UInventorySpatialGridExtension::GetItemShape(const FFaerieAddress Address) const
{
	return GetItemShape_Impl(Address).Copy();
}

FFaerieGridShape UInventorySpatialGridExtension::GetItemShapeOnGrid(const FFaerieAddress Address) const
{
	if (IsValid(InitializedContainer))
	{
		const UFaerieItem* Item = InitializedContainer->ViewItem(Address);
		const FFaerieGridPlacement Placement = GetStackPlacementData(Address);
		FFaerieGridShape Shape = GetItemShape_Impl(Item).Copy();
		Faerie::ApplyPlacementInline(Shape, Placement);
		return Shape;
	}

	return FFaerieGridShape();
}

FIntPoint UInventorySpatialGridExtension::GetStackBounds(const FFaerieAddress Address) const
{
	const FFaerieGridPlacement Placement = GetStackPlacementData(Address);
	return GetItemShape(Address).Rotate(Placement.Rotation).Normalize().GetSize();
}

bool UInventorySpatialGridExtension::CanAddAtLocation(const FFaerieGridShape& Shape, const FIntPoint Position) const
{
	return FitsInGridAnyRotation(Shape, Position, {});
}

bool UInventorySpatialGridExtension::CanAddAtLocation(const FFaerieGridShapeConstView& Shape, const FIntPoint Position) const
{
	return FitsInGridAnyRotation(Shape, Position, {});
}

Faerie::FExclusionSet UInventorySpatialGridExtension::MakeExclusionSet(const FFaerieAddress ExcludedAddress) const
{
	// Build list of excluded indices
	Faerie::FExclusionSet ExcludedPositions;
	ExcludedPositions.Reserve(4); // 4 is an average expected size of shapes. No better way to guess shape num.
	FFaerieGridShape OtherShape = GetItemShape(ExcludedAddress);
	Faerie::ApplyPlacementInline(OtherShape, GetStackPlacementData(ExcludedAddress), true);
	for (const auto& Point : OtherShape.Points)
	{
		ExcludedPositions.Add(Point);
	}
	return ExcludedPositions;
}

Faerie::FExclusionSet UInventorySpatialGridExtension::MakeExclusionSet(const TConstArrayView<FFaerieAddress> ExcludedAddresses) const
{
	// Build list of excluded indices
	Faerie::FExclusionSet ExcludedPositions;
	ExcludedPositions.Reserve(ExcludedAddresses.Num() * 4); // 4 is an average expected size of shapes. No better way to guess shape num.
	for (const FFaerieAddress& Key : ExcludedAddresses)
	{
		FFaerieGridShape OtherShape = GetItemShape(Key);
		Faerie::ApplyPlacementInline(OtherShape, GetStackPlacementData(Key));
		for (const auto& Point : OtherShape.Points)
		{
			ExcludedPositions.Add(Point);
		}
	}
	return ExcludedPositions;
}

bool UInventorySpatialGridExtension::FitsInGridAnyRotation(const FFaerieGridShapeConstView& Shape, const FIntPoint Origin, const Faerie::FExclusionSet& ExclusionSet) const
{
	FFaerieGridShape Translated = Shape.Copy().Translate(Origin);

	// Try 4 times if it FitsInGrid, rotating by 90 degrees between each test
	for (int32 i = 0; i < 4; ++i)
	{
		if (Faerie::FitsInGrid(OccupiedCells, Translated, ExclusionSet))
		{
			return true;
		}
		Translated.RotateInline(ESpatialItemRotation::Ninety);
	}
	return false;
}

FFaerieAddress UInventorySpatialGridExtension::FindOverlappingItem(const FFaerieGridShapeConstView& TranslatedShape,
																  const FFaerieAddress ExcludeAddress) const
{
	if (const FFaerieGridKeyedStack* Stack = GridContent.FindByPredicate(
		[this, &TranslatedShape, ExcludeAddress](const FFaerieGridKeyedStack& Other)
		{
			if (ExcludeAddress == Other.Key) { return false; }

			// Create a rotated and translated version of the other item's shape
			FFaerieGridShape OtherItemShape = GetItemShape(Other.Key);
			Faerie::ApplyPlacementInline(OtherItemShape, Other.Value);
			return TranslatedShape.Overlaps(OtherItemShape);
		}))
	{
		return Stack->Key;
	}
	return FFaerieAddress();
}

bool UInventorySpatialGridExtension::TrySwapItems(const FFaerieAddress AddressA, FFaerieGridPlacement& PlacementA,
												  const FFaerieAddress AddressB, FFaerieGridPlacement& PlacementB)
{
	const FFaerieGridShapeConstView ItemShapeA = GetItemShape_Impl(AddressA);
	const FFaerieGridShapeConstView ItemShapeB = GetItemShape_Impl(AddressB);

	// Get new placements for both items
	FFaerieGridPlacement PlacementANew = PlacementA;
	FFaerieGridPlacement PlacementBNew = PlacementB;
	PlacementANew.Origin = PlacementB.Origin;
	PlacementBNew.Origin = PlacementA.Origin;

	// Check if both items can exist in their new positions without overlapping each other
	const FFaerieGridShape ItemShapeANew = Faerie::ApplyPlacement(ItemShapeA, PlacementANew);
	const FFaerieGridShape ItemShapeBNew = Faerie::ApplyPlacement(ItemShapeB, PlacementBNew);
	if (ItemShapeANew.Overlaps(ItemShapeANew))
	{
		return false;
	}

	// Check if both items fit inside the grid
	const Faerie::FExclusionSet ExclusionSetA = MakeExclusionSet(AddressB);
	const Faerie::FExclusionSet ExclusionSetB = MakeExclusionSet(AddressA);
	if (!FitsInGrid(OccupiedCells, ItemShapeANew, ExclusionSetA) ||
		!FitsInGrid(OccupiedCells, ItemShapeBNew, ExclusionSetB))
	{
		return false;
	}

	const FFaerieGridShape ItemShapeAOld = Faerie::ApplyPlacement(ItemShapeA, PlacementA);
	const FFaerieGridShape ItemShapeBOld = Faerie::ApplyPlacement(ItemShapeB, PlacementB);

	// Remove Old Positions
	UnmarkShapeCells(OccupiedCells, ItemShapeAOld);
	UnmarkShapeCells(OccupiedCells, ItemShapeBOld);
	// Add To Swapped Positions
	MarkShapeCells(OccupiedCells, ItemShapeANew);
	MarkShapeCells(OccupiedCells, ItemShapeBNew);
	Swap(PlacementA.Origin, PlacementB.Origin);

	return true;
}

bool UInventorySpatialGridExtension::MoveSingleItem(const FFaerieAddress Address, FFaerieGridPlacement& Placement, const FIntPoint& NewPosition)
{
	FFaerieGridPlacement PlacementCopy = Placement;
	PlacementCopy.Origin = NewPosition;

	FFaerieGridShape ItemShape = GetItemShape(Address);
	const FFaerieGridShape NewShape = Faerie::ApplyPlacement(ItemShape, PlacementCopy);

	const Faerie::FExclusionSet ExclusionSet = MakeExclusionSet(Address);
	if (!FitsInGrid(OccupiedCells, NewShape, ExclusionSet))
	{
		return false;
	}

	Faerie::ApplyPlacementInline(ItemShape, Placement);

	UnmarkShapeCells(OccupiedCells, ItemShape);
	Placement.Origin = NewPosition;
	MarkShapeCells(OccupiedCells, NewShape);

	return true;
}