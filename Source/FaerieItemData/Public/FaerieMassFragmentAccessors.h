// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataConcepts.h"
#include "FaerieItemProxy.h"

#include "Fragments/FaerieItemAssetFragment.h"

#include "MassEntityManager.h"
#include "Mass/EntityHandle.h"

#include "StructUtils/StructView.h"
#include "Templates/SubScriptStructOf.h"

#define FAE_API FAERIEITEMDATA_API

/*
 * These functions all read from item data, either in the form of Proxies, or Entity handles, and can either return live
 * fragments from mass (during runtime), or default fragments from Item Assets.
 * Usage is as follows:
 * - Has/GetDefaultXXX functions only return fragments from Item Assets, never live fragments.
 * - Has/GetItemXXX functions only return fragments from live entities, never from Item Assets.
 * - Has/GetItemXXXOrDefault functions return a live fragment if present, and fallback to Item Asset fragments otherwise.
 * - The overloads that accept FaerieItemProxies can access Item Assets when called from the editor, while the Entity
 *   overloads cannot.
 * - By default, all functions that could read from Item Assets use FFaerieItemDataDefaults as the type of Item Asset to
 *   search. This can be changed to another subtype of FFaerieMassItemObject if one is made.
 */
namespace Faerie::ItemData
{
	//~							~//
	//~		FRAGMENT CHECKS		~//
	//~							~//

	[[nodiscard]] FAE_API bool HasDefaultFragment(const FFaerieItemProxy& Item, TNotNull<const UScriptStruct*> FragmentType, const TSubScriptStructOf<FFaerieMassItemObject>& ReferenceType = FFaerieItemDataDefaults::StaticStruct());

	template <CElementImpl T>
	[[nodiscard]] bool HasDefaultFragment(const FFaerieItemProxy& Item, const TSubScriptStructOf<FFaerieMassItemObject>& ReferenceType = FFaerieItemDataDefaults::StaticStruct())
	{
		return HasDefaultFragment(Item, T::StaticStruct(), ReferenceType);
	}

	[[nodiscard]] FAE_API bool HasItemElement(const FFaerieItemProxy& Item, TNotNull<const UScriptStruct*> FragmentType);

	template <CElementImpl T>
	[[nodiscard]] UE_REWRITE bool HasItemElement(const FFaerieItemProxy& Item)
	{
		return HasItemElement(Item, T::StaticStruct());
	}

	[[nodiscard]] FAE_API bool HasItemFragmentOrDefault(const FFaerieItemProxy& Item, TNotNull<const UScriptStruct*> FragmentType, const TSubScriptStructOf<FFaerieMassItemObject>& ReferenceType = FFaerieItemDataDefaults::StaticStruct());

	template <CElementImpl T>
	[[nodiscard]] UE_REWRITE bool HasItemFragmentOrDefault(const FFaerieItemProxy& Item, const TSubScriptStructOf<FFaerieMassItemObject>& ReferenceType = FFaerieItemDataDefaults::StaticStruct())
	{
		return HasItemFragmentOrDefault(Item, T::StaticStruct(), ReferenceType);
	}

	//~							~//
	//~		FRAGMENT ACCESSORS	~//
	//~							~//

	[[nodiscard]] FAE_API FConstStructView GetDefaultFragment(const FMassEntityManager& EntityManager, FMassEntityHandle Item, TNotNull<const UScriptStruct*> FragmentType, const TSubScriptStructOf<FFaerieMassItemObject>& ReferenceType = FFaerieItemDataDefaults::StaticStruct());
	[[nodiscard]] FAE_API FConstStructView GetDefaultFragment(const FFaerieItemProxy& Item, TNotNull<const UScriptStruct*> FragmentType, const TSubScriptStructOf<FFaerieMassItemObject>& ReferenceType = FFaerieItemDataDefaults::StaticStruct());

	template <CDataImpl T>
	[[nodiscard]] TConstStructView<T> GetDefaultFragment(const FMassEntityManager& EntityManager, FMassEntityHandle Item, const TSubScriptStructOf<FFaerieMassItemObject>& ReferenceType = FFaerieItemDataDefaults::StaticStruct())
	{
		FConstStructView FragmentView = GetDefaultFragment(EntityManager, Item, T::StaticStruct(), ReferenceType);
		return *reinterpret_cast<TConstStructView<T>*>(&FragmentView);
	}

	template <CDataImpl T>
	[[nodiscard]] TConstStructView<T> GetDefaultFragment(const FFaerieItemProxy& Item, const TSubScriptStructOf<FFaerieMassItemObject>& ReferenceType = FFaerieItemDataDefaults::StaticStruct())
	{
		FConstStructView FragmentView = GetDefaultFragment(Item, T::StaticStruct(), ReferenceType);
		return *reinterpret_cast<TConstStructView<T>*>(&FragmentView);
	}

	[[nodiscard]] FAE_API FConstStructView GetItemFragment(const FMassEntityManager& EntityManager, FMassEntityHandle ItemHandle, TNotNull<const UScriptStruct*> FragmentType);
	[[nodiscard]] FAE_API FConstStructView GetItemFragment(const FFaerieItemProxy& Item, TNotNull<const UScriptStruct*> FragmentType);

	template <CDataImpl T>
	[[nodiscard]] FAE_API UE_REWRITE const T* GetItemFragment(const FMassEntityManager& EntityManager, const FMassEntityHandle ItemHandle);

	template <CDataImpl T>
	[[nodiscard]] FAE_API UE_REWRITE const T* GetItemFragment(const FFaerieItemProxy& Item);

	[[nodiscard]] FAE_API FConstStructView GetItemFragmentOrDefault(const FMassEntityManager& EntityManager, FMassEntityHandle Item, TNotNull<const UScriptStruct*> FragmentType, const TSubScriptStructOf<FFaerieMassItemObject>& ReferenceType = FFaerieItemDataDefaults::StaticStruct());
	[[nodiscard]] FAE_API FConstStructView GetItemFragmentOrDefault(const FFaerieItemProxy& Item, TNotNull<const UScriptStruct*> FragmentType, const TSubScriptStructOf<FFaerieMassItemObject>& ReferenceType = FFaerieItemDataDefaults::StaticStruct());

	template <CDataImpl T>
	[[nodiscard]] UE_REWRITE TConstStructView<T> GetItemDataOrDefault(const FMassEntityManager& EntityManager, const FMassEntityHandle Item, const TSubScriptStructOf<FFaerieMassItemObject>& ReferenceType = FFaerieItemDataDefaults::StaticStruct())
	{
		if (const T* LiveFragment = GetItemFragment<T>(EntityManager, Item))
		{
			return TConstStructView<T>(*LiveFragment);
		}

		return GetDefaultFragment<T>(EntityManager, Item, ReferenceType);
	}

	template <CDataImpl T>
	[[nodiscard]] UE_REWRITE TConstStructView<T> GetItemDataOrDefault(const FFaerieItemProxy& Item, const TSubScriptStructOf<FFaerieMassItemObject>& ReferenceType = FFaerieItemDataDefaults::StaticStruct())
	{
		if (const T* LiveFragment = GetItemFragment<T>(Item))
		{
			return TConstStructView<T>(*LiveFragment);
		}

		return GetDefaultFragment<T>(Item, ReferenceType);
	}

	/// TEMPLATE DEFINITIONS

	template <CDataImpl T> const T* GetItemFragment(const FMassEntityManager& EntityManager, const FMassEntityHandle ItemHandle)
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

	template <CDataImpl T> const T* GetItemFragment(const FFaerieItemProxy& Item)
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