// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemProxyUtils.h"
#include "FaerieItemProxy.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemProxyUtils)

bool UFaerieItemProxyUtils::CastProxy(const FFaerieItemProxy& Proxy, UClass* Class, UObject*& ProxyObject)
{
	// @note The const_cast here is fine. This function acts as a "Resolve" of the weak pointer in the proxy struct,
	// which is usually treated as a const view of a proxy. CastProxy implicitly wants to resolve to a non-const pointer,
	// and Blueprint doesn't understand const pointers anyway.
	if (const UObject* Object = Proxy.GetObject())
	{
		ProxyObject = const_cast<UObject*>(Object);
		return ProxyObject->IsA(Class);
	}
	return false;
}

FFaerieItemProxy UFaerieItemProxyUtils::ToWeakProxy(const TScriptInterface<IFaerieItemDataProxy>& ScriptProxy)
{
	return FFaerieItemProxy(ScriptProxy);
}

const UObject* UFaerieItemProxyUtils::GetProxyObject(const FFaerieItemProxy& Proxy)
{
	return Proxy.GetObject();
}

bool UFaerieItemProxyUtils::IsValid(const FFaerieItemProxy& Proxy)
{
	return Proxy.IsValid();
}

const UFaerieItem* UFaerieItemProxyUtils::GetItemObject(const FFaerieItemProxy& Proxy)
{
	if (Proxy.IsValid())
	{
		return Proxy.GetItemObject();
	}
	return nullptr;
}

int32 UFaerieItemProxyUtils::GetCopies(const FFaerieItemProxy& Proxy)
{
	if (Proxy.IsValid())
	{
		return Proxy.GetCopies();
	}
	return 0;
}

TScriptInterface<IFaerieItemOwnerInterface> UFaerieItemProxyUtils::GetOwner(const FFaerieItemProxy& Proxy)
{
	if (Proxy.IsValid())
	{
		return Proxy.GetOwner();
	}
	return nullptr;
}

void UFaerieItemProxyUtils::BindToItemDataChanged(const FFaerieItemProxy& Proxy,
	const FFaerieItemProxyChangedEvent& Event)
{
	if (Proxy.IsValid())
	{
		(void)Proxy->BindToItemDataChanged(Event);
	}
}

void UFaerieItemProxyUtils::UnbindAllFromItemDataChanged(const FFaerieItemProxy& Proxy, const UObject* Object)
{
	if (Proxy.IsValid())
	{
		return Proxy->UnbindAllFromItemDataChanged(Object);
	}
}

FFaerieItemStackView UFaerieItemProxyUtils::ProxyToView(const FFaerieItemProxy& Proxy)
{
	return FFaerieItemStackView(GetItemObject(Proxy), GetCopies(Proxy));
}
