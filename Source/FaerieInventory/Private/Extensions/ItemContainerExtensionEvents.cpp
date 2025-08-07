// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Extensions/ItemContainerExtensionEvents.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemContainerExtensionEvents)

void UItemContainerExtensionEvents::InitializeExtension(const UFaerieItemContainerBase* Container)
{
	ExtensionEvent.Broadcast(Container, Faerie::Initialization);
}

void UItemContainerExtensionEvents::DeinitializeExtension(const UFaerieItemContainerBase* Container)
{
	ExtensionEvent.Broadcast(Container, Faerie::Deinitialization);
}

void UItemContainerExtensionEvents::PreAddition(const UFaerieItemContainerBase* Container, const FFaerieItemStackView Stack)
{
	PreAdditionEvent.Broadcast(Container, Stack);
}

void UItemContainerExtensionEvents::PostAddition(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event)
{
	PostAdditionEvent.Broadcast(Container, Event);
}

void UItemContainerExtensionEvents::PreRemoval(const UFaerieItemContainerBase* Container, const FEntryKey Key, const int32 Removal)
{
	PreRemovalEvent.Broadcast(Container, Key, Removal);
}

void UItemContainerExtensionEvents::PostRemoval(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event)
{
	PostRemovalEvent.Broadcast(Container, Event);
}

void UItemContainerExtensionEvents::PostEntryChanged(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event)
{
	PostEntryChangedEvent.Broadcast(Container, Event);
}
