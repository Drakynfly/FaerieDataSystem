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

protected:
	//~ UItemContainerExtensionBase
	virtual EEventExtensionResponse AllowsAddition(TNotNull<const UFaerieItemContainerBase*> Container, const Faerie::Utils::TArrayAdapter<FFaerieItemProxy>& Proxies, FFaerieExtensionAllowsAdditionArgs Args) const override;
	virtual EEventExtensionResponse AllowsEdit(TNotNull<const UFaerieItemContainerBase*> Container, const TNotNull<const Faerie::Container::IAddressView*> DataView, FFaerieInventoryTag EditType) const override;
	virtual void PostEventBatch(TNotNull<const UFaerieItemContainerBase*> Container, const Faerie::Inventory::FEventLogBatch& Events) override;
	//~ UItemContainerExtensionBase

	//~ UInventoryGridExtensionBase
	virtual void PreStackRemove_Client(const FFaerieGridKeyedStack& Stack) override;
	virtual void PreStackRemove_Server(const FFaerieGridKeyedStack& Stack, const FFaerieItemInstance& Item) override;

	virtual void PostStackAdd(const FFaerieGridKeyedStack& Stack) override;
	virtual void PostStackChange(const FFaerieGridKeyedStack& Stack) override;

public:
	virtual FFaerieAddress GetKeyAt(const FIntPoint& Position) const override;
	virtual bool CanAddAtLocation(Faerie::TValid<const FFaerieItemProxy&> Proxy, FIntPoint IntPoint) const override;
	virtual bool AddItemToGrid(FFaerieAddress Address, const FFaerieItemInstance& Instance) override;
	virtual bool MoveItem(FFaerieAddress Address, const FIntPoint& TargetPoint) override;
	virtual bool RotateItem(FFaerieAddress Address, EFaerieSpatialItemRotation RotationToAdd) override;
	//~ UInventoryGridExtensionBase

private:
	void RemoveItem(FFaerieAddress Address, const FFaerieItemInstance& Item);
	void RemoveItemBatch(const TConstArrayView<FFaerieAddress>& Addresses, const FFaerieItemInstance& Item);

	// The client has to manually rebuild its cell after a removal, as the item's shape is likely lost.
	void RebuildOccupiedCells();

	// Gets a shape from a shape fragment on the item, or returns a single cell at 0,0 for items with no fragment.
	FFaerieGridShapeConstView GetItemShape_Impl(const FFaerieItemInstance& Item) const;
	FFaerieGridShapeConstView GetItemShape_Impl(FFaerieAddress Address) const;

public:
	bool CanAddItemToGrid(const FFaerieGridShapeConstView& Shape) const;
	bool CanAddItemsToGrid(const TArray<FFaerieGridShapeConstView>& Shapes) const;

	// Gets the normalized shape for an item. This copies the shape!
	UFUNCTION(BlueprintCallable, Category = "Faerie|SpatialGrid")
	FFaerieGridShape GetItemShape(FFaerieAddress Address) const;

	// Gets the shape of an item transposed on the grid according to its placement.
	UFUNCTION(BlueprintCallable, Category = "Faerie|SpatialGrid")
	FFaerieGridShape GetItemShapeOnGrid(FFaerieAddress Address) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|SpatialGrid")
	FIntPoint GetStackBounds(FFaerieAddress Address) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|SpatialGrid")
	bool CanAddAtLocation(const FFaerieGridShape& Shape, FIntPoint Position) const;
	bool CanAddAtLocation(const FFaerieGridShapeConstView& Shape, FIntPoint Position) const;

protected:
	Faerie::Extensions::FExclusionSet MakeExclusionSet(FFaerieAddress ExcludedAddress) const;
	Faerie::Extensions::FExclusionSet MakeExclusionSet(const TConstArrayView<FFaerieAddress> ExcludedAddresses) const;

	bool FitsInGridAnyRotation(const FFaerieGridShapeConstView& Shape, FIntPoint Origin, const Faerie::Extensions::FExclusionSet& ExclusionSet) const;

	FFaerieAddress FindOverlappingItem(const FFaerieGridShapeConstView& TranslatedShape, FFaerieAddress ExcludeAddress) const;

	bool TrySwapItems(FFaerieAddress AddressA, FFaerieGridPlacement& PlacementA, FFaerieAddress AddressB, FFaerieGridPlacement& PlacementB);

	bool MoveSingleItem(const FFaerieAddress Address, FFaerieGridPlacement& Placement, const FIntPoint& NewPosition);
};