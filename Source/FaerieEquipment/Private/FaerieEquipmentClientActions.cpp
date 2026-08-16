// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieEquipmentClientActions.h"
#include "FaerieEquipmentSlot.h"
#include "FaerieItemStorage.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieEquipmentClientActions)

bool FFaerieClientAction_MoveFromSlot::IsValid(const TNotNull<const UFaerieInventoryClient*> Client) const
{
	return ::IsValid(Slot) &&
		Client->CanAccessContainer(Slot, StaticStruct()) &&
		Slot->IsFilled();
}

bool FFaerieClientAction_MoveFromSlot::View(Faerie::ItemData::FScopeProxy& Proxy) const
{
	if (Slot->IsFilled())
	{
		Proxy = Slot->GetView();
		return true;
	}
	return false;
}

bool FFaerieClientAction_MoveFromSlot::CanMove(const Faerie::TValid<const FFaerieItemProxy&> Proxy) const
{
	return Slot->CouldSetInSlot(Proxy);
}

bool FFaerieClientAction_MoveFromSlot::Release(FFaerieUnownedItemStack& Stack) const
{
	Stack = Slot->TakeItemFromSlot(Faerie::ItemData::EntireStack, Faerie::Inventory::Tags::RemovalMoving);
	return Stack.IsValid();
}

bool FFaerieClientAction_MoveFromSlot::Possess(const Faerie::TValid<const FFaerieUnownedItemStack&> Stack) const
{
	return Slot->SetItemInSlot(Stack);
}

bool FFaerieClientAction_MoveToSlot::IsValid(const TNotNull<const UFaerieInventoryClient*> Client) const
{
	return ::IsValid(Slot) &&
		Client->CanAccessContainer(Slot, StaticStruct());
}

bool FFaerieClientAction_MoveToSlot::View(Faerie::ItemData::FScopeProxy& Proxy) const
{
	if (Slot->IsFilled())
	{
		Proxy = Slot->GetView();
		return true;
	}
	return false;
}

bool FFaerieClientAction_MoveToSlot::CanMove(const Faerie::TValid<const FFaerieItemProxy&> Proxy) const
{
	return Slot->CouldSetInSlot(Proxy);
}

bool FFaerieClientAction_MoveToSlot::Release(FFaerieUnownedItemStack& Stack) const
{
	Stack = Slot->TakeItemFromSlot(Faerie::ItemData::EntireStack, Faerie::Inventory::Tags::RemovalMoving);
	return Stack.IsValid();
}

bool FFaerieClientAction_MoveToSlot::Possess(const Faerie::TValid<const FFaerieUnownedItemStack&> Stack) const
{
	return Slot->SetItemInSlot(Stack);
}

bool FFaerieClientAction_MoveToSlot::IsSwap() const
{
	return CanSwapSlots && Slot->IsFilled();
}