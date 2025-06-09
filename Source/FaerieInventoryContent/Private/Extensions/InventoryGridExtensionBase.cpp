// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Extensions/InventoryGridExtensionBase.h"
#include "FaerieItemContainerBase.h"
#include "FaerieItemStorage.h"
#include "Net/UnrealNetwork.h"
#include "StructUtils/StructView.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryGridExtensionBase)

namespace Faerie
{
	bool FCellGrid::GetCell(const FIntPoint Point) const
	{
		const int32 Index = Ravel(Point);
		if (!CellBits.IsValidIndex(Index))
		{
			// If cell doesn't exist, it cannot be occupied
			return false;
		}
		return CellBits[Index];
	}

	FIntPoint FCellGrid::GetDimensions() const
	{
		return Dimensions;
	}

	void FCellGrid::Reset(const FIntPoint Size)
	{
		Dimensions = Size;
		CellBits.Init(false, Size.X * Size.Y);
	}

	void FCellGrid::Resize(const FIntPoint NewSize)
	{
		const FIntPoint OldSize = Dimensions;
		TBitArray<> OldBits = CellBits;

        Reset(NewSize);

		// Copy over existing data that's still in bounds
		for (int32 y = 0; y < FMath::Min(OldSize.Y, NewSize.Y); y++)
		{
			for (int32 x = 0; x < FMath::Min(OldSize.X, NewSize.X); x++)
			{
				const int32 OldIndex = x + y * OldSize.X;
				const int32 NewIndex = x + y * NewSize.X;
				CellBits[NewIndex] = OldBits[OldIndex];
			}
		}
	}

	void FCellGrid::MarkCell(const FIntPoint& Point)
	{
		const int32 Index = Ravel(Point);
		if (!CellBits.IsValidIndex(Index))
		{
			// If cell doesn't exist, expand to fit.
			CellBits.SetNum(Index, false);
		}
		CellBits[Index] = true;
	}

	void FCellGrid::UnmarkCell(const FIntPoint& Point)
	{
		const int32 Index = Ravel(Point);
		if (!CellBits.IsValidIndex(Index))
		{
			// If cell doesn't exist, no need to unmark it.
			return;
		}
		CellBits[Index] = false;
	}

	bool FCellGrid::IsEmpty() const
	{
		return !CellBits.Contains(true);
	}

	bool FCellGrid::IsFull() const
	{
		return !CellBits.Contains(false);
	}

	int32 FCellGrid::GetNumCells() const
	{
		return Dimensions.X * Dimensions.Y;
	}

	int32 FCellGrid::GetNumMarked() const
	{
		return CellBits.CountSetBits();
	}

	int32 FCellGrid::GetNumUnmarked() const
	{
		return GetNumCells() - GetNumMarked();
	}

	int32 FCellGrid::Ravel(const FIntPoint& Point) const
	{
		return Point.Y * Dimensions.X + Point.X;
	}

	FIntPoint FCellGrid::Unravel(const int32 Index) const
	{
		const int32 X = Index % Dimensions.X;
		const int32 Y = Index / Dimensions.X;
		return FIntPoint{ X, Y };
	}
}

void UInventoryGridExtensionBase::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, GridContent, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, GridSize, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, InitializedContainer, SharedParams);
}

void UInventoryGridExtensionBase::PostInitProperties()
{
	Super::PostInitProperties();
	GridContent.ChangeListener = this;
}

void UInventoryGridExtensionBase::InitializeExtension(const UFaerieItemContainerBase* Container)
{
	checkf(!IsValid(InitializedContainer), TEXT("UInventoryGridExtensionBase doesn't support multi-initialization!"))
	InitializedContainer = const_cast<UFaerieItemContainerBase*>(Container);
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, InitializedContainer, this);

	// Add all existing items to the grid on startup.
	// This is dumb, and just adds them in order, it doesn't space pack them. To do that, we would want to sort items by size, and add largest first.
	// This is also skipping possible serialization of grid data.
	// @todo handle serialization loading
	// @todo handle items that are too large to fit / too many items (log error?)
	OccupiedCells.Reset(GridSize);
	if (const UFaerieItemStorage* ItemStorage = Cast<UFaerieItemStorage>(Container))
	{
		ItemStorage->ForEachKey(
			[this, ItemStorage](const FEntryKey Key)
			{
				PRAGMA_DISABLE_DEPRECATION_WARNINGS
				for (const auto EntryView = ItemStorage->GetEntryView(Key);
					auto&& Entry : EntryView.Get().Stacks)
				{
					if (const FInventoryKey InvKey(Key, Entry.Key);
						!AddItemToGrid(InvKey, EntryView.Get().ItemObject))
					{
						// Cannot add this item, skip the rest of stacks, and continue to next key
						break;
					}
				}
				PRAGMA_ENABLE_DEPRECATION_WARNINGS
			});
	}
}

void UInventoryGridExtensionBase::DeinitializeExtension(const UFaerieItemContainerBase* Container)
{
	// Remove all entries for this container on shutdown
	// @todo its only okay to reset these because we don't suppose multi-container! revisit later
	OccupiedCells.Reset(0);
	GridContent.Items.Reset();
	InitializedContainer = nullptr;
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, InitializedContainer, this);
}

bool UInventoryGridExtensionBase::IsCellOccupied(const FIntPoint& Point) const
{
	return OccupiedCells.GetCell(Point);
}

void UInventoryGridExtensionBase::BroadcastEvent(const FInventoryKey& Key, const EFaerieGridEventType EventType)
{
	SpatialStackChangedNative.Broadcast(Key, EventType);
	SpatialStackChangedDelegate.Broadcast(Key, EventType);
}

void UInventoryGridExtensionBase::OnRep_GridSize()
{
	GridSizeChangedNative.Broadcast(GridSize);
	GridSizeChangedDelegate.Broadcast(GridSize);
}

FFaerieItemStackView UInventoryGridExtensionBase::ViewAt(const FIntPoint& Position) const
{
	if (const FInventoryKey Key = GetKeyAt(Position);
		Key.IsValid())
	{
		return Cast<UFaerieItemStorage>(InitializedContainer)->GetStackView(Key);
	}
	return FFaerieItemStackView();
}

FFaerieGridPlacement UInventoryGridExtensionBase::GetStackPlacementData(const FInventoryKey& Key) const
{
	if (auto&& Placement = GridContent.Find(Key))
	{
		return *Placement;
	}

	return FFaerieGridPlacement();
}

void UInventoryGridExtensionBase::SetGridSize(const FIntPoint& NewGridSize)
{
	if (GridSize != NewGridSize)
	{
		// Resize to new dimensions
		GridSize = NewGridSize;
		OccupiedCells.Resize(GridSize);

		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, GridSize, this);

		// OnReps must be called manually on the server in c++
		OnRep_GridSize();
	}
}