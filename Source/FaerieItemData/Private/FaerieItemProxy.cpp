// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemProxy.h"
#include "FaerieItem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemProxy)

bool FFaerieItemProxy::IsValid() const
{
	if (const IFaerieItemDataProxy* ProxyObj = operator->())
	{
		return ::IsValid(ProxyObj->GetItemObject());
	}
	return false;
}

const UFaerieItem* FFaerieItemProxy::GetItemObject() const
{
	if (const IFaerieItemDataProxy* ProxyObj = operator->())
	{
		return ProxyObj->GetItemObject();
	}
	return nullptr;
}

int32 FFaerieItemProxy::GetCopies() const
{
	if (const IFaerieItemDataProxy* ProxyObj = operator->())
	{
		return ProxyObj->GetCopies();
	}
	return 0;
}

TScriptInterface<IFaerieItemOwnerInterface> FFaerieItemProxy::GetOwner() const
{
	if (const IFaerieItemDataProxy* ProxyObj = operator->())
	{
		return ProxyObj->GetItemOwner();
	}
	return nullptr;
}

FFaerieItemStack FFaerieItemProxy::Release(const int32 Copies) const
{
	if (const IFaerieItemDataProxy* ProxyObj = operator->())
	{
		return ProxyObj->Release(Copies);
	}
	return FFaerieItemStack();
}

const IFaerieItemDataProxy* FFaerieItemProxy::operator->() const
{
	return Cast<IFaerieItemDataProxy>(Proxy.Get());
}

FFaerieItemProxy::operator FFaerieItemStackView() const
{
	if (const IFaerieItemDataProxy* ProxyObj = operator->())
	{
		return FFaerieItemStackView(ProxyObj->GetItemObject(), ProxyObj->GetCopies());
	}
	return FFaerieItemStackView();
}
