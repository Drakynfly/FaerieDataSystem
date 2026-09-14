// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieSubObjectFilter.h"
#include "FaerieContainerIterator.h"
#include "FaerieInventoryLog.h"
#include "FaerieItem.h"
#include "FaerieItemStackContainer.h"

#include "Fragments/FaerieItemStorageFragment.h"

namespace Faerie::SubObject
{
	void GetTemplateContainersInInstanceDirect(const FFaerieItemInstance& Item, const TAdderRef<TNotNull<const UFaerieItemContainerBase*>> Containers, const TNotNull<const UClass*> Class)
	{
		const UFaerieItem* ItemAsset = Item.GetItemPtr();

		if (auto&& StacksFragment = ItemData::GetDefaultFragment<FFaerieChildStackFragment>(ItemAsset);
			StacksFragment.IsValid())
		{
			for (auto&& Slot : StacksFragment->Slots)
			{
				if (IsValid(Slot.Stack) && Slot.Stack.IsA(Class))
				{
					Containers.Add(Slot.Stack);
				}
			}
		}

		if (auto&& StorageFragment = ItemData::GetDefaultFragment<FFaerieItemStorageFragment>(ItemAsset);
			StorageFragment.IsValid())
		{
			const TObjectPtr<UFaerieItemStorage>& ItemStorage = StorageFragment->Storage.Storage;
			if (!IsValid(ItemStorage))
			{
				UE_LOGF(LogFaerieInventory, Error, "Storage invalid in ItemStorageFragment. This fragment should not contain a null container!")
			}
			else
			{
				if (ItemStorage.IsA(Class))
				{
					Containers.Add(ItemStorage);
				}
			}
		}
	}

	void GetTemplateContainersInInstanceRecursive(const FFaerieItemInstance& Item, const TAdderRef<TNotNull<const UFaerieItemContainerBase*>> Containers, const TNotNull<const UClass*> Class)
	{
		/*
		 * Most item instances will not contain more than 3 containers. That allows 1 storage and 2 slots or 3 slots,
		 * which is more than most will have.
		 */
		TArray<TNotNull<const UFaerieItemContainerBase*>, TInlineAllocator<3>> Children;
		GetTemplateContainersInInstanceDirect(Item, Children, Class);
		for (auto&& Child : Children)
		{
			Containers.Add(Child);
			for (auto It = Container::ItemRange(Child); It; ++It)
			{
				GetTemplateContainersInInstanceRecursive(*It, Containers, Class);
			}
		}
	}

	bool HasContainerInInstanceDirect_Stack(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, const TNotNull<const UFaerieItemStackContainer*> TestContainer)
	{
		const FMassEntityHandle Entity = Item.GetMassEntityHandle();
		if (!EntityManager.IsEntityValid(Entity))
		{
			return false;
		}

		if (auto* StacksFragment = ItemData::GetEntityFragment<FFaerieChildStackFragment>(EntityManager, Entity))
		{
			for (auto&& Slot : StacksFragment->Slots)
			{
				if (IsValid(Slot.Stack) && Slot.Stack == TestContainer)
				{
					return true;
				}
			}
		}

		return false;
	}

	bool HasContainerInInstanceDirect_Storage(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, const TNotNull<const UFaerieItemStorage*> TestContainer)
	{
		const FMassEntityHandle Entity = Item.GetMassEntityHandle();
		if (!EntityManager.IsEntityValid(Entity))
		{
			return false;
		}

		if (auto* StorageFragment = ItemData::GetEntityFragment<FFaerieItemStorageFragment>(EntityManager, Entity))
		{
			const TObjectPtr<UFaerieItemStorage>& ItemStorage = StorageFragment->Storage.Storage;
			if (!IsValid(ItemStorage))
			{
				UE_LOGF(LogFaerieInventory, Error, "Storage invalid in ItemStorageFragment. This fragment should not contain a null container!")
			}
			else
			{
				if (ItemStorage == TestContainer)
				{
					return true;
				}
			}
		}

		return false;
	}

	bool HasContainerInInstanceRecursive_Stack(const FMassEntityManager& EntityManager,
		const FFaerieItemInstance& Item, const TNotNull<const UFaerieItemStackContainer*> TestContainer)
	{
		const FMassEntityHandle Entity = Item.GetMassEntityHandle();
		if (!EntityManager.IsEntityValid(Entity))
		{
			return false;
		}

		if (auto* StacksFragment = ItemData::GetEntityFragment<FFaerieChildStackFragment>(EntityManager, Entity))
		{
			for (auto&& Slot : StacksFragment->Slots)
			{
				if (IsValid(Slot.Stack) && Slot.Stack == TestContainer)
				{
					return true;
				}

				for (auto It = Container::MutableItemRange(Slot.Stack); It; ++It)
				{
					if (HasContainerInInstanceRecursive_Stack(EntityManager, *It, TestContainer))
					{
						return true;
					}
				}
			}
		}
		return false;
	}

	bool HasContainerInInstanceRecursive_Storage(const FMassEntityManager& EntityManager,
		const FFaerieItemInstance& Item, const TNotNull<const UFaerieItemStorage*> TestContainer)
	{
		const FMassEntityHandle Entity = Item.GetMassEntityHandle();
		if (!EntityManager.IsEntityValid(Entity))
		{
			return false;
		}

		if (auto* StorageFragment = ItemData::GetEntityFragment<FFaerieItemStorageFragment>(EntityManager, Entity))
		{
			const TObjectPtr<UFaerieItemStorage>& ItemStorage = StorageFragment->Storage.Storage;
			if (!IsValid(ItemStorage))
			{
				UE_LOGF(LogFaerieInventory, Error, "Storage invalid in ItemStorageFragment. This fragment should not contain a null container!")
			}
			else
			{
				if (ItemStorage == TestContainer)
				{
					return true;
				}

				for (auto It = Container::MutableItemRange(ItemStorage); It; ++It)
				{
					if (HasContainerInInstanceRecursive_Storage(EntityManager, *It, TestContainer))
					{
						return true;
					}
				}
			}
		}

		return false;
	}

	void GetContainersInInstanceDirect_All(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, TAdderRef<TNotNull<UFaerieItemContainerBase*>> Containers)
	{
		const FMassEntityHandle Entity = Item.GetMassEntityHandle();
		if (!EntityManager.IsEntityValid(Entity))
		{
			return;
		}

		if (auto* StacksFragment = ItemData::GetEntityFragment<FFaerieChildStackFragment>(EntityManager, Entity))
		{
			for (auto&& Slot : StacksFragment->Slots)
			{
				if (IsValid(Slot.Stack))
				{
					Containers.Add(Slot.Stack.Get());
				}
			}
		}

		if (auto* StorageFragment = ItemData::GetEntityFragment<FFaerieItemStorageFragment>(EntityManager, Entity))
		{
			const TObjectPtr<UFaerieItemStorage>& ItemStorage = StorageFragment->Storage.Storage;
			if (!IsValid(ItemStorage))
			{
				UE_LOGF(LogFaerieInventory, Error, "Storage invalid in ItemStorageFragment. This fragment should not contain a null container!")
			}
			else
			{
				Containers.Add(ItemStorage.Get());
			}
		}
	}

	void GetContainersInInstanceDirect_Stack(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, TAdderRef<TNotNull<UFaerieItemStackContainer*>> Containers)
	{
		const FMassEntityHandle Entity = Item.GetMassEntityHandle();
		if (!EntityManager.IsEntityValid(Entity))
		{
			return;
		}

		if (auto* StacksFragment = ItemData::GetEntityFragment<FFaerieChildStackFragment>(EntityManager, Entity))
		{
			for (auto&& Slot : StacksFragment->Slots)
			{
				if (IsValid(Slot.Stack))
				{
					Containers.Add(Slot.Stack);
				}
			}
		}
	}

	void GetContainersInInstanceDirect_Storage(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, TAdderRef<TNotNull<UFaerieItemStorage*>> Containers)
	{
		const FMassEntityHandle Entity = Item.GetMassEntityHandle();
		if (!EntityManager.IsEntityValid(Entity))
		{
			return;
		}

		if (auto* StorageFragment = ItemData::GetEntityFragment<FFaerieItemStorageFragment>(EntityManager, Entity))
		{
			const TObjectPtr<UFaerieItemStorage>& ItemStorage = StorageFragment->Storage.Storage;
			if (!IsValid(ItemStorage))
			{
				UE_LOGF(LogFaerieInventory, Error, "Storage invalid in ItemStorageFragment. This fragment should not contain a null container!")
			}
			else
			{
				Containers.Add(ItemStorage);
			}
		}
	}

	void GetContainersInInstanceRecursive_All(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, TAdderRef<TNotNull<UFaerieItemContainerBase*>> Containers)
	{
		/*
		 * Most item instances will not contain more than 3 containers. That allows 1 storage and 2 slots or 3 slots,
		 * which is more than most will have.
		 */
		TArray<TNotNull<UFaerieItemContainerBase*>, TInlineAllocator<3>> Children;
		GetContainersInInstanceDirect_All(EntityManager, Item, Children);
		for (auto&& Child : Children)
		{
			Containers.Add(Child);
			for (auto It = Container::ItemRange(Child); It; ++It)
			{
				GetContainersInInstanceRecursive_Stack(EntityManager, *It, Containers);
			}
		}
	}

	void GetContainersInInstanceRecursive_Stack(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, TAdderRef<TNotNull<UFaerieItemStackContainer*>> Containers)
	{
		/*
		 * Most item instances will not contain more than 3 containers.
		 */
		TArray<TNotNull<UFaerieItemStackContainer*>, TInlineAllocator<3>> Children;
		GetContainersInInstanceDirect_Stack(EntityManager, Item, Children);
		for (auto&& Child : Children)
		{
			Containers.Add(Child);
			for (auto It = Container::ItemRange(Child); It; ++It)
			{
				GetContainersInInstanceRecursive_Stack(EntityManager, *It, Containers);
			}
		}
	}

	void GetContainersInInstanceRecursive_Storage(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, TAdderRef<TNotNull<UFaerieItemStorage*>> Containers)
	{
		/*
		 * Most item instances will not contain more than 1 storage.
		 */
		TArray<TNotNull<UFaerieItemStorage*>, TInlineAllocator<1>> Children;
		GetContainersInInstanceDirect_Storage(EntityManager, Item, Children);
		for (auto&& Child : Children)
		{
			Containers.Add(Child);
			for (auto It = Container::ItemRange(Child); It; ++It)
			{
				GetContainersInInstanceRecursive_Storage(EntityManager, *It, Containers);
			}
		}
	}

	void GetContainersInInstanceDirect(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item,
		TAdderRef<TNotNull<UFaerieItemContainerBase*>> Containers, const TNotNull<const UClass*> Class)
	{
		if (Class == UFaerieItemContainerBase::StaticClass())
		{
			GetContainersInInstanceDirect_All(EntityManager, Item, Containers);
		}
		else if (Class == UFaerieItemStackContainer::StaticClass())
		{
			GetContainersInInstanceDirect_Stack(EntityManager, Item, Containers);
		}
		else if (Class == UFaerieItemStorage::StaticClass())
		{
			GetContainersInInstanceDirect_Storage(EntityManager, Item, Containers);
		}
	}

	void GetContainersInInstanceRecursive(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, TAdderRef<TNotNull<UFaerieItemContainerBase*>> Containers, const TNotNull<const UClass*> Class)
	{
		if (Class == UFaerieItemContainerBase::StaticClass())
		{
			GetContainersInInstanceRecursive_All(EntityManager, Item, Containers);
		}
		else if (Class == UFaerieItemStackContainer::StaticClass())
		{
			GetContainersInInstanceRecursive_Stack(EntityManager, Item, Containers);
		}
		else if (Class == UFaerieItemStorage::StaticClass())
		{
			GetContainersInInstanceRecursive_Storage(EntityManager, Item, Containers);
		}
	}

	void GetChildrenInItem(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, const TAdderRef<FFaerieItemProxy> OutProxies)
	{
		for (UFaerieItemContainerBase* Container : SubObject::Iterate(EntityManager, Item))
		{
			for (auto It = Container::ItemRange(Container); It; ++It)
			{
				OutProxies.Add(It.GetPersistentProxy());
			}
		}
	}

	void GetChildrenInItemRecursive(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, const TAdderRef<FFaerieItemProxy> OutProxies)
	{
		TArray<FFaerieItemProxy> Children;
		GetChildrenInItem(EntityManager, Item, Children);
		for (auto&& Child : Children)
		{
			OutProxies.Add(Child);

			const FFaerieItemInstance Instance = Child.GetItemInstanceOrInvalid();
			if (Instance.IsMutable())
			{
				for (UFaerieItemContainerBase* Container : SubObject::Iterate(EntityManager, Instance))
				{
					for (auto It = Container::MutableItemRange(Container); It; ++It)
					{
						GetChildrenInItemRecursive(EntityManager, *It, OutProxies);
					}
				}
			}
		}
	}

	static TArray<TNotNull<UFaerieItemContainerBase*>> GetAllContainersInItem_Inline(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item)
	{
		TArray<TNotNull<UFaerieItemContainerBase*>> Containers;
		GetContainersInInstanceDirect<UFaerieItemContainerBase>(EntityManager, Item, Containers);
		return Containers;
	}

	static TArray<TNotNull<UFaerieItemContainerBase*>> GetAllContainersInItemRecursive_Inline(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item)
	{
		TArray<TNotNull<UFaerieItemContainerBase*>> Containers;
		GetContainersInInstanceRecursive<UFaerieItemContainerBase>(EntityManager, Item, Containers);
		return Containers;
	}

	FContainerIterator::FContainerIterator(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item)
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

	FRecursiveContainerIterator::FRecursiveContainerIterator(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item)
	  : Containers(GetAllContainersInItemRecursive_Inline(EntityManager, Item)),
		Iterator(Containers.CreateIterator()) {}
}
