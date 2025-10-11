// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "UI/InventoryUIActionContainer.h"
#include "Templates/SubclassOf.h"
#include "UObject/ObjectKey.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryUIActionContainer)

bool UInventoryUIActionContainer::AddAction(const TSubclassOf<UInventoryUIAction> Class)
{
	if (IsValid(Class))
	{
		ActionClasses.Add(Class);
		return true;
	}
	return false;
}

bool UInventoryUIActionContainer::AddActions(const TSet<TSubclassOf<UInventoryUIAction>> Classes)
{
	ActionClasses.Append(Classes);
	return true;
}

bool UInventoryUIActionContainer::AddActionInstance(UInventoryUIAction* Action)
{
	if (IsValid(Action))
	{
		ActionInstances.Add(Action);
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

bool UInventoryUIActionContainer::RemoveAction(const TSubclassOf<UInventoryUIAction> Class)
{
	if (IsValid(Class))
	{
		return !!ActionClasses.Remove(Class);
	}
	return false;
}

bool UInventoryUIActionContainer::RemoveActions(TSet<TSubclassOf<UInventoryUIAction>> Classes)
{
	for (auto Class : Classes)
	{
		ActionClasses.Remove(Class);
	}
	return true;
}

bool UInventoryUIActionContainer::RemoveActionInstance(UInventoryUIAction* Action)
{
	if (IsValid(Action))
	{
		return !!ActionInstances.Remove(Action);
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
	TSet<UInventoryUIAction*> AllInstances { ActionInstances };
	TSet<TSubclassOf<UInventoryUIAction>> AllClasses { ActionClasses };

	TSet<FObjectKey> HitSubcontainers;
	HitSubcontainers.Add(this);

	TArray<const UInventoryUIActionContainer*> SubContainersToSearch { SubContainers };
	while (!SubContainersToSearch.IsEmpty())
	{
		const UInventoryUIActionContainer* SubContainer = SubContainersToSearch.Pop();
		HitSubcontainers.Add(SubContainer);

		AllInstances.Append(SubContainer->ActionInstances);
		AllClasses.Append(SubContainer->ActionClasses);

		for (auto&& Container : SubContainer->SubContainers)
		{
			if (!HitSubcontainers.Contains(Container))
			{
				SubContainersToSearch.Add(Container);
			}
		}
	}

	TArray<UInventoryUIAction*> AllActions;
	AllActions.Reserve(AllInstances.Num() + AllClasses.Num());
	AllActions.Append(AllInstances.Array());
	for (auto&& ActionClass : AllClasses)
	{
		AllActions.Add(ActionClass->GetDefaultObject<UInventoryUIAction>());
	}

	return AllActions;
}
