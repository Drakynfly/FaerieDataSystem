// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Extensions/InventoryEjectionHandlerExtension.h"

#include "FaerieItemDataProxy.h"
#include "FaerieItemStorage.h"
#include "ItemContainerEvent.h"
#include "Tokens/FaerieVisualActorClassToken.h"
#include "Actors/ItemRepresentationActor.h"
#include "Engine/AssetManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryEjectionHandlerExtension)

namespace Faerie::Inventory::Tags
{
	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryTag, RemovalEject,
		"Fae.Inventory.Removal.Ejection", "Remove an item and eject it from the inventory as a pickup/visual")
}

EEventExtensionResponse UInventoryEjectionHandlerExtension::AllowsRemoval(const UFaerieItemContainerBase* Container, const FEntryKey Key,
                                                                          const FFaerieInventoryTag Reason) const
{
	if (Reason == Faerie::Inventory::Tags::RemovalEject)
	{
		return EEventExtensionResponse::Allowed;
	}

	return EEventExtensionResponse::NoExplicitResponse;
}

void UInventoryEjectionHandlerExtension::PostRemoval(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event)
{
	// This extension only listens to Ejection removals
	if (Event.Type != Faerie::Inventory::Tags::RemovalEject) return;

	// Cannot eject null item
	if (!Event.Item.IsValid()) return;

	FFaerieItemStack Stack;
	Stack.Copies = Event.Amount;

	if (Event.Item->CanMutate())
	{
		// @todo figure out handling ejection of stacks
		check(Stack.Copies == 1);
	}

	Stack.Item = const_cast<UFaerieItem*>(Event.Item.Get());

	PendingEjectionQueue.Add(Stack);

	if (!IsStreaming)
	{
		HandleNextInQueue();
	}
}

void UInventoryEjectionHandlerExtension::HandleNextInQueue()
{
	if (PendingEjectionQueue.IsEmpty()) return;

	TSoftClassPtr<AItemRepresentationActor> ClassToSpawn;

	if (auto&& ClassToken = PendingEjectionQueue[0].Item->GetToken<UFaerieVisualActorClassToken>())
	{
		ClassToSpawn = ClassToken->GetActorClass();
	}
	else
	{
		ClassToSpawn = ExtensionDefaultClass;
	}

	if (ClassToSpawn.IsValid())
	{
		PostLoadClassToSpawn(ClassToSpawn);
	}
	else
	{
		IsStreaming = true;
		UAssetManager::GetStreamableManager().RequestAsyncLoad(ClassToSpawn.ToSoftObjectPath(),
			FStreamableDelegate::CreateUObject(this, &ThisClass::PostLoadClassToSpawn, ClassToSpawn));
	}
}

void UInventoryEjectionHandlerExtension::PostLoadClassToSpawn(const TSoftClassPtr<AItemRepresentationActor> ClassToSpawn)
{
	IsStreaming = false;

	const TSubclassOf<AItemRepresentationActor> ActorClass = ClassToSpawn.Get();

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

	const AActor* OwningActor = GetTypedOuter<AActor>();

	if (!IsValid(OwningActor))
	{
		UE_LOG(LogTemp, Error, TEXT("InventoryEjectionHandlerExtension cannot find outer AActor. Ejection cancelled!"))
		return;
	}

	FTransform SpawnTransform = IsValid(RelativeSpawningComponent) ? RelativeSpawningComponent->GetComponentTransform() : OwningActor->GetTransform();
	SpawnTransform = SpawnTransform.GetRelativeTransform(RelativeSpawningTransform);
	FActorSpawnParameters Args;
	Args.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AItemRepresentationActor* NewPickup = OwningActor->GetWorld()->SpawnActor<AItemRepresentationActor>(ActorClass, SpawnTransform, Args);

	if (IsValid(NewPickup))
	{
		UFaerieItemDataStackLiteral* FaerieItemStack = NewObject<UFaerieItemDataStackLiteral>(NewPickup);
		FaerieItemStack->SetValue(PendingEjectionQueue[0]);
		NewPickup->SetSourceProxy(FaerieItemStack);
	}

	PendingEjectionQueue.RemoveAt(0);

	HandleNextInQueue();
}

bool FFaerieClientAction_EjectEntry::Server_Execute(const UFaerieInventoryClient* Client) const
{
	auto&& Storage = Handle.ItemStorage.Get();
	if (!IsValid(Storage)) return false;
	if (!Client->CanAccessStorage(Storage)) return false;

	return Storage->RemoveStack(Handle.Key, Faerie::Inventory::Tags::RemovalEject, Amount);
}