// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Extensions/EquipmentVisualizationUpdater.h"
#include "Extensions/RelevantActorsExtension.h"
#include "Extensions/VisualSlotExtension.h"

#include "EquipmentVisualizer.h"
#include "FaerieEquipmentSlot.h"
#include "FaerieItemContainerBase.h"
#include "ItemContainerEvent.h"

#include "Actors/ItemRepresentationActor.h"
#include "Components/FaerieItemMeshComponent.h"
#include "Tokens/FaerieMeshToken.h"
#include "Tokens/FaerieVisualActorClassToken.h"
#include "Tokens/FaerieVisualEquipment.h"

#include "GameFramework/Character.h"
#include "Tokens/FaerieItemStorageToken.h"

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
	if (!Container->IsA<UFaerieEquipmentSlot>())
	{
		return;
	}

	TArray<FEntryKey> Keys;
	SpawnKeys.MultiFind(Container, Keys);
	SpawnKeys.Remove(Container);

	if (!Keys.IsEmpty())
	{
		auto&& Visualizer = GetVisualizer(Container);
		if (!IsValid(Visualizer))
		{
			return;
		}

		for (auto&& Key : Keys)
		{
			RemoveVisualImpl(Visualizer, Container->Proxy(Key));
		}
	}
}

void UEquipmentVisualizationUpdater::PostAddition(const UFaerieItemContainerBase* Container,
												  const Faerie::Inventory::FEventLog& Event)
{
	if (auto Slot = Cast<UFaerieEquipmentSlot>(Container))
	{
		// A previously empty slot now has been filled with an item.
		CreateVisualForEntry(Container, Event.EntryTouched);
	}
}

void UEquipmentVisualizationUpdater::PreRemoval(const UFaerieItemContainerBase* Container, const FEntryKey Key,
	const int32 Removal)
{
	if (auto Slot = Cast<UFaerieEquipmentSlot>(Container))
	{
		// If the whole stack is being removed, remove the visual for it
		if (Container->GetStack(Key) == Removal || Removal == Faerie::ItemData::UnlimitedStack)
		{
			RemoveVisualForEntry(Container, Key);
		}
	}
}

void UEquipmentVisualizationUpdater::PostEntryChanged(const UFaerieItemContainerBase* Container,
	const Faerie::Inventory::FEventLog& Event)
{
	if (auto Slot = Cast<UFaerieEquipmentSlot>(Container))
	{
		// The item in a container has changed. Recreate the visual.
		// @todo maybe don't always do this?!?! determine if we need to. use the event tag type

		auto&& Visualizer = GetVisualizer(Container);
		if (!IsValid(Visualizer))
		{
			return;
		}
		const FFaerieItemProxy Proxy = Container->Proxy(Event.EntryTouched);
		RemoveVisualImpl(Visualizer, Proxy);
		CreateVisualImpl(Visualizer, Proxy);
	}
}

UEquipmentVisualizer* UEquipmentVisualizationUpdater::GetVisualizer(const UFaerieItemContainerBase* Container)
{
	if (!IsValid(Container))
	{
		return nullptr;
	}

	auto&& Relevants = GetExtension<URelevantActorsExtension>(Container, true);
	if (!IsValid(Relevants))
	{
		UE_LOG(LogTemp, Warning, TEXT("GetVisualizer failed: Requires a RelevantActorsExtension on the container to find the pawn (%s)!"), *Container->GetName())
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
		UE_LOG(LogTemp, Warning, TEXT("GetVisualizer failed: Failed to find relevant Pawn (%s)!"), *Container->GetName())
		return nullptr;
	}

	auto&& Visualizer = Pawn->GetComponentByClass<UEquipmentVisualizer>();
	if (!IsValid(Visualizer))
	{
		UE_LOG(LogTemp, Warning, TEXT("GetVisualizer failed: Pawn does not have a visualizer component (%s)!"), *Container->GetName())
		return nullptr;
	}

	return Visualizer;
}

void UEquipmentVisualizationUpdater::CreateVisualForEntry(const UFaerieItemContainerBase* Container, const FEntryKey Key)
{
	auto&& Visualizer = GetVisualizer(Container);
	if (!IsValid(Visualizer))
	{
		return;
	}

	if (SpawnKeys.Contains(Container))
	{
		UE_LOG(LogTemp, Warning, TEXT("Container already has an visual spawned. Existing visuals must be removed before creating new ones!"));
		return;
	}

	CreateVisualImpl(Visualizer, Container->Proxy(Key));
	SpawnKeys.Add(Container, Key);
}

void UEquipmentVisualizationUpdater::RemoveVisualForEntry(const UFaerieItemContainerBase* Container, const FEntryKey Key)
{
	auto&& Visualizer = GetVisualizer(Container);
	if (!IsValid(Visualizer))
	{
		return;
	}

	RemoveVisualImpl(Visualizer, Container->Proxy(Key));
	SpawnKeys.RemoveSingle(Container, Key);
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
		TSoftClassPtr<AItemRepresentationActor> ActorClass = nullptr;

		if (auto&& VisualToken_Deprecated = ItemObject->GetToken<UFaerieVisualEquipment>();
			IsValid(VisualToken_Deprecated))
		{
			ActorClass = VisualToken_Deprecated->GetActorClass();
		}

		if (auto&& VisualToken = ItemObject->GetToken<UFaerieVisualActorClassToken>();
			IsValid(VisualToken))
		{
			ActorClass = VisualToken->GetActorClass();
		}

		if (!ActorClass.IsNull())
		{
			// @todo implement async path here
			const TSubclassOf<AItemRepresentationActor> VisualClass = ActorClass.LoadSynchronous();
			if (!IsValid(VisualClass))
			{
				UE_LOG(LogTemp, Warning, TEXT("VisualClass failed to load!"))
				return;
			}

			AItemRepresentationActor* NewVisual = Visualizer->SpawnVisualActorNative<AItemRepresentationActor>(
				{ Proxy }, VisualClass, Attachment);
			if (IsValid(NewVisual))
			{
				NewVisual->GetOnDisplayFinished().AddWeakLambda(this,
					[this, Visualizer](bool Success, AItemRepresentationActor* Actor)
					{
						for (auto&& It = Pending.CreateIterator(); It; ++It)
						{
							if (It->Attachment.Parent->GetOwner() == Actor)
							{
								It->Attachment.Parent = Actor->GetDefaultAttachComponent();
								It->Attachment.Hidden = false;
								Visualizer->UpdateAttachment({It->Proxy}, It->Attachment);
								It.RemoveCurrentSwap();
								return;
							}
						}

						// If this wasn't pending, just update its attachment after a rebuild.
						Visualizer->ResetAttachment({Actor->GetSourceProxy() });
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

					// If this wasn't pending, just update its attachment after a rebuilt.
					Visualizer->ResetAttachment({ Proxy });
				});
			NewVisual->SetItemMeshFromToken(ItemObject->GetToken<UFaerieMeshTokenBase>());
		}
	}

	// Step 3: Recurse over children
	if (UFaerieItem* Mutable = ItemObject->MutateCast())
	{
		auto SubContainers = UFaerieItemContainerToken::GetContainersInItem<UFaerieEquipmentSlot>(Mutable);
		for (auto SubContainer : SubContainers)
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
	if (UFaerieItem* Mutable = Proxy->GetItemObject()->MutateCast())
	{
		auto SubContainers = UFaerieItemContainerToken::GetContainersInItem<UFaerieEquipmentSlot>(Mutable);
        for (auto SubContainer : SubContainers)
        {
        	auto Key = SubContainer->GetCurrentKey();
        	if (Key.IsValid())
        	{
        		RemoveVisualForEntry(SubContainer, Key);
        	}
        }
	}
}