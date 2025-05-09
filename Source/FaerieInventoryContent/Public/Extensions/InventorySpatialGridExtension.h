// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieGridStructs.h"
#include "InventoryGridExtensionBase.h"
#include "SpatialTypes.h"
#include "InventorySpatialGridExtension.generated.h"

namespace Faerie
{
	using FExclusionSet = TSet<FIntPoint>;

	// General shape utils
	[[nodiscard]] FFaerieGridShape ApplyPlacement(const FFaerieGridShapeConstView& Shape, const FFaerieGridPlacement& Placement, bool bNormalize = false, bool Reset = false);
	void ApplyPlacementInline(FFaerieGridShape& Shape, const FFaerieGridPlacement& Placement, bool bNormalize = false);

	// Cell grid utils for shapes.
	FFaerieGridPlacement FindFirstEmptyLocation(const FCellGrid& Grid, const FFaerieGridShapeConstView& Shape);
	bool FitsInGrid(const FCellGrid& Grid, const FFaerieGridShapeConstView& TranslatedShape, const FExclusionSet& ExclusionSet);
	void MarkShapeCells(FCellGrid& Grid, const FFaerieGridShapeConstView TranslatedShape);
	void UnmarkShapeCells(FCellGrid& Grid, const FFaerieGridShapeConstView& TranslatedShape);
}

/**
 *
 */
UCLASS()
class FAERIEINVENTORYCONTENT_API UInventorySpatialGridExtension : public UInventoryGridExtensionBase
{
	GENERATED_BODY()

protected:
	//~ UItemContainerExtensionBase
	virtual EEventExtensionResponse AllowsAddition(const UFaerieItemContainerBase* Container, TConstArrayView<FFaerieItemStackView> Views, FFaerieExtensionAllowsAdditionArgs Args) const override;
	virtual void PostAddition(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event) override;
	virtual void PostRemoval(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event) override;
	virtual EEventExtensionResponse AllowsEdit(const UFaerieItemContainerBase* Container, FEntryKey Key, FFaerieInventoryTag EditType) const override;
	virtual void PostEntryChanged(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event) override;
	//~ UItemContainerExtensionBase

	//~ UInventoryGridExtensionBase
	virtual void PreStackRemove_Client(const FFaerieGridKeyedStack& Stack) override;
	virtual void PreStackRemove_Server(const FFaerieGridKeyedStack& Stack, const UFaerieItem* Item) override;

	virtual void PostStackAdd(const FFaerieGridKeyedStack& Stack) override;
	virtual void PostStackChange(const FFaerieGridKeyedStack& Stack) override;

	virtual FInventoryKey GetKeyAt(const FIntPoint& Position) const override;
	virtual bool CanAddAtLocation(FFaerieItemStackView Stack, FIntPoint IntPoint) const override;
	virtual bool AddItemToGrid(const FInventoryKey& Key, const UFaerieItem* Item) override;
	virtual bool MoveItem(const FInventoryKey& Key, const FIntPoint& TargetPoint) override;
	virtual bool RotateItem(const FInventoryKey& Key) override;
	//~ UInventoryGridExtensionBase

private:
	void RemoveItem(const FInventoryKey& Key, const UFaerieItem* Item);
	void RemoveItemBatch(const TConstArrayView<FInventoryKey>& Keys, const UFaerieItem* Item);

	// The client has to manually rebuild its cell after a removal, as the item's shape is likely lost.
	void RebuildOccupiedCells();

	// Gets a shape from a shape token on the item, or returns a single cell at 0,0 for items with no token.
	FFaerieGridShape GetItemShape_Impl(const UFaerieItem* Item) const;

public:
	bool CanAddItemToGrid(const FFaerieGridShapeConstView& Shape) const;
	bool CanAddItemsToGrid(const TArray<FFaerieGridShapeConstView>& Shapes) const;

	// Gets the normalized shape for an item.
	UFUNCTION(BlueprintCallable, Category = "Faerie|SpatialGrid")
	FFaerieGridShape GetItemShape(FEntryKey Key) const;

	// Gets the shape of an item transposed on the grid according to its placement.
	UFUNCTION(BlueprintCallable, Category = "Faerie|SpatialGrid")
	FFaerieGridShape GetItemShapeOnGrid(const FInventoryKey& Key) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|SpatialGrid")
	FIntPoint GetStackBounds(const FInventoryKey& Key) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|SpatialGrid")
	bool CanAddAtLocation(const FFaerieGridShape& Shape, FIntPoint Position) const;

protected:
	Faerie::FExclusionSet MakeExclusionSet(FInventoryKey ExcludedKey) const;
	Faerie::FExclusionSet MakeExclusionSet(const TConstArrayView<FInventoryKey> ExcludedKeys) const;

	bool FitsInGridAnyRotation(const FFaerieGridShapeConstView& Shape, FIntPoint Origin, const Faerie::FExclusionSet& ExclusionSet) const;

	FInventoryKey FindOverlappingItem(const FFaerieGridShapeConstView& TranslatedShape, const FInventoryKey& ExcludeKey) const;

	bool TrySwapItems(FInventoryKey KeyA, FFaerieGridPlacement& PlacementA, FInventoryKey KeyB, FFaerieGridPlacement& PlacementB);

	bool MoveSingleItem(const FInventoryKey Key, FFaerieGridPlacement& Placement, const FIntPoint& NewPosition);
};