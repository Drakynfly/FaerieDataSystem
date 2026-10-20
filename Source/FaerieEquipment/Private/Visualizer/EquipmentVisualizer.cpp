// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Visualizer/EquipmentVisualizer.h"
#include "Visualizer/EquipmentVisualizationUpdater.h"

#include "FaerieEquipmentLog.h"
#include "FaerieEquipmentManager.h"
#include "FaerieItemStackContainer.h"
#include "EntityManagerHelpers.h"
#include "FaerieItemOwnership.h"

#include "Actors/FaerieProxyActorBase.h"

#include "Components/FaerieItemMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Engine/World.h"

#include "Fragments/FaerieActorFragment.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"

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

	void UpdateActorAttachment(AActor* Visual, const FFaerieVisualAttachment& Metadata)
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

	void UpdateComponentAttachment(USceneComponent* Visual, const FFaerieVisualAttachment& Metadata)
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

FFaerieVisualSlotElement UEquipmentVisualizer::GetDefaultVisualSlotElement() const
{
	return FFaerieVisualSlotElement{
		.AllowLeaderPose = false
	};
}

USkinnedMeshComponent* UEquipmentVisualizer::GetLeaderComponent() const
{
	return Cast<USkinnedMeshComponent>(LeaderPoseComponent.GetComponent(GetOwner()));
}

bool UEquipmentVisualizer::HasVisualForKey(const FFaerieVisualKey& Key) const
{
	return SpawnedActors.Contains(Key) || SpawnedComponents.Contains(Key);
}

void UEquipmentVisualizer::ResetAttachment(const FFaerieVisualKey& Key, const FFaerieVisualAttachment& Attachment)
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

void UEquipmentVisualizer::PostVisualComponentLoaded(const FFaerieVisualKey& Key, const TNotNull<UFaerieItemMeshComponent*> Component)
{
	bool FoundSelf = false;

	for (auto&& It = Pending.CreateIterator(); It; ++It)
	{
		// Find the pending attachment for ourself
		if (It->Key == Key)
		{
			It->ChildReady = true;
			FoundSelf = true;
		}

		// Find a pending attachment with ourself as its parent.
		if (It->Attachment.Parent == Component)
		{
			// Update the parent with our correct component.
			It->Attachment.Parent = Component->GetGeneratedMeshComponent();
			It->ParentReady = true;
		}

		if (It->ParentReady && It->ChildReady)
		{
			It->Attachment.Hidden = false;
			UpdateAttachment(It->Key, It->Attachment);
			It.RemoveCurrentSwap();
		}
	}

	if (!FoundSelf)
	{
		// If we were not a pending child attachment, then it's safe to just immediately fixup our attachment.
		FFaerieVisualMetadata& Metadata = KeyedMetadata.FindOrAdd(Key);
		Metadata.Attachment.Hidden = false;
		ResetAttachment(Key, Metadata.Attachment);
	}
}

void UEquipmentVisualizer::PostVisualActionLoaded(const FFaerieVisualKey& Key, const TNotNull<AFaerieProxyActorBase*> Actor)
{
	bool FoundSelf = false;

	for (auto&& It = Pending.CreateIterator(); It; ++It)
	{
		// Find the pending attachment for ourself
		if (It->Key == Key)
		{
			It->ChildReady = true;
			FoundSelf = true;
		}

		// Find a pending attachment with ourself as its parent.
		if (It->Attachment.Parent->GetOwner() == Actor)
		{
			// Update the parent with our correct component.
			It->Attachment.Parent = Actor->GetDefaultAttachComponent();
			It->ParentReady = true;
		}

		if (It->ParentReady && It->ChildReady)
		{
			It->Attachment.Hidden = false;
			UpdateAttachment(It->Key, It->Attachment);
			It.RemoveCurrentSwap();
		}
	}

	if (!FoundSelf)
	{
		// If we were not a pending child attachment, then it's safe to just immediately fixup our attachment.
		FFaerieVisualMetadata& Metadata = KeyedMetadata.FindOrAdd(Key);
		Metadata.Attachment.Hidden = false;
		ResetAttachment(Key, Metadata.Attachment);
	}
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
	UObject* Out = GetSpawnedActorByKey(Key);

	if (!IsValid(Out))
	{
		Out = GetSpawnedComponentByKey(Key);
	}

	return Out;
}

AActor* UEquipmentVisualizer::GetSpawnedActorByKey(const FFaerieVisualKey& Key) const
{
	if (const TObjectPtr<AActor>* Found = SpawnedActors.Find(Key))
	{
		return *Found;
	}
	return nullptr;
}

USceneComponent* UEquipmentVisualizer::GetSpawnedComponentByKey(const FFaerieVisualKey& Key) const
{
	if (const TObjectPtr<USceneComponent>* Found = SpawnedComponents.Find(Key))
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

void UEquipmentVisualizer::FlushInvalid(FMassEntityManager& EntityManager)
{
	TArray<FFaerieVisualKey> InvalidKeys;
	for (auto It = SpawnedComponents.CreateConstIterator(); It; ++It)
	{
		if (!It.Key().GetProxy().IsValid())
		{
			InvalidKeys.Add(It.Key());
		}
	}
	for (auto&& InvalidKey : InvalidKeys)
	{
		RemoveVisualImpl(EntityManager, InvalidKey);
	}
}

void UEquipmentVisualizer::RefreshVisual(FMassEntityManager& EntityManager, const FFaerieItemProxy& Proxy)
{
	if (!Proxy.HasValidInstance())
	{
		RemoveVisualImpl(EntityManager, Proxy);
	}
	else
	{
		// Remove and recreate visual
		if (HasVisualForKey(Proxy))
		{
			RemoveVisualImpl(EntityManager, Proxy);
			return;
		}
		CreateVisualImpl(EntityManager, Proxy);
	}
}

void UEquipmentVisualizer::CreateVisualImpl(FMassEntityManager& EntityManager, TValid<const FFaerieItemProxy&> Proxy, TOptional<const FFaerieItemProxy*> Parent)
{
	const FFaerieVisualKey Key{Proxy};
	if (HasVisualForKey(Key))
	{
		static constexpr bool RemoveThisBoolAndActuallyTrackDownThisWarningLater = true;
		if (RemoveThisBoolAndActuallyTrackDownThisWarningLater)
		{
			RemoveVisualImpl(EntityManager, Key);
		}
		else
		{
			UE_LOGF(LogFaerieEquipment, Warning, "Container already has an visual spawned. Existing visuals must be removed before creating new ones!");
			return;
		}
	}

	// Step 1: Figure out what we are attaching to.
	const TOptional<FFaerieVisualSlotElement> VisualSlotDataOpt = FindVisualSlotDataFromProxy(EntityManager, Proxy);
	FFaerieVisualSlotElement VisualSlotData = VisualSlotDataOpt.IsSet() ? VisualSlotDataOpt.GetValue() : GetDefaultVisualSlotElement();
	bool WantsToWaitForParentVisual = false;
	USceneComponent* AttachTo = GetComponentToAttachTo(EntityManager, Proxy, VisualSlotData.ComponentTag, WantsToWaitForParentVisual);
	if (WantsToWaitForParentVisual)
	{
		UE_LOGF(LogFaerieEquipment, Log, "CreateVisualImpl called for proxy %ls which has a parent to wait for. Skipping!", *GetNameSafe(ValidGet(Proxy).GetProxyObject()))
		return;
	}

	FFaerieVisualAttachment Attachment = BuildAttachmentData(AttachTo, VisualSlotData);

	// GetComponentToAttachTo will return a UFaerieItemMeshComponent when it wants us to defer for a pending attaching.
	if (IsValid(AttachTo) && AttachTo->IsA<UFaerieItemMeshComponent>())
	{
		// Enable hidden while in Pending. This allows this attachment to still start async loading itself, even while not attached.
		Attachment.Hidden = true;
		Pending.Emplace(Key, Attachment);
	}

#if FAERIE_DEBUG
	if (GetOwner()->GetLocalRole() == ROLE_Authority)
	{
		UE_LOGF(LogFaerieEquipment, Verbose, "Creating equipment visual for server!")
	}
	else
	{
		UE_LOGF(LogFaerieEquipment, Verbose, "Creating equipment visual for client!")
	}
#endif

	// If a parent was passed in, set it into metadata for this key.
	if (Parent.IsSet())
	{
		KeyedMetadata.FindOrAdd(Key).Parent = *Parent.GetValue();
	}

	// Step 2: What are we creating as a visual.
	const TOptional<FFaerieItemInstance> InstanceOption = ValidGet(Proxy).GetItemInstance();
	if (!InstanceOption.IsSet())
	{
		return;
	}
	const FFaerieItemInstance& Instance = InstanceOption.GetValue();

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
						if (AFaerieProxyActorBase* Actor = Visual.Get())
						{
							PostVisualActionLoaded(Key, Actor);
						}
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
			NewVisual->SetIsReplicated(false); // Do not replicate mesh components. Server and client should each create their own.
			NewVisual->GetOnMeshRebuilt().AddWeakLambda(this,
				[this, Key](const TNotNull<UFaerieItemMeshComponent*> ItemMeshComponent)
				{
					PostVisualComponentLoaded(Key, ItemMeshComponent);
				});
			NewVisual->SetItemMeshFromProxy(Proxy);
		}
	}

	// Step 3: Recurse over children
	const FMassEntityHandle Entity = Instance.GetMassEntityHandle();
	if (ItemData::IsMutable(EntityManager, Entity))
	{
		for (auto SubContainer : Equipment::SlotFilter.Iterate(EntityManager, Entity))
		{
			const FFaerieEntryKey EntryKey = SubContainer->GetCurrentKey();
			if (EntryKey.IsValid())
			{
				CreateVisualImpl(EntityManager, FFaerieItemProxy(SubContainer), &ValidGet(Proxy));
			}
		}
	}
}

void UEquipmentVisualizer::RemoveVisualImpl(FMassEntityManager& EntityManager, const FFaerieVisualKey& Key)
{
	// Before we remove a visual, we should remove its children first.
	for (auto&& Metadatum : KeyedMetadata)
	{
		if (Metadatum.Value.Parent == Key)
		{
			RemoveVisualImpl(EntityManager, Metadatum.Key);
			Metadatum.Value.Parent = FFaerieVisualKey();
		}
	}

	DestroyVisualByKey(Key);
}

AActor* UEquipmentVisualizer::SpawnVisualActor(const FFaerieVisualKey Key, const TSubclassOf<AActor>& Class, const FFaerieVisualAttachment& Attachment)
{
	if (!Key.GetProxy().IsValid())
	{
		UE_LOGF(LogFaerieEquipment, Error, "Attempted to spawn a VisualActor using an invalid key!")
		return nullptr;
	}
	if (!IsValid(Class))
	{
		UE_LOGF(LogFaerieEquipment, Error, "Attempted to spawn a VisualActor with an invalid class!")
		return nullptr;
	}
	if (SpawnedActors.Contains(Key))
	{
		UE_LOGF(LogFaerieEquipment, Error, "Attempted to spawn a VisualActor using a key already in use!")
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!ensure(IsValid(World)))
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
	const FFaerieVisualAttachment& Attachment)
{
	if (!Key.GetProxy().IsValid())
	{
		UE_LOGF(LogFaerieEquipment, Error, "Attempted to spawn a VisualComponent using an invalid key!")
		return nullptr;
	}
	if (!IsValid(Class))
	{
		UE_LOGF(LogFaerieEquipment, Error, "Attempted to spawn a VisualComponent with an invalid class!")
		return nullptr;
	}
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

FFaerieVisualAttachment UEquipmentVisualizer::FindAttachment(const FFaerieItemProxy& Proxy) const
{
	const FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());
	TOptional<FFaerieVisualSlotElement> SlotDataOpt = FindVisualSlotDataFromProxy(EntityManager, Proxy);
	const FFaerieVisualSlotElement SlotData = SlotDataOpt.IsSet() ? SlotDataOpt.GetValue() : GetDefaultVisualSlotElement();
	bool WantsToWaitForParentVisual;
	USceneComponent* AttachTo = GetComponentToAttachTo(EntityManager, Proxy, SlotData.ComponentTag, WantsToWaitForParentVisual);
	return BuildAttachmentData(AttachTo, SlotData);
}

FFaerieVisualAttachment UEquipmentVisualizer::BuildAttachmentData(USceneComponent* AttachTo, const FFaerieVisualSlotElement& SlotData)
{
	FFaerieVisualAttachment Attachment;
	Attachment.Parent = AttachTo;
	Attachment.ParentSocket = SlotData.Socket;
	Attachment.ChildSocket = SlotData.ChildSocket;
	return Attachment;
}

TOptional<FFaerieVisualSlotElement> UEquipmentVisualizer::FindVisualSlotDataFromProxy(const FMassEntityManager& EntityManager, const FFaerieItemProxy& Proxy)
{
	if (const UFaerieItemContainerBase* Container = Cast<UFaerieItemContainerBase>(Proxy.GetItemOwner()))
	{
		auto* Updater = Container->ReadContainerData<FFaerieContainerExtensionVisualUpdater>(EntityManager, true);
		auto& SlotTag = Container->ReadContainerDataChecked<FFaerieContainerDataSlotTag>(EntityManager).SlotTag;
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

USceneComponent* UEquipmentVisualizer::GetComponentToAttachTo(const FMassEntityManager& EntityManager, const FFaerieItemProxy& Proxy, const FName& ComponentTag, bool& WantsToWaitForParentVisual) const
{
	const UFaerieItemContainerBase* Container = Cast<UFaerieItemContainerBase>(Proxy.GetItemOwner());

	AActor* ParentActor = nullptr;
	USceneComponent* ParentComponent = nullptr;

	USceneComponent* AttachTo = nullptr;

	// See if we are owned by another item, and try to determine attachment to it.
	FFaerieItemProxy ParentItem;
	if (const FMassEntityHandle ParentItemHandle = Container::GetContainerOwner(EntityManager, Container);
		ParentItemHandle.IsValid())
	{
		const UFaerieItemContainerBase* ParentContainer = Container::GetItemOwner(EntityManager, ParentItemHandle);
		if (IsValid(ParentContainer))
		{
			ParentItem = ParentContainer->Proxy(ParentItemHandle);
		}
	}

	// If we have a parent, see if a visual already exists
	if (UObject* Visual = GetSpawnedVisualByKey({ ParentItem });
		IsValid(Visual))
	{
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
			AttachTo = ParentComponent;
		}
		else if (IsValid(ParentActor))
		{
			AttachTo = ParentComponent;
		}
	}
	else
	{
		// If the parent item is valid, but there is no visual for it yet, alert that we would prefer to attach later.
		WantsToWaitForParentVisual = ParentItem.IsValid();

		// In the case of no owning container, use the parent actor.
		ParentActor = GetOwner();
	}

	// If we directly found a Component, great, use it!
	if (IsValid(ParentComponent))
	{
		AttachTo = ParentComponent;
	}
	// If we only found an Actor, determine the component to use.
	else if (IsValid(ParentActor))
	{
		// First choice is the one specified by the Extension (if applicable)
		if (USceneComponent* Component = ParentActor->FindComponentByTag<USceneComponent>(ComponentTag);
			IsValid(Component))
		{
			AttachTo = Component;
		}
		else if (const ACharacter* Character = Cast<ACharacter>(ParentActor))
		{
			AttachTo = Character->GetMesh();
		}
		else
		{
			AttachTo = ParentActor->GetDefaultAttachComponent();
		}
	}

	if (UFaerieItemMeshComponent* ItemMeshComponent = Cast<UFaerieItemMeshComponent>(AttachTo))
	{
		// If the Generated Mesh already exists, use it.
		if (ItemMeshComponent->GetGeneratedMeshComponent())
		{
			AttachTo = ItemMeshComponent->GetGeneratedMeshComponent();
		}
		// If it doesn't, then just return the ItemMeshComponent, and have CreateVisualImpl move us to Pending.
		else
		{
			AttachTo = ItemMeshComponent;
		}
	}

	UE_CLOGF(!IsValid(AttachTo), LogFaerieEquipment, Error, "GetComponentToAttachTo failed to find valid component to attach to");
	return AttachTo;
}

void UEquipmentVisualizer::ResetAttachment(const FFaerieVisualKey Key)
{
	const FFaerieVisualMetadata* Metadata = KeyedMetadata.Find(Key);
	if (!Metadata) return;
	ResetAttachment(Key, Metadata->Attachment);
}

void UEquipmentVisualizer::MoveAttachment(const FFaerieVisualKey Key, const FFaerieVisualAttachment& Attachment)
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

void UEquipmentVisualizer::UpdateAttachment(const FFaerieVisualKey Key, const FFaerieVisualAttachment& Attachment)
{
	if (FFaerieVisualMetadata* Metadata = KeyedMetadata.Find(Key))
	{
		Metadata->Attachment = Attachment;
		ResetAttachment(Key, Metadata->Attachment);
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

void UEquipmentVisualizer::DumpDebugLog() const
{
#if FAERIE_DEBUG
	auto GetAttachmentString = [](const FFaerieVisualAttachment& Attachment) -> FString
	{
		const FString HiddenStr = Attachment.Hidden ? TEXT(" (hidden)") : TEXT("");
		return FString::Printf(TEXT("Parent: '%ls', ParentSocket: %ls, ChildSocket: %ls%ls"),
			*GetNameSafe(Attachment.Parent.Get()), *Attachment.ParentSocket.ToString(), *Attachment.ChildSocket.ToString(), *HiddenStr);
	};

	UE_LOGF(LogFaerieEquipment, Warning, "Start Equipment Visualizer debug log for '%ls'", *GetNameSafe(this))

	UE_LOGF(LogFaerieEquipment, Log, "PreferredTag: %ls", *PreferredTag.ToString())
	UE_LOGF(LogFaerieEquipment, Log, "LeaderComponent: '%ls'", *GetNameSafe(GetLeaderComponent()))

	UE_LOGF(LogFaerieEquipment, Log, "Num SpawnedActors: '%i'", SpawnedActors.Num())
	for (auto&& SpawnedActor : SpawnedActors)
	{
		FFaerieVisualKey Key = SpawnedActor.Key;
		TObjectPtr<AActor> Value = SpawnedActor.Value;
		UE_LOGF(LogFaerieEquipment, Log, "--- Visual Actor: '%ls' spawned for '%ls'", *GetNameSafe(Value), *GetNameSafe(Key.GetProxy().GetProxyObject()))
		if (const FFaerieVisualMetadata* Metadata = KeyedMetadata.Find(Key))
		{
			UE_LOGF(LogFaerieEquipment, Log, "    Parent: '%ls'", *GetNameSafe(Metadata->Parent.GetProxy().GetProxyObject()))
			UE_LOGF(LogFaerieEquipment, Log, "    Attachment: '%ls'", *GetAttachmentString(Metadata->Attachment))
		}
	}

	UE_LOGF(LogFaerieEquipment, Log, "Num SpawnedComponents: '%i'", SpawnedComponents.Num())
	for (auto&& SpawnedComponent : SpawnedComponents)
	{
		FFaerieVisualKey Key = SpawnedComponent.Key;
		TObjectPtr<USceneComponent> Value = SpawnedComponent.Value;
		UE_LOGF(LogFaerieEquipment, Log, "--- Visual Component: '%ls' spawned for '%ls'", *GetNameSafe(Value), *GetNameSafe(Key.GetProxy().GetProxyObject()))
		if (const FFaerieVisualMetadata* Metadata = KeyedMetadata.Find(Key))
		{
			UE_LOGF(LogFaerieEquipment, Log, "      Parent: '%ls'", *GetNameSafe(Metadata->Parent.GetProxy().GetProxyObject()))
			UE_LOGF(LogFaerieEquipment, Log, "      Attachment: '%ls'", *GetAttachmentString(Metadata->Attachment))
		}
	}

	UE_LOGF(LogFaerieEquipment, Log, "Num Pending Attachments: '%i'", Pending.Num())
	for (const FPendingAttachment& PendingAttachment : Pending)
	{
		UE_LOGF(LogFaerieEquipment, Log, "--- Pending: '%ls'", *GetNameSafe(PendingAttachment.Key.GetProxy().GetProxyObject()))
		UE_LOGF(LogFaerieEquipment, Log, "      Attachment: '%ls'", *GetAttachmentString(PendingAttachment.Attachment))
	}

	UE_LOGF(LogFaerieEquipment, Warning, "End Equipment Visualizer debug log")
#endif
}

void UEquipmentVisualizer::RefreshAllAttachments()
{
	for (auto&& Metadata : KeyedMetadata)
	{
		ResetAttachment(Metadata.Key);
	}
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