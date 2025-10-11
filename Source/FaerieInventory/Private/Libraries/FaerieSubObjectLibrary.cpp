// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieSubObjectLibrary.h"
#include "FaerieSubObjectFilter.h"

TArray<UFaerieItemContainerBase*> UFaerieSubObjectLibrary::GetAllContainersInItem(UFaerieItem* Item, const bool Recursive)
{
	if (Recursive)
	{
		return Faerie::GetAllContainersInItemRecursive(Item);
	}
	return Faerie::GetAllContainersInItem(Item);
}

TArray<UFaerieItemContainerBase*> UFaerieSubObjectLibrary::GetContainersInItemOfClass(UFaerieItem* Item,
	const TSubclassOf<UFaerieItemContainerBase> Class, const bool Recursive)
{
	if (Recursive)
	{
		return Faerie::SubObject::Filter().Recursive().ByClass(Class).Emit(Item);
	}
	return Faerie::SubObject::Filter().ByClass(Class).Emit(Item);
}

TArray<UFaerieItem*> UFaerieSubObjectLibrary::GetItemChildren(UFaerieItem* Item, const bool Recursive)
{
	if (Recursive)
	{
		return Type::Cast<TArray<UFaerieItem*>>(Faerie::GetChildrenInItemRecursive(Item));
	}
	return Type::Cast<TArray<UFaerieItem*>>(Faerie::GetChildrenInItem(Item));
}
