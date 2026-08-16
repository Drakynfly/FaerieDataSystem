// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemProxy.h"
#include "FaerieItemDataView.h"
#include "FaerieItemOwnerInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemProxy)

bool FFaerieItemProxy::IsValid() const
{
	return !!InterfacePtr;
}

bool FFaerieItemProxy::HasValidInstance() const
{
	if (InterfacePtr)
	{
		return InterfacePtr->GetItemInstance().IsSet() && Faerie::ItemData::IsValidStackAmount(InterfacePtr->GetCopies());
	}
	return false;
}

TOptional<FFaerieItemInstance> FFaerieItemProxy::GetItemInstance() const
{
	if (InterfacePtr)
	{
		return InterfacePtr->GetItemInstance();
	}
	return NullOpt;
}

FFaerieItemInstance FFaerieItemProxy::GetItemInstanceOrInvalid() const
{
	if (InterfacePtr)
	{
		auto Option = InterfacePtr->GetItemInstance();
		if (Option.IsSet())
		{
			return Option.GetValue();
		}
	}
	return FFaerieItemInstance();
}

int32 FFaerieItemProxy::GetCopies() const
{
	if (InterfacePtr)
	{
		return InterfacePtr->GetCopies();
	}
	return 0;
}

UObject* FFaerieItemProxy::GetItemOwner() const
{
	if (InterfacePtr)
	{
		return const_cast<UObject*>(Cast<UObject>(InterfacePtr->GetItemOwner()));
	}
	return nullptr;
}

Faerie::ItemData::FProxyChangeEvent::RegistrationType& FFaerieItemProxy::GetOnProxyChangeEvent() const
{
	if (const IFaerieItemDataProxy* ProxyObj = Cast<IFaerieItemDataProxy>(ProxyObject))
	{
		return const_cast<IFaerieItemDataProxy*>(ProxyObj)->GetOnProxyChangeEvent();
	}

	// GetOnProxyChangeEvent should *never* be called on invalid Proxies!
	checkNoEntry();
	static Faerie::ItemData::FProxyChangeEvent Blank;
	return Blank;
}