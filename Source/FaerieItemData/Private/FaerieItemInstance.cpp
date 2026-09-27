// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemInstance.h"
#include "FaerieItem.h"
#include "FaerieItemDataLog.h"
#include "FaerieItemEvent.h"
#include "FaerieMassFragment.h"
#include "MassEntityBuilder.h"
#include "MassEntityTemplate.h"

#include "Subsystems/ConfigLoaderSubsystem.h"

#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemInstance)

using namespace Faerie;

void FFaerieItemInstance::InitializeMassEntityImpl(FMassEntityManager& EntityManager, const TArrayView<FInstancedStruct> Fragments)
{
	const FMassEntityTemplate& EntityTemplate = EntityManager.GetWorld()->GetSubsystemChecked<UFaerieMassConfigLoaderSubsystem>()->GetItemDataTemplate();
	UE::Mass::FEntityBuilder Builder = EntityTemplate.CreateEntityBuilder(EntityManager.AsShared())
		.Add<FFaerieMassItemPointer>(Item); // Add a MassItemPointer struct to label the entity as a faerie item whether there is a valid item pointer or not.

	// Look for fragments that need to always be moved into the runtime entity on creation
	for (const FInstancedStruct& DefaultFragment : Item->GetFragmentDefaults())
	{
		const ItemData::FMassFragmentTypeInterface* Traits = ItemData::GetFragmentTraitsInterface(DefaultFragment.GetScriptStruct());
		if (Traits && Traits->HasInitializeRuntime)
		{
			FInstancedStruct FragmentCopy = DefaultFragment;
			if (Traits->InitializeRuntime(FragmentCopy.GetMutableMemory(), EntityManager, *this))
			{
				Builder.Add(MoveTemp(FragmentCopy));
			}
		}
	}

	// Add any initial fragments to the builder
	for (auto&& FragmentInstance : Fragments)
	{
		Builder.Add(MoveTemp(FragmentInstance));
	}

	// Assign mass entity handle ahead of calling Commit, because observers can be directly triggered by Commit,
	// and they may want this item to already have an entity handle.
	EntityHandle = Builder.GetEntityHandle();
	Builder.Commit();
}

bool FFaerieItemInstance::IsMutable() const
{
	if (Item)
	{
		// Always report if the ItemAsset is mutable.
		return Item->CanMutate();
	}

	// Otherwise MassEntity-based instances with no item asset are always mutable.
	return HasMassEntity();
}

bool FFaerieItemInstance::CanStack() const
{
	// Mutable instances cannot stack, due to, well, being mutable, meaning that each instance retains the ability to
	// uniquely differ from others.
	return !IsMutable();
}

bool FFaerieItemInstance::UEOpEquals(const FFaerieItemInstance& Other) const
{
	return Item == Other.Item && EntityHandle == Other.EntityHandle;
}

void FFaerieItemInstance::InitializeMassEntity(FMassEntityManager& EntityManager, const TArrayView<FInstancedStruct> Fragments)
{
	// Only mutable instances are allowed to create a mass entity.
	check(IsMutable())

	// Prevent double-registration!
	check(!EntityManager.IsEntityValid(EntityHandle));

	InitializeMassEntityImpl(EntityManager, Fragments);
}

void FFaerieItemInstance::InitializeMassEntityIfInvalid(FMassEntityManager& EntityManager)
{
	if (IsMutable() && !EntityManager.IsEntityValid(EntityHandle))
	{
		InitializeMassEntityImpl(EntityManager, {});
	}
}

void FFaerieItemInstance::DestroyMassEntity(FMassEntityManager& EntityManager)
{
	if (EntityManager.IsEntityValid(EntityHandle))
	{
		// @Todo
		//EntityManager->GetWorld()->GetSubsystemChecked<UFaerieViewModelSubsystem>()->HandleInstanceDestruction(*this);
		EntityManager.DestroyEntity(EntityHandle);
	}
	EntityHandle.Reset();
}

void FFaerieItemInstance::ImportFragmentData(FMassEntityManager& EntityManager, const TArrayView<FInstancedStruct> Fragments)
{
	if (Fragments.IsEmpty()) return;

	if (!IsMutable())
	{
		UE_LOGF(LogFaerieItemData, Error, "Attempted importing mass instanced to immutable item instance!")
		return;
	}

	// Prevent double-registration!
	check(!EntityManager.IsEntityValid(EntityHandle));

	InitializeMassEntityImpl(EntityManager, Fragments);
}

void FFaerieItemInstance::ExportFragmentData(const FMassEntityManager& EntityManager,
	TArray<FInstancedStruct>& OutStructs, const ItemData::EMassFragmentExportOptions Options) const
{
	if (EntityManager.IsEntityValid(EntityHandle))
	{
		const FMassArchetypeHandle Archetype = EntityManager.GetArchetypeForEntity(EntityHandle);
		EntityManager.ForEachArchetypeFragmentType(Archetype,
			[this, &EntityManager, &OutStructs, Options](const UScriptStruct* FragmentType)
			{
				if (EnumHasAnyFlags(Options, ItemData::OnlyFaerieMassFragments))
				{
					if (!FragmentType->IsChildOf<FFaerieMassFragment>())
					{
						return;
					}
				}

				const FStructView StructView = EntityManager.GetFragmentDataStruct(EntityHandle, FragmentType);
				OutStructs.AddDefaulted_GetRef().InitializeAs(StructView.GetScriptStruct(), StructView.GetMemory());
			});
	}
}

bool FFaerieItemInstance::IsMutable(const FMassEntityManager& EntityManager, const FMassEntityHandle Item)
{
	if (!EntityManager.IsEntityValid(Item)) return false;

	const FFaerieMassItemPointer* ItemPointer = EntityManager.GetFragmentDataPtr<FFaerieMassItemPointer>(Item);
	if (const UFaerieItem* ItemAsset = ItemPointer->Item.ResolveObjectPtr())
	{
		// Always report if the ItemAsset is mutable.
		return ItemAsset->CanMutate();
	}

	return true;
}

void FFaerieItemInstance::PostMutationEvent(FMassEntityManager& EntityManager, const ItemData::FMutationEvent& Event)
{
	FInstancedStruct EventStruct = FInstancedStruct::Make(Event);
	EntityManager.Defer().PushCommand<FMassDeferredCreateCommand>(
		[EventStruct = MoveTemp(EventStruct)](FMassEntityManager& DeferredEntityManager)
		{
			DeferredEntityManager.CreateEntity(MakeConstArrayView(&EventStruct, 1));
		});
}

void FFaerieItemInstance::PostMutationEventWithChangeList(FMassEntityManager& EntityManager,
	const ItemData::FMutationEvent& Event, const ItemData::FMutationPayloadChangeList& ChangeList)
{
	TStaticArray<FInstancedStruct, 3> EventStructs;
	EventStructs[0] = FInstancedStruct::Make(Event);
	EventStructs[1] = FInstancedStruct::Make(ChangeList);
	EventStructs[2] = FInstancedStruct::Make(ItemData::AllFields); // Include a FieldChange payload to notify any view models that care that all fields were overwritten.
	EntityManager.Defer().PushCommand<FMassDeferredCreateCommand>(
		[EventStructs = MoveTemp(EventStructs)](FMassEntityManager& DeferredEntityManager)
		{
			DeferredEntityManager.CreateEntity(EventStructs);
		});
}

void FFaerieItemInstance::PostMutationEventWithFieldChange(FMassEntityManager& EntityManager,
	const ItemData::FMutationEvent& Event, const ItemData::FFieldChangePayload FieldChanges)
{
	TStaticArray<FInstancedStruct, 2> EventStructs;
	EventStructs[0] = FInstancedStruct::Make(Event);
	EventStructs[1] = FInstancedStruct::Make(FieldChanges);
	EntityManager.Defer().PushCommand<FMassDeferredCreateCommand>(
		[EventStructs = MoveTemp(EventStructs)](FMassEntityManager& DeferredEntityManager)
		{
			DeferredEntityManager.CreateEntity(EventStructs);
		});
}

void FFaerieItemInstance::AddFragment(FMassEntityManager& EntityManager, const FMassEntityHandle Item, FInstancedStruct&& Fragment)
{
	check(EntityManager.IsEntityValid(Item));

	// Adding a fragment is only allowed for mutable instances.
	check(IsMutable(EntityManager, Item))

	EntityManager.AddFragmentInstanceListToEntity(Item, MakeArrayView(&Fragment, 1));

	ItemData::FMutationEvent Payload;
	Payload.ItemHandle = Item;
	Payload.EventType = ItemData::Tags::FragmentAdd;
	Payload.ChangeType = Fragment.GetScriptStruct();
	PostMutationEvent(EntityManager, Payload);
}

void FFaerieItemInstance::AddFragments(FMassEntityManager& EntityManager, const FMassEntityHandle Item, const TArrayView<FInstancedStruct> Fragments)
{
	check(EntityManager.IsEntityValid(Item));

	// Adding mass fragments are only allowed for mutable instances.
	check(IsMutable(EntityManager, Item))

	EntityManager.AddFragmentInstanceListToEntity(Item, Fragments);

	ItemData::FMutationEvent Payload;
	Payload.ItemHandle = Item;
	Payload.EventType = ItemData::Tags::FragmentAdd;
	Payload.ChangeType = ItemData::FMutationPayloadChangeList::StaticStruct();

	ItemData::FMutationPayloadChangeList ChangeList;
	for (const FInstancedStruct& Fragment : Fragments)
	{
		ChangeList.ChangeTypes.Add(Fragment.GetScriptStruct());
	}

	PostMutationEventWithChangeList(EntityManager, Payload, ChangeList);
}

void FFaerieItemInstance::RemoveFragment(FMassEntityManager& EntityManager, const FMassEntityHandle Item,
										  const TNotNull<const UScriptStruct*> FragmentType)
{
	if (EntityManager.IsEntityValid(Item))
	{
		EntityManager.RemoveFragmentFromEntity(Item, FragmentType);

		ItemData::FMutationEvent Payload;
		Payload.ItemHandle = Item;
		Payload.EventType = ItemData::Tags::FragmentRemove;
		Payload.ChangeType = FragmentType;
		PostMutationEvent(EntityManager, Payload);
	}
}

void FFaerieItemInstance::RemoveFragments(FMassEntityManager& EntityManager, const FMassEntityHandle Item, const TConstArrayView<const UScriptStruct*> FragmentTypes)
{
	if (EntityManager.IsEntityValid(Item))
	{
		EntityManager.RemoveFragmentListFromEntity(Item, FragmentTypes);

		ItemData::FMutationEvent Payload;
		Payload.ItemHandle = Item;
		Payload.EventType = ItemData::Tags::FragmentRemove;
		Payload.ChangeType = ItemData::FMutationPayloadChangeList::StaticStruct();

		ItemData::FMutationPayloadChangeList ChangeList;
		for (const UScriptStruct* FragmentType : FragmentTypes)
		{
			ChangeList.ChangeTypes.Add(FragmentType);
		}

		PostMutationEventWithChangeList(EntityManager, Payload, ChangeList);
	}
}

void FFaerieItemInstance::UpdateFragments(FMassEntityManager& EntityManager, const FMassEntityHandle Item, TArrayView<FInstancedStruct> Fragments,
	const bool ClearOthers)
{
	if (EntityManager.IsEntityValid(Item))
	{
		if (ClearOthers)
		{
			TArray<const UScriptStruct*> FragmentsToRemove;
			const FMassArchetypeHandle Archetype = EntityManager.GetArchetypeForEntity(Item);
			EntityManager.ForEachArchetypeFragmentType(Archetype,
				[&Fragments, &FragmentsToRemove](const UScriptStruct* FragmentType)
				{
					if (!FragmentType->IsChildOf<FFaerieMassFragment>())
					{
						return;
					}

					for (auto&& NewFragment : Fragments)
					{
						if (NewFragment.GetScriptStruct() == FragmentType)
						{
							// New fragment for this type, leave it.
							return;
						}
					}

					// No new fragment for this type, remove it.
					FragmentsToRemove.Add(FragmentType);
				});

			// Commit and emit event for removed fragments
			RemoveFragments(EntityManager, Item, FragmentsToRemove);
		}

		// Commit and emit event for added fragments
		AddFragments(EntityManager, Item, Fragments);
	}
}

void FFaerieItemInstance::OnItemFragmentEdited(FMassEntityManager& EntityManager, const FMassEntityHandle Item, const TNotNull<const UScriptStruct*> FragmentType, const ItemData::FFieldChangePayload& FieldChange)
{
	ItemData::FMutationEvent Payload;
	Payload.ItemHandle = Item;
	Payload.ChangeType = FragmentType;
	Payload.EventType = ItemData::Tags::FragmentGenericPropertyEdit;
	PostMutationEventWithFieldChange(EntityManager, Payload, FieldChange);
}
