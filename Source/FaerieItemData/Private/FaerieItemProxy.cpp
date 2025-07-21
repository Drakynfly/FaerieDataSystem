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
	if (Proxy.IsValid())
	{
		return operator->()->GetItemObject();
	}
	return nullptr;
}

int32 FFaerieItemProxy::GetCopies() const
{
	if (Proxy.IsValid())
	{
		return operator->()->GetCopies();
	}
	return 0;
}

TScriptInterface<IFaerieItemOwnerInterface> FFaerieItemProxy::GetOwner() const
{
	if (Proxy.IsValid())
	{
		return operator->()->GetItemOwner();
	}
	return nullptr;
}

bool FFaerieItemProxy::IsInstanceMutable() const
{
	if (auto&& Object = GetItemObject();
		::IsValid(Object))
	{
		return Object->IsInstanceMutable();
	}
	return false;
}