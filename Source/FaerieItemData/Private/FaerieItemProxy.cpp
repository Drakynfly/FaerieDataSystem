// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemProxy.h"
#include "FaerieItemDataView.h"
#include "MassEntityManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemProxy)

bool FFaerieItemProxy::IsValid() const
{
	return HasValidInstance();
}

bool FFaerieItemProxy::HasValidInstance() const
{
	if (!ProxyObject && !ValidWithoutProxyObject)
	{
		return false;
	}

	if (InterfacePtr)
	{
		return InterfacePtr->GetItemInstance().IsSet() && Faerie::ItemData::IsValidStackAmount(InterfacePtr->GetCopies());
	}
	return false;
}

FMassEntityHandle FFaerieItemProxy::GetEntityHandle() const
{
	if (!ProxyObject && !ValidWithoutProxyObject)
	{
		return FMassEntityHandle();
	}

	if (InterfacePtr)
	{
		return InterfacePtr->GetEntityHandle();
	}
	return FMassEntityHandle();
}

TOptional<FFaerieItemInstance> FFaerieItemProxy::GetItemInstance() const
{
	if (!ProxyObject && !ValidWithoutProxyObject)
	{
		return NullOpt;
	}

	if (InterfacePtr)
	{
		return InterfacePtr->GetItemInstance();
	}
	return NullOpt;
}

FFaerieItemInstance FFaerieItemProxy::GetItemInstanceOrInvalid() const
{
	if (!ProxyObject && !ValidWithoutProxyObject)
	{
		return FFaerieItemInstance();
	}

	if (InterfacePtr)
	{
		const TOptional<FFaerieItemInstance> Option = InterfacePtr->GetItemInstance();
		if (Option.IsSet())
		{
			return Option.GetValue();
		}
	}
	return FFaerieItemInstance();
}

int32 FFaerieItemProxy::GetCopies() const
{
	if (!ProxyObject && !ValidWithoutProxyObject)
	{
		return 0;
	}

	if (InterfacePtr)
	{
		return InterfacePtr->GetCopies();
	}
	return 0;
}

bool FFaerieItemProxy::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bSuccess)
{
	checkf(false, TEXT("Do not replicate FFaerieItemProxy, use FFaerieItemNetworkHandle instead!"))
	bSuccess = false;
	return bSuccess;
}
