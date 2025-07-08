// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "UI/InventoryUIActionContainer.h"
#include "UI/InventoryUIAction.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryUIActionContainer)

bool UInventoryUIActionContainer::AddActionOfClass(const TSubclassOf<UInventoryUIAction> Class)
{
	if (IsValid(Class))
	{
		Actions.Add(NewObject<UInventoryUIAction>(this, Class));
		return true;
	}
	return false;
}

bool UInventoryUIActionContainer::AddActionInstance(UInventoryUIAction* Action)
{
	if (IsValid(Action))
	{
		Actions.Add(Action);
		return true;
	}
	return false;
}

bool UInventoryUIActionContainer::AddSubContainer(UInventoryUIActionContainer* Container)
{
	if (IsValid(Container))
	{
		SubContainers.AddUnique(Container);
		return true;
	}
	return false;
}

bool UInventoryUIActionContainer::RemoveActionsOfClass(TSubclassOf<UInventoryUIAction> Class)
{
	return !!Actions.RemoveAll(
		[Class](const UInventoryUIAction* Action)
		{
			return Action->GetClass() == Class;
		});
}

bool UInventoryUIActionContainer::RemoveActionInstance(UInventoryUIAction* Action)
{
	if (IsValid(Action))
	{
		return !!Actions.Remove(Action);
	}
	return false;
}

bool UInventoryUIActionContainer::RemoveSubContainer(UInventoryUIActionContainer* Container)
{
	if (IsValid(Container))
	{
		return !!SubContainers.Remove(Container);
	}
	return false;
}

TArray<UInventoryUIAction*> UInventoryUIActionContainer::GetAllActions() const
{
	// @todo doesn't check for recursion or duplicates!
	TArray<UInventoryUIAction*> AllActions = Actions;
	for (auto&& SubContainer : SubContainers)
	{
		AllActions.Append(SubContainer->GetAllActions());
	}
	return AllActions;
}
