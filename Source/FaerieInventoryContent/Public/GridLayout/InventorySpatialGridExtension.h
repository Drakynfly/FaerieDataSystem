// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieGridStructs.h"
#include "InventoryGridExtensionBase.h"
#include "SpatialTypes.h"
#include "InventorySpatialGridExtension.generated.h"

namespace Faerie::Extensions
{
	using FExclusionSet = TSet<FIntPoint>;
	static const inline FExclusionSet EmptyExclusionSet{};

	// General shape utils
	[[nodiscard]] FFaerieGridShape ApplyPlacement(const FFaerieGridShapeConstView& Shape, const FFaerieGridPlacement& Placement, bool bNormalize = false);
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

public:
	//~ UInventoryGridExtensionBase
	virtual void InitializeGrid(const FFaerieContainerGridWriteContext& Context) const override;
	virtual EFaerieExtensionResponse AllowsAddition(const FFaerieContainerGridReadContext& Context, const Faerie::Utils::TArrayAdapter<FFaerieItemProxy>& Proxies, FFaerieExtensionAllowsAdditionArgs Args) const override;
	virtual EFaerieExtensionResponse AllowsEdit(const FFaerieContainerGridReadContext& Context, const TNotNull<const Faerie::Container::IAddressView*> DataView, FFaerieInventoryTag EditType) const override;
	virtual void HandleEvent(const FFaerieContainerGridWriteContext& Context, const Faerie::Container::FEvent& Event) const override;

	virtual void PreStackRemove_Client(const FFaerieContainerGridWriteContext& Context, const FFaerieGridKeyedStack& Stack) const override;
	virtual void PreStackRemove_Server(const FFaerieContainerGridWriteContext& Context, const FFaerieGridKeyedStack& Stack, const FFaerieItemInstance& Item) const override;

	virtual void PostStackAdd(const FFaerieContainerGridWriteContext& Context, const FFaerieGridKeyedStack& Stack) const override;
	virtual void PostStackChange(const FFaerieContainerGridWriteContext& Context, const FFaerieGridKeyedStack& Stack) const override;

	virtual TOptional<FFaerieAddress> GetKeyAt(const FFaerieContainerGridReadContext& Context, const FIntPoint& Position) const override;
	virtual bool CanAddAtLocation(const FFaerieContainerGridReadContext& Context, Faerie::TValid<const FFaerieItemProxy&> Proxy, FIntPoint IntPoint) const override;
	virtual bool AddItemToGrid(const FFaerieContainerGridWriteContext& Context, FFaerieAddress Address, const FFaerieItemInstance& Instance) const override;
	virtual bool MoveItem(const FFaerieContainerGridWriteContext& Context, FFaerieAddress Address, const FIntPoint& TargetPoint) const override;
	virtual bool RotateItem(const FFaerieContainerGridWriteContext& Context, FFaerieAddress Address, EFaerieSpatialItemRotation RotationToAdd) const override;
	//~ UInventoryGridExtensionBase

private:
	void RemoveItem(const FFaerieContainerGridWriteContext& Context, FFaerieAddress Address, const FFaerieItemInstance& Item) const;
	void RemoveItemBatch(const FFaerieContainerGridWriteContext& Context, const TConstArrayView<FFaerieAddress>& Addresses, const FFaerieItemInstance& Item) const;

	// The client has to manually rebuild its cell after a removal, as the item's shape is likely lost.
	void RebuildOccupiedCells(const FFaerieContainerGridWriteContext& Context) const;

	// Gets a shape from a shape fragment on the item, or returns a single cell at 0,0 for items with no fragment.
	FFaerieGridShapeConstView GetItemShape_Impl(const FFaerieItemInstance& Item) const;
	FFaerieGridShapeConstView GetItemShape_Impl(TNotNull<const UFaerieItemStorage*> Storage, FFaerieAddress Address) const;

public:
	bool CanAddItemToGrid(const FFaerieContainerGridReadContext& Context, const FFaerieGridShapeConstView& Shape) const;
	bool CanAddItemsToGrid(const FFaerieContainerGridReadContext& Context, const TConstArrayView<FFaerieGridShapeConstView>& Shapes) const;

	// Gets the normalized shape for an item. This copies the shape!
	UFUNCTION(BlueprintCallable, Category = "Faerie|SpatialGrid")
	FFaerieGridShape GetItemShape(const UFaerieItemStorage* Storage, FFaerieAddress Address) const;

	// Gets the shape of an item transposed on the grid according to its placement.
	UFUNCTION(BlueprintCallable, Category = "Faerie|SpatialGrid")
	FFaerieGridShape GetItemShapeOnGrid(const FFaerieContainerGridReadContext& Context, FFaerieAddress Address) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|SpatialGrid")
	FIntPoint GetStackBounds(const FFaerieContainerGridReadContext& Context, FFaerieAddress Address) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|SpatialGrid")
	bool CanAddAtLocation(const FFaerieContainerGridReadContext& Context, const FFaerieGridShape& Shape, FIntPoint Position) const;
	bool CanAddAtLocation(const FFaerieContainerGridReadContext& Context, const FFaerieGridShapeConstView& Shape, FIntPoint Position) const;

protected:
	Faerie::Extensions::FExclusionSet MakeExclusionSet(const FFaerieContainerGridReadContext& Context, FFaerieAddress ExcludedAddress) const;
	Faerie::Extensions::FExclusionSet MakeExclusionSet(const FFaerieContainerGridReadContext& Context, const TConstArrayView<FFaerieAddress> ExcludedAddresses) const;

	bool FitsInGridAnyRotation(const FFaerieContainerGridReadContext& Context, const FFaerieGridShapeConstView& Shape, FIntPoint Origin, const Faerie::Extensions::FExclusionSet& ExclusionSet) const;

	FFaerieAddress FindOverlappingItem(const FFaerieContainerGridReadContext& Context, const FFaerieGridShapeConstView& TranslatedShape, FFaerieAddress Address) const;

	bool TrySwapItems(const FFaerieContainerGridWriteContext& Context, FFaerieAddress AddressA, FFaerieGridPlacement& PlacementA, FFaerieAddress AddressB, FFaerieGridPlacement& PlacementB) const;

	bool MoveSingleItem(const FFaerieContainerGridWriteContext& Context, const FFaerieAddress Address, FFaerieGridPlacement& Placement, const FIntPoint& NewPosition) const;
};