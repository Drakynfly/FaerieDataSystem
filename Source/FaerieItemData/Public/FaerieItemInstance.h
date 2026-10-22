// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataEnums.h"
#include "FaerieItemDataFwd.h"
#include "FaerieItemEvent.h"
#include "FaerieMassFragment.h"

#include "Mass/EntityHandle.h"

#include "StructUtils/InstancedStruct.h"

#include "FaerieItemInstance.generated.h"

#define FAE_API FAERIEITEMDATA_API

struct FFaerieItemExportData;

namespace Faerie::ItemData
{
	struct FFieldChangePayload;
	struct FMutationEvent;
}

struct FMassEntityManager;

USTRUCT()
struct FAE_API FFaerieItemInstance
{
	GENERATED_BODY()

private:
	explicit FFaerieItemInstance(const UFaerieItem* Item) : Item(Item) {}
	explicit FFaerieItemInstance(const FMassEntityHandle Item) : EntityHandle(Item) {}

public:
	FFaerieItemInstance() = default;

	static FFaerieItemInstance FromPointer(const UFaerieItem* Item)
	{
		return FFaerieItemInstance(Item);
	}

	static FFaerieItemInstance FromEntity(const FMassEntityHandle Item)
	{
		return FFaerieItemInstance(Item);
	}

	void ConvertToRuntimeInPlace(FMassEntityManager& EntityManager);

protected:
	/*
	 * The UObject containing pre-defined default values for this instance. For dynamically created item instances,
	 * this may be null, if the instance was generated entirely from mass fragments.
	 * To create an item instance with a valid Item, use FFaerieItemInstance::FromPointer().
	 */
	UPROPERTY(VisibleAnywhere, Category = "ItemInstance")
	TObjectPtr<const UFaerieItem> Item;

	/*
	 * The Mass Entity handle for item instances that have runtime mutable data, or otherwise are associated with mass
	 * fragments.
	 * The value is set by calling InitializeMassEntity/InitializeMassEntityIfInvalid, or using FFaerieItemInstance::FromFragments().
	 */
	UPROPERTY(VisibleAnywhere, Category = "ItemInstance")
	FMassEntityHandle EntityHandle;

public:
	UE_REWRITE bool HasItemAsset() const { return !!Item; }
	UE_REWRITE bool HasMassEntity() const { return EntityHandle.IsSet(); }

	UE_REWRITE bool IsMassEntityNull() const { return !EntityHandle.IsSet(); }

	// The instance is empty if it has neither a MassEntityHandle nor an ItemAsset pointer.
	UE_REWRITE bool IsEmpty() const { return IsMassEntityNull() && !HasItemAsset(); }

	UE_REWRITE const UFaerieItem* GetItemPtr() const { return Item; }
	UE_REWRITE FMassEntityHandle GetMassEntityHandle() const { return EntityHandle; }

	bool IsMutable() const;

	/*
	 * Import mass fragments to this item instance.
	 */
	void ImportFragmentData(FMassEntityManager& EntityManager, FFaerieItemExportData& ExportData);

	/*
	 * Export a list of all mass fragments for this item instance.
	 */
	static void ExportFragmentData(const FMassEntityManager& EntityManager, FMassEntityHandle Item, FFaerieItemExportData& ExportData, Faerie::ItemData::EMassFragmentExportOptions Options);


	/* Generic operations */

	[[nodiscard]] bool UEOpEquals(const FFaerieItemInstance& Other) const;

	[[nodiscard]] UE_REWRITE friend uint32 GetTypeHash(const FFaerieItemInstance& Value)
	{
		return HashCombineFast(GetTypeHash(Value.GetItemPtr()), GetTypeHash(Value.GetMassEntityHandle()));
	}
};

namespace Faerie::ItemData
{
	[[nodiscard]] FAE_API FMassEntityHandle InitializeItemEntity(FMassEntityManager& EntityManager, const UFaerieItem* ItemAsset, const FMassTagBitSet& Tags, TConstArrayView<FInstancedStruct> Fragments);

	// Utility for getting a view of a faerie fragment type
	// @Todo move elsewhere
	[[nodiscard]] FAE_API FConstStructView GetFaerieFragmentView(const FMassEntityManager& EntityManager, FMassEntityHandle Item, TNotNull<const UScriptStruct*> Type);

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

	// Call on Server Only to destroy the mass entity for this item instance.
	FAE_API void DestroyMassEntity(FMassEntityManager& EntityManager, FMassEntityHandle Item);
}

#undef FAE_API