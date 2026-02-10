// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieSubObjectLibrary.h"
#include "FaerieItem.h"
#include "FaerieSubObjectFilter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieSubObjectLibrary)

TArray<UFaerieItemContainerBase*> UFaerieSubObjectLibrary::GetAllContainersInItem(UFaerieItem* Item, const bool Recursive)
{
	if (!IsValid(Item)) return {};

	if (Recursive)
	{
		return Faerie::GetAllContainersInItemRecursive(Item);
	}
	return Faerie::GetAllContainersInItem(Item);
}

TArray<UFaerieItemContainerBase*> UFaerieSubObjectLibrary::GetContainersInItemOfClass(UFaerieItem* Item,
	const TSubclassOf<UFaerieItemContainerBase> Class, const bool Recursive)
{
	if (!IsValid(Item)) return {};

	Faerie::SubObject::FClassFilter ClassFilter(Class);

	if (Recursive)
	{
		return Faerie::SubObject::Filter().Recursive().By(MoveTemp(ClassFilter)).Emit(Item);
	}
	return Faerie::SubObject::Filter().By(MoveTemp(ClassFilter)).Emit(Item);
}

TArray<UFaerieItem*> UFaerieSubObjectLibrary::GetItemChildren(UFaerieItem* Item, const bool Recursive)
{
	if (!IsValid(Item)) return {};

	if (Recursive)
	{
		return Type::Cast<TArray<UFaerieItem*>>(Faerie::GetChildrenInItemRecursive(Item));
	}
	return Type::Cast<TArray<UFaerieItem*>>(Faerie::GetChildrenInItem(Item));
}
