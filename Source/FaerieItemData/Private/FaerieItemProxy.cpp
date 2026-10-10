// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemProxy.h"
#include "FaerieItemDataLog.h"
#include "FaerieItemDataView.h"

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

UObject* FFaerieItemProxy::GetItemOwner() const
{
	if (!ProxyObject && !ValidWithoutProxyObject)
	{
		return nullptr;
	}

	if (InterfacePtr)
	{
		return const_cast<UObject*>(Cast<UObject>(InterfacePtr->GetItemOwner()));
	}
	return nullptr;
}

const UWorld* FFaerieItemProxy::ExtractWorld() const
{
	if (!ProxyObject && !ValidWithoutProxyObject)
	{
		return nullptr;
	}

	if (ProxyObject)
	{
		if (const UWorld* WorldFromProxy = ProxyObject->GetWorld())
		{
			return WorldFromProxy;
		}
	}
	if (InterfacePtr)
	{
		if (const UObject* Owner = InterfacePtr->GetItemOwner())
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

#if WITH_EDITOR
const UWorld* FFaerieItemProxy::TestExtractWorld() const
{
	if (!ProxyObject && !ValidWithoutProxyObject)
	{
		return nullptr;
	}

	if (ProxyObject)
	{
		if (const UWorld* WorldFromProxy = ProxyObject->GetWorld())
		{
			return WorldFromProxy;
		}
	}
	if (InterfacePtr)
	{
		if (const UObject* Owner = InterfacePtr->GetItemOwner())
		{
			if (const UWorld* WorldFromOwner = Cast<UObject>(Owner)->GetWorld())
			{
				return WorldFromOwner;
			}
		}
	}

	return nullptr;
}
#endif

bool FFaerieItemProxy::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bSuccess)
{
	checkf(false, TEXT("Do not replicate FFaerieItemProxy, use FFaerieItemNetworkHandle instead!"))
	bSuccess = false;
	return bSuccess;
}
