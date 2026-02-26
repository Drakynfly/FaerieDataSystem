// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieSubObjectFilter.h"
#include "FaerieContainerIterator.h"
#include "FaerieItem.h"
#include "FaerieItemContainerBase.h"

namespace Faerie::SubObject
{
	auto GetItemContainersFilter()
	{
		static const auto ContainerTokenFilter = Token::Filter().ByClass<UFaerieItemContainerToken>().ByMutable();
		return ContainerTokenFilter;
	}

	void GetAllContainersInItem(const TNotNull<UFaerieItem*> Item, TArray<UFaerieItemContainerBase*>& OutContainers)
	{
		for (UFaerieItemContainerToken* Token : GetItemContainersFilter().Iterate(Item))
		{
			OutContainers.Add(Token->GetItemContainer());
		}
	}

	void GetAllContainersInItemRecursive(const TNotNull<UFaerieItem*> Item, TArray<UFaerieItemContainerBase*>& OutContainers)
	{
		TArray<UFaerieItemContainerBase*> Containers;
		GetAllContainersInItem(Item, Containers);
		OutContainers.Append(Containers);
		for (UFaerieItemContainerBase* Container : Containers)
		{
			for (auto It = Container::ItemRange(Container); It; ++It)
			{
				GetAllContainersInItemRecursive(*It, OutContainers);
			}
		}
	}

	TArray<UFaerieItemContainerBase*> GetAllContainersInItem(const TNotNull<UFaerieItem*> Item)
	{
		TArray<UFaerieItemContainerBase*> Containers;
		GetAllContainersInItem(Item, Containers);
		return Containers;
	}

	TArray<UFaerieItemContainerBase*> GetAllContainersInItemRecursive(const TNotNull<UFaerieItem*> Item)
	{
		TArray<UFaerieItemContainerBase*> Containers;
		GetAllContainersInItemRecursive(Item, Containers);
		return Containers;
	}

	void GetChildrenInItem(const TNotNull<UFaerieItem*> Item, TArray<const UFaerieItem*>& OutChildren)
	{
		for (UFaerieItemContainerBase* Container : SubObject::Iterate(Item))
		{
			for (auto It = Container::ConstItemRange(Container); It; ++It)
			{
				OutChildren.Add(*It);
			}
		}
	}

	void GetChildrenInItemRecursive(const TNotNull<UFaerieItem*> Item, TArray<const UFaerieItem*>& OutChildren)
	{
		TArray<const UFaerieItem*> Children;
		GetChildrenInItem(Item, OutChildren);
		OutChildren.Append(OutChildren);
		for (const UFaerieItem* Child : Children)
		{
			if (UFaerieItem* Mutable = Child->MutateCast())
			{
				for (UFaerieItemContainerBase* Container : SubObject::Iterate(Mutable))
				{
					for (auto It = Container::ItemRange(Container); It; ++It)
					{
						GetChildrenInItemRecursive(*It, OutChildren);
					}
				}
			}
		}
	}

	TArray<const UFaerieItem*> GetChildrenInItem(const TNotNull<UFaerieItem*> Item)
	{
		TArray<const UFaerieItem*> Items;
		GetChildrenInItem(Item, Items);
		return Items;
	}

	TArray<const UFaerieItem*> GetChildrenInItemRecursive(const TNotNull<UFaerieItem*> Item)
	{
		TArray<const UFaerieItem*> Items;
		GetChildrenInItemRecursive(Item, Items);
		return Items;
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

	FContainerIterator::FContainerIterator(const TNotNull<UFaerieItem*> Item)
	  : Iterator(GetItemContainersFilter().Iterate(Item)) {}

	FRecursiveContainerIterator::FRecursiveContainerIterator(const TNotNull<UFaerieItem*> Item)
	  : Containers(GetAllContainersInItemRecursive(Item)),
		Iterator(Containers.CreateIterator()) {}
}
