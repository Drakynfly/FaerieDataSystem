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
		ItemData::FOptionalEntityManager EntityManager(Proxy.GetProxyObject());
		return Container::GetItemStackLimit(EntityManager, Proxy->GetItemInstance().GetValue());
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
	for (auto It = Storage::FIterator_AllAddresses(Storage); It; ++It)
	{
		Proxies.Add(const_cast<UFaerieItemStackProxy*>(Storage->GetProxy(*It)));
	}
	return Proxies;
}

FFaerieAddress UFaerieStorageLibrary::QueryFirst(UFaerieItemStorage* Storage, const FFaerieViewPredicate& Filter)
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
		.First(Storage);
}

bool UFaerieStorageLibrary::FindSubobject(UObject* WorldContextObject, const FFaerieItemInstance& Instance, const TSubclassOf<UFaerieItemContainerBase> Class,
	UFaerieItemContainerBase*& FoundContainers, const bool Recursive)
{
	if (!IsValid(WorldContextObject))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid WorldContextObject passed to UFaerieStorageLibrary::FindSubobject"), ELogVerbosity::Error);
		return false;
	}

	if (!Instance.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Instance passed to UFaerieStorageLibrary::FindSubobject"), ELogVerbosity::Error);
		return false;
	}

	if (!Instance.IsMutable()) return false;

#if WITH_EDITOR
	if (WorldContextObject->GetWorld()->IsEditorWorld())
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
		const ItemData::FRequireEntityManager EntityManager(WorldContextObject);
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

void UFaerieStorageLibrary::FindSubObjectsByClass(UObject* WorldContextObject, const FFaerieItemInstance& Instance, const TSubclassOf<UFaerieItemContainerBase> Class,
	TArray<UFaerieItemContainerBase*>& FoundContainers, const bool Recursive)
{
	if (!IsValid(WorldContextObject))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid WorldContextObject passed to UFaerieStorageLibrary::FindSubobject"), ELogVerbosity::Error);
		return;
	}

	if (!Instance.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Instance passed to UFaerieStorageLibrary::FindSubObjectsByClass"), ELogVerbosity::Error);
		return;
	}

	if (!Instance.IsMutable()) return;

#if WITH_EDITOR
	if (WorldContextObject->GetWorld()->IsEditorWorld())
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
		const ItemData::FRequireEntityManager EntityManager(WorldContextObject);
		if (Recursive)
		{
			SubObject::GetContainersInInstanceRecursive(EntityManager, Instance, *reinterpret_cast<TArray<TNotNull<UFaerieItemContainerBase*>>*>(&FoundContainers), Class);
		}
		else
		{
			SubObject::GetContainersInInstanceDirect(EntityManager, Instance, *reinterpret_cast<TArray<TNotNull< UFaerieItemContainerBase*>>*>(&FoundContainers), Class);
		}
	}
}

void UFaerieStorageLibrary::GetAllContainersInItem(UObject* WorldContextObject, const FFaerieItemInstance& Instance, TArray<UFaerieItemContainerBase*>& FoundContainers, const bool Recursive)
{
	if (!IsValid(WorldContextObject))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid WorldContextObject passed to UFaerieStorageLibrary::GetAllContainersInItem"), ELogVerbosity::Error);
		return;
	}

	if (!Instance.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Instance passed to UFaerieStorageLibrary::GetAllContainersInItem"), ELogVerbosity::Error);
		return;
	}

	if (!Instance.IsMutable()) return;

#if WITH_EDITOR
	if (WorldContextObject->GetWorld()->IsEditorWorld())
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
		const ItemData::FRequireEntityManager EntityManager(WorldContextObject);
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

void UFaerieStorageLibrary::GetItemChildren(UObject* WorldContextObject, const FFaerieItemInstance& Instance, TArray<FFaerieItemInstance>& FoundInstances, const bool Recursive)
{
	if (!IsValid(WorldContextObject))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid WorldContextObject passed to UFaerieStorageLibrary::GetItemChildren"), ELogVerbosity::Error);
		return;
	}

	if (!Instance.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Instance passed to UFaerieStorageLibrary::GetItemChildren"), ELogVerbosity::Error);
		return;
	}

	if (!Instance.IsMutable()) return;

	static_assert(sizeof(FFaerieItemInstance) == sizeof(ItemData::FReference));

	const ItemData::FRequireEntityManager EntityManager(WorldContextObject);

	if (Recursive)
	{
		SubObject::GetChildrenInItemRecursive(EntityManager, Instance, *reinterpret_cast<TArray<ItemData::FReference>*>(&FoundInstances));
	}
	else
	{
		SubObject::GetChildrenInItem(EntityManager, Instance, *reinterpret_cast<TArray<ItemData::FReference>*>(&FoundInstances));
	}
}
