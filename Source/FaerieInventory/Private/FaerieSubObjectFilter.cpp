// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieSubObjectFilter.h"
#include "FaerieContainerIterator.h"
#include "FaerieInventoryLog.h"
#include "FaerieItem.h"
#include "FaerieItemStackContainer.h"

#include "Fragments/FaerieItemStorageFragment.h"

namespace Faerie::SubObject
{
	void GetTemplateContainersInInstanceDirect(const TValid<const FFaerieItemInstance&> Item, TArray<TNotNull<const UFaerieItemContainerBase*>>& Containers, const TNotNull<const UClass*> Class)
	{
		const UFaerieItem* ItemAsset = ValidGet(Item).GetItemPtr();
		auto StacksFragment = ItemData::GetDefaultFragment<FFaerieChildStackFragment>(ItemAsset);
		auto StorageFragment = ItemData::GetDefaultFragment<FFaerieItemStorageFragment>(ItemAsset);

		if (StacksFragment.IsValid())
		{
			for (auto&& Slot : StacksFragment->Slots)
			{
				if (IsValid(Slot.Stack) && Slot.Stack.IsA(Class))
				{
					Containers.Add(Slot.Stack);
				}
			}
		}

		if (StorageFragment.IsValid())
		{
			const TObjectPtr<UFaerieItemStorage>& ItemStorage = StorageFragment->Storage.Storage;
			if (IsValid(ItemStorage) && ItemStorage.IsA(Class))
			{
				Containers.Add(ItemStorage);
			}
			else
			{
				UE_LOG(LogFaerieInventory, Error, TEXT("Storage invalid in ItemStorageFragment. This fragment should not contain a null container!"))
			}
		}
	}

	void GetTemplateContainersInInstanceRecursive(const TValid<const FFaerieItemInstance&> Item,
		TArray<TNotNull<const UFaerieItemContainerBase*>>& Containers, const TNotNull<const UClass*> Class)
	{
		const int32 CountBeforeThisRecursion = Containers.Num();
		GetTemplateContainersInInstanceDirect(Item, Containers, Class);
		const int32 CountAfterThisRecursion = Containers.Num();
		for (int32 i = CountBeforeThisRecursion; i < CountAfterThisRecursion; ++i)
		{
			for (auto It = Container::ItemRange(Containers[i]); It; ++It)
			{
				GetTemplateContainersInInstanceRecursive(*It, Containers, Class);
			}
		}
	}

	void GetContainersInInstanceDirect(const FMassEntityManager& EntityManager, const TValid<const FFaerieItemInstance&> Item, TArray<TNotNull<UFaerieItemContainerBase*>>& Containers, const TNotNull<const UClass*> Class)
	{
		const FMassEntityHandle Entity = ValidGet(Item).GetMassEntityHandle();
		auto* StacksFragment = ItemData::GetEntityFragment<FFaerieChildStackFragment>(EntityManager, Entity);
		auto* StorageFragment = ItemData::GetEntityFragment<FFaerieItemStorageFragment>(EntityManager, Entity);

		if (StacksFragment)
		{
			for (auto&& Slot : StacksFragment->Slots)
			{
				if (IsValid(Slot.Stack) && Slot.Stack.IsA(Class))
				{
					Containers.Add(Slot.Stack);
				}
			}
		}

		if (StorageFragment)
		{
			const TObjectPtr<UFaerieItemStorage>& ItemStorage = StorageFragment->Storage.Storage;
			if (IsValid(ItemStorage) && ItemStorage.IsA(Class))
			{
				Containers.Add(ItemStorage);
			}
			else
			{
				UE_LOG(LogFaerieInventory, Error, TEXT("Storage invalid in ItemStorageFragment. This fragment should not contain a null container!"))
			}
		}
	}

	void GetContainersInInstanceRecursive(const FMassEntityManager& EntityManager,
		const TValid<const FFaerieItemInstance&> Item, TArray<TNotNull<UFaerieItemContainerBase*>>& Containers,
		const TNotNull<const UClass*> Class)
	{
		const int32 CountBeforeThisRecursion = Containers.Num();
		GetContainersInInstanceDirect(EntityManager, Item, Containers, Class);
		const int32 CountAfterThisRecursion = Containers.Num();
		for (int32 i = CountBeforeThisRecursion; i < CountAfterThisRecursion; ++i)
		{
			for (auto It = Container::ItemRange(Containers[i]); It; ++It)
			{
				GetContainersInInstanceRecursive(EntityManager, *It, Containers, Class);
			}
		}
	}

	void GetChildrenInItem(const FMassEntityManager& EntityManager, const TValid<const FFaerieItemInstance&> Item, TArray<FFaerieItemInstance>& OutInstances)
	{
		for (UFaerieItemContainerBase* Container : SubObject::Iterate(EntityManager, Item))
		{
			for (auto It = Container::ItemRange(Container); It; ++It)
			{
				OutInstances.Add(*It);
			}
		}
	}

	void GetChildrenInItemRecursive(const FMassEntityManager& EntityManager, const TValid<const FFaerieItemInstance&> Item, TArray<FFaerieItemInstance>& OutInstances)
	{
		const int32 CountBeforeThisRecursion = OutInstances.Num();
		GetChildrenInItem(EntityManager, Item, OutInstances);
		const int32 CountAfterThisRecursion = OutInstances.Num();
		for (int32 i = CountBeforeThisRecursion; i < CountAfterThisRecursion; ++i)
		{
			auto&& Child = OutInstances[i];

			if (Child.IsMutable())
			{
				for (UFaerieItemContainerBase* Container : SubObject::Iterate(EntityManager, Child))
				{
					for (auto It = Container::MutableItemRange(Container); It; ++It)
					{
						GetChildrenInItemRecursive(EntityManager, *It, OutInstances);
					}
				}
			}
		}
	}

	namespace StaticPredicates
	{
		bool ClassEquals(const TNotNull<const UFaerieItemContainerBase*> Container, const TSubclassOf<UFaerieItemContainerBase>& Class)
		{
			return Class == Container->GetClass();
		}

		bool ClassEqualsOrChildOf(const TNotNull<const UFaerieItemContainerBase*> Container, const TSubclassOf<UFaerieItemContainerBase>& Class)
		{
			return Class->IsChildOf(Container->GetClass());
		}
	}

	static TArray<TNotNull<UFaerieItemContainerBase*>> GetAllContainersInItem_Inline(const FMassEntityManager& EntityManager, const TValid<const FFaerieItemInstance&> Item)
	{
		TArray<TNotNull<UFaerieItemContainerBase*>> Containers;
		GetContainersInInstanceDirect(EntityManager, Item, Containers);
		return Containers;
	}

	static TArray<TNotNull<UFaerieItemContainerBase*>> GetAllContainersInItemRecursive_Inline(const FMassEntityManager& EntityManager, const TValid<const FFaerieItemInstance&> Item)
	{
		TArray<TNotNull<UFaerieItemContainerBase*>> Containers;
		GetContainersInInstanceRecursive(EntityManager, Item, Containers);
		return Containers;
	}

	FContainerIterator::FContainerIterator(const FMassEntityManager& EntityManager, const TValid<const FFaerieItemInstance&> Item)
	  : Containers(GetAllContainersInItem_Inline(EntityManager, Item)),
		Iterator(Containers.CreateIterator()) {}

	void FContainerIterator::operator++()
	{
		do
		{
			++Iterator;
		}
		while (Iterator && !IsValid(NotNullGet(Iterator.operator*())));
	}

	FRecursiveContainerIterator::FRecursiveContainerIterator(const FMassEntityManager& EntityManager, const TValid<const FFaerieItemInstance&> Item)
	  : Containers(GetAllContainersInItemRecursive_Inline(EntityManager, Item)),
		Iterator(Containers.CreateIterator()) {}
}
