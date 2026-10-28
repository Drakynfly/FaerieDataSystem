// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataConcepts.h"
#include "FaerieItemDataDefines.h"
#include "FaerieItemDataFwd.h"
#include "FaerieItemDataEnums.h"
#include "FaerieItemProxy.h"
#include "FaerieMassFragment.h"

#include "MassEntityManager.h"
#include "Mass/EntityHandle.h"

#include "NativeGameplayTags.h"

#include "StructUtils/StructView.h"

#include "FaerieItem.generated.h"

#define FAE_API FAERIEITEMDATA_API

namespace Faerie::ItemData
{
	namespace Tags
	{
		FAE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(FragmentAdd)
		FAE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(FragmentRemove)
		FAE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(FragmentGenericPropertyEdit)
	}
}

/*
 * Const Shared fragment to keep a reference to a faerie item object.
 */
USTRUCT()
struct FFaerieMassItemObject : public FFaerieMassConstSharedFragment
{
	GENERATED_BODY()

	FFaerieMassItemObject() = default;
	FFaerieMassItemObject(const UFaerieItem* Item) : ItemAsset(Item) {}

	UPROPERTY(VisibleAnywhere, Category = "MassItemObject")
	TObjectPtr<const UFaerieItem> ItemAsset;

____FAERIE_FRAGMENT_DECL(FFaerieMassItemObject)
};

// Item Asset used for "default" values.
USTRUCT()
struct FFaerieItemDataDefaults : public FFaerieMassItemObject
{
	GENERATED_BODY()
	using FFaerieMassItemObject::FFaerieMassItemObject;
____FAERIE_FRAGMENT_DECL(FFaerieItemDataDefaults)
};


/**
 * A runtime instance of an item.
 */
UCLASS(DefaultToInstanced, EditInlineNew, BlueprintType)
class FAE_API UFaerieItem : public UObject
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
	static TNotNull<UFaerieItem*> CreateNewInstance(TConstArrayView<FInstancedStruct> Fragments, TNotNull<UObject*> InstanceOuter, EFaerieItemInstancingMutability Mutability = EFaerieItemInstancingMutability::Automatic);

	// Creates a new faerie item object using this instance as a template. Duplicates are instance-mutable by default.
	TNotNull<const UFaerieItem*> CreateDuplicate(TNotNull<UObject*> InstanceOuter, EFaerieItemInstancingMutability Mutability = EFaerieItemInstancingMutability::Automatic) const;
#endif

	UE_REWRITE TConstArrayView<FInstancedStruct> GetFragmentDefaults() const { return FragmentDefaults; }

	UE_REWRITE int32 GetAssetFormatVersion() const { return FormatVersion; }

	bool HasDefaultFragment(TNotNull<const UScriptStruct*> StructType, const UScriptStruct* ReferenceType = FFaerieItemDataDefaults::StaticStruct()) const;

	template <Faerie::ItemData::CElementImpl T>
	UE_REWRITE bool HasDefaultFragment(const UScriptStruct* ReferenceType = FFaerieItemDataDefaults::StaticStruct()) const
	{
		return HasDefaultFragment(const_cast<UScriptStruct*>(T::StaticStruct()), ReferenceType);
	}

	// Look for a default fragment value in an item asset.
	FConstStructView GetDefaultFragment(TNotNull<const UScriptStruct*> StructType, const UScriptStruct* ReferenceType = FFaerieItemDataDefaults::StaticStruct()) const;

	template <Faerie::ItemData::CDataImpl T>
	UE_REWRITE TConstStructView<T> GetDefaultFragment(const UScriptStruct* ReferenceType = FFaerieItemDataDefaults::StaticStruct()) const
	{
		FConstStructView FragmentView = GetDefaultFragment(const_cast<UScriptStruct*>(T::StaticStruct()), ReferenceType);
		return *reinterpret_cast<TConstStructView<T>*>(&FragmentView);
	}

#if WITH_EDITORONLY_DATA
	// Only the Editor can set the defaults for fragment data. It should always be treated as immutable during runtime.

	void SetDefaultFragment(FConstStructView DefaultStructValue);

	template <Faerie::ItemData::CDataImpl T>
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
	//~							~//
	//~		FRAGMENT CHECKS		~//
	//~							~//

	[[nodiscard]] FAE_API bool HasDefaultFragment(const FFaerieItemProxy& Item, TNotNull<const UScriptStruct*> FragmentType, const UScriptStruct* ReferenceType = FFaerieItemDataDefaults::StaticStruct());

	template <CElementImpl T>
	[[nodiscard]] bool HasDefaultFragment(const FFaerieItemProxy& Item, const UScriptStruct* ReferenceType = FFaerieItemDataDefaults::StaticStruct())
	{
		return HasDefaultFragment(Item, T::StaticStruct(), ReferenceType);
	}

	[[nodiscard]] FAE_API bool HasItemElement(const FFaerieItemProxy& Item, TNotNull<const UScriptStruct*> FragmentType);

	template <CElementImpl T>
	[[nodiscard]] UE_REWRITE bool HasItemElement(const FFaerieItemProxy& Item)
	{
		return HasItemElement(Item, T::StaticStruct());
	}

	[[nodiscard]] FAE_API bool HasEntityFragmentOrDefault(const FFaerieItemProxy& Item, TNotNull<const UScriptStruct*> FragmentType, const UScriptStruct* ReferenceType = FFaerieItemDataDefaults::StaticStruct());

	template <CElementImpl T>
	[[nodiscard]] UE_REWRITE bool HasEntityFragmentOrDefault(const FFaerieItemProxy& Item, const UScriptStruct* ReferenceType = FFaerieItemDataDefaults::StaticStruct())
	{
		return HasEntityFragmentOrDefault(Item, T::StaticStruct(), ReferenceType);
	}

	//~							~//
	//~		FRAGMENT ACCESSORS	~//
	//~							~//

	[[nodiscard]] FAE_API FConstStructView GetDefaultFragment(const FMassEntityManager& EntityManager, FMassEntityHandle Item, TNotNull<const UScriptStruct*> FragmentType, const UScriptStruct* ReferenceType = FFaerieItemDataDefaults::StaticStruct());
	[[nodiscard]] FAE_API FConstStructView GetDefaultFragment(const FFaerieItemProxy& Item, TNotNull<const UScriptStruct*> FragmentType, const UScriptStruct* ReferenceType = FFaerieItemDataDefaults::StaticStruct());

	template <CDataImpl T>
	[[nodiscard]] TConstStructView<T> GetDefaultFragment(const FMassEntityManager& EntityManager, FMassEntityHandle Item, const UScriptStruct* ReferenceType = FFaerieItemDataDefaults::StaticStruct())
	{
		FConstStructView FragmentView = GetDefaultFragment(EntityManager, Item, T::StaticStruct(), ReferenceType);
		return *reinterpret_cast<TConstStructView<T>*>(&FragmentView);
	}

	template <CDataImpl T>
	[[nodiscard]] TConstStructView<T> GetDefaultFragment(const FFaerieItemProxy& Item, const UScriptStruct* ReferenceType = FFaerieItemDataDefaults::StaticStruct())
	{
		FConstStructView FragmentView = GetDefaultFragment(Item, T::StaticStruct(), ReferenceType);
		return *reinterpret_cast<TConstStructView<T>*>(&FragmentView);
	}

	[[nodiscard]] FAE_API FConstStructView GetEntityFragment(const FMassEntityManager& EntityManager, FMassEntityHandle ItemHandle, TNotNull<const UScriptStruct*> FragmentType);
	[[nodiscard]] FAE_API FConstStructView GetEntityFragment(const FFaerieItemProxy& Item, TNotNull<const UScriptStruct*> FragmentType);

	template <CDataImpl T>
	[[nodiscard]] FAE_API UE_REWRITE const T* GetEntityFragment(const FMassEntityManager& EntityManager, const FMassEntityHandle ItemHandle);

	template <CDataImpl T>
	[[nodiscard]] FAE_API UE_REWRITE const T* GetEntityFragment(const FFaerieItemProxy& Item);

	[[nodiscard]] FAE_API FConstStructView GetEntityFragmentOrDefault(const FMassEntityManager& EntityManager, FMassEntityHandle Item, TNotNull<const UScriptStruct*> FragmentType, const UScriptStruct* ReferenceType = FFaerieItemDataDefaults::StaticStruct());
	[[nodiscard]] FAE_API FConstStructView GetEntityFragmentOrDefault(const FFaerieItemProxy& Item, TNotNull<const UScriptStruct*> FragmentType, const UScriptStruct* ReferenceType = FFaerieItemDataDefaults::StaticStruct());

	template <CDataImpl T>
	[[nodiscard]] UE_REWRITE TConstStructView<T> GetItemDataOrDefault(const FMassEntityManager& EntityManager, const FMassEntityHandle Item, const UScriptStruct* ReferenceType = FFaerieItemDataDefaults::StaticStruct())
	{
		if (const T* LiveFragment = GetEntityFragment<T>(EntityManager, Item))
		{
			return TConstStructView<T>(*LiveFragment);
		}

		return GetDefaultFragment<T>(EntityManager, Item, ReferenceType);
	}

	template <CDataImpl T>
	[[nodiscard]] UE_REWRITE TConstStructView<T> GetItemDataOrDefault(const FFaerieItemProxy& Item, const UScriptStruct* ReferenceType = FFaerieItemDataDefaults::StaticStruct())
	{
		if (const T* LiveFragment = GetEntityFragment<T>(Item))
		{
			return TConstStructView<T>(*LiveFragment);
		}

		return GetDefaultFragment<T>(Item, ReferenceType);
	}

	/// TEMPLATE DEFINITIONS

	template <CDataImpl T> const T* GetEntityFragment(const FMassEntityManager& EntityManager,
		const FMassEntityHandle ItemHandle)
	{
		if (EntityManager.IsEntityValid(ItemHandle))
		{
			if constexpr (CFragmentImpl<T>)
			{
				if (const T* Fragment = EntityManager.GetFragmentDataPtr<T>(ItemHandle))
				{
					return Fragment;
				}
			}
			else if constexpr (CSharedImpl<T>)
			{
				if (const T* Fragment = EntityManager.GetSharedFragmentDataPtr<T>(ItemHandle))
				{
					return Fragment;
				}
			}
			else if constexpr (CConstSharedImpl<T>)
			{
				if (const T* Fragment = EntityManager.GetConstSharedFragmentDataPtr<T>(ItemHandle))
				{
					return Fragment;
				}
			}
		}

		return nullptr;
	}

	template <CDataImpl T> const T* GetEntityFragment(const FFaerieItemProxy& Item)
	{
		if (Item.HasEntityManager() && Item.GetEntityManager()->IsEntityValid(Item.GetEntityHandle()))
		{
			if constexpr (CFragmentImpl<T>)
			{
				if (const T* Fragment = Item.GetEntityManager()->GetFragmentDataPtr<T>(Item.GetEntityHandle()))
				{
					return Fragment;
				}
			}
			else if constexpr (CSharedImpl<T>)
			{
				if (const T* Fragment = Item.GetEntityManager()->GetSharedFragmentDataPtr<T>(Item.GetEntityHandle()))
				{
					return Fragment;
				}
			}
			else if constexpr (CConstSharedImpl<T>)
			{
				if (const T* Fragment = Item.GetEntityManager()->GetConstSharedFragmentDataPtr<T>(Item.GetEntityHandle()))
				{
					return Fragment;
				}
			}
		}

		return nullptr;
	}
}

#undef FAE_API