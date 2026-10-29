// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataEnums.h"
#include "FaerieMassFragment.h"
#include "MassElement.h"

#include "Mass/EntityHandle.h"

#include "StructUtils/InstancedStruct.h"

#define FAE_API FAERIEITEMDATA_API

struct FFaerieItemExportData;

namespace Faerie::ItemData
{
	struct FMutationPayloadChangeList;
	struct FFieldChangePayload;
	struct FMutationEvent;
}

struct FMassEntityManager;

namespace Faerie::ItemData
{
	/*
	 * Import mass fragments to this item instance.
	 */
	[[nodiscard]] FAE_API FMassEntityHandle ImportFragmentData(FMassEntityManager& EntityManager, const FFaerieItemExportData& ExportData);

	/*
	 * Export a list of all mass fragments for this item instance.
	 */
	FAE_API void ExportFragmentData(const FMassEntityManager& EntityManager, FMassEntityHandle Item, FFaerieItemExportData& ExportData, EMassFragmentExportOptions Options);


	[[nodiscard]] FAE_API FMassEntityHandle InitializeItemEntity(FMassEntityManager& EntityManager, const FMassTagBitSet& Tags, TArray<FInstancedStruct>&& Fragments);
	[[nodiscard]] FAE_API FMassEntityHandle InitializeItemEntity(FMassEntityManager& EntityManager, const FMassTagBitSet& Tags, TConstArrayView<FInstancedStruct> Fragments);

	FAE_API bool IsMutable(const FMassEntityManager& EntityManager, FMassEntityHandle Item);

	FAE_API void PostMutationEvent(FMassEntityManager& EntityManager, const FMutationEvent& Event);
	FAE_API void PostMutationEventWithChangeList(FMassEntityManager& EntityManager, const FMutationEvent& Event, const FMutationPayloadChangeList& ChangeList);
	FAE_API void PostMutationEventWithFieldChange(FMassEntityManager& EntityManager, const FMutationEvent& Event, FFieldChangePayload FieldChanges);

	// @todo do we need to make Deferred command versions of these?
	FAE_API void AddFragment(FMassEntityManager& EntityManager, FMassEntityHandle Item, FInstancedStruct&& Fragment);
	FAE_API void AddFragments(FMassEntityManager& EntityManager, FMassEntityHandle Item, TConstArrayView<FInstancedStruct> Fragments);

	FAE_API void RemoveFragment(FMassEntityManager& EntityManager, FMassEntityHandle Item, TNotNull<const UScriptStruct*> FragmentType);
	FAE_API void RemoveFragments(FMassEntityManager& EntityManager, FMassEntityHandle Item, TConstArrayView<const UScriptStruct*> FragmentTypes);

	FAE_API void UpdateFragments(FMassEntityManager& EntityManager, FMassEntityHandle Item, TConstArrayView<FInstancedStruct> Fragments, bool ClearOthers);

	FAE_API void OnItemFragmentEdited(FMassEntityManager& EntityManager, FMassEntityHandle Item, TNotNull<const UScriptStruct*> FragmentType, const FFieldChangePayload& FieldChange);
}

#undef FAE_API