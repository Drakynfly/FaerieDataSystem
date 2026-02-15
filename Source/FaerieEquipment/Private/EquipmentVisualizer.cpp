// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "EquipmentVisualizer.h"
#include "FaerieContainerExtensionInterface.h"
#include "FaerieEquipmentLog.h"
#include "FaerieEquipmentSlot.h"
#include "Components/FaerieItemMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Engine/World.h"
#include "Extensions/VisualSlotExtension.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EquipmentVisualizer)

namespace Faerie
{
	// @todo these are hardcoded for now.
	static const FAttachmentTransformRules TempTransformRules{
		EAttachmentRule::SnapToTarget,
		EAttachmentRule::SnapToTarget,
		EAttachmentRule::KeepRelative,
		false
	};

	void UpdateActorAttachment(AActor* Visual, const FEquipmentVisualAttachment& Metadata)
	{
		if (IsValid(Metadata.Parent.Get()))
		{
			Visual->AttachToComponent(Metadata.Parent.Get(), TempTransformRules, Metadata.ParentSocket);

			const USceneComponent* SelfComponent = Visual->GetDefaultAttachComponent();
			if (SelfComponent->DoesSocketExist(Metadata.ChildSocket))
			{
				const FVector Offset = -SelfComponent->GetSocketTransform(Metadata.ChildSocket, RTS_Component).GetTranslation();
				Visual->AddActorLocalOffset(Offset);
			}

			Visual->SetActorHiddenInGame(Metadata.Hidden);
		}
		else
		{
			Visual->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		}
	}

	void UpdateComponentAttachment(USceneComponent* Visual, const FEquipmentVisualAttachment& Metadata)
	{
		if (IsValid(Metadata.Parent.Get()))
		{
			Visual->AttachToComponent(Metadata.Parent.Get(), TempTransformRules, Metadata.ParentSocket);
			if (Visual->DoesSocketExist(Metadata.ChildSocket))
			{
				const FVector Offset = -Visual->GetSocketTransform(Metadata.ChildSocket, RTS_Component).GetTranslation();
				Visual->AddLocalOffset(Offset);
			}
			Visual->SetVisibility(!Metadata.Hidden, true);
		}
		else
		{
			Visual->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		}
	}
}

UEquipmentVisualizer::UEquipmentVisualizer()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UEquipmentVisualizer::OnComponentDestroyed(const bool bDestroyingHierarchy)
{
	for (auto&& Element : SpawnedActors)
	{
		if (IsValid(Element.Value))
		{
			Element.Value->OnDestroyed.RemoveAll(this);
			Element.Value->Destroy();
		}
	}

	for (auto&& Element : SpawnedComponents)
	{
		if (IsValid(Element.Value))
		{
			Element.Value->DestroyComponent();
		}
	}

	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

USkinnedMeshComponent* UEquipmentVisualizer::GetLeaderComponent() const
{
	return Cast<USkinnedMeshComponent>(LeaderPoseComponent.GetComponent(GetOwner()));
}

bool UEquipmentVisualizer::HasVisualForKey(const FFaerieVisualKey Key) const
{
	if (!Key.IsValid()) return false;
	return SpawnedActors.Contains(Key) || SpawnedComponents.Contains(Key);
}

UObject* UEquipmentVisualizer::GetSpawnedVisualByClass(const TSubclassOf<UObject> Class, FFaerieVisualKey& Key) const
{
	if (!IsValid(Class)) return nullptr;

	UObject* Out = GetSpawnedActorByClass(Class.Get(), Key);

	if (!IsValid(Out))
	{
		Out = GetSpawnedComponentByClass(Class.Get(), Key);
	}

	return Out;
}

AActor* UEquipmentVisualizer::GetSpawnedActorByClass(const TSubclassOf<AActor> Class, FFaerieVisualKey& Key) const
{
	if (!IsValid(Class)) return nullptr;

	for (auto&& SpawnedActor : SpawnedActors)
	{
		if (SpawnedActor.Value && SpawnedActor.Value.IsA(Class))
		{
			Key = SpawnedActor.Key;
			return SpawnedActor.Value;
		}
	}
	return nullptr;
}

USceneComponent* UEquipmentVisualizer::GetSpawnedComponentByClass(const TSubclassOf<USceneComponent> Class, FFaerieVisualKey& Key) const
{
	if (!IsValid(Class)) return nullptr;

	for (auto&& SpawnedComponent : SpawnedComponents)
	{
		if (SpawnedComponent.Value && SpawnedComponent.Value.IsA(Class))
		{
			Key = SpawnedComponent.Key;
			return SpawnedComponent.Value;
		}
	}
	return nullptr;
}

UObject* UEquipmentVisualizer::GetSpawnedVisualByKey(const FFaerieVisualKey Key) const
{
	if (!Key.IsValid()) return nullptr;

	UObject* Out = GetSpawnedActorByKey(Key);

	if (!IsValid(Out))
	{
		Out = GetSpawnedComponentByKey(Key);
	}

	return Out;
}

AActor* UEquipmentVisualizer::GetSpawnedActorByKey(const FFaerieVisualKey Key) const
{
	if (!Key.IsValid()) return nullptr;

	if (auto&& Found = SpawnedActors.Find(Key))
	{
		return *Found;
	}
	return nullptr;
}

USceneComponent* UEquipmentVisualizer::GetSpawnedComponentByKey(const FFaerieVisualKey Key) const
{
	if (!Key.IsValid()) return nullptr;

	if (auto&& Found = SpawnedComponents.Find(Key))
	{
		return *Found;
	}
	return nullptr;
}

TArray<AActor*> UEquipmentVisualizer::GetSpawnedActors() const
{
	TArray<AActor*> Array;
	SpawnedActors.GenerateValueArray(ObjectPtrWrap(Array));
	return Array;
}

TArray<USceneComponent*> UEquipmentVisualizer::GetSpawnedComponents() const
{
	TArray<USceneComponent*> Array;
	SpawnedComponents.GenerateValueArray(ObjectPtrWrap(Array));
	return Array;
}

AActor* UEquipmentVisualizer::SpawnVisualActor(const FFaerieVisualKey Key, const TSubclassOf<AActor>& Class, const FEquipmentVisualAttachment& Attachment)
{
	if (!Key.IsValid()) return nullptr;
	if (!IsValid(Class)) return nullptr;

	if (SpawnedActors.Contains(Key))
	{
		UE_LOG(LogFaerieEquipment, Error, TEXT("Attempted to spawn a VisualActor using a key already in use!"))
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.Owner = GetOwner();
	if (AActor* NewActor = World->SpawnActor(Class, &FTransform::Identity, Params))
	{
		NewActor->OnDestroyed.AddDynamic(this, &ThisClass::OnVisualActorDestroyed);

		SpawnedActors.Add(Key, NewActor);
		ReverseMap.Add(NewActor, Key);

		if (Attachment.Parent.IsValid())
		{
			Faerie::UpdateActorAttachment(NewActor, Attachment);

			KeyedMetadata.FindOrAdd(Key).Attachment = Attachment;
		}

		KeyedMetadata.FindOrAdd(Key).ChangeCallback.Broadcast(Key, NewActor);
		OnAnyVisualSpawnedNative.Broadcast(Key, NewActor);
		OnAnyVisualSpawned.Broadcast(Key, NewActor);

		return NewActor;
	}

	return nullptr;
}

USceneComponent* UEquipmentVisualizer::SpawnVisualComponent(const FFaerieVisualKey Key, const TSubclassOf<USceneComponent>& Class,
	const FEquipmentVisualAttachment& Attachment)
{
	if (!Key.IsValid()) return nullptr;
	if (!IsValid(Class)) return nullptr;

	if (SpawnedComponents.Contains(Key))
	{
		UE_LOG(LogFaerieEquipment, Error, TEXT("Attempted to spawn a VisualComponent using a key already in use!"))
		return nullptr;
	}

	if (USceneComponent* NewComponent = NewObject<USceneComponent>(GetOwner(), Class);
		IsValid(NewComponent))
	{
		GetOwner()->AddInstanceComponent(NewComponent);
		NewComponent->RegisterComponent();

		SpawnedComponents.Add(Key, NewComponent);
		ReverseMap.Add(NewComponent, Key);

		KeyedMetadata.FindOrAdd(Key).Attachment = Attachment;

		Faerie::UpdateComponentAttachment(NewComponent, Attachment);

		KeyedMetadata.FindOrAdd(Key).ChangeCallback.Broadcast(Key, NewComponent);
		OnAnyVisualSpawnedNative.Broadcast(Key, NewComponent);
		OnAnyVisualSpawned.Broadcast(Key, NewComponent);

		return NewComponent;
	}

	return nullptr;
}

bool UEquipmentVisualizer::DestroyVisual(UObject* Visual, const bool ClearMetadata)
{
	const FFaerieVisualKey Key = ReverseMap.FindAndRemoveChecked(Visual);

	if (ClearMetadata)
	{
		KeyedMetadata.Remove(Key);
	}

	if (AActor* VisualActor = Cast<AActor>(Visual))
	{
		VisualActor->Destroy();
		SpawnedActors.Remove(Key);

		OnAnyVisualDestroyedNative.Broadcast(Key);
		OnAnyVisualDestroyed.Broadcast(Key);

		return true;
	}

	if (USceneComponent* VisualComponent = Cast<USceneComponent>(Visual))
	{
		VisualComponent->DestroyComponent();
		SpawnedComponents.Remove(Key);

		OnAnyVisualDestroyedNative.Broadcast(Key);
		OnAnyVisualDestroyed.Broadcast(Key);

		return true;
	}

	return false;
}

bool UEquipmentVisualizer::DestroyVisualByKey(const FFaerieVisualKey Key, const bool ClearMetadata)
{
	if (!Key.IsValid()) return false;

	if (ClearMetadata)
	{
		KeyedMetadata.Remove(Key);
	}

	if (AActor* Visual = GetSpawnedActorByKey(Key))
	{
		// This will clear from SpawnedActors and ReverseMap via OnDestroyed
		Visual->Destroy();
		SpawnedActors.Remove(Key);

		OnAnyVisualDestroyedNative.Broadcast(Key);
		OnAnyVisualDestroyed.Broadcast(Key);

		ReverseMap.Remove(Visual);

		return true;
	}

	if (USceneComponent* VisualComponent = GetSpawnedComponentByKey(Key))
	{
		VisualComponent->DestroyComponent();
		SpawnedComponents.Remove(Key);

		OnAnyVisualDestroyedNative.Broadcast(Key);
		OnAnyVisualDestroyed.Broadcast(Key);

		ReverseMap.Remove(VisualComponent);

		return true;
	}

	return false;
}

FEquipmentVisualAttachment UEquipmentVisualizer::FindAttachment(const FFaerieItemProxy Proxy) const
{
	const UVisualSlotExtension* SlotExtension;
	return FindAttachment(Proxy, SlotExtension);
}

FEquipmentVisualAttachment UEquipmentVisualizer::FindAttachment(const FFaerieItemProxy Proxy, const UVisualSlotExtension*& SlotExtension) const
{
	FEquipmentVisualAttachment Attachment;

	const IFaerieContainerExtensionInterface* Container = Cast<IFaerieContainerExtensionInterface>(Proxy->GetItemOwner().GetObject());

	// If there is a VisualSlotExtension on this container, then defer to it.
	SlotExtension = Faerie::GetExtension<UVisualSlotExtension>(Container, true);

	AActor* ParentActor = nullptr;
	USceneComponent* ParentComponent = nullptr;

	// See if we are owned by a slot, and try to determine attachment to it.
	const UFaerieEquipmentSlot* OwningSlot = Cast<UObject>(Container)->GetTypedOuter<UFaerieEquipmentSlot>();
	if (IsValid(OwningSlot))
	{
		UObject* Visual = GetSpawnedVisualByKey({ OwningSlot->Proxy() });

		if (AActor* Actor = Cast<AActor>(Visual))
		{
			ParentActor = Actor;
		}
		else if (USceneComponent* Component = Cast<USceneComponent>(Visual))
		{
			ParentComponent = Component;
		}

		if (IsValid(ParentComponent))
		{
			Attachment.Parent = ParentComponent;
		}
		else if (IsValid(ParentActor))
		{
			Attachment.Parent = ParentComponent;
		}
	}
	else
	{
		// In the case of no owning slot, use the parent actor.
		ParentActor = GetOwner();
	}

	// If we directly found a Component, great, use it!
	if (IsValid(ParentComponent))
	{
		if (UFaerieItemMeshComponent* ItemMeshComponent = Cast<UFaerieItemMeshComponent>(ParentComponent))
		{
			// If the Generated Mesh already exists, use it.
			if (ItemMeshComponent->GetGeneratedMeshComponent())
			{
				Attachment.Parent = ItemMeshComponent->GetGeneratedMeshComponent();
			}
			// If it doesn't, then just return the ItemMeshComponent, and have CreateVisualImpl move us to Pending.
			else
			{
				Attachment.Parent = ItemMeshComponent;
			}
		}
		else
		{
			Attachment.Parent = ParentComponent;
		}
	}
	// If we only found an Actor, determine the component to use.
	else if (IsValid(ParentActor))
	{
		// First choice is the one specified by the Extension (if applicable)
		if (auto Component = IsValid(SlotExtension)
				? ParentActor->FindComponentByTag<USceneComponent>(SlotExtension->GetComponentTag()) : nullptr;
			IsValid(Component))
		{
			Attachment.Parent = Component;
		}
		else if (const ACharacter* Character = Cast<ACharacter>(ParentActor))
		{
			Attachment.Parent = Character->GetMesh();
		}
		else
		{
			Attachment.Parent = ParentActor->GetDefaultAttachComponent();

			if (UFaerieItemMeshComponent* ItemMeshComponent = Cast<UFaerieItemMeshComponent>(Attachment.Parent.Get()))
			{
				// If the Generated Mesh already exists, use it.
				if (ItemMeshComponent->GetGeneratedMeshComponent())
				{
					Attachment.Parent = ItemMeshComponent->GetGeneratedMeshComponent();
				}
				// If it doesn't, then just return the ItemMeshComponent, and have CreateVisualImpl move us to Pending.
				else
				{
					Attachment.Parent = ItemMeshComponent;
				}
			}
		}
	}

	if (IsValid(SlotExtension))
	{
		Attachment.ParentSocket = SlotExtension->GetSocket();
		Attachment.ChildSocket = SlotExtension->GetChildSocket();
	}

	return Attachment;
}

void UEquipmentVisualizer::ResetAttachment(const FFaerieVisualKey Key)
{
	const FEquipmentVisualMetadata* Metadata = KeyedMetadata.Find(Key);
	if (!Metadata) return;

	if (AActor* Visual = GetSpawnedActorByKey(Key))
	{
		Faerie::UpdateActorAttachment(Visual, Metadata->Attachment);
	}
	else if (USceneComponent* VisualComponent = GetSpawnedComponentByKey(Key))
	{
		Faerie::UpdateComponentAttachment(VisualComponent, Metadata->Attachment);
	}
}

void UEquipmentVisualizer::MoveAttachment(const FFaerieVisualKey Key, const FEquipmentVisualAttachment& Attachment)
{
	if (AActor* Visual = GetSpawnedActorByKey(Key))
	{
		Faerie::UpdateActorAttachment(Visual, Attachment);
	}
	else if (USceneComponent* VisualComponent = GetSpawnedComponentByKey(Key))
	{
		Faerie::UpdateComponentAttachment(VisualComponent, Attachment);
	}
}

void UEquipmentVisualizer::UpdateAttachment(const FFaerieVisualKey Key, const FEquipmentVisualAttachment& Attachment)
{
	if (FEquipmentVisualMetadata* Metadata = KeyedMetadata.Find(Key))
	{
		Metadata->Attachment = Attachment;
		ResetAttachment(Key);
	}
}

void UEquipmentVisualizer::AwaitOrReceiveUpdate(const FFaerieVisualKey Key, const FEquipmentVisualizerCallback Callback)
{
	// Set callback for any future changes to this key
	KeyedMetadata.FindOrAdd(Key).ChangeCallback.Add(Callback);

	// If a visual by this already exists, execute the callback right now.
	if (auto&& Visual = GetSpawnedVisualByKey(Key))
	{
		Callback.Execute(Key, Visual);
	}
}

FFaerieVisualKey UEquipmentVisualizer::MakeVisualKeyFromProxy(const TScriptInterface<IFaerieItemDataProxy>& Proxy)
{
	return { Proxy.GetInterface() };
}

void UEquipmentVisualizer::OnVisualActorDestroyed(AActor* DestroyedActor)
{
	if (const FFaerieVisualKey* Key = ReverseMap.Find(DestroyedActor))
	{
		SpawnedActors.Remove(*Key);
		OnAnyVisualDestroyedNative.Broadcast(*Key);
		OnAnyVisualDestroyed.Broadcast(*Key);
	}

	ReverseMap.Remove(DestroyedActor);
}

/*
void UEquipmentVisualizer::OnVisualComponentDestroyed(USceneComponent* DestroyedComponent)
{
	if (const auto Key = ReverseMap.Find(DestroyedComponent))
	{
		SpawnedComponents.Remove(*Key);
		OnAnyVisualDestroyedNative.Broadcast(*Key);
		OnAnyVisualDestroyed.Broadcast(*Key);
	}

	ReverseMap.Remove(DestroyedComponent);
}
*/