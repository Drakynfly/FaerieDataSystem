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

	void GetTemplateContainersInInstanceRecursive(const FFaerieItemInstance& Item,
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

	void GetContainersInInstanceDirect(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, const TAdderRef<TNotNull<UFaerieItemContainerBase*>> Containers, const TNotNull<const UClass*> Class)
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
				if (IsValid(Slot.Stack) && Slot.Stack.IsA(Class))
				{
					Containers.Add(Slot.Stack);
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
				if (ItemStorage.IsA(Class))
				{
					Containers.Add(ItemStorage);
				}
			}
		}
	}

	void GetContainersInInstanceRecursive(const FMassEntityManager& EntityManager,
		const FFaerieItemInstance& Item, TArray<TNotNull<UFaerieItemContainerBase*>>& Containers,
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

	template <typename TFaerieItemContainerBase>
	bool HasContainerInInstanceDirect(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item,
		const TNotNull<const TFaerieItemContainerBase*> TestContainer)
	{
		const FMassEntityHandle Entity = Item.GetMassEntityHandle();
		if (!EntityManager.IsEntityValid(Entity))
		{
			return false;
		}

		if constexpr (std::is_base_of_v<UFaerieItemStackContainer, TFaerieItemContainerBase>)
		{
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
		}
		if constexpr (std::is_base_of_v<UFaerieItemStorage, TFaerieItemContainerBase>)
		{
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
		}

		return false;
	}

	template bool FAERIEINVENTORY_API HasContainerInInstanceDirect(const FMassEntityManager&, const FFaerieItemInstance&, const TNotNull<const UFaerieItemStackContainer*>);
	template bool FAERIEINVENTORY_API HasContainerInInstanceDirect(const FMassEntityManager&, const FFaerieItemInstance&, const TNotNull<const UFaerieItemStorage*>);

	template <typename TFaerieItemContainerBase>
	bool HasContainerInInstanceRecursive(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item,
		const TNotNull<const TFaerieItemContainerBase*> TestContainer)
	{
		struct FLocal
		{
			static bool HasContainerInInstanceRecursive_Stack(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, const TNotNull<const UFaerieItemContainerBase*> TestContainer)
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

			static bool HasContainerInInstanceRecursive_Storage(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, const TNotNull<const UFaerieItemContainerBase*> TestContainer)
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
		};

		if constexpr (std::is_base_of_v<UFaerieItemStackContainer, TFaerieItemContainerBase>)
		{
			return FLocal::HasContainerInInstanceRecursive_Stack(EntityManager, Item, TestContainer);
		}
		if constexpr (std::is_base_of_v<UFaerieItemStorage, TFaerieItemContainerBase>)
		{
			return FLocal::HasContainerInInstanceRecursive_Storage(EntityManager, Item, TestContainer);
		}
	}

	template bool FAERIEINVENTORY_API HasContainerInInstanceRecursive(const FMassEntityManager&, const FFaerieItemInstance&, const TNotNull<const UFaerieItemStackContainer*>);
	template bool FAERIEINVENTORY_API HasContainerInInstanceRecursive(const FMassEntityManager&, const FFaerieItemInstance&, const TNotNull<const UFaerieItemStorage*>);

	void GetChildrenInItem(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, TArray<FFaerieItemProxy>& OutProxies)
	{
		for (UFaerieItemContainerBase* Container : SubObject::Iterate(EntityManager, Item))
		{
			for (auto It = Container::ItemRange(Container); It; ++It)
			{
				OutProxies.Add(It.GetPersistentProxy());
			}
		}
	}

	void GetChildrenInItemRecursive(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, TArray<FFaerieItemProxy>& OutProxies)
	{
		const int32 CountBeforeThisRecursion = OutProxies.Num();
		GetChildrenInItem(EntityManager, Item, OutProxies);
		const int32 CountAfterThisRecursion = OutProxies.Num();
		for (int32 i = CountBeforeThisRecursion; i < CountAfterThisRecursion; ++i)
		{
			auto&& Child = OutProxies[i].GetItemInstanceOrInvalid();

			if (Child.IsMutable())
			{
				for (UFaerieItemContainerBase* Container : SubObject::Iterate(EntityManager, Child))
				{
					for (auto It = Container::MutableItemRange(Container); It; ++It)
					{
						GetChildrenInItemRecursive(EntityManager, *It, OutProxies);
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

	static TArray<TNotNull<UFaerieItemContainerBase*>> GetAllContainersInItem_Inline(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item)
	{
		TArray<TNotNull<UFaerieItemContainerBase*>> Containers;
		GetContainersInInstanceDirect(EntityManager, Item, Containers);
		return Containers;
	}

	static TArray<TNotNull<UFaerieItemContainerBase*>> GetAllContainersInItemRecursive_Inline(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item)
	{
		TArray<TNotNull<UFaerieItemContainerBase*>> Containers;
		GetContainersInInstanceRecursive(EntityManager, Item, Containers);
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
