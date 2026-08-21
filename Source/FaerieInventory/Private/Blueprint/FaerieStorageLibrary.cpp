// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieStorageLibrary.h"
#include "DelegateCommon.h"
#include "EntityManagerHelpers.h"
#include "FaerieContainerFilter.h"
#include "FaerieContainerFilterTypes.h"
#include "FaerieItemStorage.h"
#include "FaerieItemStorageIterators.h"
#include "FaerieSubObjectFilter.h"
#include "ItemStackProxy.h"

#include "Engine/World.h"
#include "Fragments/FaerieStackLimitFragment.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieStorageLibrary)

using namespace Faerie;

FString UFaerieStorageLibrary::ToString_Address(const FFaerieAddress Address)
{
	auto Keys = UFaerieItemStorage::BreakAddress(Address);
	return Keys.Get<0>().ToString() + TEXT(":") + Keys.Get<1>().ToString();
}

int32 UFaerieStorageLibrary::GetItemStackLimit(const FFaerieItemProxy& Proxy)
{
	if (Proxy.IsValid())
	{
		return Container::GetItemStackLimit(ItemData::GetFaerieEntityManager(), Proxy.GetItemInstanceOrInvalid());
	}
	return 0;
}

bool UFaerieStorageLibrary::GetNetworkHandleFromProxy(const FFaerieItemProxy& Proxy,
	FFaerieItemNetworkHandle& OutHandle)
{
	OutHandle = FFaerieItemNetworkHandle::FromProxy(Proxy);
	return OutHandle.IsValid();
}

FFaerieItemProxy UFaerieStorageLibrary::GetProxyFromNetworkHandle(const FFaerieItemNetworkHandle& Handle)
{
	return Handle.ResolveProxy();
}

TArray<UFaerieItemStackProxy*> UFaerieStorageLibrary::GetAllStackProxies(UFaerieItemStorage* Storage)
{
	if (!IsValid(Storage))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Storage passed to UFaerieStorageLibrary::GetAllStackProxies"), ELogVerbosity::Error);
		return {};
	}

	TArray<UFaerieItemStackProxy*> Proxies;
	Proxies.Reserve(Storage->GetStackCount());
	for (auto It = Container::FIterator_AllAddresses(Storage); It; ++It)
	{
		Proxies.Add(const_cast<UFaerieItemStackProxy*>(Storage->GetProxy(*It)));
	}
	return Proxies;
}

FFaerieAddress UFaerieStorageLibrary::QueryFirst(UFaerieItemStorage* Storage, const FFaerieProxyPredicate& Filter)
{
	if (!IsValid(Storage))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Storage passed to UFaerieStorageLibrary::QueryFirst"), ELogVerbosity::Error);
		return FFaerieAddress();
	}

	if (!Filter.IsBound())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Filter passed to UFaerieStorageLibrary::QueryFirst"), ELogVerbosity::Error);
		return FFaerieAddress();
	}

	return Container::FAddressFilter()
		.By(Container::FCallbackFilter{DYNAMIC_TO_NATIVE(ItemData::FViewPredicate, Filter)})
		.First(ItemData::GetFaerieEntityManager(), Storage);
}

UFaerieItemContainerBase* UFaerieStorageLibrary::GetOwningContainer_Proxy(const FFaerieItemProxy& Proxy)
{
	return Cast<UFaerieItemContainerBase>(Proxy.GetItemOwner());
}

bool UFaerieStorageLibrary::FindSubobject(const FFaerieItemProxy& Proxy, const TSubclassOf<UFaerieItemContainerBase> Class,
										  UFaerieItemContainerBase*& FoundContainers, const bool Recursive)
{
	if (!Proxy.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Proxy passed to UFaerieStorageLibrary::FindSubobject"), ELogVerbosity::Error);
		return false;
	}

	auto InstanceOpt = Proxy.GetItemInstance();
	if (!InstanceOpt.IsSet())
	{
		return false;
	}

	auto Instance = InstanceOpt.GetValue();
	if (!Instance.IsMutable()) return false;

#if WITH_EDITOR
	if (!ItemData::HasFaerieEntityManagerBeenAssigned())
	{
		TArray<TNotNull<const UFaerieItemContainerBase*>> Containers;
		if (Recursive)
		{
			SubObject::GetTemplateContainersInInstanceRecursive(Instance, Containers, Class);
		}
		else
		{
			SubObject::GetTemplateContainersInInstanceDirect(Instance, Containers, Class);
		}

		for (const TNotNull<const UFaerieItemContainerBase*> SubObject : Containers)
		{
			FoundContainers = const_cast<UFaerieItemContainerBase*>(NotNullGet(SubObject));
			return true;
		}
	}
	else
#endif
	{
		auto& EntityManager = ItemData::GetFaerieEntityManagerChecked();
		TArray<TNotNull<UFaerieItemContainerBase*>> Containers;
		if (Recursive)
		{
			SubObject::GetContainersInInstanceRecursive(EntityManager, Instance, Containers, Class);
		}
		else
		{
			SubObject::GetContainersInInstanceDirect(EntityManager, Instance, Containers, Class);
		}

		for (const TNotNull<UFaerieItemContainerBase*> SubObject : Containers)
		{
			FoundContainers = SubObject;
			return true;
		}
	}

	return false;
}

void UFaerieStorageLibrary::FindSubObjectsByClass(const FFaerieItemProxy& Proxy, const TSubclassOf<UFaerieItemContainerBase> Class,
	TArray<UFaerieItemContainerBase*>& FoundContainers, const bool Recursive)
{
	if (!Proxy.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Proxy passed to UFaerieStorageLibrary::FindSubObjectsByClass"), ELogVerbosity::Error);
		return;
	}

	auto InstanceOpt = Proxy.GetItemInstance();
	if (!InstanceOpt.IsSet())
	{
		return;
	}

	auto Instance = InstanceOpt.GetValue();
	if (!Instance.IsMutable()) return;

#if WITH_EDITOR
	if (!ItemData::HasFaerieEntityManagerBeenAssigned())
	{
		if (Recursive)
		{
			SubObject::GetTemplateContainersInInstanceRecursive(Instance, *reinterpret_cast<TArray<TNotNull<const UFaerieItemContainerBase*>>*>(&FoundContainers), Class);
		}
		else
		{
			SubObject::GetTemplateContainersInInstanceDirect(Instance, *reinterpret_cast<TArray<TNotNull<const UFaerieItemContainerBase*>>*>(&FoundContainers), Class);
		}
	}
	else
#endif
	{
		auto& EntityManager = ItemData::GetFaerieEntityManagerChecked();
		if (Recursive)
		{
			SubObject::GetContainersInInstanceRecursive(EntityManager, Instance, *reinterpret_cast<TArray<TNotNull<UFaerieItemContainerBase*>>*>(&FoundContainers), Class);
		}
		else
		{
			SubObject::GetContainersInInstanceDirect(EntityManager, Instance, FoundContainers, Class);
		}
	}
}

void UFaerieStorageLibrary::GetAllContainersInItem(const FFaerieItemProxy& Proxy, TArray<UFaerieItemContainerBase*>& FoundContainers, const bool Recursive)
{
	if (!Proxy.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Proxy passed to UFaerieStorageLibrary::GetAllContainersInItem"), ELogVerbosity::Error);
		return;
	}

	auto InstanceOpt = Proxy.GetItemInstance();
	if (!InstanceOpt.IsSet())
	{
		return;
	}

	auto Instance = InstanceOpt.GetValue();
	if (!Instance.IsMutable()) return;

#if WITH_EDITOR
	if (!ItemData::HasFaerieEntityManagerBeenAssigned())
	{
		if (Recursive)
		{
			SubObject::GetTemplateContainersInInstanceRecursive(Instance, *reinterpret_cast<TArray<TNotNull<const UFaerieItemContainerBase*>>*>(&FoundContainers));
		}
		else
		{
			SubObject::GetTemplateContainersInInstanceDirect(Instance, *reinterpret_cast<TArray<TNotNull<const UFaerieItemContainerBase*>>*>(&FoundContainers));
		}
	}
	else
#endif
	{
		auto& EntityManager = ItemData::GetFaerieEntityManagerChecked();
		if (Recursive)
		{
			SubObject::GetContainersInInstanceRecursive(EntityManager, Instance, *reinterpret_cast<TArray<TNotNull<UFaerieItemContainerBase*>>*>(&FoundContainers));
		}
		else
		{
			SubObject::GetContainersInInstanceDirect(EntityManager, Instance, *reinterpret_cast<TArray<TNotNull<UFaerieItemContainerBase*>>*>(&FoundContainers));
		}
	}
}

void UFaerieStorageLibrary::GetItemChildren(const FFaerieItemProxy& Proxy, TArray<FFaerieItemProxy>& FoundChildren, const bool Recursive)
{
	if (!Proxy.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Proxy passed to UFaerieStorageLibrary::GetItemChildren"), ELogVerbosity::Error);
		return;
	}

	auto InstanceOpt = Proxy.GetItemInstance();
	if (!InstanceOpt.IsSet())
	{
		return;
	}

	auto Instance = InstanceOpt.GetValue();
	if (!Instance.IsMutable()) return;

	if (!ItemData::HasFaerieEntityManagerBeenAssigned())
	{
		FFrame::KismetExecutionMessage(TEXT("No Entity Manager assigned to handle UFaerieStorageLibrary::GetItemChildren"), ELogVerbosity::Error);
		return;
	}

	auto& EntityManager = ItemData::GetFaerieEntityManagerChecked();
	if (Recursive)
	{
		SubObject::GetChildrenInItemRecursive(EntityManager, Instance, FoundChildren);
	}
	else
	{
		SubObject::GetChildrenInItem(EntityManager, Instance, FoundChildren);
	}
}
