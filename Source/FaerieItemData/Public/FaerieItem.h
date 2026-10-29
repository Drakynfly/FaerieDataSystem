// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataConcepts.h"
#include "FaerieItemDataDefines.h"
#include "FaerieItemDataFwd.h"
#include "FaerieItemProxy.h"
#include "Fragments/FaerieItemAssetFragment.h"

#include "MassEntityManager.h"
#include "Mass/EntityHandle.h"

#include "NativeGameplayTags.h"
#include "StructUtils/StructView.h"

#define FAE_API FAERIEITEMDATA_API

namespace Faerie::ItemData
{
	namespace Tags
	{
		FAE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(FragmentAdd)
		FAE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(FragmentRemove)
		FAE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(FragmentGenericPropertyEdit)
	}

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

	template <CDataImpl T> const T* GetEntityFragment(const FMassEntityManager& EntityManager, const FMassEntityHandle ItemHandle)
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