// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieGridStructs.h"
#include "InventoryGridExtensionBase.h"
#include "SpatialTypes.h"
#include "InventorySpatialGridExtension.generated.h"

/**
 *
 */
UCLASS()
class FAERIEINVENTORYCONTENT_API UInventorySpatialGridExtension : public UInventoryGridExtensionBase
{
	GENERATED_BODY()

protected:
	//~ UItemContainerExtensionBase
	virtual void InitializeExtension(const UFaerieItemContainerBase* Container) override;
	virtual EEventExtensionResponse AllowsAddition(const UFaerieItemContainerBase* Container, FFaerieItemStackView Stack, EFaerieStorageAddStackBehavior AddStackBehavior) const override;
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

	virtual bool CanAddAtLocation(FFaerieItemStackView Stack, FIntPoint IntPoint) const override;
	virtual bool MoveItem(const FInventoryKey& Key, const FIntPoint& TargetPoint) override;
	virtual bool RotateItem(const FInventoryKey& Key) override;
	//~ UInventoryGridExtensionBase

private:
	bool AddItemToGrid(const FInventoryKey& Key, const UFaerieItem* Item);
	void RemoveItem(const FInventoryKey& Key, const UFaerieItem* Item);
	void RemoveItemBatch(const TConstArrayView<FInventoryKey>& Keys, const UFaerieItem* Item);

	// The client has to manually rebuild its cell after a removal, as the item's shape is likely lost.
	void RebuildOccupiedCells();

	// Gets a shape from a shape token on the item, or returns a single cell at 0,0 for items with no token.
	FFaerieGridShape GetItemShape_Impl(const UFaerieItem* Item) const;

public:
	bool CanAddItemToGrid(const FFaerieGridShapeConstView& Shape) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|SpatialGrid")
	FFaerieGridShape GetItemShape(FEntryKey Key) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|SpatialGrid")
	FIntPoint GetStackBounds(const FInventoryKey& Key) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|SpatialGrid")
	bool CanAddAtLocation(const FFaerieGridShape& Shape, FIntPoint Position) const;

protected:
	using FExclusionSet = TSet<FIntPoint>;

	[[nodiscard]] static FFaerieGridShape ApplyPlacement(const FFaerieGridShapeConstView& Shape, const FFaerieGridPlacement& Placement);
	static void ApplyPlacementInline(FFaerieGridShape& Shape, const FFaerieGridPlacement& Placement);

	FExclusionSet MakeExclusionSet(FInventoryKey ExcludedKey) const;
	FExclusionSet MakeExclusionSet(const TConstArrayView<FInventoryKey> ExcludedKeys) const;

	FFaerieGridPlacement FindFirstEmptyLocation(const FFaerieGridShapeConstView& Shape) const;

	bool FitsInGrid(const FFaerieGridShapeConstView& TranslatedShape, const FExclusionSet& ExclusionSet) const;

	bool FitsInGridAnyRotation(const FFaerieGridShapeConstView& Shape, FIntPoint Origin, const FExclusionSet& ExclusionSet) const;

	FInventoryKey FindOverlappingItem(const FFaerieGridShapeConstView& TranslatedShape, const FInventoryKey& ExcludeKey) const;

	bool TrySwapItems(FInventoryKey KeyA, FFaerieGridPlacement& PlacementA, FInventoryKey KeyB, FFaerieGridPlacement& PlacementB);

	bool MoveSingleItem(const FInventoryKey Key, FFaerieGridPlacement& Placement, const FIntPoint& NewPosition);

	void AddItemPosition(const FFaerieGridShapeConstView TranslatedShape);
	void RemoveItemPosition(const FFaerieGridShapeConstView& TranslatedShape);
};