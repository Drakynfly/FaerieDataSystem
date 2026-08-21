// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataConcepts.h"
#include "FaerieItemDataDefines.h"
#include "FaerieItemDataFwd.h"
#include "FaerieItemDataEnums.h"
#include "FaerieMassFragment.h"
#include "MassEntityManager.h"

#include "Mass/EntityHandle.h"
#include "NativeGameplayTags.h"

#include "StructUtils/StructView.h"

#include "UObject/ObjectKey.h"

#include "FaerieItem.generated.h"

namespace Faerie::ItemData
{
	namespace Tags
	{
		FAERIEITEMDATA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(FragmentAdd)
		FAERIEITEMDATA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(FragmentRemove)
		FAERIEITEMDATA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(FragmentGenericPropertyEdit)

		// @todo remove this tag
		FAERIEITEMDATA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(PrimaryIdentifier);

		FAERIEITEMDATA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ReferenceDefaults)
	}
}

/**
 * A runtime instance of an item.
 */
UCLASS(DefaultToInstanced, EditInlineNew, BlueprintType)
class FAERIEITEMDATA_API UFaerieItem : public UObject
{
	GENERATED_BODY()

	friend class UFaerieItemAsset;

public:
	//~ Begin UObject interface
	virtual void PostInitProperties() override;
	virtual void PreSave(FObjectPreSaveContext SaveContext) override;
	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
	//~ End UObject interface

private:
	// Hide GetWorld from API.
	// ReSharper disable once CppOverrideWithDifferentVisibility
	UE_REWRITE virtual class UWorld* GetWorld() const override { return Super::GetWorld(); }

public:
#if WITH_EDITOR
	// Creates a new faerie item object with the given fragments. These are instance-mutable by default.
	static TNotNull<const UFaerieItem*> CreateNewInstance(TConstArrayView<FInstancedStruct> Fragments, TNotNull<UObject*> InstanceOuter, EFaerieItemInstancingMutability Mutability = EFaerieItemInstancingMutability::Automatic);

	// Creates a new faerie item object using this instance as a template. Duplicates are instance-mutable by default.
	TNotNull<const UFaerieItem*> CreateDuplicate(TNotNull<UObject*> InstanceOuter, EFaerieItemInstancingMutability Mutability = EFaerieItemInstancingMutability::Automatic) const;
#endif

	UE_REWRITE TConstArrayView<FInstancedStruct> GetFragmentDefaults() const { return FragmentDefaults; }

	UE_REWRITE int32 GetAssetFormatVersion() const { return FormatVersion; }

	// Look for a default fragment value in an item asset.
	TConstStructView<FFaerieMassFragment> GetDefaultFragment(TNotNull<const UScriptStruct*> StructType, const FGameplayTag ReferenceTag = Faerie::ItemData::Tags::ReferenceDefaults) const;

	template <Faerie::ItemData::CFragmentImpl T>
	UE_REWRITE const T* GetDefaultFragment(const FGameplayTag ReferenceTag = Faerie::ItemData::Tags::ReferenceDefaults) const
	{
		return GetDefaultFragment(const_cast<UScriptStruct*>(T::StaticStruct()), ReferenceTag).GetPtr<const T>();
	}

#if WITH_EDITORONLY_DATA
	// Only the Editor can set the defaults for fragment data. It should always be treated as immutable during runtime.

	void SetDefaultFragment(FConstStructView DefaultStructValue);

	template <Faerie::ItemData::CFragmentImpl T>
	void SetDefaultFragment(const T& DefaultStructValue)
	{
		return SetDefaultFragment(FConstStructView::Make(DefaultStructValue));
	}
#endif

	// Does this item instance expect to mutate during runtime. This disables stacking.
	bool CanMutate() const;

protected:
	bool DetermineFragmentMutability() const;

	// Mass fragments for this item instance that are not registered to the mass subsystem. Used to hold default values
	// for instances generated in the editor.
	UPROPERTY(VisibleInstanceOnly, Category = "FaerieItem")
	TArray<FInstancedStruct /* TInstancedStruct<FFaerieMassFragment> */> FragmentDefaults;

	// In order for an item instance to be changed at runtime, it must be mutable. This disables stacking.
	UPROPERTY(VisibleInstanceOnly, Category = "FaerieItem")
	bool InstancesCanMutate = false;

	// Version number for validation of instances created from an item asset.
	// Stored as int32, but interpreted as Faerie::ItemData::EFormatVersion.
	UPROPERTY(VisibleInstanceOnly, Category = "FaerieItem")
	int32 FormatVersion = INDEX_NONE;
};

namespace Faerie::ItemData
{
	template <CFragmentImpl T>
	[[nodiscard]] UE_REWRITE const T* GetEntityFragment(const FMassEntityManager& EntityManager, const FMassEntityHandle ItemHandle)
	{
		if (EntityManager.IsEntityValid(ItemHandle))
		{
			if (const T* Fragment = EntityManager.GetFragmentDataPtr<T>(ItemHandle))
			{
				return Fragment;
			}
		}

		return nullptr;
	}

	[[nodiscard]] FAERIEITEMDATA_API FConstStructView GetEntityFragment(const FMassEntityManager& EntityManager, FMassEntityHandle ItemHandle, TNotNull<const UScriptStruct*> FragmentType);


	[[nodiscard]] FAERIEITEMDATA_API TConstStructView<FFaerieMassFragment> GetEntityFragmentOrDefault(const FMassEntityManager* EntityManager, const FFaerieItemInstance& Instance, TNotNull<const UScriptStruct*> FragmentType, FGameplayTag ReferenceTag = Tags::ReferenceDefaults);


	template <CFragmentImpl T>
	[[nodiscard]] UE_REWRITE TConstStructView<T> GetEntityFragmentOrDefault(const FMassEntityManager* EntityManager, const FFaerieItemInstance& Instance, FGameplayTag ReferenceTag = Tags::ReferenceDefaults)
	{
		TConstStructView<FFaerieMassFragment> FragmentCopy = GetEntityFragmentOrDefault(EntityManager, Instance, T::StaticStruct(), ReferenceTag);
		return *reinterpret_cast<TConstStructView<T>*>(&FragmentCopy);
	}


	[[nodiscard]] FAERIEITEMDATA_API TConstStructView<FFaerieMassFragment> GetDefaultFragment(const UFaerieItem* ItemAsset, TNotNull<const UScriptStruct*> FragmentType, FGameplayTag ReferenceTag = Tags::ReferenceDefaults);


	template <CFragmentImpl T>
	[[nodiscard]] TConstStructView<T> GetDefaultFragment(const UFaerieItem* ItemAsset, FGameplayTag ReferenceTag = Tags::ReferenceDefaults)
	{
		TConstStructView<FFaerieMassFragment> FragmentCopy = GetDefaultFragment(ItemAsset, T::StaticStruct(), ReferenceTag);
		return *reinterpret_cast<TConstStructView<T>*>(&FragmentCopy);
	}
}

/*
 * Keeps track of the last time this item was modified. Allows, for example, sorting items by recently touched.
 */
USTRUCT(meta = (Hidden))
struct FFaerieItemModificationDate : public FFaerieMassFragment
{
	GENERATED_BODY()

	FFaerieItemModificationDate() = default;
	FFaerieItemModificationDate(const FDateTime& LastModified)
	  : LastModified(LastModified) {}

	FDateTime LastModified = FDateTime();
};

/**
 * Not replicated, only the creator of the item can see these
 */
USTRUCT()
struct FFaerieMassItemPointer : public FMassFragment
{
	GENERATED_BODY()

	FFaerieMassItemPointer() = default;
	FFaerieMassItemPointer(const UFaerieItem* Item)
	  : Item(Item) {}

	TObjectKey<const UFaerieItem> Item;
};

/**
 * Not replicated, only the creator of the item can see these
 */
USTRUCT()
struct FAERIEITEMDATA_API FFaerieMassItemOwner : public FMassConstSharedFragment
{
	GENERATED_BODY()

	FFaerieMassItemOwner() = default;
	FFaerieMassItemOwner(const UObject* Owner)
	  : Owner(Owner) {}

	IFaerieItemOwnerInterface* GetInterface() const;
	FWeakObjectPtr Owner;
};