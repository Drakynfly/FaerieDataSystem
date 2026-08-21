// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Extensions/InventoryEjectionHandlerExtension.h"
#include "EntityManagerHelpers.h"
#include "FaerieInventoryContentLog.h"
#include "FaerieItemStorage.h"
#include "ItemContainerEvent.h"
#include "Actions/FaerieInventoryClient.h"
#include "Actors/FaerieItemOwningActorBase.h"
#include "Fragments/FaerieActorFragment.h"
#include "Engine/AssetManager.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryEjectionHandlerExtension)

using namespace Faerie;

namespace Faerie::Inventory::Tags
{
	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryTag, RemovalEject,
		"Fae.Inventory.Removal.Ejection", "Remove an item and eject it from the inventory as a pickup/visual")
}

EEventExtensionResponse UInventoryEjectionHandlerExtension::AllowsRemoval(const TNotNull<const UFaerieItemContainerBase*> Container,
	const TNotNull<const Container::IAddressView*> DataView, const FFaerieInventoryTag Reason) const
{
	if (Reason == Inventory::Tags::RemovalEject)
	{
		return EEventExtensionResponse::Allowed;
	}

	return EEventExtensionResponse::NoExplicitResponse;
}

void UInventoryEjectionHandlerExtension::PostEventBatch(const TNotNull<const UFaerieItemContainerBase*> Container, const Inventory::FEventLogBatch& Events)
{
	// This extension only listens to Ejection removals
    if (Events.Type != Inventory::Tags::RemovalEject) return;

	for (auto&& Event : Events.Data)
	{
		// Cannot eject null item
		if (Event.Instance.IsEmpty()) continue;

#if DO_CHECK
		if (Event.Instance.IsMutable())
		{
			check(Event.Copies == 1);
		}
#endif
		Enqueue(FFaerieUnownedItemStack(Event.Instance, Event.Copies));
	}
}

void UInventoryEjectionHandlerExtension::Enqueue(const FFaerieUnownedItemStack& Stack)
{
	PendingEjectionQueue.Add(Stack);

	if (!IsStreaming)
	{
		HandleNextInQueue();
	}
}

void UInventoryEjectionHandlerExtension::HandleNextInQueue()
{
	if (PendingEjectionQueue.IsEmpty()) return;

	TSoftClassPtr<AFaerieItemOwningActorBase> ClassToSpawn;

	auto* EntityManager = ItemData::GetFaerieEntityManager();
	auto ActorClassFragment = Faerie::ItemData::GetEntityFragmentOrDefault<FFaerieActorFragment>(EntityManager, PendingEjectionQueue[0].Instance);
	if (ActorClassFragment.IsValid())
	{
		ClassToSpawn = ActorClassFragment->OwningActorClass;
	}

	if (ClassToSpawn.IsNull())
	{
		ClassToSpawn = ExtensionDefaultClass;
	}

	if (ClassToSpawn.IsValid())
	{
		SpawnVisualizer(ClassToSpawn.Get());
	}
	else if (ClassToSpawn.IsPending())
	{
		IsStreaming = true;
		UAssetManager::GetStreamableManager().RequestAsyncLoad(ClassToSpawn.ToSoftObjectPath(),
			FStreamableDelegateWithHandle::CreateUObject(this, &ThisClass::PostLoadClassToSpawn));
	}
	else
	{
		UE_LOGF(LogFaerieInventoryContent, Error, "InventoryEjectionHandlerExtension encountered invalid ClassToSpawn, cannot eject Item!")
	}
}

void UInventoryEjectionHandlerExtension::PostLoadClassToSpawn(TSharedPtr<struct FStreamableHandle> Handle)
{
	IsStreaming = false;

	const TSubclassOf<AFaerieItemOwningActorBase> ActorClass = Handle->GetLoadedAsset<UClass>();

	if (!IsValid(ActorClass))
	{
		// Loading the actor class failed. Still remove the pending stack from the queue, tho.
		PendingEjectionQueue.RemoveAt(0);
		return HandleNextInQueue();
	}

	if (!ensure(!PendingEjectionQueue.IsEmpty()))
	{
		return;
	}

	SpawnVisualizer(ActorClass);
}

void UInventoryEjectionHandlerExtension::SpawnVisualizer(const TSubclassOf<AFaerieItemOwningActorBase>& Class)
{
	const AActor* OwningActor = GetTypedOuter<AActor>();

	if (!IsValid(OwningActor))
	{
		UE_LOGF(LogFaerieInventoryContent, Error, "InventoryEjectionHandlerExtension cannot find outer AActor. Ejection cancelled!")
		return;
	}

	FTransform SpawnTransform = IsValid(RelativeSpawningComponent) ? RelativeSpawningComponent->GetComponentTransform() : OwningActor->GetTransform();
	SpawnTransform = SpawnTransform.GetRelativeTransform(RelativeSpawningTransform);
	FActorSpawnParameters Args;
	Args.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	if (AFaerieItemOwningActorBase* NewPickup = OwningActor->GetWorld()->SpawnActor<AFaerieItemOwningActorBase>(Class, SpawnTransform, Args);
		IsValid(NewPickup))
	{
		NewPickup->SetOwnedStack(PendingEjectionQueue[0]);
	}

	PendingEjectionQueue.RemoveAt(0);

	HandleNextInQueue();
}

bool FFaerieClientAction_EjectEntry::Server_Execute(const TNotNull<const UFaerieInventoryClient*> Client) const
{
	if (!ItemStorage.IsValid()) return false;
	if (!Client->CanAccessContainer(ItemStorage.Get(), StaticStruct())) return false;

	return ItemStorage->RemoveStack(Address, Inventory::Tags::RemovalEject, Amount);
}

bool FFaerieClientAction_EjectViaRelease::Server_Execute(const TNotNull<const UFaerieInventoryClient*> Client) const
{
	if (!Handle.IsValid()) return false;
	if (!Client->CanAccessContainer(Handle.Container.Get(), StaticStruct())) return false;

	UInventoryEjectionHandlerExtension* Ejector = Extensions::Get<UInventoryEjectionHandlerExtension>(Handle.Container->GetExtensions(), true);
	if (!IsValid(Ejector))
	{
		return false;
	}

	if (const TOptional<FFaerieUnownedItemStack> Stack = Handle.Container->Release(Handle.Address, Amount);
		Stack.IsSet())
	{
		Ejector->Enqueue(Stack.GetValue());
		return true;
	}
	return false;
}
