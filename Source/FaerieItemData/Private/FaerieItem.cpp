// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItem.h"
#include "FaerieItemAsset.h"
#include "FaerieItemProxy.h"
#include "MassEntityView.h"

namespace Faerie::ItemData
{
	namespace Tags
	{
		UE_DEFINE_GAMEPLAY_TAG(FragmentAdd, "Fae.Token.Add")
		UE_DEFINE_GAMEPLAY_TAG(FragmentRemove, "Fae.Token.Remove")
		UE_DEFINE_GAMEPLAY_TAG(FragmentGenericPropertyEdit, "Fae.Token.GenericPropertyEdit")
	}

	bool HasDefaultFragment(const FFaerieItemProxy& Item, const TNotNull<const UScriptStruct*> FragmentType,
		const UScriptStruct* ReferenceType)
	{
		if (Item.HasEntityManager() && Item.GetEntityManager()->IsEntityValid(Item.GetEntityHandle()))
		{
			if (auto&& ItemAssetFragment = Item.GetEntityManager()->GetConstSharedFragmentDataPtr<FFaerieItemDataDefaults>(Item.GetEntityHandle()))
			{
				if (ItemAssetFragment->ItemAsset->HasDefaultFragment(FragmentType))
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
		if (Item.HasEntityManager() && Item.GetEntityManager()->IsEntityValid(Item.GetEntityHandle()))
		{
			if (const FMassEntityView View = FMassEntityView::TryMakeView(*Item.GetEntityManager(), Item.GetEntityHandle());
				View.IsValid())
			{
				return View.HasElement(FragmentType, UE::Mass::EIncludeSparseElements::Yes);
			}
		}

		return false;
	}

	bool HasEntityFragmentOrDefault(const FFaerieItemProxy& Item, const TNotNull<const UScriptStruct*> FragmentType,
		const UScriptStruct* ReferenceType)
	{
		// Look for a live fragment of the given type.
		if (HasItemElement(Item, FragmentType))
		{
			return true;
		}

		return HasDefaultFragment(Item, FragmentType, ReferenceType);
	}

	FConstStructView GetDefaultFragment(const FMassEntityManager& EntityManager, const FMassEntityHandle Item,
		const TNotNull<const UScriptStruct*> FragmentType, const UScriptStruct* ReferenceType)
	{
		if (auto&& ItemAssetFragment = EntityManager.GetConstSharedFragmentDataPtr<FFaerieItemDataDefaults>(Item))
		{
			if (const FConstStructView Fragment = ItemAssetFragment->ItemAsset->GetDefaultFragment(FragmentType, ReferenceType);
				Fragment.IsValid())
			{
				return Fragment;
			}
		}

		return FConstStructView();
	}

	FConstStructView GetDefaultFragment(const FFaerieItemProxy& Item, const TNotNull<const UScriptStruct*> FragmentType,
		const UScriptStruct* ReferenceType)
	{
		if (Item.HasEntityManager())
		{
			return GetDefaultFragment(*Item.GetEntityManager(), Item.GetEntityHandle(), FragmentType, ReferenceType);
		}

#if WITH_EDITOR
		if (const UFaerieItemAsset* ItemAsset = Item.GetItemPtr())
		{
			return ItemAsset->GetDefaultFragment(FragmentType, ReferenceType);
		}
#endif

		return FConstStructView();
	}

	FConstStructView GetEntityFragment(const FMassEntityManager& EntityManager, const FMassEntityHandle ItemHandle,
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

	FConstStructView GetEntityFragment(const FFaerieItemProxy& Item, const TNotNull<const UScriptStruct*> FragmentType)
	{
		if (Item.HasEntityManager())
		{
			return GetEntityFragment(*Item.GetEntityManager(), Item.GetEntityHandle(), FragmentType);
		}
		return FConstStructView();
	}

	FConstStructView GetEntityFragmentOrDefault(const FMassEntityManager& EntityManager, const FMassEntityHandle Item, const TNotNull<const UScriptStruct*> FragmentType, const UScriptStruct* ReferenceType)
	{
		if (EntityManager.IsEntityValid(Item))
		{
			// Look for a live fragment of the given type.
			if (const FConstStructView View = GetEntityFragment(EntityManager, Item, FragmentType);
				View.IsValid())
			{
				return View;
			}
		}

		return GetDefaultFragment(EntityManager, Item, FragmentType, ReferenceType);
	}

	FConstStructView GetEntityFragmentOrDefault(const FFaerieItemProxy& Item, const TNotNull<const UScriptStruct*> FragmentType, const UScriptStruct* ReferenceType)
	{
		if (Item.HasEntityManager())
		{
			if (const FConstStructView View = GetEntityFragment(*Item.GetEntityManager(), Item.GetEntityHandle(), FragmentType);
				View.IsValid())
			{
				return View;
			}
		}
		return GetDefaultFragment(Item, FragmentType, ReferenceType);
	}
}
