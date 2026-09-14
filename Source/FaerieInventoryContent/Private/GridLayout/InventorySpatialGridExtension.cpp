// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "GridLayout/InventorySpatialGridExtension.h"
#include "GridLayout/FaerieShapeFragment.h"

#include "FaerieInventoryContentLog.h"
#include "EntityManagerHelpers.h"
#include "FaerieContainerEvent.h"

#include "FaerieItemContainerBase.h"
#include "FaerieItemStorage.h"
#include "FaerieItemStorageIterators.h"
#include "ItemContainerEvent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventorySpatialGridExtension)

DECLARE_STATS_GROUP(TEXT("InventorySpatialGridExtension"), STATGROUP_FaerieSpatialGrid, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Client OccupiedCells rebuild"), STAT_Client_CellRebuild, STATGROUP_FaerieSpatialGrid);

namespace Faerie::Extensions
{
	FFaerieGridPlacement FindFirstEmptyLocation(const FCellGrid& Grid, const FFaerieGridShapeConstView& Shape)
	{
		const FIntVector2 GridSize = Grid.GetDimensions();

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
		const FIntVector2 GridSize = Grid.GetDimensions();

		// Calculate shape bounds
		const FIntRect Bounds = TranslatedShape.GetBounds();

		// Early exit if shape is obviously too large
		if (Bounds.Max.X > GridSize.X || Bounds.Max.Y > GridSize.Y)
		{
			UE_LOGF(LogFaerieInventoryContent, Warning, "Item Too Big");
			return false;
		}

		// Check if all points in the shape fit within the grid and don't overlap with occupied cells
		for (const FIntPoint& Point : TranslatedShape.Points)
		{
			// Check if point is within grid bounds
			if (Point.X < 0 || Point.X >= GridSize.X ||
				Point.Y < 0 || Point.Y >= GridSize.Y)
			{
				UE_LOGF(LogFaerieInventoryContent, Warning, "Item Falls Outside Grid");
				return false;
			}

			// If this index is not in the excluded list, check if it's occupied
			if (!ExclusionSet.Contains(Point) && Grid.GetCell(Point))
			{
				UE_LOGF(LogFaerieInventoryContent, Warning, "Cell Is Occupied");
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

void UInventorySpatialGridExtension::InitializeGrid(const FFaerieContainerGridWriteContext& Context) const
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

EFaerieExtensionResponse UInventorySpatialGridExtension::AllowsAddition(const FFaerieContainerGridReadContext& Context,
																	   const Utils::TArrayAdapter<FFaerieItemProxy>& Proxies,
																	   const FFaerieExtensionAllowsAdditionArgs Args) const
{
	// @todo add boolean in config to allow items without a shape

	if (Proxies.Num() == 1)
	{
		if (!CanAddItemToGrid(Context, GetItemShape_Impl(Proxies[0].GetItemInstance().GetValue())))
		{
			return EFaerieExtensionResponse::Disallowed;
		}
	}

	TArray<FFaerieGridShapeConstView> Shapes;
	for (int32 i = 0; i < Proxies.Num(); ++i)
	{
		const FFaerieItemProxy& Proxy = Proxies[i];
		Shapes.Add(GetItemShape_Impl(Proxy.GetItemInstanceOrInvalid()));
	}

	if (!CanAddItemsToGrid(Context, Shapes))
	{
		return EFaerieExtensionResponse::Disallowed;
	}

	// Should not reach this;
	return EFaerieExtensionResponse::NoExplicitResponse;
}

EFaerieExtensionResponse UInventorySpatialGridExtension::AllowsEdit(const FFaerieContainerGridReadContext& Context,
																   const TNotNull<const Container::IAddressView*> DataView,
																   const FFaerieInventoryTag EditType) const
{
	if (EditType == Inventory::Tags::Split)
	{
		if (!CanAddItemToGrid(Context, GetItemShape_Impl(DataView->GetItemInstance().GetValue())))
		{
			return EFaerieExtensionResponse::Disallowed;
		}
	}

	return EFaerieExtensionResponse::NoExplicitResponse;
}

void UInventorySpatialGridExtension::HandleEvent(const FFaerieContainerGridWriteContext& Context, const Container::FEvent& Event) const
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

		// get addresses to remove
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
		RemoveItemBatch(Context, AddressesToRemove, Event.Instance);
	}
}

void UInventorySpatialGridExtension::PreStackRemove_Client(const FFaerieContainerGridWriteContext& Context, const FFaerieGridKeyedStack& Stack) const
{
	RebuildOccupiedCells(Context);

	BroadcastEvent(Stack.Key, EFaerieGridEventType::ItemRemoved);
}

void UInventorySpatialGridExtension::PreStackRemove_Server(const FFaerieContainerGridWriteContext& Context, const FFaerieGridKeyedStack& Stack, const FFaerieItemInstance& Item) const
{
	// This is to account for removals through proxies that don't directly interface with the grid
	const FFaerieGridShape Translated = Extensions::ApplyPlacement(GetItemShape_Impl(Item), Stack.Value);
	Extensions::UnmarkShapeCells(Context.Unwrap().OccupiedCells, Translated);

	BroadcastEvent(Stack.Key, EFaerieGridEventType::ItemRemoved);
}

void UInventorySpatialGridExtension::PostStackAdd(const FFaerieContainerGridWriteContext& Context, const FFaerieGridKeyedStack& Stack) const
{
	BroadcastEvent(Stack.Key, EFaerieGridEventType::ItemAdded);
}

void UInventorySpatialGridExtension::PostStackChange(const FFaerieContainerGridWriteContext& Context, const FFaerieGridKeyedStack& Stack) const
{
	if (Context.GetStorage()->ContainsAddress(Stack.Key))
	{
		BroadcastEvent(Stack.Key, EFaerieGridEventType::ItemChanged);
	}
}

TOptional<FFaerieAddress> UInventorySpatialGridExtension::GetKeyAt(const FFaerieContainerGridReadContext& Context, const FIntPoint& Position) const
{
	for (auto&& Element : Context.GetGrid())
	{
		// Easy check first
		if (Element.Value.Origin == Position) return Element.Key;

		FFaerieGridShape Shape = GetItemShape_Impl(Context.GetStorage(), Element.Key).Copy();
		Extensions::ApplyPlacementInline(Shape, Element.Value);
		if (Shape.Contains(Position))
		{
			return Element.Key;
		}
	}

	return NullOpt;
}

bool UInventorySpatialGridExtension::CanAddAtLocation(const FFaerieContainerGridReadContext& Context, const TValid<const FFaerieItemProxy&> Proxy, const FIntPoint IntPoint) const
{
	const FFaerieGridShapeConstView Shape = GetItemShape_Impl(ValidGet(Proxy).GetItemInstanceOrInvalid());
	return CanAddAtLocation(Context, Shape, IntPoint);
}

bool UInventorySpatialGridExtension::AddItemToGrid(const FFaerieContainerGridWriteContext& Context, const FFaerieAddress Address, const FFaerieItemInstance& Instance) const
{
	if (!Address.IsValid())
	{
		return false;
	}

	if (Context.GetGrid().Contains(Address))
	{
		// Already in the grid...
		return true;
	}

	FFaerieGridShape Shape = GetItemShape_Impl(Instance).Copy();

	const FFaerieGridPlacement DesiredItemPlacement = Extensions::FindFirstEmptyLocation(Context.GetOccupiedCells(), Shape);

	if (DesiredItemPlacement.Origin == FIntPoint::NoneValue)
	{
		return false;
	}

	Context.Unwrap().GridContent.Insert(Address, DesiredItemPlacement);

	Extensions::ApplyPlacementInline(Shape, DesiredItemPlacement);
	Extensions::MarkShapeCells(Context.Unwrap().OccupiedCells, Shape);

	return true;
}

bool UInventorySpatialGridExtension::MoveItem(const FFaerieContainerGridWriteContext& Context, const FFaerieAddress Address, const FIntPoint& TargetPoint) const
{
	const FFaerieGridShapeConstView ItemShape = GetItemShape_Impl(Context.GetStorage(), Address);

	// Create placement at target point with current rotation
	const FFaerieGridPlacement NewPlacement(TargetPoint, Context.GetStackPlacementData(Address).Rotation);

	// Get the rotated shape based on current stack rotation so we can correctly get items that would overlap
	const FFaerieGridShape NewShape = Extensions::ApplyPlacement(ItemShape, NewPlacement, true);

	// If this new position overlaps an existing item
	if (const FFaerieAddress OverlappingAddress = FindOverlappingItem(Context, NewShape, Address);
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
			if (Context.GetStorage()->MergeStacks(Key.Get<0>(), Key.Get<1>(), OverlappingKey.Get<1>()))
			{
				return true;
			}
		}

		const FFaerieGridContent::FScopedStackHandle StackHandleA = Context.Unwrap().GridContent.GetHandle(Address);
		const FFaerieGridContent::FScopedStackHandle StackHandleB = Context.Unwrap().GridContent.GetHandle(OverlappingAddress);

		return TrySwapItems(Context,
			Address, StackHandleA.Get(),
			OverlappingAddress, StackHandleB.Get());
	}

	// Copied logic from MoveSingleItem, but optimized to use existing variables.
	{
		const Extensions::FExclusionSet ExclusionSet = MakeExclusionSet(Context, Address);
		if (!Extensions::FitsInGrid(Context.GetOccupiedCells(), NewShape, ExclusionSet))
		{
			return false;
		}

		const FFaerieGridContent::FScopedStackHandle StackHandle = Context.Unwrap().GridContent.GetHandle(Address);

		const FFaerieGridShape OldShape = Extensions::ApplyPlacement(ItemShape, StackHandle.Get(), true);
		Extensions::UnmarkShapeCells(Context.Unwrap().OccupiedCells, OldShape);
		StackHandle->Origin = TargetPoint;
		Extensions::MarkShapeCells(Context.Unwrap().OccupiedCells, NewShape);
	}

	return true;
}

bool UInventorySpatialGridExtension::RotateItem(const FFaerieContainerGridWriteContext& Context, const FFaerieAddress Address, const EFaerieSpatialItemRotation RotationToAdd) const
{
	const FFaerieGridShapeConstView ItemShape = GetItemShape_Impl(Context.GetStorage(), Address);

	const FFaerieGridContent::FScopedStackHandle Handle = Context.Unwrap().GridContent.GetHandle(Address);

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

	const Extensions::FExclusionSet ExclusionSet = MakeExclusionSet(Context, Address);
	if (!Extensions::FitsInGrid(Context.GetOccupiedCells(), NewShape, ExclusionSet))
	{
		return false;
	}

	const FIntRect OldBounds = OldShape.GetBounds();
	const FIntRect NewBounds = NewShape.GetBounds();

	// Clear old occupied cells
	Extensions::UnmarkShapeCells(Context.Unwrap().OccupiedCells, OldShape);

	Handle->Rotation = NewPlacement.Rotation;
	if (OldBounds != NewBounds)
	{
		Handle->Origin = NewBounds.Min;
	}
	// Set new occupied cells taking into account rotation
	Extensions::MarkShapeCells(Context.Unwrap().OccupiedCells, NewShape);

	return true;
}

void UInventorySpatialGridExtension::RemoveItem(const FFaerieContainerGridWriteContext& Context, const FFaerieAddress Address, const FFaerieItemInstance& Item) const
{
	Context.Unwrap().GridContent.BSOA::Remove(Address,
		[Item, Context, this](const FFaerieGridKeyedStack& Stack)
		{
			PreStackRemove_Server(Context, Stack, Item);
		});
}

void UInventorySpatialGridExtension::RemoveItemBatch(const FFaerieContainerGridWriteContext& Context, const TConstArrayView<FFaerieAddress>& Addresses, const FFaerieItemInstance& Item) const
{
	for (const FFaerieAddress AddressToRemove : Addresses)
	{
		RemoveItem(Context, AddressToRemove, Item);
		BroadcastEvent(AddressToRemove, EFaerieGridEventType::ItemRemoved);
	}
	Context.Unwrap().GridContent.MarkArrayDirty();
}

void UInventorySpatialGridExtension::RebuildOccupiedCells(const FFaerieContainerGridWriteContext& Context) const
{
	SCOPE_CYCLE_COUNTER(STAT_Client_CellRebuild);

	Context.Unwrap().OccupiedCells.Reset(Context.GetGridSize());

	for (const auto& SpatialEntry : Context.GetGrid())
	{
		if (auto DataView = Context.GetStorage()->ViewAddress(SpatialEntry.Key);
			DataView.IsValid())
		{
			const FFaerieGridShapeConstView Shape = GetItemShape_Impl(DataView.Instance);
			const FFaerieGridShape Translated = Extensions::ApplyPlacement(Shape, SpatialEntry.Value);
			Extensions::MarkShapeCells(Context.Unwrap().OccupiedCells, Translated);
		}
	}
}

FFaerieGridShapeConstView UInventorySpatialGridExtension::GetItemShape_Impl(const FFaerieItemInstance& Item) const
{
	auto* EntityManager = ItemData::GetFaerieEntityManager();
	auto ShapeFragment = ItemData::GetEntityFragmentOrDefault<FFaerieShapeFragment>(EntityManager, Item);
	if (ShapeFragment.IsValid())
	{
		return ShapeFragment->Shape;
	}
	return FFaerieGridShape::Square1;
}

FFaerieGridShapeConstView UInventorySpatialGridExtension::GetItemShape_Impl(const TNotNull<const UFaerieItemStorage*> Storage, const FFaerieAddress Address) const
{
	if (const TOptional<FFaerieItemInstance> Instance = Storage->ViewInstance(Address);
		Instance.IsSet())
	{
		return GetItemShape_Impl(Instance.GetValue());
	}

	return FFaerieGridShapeConstView();
}

bool UInventorySpatialGridExtension::CanAddItemToGrid(const FFaerieContainerGridReadContext& Context, const FFaerieGridShapeConstView& Shape) const
{
	const FFaerieGridPlacement TestPlacement = Extensions::FindFirstEmptyLocation(Context.GetOccupiedCells(), Shape);
	return TestPlacement.Origin != FIntPoint::NoneValue;
}

bool UInventorySpatialGridExtension::CanAddItemsToGrid(const FFaerieContainerGridReadContext& Context, const TConstArrayView<FFaerieGridShapeConstView>& Shapes) const
{
	// @todo obviously this is not very ideal. It just throws each item into the grid first place it goes. A proper shape-packing algo would be nice.

	// Copy occupied cells so we can test if each shape can fit in it.
	Extensions::FCellGrid CellsCopy = Context.GetOccupiedCells();
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

FFaerieGridShape UInventorySpatialGridExtension::GetItemShape(const UFaerieItemStorage* Storage, const FFaerieAddress Address) const
{
	return GetItemShape_Impl(Storage, Address).Copy();
}

FFaerieGridShape UInventorySpatialGridExtension::GetItemShapeOnGrid(const FFaerieContainerGridReadContext& Context, const FFaerieAddress Address) const
{
	if (const TOptional<FFaerieItemInstance> Instance = Context.GetStorage()->ViewInstance(Address);
		Instance.IsSet())
	{
		const FFaerieGridPlacement Placement = Context.GetStackPlacementData(Address);
		FFaerieGridShape Shape = GetItemShape_Impl(Instance.GetValue()).Copy();
		Extensions::ApplyPlacementInline(Shape, Placement);
		return Shape;
	}

	return FFaerieGridShape();
}

FIntPoint UInventorySpatialGridExtension::GetStackBounds(const FFaerieContainerGridReadContext& Context, const FFaerieAddress Address) const
{
	const FFaerieGridPlacement Placement = Context.GetStackPlacementData(Address);
	return GetItemShape(Context.GetStorage(), Address).Rotate(Placement.Rotation).Normalize().GetSize();
}

bool UInventorySpatialGridExtension::CanAddAtLocation(const FFaerieContainerGridReadContext& Context, const FFaerieGridShape& Shape, const FIntPoint Position) const
{
	return FitsInGridAnyRotation(Context, Shape, Position, Extensions::EmptyExclusionSet);
}

bool UInventorySpatialGridExtension::CanAddAtLocation(const FFaerieContainerGridReadContext& Context, const FFaerieGridShapeConstView& Shape, const FIntPoint Position) const
{
	return FitsInGridAnyRotation(Context, Shape, Position, Extensions::EmptyExclusionSet);
}

Extensions::FExclusionSet UInventorySpatialGridExtension::MakeExclusionSet(const FFaerieContainerGridReadContext& Context, const FFaerieAddress ExcludedAddress) const
{
	// Build list of excluded indices
	Extensions::FExclusionSet ExcludedPositions;
	ExcludedPositions.Reserve(4); // 4 is an average expected size of shapes. No better way to guess shape num.
	FFaerieGridShape OtherShape = GetItemShape(Context.GetStorage(), ExcludedAddress);
	Extensions::ApplyPlacementInline(OtherShape, Context.GetStackPlacementData(ExcludedAddress), true);
	for (const auto& Point : OtherShape.Points)
	{
		ExcludedPositions.Add(Point);
	}
	return ExcludedPositions;
}

Extensions::FExclusionSet UInventorySpatialGridExtension::MakeExclusionSet(const FFaerieContainerGridReadContext& Context, const TConstArrayView<FFaerieAddress> ExcludedAddresses) const
{
	// Build list of excluded indices
	Extensions::FExclusionSet ExcludedPositions;
	ExcludedPositions.Reserve(ExcludedAddresses.Num() * 4); // 4 is an average expected size of shapes. No better way to guess shape num.
	for (const FFaerieAddress& Key : ExcludedAddresses)
	{
		FFaerieGridShape OtherShape = GetItemShape(Context.GetStorage(), Key);
		Extensions::ApplyPlacementInline(OtherShape, Context.GetStackPlacementData(Key));
		for (const auto& Point : OtherShape.Points)
		{
			ExcludedPositions.Add(Point);
		}
	}
	return ExcludedPositions;
}

bool UInventorySpatialGridExtension::FitsInGridAnyRotation(const FFaerieContainerGridReadContext& Context, const FFaerieGridShapeConstView& Shape, const FIntPoint Origin, const Extensions::FExclusionSet& ExclusionSet) const
{
	FFaerieGridShape TestShape = Shape.Copy();

	// Try 4 times if it FitsInGrid, rotating by 90 degrees between each test
	for (int32 i = 0; i < 4; ++i)
	{
		TestShape.Translate(Origin); // Apply origin offset

		if (Extensions::FitsInGrid(Context.GetOccupiedCells(), TestShape, ExclusionSet))
		{
			return true;
		}

		TestShape.Translate(Origin * -1); // Undo origin offset

		TestShape.RotateAroundCenter(); // Apply next rotation
	}
	return false;
}

FFaerieAddress UInventorySpatialGridExtension::FindOverlappingItem(const FFaerieContainerGridReadContext& Context,
	const FFaerieGridShapeConstView& TranslatedShape, const FFaerieAddress ExcludeAddress) const
{
	if (const FFaerieGridKeyedStack* Stack = Context.GetGrid().FindByPredicate(
		[this, &TranslatedShape, ExcludeAddress, Context](const FFaerieGridKeyedStack& Other)
		{
			if (ExcludeAddress == Other.Key) { return false; }

			// Create a rotated and translated version of the other item's shape
			FFaerieGridShape OtherItemShape = GetItemShape(Context.GetStorage(), Other.Key);
			Extensions::ApplyPlacementInline(OtherItemShape, Other.Value);
			return TranslatedShape.Overlaps(OtherItemShape);
		}))
	{
		return Stack->Key;
	}
	return FFaerieAddress();
}

bool UInventorySpatialGridExtension::TrySwapItems(const FFaerieContainerGridWriteContext& Context,
	const FFaerieAddress AddressA, FFaerieGridPlacement& PlacementA,
	const FFaerieAddress AddressB, FFaerieGridPlacement& PlacementB) const
{
	const FFaerieGridShapeConstView ItemShapeA = GetItemShape_Impl(Context.GetStorage(), AddressA);
	const FFaerieGridShapeConstView ItemShapeB = GetItemShape_Impl(Context.GetStorage(), AddressB);

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
	const Extensions::FExclusionSet ExclusionSetA = MakeExclusionSet(Context, AddressB);
	const Extensions::FExclusionSet ExclusionSetB = MakeExclusionSet(Context, AddressA);
	if (!Extensions::FitsInGrid(Context.Unwrap().OccupiedCells, ItemShapeANew, ExclusionSetA) ||
		!Extensions::FitsInGrid(Context.Unwrap().OccupiedCells, ItemShapeBNew, ExclusionSetB))
	{
		return false;
	}

	const FFaerieGridShape ItemShapeAOld = Extensions::ApplyPlacement(ItemShapeA, PlacementA);
	const FFaerieGridShape ItemShapeBOld = Extensions::ApplyPlacement(ItemShapeB, PlacementB);

	// Remove Old Positions
	Extensions::UnmarkShapeCells(Context.Unwrap().OccupiedCells, ItemShapeAOld);
	Extensions::UnmarkShapeCells(Context.Unwrap().OccupiedCells, ItemShapeBOld);
	// Add To Swapped Positions
	Extensions::MarkShapeCells(Context.Unwrap().OccupiedCells, ItemShapeANew);
	Extensions::MarkShapeCells(Context.Unwrap().OccupiedCells, ItemShapeBNew);
	Swap(PlacementA.Origin, PlacementB.Origin);

	return true;
}

bool UInventorySpatialGridExtension::MoveSingleItem(const FFaerieContainerGridWriteContext& Context,
	const FFaerieAddress Address, FFaerieGridPlacement& Placement, const FIntPoint& NewPosition) const
{
	FFaerieGridPlacement PlacementCopy = Placement;
	PlacementCopy.Origin = NewPosition;

	FFaerieGridShape ItemShape = GetItemShape(Context.GetStorage(), Address);
	const FFaerieGridShape NewShape = Extensions::ApplyPlacement(ItemShape, PlacementCopy);

	const Extensions::FExclusionSet ExclusionSet = MakeExclusionSet(Context, Address);
	if (!Extensions::FitsInGrid(Context.Unwrap().OccupiedCells, NewShape, ExclusionSet))
	{
		return false;
	}

	Extensions::ApplyPlacementInline(ItemShape, Placement);

	Extensions::UnmarkShapeCells(Context.Unwrap().OccupiedCells, ItemShape);
	Placement.Origin = NewPosition;
	Extensions::MarkShapeCells(Context.Unwrap().OccupiedCells, NewShape);

	return true;
}
