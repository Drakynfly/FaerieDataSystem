// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemContainerStructs.h"
#include "FaerieItemProxy.h"

#include "FaerieItemProxyBase.generated.h"

UINTERFACE(NotBlueprintable)
class UFaerieContainerProxy : public UFaerieItemDataProxy
{
	GENERATED_BODY()
};

/**
 * A proxy that is aware of FaerieInventory module types.
 */
class IFaerieContainerProxy : public IFaerieItemDataProxy
{
	GENERATED_BODY()

public:
	virtual FFaerieAddress Proxy_GetAddress() const PURE_VIRTUAL(IFaerieContainerProxy::GetAddress, return FFaerieAddress(); )
	virtual FFaerieItemNetworkHandle Proxy_GetNetworkHandle() const PURE_VIRTUAL(IFaerieContainerProxy::Proxy_GetNetworkHandle, return FFaerieItemNetworkHandle(); )
};