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

void FFaerieItemInstance::ConvertToRuntimeInPlace(FMassEntityManager& EntityManager)
{
	check(!EntityHandle.IsSet())
	EntityHandle = ItemData::InitializeItemEntity(EntityManager, Item, FMassTagBitSet(), {});
	Item = nullptr;
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
	return EntityHandle == Other.EntityHandle && Item == Other.Item;
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

	for (auto&& It = ExportData.MassTags.CreateIterator(); It; ++It)
	{
		if (const ItemData::FMassTypeTraitsInterface* Traits = ItemData::GetFragmentTraitsInterface(*It))
		{
			if (!Traits->Serialized)
			{
				UE_LOGF(LogFaerieItemData, Warning, "ImportFragmentData: found tag '%ls' that is barred from serialization. Discarding... ", *GetNameSafe(*It))
				It.RemoveCurrentSwap();
			}
		}
	}

	for (auto&& It = ExportData.MassInstances.CreateIterator(); It; ++It)
	{
		if (const ItemData::FMassTypeTraitsInterface* Traits = ItemData::GetFragmentTraitsInterface(It->GetScriptStruct()))
		{
			if (!Traits->Serialized)
			{
				UE_LOGF(LogFaerieItemData, Warning, "ImportFragmentData: found fragment '%ls' that is barred from serialization. Discarding... ", *GetNameSafe(It->GetScriptStruct()))
				It.RemoveCurrentSwap();
			}
		}
	}

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

				if (ItemData::IsFaerieMassTag(FragmentType))
				{
					if (const ItemData::FMassTypeTraitsInterface* Traits = ItemData::GetFragmentTraitsInterface(FragmentType))
					{
						if (!Traits->Serialized)
						{
							// Skip fragments that have serialization disabled by its traits.
							return true;
						}
					}

					ExportData.MassTags.Add(FragmentType);
					return true;
				}

				FConstStructView Fragment = ItemData::GetFaerieFragmentView(EntityManager, Item, FragmentType);
				if (!Fragment.IsValid())
				{
					return true;
				}

				// If the fragment type has a custom save data type, convert to it.
				if (const ItemData::FMassTypeTraitsInterface* Traits = ItemData::GetFragmentTraitsInterface(FragmentType))
				{
					if (!Traits->Serialized)
					{
						// Skip fragments that have serialization disabled by its traits.
						return true;
					}

					FInstancedStruct SaveDataFragment; // New blank to write to.
					if (Traits->HasSaveDataConverter && Traits->SaveDataConverter(const_cast<uint8*>(Fragment.GetMemory()),
						const_cast<FMassEntityManager&>(EntityManager), SaveDataFragment))
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
	FMassEntityHandle InitializeItemEntity(FMassEntityManager& EntityManager, const UFaerieItem* ItemAsset, const FMassTagBitSet& Tags, const TConstArrayView<FInstancedStruct> Fragments)
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

		if (ItemAsset)
		{
			Builder.Add<FFaerieItemDataDefaults>(FFaerieItemDataDefaults(ItemAsset));
		}

		// Look for fragments that need to always be moved into the runtime entity on creation
		if (IsValid(ItemAsset))
		{
			for (const FInstancedStruct& DefaultFragment : ItemAsset->GetFragmentDefaults())
			{
				const FMassTypeTraitsInterface* Traits = GetFragmentTraitsInterface(DefaultFragment.GetScriptStruct());
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
		for (FInstancedStruct Fragment : Fragments)
		{
			// If the fragment type has a custom save data type, convert from it.
			if (Fragment.GetScriptStruct()->IsChildOf<FFaerieItemFragmentSaveData>())
			{
				TNotNull<const UScriptStruct*> RealFragmentType = Fragment.Get<FFaerieItemFragmentSaveData>().GetFragmentType();
				if (const FMassTypeTraitsInterface* Traits = GetFragmentTraitsInterface(RealFragmentType))
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
			else if (const FMassTypeTraitsInterface* Traits = GetFragmentTraitsInterface(Fragment.GetScriptStruct()))
			{
				Traits->InitializeRuntime(Fragment.GetMutableMemory(), EntityManager, Builder.GetEntityHandle());
			}

			Builder.Add(MoveTemp(Fragment));
		}

		return Builder.Commit();
	}

	FConstStructView GetFaerieFragmentView(const FMassEntityManager& EntityManager, const FMassEntityHandle Item, const TNotNull<const UScriptStruct*> Type)
	{
		if (Type->IsChildOf(FFaerieMassFragment::StaticStruct()) ||
			Type->IsChildOf(FFaerieMassSparseFragment::StaticStruct()))
		{
			return EntityManager.GetFragmentDataStruct(Item, Type);
		}
		if (Type->IsChildOf(FFaerieMassSharedFragment::StaticStruct()))
		{
			return EntityManager.GetSharedFragmentDataStruct(Item, Type);
		}
		if (Type->IsChildOf(FFaerieMassConstSharedFragment::StaticStruct()))
		{
			return EntityManager.GetConstSharedFragmentDataStruct(Item, Type);
		}
		return FConstStructView();
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

		const UScriptStruct* Type = Fragment.GetScriptStruct();
		if (Type->IsChildOf<FFaerieMassFragment>() || Type->IsChildOf<FFaerieMassSparseFragment>())
		{
			EntityManager.AddFragmentInstanceListToEntity(Item, MakeArrayView(&Fragment, 1));
		}
		else if (Type->IsChildOf<FFaerieMassSharedFragment>())
		{
			const FSharedStruct SharedFragment = EntityManager.GetOrCreateSharedFragment(*Type, Fragment.GetMemory());
			EntityManager.RemoveSharedFragmentFromEntity(Item, SharedFragment.GetScriptStruct());
			EntityManager.AddSharedFragmentToEntity(Item, SharedFragment);
		}
		else if (Type->IsChildOf<FFaerieMassConstSharedFragment>())
		{
			const FConstSharedStruct ConstSharedFragment = EntityManager.GetOrCreateConstSharedFragment(*Type, Fragment.GetMemory());
			EntityManager.RemoveConstSharedFragmentFromEntity(Item, ConstSharedFragment.GetScriptStruct());
			EntityManager.AddConstSharedFragmentToEntity(Item, ConstSharedFragment);
		}
		else
		{
			check(0)
		}

		const FMutationEvent Payload(Item, Tags::FragmentAdd, Type);
		PostMutationEvent(EntityManager, Payload);
	}

	void AddFragments(FMassEntityManager& EntityManager, const FMassEntityHandle Item, const TConstArrayView<FInstancedStruct> Fragments)
	{
		check(EntityManager.IsEntityValid(Item));

		TArray<FInstancedStruct> SimpleFragments;
		TArray<FSharedStruct> SharedFragments;
		TArray<FConstSharedStruct> ConstSharedFragments;

		for (auto&& Fragment : Fragments)
		{
			const UScriptStruct* Type = Fragment.GetScriptStruct();
			if (Type->IsChildOf<FFaerieMassFragment>() || Type->IsChildOf<FFaerieMassSparseFragment>())
			{
				SimpleFragments.Add(Fragment);
			}
			else if (Type->IsChildOf<FFaerieMassSharedFragment>())
			{
				SharedFragments.Add(EntityManager.GetOrCreateSharedFragment(*Type, Fragment.GetMemory()));
			}
			else if (Type->IsChildOf<FFaerieMassConstSharedFragment>())
			{
				ConstSharedFragments.Add(EntityManager.GetOrCreateConstSharedFragment(*Type, Fragment.GetMemory()));
			}
			else
			{
				check(0)
			}
		}

		EntityManager.AddFragmentInstanceListToEntity(Item, SimpleFragments);

		// Cannot set value of shared fragments, so we have to remove any existing ones first. Note that this is massively
		// expensive as it triggers two archetype rebuilds. Oh well.
		for (const FSharedStruct& SharedFragment : SharedFragments)
		{
			EntityManager.RemoveSharedFragmentFromEntity(Item, SharedFragment.GetScriptStruct());
			EntityManager.AddSharedFragmentToEntity(Item, SharedFragment);
		}
		for (const FConstSharedStruct& ConstSharedFragment : ConstSharedFragments)
		{
			EntityManager.RemoveConstSharedFragmentFromEntity(Item, ConstSharedFragment.GetScriptStruct());
			EntityManager.AddConstSharedFragmentToEntity(Item, ConstSharedFragment);
		}

		const FMutationEvent Payload(Item, Tags::FragmentAdd, FMutationPayloadChangeList::StaticStruct());

		FMutationPayloadChangeList ChangeList;
		for (const FInstancedStruct& Fragment : Fragments)
		{
			ChangeList.ChangeTypes.Add(Fragment.GetScriptStruct());
		}

		PostMutationEventWithChangeList(EntityManager, Payload, ChangeList);
	}

	void RemoveFragment(FMassEntityManager& EntityManager, const FMassEntityHandle Item, const TNotNull<const UScriptStruct*> FragmentType)
	{
		if (EntityManager.IsEntityValid(Item))
		{
			EntityManager.RemoveFragmentFromEntity(Item, FragmentType);

			const FMutationEvent Payload(Item, Tags::FragmentRemove, FragmentType);
			PostMutationEvent(EntityManager, Payload);
		}
	}

	void RemoveFragments(FMassEntityManager& EntityManager, const FMassEntityHandle Item, const TConstArrayView<const UScriptStruct*> FragmentTypes)
	{
		if (EntityManager.IsEntityValid(Item))
		{
			EntityManager.RemoveFragmentListFromEntity(Item, FragmentTypes);

			const FMutationEvent Payload(Item, Tags::FragmentRemove, FMutationPayloadChangeList::StaticStruct());

			FMutationPayloadChangeList ChangeList;
			for (const UScriptStruct* FragmentType : FragmentTypes)
			{
				ChangeList.ChangeTypes.Add(FragmentType);
			}

			PostMutationEventWithChangeList(EntityManager, Payload, ChangeList);
		}
	}

	void UpdateFragments(FMassEntityManager& EntityManager, const FMassEntityHandle Item, TConstArrayView<FInstancedStruct> Fragments,
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
							for (const FInstancedStruct& NewFragment : Fragments)
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
		const FMutationEvent Payload(Item, Tags::FragmentGenericPropertyEdit, FragmentType);
		PostMutationEventWithFieldChange(EntityManager, Payload, FieldChange);
	}
}
