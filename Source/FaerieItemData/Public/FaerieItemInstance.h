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

USTRUCT(BlueprintType)
struct FAERIEITEMDATA_API FFaerieItemInstance
{
	GENERATED_BODY()

private:
	explicit FFaerieItemInstance(const UFaerieItem* Item) : Item(Item) {}

public:
	FFaerieItemInstance() = default;

	FFaerieItemInstance(const UFaerieItem* Item, const FMassEntityHandle Handle)
	  : Item(Item), EntityHandle(Handle)
	{}

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
	UPROPERTY(VisibleAnywhere, Category = "ItemInstance")
	TObjectPtr<const UFaerieItem> Item;

	UPROPERTY(VisibleAnywhere, Category = "ItemInstance")
	FMassEntityHandle EntityHandle;

private:
	void InitializeMassEntityImpl(FMassEntityManager& EntityManager, TArrayView<FInstancedStruct> Fragments);
	void UpdateTimestamp(bool CreateIfMissing) const;
	void NotifyOwnerOfChange(const FMassEntityManager& EntityManager, TNotNull<const UScriptStruct*> FragmentType, FGameplayTag Tag) const;

public:
	UE_REWRITE bool HasItemAsset() const { return !!Item; }
	UE_REWRITE bool HasMassEntity() const { return EntityHandle.IsValid(); }

	UE_REWRITE const UFaerieItem* GetItemPtr() const { return Item; }
	UE_REWRITE FMassEntityHandle GetMassEntityHandle() const { return EntityHandle; }

	UE_REWRITE bool IsValid() const { return HasItemAsset() || HasMassEntity(); }

	bool IsMutable() const;

	void InitializeMassEntity(FMassEntityManager& EntityManager, TArrayView<FInstancedStruct> Fragments = {});
	void InitializeMassEntityIfInvalid(FMassEntityManager& EntityManager);

	void DestroyMassEntity(FMassEntityManager& EntityManager);

	/*
	 * Import mass fragments to this item instance.
	 */
	void ImportFragmentData(FMassEntityManager& EntityManager, TArrayView<FInstancedStruct> Fragments);

	/*
	 * Export a list of all mass fragments for this item instance.
	 */
	void ExportFragmentData(const FMassEntityManager& EntityManager, TArray<FInstancedStruct>& OutStructs, Faerie::ItemData::EMassFragmentExportOptions Options) const;

	// @todo do we need to make Deferred command versions of these?
	void AddFragment(FMassEntityManager& EntityManager, FInstancedStruct&& Fragment);
	void AddFragments(FMassEntityManager& EntityManager, TArrayView<FInstancedStruct> Fragments);

	void RemoveFragment(FMassEntityManager& EntityManager, TNotNull<const UScriptStruct*> FragmentType);

	void OnItemFragmentEdited(const FMassEntityManager& EntityManager, TNotNull<const UScriptStruct*> FragmentType, FGameplayTag Tag) const;
	void OnItemFragmentEdited(const FMassEntityManager& EntityManager, TConstStructView<FFaerieMassFragment> FragmentView, const Faerie::ItemData::FFieldChange& FieldChange) const;

	//~		INTEROP FUNCTIONS WHILE UPGRADING	 ~/

	FDateTime GetLastModified() const;

	[[nodiscard]] bool UEOpEquals(const FFaerieItemInstance& Other) const;

	friend [[nodiscard]] UE_REWRITE uint32 GetTypeHash(const FFaerieItemInstance& Value)
	{
		return HashCombineFast(GetTypeHash(Value.GetItemPtr()), GetTypeHash(Value.GetMassEntityHandle()));
	}
};

/*
 * A struct used to keep track of a faerie item over time. Currently implemented as a pointer to the item instance.
 * This exists to be future proofing against later changes to UFaerieItem.
 */
USTRUCT()
struct FFaerieItemStableHandle
{
	GENERATED_BODY()

	FFaerieItemStableHandle() = default;
	FFaerieItemStableHandle(const FFaerieItemInstance& Item)
	  : Item(Item) {}

	bool IsValid() const { return Item.IsValid(); }

		  FFaerieItemInstance& Get() { return Item; }
	const FFaerieItemInstance& Get() const { return Item; }

protected:
	UPROPERTY()
	FFaerieItemInstance Item;
};