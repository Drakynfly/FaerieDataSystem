// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Extensions/InventoryEjectionHandlerExtension.h"
#include "EntityManagerHelpers.h"
#include "FaerieContainerEvent.h"
#include "FaerieInventoryContentLog.h"
#include "ItemContainerEvent.h"
#include "MassExecutionContext.h"

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
		"Fae.Inventory.Removal.Ejection", "Remove an item and eject it from the inventory as a pickup/visual if the container has Ejection Config data.")
}

void FFaerieItemContainerEjectionConfig::HandleNextInQueue(const Content::FEjectionData& Ejection) const
{
	TSoftClassPtr<AFaerieItemOwningActorBase> ClassToSpawn;

	const FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(Ejection.Owner->GetWorld());
	auto ActorClassFragment = Faerie::ItemData::GetItemDataOrDefault<FFaerieActorFragment>(EntityManager, Ejection.Stack.Item);
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
		SpawnVisualizer(ClassToSpawn.Get(), Ejection);
	}
	else if (ClassToSpawn.IsPending())
	{
		UAssetManager::GetStreamableManager().RequestAsyncLoad(ClassToSpawn.ToSoftObjectPath(),
			FStreamableDelegateWithHandle::CreateRaw(this, &FFaerieItemContainerEjectionConfig::PostLoadClassToSpawn, Ejection));
	}
	else
	{
		UE_LOGF(LogFaerieInventoryContent, Error, "InventoryEjectionHandlerExtension encountered invalid ClassToSpawn, cannot eject Item!")
	}
}

void FFaerieItemContainerEjectionConfig::PostLoadClassToSpawn(TSharedPtr<struct FStreamableHandle> Handle, const Content::FEjectionData Ejection) const
{
	const TSubclassOf<AFaerieItemOwningActorBase> ActorClass = Handle->GetLoadedAsset<UClass>();

	if (!IsValid(ActorClass))
	{
		// Loading the actor class failed.
		return;
	}

	SpawnVisualizer(ActorClass, Ejection);
}

void FFaerieItemContainerEjectionConfig::SpawnVisualizer(const TSubclassOf<AFaerieItemOwningActorBase>& Class, const Content::FEjectionData& Ejection) const
{
	const AActor* OwningActor = Ejection.Owner.Get();
	if (!IsValid(OwningActor))
	{
		UE_LOGF(LogFaerieInventoryContent, Error, "InventoryEjectionHandlerExtension cannot find outer AActor. Ejection cancelled!")
		return;
	}

	FTransform SpawnTransform = IsValid(RelativeSpawningComponent) ? RelativeSpawningComponent->GetComponentTransform() : OwningActor->GetTransform();
	SpawnTransform = SpawnTransform.GetRelativeTransform(RelativeSpawningTransform);
	FActorSpawnParameters Args;
	Args.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	UWorld* World = OwningActor->GetWorld();
	if (AFaerieItemOwningActorBase* NewPickup = World->SpawnActor<AFaerieItemOwningActorBase>(Class, SpawnTransform, Args);
		IsValid(NewPickup))
	{
		NewPickup->SetOwnedStack(ItemData::GetFaerieEntityManagerChecked(World), Ejection.Stack);
	}
}

UFaerieContainerEjectionHandler::UFaerieContainerEjectionHandler()
  : EntityQuery(*this)
{
	// Process only on the server.
	ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);

	ExecutionOrder.ExecuteBefore.Add(Container::EventCleanup);

	// We read container data and spawn actors.
	bRequiresGameThreadExecution = true;
}

void UFaerieContainerEjectionHandler::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<Container::FContainerEventPayload>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);
}

void UFaerieContainerEjectionHandler::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(Context, [](const FMassExecutionContext& InContext)
	{
		const TConstArrayView<Container::FContainerEventPayload> Events = InContext.GetFragmentView<Container::FContainerEventPayload>();
		for (const Container::FContainerEventPayload& Event : Events)
		{
			// This observer only listens to Ejection removals
			// @todo this could be rolled into event somehow... could the type tag be check in the requirements
			if (Event.Type != Inventory::Tags::RemovalEject) continue;

			if (!Event.Item.IsSet()) continue;

			UFaerieItemContainerBase* Container = Event.Container.Get();
			if (!IsValid(Container))
			{
				continue;
			}

			const FFaerieItemContainerEjectionConfig* Config = Container->ReadContainerData<FFaerieItemContainerEjectionConfig>(InContext.GetEntityManagerChecked(), true);
			if (!Config)
			{
				continue;
			}

#if DO_CHECK
			if (ItemData::IsMutable(InContext.GetEntityManagerChecked(), Event.Item))
			{
				check(Event.Copies == 1);
			}
#endif


			AActor* Actor = Container->GetTypedOuter<AActor>();
			if (!Actor)
			{
				continue;
			}

			Content::FEjectionData Ejection;
			Ejection.Owner = Actor;
			Ejection.Stack = FFaerieUnownedItemStack(Event.Item, Event.Copies);

			Config->HandleNextInQueue(Ejection);
		}
	});
}

bool FFaerieClientAction_EjectEntry::Server_Execute(const TNotNull<const UFaerieInventoryClient*> Client, FMassEntityManager& EntityManager) const
{
	if (!Handle.IsValid()) return false;
	UFaerieItemContainerBase* Container = Handle.Container.Get();
	if (!Client->CanAccessContainer(EntityManager, Container, StaticStruct())) return false;

	const TOptional<FFaerieUnownedItemStack> Stack = Container->Release(EntityManager, Handle.Address, Amount, Inventory::Tags::RemovalEject);
	return Stack.IsSet();
}