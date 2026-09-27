// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "StackContainerActions.h"
#include "FaerieItemStackContainer.h"
#include "FaerieItemStorage.h"

#include "Actions/FaerieInventoryClient.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(StackContainerActions)

bool FFaerieClientAction_MoveFromStackContainer::IsValid(const TNotNull<const UFaerieInventoryClient*> Client) const
{
	return ::IsValid(Stack) &&
		Client->CanAccessContainer(Stack, StaticStruct()) &&
		Stack->IsFilled();
}

bool FFaerieClientAction_MoveFromStackContainer::View(const FMassEntityManager& EntityManager, Faerie::ItemData::FScopeProxy& Proxy) const
{
	if (Stack->IsFilled())
	{
		Proxy = Stack->GetView();
		return true;
	}
	return false;
}

bool FFaerieClientAction_MoveFromStackContainer::CanMove(const FMassEntityManager& EntityManager, const Faerie::TValid<const FFaerieItemProxy&> Proxy) const
{
	return Stack->CouldSetInSlot(Proxy);
}

bool FFaerieClientAction_MoveFromStackContainer::Release(FMassEntityManager& EntityManager, FFaerieUnownedItemStack& OutStack) const
{
	OutStack = Stack->TakeItemFromSlot(Faerie::ItemData::EntireStack, Faerie::Inventory::Tags::RemovalMoving);
	return OutStack.IsValid();
}

bool FFaerieClientAction_MoveFromStackContainer::Possess(FMassEntityManager& EntityManager, const Faerie::TValid<const FFaerieUnownedItemStack&> InStack) const
{
	return Stack->SetItemInSlot(InStack);
}

bool FFaerieClientAction_MoveToStackContainer::IsValid(const TNotNull<const UFaerieInventoryClient*> Client) const
{
	return ::IsValid(Stack) &&
		Client->CanAccessContainer(Stack, StaticStruct());
}

bool FFaerieClientAction_MoveToStackContainer::View(const FMassEntityManager& EntityManager, Faerie::ItemData::FScopeProxy& Proxy) const
{
	if (Stack->IsFilled())
	{
		Proxy = Stack->GetView();
		return true;
	}
	return false;
}

bool FFaerieClientAction_MoveToStackContainer::CanMove(const FMassEntityManager& EntityManager, const Faerie::TValid<const FFaerieItemProxy&> Proxy) const
{
	return Stack->CouldSetInSlot(Proxy);
}

bool FFaerieClientAction_MoveToStackContainer::Release(FMassEntityManager& EntityManager, FFaerieUnownedItemStack& OutStack) const
{
	OutStack = Stack->TakeItemFromSlot(Faerie::ItemData::EntireStack, Faerie::Inventory::Tags::RemovalMoving);
	return OutStack.IsValid();
}

bool FFaerieClientAction_MoveToStackContainer::Possess(FMassEntityManager& EntityManager, const Faerie::TValid<const FFaerieUnownedItemStack&> InStack) const
{
	return Stack->SetItemInSlot(InStack);
}

bool FFaerieClientAction_MoveToStackContainer::IsSwap(const FMassEntityManager& EntityManager) const
{
	return CanSwapContent && Stack->IsFilled();
}