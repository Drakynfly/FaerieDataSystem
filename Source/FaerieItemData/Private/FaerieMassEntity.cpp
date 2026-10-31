// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieMassEntity.h"
#include "FaerieMassFragmentAccessors.h"
#include "FaerieItemAsset.h"
#include "FaerieItemDataLog.h"
#include "FaerieItemEvent.h"
#include "FaerieItemExportData.h"
#include "FaerieMassFragment.h"
#include "FaerieMassTypeTraits.h"
#include "MassEntityBuilder.h"
#include "MassEntityTemplate.h"

#include "Subsystems/ConfigLoaderSubsystem.h"

#include "Engine/World.h"

namespace Faerie::ItemData
{
	FMassEntityHandle ImportFragmentData(FMassEntityManager& EntityManager, const FFaerieItemExportData& ExportData)
	{
		FFaerieItemExportData CopyToFilter = ExportData;

		for (auto&& It = CopyToFilter.MassTags.CreateIterator(); It; ++It)
		{
			if (const FMassTypeTraitsInterface* Traits = GetMassTraitsInterface(*It))
			{
				if (!Traits->Serialized)
				{
					UE_LOGF(LogFaerieItemData, Warning, "ImportFragmentData: found tag '%ls' that is barred from serialization. Discarding... ", *GetNameSafe(*It))
					It.RemoveCurrentSwap();
				}
			}
		}

		for (auto&& It = CopyToFilter.MassInstances.CreateIterator(); It; ++It)
		{
			if (const FMassTypeTraitsInterface* Traits = GetMassTraitsInterface(It->GetScriptStruct()))
			{
				if (!Traits->Serialized)
				{
					UE_LOGF(LogFaerieItemData, Warning, "ImportFragmentData: found fragment '%ls' that is barred from serialization. Discarding... ", *GetNameSafe(It->GetScriptStruct()))
					It.RemoveCurrentSwap();
				}
			}
		}

		FMassTagBitSet BitSet;
		Algo::Copy(CopyToFilter.MassTags, BitSet);
		return InitializeItemEntity(EntityManager, BitSet, MoveTemp(CopyToFilter.MassInstances));
	}

	void ExportFragmentData(const FMassEntityManager& EntityManager, const FMassEntityHandle Item,
		FFaerieItemExportData& ExportData, const EMassFragmentExportOptions Options)
	{
		if (EntityManager.IsEntityValid(Item))
		{
			const FMassArchetypeHandle Archetype = EntityManager.GetArchetypeForEntity(Item);

			EntityManager.ForEachArchetypeElementType(Archetype,
				[Item, &EntityManager, &ExportData, Options](const UScriptStruct* ElementType)
				{
					if (EnumHasAnyFlags(Options, OnlyFaerieMassFragments))
					{
						if (!IsFaerieMassType(ElementType))
						{
							return true;
						}
					}

					if (IsFaerieMassTag(ElementType))
					{
						if (const FMassTypeTraitsInterface* Traits = GetMassTraitsInterface(ElementType))
						{
							if (!Traits->Serialized)
							{
								// Skip fragments that have serialization disabled by its traits.
								return true;
							}
						}

						ExportData.MassTags.Add(ElementType);
						return true;
					}

					FConstStructView Fragment = GetItemFragment(EntityManager, Item, ElementType);
					if (!Fragment.IsValid())
					{
						return true;
					}

					// If the fragment type has a custom save data type, convert to it.
					if (const FMassTypeTraitsInterface* Traits = GetMassTraitsInterface(ElementType))
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
						else
						{
							ExportData.MassInstances.Emplace(Fragment);
						}
					}
					else
					{
						// By default, export fragments that don't implement any traits overrides.
						ExportData.MassInstances.Emplace(Fragment);
					}
					return true;
				});
		}
	}

	FMassEntityHandle InitializeItemEntity(FMassEntityManager& EntityManager, const FMassTagBitSet& Tags, TArray<FInstancedStruct>&& Fragments)
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

		// Add any initial fragments to the builder, running init code as needed
		for (FInstancedStruct& Fragment : Fragments)
		{
			// If the fragment type has a custom save data type, convert from it.
			if (Fragment.GetScriptStruct()->IsChildOf<FFaerieItemFragmentSaveData>())
			{
				TNotNull<const UScriptStruct*> RealFragmentType = Fragment.Get<FFaerieItemFragmentSaveData>().GetFragmentType();
				const FMassTypeTraitsInterface* Traits = GetMassTraitsInterface(RealFragmentType);
				if (!Traits)
				{
					UE_LOGF(LogFaerieItemData, Error, "Fragment '%ls' must implement SaveDataConverter in order to convert from '%ls'!", *GetNameSafe(RealFragmentType), *GetNameSafe(Fragment.GetScriptStruct()))
				}
				checkf(Traits->HasSaveDataConverter, TEXT("Traits must implement SaveDataConverter if its returned as the fragment type of an ItemFragmentSaveData type!"))

				FInstancedStruct NewFragment; // New blank to write to.
				NewFragment.InitializeAs(RealFragmentType);

				// Load in save data...
				if (Traits->SaveDataConverter(NewFragment.GetMutableMemory(), EntityManager, Fragment))
				{
					Fragment = NewFragment;
				}
				else
				{
					UE_LOGF(LogFaerieItemData, Error, "Fragment '%ls' failed SaveDataConverter. Converted from '%ls'!", *GetNameSafe(RealFragmentType), *GetNameSafe(Fragment.GetScriptStruct()))
				}

				// ... then initialize it. Order is important, because init'ing data that gets overwritten by loading doesn't do any good.
				if (!Traits->InitializeRuntime(NewFragment.GetMutableMemory(), Builder))
				{
					UE_LOGF(LogFaerieItemData, Error, "Fragment '%ls' failed InitializeRuntime. Converted from '%ls'!", *GetNameSafe(RealFragmentType), *GetNameSafe(Fragment.GetScriptStruct()))
				}
			}
			else if (const FMassTypeTraitsInterface* Traits = GetMassTraitsInterface(Fragment.GetScriptStruct()))
			{
				if (!Traits->InitializeRuntime(Fragment.GetMutableMemory(), Builder))
				{
					UE_LOGF(LogFaerieItemData, Error, "Fragment '%ls' failed InitializeRuntime!", *GetNameSafe(Fragment.GetScriptStruct()))
				}
			}

			Builder.Add(MoveTemp(Fragment));
		}

		return Builder.Commit();
	}

	FMassEntityHandle InitializeItemEntity(FMassEntityManager& EntityManager, const FMassTagBitSet& Tags, const TConstArrayView<FInstancedStruct> Fragments)
	{
		TArray<FInstancedStruct> Copy(Fragments);
		return InitializeItemEntity(EntityManager, Tags, MoveTemp(Copy));
	}

	bool IsMutable(const FMassEntityManager& EntityManager, const FMassEntityHandle Item)
	{
		if (!EntityManager.IsEntityValid(Item)) return false;

		// We check the Defaults asset for mutability.
		// @Todo should we check all FFaerieMassItemObject fragments?
		if (const FFaerieItemDataDefaults* ItemDefaults = EntityManager.GetConstSharedFragmentDataPtr<FFaerieItemDataDefaults>(Item))
		{
			if (const UFaerieItemAsset* ItemAsset = ItemDefaults->ItemAsset)
			{
				// Always report if the ItemAsset is mutable.
				return ItemAsset->GetCanMutate();
			}
		}

		// If no defaults item is present, assume we are mutable.
		// @todo we could also add a tag or something if we need to block mutation when an item asset isn't present...
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

		TArray<FInstancedStruct, TInlineAllocator<4>> SimpleFragments;
		TArray<FSharedStruct, TInlineAllocator<2>> SharedFragments;
		TArray<FConstSharedStruct, TInlineAllocator<2>> ConstSharedFragments;

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
