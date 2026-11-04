// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Visualizer/EquipmentVisualizer.h"
#include "Visualizer/EquipmentVisualizationUpdater.h"

#include "FaerieEquipmentLog.h"
#include "FaerieEquipmentManager.h"
#include "FaerieItemStackContainer.h"
#include "EntityManagerHelpers.h"
#include "FaerieMassEntity.h"
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

	void UpdateActorAttachment(AActor* Visual, const FFaerieVisualAttachment& Entry)
	{
		if (IsValid(Entry.Parent.Get()))
		{
			Visual->AttachToComponent(Entry.Parent.Get(), TempTransformRules, Entry.ParentSocket);

			USceneComponent* SelfComponent = Visual->GetDefaultAttachComponent();
			if (SelfComponent->DoesSocketExist(Entry.ChildSocket))
			{
				const FVector Offset = -SelfComponent->GetSocketTransform(Entry.ChildSocket, RTS_Component).GetTranslation();
				SelfComponent->SetRelativeLocation(FVector::Zero()); // Reset offset before adding so it doesn't drift from re-runs.
				SelfComponent->AddLocalOffset(Offset);
			}

			Visual->SetActorHiddenInGame(Entry.Hidden);
		}
		else
		{
			Visual->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		}
	}

	void UpdateComponentAttachment(USceneComponent* Visual, const FFaerieVisualAttachment& Entry)
	{
		if (IsValid(Entry.Parent.Get()))
		{
			Visual->AttachToComponent(Entry.Parent.Get(), TempTransformRules, Entry.ParentSocket);

			if (Visual->DoesSocketExist(Entry.ChildSocket))
			{
				const FVector Offset = -Visual->GetSocketTransform(Entry.ChildSocket, RTS_Component).GetTranslation();
				Visual->SetRelativeLocation(FVector::Zero()); // Reset offset before adding so it doesn't drift from re-runs.
				Visual->AddLocalOffset(Offset);
			}

			Visual->SetVisibility(!Entry.Hidden, true);
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
	for (auto&& Element : VisualData)
	{
		if (USceneComponent* VisualComponent = Cast<USceneComponent>(Element.Value.Visual))
		{
			VisualComponent->DestroyComponent();
		}
		else if (AActor* VisualActor = Cast<AActor>(Element.Value.Visual))
		{
			VisualActor->OnDestroyed.RemoveAll(this);
			VisualActor->Destroy();
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
	if (const FFaerieVisualizationEntry* Entry = VisualData.Find(Key))
	{
		return IsValid(Entry->Visual);
	}
	return false;
}

void UEquipmentVisualizer::ResetAttachment(const FFaerieVisualizationEntry& Entry)
{
	if (AActor* VisualActor = Cast<AActor>(Entry.Visual))
	{
		Equipment::UpdateActorAttachment(VisualActor, Entry.Attachment);
	}
	else if (USceneComponent* VisualComponent = Cast<USceneComponent>(Entry.Visual))
	{
		Equipment::UpdateComponentAttachment(VisualComponent, Entry.Attachment);
	}
}

void UEquipmentVisualizer::PostVisualComponentLoaded(const FFaerieVisualKey& Key, const TNotNull<UFaerieItemMeshComponent*> Component)
{
	// There are multiple possible attachments to update after a component load. Ourself obviously, but also any children
	// that waiting for us, so they can be offset correctly.

	for (auto&& It = VisualData.CreateIterator(); It; ++It)
	{
		FFaerieVisualizationEntry& Val = It.Value();

		// Find the pending attachment for ourself
		if (It->Key == Key)
		{
			Val.SelfReady = true;
			Val.ChangeCallback.Broadcast(Key, Val.Visual);
		}

		// Find a pending attachment with ourself as its parent.
		if (Val.Attachment.Parent == Component)
		{
			// Update the parent with our correct component.
			Val.Attachment.Parent = Component->GetGeneratedMeshComponent();
			Val.ParentReady = true;
		}

		// If both sides are ready now update the attachment.
		if (Val.ParentReady && Val.SelfReady)
		{
			Val.Attachment.Hidden = false;
			ResetAttachment(Val);
		}
	}
}

void UEquipmentVisualizer::PostVisualActionLoaded(const FFaerieVisualKey& Key, const TNotNull<AFaerieProxyActorBase*> Actor)
{
	// There are multiple possible attachments to update after a component load. Ourself obviously, but also any children
	// that waiting for us, so they can be offset correctly.

	for (auto&& It = VisualData.CreateIterator(); It; ++It)
	{
		FFaerieVisualizationEntry& Val = It.Value();

		// Find the pending attachment for ourself
		if (It->Key == Key)
		{
			Val.SelfReady = true;
			Val.ChangeCallback.Broadcast(Key, Val.Visual);
		}

		// Find a pending attachment with ourself as its parent.
		if (Val.Attachment.Parent->GetOwner() == Actor)
		{
			// Update the parent with our correct component.
			Val.Attachment.Parent = Actor->GetDefaultAttachComponent();
			Val.ParentReady = true;
		}

		if (Val.ParentReady && Val.SelfReady)
		{
			Val.Attachment.Hidden = false;
			ResetAttachment(Val);
		}
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

	for (auto&& Entry : VisualData)
	{
		if (Entry.Value.Visual.IsA(Class))
		{
			Key = Entry.Key;
			return Cast<AActor>(Entry.Value.Visual);
		}
	}
	return nullptr;
}

USceneComponent* UEquipmentVisualizer::GetSpawnedComponentByClass(const TSubclassOf<USceneComponent> Class, FFaerieVisualKey& Key) const
{
	if (!IsValid(Class)) return nullptr;

	for (auto&& Entry : VisualData)
	{
		if (Entry.Value.Visual.IsA(Class))
		{
			Key = Entry.Key;
			return Cast<USceneComponent>(Entry.Value.Visual);
		}
	}
	return nullptr;
}

UObject* UEquipmentVisualizer::GetSpawnedVisualByKey(const FFaerieVisualKey& Key) const
{
	if (const FFaerieVisualizationEntry* Entry = VisualData.Find(Key))
	{
		return Entry->Visual;
	}
	return nullptr;
}

AActor* UEquipmentVisualizer::GetSpawnedActorByKey(const FFaerieVisualKey& Key) const
{
	return Cast<AActor>(GetSpawnedVisualByKey(Key));
}

USceneComponent* UEquipmentVisualizer::GetSpawnedComponentByKey(const FFaerieVisualKey& Key) const
{
	return Cast<USceneComponent>(GetSpawnedVisualByKey(Key));
}

TArray<AActor*> UEquipmentVisualizer::GetSpawnedActors() const
{
	TArray<AActor*> Array;
	for (auto&& Entry : VisualData)
	{
		if (AActor* Actor = Cast<AActor>(Entry.Value.Visual))
		{
			Array.Add(Actor);
		}
	}
	return Array;
}

TArray<USceneComponent*> UEquipmentVisualizer::GetSpawnedComponents() const
{
	TArray<USceneComponent*> Array;
	for (auto&& Entry : VisualData)
	{
		if (USceneComponent* Component = Cast<USceneComponent>(Entry.Value.Visual))
		{
			Array.Add(Component);
		}
	}
	return Array;
}

void UEquipmentVisualizer::FlushInvalid(FMassEntityManager& EntityManager)
{
	TArray<FFaerieVisualKey> InvalidKeys;
	for (auto It = VisualData.CreateConstIterator(); It; ++It)
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
	if (!Proxy.HasValidStack())
	{
		RemoveVisualImpl(EntityManager, Proxy);
	}
	else
	{
		// Remove and recreate visual
		if (HasVisualForKey(Proxy))
		{
			RemoveVisualImpl(EntityManager, Proxy);
		}
		CreateVisualImpl(EntityManager, Proxy);
	}
}

void UEquipmentVisualizer::CreateVisualImpl(FMassEntityManager& EntityManager, TValid<const FFaerieItemProxy&> Proxy, TOptional<const FFaerieItemProxy*> Parent)
{
	const FFaerieVisualKey Key{Proxy};

	// Step 1: Figure out what we are attaching to.

	const UFaerieItemContainerBase* OwningContainer = Container::GetItemOwner(Proxy);
	if (!IsValid(OwningContainer))
	{
		UE_LOGF(LogFaerieEquipment, Error, "CreateVisualImpl called for proxy %ls which has no owning container. Skipping!", *GetNameSafe(ValidGet(Proxy).GetProxyObject()))
		return;
	}
	const TOptional<FFaerieVisualSlotElement> VisualSlotDataOpt = FindVisualSlotData(EntityManager, OwningContainer);
	FFaerieVisualSlotElement VisualSlotData = VisualSlotDataOpt.IsSet() ? VisualSlotDataOpt.GetValue() : GetDefaultVisualSlotElement();
	bool WantsToWaitForParentVisual = false;
	USceneComponent* AttachTo = GetComponentToAttachTo(EntityManager, OwningContainer, VisualSlotData.ComponentTag, WantsToWaitForParentVisual);
	if (WantsToWaitForParentVisual)
	{
		UE_LOGF(LogFaerieEquipment, Log, "CreateVisualImpl called for proxy %ls which has a parent to wait for. Skipping!", *GetNameSafe(ValidGet(Proxy).GetProxyObject()))
		return;
	}

	FFaerieVisualizationEntry& Entry = VisualData.FindOrAdd(Key);

	Entry.Attachment = BuildAttachmentData(AttachTo, VisualSlotData);

	// Enable hidden while pending load. This allows this attachment to still start async loading itself, even while not attached.
	Entry.Attachment.Hidden = true;

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
		Entry.Parent = *Parent.GetValue();
	}
	if (FFaerieVisualizationEntry* ParentEntry = VisualData.Find(Entry.Parent))
	{
		Entry.ParentReady = ParentEntry->SelfReady;
	}
	else
	{
		// If we have no parent, we don't need to wait for them to be ready.
		Entry.ParentReady = true;
	}

	// Step 2: What are we creating as a visual.

	// Path 1: A Visual Actor
	{
		const TConstStructView<FFaerieProxyActorFragment> ProxyClassFragment = Faerie::ItemData::GetItemDataOrDefault<FFaerieProxyActorFragment>(Proxy);
		if (ProxyClassFragment.IsValid())
		{
			// @todo implement async path here
			TSubclassOf<AFaerieProxyActorBase> ActorClass = ProxyClassFragment->ProxyActorClass.LoadSynchronous();

			if (!IsValid(ActorClass))
			{
				UE_LOGF(LogFaerieEquipment, Warning, "VisualClass failed to load!")
				return;
			}

			AFaerieProxyActorBase* NewVisual = SpawnVisualActorNative<AFaerieProxyActorBase>(Key, ActorClass);
			if (!IsValid(NewVisual))
			{
				UE_LOGF(LogFaerieEquipment, Error, "SpawnVisualActorNative failed for class '%ls'", *GetNameSafe(ActorClass))
				return;
			}

			Entry.Visual = NewVisual;

			NewVisual->GetOnDisplayFinished().AddWeakLambda(this,
				[this, Key, Visual = TWeakObjectPtr<AFaerieProxyActorBase>(NewVisual)](bool Success)
				{
					if (AFaerieProxyActorBase* Actor = Visual.Get())
					{
						PostVisualActionLoaded(Key, Actor);
					}
				});
			NewVisual->SetSourceProxy(Proxy);
		}
	}

	// Path 2: A Visual Component
	if (Entry.Visual == nullptr)
	{
		FGameplayTag CompPreferredTag = Mesh::Tags::MeshPurpose_Default;

		// If the slot overrides which tag is used to visualize it, use that tag.
		if (VisualSlotData.PreferredTag.IsValid())
		{
			CompPreferredTag = VisualSlotData.PreferredTag;
		}
		// Otherwise, use the tag that this visualizer defaults to.
		else if (PreferredTag.IsValid())
		{
			CompPreferredTag = PreferredTag;
		}

		// Some extensions might ban leader poses (like items held in hands)
		if (VisualSlotData.AllowLeaderPose)
		{
			// Reset attachment location to main mesh when using LeaderPose.
			Entry.Attachment.Parent = Cast<ACharacter>(GetOwner())->GetMesh();
			Entry.Attachment.ParentSocket = NAME_None;
			Entry.Attachment.ChildSocket = NAME_None;
		}

		UFaerieItemMeshComponent* NewVisual = SpawnVisualComponentNative<UFaerieItemMeshComponent>(Key,
			UFaerieItemMeshComponent::StaticClass());
		if (!IsValid(NewVisual))
		{
			UE_LOGF(LogFaerieEquipment, Error, "SpawnVisualComponentNative failed for class '%ls'", *GetNameSafe(UFaerieItemMeshComponent::StaticClass()))
			return;
		}

		Entry.Visual = NewVisual;

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

	ensureAlwaysMsgf(IsValid(Entry.Visual), TEXT("Failed to create visual object!"));

	// Step 3: Recurse over children
	if (ValidGet(Proxy).IsMutable())
	{
		const FMassEntityHandle Entity = ValidGet(Proxy).GetEntityHandle();

		for (TNotNull<UFaerieItemStackContainer*> SubContainer : Equipment::SlotFilter.Iterate(EntityManager, Entity))
		{
			const FFaerieEntryKey EntryKey = SubContainer->GetCurrentKey();
			if (EntryKey.IsValid())
			{
				FFaerieItemProxy ChildProxy(EntityManager.AsShared(), SubContainer);

				if (HasVisualForKey(ChildProxy))
				{
					static constexpr bool RemoveThisBoolAndActuallyTrackDownThisWarningLater = true;
					if (RemoveThisBoolAndActuallyTrackDownThisWarningLater)
					{
						RemoveVisualImpl(EntityManager, ChildProxy);
					}
					else
					{
						UE_LOGF(LogFaerieEquipment, Warning, "Container already has an visual spawned. Existing visuals must be removed before creating new ones!");
						return;
					}
				}

				CreateVisualImpl(EntityManager, ChildProxy, &ValidGet(Proxy));
			}
		}
	}
}

void UEquipmentVisualizer::RemoveVisualImpl(FMassEntityManager& EntityManager, const FFaerieVisualKey& Key)
{
	// Before we remove a visual, we should remove its children first.
	for (auto&& Metadatum : VisualData)
	{
		if (Metadatum.Value.Parent == Key)
		{
			RemoveVisualImpl(EntityManager, Metadatum.Key);
			Metadatum.Value.Parent = FFaerieVisualKey();
		}
	}

	DestroyVisualByKey(Key);
}

AActor* UEquipmentVisualizer::SpawnVisualActor(const FFaerieVisualKey Key, const TSubclassOf<AActor>& Class)
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
	if (HasVisualForKey(Key))
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
		return NewActor;
	}

	return nullptr;
}

USceneComponent* UEquipmentVisualizer::SpawnVisualComponent(const FFaerieVisualKey Key, const TSubclassOf<USceneComponent>& Class)
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
	if (HasVisualForKey(Key))
	{
		UE_LOGF(LogFaerieEquipment, Error, "Attempted to spawn a VisualComponent using a key already in use!")
		return nullptr;
	}

	if (USceneComponent* NewComponent = NewObject<USceneComponent>(GetOwner(), Class);
		IsValid(NewComponent))
	{
		GetOwner()->AddInstanceComponent(NewComponent);
		NewComponent->RegisterComponent();
		return NewComponent;
	}

	return nullptr;
}

void UEquipmentVisualizer::DestroyVisual(UObject* Visual, const bool ClearMetadata)
{
	TPair<FFaerieVisualKey, FFaerieVisualizationEntry>* VisualEntry = nullptr;
	for (auto&& Entry : VisualData)
	{
		if (Entry.Value.Visual == Visual)
		{
			VisualEntry = &Entry;
		}
	}
	if (!VisualEntry)
	{
		return;
	}
	const FFaerieVisualKey& Key = VisualEntry->Key;

	VisualEntry->Value.Visual = nullptr;

	if (AActor* VisualActor = Cast<AActor>(Visual))
	{
		VisualActor->Destroy();
	}
	else if (USceneComponent* VisualComponent = Cast<USceneComponent>(Visual))
	{
		VisualComponent->DestroyComponent();
	}

	if (ClearMetadata)
	{
		VisualData.Remove(Key);
	}
}

void UEquipmentVisualizer::DestroyVisualByKey(const FFaerieVisualKey Key, const bool ClearMetadata)
{
	if (FFaerieVisualizationEntry* Entry = VisualData.Find(Key))
	{
		if (AActor* VisualActor = Cast<AActor>(Entry->Visual))
		{
			VisualActor->OnDestroyed.RemoveAll(this);
			VisualActor->Destroy();
		}
		else if (USceneComponent* VisualComponent = Cast<USceneComponent>(Entry->Visual))
		{
			VisualComponent->DestroyComponent();
		}
		Entry->Visual = nullptr;
	}

	if (ClearMetadata)
	{
		VisualData.Remove(Key);
	}
}

FFaerieVisualAttachment UEquipmentVisualizer::FindAttachment(const FFaerieItemProxy& Proxy) const
{
	UFaerieItemContainerBase* OwningContainer = Container::GetItemOwner(Proxy);
	if (!IsValid(OwningContainer))
	{
		return FFaerieVisualAttachment();
	}

	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());
	TOptional<FFaerieVisualSlotElement> SlotDataOpt = FindVisualSlotData(EntityManager, OwningContainer);
	const FFaerieVisualSlotElement SlotData = SlotDataOpt.IsSet() ? SlotDataOpt.GetValue() : GetDefaultVisualSlotElement();
	bool WantsToWaitForParentVisual;
	USceneComponent* AttachTo = GetComponentToAttachTo(EntityManager, OwningContainer, SlotData.ComponentTag, WantsToWaitForParentVisual);
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

TOptional<FFaerieVisualSlotElement> UEquipmentVisualizer::FindVisualSlotData(const FMassEntityManager& EntityManager, const TNotNull<const UFaerieItemContainerBase*> Container)
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
	return NullOpt;
}

USceneComponent* UEquipmentVisualizer::GetComponentToAttachTo(const FMassEntityManager& EntityManager, const TNotNull<const UFaerieItemContainerBase*> Container, const FName& ComponentTag, bool& WantsToWaitForParentVisual) const
{
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
			ParentItem = ParentContainer->Proxy(EntityManager, ParentItemHandle);
		}
	}

	UObject* ParentVisual = nullptr;
	if (ParentItem.HasValidStack())
	{
		ParentVisual = GetSpawnedVisualByKey({ ParentItem });
	}

	// If we have a parent, see if a visual already exists
	if (IsValid(ParentVisual))
	{
		if (AActor* Actor = Cast<AActor>(ParentVisual))
		{
			ParentActor = Actor;
		}
		else if (USceneComponent* Component = Cast<USceneComponent>(ParentVisual))
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
	if (const FFaerieVisualizationEntry* Entry = VisualData.Find(Key))
	{
		ResetAttachment(*Entry);
	}
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
	if (FFaerieVisualizationEntry* Entry = VisualData.Find(Key))
	{
		Entry->Attachment = Attachment;
		ResetAttachment(*Entry);
	}
}

void UEquipmentVisualizer::AwaitOrReceiveUpdate(const FFaerieVisualKey Key, const FEquipmentVisualizerCallback Callback)
{
	FFaerieVisualizationEntry& Entry = VisualData.FindOrAdd(Key);
	// Set callback for any future changes to this key
	Entry.ChangeCallback.Add(Callback);

	// If a visual by this already exists, execute the callback right now.
	if (Entry.Visual && Entry.SelfReady)
	{
		Callback.Execute(Key, Entry.Visual);
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

	UE_LOGF(LogFaerieEquipment, Log, "Num Visuals: '%i'", VisualData.Num())
	for (auto&& Entry : VisualData)
	{
		FFaerieVisualKey Key = Entry.Key;
		TObjectPtr<UObject> Value = Entry.Value.Visual;
		UE_LOGF(LogFaerieEquipment, Log, "--- Visual: '%ls' spawned for '%ls'", *GetNameSafe(Value), *GetNameSafe(Key.GetProxy().GetProxyObject()))
		UE_LOGF(LogFaerieEquipment, Log, "    Attachment: '%ls'", *GetAttachmentString(Entry.Value.Attachment))
		UE_LOGF(LogFaerieEquipment, Log, "    Parent: '%ls', IsReady: '%ls', IsParentReady: '%ls'",
			*GetNameSafe(Entry.Value.Parent.GetProxy().GetProxyObject()),
			Entry.Value.SelfReady ? TEXT("true") : TEXT("false"),
			Entry.Value.ParentReady ? TEXT("true") : TEXT("false"))
	}

	UE_LOGF(LogFaerieEquipment, Warning, "End Equipment Visualizer debug log")
#endif
}

void UEquipmentVisualizer::RefreshAllAttachments()
{
	for (auto&& Entry : VisualData)
	{
		ResetAttachment(Entry.Value);
	}
}

void UEquipmentVisualizer::OnVisualActorDestroyed(AActor* DestroyedActor)
{
	DestroyVisual(DestroyedActor);
}