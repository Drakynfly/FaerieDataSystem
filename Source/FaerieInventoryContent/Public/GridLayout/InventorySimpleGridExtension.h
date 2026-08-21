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
	void RemoveItem(FFaerieAddress Address, const FFaerieItemInstance& Instance);
	void RemoveItemBatch(const TConstArrayView<FFaerieAddress>& Keys, const FFaerieItemInstance& Instance);

public:
	FFaerieGridPlacement FindFirstEmptyLocation() const;

protected:
	FFaerieAddress FindOverlappingItem(FFaerieAddress ExcludeAddress) const;

	void SwapItems(FFaerieGridPlacement& PlacementA, FFaerieGridPlacement& PlacementB);
	void MoveSingleItem(FFaerieGridPlacement& Placement, const FIntPoint& NewPosition);
};