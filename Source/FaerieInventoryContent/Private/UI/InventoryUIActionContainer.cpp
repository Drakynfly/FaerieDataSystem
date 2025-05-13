// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "UI/InventoryUIActionContainer.h"
#include "UI/InventoryUIAction.h"

bool UInventoryUIActionContainer::AddActionOfClass(const TSubclassOf<UInventoryUIAction2> Class)
{
	if (Class)
	{
		Actions.Add(NewObject<UInventoryUIAction2>(this, Class));
	}
	return true;
}

bool UInventoryUIActionContainer::AddActionInstance(UInventoryUIAction2* Action)
{
	if (Action)
	{
		Actions.Add(Action);
	}
	return true;
}

bool UInventoryUIActionContainer::RemoveActionOfClass(TSubclassOf<UInventoryUIAction2> Class)
{
	return !!Actions.RemoveAll(
		[Class](const UInventoryUIAction2* Action)
		{
			return Action->GetClass() == Class;
		});
}

bool UInventoryUIActionContainer::RemoveActionInstance(UInventoryUIAction2* Action)
{
	if (Action)
	{
		Actions.Remove(Action);
	}
	return true;
}
