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
		return Container::GetItemStackLimit(ItemData::GetFaerieEntityManager(Proxy.ExtractWorld()), Proxy.GetItemInstanceOrInvalid());
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
		.First(ItemData::GetFaerieEntityManager(Storage->GetWorld()), Storage);
}

UFaerieItemContainerBase* UFaerieStorageLibrary::GetOwningContainer(const FFaerieItemProxy& Proxy)
{
	return Cast<UFaerieItemContainerBase>(Proxy.GetItemOwner());
}

bool UFaerieStorageLibrary::FindSubobject(const FFaerieItemProxy& Proxy, const TSubclassOf<UFaerieItemContainerBase> Class,
										  UFaerieItemContainerBase*& FoundContainer, const bool Recursive)
{
	if (!Proxy.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Proxy passed to UFaerieStorageLibrary::FindSubobject"), ELogVerbosity::Error);
		return false;
	}

	const TOptional<FFaerieItemInstance> InstanceOpt = Proxy.GetItemInstance();
	if (!InstanceOpt.IsSet())
	{
		return false;
	}

	const FFaerieItemInstance Instance = InstanceOpt.GetValue();
	if (!Instance.IsMutable()) return false;

	const UWorld* World = Proxy.ExtractWorld();

#if WITH_EDITOR
	if (!ItemData::HasFaerieEntityManagerBeenAssigned(World))
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

		// Return first
		if (!Containers.IsEmpty())
		{
			FoundContainer = const_cast<UFaerieItemContainerBase*>(NotNullGet(Containers[0]));
			return true;
		}
	}
	else
#endif
	{
		FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(World);
		TArray<TNotNull<UFaerieItemContainerBase*>> Containers;
		if (Recursive)
		{
			SubObject::GetContainersInInstanceRecursive(EntityManager, Instance, Containers, Class);
		}
		else
		{
			SubObject::GetContainersInInstanceDirect(EntityManager, Instance, Containers, Class);
		}

		// Return first
		if (!Containers.IsEmpty())
		{
			FoundContainer = Containers[0];
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

	const TOptional<FFaerieItemInstance> InstanceOpt = Proxy.GetItemInstance();
	if (!InstanceOpt.IsSet())
	{
		return;
	}

	const FFaerieItemInstance Instance = InstanceOpt.GetValue();
	if (!Instance.IsMutable()) return;

	const UWorld* World = Proxy.ExtractWorld();

#if WITH_EDITOR
	if (!ItemData::HasFaerieEntityManagerBeenAssigned(World))
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
		FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(World);
		if (Recursive)
		{
			SubObject::GetContainersInInstanceRecursive(EntityManager, Instance, FoundContainers, Class);
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

	const FFaerieItemInstance Instance = InstanceOpt.GetValue();
	if (!Instance.IsMutable()) return;

	const UWorld* World = Proxy.ExtractWorld();

#if WITH_EDITOR
	if (!ItemData::HasFaerieEntityManagerBeenAssigned(World))
	{
		// Blueprint should normally not get access to read-only containers... but only the editor can run this code anyway so its *fine*
		TArray<const UFaerieItemContainerBase*>& ConstContainerArray = reinterpret_cast<TArray<const UFaerieItemContainerBase*>&>(FoundContainers);
		if (Recursive)
		{
			SubObject::GetTemplateContainersInInstanceRecursive(Instance, ConstContainerArray);
		}
		else
		{
			SubObject::GetTemplateContainersInInstanceDirect(Instance, ConstContainerArray);
		}
	}
	else
#endif
	{
		FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(World);
		if (Recursive)
		{
			SubObject::GetContainersInInstanceRecursive<UFaerieItemContainerBase>(EntityManager, Instance, FoundContainers);
		}
		else
		{
			SubObject::GetContainersInInstanceDirect<UFaerieItemContainerBase>(EntityManager, Instance, FoundContainers);
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

	const TOptional<FFaerieItemInstance> InstanceOpt = Proxy.GetItemInstance();
	if (!InstanceOpt.IsSet())
	{
		return;
	}

	const FFaerieItemInstance Instance = InstanceOpt.GetValue();
	if (!Instance.IsMutable()) return;

	const UWorld* World = Proxy.ExtractWorld();

	if (!ItemData::HasFaerieEntityManagerBeenAssigned(World))
	{
		FFrame::KismetExecutionMessage(TEXT("No Entity Manager assigned to handle UFaerieStorageLibrary::GetItemChildren"), ELogVerbosity::Error);
		return;
	}

	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(World);
	if (Recursive)
	{
		SubObject::GetChildrenInItemRecursive(EntityManager, Instance, FoundChildren);
	}
	else
	{
		SubObject::GetChildrenInItem(EntityManager, Instance, FoundChildren);
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
