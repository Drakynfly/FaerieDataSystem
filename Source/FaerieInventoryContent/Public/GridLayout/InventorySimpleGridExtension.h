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
	void RemoveItem(const FFaerieContainerGridWriteContext& Context, FFaerieAddress Address, const FFaerieItemInstance& Instance) const;
	void RemoveItemBatch(const FFaerieContainerGridWriteContext& Context, const TConstArrayView<FFaerieAddress>& Keys, const FFaerieItemInstance& Instance) const;

public:
	static FFaerieGridPlacement FindFirstEmptyLocation(const FFaerieContainerGridReadContext& Context);

protected:
	TOptional<FFaerieAddress> FindOverlappingItem(const FFaerieContainerGridReadContext& Context, const FIntPoint& Position, FFaerieAddress ExcludeAddress) const;

	static void MoveSingleItem(const FFaerieContainerGridWriteContext& Context, FFaerieGridPlacement& Placement, const FIntPoint& NewPosition);
};