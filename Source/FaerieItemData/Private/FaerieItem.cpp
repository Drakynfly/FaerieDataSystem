// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItem.h"
#include "AssetLoadFlagFixer.h"
#include "EntityManagerHelpers.h"
#include "FaerieItemAsset.h"
#include "FaerieItemDataLog.h"
#include "FaerieItemOwnerInterface.h"
#include "Fragments/FaerieReferenceFragment.h"
#include "Templates/SubScriptStructOf.h"
#include "UObject/ObjectSaveContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItem)

namespace Faerie::ItemData::Tags
{
	UE_DEFINE_GAMEPLAY_TAG(FragmentAdd, "Fae.Token.Add")
	UE_DEFINE_GAMEPLAY_TAG(FragmentRemove, "Fae.Token.Remove")
	UE_DEFINE_GAMEPLAY_TAG(FragmentGenericPropertyEdit, "Fae.Token.GenericPropertyEdit")

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(PrimaryIdentifier, "Fae.Token.PrimaryIdentifier", "DEPRECATED");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(ReferenceDefaults, "Fae.Reference.Defaults", "The default slot for tagged reference assets. Used by default unless overridden.")
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
		if (const ItemData::FMassFragmentTypeInterface* Traits = ItemData::GetFragmentTraitsInterface(Fragment.GetScriptStruct()))
		{
            Result = CombineDataValidationResults(Result, Traits->IsDataValid(Fragment.GetMemory(), Context));
		}
	}

	return Result;
}

#undef LOCTEXT_NAMESPACE

#endif

#if WITH_EDITOR
TNotNull<const UFaerieItem*> UFaerieItem::CreateNewInstance(const TConstArrayView<FInstancedStruct> Fragments,
													const TNotNull<UObject*> InstanceOuter,
													const EFaerieItemInstancingMutability Mutability)
{
	const TNotNull<UFaerieItem*> Instance = NewObject<UFaerieItem>(InstanceOuter);

	// Add the fragments to the new object.
	Instance->FragmentDefaults.Reserve(Fragments.Num());
	for (auto&& Fragment : Fragments)
	{
		if (!Fragment.IsValid()) continue;
		if (!Fragment.GetScriptStruct()->IsChildOf<FFaerieMassFragment>()) continue;
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

TNotNull<const UFaerieItem*> UFaerieItem::CreateDuplicate(const TNotNull<UObject*> WorldContextObject,
												  const EFaerieItemInstancingMutability Mutability) const
{
	const TNotNull<UFaerieItem*> Instance = NewObject<UFaerieItem>(WorldContextObject);

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

TConstStructView<FFaerieMassFragment> UFaerieItem::GetDefaultFragment(const TNotNull<const UScriptStruct*> StructType, const FGameplayTag ReferenceTag) const
{
	for (auto&& DefaultFragment : FragmentDefaults)
	{
		if (DefaultFragment.GetScriptStruct()->IsChildOf(StructType))
		{
			return *reinterpret_cast<const TInstancedStruct<FFaerieMassFragment>*>(&DefaultFragment);
		}
	}

	// Fallback to retrieving a default value via reference.
	// Iterate in reverse for now (since static references are usually placed at the end.
	// @todo control with a cvar or replace with order sorted fragments...
	for (int32 i = FragmentDefaults.Num() - 1; i >= 0; --i)
	{
		if (FragmentDefaults[i].GetScriptStruct() == FFaerieReferenceFragment::StaticStruct())
		{
			auto&& ReferenceFragment = FragmentDefaults[i].Get<FFaerieReferenceFragment>();
			if (auto&& Reference = ReferenceFragment.GetReferencedItem(ReferenceTag, false))
			{
				// Recurse our search into the referenced asset.
				return Reference->GetDefaultFragment(StructType, ReferenceTag);
			}
		}
	}

	return TConstStructView<FFaerieMassFragment>();
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
		if (const ItemData::FMassFragmentTypeInterface* Traits = ItemData::GetFragmentTraitsInterface(Fragment.GetScriptStruct()))
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

IFaerieItemOwnerInterface* FFaerieMassItemOwner::GetInterface() const
{
	return Cast<IFaerieItemOwnerInterface>(Owner.Get());
}

namespace Faerie::ItemData
{
	FMassEntityManager& GetEntityManager(const FRequireEntityManager& EntityManager)
	{
		return EntityManager.Resolve();
	}

	FConstStructView GetEntityFragment(const FRequireEntityManager& EntityManager, const FMassEntityHandle ItemHandle, const TNotNull<const UScriptStruct*> FragmentType)
	{
		FMassEntityManager& Manager = EntityManager.Resolve();
		if (Manager.IsEntityValid(ItemHandle))
		{
			return Manager.GetFragmentDataStruct(ItemHandle, FragmentType);
		}

		return FConstStructView();
	}

	TConstStructView<FFaerieMassFragment> GetEntityFragmentOrDefault(const FOptionalEntityManager& EntityManager,
		const FReference& Reference, const TNotNull<const UScriptStruct*> FragmentType, const FGameplayTag ReferenceTag)
	{
		if (FMassEntityManager* Manager = EntityManager.ResolvePtr())
		{
			if (Manager->IsEntityValid(Reference->GetMassEntityHandle()))
			{
				// Look for a live fragment of the given type.
				if (FConstStructView View = Manager->GetFragmentDataStruct(Reference->GetMassEntityHandle(), FragmentType);
					View.IsValid())
				{
					return *reinterpret_cast<TConstStructView<FFaerieMassFragment>*>(&View);
				}

				if (auto&& ReferenceFragment = GetEntityFragment<FFaerieReferenceFragment>(*Manager, Reference->GetMassEntityHandle()))
				{
					if (const UFaerieItem* ReferencedAsset = ReferenceFragment->GetReferencedItem(ReferenceTag, false))
					{
						// Recurse our search into the referenced asset.
						if (auto Default = ReferencedAsset->GetDefaultFragment(FragmentType, ReferenceTag);
							Default.IsValid())
						{
							return Default;
						}
					}
				}
			}
		}

		return GetDefaultFragment(Reference->GetItemPtr(), FragmentType);
	}

	TConstStructView<FFaerieMassFragment> GetDefaultFragment(const UFaerieItem* ItemAsset, const TNotNull<const UScriptStruct*> FragmentType, const FGameplayTag ReferenceTag)
	{
		if (ItemAsset)
		{
			if (const TConstStructView<FFaerieMassFragment>& DefaultFragment = ItemAsset->GetDefaultFragment(FragmentType, ReferenceTag);
				DefaultFragment.IsValid())
			{
				return DefaultFragment;
			}
		}

		return TConstStructView<FFaerieMassFragment>();
	}
}
