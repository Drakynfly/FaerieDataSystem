// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemProxy.h"
#include "FaerieItem.h"
#include "FaerieItemDataView.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemProxy)

bool FFaerieItemProxy::IsValid() const
{
	if (const IFaerieItemDataProxy* ProxyObj = GetInterface())
	{
		return ProxyObj->GetItemInstance().IsSet() &&
			Faerie::ItemData::IsValidStackAmount(ProxyObj->GetCopies());
	}
	return false;
}

const IFaerieItemDataProxy* FFaerieItemProxy::operator->() const
{
	return GetInterface();
}

Faerie::ItemData::FProxyChangeEvent::RegistrationType& FFaerieItemProxy::GetOnProxyChangeEvent() const
{
	if (const IFaerieItemDataProxy* ProxyObj = GetInterface())
	{
		return const_cast<IFaerieItemDataProxy*>(ProxyObj)->GetOnProxyChangeEvent();
	}

	// GetOnProxyChangeEvent should *never* be called on invalid Proxies!
	checkNoEntry();
	static Faerie::ItemData::FProxyChangeEvent Blank;
	return Blank;
}