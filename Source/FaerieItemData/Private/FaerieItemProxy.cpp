// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemProxy.h"
#include "FaerieItemDataLog.h"
#include "FaerieItemDataView.h"
#include "FaerieItemOwnerInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemProxy)

bool FFaerieItemProxy::IsValid() const
{
	return HasValidInstance();
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

const UWorld* FFaerieItemProxy::ExtractWorld() const
{
	if (ProxyObject)
	{
		if (const UWorld* WorldFromProxy = ProxyObject->GetWorld())
		{
			return WorldFromProxy;
		}
	}
	if (InterfacePtr)
	{
		if (const IFaerieItemOwnerInterface* Owner = InterfacePtr->GetItemOwner())
		{
			if (const UWorld* WorldFromOwner = Cast<UObject>(Owner)->GetWorld())
			{
				return WorldFromOwner;
			}
		}
	}

	UE_LOGF(LogFaerieItemData, Error, "Unable to determine world from item proxy. We are likely a non-persistent proxy, which should not be used to fetch worlds from!")
	return nullptr;
}

bool FFaerieItemProxy::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bSuccess)
{
	checkf(false, TEXT("Do not replicate FFaerieItemProxy, use FFaerieItemNetworkHandle instead!"))
	bSuccess = false;
	return bSuccess;
}
