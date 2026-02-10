// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieEquipmentClientActions.h"
#include "FaerieEquipmentSlot.h"
#include "FaerieItemStorage.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieEquipmentClientActions)

bool FFaerieClientAction_SetItemInSlot::Server_Execute(const UFaerieInventoryClient* Client) const
{
	if (!IsValid(Slot)) return false;
	if (!Client->CanAccessContainer(Slot, StaticStruct()))  return false;
	if (!IsValid(Stack.Item)) return false;

	return Slot->SetItemInSlot(Stack);
}

bool FFaerieClientAction_MoveFromSlot::IsValid(const UFaerieInventoryClient* Client) const
{
	if (!::IsValid(Slot) ||
		!Client->CanAccessContainer(Slot, StaticStruct()) ||
		!Slot->IsFilled()) return false;
	return true;
}

bool FFaerieClientAction_MoveFromSlot::View(FFaerieItemStackView& View) const
{
	View = Slot->View();
	return true;
}

bool FFaerieClientAction_MoveFromSlot::CanMove(const FFaerieItemStackView& View) const
{
	return Slot->CouldSetInSlot(View);
}

bool FFaerieClientAction_MoveFromSlot::Release(FFaerieItemStack& Stack) const
{
	Stack = Slot->TakeItemFromSlot(Faerie::ItemData::EntireStack);
	return ::IsValid(Stack.Item);
}

bool FFaerieClientAction_MoveFromSlot::Possess(const FFaerieItemStack& Stack) const
{
	Slot->SetItemInSlot(Stack);
	return true;
}

bool FFaerieClientAction_MoveToSlot::IsValid(const UFaerieInventoryClient* Client) const
{
	if (!::IsValid(Slot) ||
		!Client->CanAccessContainer(Slot, StaticStruct())) return false;
	return true;
}

bool FFaerieClientAction_MoveToSlot::View(FFaerieItemStackView& View) const
{
	View = Slot->View();
	return true;
}

bool FFaerieClientAction_MoveToSlot::CanMove(const FFaerieItemStackView& View) const
{
	return Slot->CouldSetInSlot(View);
}

bool FFaerieClientAction_MoveToSlot::Release(FFaerieItemStack& Stack) const
{
	Stack = Slot->TakeItemFromSlot(Faerie::ItemData::EntireStack);
	return ::IsValid(Stack.Item);
}

bool FFaerieClientAction_MoveToSlot::Possess(const FFaerieItemStack& Stack) const
{
	Slot->SetItemInSlot(Stack);
	return true;
}

bool FFaerieClientAction_MoveToSlot::IsSwap() const
{
	return CanSwapSlots && Slot->IsFilled();
}