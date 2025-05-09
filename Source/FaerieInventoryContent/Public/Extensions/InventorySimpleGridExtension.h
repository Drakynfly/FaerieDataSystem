// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieGridStructs.h"
#include "InventoryGridExtensionBase.h"
#include "InventorySimpleGridExtension.generated.h"

/**
 *
 */
UCLASS()
class FAERIEINVENTORYCONTENT_API UInventorySimpleGridExtension : public UInventoryGridExtensionBase
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

public:
	FFaerieGridPlacement FindFirstEmptyLocation() const;

protected:
	FInventoryKey FindOverlappingItem(const FInventoryKey& ExcludeKey) const;

	void SwapItems(FFaerieGridPlacement& PlacementA, FFaerieGridPlacement& PlacementB);
	void MoveSingleItem(FFaerieGridPlacement& Placement, const FIntPoint& NewPosition);
};