// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Extensions/ItemContainerExtensionEvents.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemContainerExtensionEvents)

using namespace Faerie;

void UItemContainerExtensionEvents::InitializeExtension(const TNotNull<const UFaerieItemContainerBase*> Container)
{
	ExtensionEvent.Broadcast(Container, Extensions::Initialization);
}

void UItemContainerExtensionEvents::DeinitializeExtension(const TNotNull<const UFaerieItemContainerBase*> Container)
{
	ExtensionEvent.Broadcast(Container, Extensions::Deinitialization);
}

void UItemContainerExtensionEvents::PreAddition(const TNotNull<const UFaerieItemContainerBase*> Container, const FFaerieItemDataView& View)
{
	PreAdditionEvent.Broadcast(Container, View);
}

void UItemContainerExtensionEvents::PreRemoval(const TNotNull<const UFaerieItemContainerBase*> Container, const ItemData::TNonNullViewPtr<Container::IEntryView> DataView, const int32 Removal)
{
	PreRemovalEvent.Broadcast(Container, DataView, Removal);
}

void UItemContainerExtensionEvents::PostEventBatch(const TNotNull<const UFaerieItemContainerBase*> Container, const Inventory::FEventLogBatch& Events)
{
	OnPostEventBatch.Broadcast(Container, Events);
}