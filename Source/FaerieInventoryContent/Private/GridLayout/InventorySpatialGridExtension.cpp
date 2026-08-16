// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "GridLayout/InventorySpatialGridExtension.h"
#include "GridLayout/FaerieShapeFragment.h"

#include "FaerieInventoryContentLog.h"
#include "EntityManagerHelpers.h"

#include "FaerieItemContainerBase.h"
#include "FaerieItemStorage.h"
#include "ItemContainerEvent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventorySpatialGridExtension)

DECLARE_STATS_GROUP(TEXT("InventorySpatialGridExtension"), STATGROUP_FaerieSpatialGrid, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Client OccupiedCells rebuild"), STAT_Client_CellRebuild, STATGROUP_FaerieSpatialGrid);

namespace Faerie::Extensions
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
		const bool ShapeIsSymmetrical = Shape.IsSymmetrical();

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

				if (ShapeIsSymmetrical)
				{
					// Shape is symmetrical, no need to set TestPlacement.Rotation as they are all the same.
					const FFaerieGridShape Translated = ApplyPlacement(Shape, TestPlacement); // @todo this is *way* too many array allocations. optimize this!
					if (FitsInGrid(Grid, Translated, EmptyExclusionSet))
					{
						return TestPlacement;
					}
				}
				else
				{
					// @todo this is *way* too many array allocations. optimize this!
					FFaerieGridShape Translated = ApplyPlacement(Shape, TestPlacement);
					if (FitsInGrid(Grid, Translated, EmptyExclusionSet))
					{
						return TestPlacement;
					}

					TestPlacement.Rotation = EFaerieSpatialItemRotation::Ninety;
					Translated = ApplyPlacement(Shape, TestPlacement);
					if (FitsInGrid(Grid, Translated, EmptyExclusionSet))
					{
						return TestPlacement;
					}

					TestPlacement.Rotation = EFaerieSpatialItemRotation::One_Eighty;
					Translated = ApplyPlacement(Shape, TestPlacement);
					if (FitsInGrid(Grid, Translated, EmptyExclusionSet))
					{
						return TestPlacement;
					}

					TestPlacement.Rotation = EFaerieSpatialItemRotation::Two_Seventy;
					Translated = ApplyPlacement(Shape, TestPlacement);
					if (FitsInGrid(Grid, Translated, EmptyExclusionSet))
					{
						return TestPlacement;
					}
				}
			}
		}
		// No valid placement found
		return FFaerieGridPlacement{FIntPoint::NoneValue};
	}

	FFaerieGridShape ApplyPlacement(const FFaerieGridShapeConstView& Shape, const FFaerieGridPlacement& Placement, const bool bNormalize)
	{
		FFaerieGridShape ShapeCopy = Shape.Copy();
		ShapeCopy.Rotate(Placement.Rotation);
		if (bNormalize)
		{
			ShapeCopy.Normalize();
		}
		ShapeCopy.Translate(Placement.Origin);
		return ShapeCopy;
	}

	void ApplyPlacementInline(FFaerieGridShape& Shape, const FFaerieGridPlacement& Placement, const bool bNormalize)
	{
		Shape.Rotate(Placement.Rotation);
		if (bNormalize)
		{
			Shape.Normalize();
		}
		Shape.Translate(Placement.Origin);
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

using namespace Faerie;

EEventExtensionResponse UInventorySpatialGridExtension::AllowsAddition(const TNotNull<const UFaerieItemContainerBase*> Container,
																	   const Utils::TArrayAdapter<FFaerieItemProxy>& Proxies,
																	   const FFaerieExtensionAllowsAdditionArgs Args) const
{
	// @todo add boolean in config to allow items without a shape

	if (Proxies.Num() == 1)
	{
		if (!CanAddItemToGrid(GetItemShape_Impl(Proxies[0].GetItemInstance().GetValue())))
		{
			return EEventExtensionResponse::Disallowed;
		}
	}

	switch (Args.TestType)
	{
	case EFaerieStorageAddStackTestMultiType::IndividualTests:
		{
			for (int32 i = 0; i < Proxies.Num(); ++i)
			{
				const FFaerieItemProxy Proxy = Proxies[i];
				if (!CanAddItemToGrid(GetItemShape_Impl(Proxy.GetItemInstance().GetValue())))
				{
					return EEventExtensionResponse::Disallowed;
				}
			}

			return EEventExtensionResponse::Allowed;
		}

	case EFaerieStorageAddStackTestMultiType::GroupTest:
		{
			TArray<FFaerieGridShapeConstView> Shapes;
			for (int32 i = 0; i < Proxies.Num(); ++i)
			{
				const FFaerieItemProxy Proxy = Proxies[i];
				Shapes.Add(GetItemShape_Impl(Proxy.GetItemInstance().GetValue()));
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

EEventExtensionResponse UInventorySpatialGridExtension::AllowsEdit(const TNotNull<const UFaerieItemContainerBase*> Container,
																   const TNotNull<const Container::IAddressView*> DataView,
																   const FFaerieInventoryTag EditType) const
{
	if (EditType == Inventory::Tags::Split)
	{
		if (!CanAddItemToGrid(GetItemShape_Impl(DataView->GetItemInstance().GetValue())))
		{
			return EEventExtensionResponse::Disallowed;
		}
	}

	return EEventExtensionResponse::NoExplicitResponse;
}

void UInventorySpatialGridExtension::PostEventBatch(const TNotNull<const UFaerieItemContainerBase*> Container, const Inventory::FEventLogBatch& Events)
{
	if (Events.IsAdditionEvent())
	{
		for (auto&& Event : Events.Data)
		{
			// @todo don't add items for existing keys

            for (const FFaerieAddress Address : Event.AddressesTouched)
            {
                AddItemToGrid(Address, Event.Instance);
            }
		}
	}
	else if (Events.IsRemovalEvent())
	{
		for (auto&& Event : Events.Data)
		{
			if (const UFaerieItemStorage* ItemStorage = Cast<UFaerieItemStorage>(Container))
            {
                // Create a temporary array to store keys that need to be removed
                TArray<FFaerieAddress> AddressesToRemove;

                for (const FFaerieAddress Address : Event.AddressesTouched)
                {
                	if (ItemStorage->ContainsAddress(Address))
                	{
                		PostStackChange({ Address, GetStackPlacementData(Address) });
                	}
                	else
                	{
                		AddressesToRemove.Add(Address);
                	}
                }
                RemoveItemBatch(AddressesToRemove, Event.Instance);
            }
		}
	}
	else
	{
		check(Events.IsEditEvent())

		for (auto&& Event : Events.Data)
		{
			// Create a temporary array to store keys that need to be removed
			TArray<FFaerieAddress> AddressesToRemove;

			// get addresses to remove
            for (const FFaerieAddress Address : Event.AddressesTouched)
            {
                if (const UFaerieItemStorage* Storage = Cast<UFaerieItemStorage>(InitializedContainer);
                	!Storage->ContainsAddress(Address))
                {
                	AddressesToRemove.Add(Address);
                }
                else
                {
                	if (GridContent.Contains(Address))
                	{
                		BroadcastEvent(Address, EFaerieGridEventType::ItemChanged);
                	}
                	else
                	{
                		AddItemToGrid(Address, Event.Instance);
                	}
                }
            }

			// remove the stored keys
			RemoveItemBatch(AddressesToRemove, Event.Instance);
		}
	}
}

void UInventorySpatialGridExtension::PreStackRemove_Client(const FFaerieGridKeyedStack& Stack)
{
	RebuildOccupiedCells();

	BroadcastEvent(Stack.Key, EFaerieGridEventType::ItemRemoved);
}

void UInventorySpatialGridExtension::PreStackRemove_Server(const FFaerieGridKeyedStack& Stack, const TValid<const FFaerieItemInstance&> Item)
{
	// This is to account for removals through proxies that don't directly interface with the grid
	const FFaerieGridShape Translated = Extensions::ApplyPlacement(GetItemShape_Impl(Item), Stack.Value);
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
		Storage->ContainsAddress(Stack.Key))
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
		Extensions::ApplyPlacementInline(Shape, Element.Value);
		if (Shape.Contains(Position))
		{
			return Element.Key;
		}
	}

	return FFaerieAddress();
}

bool UInventorySpatialGridExtension::CanAddAtLocation(const TValid<const FFaerieItemProxy&> Proxy, const FIntPoint IntPoint) const
{
	const FFaerieGridShapeConstView Shape = GetItemShape_Impl(ValidGet(Proxy).GetItemInstanceOrInvalid());
	return CanAddAtLocation(Shape, IntPoint);
}

bool UInventorySpatialGridExtension::AddItemToGrid(const FFaerieAddress Address, const FFaerieItemInstance& Instance)
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

	FFaerieGridShape Shape = GetItemShape_Impl(Instance).Copy();

	const FFaerieGridPlacement DesiredItemPlacement = FindFirstEmptyLocation(OccupiedCells, Shape);

	if (DesiredItemPlacement.Origin == FIntPoint::NoneValue)
	{
		return false;
	}

	GridContent.Insert(Address, DesiredItemPlacement);

	Extensions::ApplyPlacementInline(Shape, DesiredItemPlacement);
	MarkShapeCells(OccupiedCells, Shape);

	return true;
}

bool UInventorySpatialGridExtension::MoveItem(const FFaerieAddress Address, const FIntPoint& TargetPoint)
{
	const FFaerieGridShapeConstView ItemShape = GetItemShape_Impl(Address);

	// Create placement at target point with current rotation
	const FFaerieGridPlacement NewPlacement(TargetPoint, GetStackPlacementData(Address).Rotation);

	// Get the rotated shape based on current stack rotation so we can correctly get items that would overlap
	const FFaerieGridShape NewShape = Extensions::ApplyPlacement(ItemShape, NewPlacement, true);

	// If this new position overlaps an existing item
	if (const FFaerieAddress OverlappingAddress = FindOverlappingItem(NewShape, Address);
		OverlappingAddress.IsValid())
	{
		const TTuple<FFaerieEntryKey, FFaerieStackKey> Key = UFaerieItemStorage::BreakAddress(Address);
		const TTuple<FFaerieEntryKey, FFaerieStackKey> OverlappingKey = UFaerieItemStorage::BreakAddress(OverlappingAddress);

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
		const Extensions::FExclusionSet ExclusionSet = MakeExclusionSet(Address);
		if (!FitsInGrid(OccupiedCells, NewShape, ExclusionSet))
		{
			return false;
		}

		const FFaerieGridContent::FScopedStackHandle StackHandle = GridContent.GetHandle(Address);

		const FFaerieGridShape OldShape = Extensions::ApplyPlacement(ItemShape, StackHandle.Get(), true);
		UnmarkShapeCells(OccupiedCells, OldShape);
		StackHandle->Origin = TargetPoint;
		MarkShapeCells(OccupiedCells, NewShape);
	}

	return true;
}

bool UInventorySpatialGridExtension::RotateItem(const FFaerieAddress Address, const EFaerieSpatialItemRotation RotationToAdd)
{
	const FFaerieGridShapeConstView ItemShape = GetItemShape_Impl(Address);

	const FFaerieGridContent::FScopedStackHandle Handle = GridContent.GetHandle(Address);

	if (ItemShape.IsSymmetrical())
	{
		// If the shape is symmetrical we can skip handling shape diffing.
		Handle->Rotation = Spatial::AddRotations(Handle->Rotation, RotationToAdd);
		return true;
	}

	// Store old points before transformations so we can clear them from the bit grid
	const FFaerieGridShape OldShape = Extensions::ApplyPlacement(ItemShape, Handle.Get(), true);

	FFaerieGridPlacement NewPlacement = *Handle;
	NewPlacement.Rotation = Spatial::AddRotations(NewPlacement.Rotation, RotationToAdd);
	const FFaerieGridShape NewShape = Extensions::ApplyPlacement(ItemShape, NewPlacement);

	const Extensions::FExclusionSet ExclusionSet = MakeExclusionSet(Address);
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

void UInventorySpatialGridExtension::RemoveItem(const FFaerieAddress Address, const TValid<const FFaerieItemInstance&> Item)
{
	GridContent.BSOA::Remove(Address,
		[Item, this](const FFaerieGridKeyedStack& Stack)
		{
			PreStackRemove_Server(Stack, Item);
		});
}

void UInventorySpatialGridExtension::RemoveItemBatch(const TConstArrayView<FFaerieAddress>& Addresses, const TValid<const FFaerieItemInstance&> Item)
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
		if (auto DataView = InitializedContainer->ViewAddress(SpatialEntry.Key);
			DataView.IsValid())
		{
			const FFaerieGridShapeConstView Shape = GetItemShape_Impl(DataView.Instance);
			const FFaerieGridShape Translated = Extensions::ApplyPlacement(Shape, SpatialEntry.Value);
			MarkShapeCells(OccupiedCells, Translated);
		}
	}
}

FFaerieGridShapeConstView UInventorySpatialGridExtension::GetItemShape_Impl(const TValid<const FFaerieItemInstance&> Item) const
{
	auto* EntityManager = ItemData::GetFaerieEntityManager();
	auto ShapeFragment = ItemData::GetEntityFragmentOrDefault<FFaerieShapeFragment>(EntityManager, Item);
	if (ShapeFragment.IsValid())
	{
		return ShapeFragment->Shape;
	}
	return FFaerieGridShape::Square1;
}

FFaerieGridShapeConstView UInventorySpatialGridExtension::GetItemShape_Impl(const FFaerieAddress Address) const
{
	if (IsValid(InitializedContainer))
	{
		if (const FFaerieItemInstance Instance = InitializedContainer->ViewInstance(Address);
			Instance.IsValid())
		{
			return GetItemShape_Impl(Instance);
		}
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
	Extensions::FCellGrid CellsCopy = OccupiedCells;
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
		if (const FFaerieItemInstance Instance = InitializedContainer->ViewInstance(Address);
			Instance.IsValid())
		{
			const FFaerieGridPlacement Placement = GetStackPlacementData(Address);
			FFaerieGridShape Shape = GetItemShape_Impl(Instance).Copy();
			Extensions::ApplyPlacementInline(Shape, Placement);
			return Shape;
		}
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
	return FitsInGridAnyRotation(Shape, Position, Extensions::EmptyExclusionSet);
}

bool UInventorySpatialGridExtension::CanAddAtLocation(const FFaerieGridShapeConstView& Shape, const FIntPoint Position) const
{
	return FitsInGridAnyRotation(Shape, Position, Extensions::EmptyExclusionSet);
}

Extensions::FExclusionSet UInventorySpatialGridExtension::MakeExclusionSet(const FFaerieAddress ExcludedAddress) const
{
	// Build list of excluded indices
	Extensions::FExclusionSet ExcludedPositions;
	ExcludedPositions.Reserve(4); // 4 is an average expected size of shapes. No better way to guess shape num.
	FFaerieGridShape OtherShape = GetItemShape(ExcludedAddress);
	Extensions::ApplyPlacementInline(OtherShape, GetStackPlacementData(ExcludedAddress), true);
	for (const auto& Point : OtherShape.Points)
	{
		ExcludedPositions.Add(Point);
	}
	return ExcludedPositions;
}

Extensions::FExclusionSet UInventorySpatialGridExtension::MakeExclusionSet(const TConstArrayView<FFaerieAddress> ExcludedAddresses) const
{
	// Build list of excluded indices
	Extensions::FExclusionSet ExcludedPositions;
	ExcludedPositions.Reserve(ExcludedAddresses.Num() * 4); // 4 is an average expected size of shapes. No better way to guess shape num.
	for (const FFaerieAddress& Key : ExcludedAddresses)
	{
		FFaerieGridShape OtherShape = GetItemShape(Key);
		Extensions::ApplyPlacementInline(OtherShape, GetStackPlacementData(Key));
		for (const auto& Point : OtherShape.Points)
		{
			ExcludedPositions.Add(Point);
		}
	}
	return ExcludedPositions;
}

bool UInventorySpatialGridExtension::FitsInGridAnyRotation(const FFaerieGridShapeConstView& Shape, const FIntPoint Origin, const Extensions::FExclusionSet& ExclusionSet) const
{
	FFaerieGridShape TestShape = Shape.Copy();

	// Try 4 times if it FitsInGrid, rotating by 90 degrees between each test
	for (int32 i = 0; i < 4; ++i)
	{
		TestShape.Translate(Origin); // Apply origin offset

		if (Faerie::Extensions::FitsInGrid(OccupiedCells, TestShape, ExclusionSet))
		{
			return true;
		}

		TestShape.Translate(Origin * -1); // Undo origin offset

		TestShape.RotateAroundCenter(); // Apply next rotation
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
			Extensions::ApplyPlacementInline(OtherItemShape, Other.Value);
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
	const FFaerieGridShape ItemShapeANew = Extensions::ApplyPlacement(ItemShapeA, PlacementANew);
	const FFaerieGridShape ItemShapeBNew = Extensions::ApplyPlacement(ItemShapeB, PlacementBNew);
	if (ItemShapeANew.Overlaps(ItemShapeANew))
	{
		return false;
	}

	// Check if both items fit inside the grid
	const Extensions::FExclusionSet ExclusionSetA = MakeExclusionSet(AddressB);
	const Extensions::FExclusionSet ExclusionSetB = MakeExclusionSet(AddressA);
	if (!FitsInGrid(OccupiedCells, ItemShapeANew, ExclusionSetA) ||
		!FitsInGrid(OccupiedCells, ItemShapeBNew, ExclusionSetB))
	{
		return false;
	}

	const FFaerieGridShape ItemShapeAOld = Extensions::ApplyPlacement(ItemShapeA, PlacementA);
	const FFaerieGridShape ItemShapeBOld = Extensions::ApplyPlacement(ItemShapeB, PlacementB);

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
	const FFaerieGridShape NewShape = Extensions::ApplyPlacement(ItemShape, PlacementCopy);

	const Extensions::FExclusionSet ExclusionSet = MakeExclusionSet(Address);
	if (!FitsInGrid(OccupiedCells, NewShape, ExclusionSet))
	{
		return false;
	}

	Extensions::ApplyPlacementInline(ItemShape, Placement);

	UnmarkShapeCells(OccupiedCells, ItemShape);
	Placement.Origin = NewPosition;
	MarkShapeCells(OccupiedCells, NewShape);

	return true;
}