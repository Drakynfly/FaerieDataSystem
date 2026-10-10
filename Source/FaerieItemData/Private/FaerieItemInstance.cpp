// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemInstance.h"
#include "FaerieItem.h"
#include "FaerieItemDataLog.h"
#include "FaerieItemEvent.h"
#include "FaerieItemExportData.h"
#include "FaerieMassFragment.h"
#include "MassEntityBuilder.h"
#include "MassEntityTemplate.h"

#include "Subsystems/ConfigLoaderSubsystem.h"

#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemInstance)

using namespace Faerie;

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
	if (EntityHandle == Other.EntityHandle)
	{
		return true;
	}

	return Item == Other.Item;
}

void FFaerieItemInstance::InitializeMassEntity(FMassEntityManager& EntityManager, const TArrayView<FInstancedStruct> Fragments)
{
	// Only mutable instances are allowed to create a mass entity.
	check(IsMutable())

	// Prevent double-registration!
	check(!EntityManager.IsEntityValid(EntityHandle));

	EntityHandle = ItemData::InitializeItemEntity(EntityManager, Item, FMassTagBitSet(), Fragments);
}

void FFaerieItemInstance::InitializeMassEntityIfInvalid(FMassEntityManager& EntityManager)
{
	if (IsMutable() && !EntityManager.IsEntityValid(EntityHandle))
	{
		EntityHandle = ItemData::InitializeItemEntity(EntityManager, Item, FMassTagBitSet(), {});
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

void FFaerieItemInstance::ImportFragmentData(FMassEntityManager& EntityManager, FFaerieItemExportData& ExportData)
{
	if (!IsMutable())
	{
		UE_LOGF(LogFaerieItemData, Error, "Attempted importing mass instanced to immutable item instance!")
		return;
	}

	// Prevent double-registration!
	check(!EntityManager.IsEntityValid(EntityHandle));

	FMassTagBitSet BitSet;
	Algo::Copy(ExportData.MassTags, BitSet);
	EntityHandle = ItemData::InitializeItemEntity(EntityManager, Item, BitSet, ExportData.MassInstances);
}

void FFaerieItemInstance::ExportFragmentData(const FMassEntityManager& EntityManager, const FMassEntityHandle Item,
	FFaerieItemExportData& ExportData, const ItemData::EMassFragmentExportOptions Options)
{
	if (EntityManager.IsEntityValid(Item))
	{
		const FMassArchetypeHandle Archetype = EntityManager.GetArchetypeForEntity(Item);

		EntityManager.ForEachArchetypeElementType(Archetype,
			[Item, &EntityManager, &ExportData, Options](const UScriptStruct* FragmentType)
			{
				if (EnumHasAnyFlags(Options, ItemData::OnlyFaerieMassFragments))
				{
					if (!ItemData::IsFaerieMassType(FragmentType))
					{
						return true;
					}
				}

				if (FragmentType->IsChildOf(FFaerieMassTag::StaticStruct()) ||
					FragmentType->IsChildOf(FFaerieMassSparseTag::StaticStruct()))
				{
					ExportData.MassTags.Add(FragmentType);
					return true;
				}

				FConstStructView Fragment;
				if (FragmentType->IsChildOf(FFaerieMassFragment::StaticStruct()) ||
					FragmentType->IsChildOf(FFaerieMassSparseFragment::StaticStruct()))
				{
					Fragment = EntityManager.GetFragmentDataStruct(Item, FragmentType);
				}
				else if (FragmentType->IsChildOf(FFaerieMassSharedFragment::StaticStruct()))
				{
					Fragment = EntityManager.GetSharedFragmentDataStruct(Item, FragmentType);
				}
				else if (FragmentType->IsChildOf(FFaerieMassConstSharedFragment::StaticStruct()))
				{
					Fragment = EntityManager.GetConstSharedFragmentDataStruct(Item, FragmentType);
				}

				if (!Fragment.IsValid())
				{
					return true;
				}

				// If the fragment type has a custom save data type, convert to it.
				if (const ItemData::FMassFragmentTypeInterface* Traits = ItemData::GetFragmentTraitsInterface(Fragment.GetScriptStruct());
					Traits && Traits->HasSaveDataConverter)
				{
					FInstancedStruct SaveDataFragment; // New blank to write to.
					if (Traits->SaveDataConverter(const_cast<uint8*>(Fragment.GetMemory()), const_cast<FMassEntityManager&>(EntityManager), SaveDataFragment))
					{
						ExportData.MassInstances.Add(SaveDataFragment);
					}
				}
				else
				{
					ExportData.MassInstances.Emplace(Fragment);
				}
				return true;
			});
	}
}

namespace Faerie::ItemData
{
	FMassEntityHandle InitializeItemEntity(FMassEntityManager& EntityManager, const UFaerieItem* ItemAsset, const FMassTagBitSet& Tags, const TArrayView<FInstancedStruct> Fragments)
	{
		const FMassEntityTemplate& EntityTemplate = EntityManager.GetWorld()->GetSubsystemChecked<UFaerieMassConfigLoaderSubsystem>()->GetItemDataTemplate();

		FMassArchetypeCompositionDescriptor Composition = EntityTemplate.GetCompositionDescriptor();
		Composition.GetElementsBitSet() += Tags;

		UE::Mass::FEntityBuilder Builder = UE::Mass::FEntityBuilder::Make(EntityManager.AsShared(),
			Composition,
			EntityTemplate.GetInitialFragmentValues(),
			EntityTemplate.GetSharedFragmentValues().GetConstSharedFragments(),
			EntityTemplate.GetSharedFragmentValues().GetSharedFragments());

		// const_cast needed because GetArchetypeCreationParams is not const for some reason. Likely to be fixed later.
		Builder.ConfigureArchetypeCreation(const_cast<FMassEntityTemplateData&>(EntityTemplate.GetTemplateData()).GetArchetypeCreationParams());

		ensureMsgf(EntityTemplate.GetObjectFragmentInitializers().Num() == 0, TEXT("%hs: EntityBuilder doesn't support object initializers (yet)."), __FUNCTION__);

		// Add an ItemDataDefaults fragment to label the entity as a faerie item whether there is a valid item pointer or not.
		Builder.Add<FFaerieItemDataDefaults>(FFaerieItemDataDefaults(ItemAsset));

		// Look for fragments that need to always be moved into the runtime entity on creation
		if (IsValid(ItemAsset))
		{
			for (const FInstancedStruct& DefaultFragment : ItemAsset->GetFragmentDefaults())
			{
				const FMassFragmentTypeInterface* Traits = GetFragmentTraitsInterface(DefaultFragment.GetScriptStruct());
				if (Traits && Traits->HasInitializeRuntime)
				{
					FInstancedStruct FragmentCopy = DefaultFragment;
					if (Traits->InitializeRuntime(FragmentCopy.GetMutableMemory(), EntityManager, Builder.GetEntityHandle()))
					{
						Builder.Add(MoveTemp(FragmentCopy));
					}
				}
			}
		}

		// Add any initial fragments to the builder, running init code as needed
		for (FInstancedStruct& Fragment : Fragments)
		{
			// If the fragment type has a custom save data type, convert from it.
			if (Fragment.GetScriptStruct()->IsChildOf<FFaerieItemFragmentSaveData>())
			{
				TNotNull<const UScriptStruct*> RealFragmentType = Fragment.Get<FFaerieItemFragmentSaveData>().GetFragmentType();
				if (const FMassFragmentTypeInterface* Traits = GetFragmentTraitsInterface(RealFragmentType))
				{
					FInstancedStruct NewFragment; // New blank to write to.
					NewFragment.InitializeAs(RealFragmentType);

					// Load in save data...
					if (Traits->SaveDataConverter(NewFragment.GetMutableMemory(), EntityManager, Fragment))
					{
						Fragment = NewFragment;
					}

					// ... then initialize it. Order is important, because init'ing data that gets overwritten by loading doesn't do any good.
					Traits->InitializeRuntime(NewFragment.GetMutableMemory(), EntityManager, Builder.GetEntityHandle());
				}
			}
			else if (const FMassFragmentTypeInterface* Traits = GetFragmentTraitsInterface(Fragment.GetScriptStruct()))
			{
				Traits->InitializeRuntime(Fragment.GetMutableMemory(), EntityManager, Builder.GetEntityHandle());
			}

			Builder.Add(MoveTemp(Fragment));
		}

		return Builder.Commit();
	}

	bool IsMutable(const FMassEntityManager& EntityManager, const FMassEntityHandle Item)
	{
		if (!EntityManager.IsEntityValid(Item)) return false;

		if (const FFaerieItemDataDefaults* ItemDefaults = EntityManager.GetConstSharedFragmentDataPtr<FFaerieItemDataDefaults>(Item))
		{
			if (const UFaerieItem* ItemAsset = ItemDefaults->ItemAsset)
			{
				// Always report if the ItemAsset is mutable.
				return ItemAsset->CanMutate();
			}
		}

		return true;
	}

	void PostMutationEvent(FMassEntityManager& EntityManager, const FMutationEvent& Event)
	{
		FInstancedStruct EventStruct = FInstancedStruct::Make(Event);
		EntityManager.Defer().PushCommand<FMassDeferredCreateCommand>(
			[EventStruct = MoveTemp(EventStruct)](FMassEntityManager& DeferredEntityManager)
			{
				DeferredEntityManager.CreateEntity(MakeConstArrayView(&EventStruct, 1));
			});
	}

	void PostMutationEventWithChangeList(FMassEntityManager& EntityManager,
		const FMutationEvent& Event, const FMutationPayloadChangeList& ChangeList)
	{
		TStaticArray<FInstancedStruct, 3> EventStructs;
		EventStructs[0] = FInstancedStruct::Make(Event);
		EventStructs[1] = FInstancedStruct::Make(ChangeList);
		EventStructs[2] = FInstancedStruct::Make(AllFields); // Include a FieldChange payload to notify any view models that care that all fields were overwritten.
		EntityManager.Defer().PushCommand<FMassDeferredCreateCommand>(
			[EventStructs = MoveTemp(EventStructs)](FMassEntityManager& DeferredEntityManager)
			{
				DeferredEntityManager.CreateEntity(EventStructs);
			});
	}

	void PostMutationEventWithFieldChange(FMassEntityManager& EntityManager,
		const FMutationEvent& Event, const FFieldChangePayload FieldChanges)
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

	void AddFragment(FMassEntityManager& EntityManager, const FMassEntityHandle Item, FInstancedStruct&& Fragment)
	{
		check(EntityManager.IsEntityValid(Item));

		// Adding a fragment is only allowed for mutable instances.
		check(IsMutable(EntityManager, Item))

		EntityManager.AddFragmentInstanceListToEntity(Item, MakeArrayView(&Fragment, 1));

		FMutationEvent Payload;
		Payload.ItemHandle = Item;
		Payload.EventType = Tags::FragmentAdd;
		Payload.ChangeType = Fragment.GetScriptStruct();
		PostMutationEvent(EntityManager, Payload);
	}

	void AddFragments(FMassEntityManager& EntityManager, const FMassEntityHandle Item, const TArrayView<FInstancedStruct> Fragments)
	{
		check(EntityManager.IsEntityValid(Item));

		// Adding mass fragments are only allowed for mutable instances.
		check(IsMutable(EntityManager, Item))

		EntityManager.AddFragmentInstanceListToEntity(Item, Fragments);

		FMutationEvent Payload;
		Payload.ItemHandle = Item;
		Payload.EventType = Tags::FragmentAdd;
		Payload.ChangeType = FMutationPayloadChangeList::StaticStruct();

		FMutationPayloadChangeList ChangeList;
		for (const FInstancedStruct& Fragment : Fragments)
		{
			ChangeList.ChangeTypes.Add(Fragment.GetScriptStruct());
		}

		PostMutationEventWithChangeList(EntityManager, Payload, ChangeList);
	}

	void RemoveFragment(FMassEntityManager& EntityManager, const FMassEntityHandle Item,
											  const TNotNull<const UScriptStruct*> FragmentType)
	{
		if (EntityManager.IsEntityValid(Item))
		{
			EntityManager.RemoveFragmentFromEntity(Item, FragmentType);

			FMutationEvent Payload;
			Payload.ItemHandle = Item;
			Payload.EventType = Tags::FragmentRemove;
			Payload.ChangeType = FragmentType;
			PostMutationEvent(EntityManager, Payload);
		}
	}

	void RemoveFragments(FMassEntityManager& EntityManager, const FMassEntityHandle Item, const TConstArrayView<const UScriptStruct*> FragmentTypes)
	{
		if (EntityManager.IsEntityValid(Item))
		{
			EntityManager.RemoveFragmentListFromEntity(Item, FragmentTypes);

			FMutationEvent Payload;
			Payload.ItemHandle = Item;
			Payload.EventType = Tags::FragmentRemove;
			Payload.ChangeType = FMutationPayloadChangeList::StaticStruct();

			FMutationPayloadChangeList ChangeList;
			for (const UScriptStruct* FragmentType : FragmentTypes)
			{
				ChangeList.ChangeTypes.Add(FragmentType);
			}

			PostMutationEventWithChangeList(EntityManager, Payload, ChangeList);
		}
	}

	void UpdateFragments(FMassEntityManager& EntityManager, const FMassEntityHandle Item, TArrayView<FInstancedStruct> Fragments,
		const bool ClearOthers)
	{
		if (EntityManager.IsEntityValid(Item))
		{
			if (ClearOthers)
			{
				TArray<const UScriptStruct*> FragmentsToRemove;
				const FMassArchetypeHandle Archetype = EntityManager.GetArchetypeForEntity(Item);
				EntityManager.ForEachArchetypeElementType(Archetype,
					[&Fragments, &FragmentsToRemove](const UScriptStruct* FragmentType)
					{
						if (IsFaerieMassType(FragmentType))
						{
							for (FInstancedStruct& NewFragment : Fragments)
							{
								if (NewFragment.GetScriptStruct() == FragmentType)
								{
									// New fragment for this type, leave it.
									return true;
								}
							}

							// No new fragment for this type, remove it.
							FragmentsToRemove.Add(FragmentType);
						}

						return true;
					});

				// Commit and emit event for removed fragments
				RemoveFragments(EntityManager, Item, FragmentsToRemove);
			}

			// Commit and emit event for added fragments
			AddFragments(EntityManager, Item, Fragments);
		}
	}

	void OnItemFragmentEdited(FMassEntityManager& EntityManager, const FMassEntityHandle Item,
		const TNotNull<const UScriptStruct*> FragmentType, const FFieldChangePayload& FieldChange)
	{
		FMutationEvent Payload;
		Payload.ItemHandle = Item;
		Payload.ChangeType = FragmentType;
		Payload.EventType = Tags::FragmentGenericPropertyEdit;
		PostMutationEventWithFieldChange(EntityManager, Payload, FieldChange);
	}
}
