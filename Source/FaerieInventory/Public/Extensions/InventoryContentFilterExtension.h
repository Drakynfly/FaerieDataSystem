// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "ItemContainerExtensionBase.h"
#include "InventoryContentFilterExtension.generated.h"

class UFaerieItemTemplate;

/**
 * An extension that only allows items matching a filter to be contained.
 */
USTRUCT()
struct FAERIEINVENTORY_API FFaerieItemContainerContentFilter : public FFaerieItemContainerExtensionBase
{
	GENERATED_BODY()

	//~ FFaerieItemContainerExtensionBase
	virtual EFaerieExtensionResponse AllowsAddition(TNotNull<const UFaerieItemContainerBase*> Container, const Faerie::Utils::TArrayAdapter<FFaerieItemProxy>& Proxies, FFaerieExtensionAllowsAdditionArgs Args) const override;
	//~ FFaerieItemContainerExtensionBase

	UPROPERTY(EditAnywhere, Category = "ItemContainerContentFilter")
	TObjectPtr<class UFaerieItemTemplate> Filter;

____FAERIE_CONTAINER_DATA_DECL(FFaerieItemContainerContentFilter)
};