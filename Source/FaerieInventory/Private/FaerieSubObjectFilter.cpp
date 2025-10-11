// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieSubObjectFilter.h"
#include "FaerieContainerFilter.h"
#include "FaerieContainerFilterTypes.h"
#include "FaerieItem.h"
#include "FaerieItemContainerBase.h"
#include "FaerieItemTokenFilter.h"
#include "FaerieItemTokenFilterTypes.h"
#include "Tokens/FaerieItemStorageToken.h"

namespace Faerie
{
	void GetAllContainersInItem(UFaerieItem* Item, TArray<UFaerieItemContainerBase*>& OutContainers)
	{
		if (!ensure(IsValid(Item))) return;

		auto&& Filter = Token::Filter(Item).ByClass<UFaerieItemContainerToken>().By<Token::FMutableFilter>();

		for (UFaerieItemContainerToken* Token : Filter)
		{
			OutContainers.Add(Token->GetItemContainer());
		}
	}

	void GetAllContainersInItemRecursive(UFaerieItem* Item, TArray<UFaerieItemContainerBase*>& OutContainers)
	{
		if (!ensure(IsValid(Item))) return;

		TArray<UFaerieItemContainerBase*> Containers;
		GetAllContainersInItem(Item, Containers);
		OutContainers.Append(Containers);
		for (UFaerieItemContainerBase* Container : Containers)
		{
			for (UFaerieItem* SubItem : Container::KeyFilter(Container).Run<Container::FMutableFilter>().Items())
			{
				GetAllContainersInItemRecursive(SubItem, OutContainers);
			}
		}
	}

	TArray<UFaerieItemContainerBase*> GetAllContainersInItem(UFaerieItem* Item)
	{
		TArray<UFaerieItemContainerBase*> Containers;
		GetAllContainersInItem(Item, Containers);
		return Containers;
	}

	TArray<UFaerieItemContainerBase*> GetAllContainersInItemRecursive(UFaerieItem* Item)
	{
		TArray<UFaerieItemContainerBase*> Containers;
		GetAllContainersInItemRecursive(Item, Containers);
		return Containers;
	}

	void GetChildrenInItem(UFaerieItem* Item, TArray<const UFaerieItem*>& OutChildren)
	{
		for (UFaerieItemContainerBase* Container : SubObject::Iterate(Item))
		{
			for (const UFaerieItem* Child : Container::ItemRange(Container))
			{
				OutChildren.Add(Child);
			}
		}
	}

	void GetChildrenInItemRecursive(UFaerieItem* Item, TArray<const UFaerieItem*>& OutChildren)
	{
		if (!ensure(IsValid(Item))) return;

		TArray<const UFaerieItem*> Children;
		GetChildrenInItem(Item, OutChildren);
		OutChildren.Append(OutChildren);
		for (const UFaerieItem* Child : Children)
		{
			if (UFaerieItem* Mutable = Child->MutateCast())
			{
				for (UFaerieItemContainerBase* Container : SubObject::Iterate(Mutable))
				{
					for (const UFaerieItem* SubChild : Container::ItemRange(Container))
					{
						if (UFaerieItem* SubMutable = SubChild->MutateCast())
						{
							GetChildrenInItemRecursive(SubMutable, OutChildren);
						}
					}
				}
			}
		}
	}

	TArray<const UFaerieItem*> GetChildrenInItem(UFaerieItem* Item)
	{
		TArray<const UFaerieItem*> Items;
		GetChildrenInItem(Item, Items);
		return Items;
	}

	TArray<const UFaerieItem*> GetChildrenInItemRecursive(UFaerieItem* Item)
	{
		TArray<const UFaerieItem*> Items;
		GetChildrenInItemRecursive(Item, Items);
		return Items;
	}

	namespace SubObject
	{
		namespace Filters
		{
			void ByClass(TArray<UFaerieItemContainerBase*>& Containers, const TSubclassOf<UFaerieItemContainerBase>& Class)
			{
				Containers.RemoveAll([Class](const UFaerieItemContainerBase* Container)
				{
					return Class != Container->GetClass();
				});
			}
		}

		FIterator::FIterator(UFaerieItem* Item)
		  : Containers(GetAllContainersInItem(Item)),
			Iterator(Containers.CreateIterator()) {}
	}
}
