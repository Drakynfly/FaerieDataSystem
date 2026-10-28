// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItem.h"
#include "FaerieItemAsset.h"
#include "FaerieItemProxy.h"
#include "MassEntityView.h"

#include "Fragments/FaerieReferenceFragment.h"
#include "UObject/ObjectSaveContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItem)

namespace Faerie::ItemData::Tags
{
	UE_DEFINE_GAMEPLAY_TAG(FragmentAdd, "Fae.Token.Add")
	UE_DEFINE_GAMEPLAY_TAG(FragmentRemove, "Fae.Token.Remove")
	UE_DEFINE_GAMEPLAY_TAG(FragmentGenericPropertyEdit, "Fae.Token.GenericPropertyEdit")
}

using namespace Faerie;

void UFaerieItem::PostInitProperties()
{
	Super::PostInitProperties();

#if WITH_EDITOR
	const bool DeterminedMutability = DetermineFragmentMutability();
	if (DeterminedMutability != InstancesCanMutate)
	{
		InstancesCanMutate = DeterminedMutability;
		(void)MarkPackageDirty();
	}
#endif
}

void UFaerieItem::PreSave(FObjectPreSaveContext SaveContext)
{
#if WITH_EDITOR
	InstancesCanMutate = DetermineFragmentMutability();
#endif

	Super::PreSave(SaveContext);
}

void UFaerieItem::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	// Items loaded from disk in shipping builds don't need to re-cache this.
	const bool DeterminedMutability = DetermineFragmentMutability();
	if (DeterminedMutability != InstancesCanMutate)
	{
		InstancesCanMutate = DeterminedMutability;
		(void)MarkPackageDirty();
	}
#endif
}

#if WITH_EDITOR

#define LOCTEXT_NAMESPACE "FaerieItemAssetValidation"

EDataValidationResult UFaerieItem::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	for (const FInstancedStruct& Fragment : FragmentDefaults)
	{
		if (const ItemData::FMassTypeTraitsInterface* Traits = ItemData::GetMassTraitsInterface(Fragment.GetScriptStruct()))
		{
            Result = CombineDataValidationResults(Result, Traits->IsDataValid(Fragment.GetMemory(), Context));
		}
	}

	return Result;
}

#undef LOCTEXT_NAMESPACE

#endif

#if WITH_EDITOR
TNotNull<UFaerieItem*> UFaerieItem::CreateNewInstance(const TConstArrayView<FInstancedStruct> Fragments,
													const TNotNull<UObject*> InstanceOuter,
													const EFaerieItemInstancingMutability Mutability)
{
	const TNotNull<UFaerieItem*> Instance = NewObject<UFaerieItem>(InstanceOuter);

	// Add the fragments to the new object.
	Instance->FragmentDefaults.Reserve(Fragments.Num());
	for (auto&& Fragment : Fragments)
	{
		if (!Fragment.IsValid()) continue;
		if (!ItemData::IsFaerieMassType(Fragment.GetScriptStruct())) continue;
		Instance->FragmentDefaults.Add(Fragment);
	}

	switch (Mutability)
	{
	case EFaerieItemInstancingMutability::Automatic:
		Instance->InstancesCanMutate = Instance->DetermineFragmentMutability();
		break;
	case EFaerieItemInstancingMutability::Mutable:
		Instance->InstancesCanMutate = true;
		break;
	case EFaerieItemInstancingMutability::Immutable:
		Instance->InstancesCanMutate = false;
		break;
	}

	// New items always have most recent format.
	Instance->FormatVersion = static_cast<int32>(ItemData::EFormatVersion::LatestVersion);

	return Instance;
}

TNotNull<const UFaerieItem*> UFaerieItem::CreateDuplicate(const TNotNull<UObject*> InstanceOuter,
												  const EFaerieItemInstancingMutability Mutability) const
{
	const TNotNull<UFaerieItem*> Instance = NewObject<UFaerieItem>(InstanceOuter);

	switch (Mutability)
	{
	case EFaerieItemInstancingMutability::Automatic:
		Instance->InstancesCanMutate = InstancesCanMutate;
		break;
	case EFaerieItemInstancingMutability::Mutable:
		Instance->InstancesCanMutate = true;
		break;
	case EFaerieItemInstancingMutability::Immutable:
		Instance->InstancesCanMutate = false;
		break;
	}

	// Duplicated items have the format of this item.
	Instance->FormatVersion = FormatVersion;

	return Instance;
}
#endif

bool UFaerieItem::HasDefaultFragment(const TNotNull<const UScriptStruct*> StructType, const UScriptStruct* ReferenceType) const
{
	for (auto&& DefaultFragment : FragmentDefaults)
	{
		if (DefaultFragment.GetScriptStruct()->IsChildOf(StructType))
		{
			return true;
		}
	}

	if (ReferenceType)
	{
		// Fallback to retrieving a default value via reference.
		// Iterate in reverse for now (since static references are usually placed at the end.
		// @todo control with a cvar or replace with order sorted fragments...
		for (int32 i = FragmentDefaults.Num() - 1; i >= 0; --i)
		{
			if (FragmentDefaults[i].GetScriptStruct() == ReferenceType)
			{
				if (const FFaerieMassItemObject* ReferenceFragment = FragmentDefaults[i].GetPtr<FFaerieMassItemObject>())
				{
					// Recurse our search into the referenced asset.
					if (ReferenceFragment->ItemAsset->HasDefaultFragment(StructType, ReferenceType))
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

FConstStructView UFaerieItem::GetDefaultFragment(const TNotNull<const UScriptStruct*> StructType, const UScriptStruct* ReferenceType) const
{
	for (auto&& DefaultFragment : FragmentDefaults)
	{
		if (DefaultFragment.GetScriptStruct()->IsChildOf(StructType))
		{
			return DefaultFragment;
		}
	}

	// Fallback to retrieving a default value via reference.
	// Iterate in reverse for now (since static references are usually placed at the end.
	// @todo control with a cvar or replace with order sorted fragments...
	for (int32 i = FragmentDefaults.Num() - 1; i >= 0; --i)
	{
		if (FragmentDefaults[i].GetScriptStruct() == ReferenceType)
		{
			if (const FFaerieMassItemObject* ReferenceFragment = FragmentDefaults[i].GetPtr<FFaerieMassItemObject>())
			{
				// Recurse our search into the referenced asset.
				return ReferenceFragment->ItemAsset->GetDefaultFragment(StructType, ReferenceType);
			}
		}
	}

	return FConstStructView();
}

#if WITH_EDITORONLY_DATA
void UFaerieItem::SetDefaultFragment(const FConstStructView DefaultStructValue)
{
	for (auto&& DefaultFragment : FragmentDefaults)
	{
		if (DefaultFragment.GetScriptStruct() == DefaultStructValue.GetScriptStruct())
		{
			DefaultFragment = DefaultStructValue;
			return;
		}
	}

	FragmentDefaults.Add(FInstancedStruct(DefaultStructValue));
}
#endif

bool UFaerieItem::CanMutate() const
{
	return InstancesCanMutate;
}

bool UFaerieItem::DetermineFragmentMutability() const
{
	for (auto&& Fragment : FragmentDefaults)
	{
		if (!Fragment.IsValid()) continue;

		// Some fragments force mutability on.
		if (const ItemData::FMassTypeTraitsInterface* Traits = ItemData::GetMassTraitsInterface(Fragment.GetScriptStruct()))
		{
			if (Traits->RequiresMutable)
			{
				return true;
			}
		}
	}

	// No fragment needs mutability, so without a flag to say otherwise, assume mutability as false.
	return false;
}

namespace Faerie::ItemData
{
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
		if (const UFaerieItem* ItemAsset = Item.GetItemPtr())
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
		return GetEntityFragmentOrDefault(*Item.GetEntityManager(), Item.GetEntityHandle(), FragmentType, ReferenceType);
	}
}
