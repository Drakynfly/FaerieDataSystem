// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemAsset.h"

#include "FaerieMassFragmentAccessors.h"
#include "FaerieItemDataView.h"
#include "FaerieMassEntity.h"
#include "FaerieItemTemplate.h"
#include "FaerieItemInstancingContext.h"

#include "UObject/AssetRegistryTagsContext.h"
#include "UObject/ObjectSaveContext.h"

#if WITH_EDITOR
#include "ThumbnailRendering/SceneThumbnailInfo.h"
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemAsset)

using namespace Faerie;

#if WITH_EDITORONLY_DATA

namespace Faerie::ItemAssetPrivate
{
	static const FName NAME_IsEditorTemplate("IsEditorTemplate");
}

#define LOCTEXT_NAMESPACE "FaerieItemAssetMetadata"

void UFaerieItemAsset::GetAssetRegistryTagMetadata(TMap<FName, FAssetRegistryTagMetadata>& OutMetadata) const
{
	Super::GetAssetRegistryTagMetadata(OutMetadata);

	OutMetadata.Add(
		ItemAssetPrivate::NAME_IsEditorTemplate,
		FAssetRegistryTagMetadata()
		.SetDisplayName(LOCTEXT("IsEditorTemplate", "Is Editor Template"))
		.SetTooltip(LOCTEXT("IsEditorTemplateTooltip", "This asset appears in the template section when creating a new asset."))
	);
}

#undef LOCTEXT_NAMESPACE

#endif

void UFaerieItemAsset::GetAssetRegistryTags(FAssetRegistryTagsContext Context) const
{
	Super::GetAssetRegistryTags(Context);

	Context.AddTag(FAssetRegistryTag(MutableSourceTag, CanBeMutable() ? TEXT("True") : TEXT("False"), FAssetRegistryTag::TT_Alphabetical));
#if WITH_EDITORONLY_DATA
	Context.AddTag(FAssetRegistryTag(TEXT("ItemAssetVersion"), LexToString(AssetVersion), FAssetRegistryTag::TT_Numerical));
#endif
}

void UFaerieItemAsset::PreSave(FObjectPreSaveContext SaveContext)
{
#if WITH_EDITOR
	CachedMutability = DetermineFragmentMutability();
#endif

	Super::PreSave(SaveContext);
}

void UFaerieItemAsset::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	// Items loaded from disk in shipping builds don't need to re-cache this.
	const bool DeterminedMutability = DetermineFragmentMutability();
	if (DeterminedMutability != CachedMutability)
	{
		CachedMutability = DeterminedMutability;
		(void)MarkPackageDirty();
	}

	// Make sure thumbnail info exists
	if (!IsValid(ThumbnailInfo))
	{
		ThumbnailInfo = NewObject<USceneThumbnailInfo>(this, NAME_None, RF_Transactional);
	}
#endif
}

#if WITH_EDITOR

#define LOCTEXT_NAMESPACE "FaerieItemAssetValidation"

EDataValidationResult UFaerieItemAsset::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	for (const FInstancedStruct& Fragment : MassFragments)
	{
		if (const ItemData::FMassTypeTraitsInterface* Traits = ItemData::GetMassTraitsInterface(Fragment.GetScriptStruct()))
		{
			Result = CombineDataValidationResults(Result, Traits->IsDataValid(Fragment.GetMemory(), Context));
		}
	}

	if (!IsValid(Template))
	{
		Context.AddWarning(LOCTEXT("InvalidTemplateObject", "Template is invalid! Unable to check Item for pattern-correctness."));
	}
	else
	{
		const ItemData::FScopeProxy Proxy(this, 1);

		if (TArray<FText> TemplateMatchErrors;
			!Template->MatchesWithErrors(FFaerieItemProxy(FFaerieItemProxy::ESingleFrame, nullptr, &Proxy), TemplateMatchErrors))
		{
			Context.AddError(LOCTEXT("PatternMatchFailed", "Item failed to match the pattern of its Template!"));

			for (auto&& TemplateMatchError : TemplateMatchErrors)
			{
				Context.AddError(TemplateMatchError);
			}
		}
	}


	return Result;
}

#undef LOCTEXT_NAMESPACE

#endif

bool UFaerieItemAsset::CanBeMutable() const
{
	return CachedMutability;
}

ItemData::FItemDataBuilder UFaerieItemAsset::CreateItemStack(const FFaerieItemInstancingContext& Context) const
{
	int32 OutCopies = 1;

	if (Context.CopiesOverride.IsSet())
	{
		if (!CachedMutability || Context.EmitStackEvenIfMutableBecauseCallerKnowsToDuplicateItem)
		{
			OutCopies = Context.CopiesOverride.GetValue();
		}
	}

#if WITH_EDITOR
	if (Context.RunningInEditor)
	{
		if (Context.InstanceNewItemAsset)
		{
			check(Context.Editor_ItemInstanceOuter);
			return ItemData::FItemDataBuilder(CreateReferencingInstance_Editor(Context.Editor_ItemInstanceOuter), OutCopies);
		}

		return ItemData::FItemDataBuilder(const_cast<ThisClass*>(this), OutCopies);
	}
	else
#endif
	{
		check(Context.EntityManager);
		return ItemData::FItemDataBuilder(CreateReferencingInstance(*Context.EntityManager), OutCopies);
	}
}

#if WITH_EDITOR
UFaerieItemAsset* UFaerieItemAsset::CreateReferencingInstance_Editor(const TNotNull<UObject*> InstanceOuter) const
{
	// Create and return a new item asset instance
	return DuplicateObject(this, InstanceOuter);
}
#endif

FMassEntityHandle UFaerieItemAsset::CreateReferencingInstance(FMassEntityManager& EntityManager) const
{
	FInstancedStruct FragmentStruct;
	FragmentStruct.InitializeAs<FFaerieItemDataDefaults>();

	FFaerieItemDataDefaults& Default = FragmentStruct.GetMutable<FFaerieItemDataDefaults>();
	Default.ItemAsset = this;

	return ItemData::InitializeItemEntity(EntityManager, FMassTagBitSet(), MakeConstArrayView(&FragmentStruct, 1));
}

bool UFaerieItemAsset::HasDefaultFragment(const TNotNull<const UScriptStruct*> StructType, const TSubScriptStructOf<FFaerieMassItemObject>& ReferenceType) const
{
	for (auto&& DefaultFragment : MassFragments)
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
		for (int32 i = MassFragments.Num() - 1; i >= 0; --i)
		{
			if (MassFragments[i].GetScriptStruct() == ReferenceType)
			{
				if (const FFaerieMassItemObject* ReferenceFragment = MassFragments[i].GetPtr<FFaerieMassItemObject>())
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

FConstStructView UFaerieItemAsset::GetDefaultFragment(const TNotNull<const UScriptStruct*> StructType, const TSubScriptStructOf<FFaerieMassItemObject>& ReferenceType) const
{
	for (auto&& DefaultFragment : MassFragments)
	{
		if (DefaultFragment.GetScriptStruct()->IsChildOf(StructType))
		{
			return DefaultFragment;
		}
	}

	// Fallback to retrieving a default value via reference.
	// Iterate in reverse for now (since static references are usually placed at the end.
	// @todo control with a cvar or replace with order sorted fragments...
	for (int32 i = MassFragments.Num() - 1; i >= 0; --i)
	{
		if (MassFragments[i].GetScriptStruct() == ReferenceType)
		{
			if (const FFaerieMassItemObject* ReferenceFragment = MassFragments[i].GetPtr<FFaerieMassItemObject>())
			{
				// Recurse our search into the referenced asset.
				return ReferenceFragment->ItemAsset->GetDefaultFragment(StructType, ReferenceType);
			}
		}
	}

	return FConstStructView();
}

#if WITH_EDITORONLY_DATA
bool UFaerieItemAsset::DetermineFragmentMutability()
{
	for (const FInstancedStruct& Fragment : MassFragments)
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

void UFaerieItemAsset::SetDefaultFragment(const FConstStructView DefaultStructValue)
{
	if (!DefaultStructValue.GetScriptStruct()->IsChildOf<FFaerieMassFragment>())
	{
		// Temporary guard
		unimplemented()
		return;
	}

	for (auto&& DefaultFragment : MassFragments)
	{
		if (DefaultFragment.GetScriptStruct() == DefaultStructValue.GetScriptStruct())
		{
			DefaultFragment = DefaultStructValue;
			return;
		}
	}

	MassFragments.Add(FInstancedStruct(DefaultStructValue));
}
#endif

#undef LOCTEXT_NAMESPACE
