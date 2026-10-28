// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemAsset.h"

#include "FaerieItem.h"
#include "FaerieItemDataView.h"
#include "FaerieItemInstance.h"
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
	static const FName ItemInstanceName = TEXT("AssetInstance");
	if (!IsValid(Item) || (Item->GetName() != ItemInstanceName))
	{
		Item = NewObject<UFaerieItem>(this, ItemInstanceName);

		// We call this manually, to update the LastModified.
		Item->PreSave(SaveContext);
	}

	// Setting RF_Public suppresses "Illegal reference to private object" warnings when referenced by a Level.
	Item->SetFlags(RF_Public);

	switch (InstanceMutability)
	{
	case EFaerieItemInstancingMutability::Automatic:
		Item->InstancesCanMutate = Item->DetermineFragmentMutability();
		break;
	case EFaerieItemInstancingMutability::Mutable:
		Item->InstancesCanMutate = true;
		break;
	case EFaerieItemInstancingMutability::Immutable:
		Item->InstancesCanMutate = false;
		break;
	}

	Item->FragmentDefaults.Empty();
	for (const TInstancedStruct<FFaerieMassFragment>& Fragment : Fragments)
	{
		Item->FragmentDefaults.Add(FInstancedStruct(FConstStructView(Fragment.GetScriptStruct(), Fragment.GetMemory())));
	}

	// Bring item version up to date on re-save.
	Item->FormatVersion = static_cast<int32>(ItemData::EFormatVersion::LatestVersion);
#endif

	Super::PreSave(SaveContext);
}

void UFaerieItemAsset::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
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

	if (!IsValid(Item))
	{
		Context.AddError(LOCTEXT("InvalidItemObject", "Item is invalid! Please try making sure asset is correctly configured and resave this asset."));
		Result = EDataValidationResult::Invalid;
	}
	else
	{
		Result = CombineDataValidationResults(Result, Item.Get()->IsDataValid(Context));

		if (!IsValid(Template))
		{
			Context.AddWarning(LOCTEXT("InvalidTemplateObject", "Template is invalid! Unable to check Item for pattern-correctness."));
		}
		else
		{
			const ItemData::FScopeProxy Proxy(Item, 1);

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
	}

	return Result;
}

#undef LOCTEXT_NAMESPACE

#endif

bool UFaerieItemAsset::CanBeMutable() const
{
	if (IsValid(Item))
	{
		// Item is part of this asset, which will cannot have InstanceMutable set. Assets are never InstanceMutable, as
		// they are non-instanced templates that get duplicates made from them if they are supposed to be modifiable.
		// Checking for DataMutable lets us know if this item has fragments that can mutate.
		return Item->CanMutate();
	}
	return false;
}

ItemData::FItemDataBuilder UFaerieItemAsset::CreateItemStack(const FFaerieItemInstancingContext& Context) const
{
	if (!IsValidChecked(Item)) return ItemData::FItemDataBuilder();

	int32 OutCopies = 1;

	if (Context.CopiesOverride.IsSet())
	{
		if (!Item->CanMutate() || Context.EmitStackEvenIfMutableBecauseCallerKnowsToDuplicateItem)
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

		return ItemData::FItemDataBuilder(Item, OutCopies);
	}
	else
#endif
	{
		check(Context.EntityManager);
		return ItemData::FItemDataBuilder(CreateReferencingInstance_Runtime(*Context.EntityManager), OutCopies);
	}
}

#if WITH_EDITOR
UFaerieItem* UFaerieItemAsset::CreateReferencingInstance_Editor(const TNotNull<UObject*> InstanceOuter) const
{
	FInstancedStruct FragmentStruct;
	FragmentStruct.InitializeAs<FFaerieItemDataDefaults>();

	FFaerieItemDataDefaults& Default = FragmentStruct.GetMutable<FFaerieItemDataDefaults>();
	Default.ItemAsset = Item;

	// Create and return a new item instance
	return UFaerieItem::CreateNewInstance(MakeConstArrayView(&FragmentStruct, 1), InstanceOuter, EFaerieItemInstancingMutability::Mutable);
}
#endif

FMassEntityHandle UFaerieItemAsset::CreateReferencingInstance_Runtime(FMassEntityManager& EntityManager) const
{
	FInstancedStruct FragmentStruct;
	FragmentStruct.InitializeAs<FFaerieItemDataDefaults>();

	FFaerieItemDataDefaults& Default = FragmentStruct.GetMutable<FFaerieItemDataDefaults>();
	Default.ItemAsset = Item;

	return ItemData::InitializeItemEntity(EntityManager, nullptr, FMassTagBitSet(), MakeConstArrayView(&FragmentStruct, 1));
}

#undef LOCTEXT_NAMESPACE
