// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemContainerStructs.h"
#include "FaerieItemContainerBase.h"
#include "FaerieItemProxyBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemContainerStructs)

bool FFaerieItemNetworkHandle::IsValid() const
{
	return Container.IsValid() && Container->Contains(Address);
}

FFaerieItemProxy FFaerieItemNetworkHandle::ResolveProxy() const
{
	if (Container.IsValid())
	{
		return Container->Proxy(Address);
	}
	return FFaerieItemProxy();
}

FFaerieItemNetworkHandle FFaerieItemNetworkHandle::FromProxy(const FFaerieItemProxy& Proxy)
{
	if (const IFaerieContainerProxy* ContainerProxy = Proxy.GetTypedProxyObject<IFaerieContainerProxy>())
	{
		return ContainerProxy->Proxy_GetNetworkHandle();
	}
	return FFaerieItemNetworkHandle();
}

bool FFaerieItemNetworkHandle::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	Ar << Container;
	Ar << Address;
	bOutSuccess = true;
	return true;
}
