// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemContainerStructs.h"
#include "Actions/FaerieClientActionBase.h"
#include "ItemUseConsumableRequest.generated.h"

/**
 * Request to use a consumable.
 */
USTRUCT(BlueprintType)
struct FFaerieClientAction_UseConsumable final : public FFaerieClientActionBase
{
	GENERATED_BODY()

	virtual bool Server_Execute(const UFaerieInventoryClient* Client) const override;

	UPROPERTY(BlueprintReadWrite, Category = "UseConsumable")
	FFaerieAddressableHandle Handle;
};