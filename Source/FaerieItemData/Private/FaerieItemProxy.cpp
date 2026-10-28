// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemProxy.h"
#include "FaerieItem.h"
#include "FaerieItemDataView.h"
#include "FaerieItemInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemProxy)

using namespace Faerie;

bool FFaerieItemProxy::IsValid() const
{
	return HasValidStack();
}

bool FFaerieItemProxy::IsMutable() const
{
	if (EntityManager.IsValid())
	{
		return ItemData::IsMutable(*EntityManager.Get(), GetEntityHandle());
	}

#if WITH_EDITOR
	if (const UFaerieItem* ItemAsset = GetItemPtr())
	{
		return ItemAsset->CanMutate();
	}
#endif

	return false;
}

bool FFaerieItemProxy::HasValidStack() const
{
	if (!ProxyObject && !ValidWithoutProxyObject)
	{
		return false;
	}

	if (InterfacePtr)
	{
		if (!ItemData::IsValidStackAmount(InterfacePtr->GetCopies()))
		{
			return false;
		}

		return InterfacePtr->GetEntityHandle().IsSet()
#if WITH_EDITOR
			|| ::IsValid(InterfacePtr->GetItemAsset())
#endif
			;
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

#if WITH_EDITOR
const UFaerieItem* FFaerieItemProxy::GetItemPtr() const
{
	if (!ProxyObject && !ValidWithoutProxyObject)
	{
		return nullptr;
	}

	if (InterfacePtr)
	{
		if (const UFaerieItem* ItemAsset = InterfacePtr->GetItemAsset())
		{
			return ItemAsset;
		}
	}
	return nullptr;
}
#endif

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
