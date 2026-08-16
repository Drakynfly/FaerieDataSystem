// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "ItemContainerExtensionBase.h"

#include "InventoryContentFilterExtension.generated.h"

struct FFaerieItemDataFilterBase;

/**
 * An extension that only allows items matching a filter to be contained.
 */
UCLASS()
class FAERIEINVENTORYCONTENT_API UInventoryContentFilterExtension : public UItemContainerExtensionBase
{
	GENERATED_BODY()

protected:
	//~ UItemContainerExtensionBase
	virtual EEventExtensionResponse AllowsAddition(TNotNull<const UFaerieItemContainerBase*> Container, const Faerie::Utils::TArrayAdapter<FFaerieItemProxy>& Proxies, FFaerieExtensionAllowsAdditionArgs Args) const override;
	//~ UItemContainerExtensionBase

	// Filter used to determine if an item can be contained in the inventory
	UPROPERTY(EditAnywhere, Category = "Config", meta = (DisplayThumbnail = false))
	TInstancedStruct<FFaerieItemDataFilterBase> Filter;
};