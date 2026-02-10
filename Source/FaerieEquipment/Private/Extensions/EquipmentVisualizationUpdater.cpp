// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Extensions/EquipmentVisualizationUpdater.h"
#include "Extensions/RelevantActorsExtension.h"
#include "Extensions/VisualSlotExtension.h"

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
#include "Tokens/FaerieMeshToken.h"
#include "Tokens/FaerieVisualActorClassToken.h"
#include "Tokens/FaerieVisualEquipment.h"

#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EquipmentVisualizationUpdater)

void UEquipmentVisualizationUpdater::InitializeExtension(const UFaerieItemContainerBase* Container)
{
	/*
	 * Normally, extensions run logic here for the container being initialized.
	 * However, we cannot in this case, as the visuals this extension generates can be dependent on the state of other
	 * containers, which may not have been Initialized with us yet. Instead, PostAddition handles logic for dependent
	 * containers, as CreateVisualImpl recurses over children.
	 */
}

void UEquipmentVisualizationUpdater::DeinitializeExtension(const UFaerieItemContainerBase* Container)
{
	if (const UFaerieEquipmentSlot* Slot = Cast<UFaerieEquipmentSlot>(Container))
	{
		auto&& Visualizer = GetVisualizer(Slot);
		if (!IsValid(Visualizer))
		{
			return;
		}

		RemoveVisualImpl(Visualizer, Slot);
	}
}

void UEquipmentVisualizationUpdater::PostAddition(const UFaerieItemContainerBase* Container,
												  const Faerie::Inventory::FEventLog& Event)
{
	if (auto Slot = Cast<UFaerieEquipmentSlot>(Container))
	{
		// A previously empty slot now has been filled with an item.
		CreateVisualForEntry(Slot, Event.EntryTouched);
	}
}

void UEquipmentVisualizationUpdater::PreRemoval(const UFaerieItemContainerBase* Container, const FEntryKey Key,
	const int32 Removal)
{
	if (auto Slot = Cast<UFaerieEquipmentSlot>(Container))
	{
		// If the whole stack is being removed, remove the visual for it
		if (Removal == Faerie::ItemData::EntireStack ||
			Slot->GetStack() == Removal)
		{
			RemoveVisualForEntry(Slot, Key);
		}
	}
}

void UEquipmentVisualizationUpdater::PostEntryChanged(const UFaerieItemContainerBase* Container,
	const Faerie::Inventory::FEventLog& Event)
{
	if (auto Slot = Cast<UFaerieEquipmentSlot>(Container))
	{
		checkNoEntry() // Right now, EquipmentSlots don't use PostEntryChanged.

		// The item in a container has changed. Recreate the visual.
		// @todo maybe don't always do this?!?! determine if we need to. use the event tag type

		auto&& Visualizer = GetVisualizer(Slot);
		if (!IsValid(Visualizer))
		{
			return;
		}
		const FFaerieItemProxy Proxy = Slot;
		RemoveVisualImpl(Visualizer, Proxy);
		CreateVisualImpl(Visualizer, Proxy);
	}
}

UEquipmentVisualizer* UEquipmentVisualizationUpdater::GetVisualizer(const UFaerieEquipmentSlot* Slot)
{
	if (!IsValid(Slot))
	{
		return nullptr;
	}

	auto&& Relevants = GetExtension<URelevantActorsExtension>(Slot, true);
	if (!IsValid(Relevants))
	{
		UE_LOG(LogFaerieEquipment, Warning, TEXT("GetVisualizer failed: Requires a RelevantActorsExtension on the container to find the pawn (%s)!"), *Slot->GetName())
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
		UE_LOG(LogFaerieEquipment, Warning, TEXT("GetVisualizer failed: Failed to find relevant Pawn (%s)!"), *Slot->GetName())
		return nullptr;
	}

	auto&& Visualizer = Pawn->GetComponentByClass<UEquipmentVisualizer>();
	if (!IsValid(Visualizer))
	{
		UE_LOG(LogFaerieEquipment, Warning, TEXT("GetVisualizer failed: Pawn does not have a visualizer component (%s)!"), *Slot->GetName())
		return nullptr;
	}

	return Visualizer;
}

void UEquipmentVisualizationUpdater::CreateVisualForEntry(const UFaerieEquipmentSlot* Slot, const FEntryKey Key)
{
	auto&& Visualizer = GetVisualizer(Slot);
	if (!IsValid(Visualizer))
	{
		return;
	}

	if (Visualizer->HasVisualForKey({Slot}))
	{
		UE_LOG(LogFaerieEquipment, Warning, TEXT("Container already has an visual spawned. Existing visuals must be removed before creating new ones!"));
		return;
	}

	CreateVisualImpl(Visualizer, Slot);
}

void UEquipmentVisualizationUpdater::RemoveVisualForEntry(const UFaerieEquipmentSlot* Slot, const FEntryKey Key)
{
	auto&& Visualizer = GetVisualizer(Slot);
	if (!IsValid(Visualizer))
	{
		return;
	}

	RemoveVisualImpl(Visualizer, Slot);
}

void UEquipmentVisualizationUpdater::CreateVisualImpl(UEquipmentVisualizer* Visualizer, const FFaerieItemProxy Proxy)
{
	if (!Proxy.IsValid())
	{
		return;
	}

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
	const UFaerieItem* ItemObject = Proxy->GetItemObject();

	// Path 1: A Visual Actor
	{
		TSoftClassPtr<AFaerieProxyActorBase> ActorClass = nullptr;

		if (auto&& VisualToken_Deprecated = ItemObject->GetToken<UFaerieVisualEquipment>();
			IsValid(VisualToken_Deprecated))
		{
			ActorClass = VisualToken_Deprecated->GetActorClass();
		}

		if (auto&& VisualToken = ItemObject->GetToken<UFaerieVisualActorClassToken>();
			IsValid(VisualToken))
		{
			ActorClass = VisualToken->GetProxyActorClass();
		}

		if (!ActorClass.IsNull())
		{
			// @todo implement async path here
			const TSubclassOf<AFaerieProxyActorBase> VisualClass = ActorClass.LoadSynchronous();
			if (!IsValid(VisualClass))
			{
				UE_LOG(LogFaerieEquipment, Warning, TEXT("VisualClass failed to load!"))
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
		FGameplayTag PreferredTag = Faerie::ItemMesh::Tags::MeshPurpose_Default;
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
				[this, Proxy, Visualizer](UFaerieItemMeshComponent* ItemMeshComponent)
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
			NewVisual->SetItemMeshFromToken(ItemObject->GetToken<UFaerieMeshTokenBase>());
		}
	}

	// Step 3: Recurse over children
	if (UFaerieItem* Mutable = ItemObject->MutateCast())
	{
		for (auto SubContainer : Faerie::Equipment::SlotFilter.Iterate(Mutable))
		{
			auto Key = SubContainer->GetCurrentKey();
			if (Key.IsValid())
			{
				CreateVisualForEntry(SubContainer, Key);
			}
		}
	}
}

void UEquipmentVisualizationUpdater::RemoveVisualImpl(UEquipmentVisualizer* Visualizer, const FFaerieItemProxy Proxy)
{
	check(Visualizer);
	Visualizer->DestroyVisualByKey({Proxy});

	// Recurse over children
	auto Item = Proxy->GetItemObject();
	if (!IsValid(Item))
	{
		return;
	}

	if (UFaerieItem* Mutable = Item->MutateCast())
	{
        for (UFaerieEquipmentSlot* SubContainer : Faerie::Equipment::SlotFilter.Iterate(Mutable))
        {
        	auto Key = SubContainer->GetCurrentKey();
        	if (Key.IsValid())
        	{
        		RemoveVisualForEntry(SubContainer, Key);
        	}
        }
	}
}