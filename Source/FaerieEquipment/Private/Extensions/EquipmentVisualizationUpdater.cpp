// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Extensions/EquipmentVisualizationUpdater.h"
#include "Extensions/RelevantActorsExtension.h"
#include "Extensions/VisualSlotExtension.h"
#include "EntityManagerHelpers.h"

#include "EquipmentVisualizer.h"
#include "FaerieEquipmentLog.h"
#include "FaerieEquipmentSlot.h"
#include "FaerieItem.h"
#include "FaerieItemContainerBase.h"
#include "FaerieSubObjectFilter.h"
#include "ItemContainerEvent.h"

#include "Actors/FaerieProxyActorBase.h"
#include "Components/FaerieItemMeshComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include "Fragments/FaerieActorFragment.h"

#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EquipmentVisualizationUpdater)

using namespace Faerie;

void UEquipmentVisualizationUpdater::InitializeExtension(const TNotNull<const UFaerieItemContainerBase*> Container)
{
	/*
	 * Normally, extensions run logic here for the container being initialized.
	 * However, we cannot in this case, as the visuals this extension generates can be dependent on the state of other
	 * containers, which may not have been Initialized with us yet. Instead, PostEventBatch handles logic for dependent
	 * containers, as CreateVisualImpl recurses over children.
	 */
}

void UEquipmentVisualizationUpdater::DeinitializeExtension(const TNotNull<const UFaerieItemContainerBase*> Container)
{
	if (const UFaerieEquipmentSlot* Slot = Cast<UFaerieEquipmentSlot>(Container))
	{
		auto&& Visualizer = GetVisualizer(Slot);
		if (!IsValid(Visualizer))
		{
			return;
		}

		RemoveVisualImpl(Visualizer, FFaerieItemProxy(Slot));
	}
}

void UEquipmentVisualizationUpdater::PreRemoval(const TNotNull<const UFaerieItemContainerBase*> Container,
	const TNotNull<const Container::IEntryView*> DataView, const int32 Removal)
{
	if (auto Slot = Cast<UFaerieEquipmentSlot>(Container))
	{
		// If the whole stack is being removed, remove the visual for it
		if (Removal == ItemData::EntireStack ||
			Slot->GetStackCopies() == Removal)
		{
			RemoveVisualForEntry(Slot, DataView->ResolveKey());
		}
	}
}

void UEquipmentVisualizationUpdater::PostEventBatch(const TNotNull<const UFaerieItemContainerBase*> Container, const Inventory::FEventLogBatch& Events)
{
	if (Events.IsAdditionEvent())
	{
		if (auto&& Slot = Cast<UFaerieEquipmentSlot>(Container))
		{
			// A previously empty slot now has been filled with an item.
			CreateVisualForEntry(Slot, Events.Data.Last().EntryTouched);
		}
	}
	else if (Events.IsRemovalEvent())
	{
	}
	else
	{
		check(Events.IsEditEvent())

		if (auto&& Slot = Cast<UFaerieEquipmentSlot>(Container))
		{
			checkNoEntry() // Right now, EquipmentSlots don't use EditEvents.

			// The item in a container has changed. Recreate the visual.
			// @todo maybe don't always do this?!?! determine if we need to. use the event tag type

			auto&& Visualizer = GetVisualizer(Slot);
			if (!IsValid(Visualizer))
			{
				return;
			}
			const FFaerieItemProxy Proxy(Slot);
			RemoveVisualImpl(Visualizer, Proxy);
			CreateVisualImpl(Visualizer, Proxy);
		}
	}
}

UEquipmentVisualizer* UEquipmentVisualizationUpdater::GetVisualizer(const UFaerieEquipmentSlot* Slot)
{
	if (!IsValid(Slot))
	{
		return nullptr;
	}

	auto&& Relevants = Extensions::Get<URelevantActorsExtension>(Slot->GetExtensions(), true);
	if (!IsValid(Relevants))
	{
		UE_LOGF(LogFaerieEquipment, Warning, "GetVisualizer failed: Requires a RelevantActorsExtension on the container to find the pawn (%ls)!", *Slot->GetName())
		return nullptr;
	}

	auto&& Pawn = Relevants->FindActor<APawn>();

	// @Todo: this is a little hacky, but fixes the case where this code is reached by the LevelViewport before the Relevants has been populated.
	if (!IsValid(Pawn))
	{
		Pawn = Relevants->GetTypedOuter<APawn>();
	}

	if (!IsValid(Pawn))
	{
		UE_LOGF(LogFaerieEquipment, Warning, "GetVisualizer failed: Failed to find relevant Pawn (%ls)!", *Slot->GetName())
		return nullptr;
	}

	auto&& Visualizer = Pawn->GetComponentByClass<UEquipmentVisualizer>();
	if (!IsValid(Visualizer))
	{
		UE_LOGF(LogFaerieEquipment, Warning, "GetVisualizer failed: Pawn does not have a visualizer component (%ls)!", *Slot->GetName())
		return nullptr;
	}

	return Visualizer;
}

void UEquipmentVisualizationUpdater::CreateVisualForEntry(const UFaerieEquipmentSlot* Slot, const FFaerieEntryKey Key)
{
	auto&& Visualizer = GetVisualizer(Slot);
	if (!IsValid(Visualizer))
	{
		return;
	}

	if (Visualizer->HasVisualForKey({ FFaerieItemProxy(Slot)}))
	{
		UE_LOGF(LogFaerieEquipment, Warning, "Container already has an visual spawned. Existing visuals must be removed before creating new ones!");
		return;
	}

	CreateVisualImpl(Visualizer,  FFaerieItemProxy(Slot));
}

void UEquipmentVisualizationUpdater::RemoveVisualForEntry(const UFaerieEquipmentSlot* Slot, const FFaerieEntryKey Key)
{
	auto&& Visualizer = GetVisualizer(Slot);
	if (!IsValid(Visualizer))
	{
		return;
	}

	RemoveVisualImpl(Visualizer, FFaerieItemProxy(Slot));
}

void UEquipmentVisualizationUpdater::CreateVisualImpl(UEquipmentVisualizer* Visualizer, TValid<const FFaerieItemProxy&> Proxy)
{
	// Step 1: Figure out what we are attaching to.

	const UVisualSlotExtension* SlotExtension = nullptr;
	bool CanLeaderPoseMesh = false;
	FEquipmentVisualAttachment Attachment = Visualizer->FindAttachment(Proxy, SlotExtension);

	// FindAttachmentParent will return a UFaerieItemMeshComponent when it wants us to defer for a pending attaching.
	if (Attachment.Parent->IsA<UFaerieItemMeshComponent>())
	{
		// Enable hidden while in Pending. This allows this attachment to still start async loading itself, even while not attached.
		Attachment.Hidden = true;
		Pending.Emplace(Proxy, Attachment);
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

			AFaerieProxyActorBase* NewVisual = Visualizer->SpawnVisualActorNative<AFaerieProxyActorBase>(
				{ Proxy }, VisualClass, Attachment);
			if (IsValid(NewVisual))
			{
				NewVisual->GetOnDisplayFinished().AddWeakLambda(this,
					[this, Visualizer, Visual = TWeakObjectPtr<AFaerieProxyActorBase>(NewVisual)](bool Success)
					{
						if (!Visual.IsValid()) return;

						for (auto&& It = Pending.CreateIterator(); It; ++It)
						{
							if (It->Attachment.Parent->GetOwner() == Visual)
							{
								It->Attachment.Parent = Visual->GetDefaultAttachComponent();
								It->Attachment.Hidden = false;
								Visualizer->UpdateAttachment({It->Proxy}, It->Attachment);
								It.RemoveCurrentSwap();
								return;
							}
						}

						// If this wasn't pending, just update its attachment after a rebuild.
						Visualizer->ResetAttachment({Visual->GetSourceProxy() });
					});
				NewVisual->SetSourceProxy(Proxy);
				return;
			}
		}
	}

	// Path 2: A Visual Component
	{
		FGameplayTag PreferredTag = Mesh::Tags::MeshPurpose_Default;
		if (Visualizer->GetPreferredTag().IsValid() &&
			ensure(Visualizer->GetPreferredTag().GetTagName().IsValid()))
		{
			PreferredTag = Visualizer->GetPreferredTag();
		}

		// Some extensions might ban leader poses (like items held in hands)
		if (IsValid(SlotExtension))
		{
			if (SlotExtension->GetAllowLeaderPose())
			{
				CanLeaderPoseMesh = true;

				// Reset attachment location to main mesh when using LeaderPose.
				Attachment.Parent = Cast<ACharacter>(Visualizer->GetOwner())->GetMesh();
				Attachment.ParentSocket = NAME_None;
				Attachment.ChildSocket = NAME_None;
			}

			if (SlotExtension->GetPreferredTag().IsValid() &&
				ensure(SlotExtension->GetPreferredTag().GetTagName().IsValid()))
			{
				PreferredTag = SlotExtension->GetPreferredTag();
			}
		}

		UFaerieItemMeshComponent* NewVisual = Visualizer->SpawnVisualComponentNative<UFaerieItemMeshComponent>(
			{ Proxy }, UFaerieItemMeshComponent::StaticClass(), Attachment);
		if (IsValid(NewVisual))
		{
			// If there is no AnimClass on the mesh, it would prefer using LeaderPose as a fallback
			if (CanLeaderPoseMesh)
			{
				NewVisual->SetSkeletalMeshLeaderPoseComponent(Visualizer->GetLeaderComponent());
			}

			NewVisual->SetPreferredTag(PreferredTag);
			NewVisual->SetIsReplicated(true); // Enable replication, as it's off by default.
			NewVisual->GetOnMeshRebuilt().AddWeakLambda(this,
				[this, Proxy, Visualizer](const TNotNull<UFaerieItemMeshComponent*> ItemMeshComponent)
				{
					for (auto&& It = Pending.CreateIterator(); It; ++It)
					{
						if (It->Attachment.Parent == ItemMeshComponent)
						{
							It->Attachment.Parent = ItemMeshComponent->GetGeneratedMeshComponent();
							It->Attachment.Hidden = false;
							Visualizer->UpdateAttachment({It->Proxy}, It->Attachment);
							It.RemoveCurrentSwap();
							return;
						}
					}

					// If this wasn't pending, just update its attachment after a rebuild.
					Visualizer->ResetAttachment({ Proxy });
				});
			NewVisual->SetItemMeshFromProxy(Proxy);
		}
	}

	// Step 3: Recurse over children
	if (Instance.IsMutable())
	{
		for (auto SubContainer : Equipment::SlotFilter.Iterate(EntityManager, Instance))
		{
			auto Key = SubContainer->GetCurrentKey();
			if (Key.IsValid())
			{
				CreateVisualForEntry(SubContainer, Key);
			}
		}
	}
}

void UEquipmentVisualizationUpdater::RemoveVisualImpl(UEquipmentVisualizer* Visualizer, const TValid<const FFaerieItemProxy&> Proxy)
{
	check(Visualizer);
	Visualizer->DestroyVisualByKey({Proxy});

	// Recurse over children
	const TOptional<FFaerieItemInstance> Instance = ValidGet(Proxy).GetItemInstance();
	if (!Instance.IsSet())
	{
		return;
	}

	if (Instance->IsMutable())
	{
		auto& EntityManager = ItemData::GetFaerieEntityManagerChecked();
        for (UFaerieEquipmentSlot* SubContainer : Equipment::SlotFilter.Iterate(EntityManager, Instance.GetValue()))
        {
        	auto Key = SubContainer->GetCurrentKey();
        	if (Key.IsValid())
        	{
        		RemoveVisualForEntry(SubContainer, Key);
        	}
        }
	}
}