// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemProxy.h"
#include "FaerieItem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemProxy)

bool FFaerieItemProxy::IsValid() const
{
	return ::IsValid(GetItemObject());
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

bool FFaerieItemProxy::IsInstanceMutable() const
{
	if (const UFaerieItem* Object = GetItemObject();
		::IsValid(Object))
	{
		return Object->IsInstanceMutable();
	}
	return false;
}

FFaerieItemProxy::operator FFaerieItemStackView() const
{
	if (const IFaerieItemDataProxy* ProxyObj = operator->())
	{
		return FFaerieItemStackView(ProxyObj->GetItemObject(), ProxyObj->GetCopies());
	}
	return FFaerieItemStackView();
}
