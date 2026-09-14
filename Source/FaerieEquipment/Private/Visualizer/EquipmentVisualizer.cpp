// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Visualizer/EquipmentVisualizer.h"

#include "FaerieEquipmentLog.h"
#include "FaerieEquipmentManager.h"
#include "FaerieItemStackContainer.h"

#include "Actors/FaerieProxyActorBase.h"

#include "Components/FaerieItemMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Engine/World.h"

#include "Fragments/FaerieActorFragment.h"

#include "EntityManagerHelpers.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"

#include "Visualizer/EquipmentVisualizationUpdater.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EquipmentVisualizer)

using namespace Faerie;

namespace Faerie::Equipment
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

UObject* UEquipmentVisualizer::GetSpawnedVisualByKey(const FFaerieVisualKey& Key) const
{
	if (!Key.IsValid()) return nullptr;

	UObject* Out = GetSpawnedActorByKey(Key);

	if (!IsValid(Out))
	{
		Out = GetSpawnedComponentByKey(Key);
	}

	return Out;
}

AActor* UEquipmentVisualizer::GetSpawnedActorByKey(const FFaerieVisualKey& Key) const
{
	if (!Key.IsValid()) return nullptr;

	if (auto&& Found = SpawnedActors.Find(Key))
	{
		return *Found;
	}
	return nullptr;
}

USceneComponent* UEquipmentVisualizer::GetSpawnedComponentByKey(const FFaerieVisualKey& Key) const
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


void UEquipmentVisualizer::CreateVisualImpl(TValid<const FFaerieItemProxy&> Proxy, const FFaerieItemProxy* Parent)
{
	const FFaerieVisualKey Key{Proxy};
	if (HasVisualForKey(Key))
	{
		UE_LOGF(LogFaerieEquipment, Warning, "Container already has an visual spawned. Existing visuals must be removed before creating new ones!");
		return;
	}

	// Step 1: Figure out what we are attaching to.

	const TOptional<FFaerieVisualSlotElement> VisualSlotDataOpt = FindVisualSlotDataFromProxy(Proxy);
	if (!VisualSlotDataOpt.IsSet())
	{
		UE_LOGF(LogFaerieEquipment, Warning, "No visual slot data found from proxy!");
		return;
	}
	const FFaerieVisualSlotElement& VisualSlotData = VisualSlotDataOpt.GetValue();
	FEquipmentVisualAttachment Attachment = BuildAttachmentData(Proxy, VisualSlotData);

	// FindAttachmentParent will return a UFaerieItemMeshComponent when it wants us to defer for a pending attaching.
	if (Attachment.Parent->IsA<UFaerieItemMeshComponent>())
	{
		// Enable hidden while in Pending. This allows this attachment to still start async loading itself, even while not attached.
		Attachment.Hidden = true;
		Pending.Emplace(Key, Attachment);
	}

	// Step 2: What are we creating as a visual.
	const TOptional<FFaerieItemInstance> InstanceOption = ValidGet(Proxy).GetItemInstance();
	if (!InstanceOption.IsSet())
	{
		return;
	}
	const FFaerieItemInstance& Instance = InstanceOption.GetValue();

	auto& EntityManager = ItemData::GetFaerieEntityManagerChecked();

	// Path 1: A Visual Actor
	{
		TSoftClassPtr<AFaerieProxyActorBase> ActorClass = nullptr;

		auto ProxyClassFragment = Faerie::ItemData::GetEntityFragmentOrDefault<FFaerieProxyActorFragment>(&EntityManager, Instance);
		if (ProxyClassFragment.IsValid())
		{
			ActorClass = ProxyClassFragment->ProxyActorClass;
		}

		if (!ActorClass.IsNull())
		{
			// @todo implement async path here
			const TSubclassOf<AFaerieProxyActorBase> VisualClass = ActorClass.LoadSynchronous();
			if (!IsValid(VisualClass))
			{
				UE_LOGF(LogFaerieEquipment, Warning, "VisualClass failed to load!")
				return;
			}

			AFaerieProxyActorBase* NewVisual = SpawnVisualActorNative<AFaerieProxyActorBase>(
				Key, VisualClass, Attachment);
			if (IsValid(NewVisual))
			{
				NewVisual->GetOnDisplayFinished().AddWeakLambda(this,
					[this, Key, Visual = TWeakObjectPtr<AFaerieProxyActorBase>(NewVisual)](bool Success)
					{
						if (!Visual.IsValid()) return;

						for (auto&& It = Pending.CreateIterator(); It; ++It)
						{
							if (It->Attachment.Parent->GetOwner() == Visual)
							{
								It->Attachment.Parent = Visual->GetDefaultAttachComponent();
								It->Attachment.Hidden = false;
								UpdateAttachment(It->Key, It->Attachment);
								It.RemoveCurrentSwap();
								return;
							}
						}

						// If this wasn't pending, just update its attachment after a rebuild.
						ResetAttachment(Key);
					});
				NewVisual->SetSourceProxy(Proxy);
				return;
			}
		}
	}

	// Path 2: A Visual Component
	{
		FGameplayTag CompPreferredTag = Mesh::Tags::MeshPurpose_Default;
		if (PreferredTag.IsValid() &&
			ensure(PreferredTag.GetTagName().IsValid()))
		{
			CompPreferredTag = PreferredTag;
		}

		// Some extensions might ban leader poses (like items held in hands)
		if (VisualSlotData.AllowLeaderPose)
		{
			// Reset attachment location to main mesh when using LeaderPose.
			Attachment.Parent = Cast<ACharacter>(GetOwner())->GetMesh();
			Attachment.ParentSocket = NAME_None;
			Attachment.ChildSocket = NAME_None;
		}

		if (VisualSlotData.PreferredTag.IsValid() &&
			ensure(VisualSlotData.PreferredTag.GetTagName().IsValid()))
		{
			CompPreferredTag = VisualSlotData.PreferredTag;
		}

		UFaerieItemMeshComponent* NewVisual = SpawnVisualComponentNative<UFaerieItemMeshComponent>(Key,
			UFaerieItemMeshComponent::StaticClass(), Attachment);
		if (IsValid(NewVisual))
		{
			// If there is no AnimClass on the mesh, it would prefer using LeaderPose as a fallback
			if (VisualSlotData.AllowLeaderPose)
			{
				NewVisual->SetSkeletalMeshLeaderPoseComponent(GetLeaderComponent());
			}

			NewVisual->SetPreferredTag(CompPreferredTag);
			NewVisual->SetIsReplicated(true); // Enable replication, as it's off by default.
			NewVisual->GetOnMeshRebuilt().AddWeakLambda(this,
				[this, Key](const TNotNull<UFaerieItemMeshComponent*> ItemMeshComponent)
				{
					for (auto&& It = Pending.CreateIterator(); It; ++It)
					{
						if (It->Attachment.Parent == ItemMeshComponent)
						{
							It->Attachment.Parent = ItemMeshComponent->GetGeneratedMeshComponent();
							It->Attachment.Hidden = false;
							UpdateAttachment(It->Key, It->Attachment);
							It.RemoveCurrentSwap();
							return;
						}
					}

					// If this wasn't pending, just update its attachment after a rebuild.
					ResetAttachment(Key);
				});
			NewVisual->SetItemMeshFromProxy(Proxy);
		}
	}

	// Step 3: Recurse over children
	if (Parent)
	{
		KeyedMetadata.FindOrAdd(Key).Parent = *Parent;
	}

	if (Instance.IsMutable())
	{
		for (auto SubContainer : Equipment::SlotFilter.Iterate(EntityManager, Instance))
		{
			auto EntryKey = SubContainer->GetCurrentKey();
			if (EntryKey.IsValid())
			{
				CreateVisualImpl(FFaerieItemProxy(SubContainer), &ValidGet(Proxy));
			}
		}
	}
}

void UEquipmentVisualizer::RemoveVisualImpl(const TValid<const FFaerieItemProxy&> Proxy)
{
	const FFaerieVisualKey Key{Proxy};

	// Before we remove a visual, we should remove its children first.
	for (auto&& Metadatum : KeyedMetadata)
	{
		if (Metadatum.Value.Parent == Key)
		{
			RemoveVisualImpl(Metadatum.Key.Proxy);
			Metadatum.Value.Parent = FFaerieVisualKey();
		}
	}

	DestroyVisualByKey(Key);
}

AActor* UEquipmentVisualizer::SpawnVisualActor(const FFaerieVisualKey Key, const TSubclassOf<AActor>& Class, const FEquipmentVisualAttachment& Attachment)
{
	if (!Key.IsValid()) return nullptr;
	if (!IsValid(Class)) return nullptr;

	if (SpawnedActors.Contains(Key))
	{
		UE_LOGF(LogFaerieEquipment, Error, "Attempted to spawn a VisualActor using a key already in use!")
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
			Equipment::UpdateActorAttachment(NewActor, Attachment);

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
		UE_LOGF(LogFaerieEquipment, Error, "Attempted to spawn a VisualComponent using a key already in use!")
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

		Equipment::UpdateComponentAttachment(NewComponent, Attachment);

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

FEquipmentVisualAttachment UEquipmentVisualizer::FindAttachment(const FFaerieItemProxy& Proxy) const
{
	TOptional<FFaerieVisualSlotElement> SlotData = FindVisualSlotDataFromProxy(Proxy);
	if (!SlotData.IsSet())
	{
		return FEquipmentVisualAttachment();
	}
	return BuildAttachmentData(Proxy, SlotData.GetValue());
}

FEquipmentVisualAttachment UEquipmentVisualizer::BuildAttachmentData(const FFaerieItemProxy& Proxy, const FFaerieVisualSlotElement& SlotData) const
{
	FEquipmentVisualAttachment Attachment;

	const UFaerieItemContainerBase* Container = Cast<UFaerieItemContainerBase>(Proxy.GetItemOwner());

	AActor* ParentActor = nullptr;
	USceneComponent* ParentComponent = nullptr;

	// See if we are owned by a slot, and try to determine attachment to it.
	const UFaerieItemStackContainer* OwningSlot = Cast<UObject>(Container)->GetTypedOuter<UFaerieItemStackContainer>();
	if (IsValid(OwningSlot))
	{
		UObject* Visual = GetSpawnedVisualByKey({ FFaerieItemProxy(OwningSlot) });

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
		if (USceneComponent* Component = ParentActor->FindComponentByTag<USceneComponent>(SlotData.ComponentTag);
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

	Attachment.ParentSocket = SlotData.Socket;
	Attachment.ChildSocket = SlotData.ChildSocket;

	return Attachment;
}

TOptional<FFaerieVisualSlotElement> UEquipmentVisualizer::FindVisualSlotDataFromProxy(const FFaerieItemProxy& Proxy)
{
	if (const UFaerieItemContainerBase* Container = Cast<UFaerieItemContainerBase>(Proxy.GetItemOwner()))
	{
		auto* Updater = Container->ReadContainerData<FFaerieContainerExtensionVisualUpdater>(true);
		auto& SlotTag = Container->ReadContainerDataChecked<FFaerieContainerDataSlotTag>().SlotTag;
		if (Updater && Updater->Config)
		{
			if (const FFaerieVisualSlotElement* Element = Updater->Config->GetElements().Find(SlotTag))
			{
				return *Element;
			}
		}
	}
	return NullOpt;
}

void UEquipmentVisualizer::ResetAttachment(const FFaerieVisualKey Key)
{
	const FEquipmentVisualMetadata* Metadata = KeyedMetadata.Find(Key);
	if (!Metadata) return;

	if (AActor* Visual = GetSpawnedActorByKey(Key))
	{
		Equipment::UpdateActorAttachment(Visual, Metadata->Attachment);
	}
	else if (USceneComponent* VisualComponent = GetSpawnedComponentByKey(Key))
	{
		Equipment::UpdateComponentAttachment(VisualComponent, Metadata->Attachment);
	}
}

void UEquipmentVisualizer::MoveAttachment(const FFaerieVisualKey Key, const FEquipmentVisualAttachment& Attachment)
{
	if (AActor* Visual = GetSpawnedActorByKey(Key))
	{
		Equipment::UpdateActorAttachment(Visual, Attachment);
	}
	else if (USceneComponent* VisualComponent = GetSpawnedComponentByKey(Key))
	{
		Equipment::UpdateComponentAttachment(VisualComponent, Attachment);
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

FFaerieVisualKey UEquipmentVisualizer::MakeVisualKey(const FFaerieItemProxy& Proxy)
{
	return { Proxy };
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