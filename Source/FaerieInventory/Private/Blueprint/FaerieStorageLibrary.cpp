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
	const TOptional<FFaerieItemInstance> InstanceOpt = Proxy.GetItemInstance();
	if (!InstanceOpt.IsSet())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Proxy passed to UFaerieStorageLibrary::GetItemStackLimit"), ELogVerbosity::Error);
		return 0;
	}

	return Container::GetItemStackLimit(Proxy.GetEntityManager().Get(), InstanceOpt.GetValue());
}

bool UFaerieStorageLibrary::GetNetworkHandleFromProxy(const FFaerieItemProxy& Proxy, FFaerieItemNetworkHandle& OutHandle)
{
	if (Proxy.HasValidInstance())
	{
		OutHandle = FFaerieItemNetworkHandle::FromProxy(Proxy);
	}
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
		.First(ItemData::GetFaerieEntityManager(Storage->GetWorld()), Storage);
}

UFaerieItemContainerBase* UFaerieStorageLibrary::GetOwningContainer(const FFaerieItemProxy& Proxy)
{
	if (!Proxy.HasEntityManager())
	{
		FFrame::KismetExecutionMessage(TEXT("No Entity Manager assigned to handle UFaerieStorageLibrary::GetOwningContainer"), ELogVerbosity::Error);
		return nullptr;
	}
	return Container::GetItemOwner(Proxy);
}

bool UFaerieStorageLibrary::FindSubobject(const FFaerieItemProxy& Proxy, const TSubclassOf<UFaerieItemContainerBase> Class,
										  UFaerieItemContainerBase*& FoundContainer, const bool Recursive)
{
	const TOptional<FFaerieItemInstance> InstanceOpt = Proxy.GetItemInstance();
	if (!InstanceOpt.IsSet())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Proxy passed to UFaerieStorageLibrary::FindSubobject"), ELogVerbosity::Error);
		return false;
	}

	const FFaerieItemInstance Instance = InstanceOpt.GetValue();
	if (!Instance.IsMutable()) return false;

	if (const TSharedPtr<FMassEntityManager>& EntityManager = Proxy.GetEntityManager())
	{
		TArray<TNotNull<UFaerieItemContainerBase*>> Containers;
		if (Recursive)
		{
			SubObject::GetContainersInInstanceRecursive(*EntityManager.Get(), Instance.GetMassEntityHandle(), Containers, Class);
		}
		else
		{
			SubObject::GetContainersInInstanceDirect(*EntityManager.Get(), Instance.GetMassEntityHandle(), Containers, Class);
		}

		// Return first
		if (!Containers.IsEmpty())
		{
			FoundContainer = Containers[0];
			return true;
		}
	}
	else
	{
		TArray<TNotNull<const UFaerieItemContainerBase*>> Containers;
		if (const UFaerieItem* ItemAsset = Instance.GetItemPtr())
		{
			if (Recursive)
			{
				SubObject::GetTemplateContainersInInstanceRecursive(ItemAsset, Containers, Class);
			}
			else
			{
				SubObject::GetTemplateContainersInInstanceDirect(ItemAsset, Containers, Class);
			}
		}

		// Return first
		if (!Containers.IsEmpty())
		{
			FoundContainer = const_cast<UFaerieItemContainerBase*>(NotNullGet(Containers[0]));
			return true;
		}
	}

	return false;
}

void UFaerieStorageLibrary::FindSubObjectsByClass(const FFaerieItemProxy& Proxy, const TSubclassOf<UFaerieItemContainerBase> Class,
	TArray<UFaerieItemContainerBase*>& FoundContainers, const bool Recursive)
{
	const TOptional<FFaerieItemInstance> InstanceOpt = Proxy.GetItemInstance();
	if (!InstanceOpt.IsSet())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Proxy passed to UFaerieStorageLibrary::FindSubObjectsByClass"), ELogVerbosity::Error);
		return;
	}

	const FFaerieItemInstance Instance = InstanceOpt.GetValue();
	if (!Instance.IsMutable()) return;

	if (const TSharedPtr<FMassEntityManager>& EntityManager = Proxy.GetEntityManager())
	{
		if (Recursive)
		{
			SubObject::GetContainersInInstanceRecursive(*EntityManager.Get(), Instance.GetMassEntityHandle(), FoundContainers, Class);
		}
		else
		{
			SubObject::GetContainersInInstanceDirect(*EntityManager.Get(), Instance.GetMassEntityHandle(), FoundContainers, Class);
		}
	}
	else
	{
		if (const UFaerieItem* ItemAsset = Instance.GetItemPtr())
		{
			if (Recursive)
			{
				SubObject::GetTemplateContainersInInstanceRecursive(ItemAsset, *reinterpret_cast<TArray<TNotNull<const UFaerieItemContainerBase*>>*>(&FoundContainers), Class);
			}
			else
			{
				SubObject::GetTemplateContainersInInstanceDirect(ItemAsset, *reinterpret_cast<TArray<TNotNull<const UFaerieItemContainerBase*>>*>(&FoundContainers), Class);
			}
		}
	}
}

void UFaerieStorageLibrary::GetAllContainersInItem(const FFaerieItemProxy& Proxy, TArray<UFaerieItemContainerBase*>& FoundContainers, const bool Recursive)
{
	TOptional<FFaerieItemInstance> InstanceOpt = Proxy.GetItemInstance();
	if (!InstanceOpt.IsSet())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Proxy passed to UFaerieStorageLibrary::GetAllContainersInItem"), ELogVerbosity::Error);
		return;
	}

	const FFaerieItemInstance Instance = InstanceOpt.GetValue();
	if (!Instance.IsMutable()) return;

	if (const TSharedPtr<FMassEntityManager>& EntityManager = Proxy.GetEntityManager())
	{
		if (Recursive)
		{
			SubObject::GetContainersInInstanceRecursive<UFaerieItemContainerBase>(*EntityManager.Get(), Instance.GetMassEntityHandle(), FoundContainers);
		}
		else
		{
			SubObject::GetContainersInInstanceDirect<UFaerieItemContainerBase>(*EntityManager.Get(), Instance.GetMassEntityHandle(), FoundContainers);
		}
	}
	else
	{
		if (const UFaerieItem* ItemAsset = Instance.GetItemPtr())
		{
			// Blueprint should normally not get access to read-only containers... but only the editor can run this code anyway so its *fine*
			TArray<const UFaerieItemContainerBase*>& ConstContainerArray = reinterpret_cast<TArray<const UFaerieItemContainerBase*>&>(FoundContainers);
			if (Recursive)
			{
				SubObject::GetTemplateContainersInInstanceRecursive(ItemAsset, ConstContainerArray);
			}
			else
			{
				SubObject::GetTemplateContainersInInstanceDirect(ItemAsset, ConstContainerArray);
			}
		}
	}
}

void UFaerieStorageLibrary::GetItemChildren(const FFaerieItemProxy& Proxy, TArray<FFaerieItemProxy>& FoundChildren, const bool Recursive)
{
	const TOptional<FFaerieItemInstance> InstanceOpt = Proxy.GetItemInstance();
	if (!InstanceOpt.IsSet())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Proxy passed to UFaerieStorageLibrary::GetItemChildren"), ELogVerbosity::Error);
		return;
	}

	const FFaerieItemInstance Instance = InstanceOpt.GetValue();

	// Only mutable instances have children, so we can early-out it it's not.
	if (!Instance.IsMutable()) return;

	const TSharedPtr<FMassEntityManager>& EntityManager = Proxy.GetEntityManager();
	if (!EntityManager)
	{
		FFrame::KismetExecutionMessage(TEXT("No Entity Manager assigned to handle UFaerieStorageLibrary::GetItemChildren"), ELogVerbosity::Error);
		return;
	}

	if (Recursive)
	{
		SubObject::GetChildrenInItemRecursive(*EntityManager.Get(), Instance.GetMassEntityHandle(), FoundChildren);
	}
	else
	{
		SubObject::GetChildrenInItem(*EntityManager.Get(), Instance.GetMassEntityHandle(), FoundChildren);
	}
}

bool UFaerieStorageLibrary::FindExtension(const UFaerieItemContainerBase* Container, UScriptStruct* ExtensionType,
	FInstancedStruct& FoundExtension, const bool RecurseParents)
{
	if (!IsValid(Container))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Container passed to UFaerieStorageLibrary::FindExtension"), ELogVerbosity::Error);
		return false;
	}

	if (!IsValid(ExtensionType))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid ExtensionType passed to UFaerieStorageLibrary::FindExtension"), ELogVerbosity::Error);
		return false;
	}

	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(Container->GetWorld());
	FoundExtension = Container->ReadContainerData(EntityManager, ExtensionType, RecurseParents);
	return FoundExtension.IsValid();
}
