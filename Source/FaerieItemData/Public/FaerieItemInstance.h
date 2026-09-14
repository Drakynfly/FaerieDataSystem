// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataEnums.h"
#include "FaerieItemDataFwd.h"
#include "GameplayTagContainer.h"

#include "Mass/EntityHandle.h"

#include "StructUtils/InstancedStruct.h"

#include "FaerieItemInstance.generated.h"

namespace Faerie::ItemData
{
	struct FFieldChange;
}

struct FMassEntityManager;

USTRUCT()
struct FAERIEITEMDATA_API FFaerieItemInstance
{
	GENERATED_BODY()

private:
	explicit FFaerieItemInstance(const UFaerieItem* Item) : Item(Item) {}

public:
	FFaerieItemInstance() = default;

	FFaerieItemInstance(const UFaerieItem* Item, const FMassEntityHandle Handle)
	  : Item(Item), EntityHandle(Handle) {}

	static FFaerieItemInstance FromPointer(const UFaerieItem* Item)
	{
		return FFaerieItemInstance(Item);
	}

	static FFaerieItemInstance FromFragments(FMassEntityManager& EntityManager, const TArrayView<FInstancedStruct> Fragments)
	{
		FFaerieItemInstance Instance;
		Instance.ImportFragmentData(EntityManager, Fragments);
		return Instance;
	}

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
	 * The value is set by calling InitializeMassEntity/InitializeMassEntityIfInvalid, or
	 */
	UPROPERTY(VisibleAnywhere, Category = "ItemInstance")
	FMassEntityHandle EntityHandle;

private:
	void InitializeMassEntityImpl(FMassEntityManager& EntityManager, TArrayView<FInstancedStruct> Fragments);
	void UpdateTimestamp(const FMassEntityManager& EntityManager, bool CreateIfMissing) const;
	void NotifyOwnerOfChange(const FMassEntityManager& EntityManager, FGameplayTag Tag) const;

public:
	UE_REWRITE bool HasItemAsset() const { return !!Item; }
	UE_REWRITE bool HasMassEntity() const { return EntityHandle.IsSet(); }

	UE_REWRITE bool IsMassEntityNull() const { return !EntityHandle.IsSet(); }

	// The instance is empty if it has neither a MassEntityHandle nor an ItemAsset pointer.
	UE_REWRITE bool IsEmpty() const { return IsMassEntityNull() && !HasItemAsset(); }

	UE_REWRITE const UFaerieItem* GetItemPtr() const { return Item; }
	UE_REWRITE FMassEntityHandle GetMassEntityHandle() const { return EntityHandle; }

	bool IsMutable() const;
	bool CanStack() const;

	void InitializeMassEntity(FMassEntityManager& EntityManager, TArrayView<FInstancedStruct> Fragments = {});
	void InitializeMassEntityIfInvalid(FMassEntityManager& EntityManager);

	// Call on Server Only to destroy the mass entity for this item instance.
	void DestroyMassEntity(FMassEntityManager& EntityManager);

	/*
	 * Import mass fragments to this item instance.
	 */
	void ImportFragmentData(FMassEntityManager& EntityManager, TArrayView<FInstancedStruct> Fragments);

	/*
	 * Export a list of all mass fragments for this item instance.
	 */
	void ExportFragmentData(const FMassEntityManager& EntityManager, TArray<FInstancedStruct>& OutStructs, Faerie::ItemData::EMassFragmentExportOptions Options) const;


	/**~~-									-~~**/
	/**~~-		MUTABLE INSTANCE API		-~~**/
	/**~~-									-~~**/

	// @todo do we need to make Deferred command versions of these?
	void AddFragment(FMassEntityManager& EntityManager, FInstancedStruct&& Fragment);
	void AddFragments(FMassEntityManager& EntityManager, TArrayView<FInstancedStruct> Fragments);

	void RemoveFragment(FMassEntityManager& EntityManager, TNotNull<const UScriptStruct*> FragmentType);

	void TempNestedContainerChanged(const FMassEntityManager& EntityManager) const;
	void OnItemFragmentEdited(const FMassEntityManager& EntityManager, TConstStructView<FFaerieMassFragment> FragmentView, const Faerie::ItemData::FFieldChange& FieldChange) const;

	/* Generic operations */

	[[nodiscard]] bool UEOpEquals(const FFaerieItemInstance& Other) const;

	friend [[nodiscard]] UE_REWRITE uint32 GetTypeHash(const FFaerieItemInstance& Value)
	{
		return HashCombineFast(GetTypeHash(Value.GetItemPtr()), GetTypeHash(Value.GetMassEntityHandle()));
	}
};