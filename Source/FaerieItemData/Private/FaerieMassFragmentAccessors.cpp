// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieMassFragmentAccessors.h"
#include "FaerieItemAsset.h"
#include "FaerieItemProxy.h"
#include "MassEntityView.h"

namespace Faerie::ItemData
{
	bool HasDefaultFragment(const FFaerieItemProxy& Item, const TNotNull<const UScriptStruct*> FragmentType,
		const TSubScriptStructOf<FFaerieMassItemObject>& ReferenceType)
	{
		if (Item.HasEntityManager() && Item.GetEntityManager()->IsEntityValid(Item.GetEntityHandle()))
		{
			if (FConstStructView&& ItemAssetFragment = Item.GetEntityManager()->GetConstSharedFragmentDataStruct(
					Item.GetEntityHandle(), ReferenceType);
				ItemAssetFragment.IsValid())
			{
				if (ItemAssetFragment.Get<FFaerieMassItemObject>().ItemAsset->HasDefaultFragment(FragmentType))
				{
					return true;
				}
			}
		}

#if WITH_EDITOR
		if (const UFaerieItemAsset* ItemAsset = Item.GetItemPtr())
		{
			return ItemAsset->HasDefaultFragment(FragmentType, ReferenceType);
		}
#endif

		return false;
	}

	bool HasItemElement(const FFaerieItemProxy& Item, const TNotNull<const UScriptStruct*> FragmentType)
	{
		if (Item.HasEntityManager())
		{
			if (const FMassEntityView View = FMassEntityView::TryMakeView(*Item.GetEntityManager(), Item.GetEntityHandle());
				View.IsValid())
			{
				return View.HasElement(FragmentType, UE::Mass::EIncludeSparseElements::Yes);
			}
		}

		return false;
	}

	bool HasItemFragmentOrDefault(const FFaerieItemProxy& Item, const TNotNull<const UScriptStruct*> FragmentType,
		const TSubScriptStructOf<FFaerieMassItemObject>& ReferenceType)
	{
		// Look for a live fragment of the given type.
		if (HasItemElement(Item, FragmentType))
		{
			return true;
		}

		return HasDefaultFragment(Item, FragmentType, ReferenceType);
	}

	FConstStructView GetDefaultFragment(const FMassEntityManager& EntityManager, const FMassEntityHandle Item,
		const TNotNull<const UScriptStruct*> FragmentType, const TSubScriptStructOf<FFaerieMassItemObject>& ReferenceType)
	{
		if (EntityManager.IsEntityValid(Item))
		{
			if (auto&& ItemAssetFragment = EntityManager.GetConstSharedFragmentDataStruct(Item, ReferenceType);
				ItemAssetFragment.IsValid())
			{
				if (const FConstStructView Fragment = ItemAssetFragment.Get<FFaerieMassItemObject>().ItemAsset->GetDefaultFragment(
					FragmentType, ReferenceType);
					Fragment.IsValid())
				{
					return Fragment;
				}
			}
		}

		return FConstStructView();
	}

	FConstStructView GetDefaultFragment(const FFaerieItemProxy& Item, const TNotNull<const UScriptStruct*> FragmentType,
		const TSubScriptStructOf<FFaerieMassItemObject>& ReferenceType)
	{
		if (Item.HasEntityManager())
		{
			if (const FConstStructView Fragment = GetDefaultFragment(*Item.GetEntityManager(), Item.GetEntityHandle(),
					FragmentType, ReferenceType);
				Fragment.IsValid())
			{
				return Fragment;
			}
		}

#if WITH_EDITOR
		if (const UFaerieItemAsset* ItemAsset = Item.GetItemPtr())
		{
			return ItemAsset->GetDefaultFragment(FragmentType, ReferenceType);
		}
#endif

		return FConstStructView();
	}

	FConstStructView GetItemFragment(const FMassEntityManager& EntityManager, const FMassEntityHandle ItemHandle,
		const TNotNull<const UScriptStruct*> FragmentType)
	{
		if (EntityManager.IsEntityValid(ItemHandle))
		{
			if (FragmentType->IsChildOf<FFaerieMassFragment>() || FragmentType->IsChildOf<FFaerieMassSparseFragment>())
			{
				return EntityManager.GetFragmentDataStruct(ItemHandle, FragmentType);
			}
			if (FragmentType->IsChildOf<FFaerieMassSharedFragment>())
			{
				return EntityManager.GetSharedFragmentDataStruct(ItemHandle, FragmentType);
			}
			if (FragmentType->IsChildOf<FFaerieMassConstSharedFragment>())
			{
				return EntityManager.GetConstSharedFragmentDataStruct(ItemHandle, FragmentType);
			}
		}

		return FConstStructView();
	}

	FConstStructView GetItemFragment(const FFaerieItemProxy& Item, const TNotNull<const UScriptStruct*> FragmentType)
	{
		if (Item.HasEntityManager())
		{
			return GetItemFragment(*Item.GetEntityManager(), Item.GetEntityHandle(), FragmentType);
		}
		return FConstStructView();
	}

	FConstStructView GetItemFragmentOrDefault(const FMassEntityManager& EntityManager, const FMassEntityHandle Item, const TNotNull<const UScriptStruct*> FragmentType, const TSubScriptStructOf<FFaerieMassItemObject>& ReferenceType)
	{
		if (EntityManager.IsEntityValid(Item))
		{
			// Look for a live fragment of the given type.
			if (const FConstStructView View = GetItemFragment(EntityManager, Item, FragmentType);
				View.IsValid())
			{
				return View;
			}
		}

		return GetDefaultFragment(EntityManager, Item, FragmentType, ReferenceType);
	}

	FConstStructView GetItemFragmentOrDefault(const FFaerieItemProxy& Item, const TNotNull<const UScriptStruct*> FragmentType, const TSubScriptStructOf<FFaerieMassItemObject>& ReferenceType)
	{
		if (Item.HasEntityManager())
		{
			if (const FConstStructView View = GetItemFragment(*Item.GetEntityManager(), Item.GetEntityHandle(), FragmentType);
				View.IsValid())
			{
				return View;
			}
		}
		return GetDefaultFragment(Item, FragmentType, ReferenceType);
	}
}
