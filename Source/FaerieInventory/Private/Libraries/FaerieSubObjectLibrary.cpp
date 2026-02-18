// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieSubObjectLibrary.h"
#include "FaerieItem.h"
#include "FaerieSubObjectFilter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieSubObjectLibrary)

using namespace Faerie;

TArray<UFaerieItemContainerBase*> UFaerieSubObjectLibrary::GetAllContainersInItem(UFaerieItem* Item, const bool Recursive)
{
	if (!IsValid(Item)) return {};

	if (Recursive)
	{
		return SubObject::GetAllContainersInItemRecursive(Item);
	}
	return SubObject::GetAllContainersInItem(Item);
}

TArray<UFaerieItemContainerBase*> UFaerieSubObjectLibrary::GetContainersInItemOfClass(UFaerieItem* Item,
	const TSubclassOf<UFaerieItemContainerBase> Class, const bool Recursive)
{
	if (!IsValid(Item)) return {};

	SubObject::FClassFilter ClassFilter(Class);

	if (Recursive)
	{
		return SubObject::Filter().Recursive().By(MoveTemp(ClassFilter)).Emit(Item);
	}
	return SubObject::Filter().By(MoveTemp(ClassFilter)).Emit(Item);
}

TArray<UFaerieItem*> UFaerieSubObjectLibrary::GetItemChildren(UFaerieItem* Item, const bool Recursive)
{
	if (!IsValid(Item)) return {};

	if (Recursive)
	{
		return Utils::Cast<TArray<UFaerieItem*>>(SubObject::GetChildrenInItemRecursive(Item));
	}
	return Utils::Cast<TArray<UFaerieItem*>>(SubObject::GetChildrenInItem(Item));
}
