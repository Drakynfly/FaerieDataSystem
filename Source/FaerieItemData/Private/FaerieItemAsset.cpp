// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemAsset.h"

#include "FaerieItem.h"
#include "FaerieItemStackView.h"
#include "FaerieItemTemplate.h"

#include "Tokens/FaerieInfoToken.h"

#include "UObject/ObjectSaveContext.h"

#if WITH_EDITOR
#include "ThumbnailRendering/SceneThumbnailInfo.h"
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemAsset)

#define LOCTEXT_NAMESPACE "UFaerieItemAsset"

namespace Faerie::ItemAssetPrivate
{
	static const FName NAME_IsEditorTemplate("IsEditorTemplate");
}

#if WITH_EDITORONLY_DATA
void UFaerieItemAsset::GetAssetRegistryTagMetadata(TMap<FName, FAssetRegistryTagMetadata>& OutMetadata) const
{
	Super::GetAssetRegistryTagMetadata(OutMetadata);

	OutMetadata.Add(
		Faerie::ItemAssetPrivate::NAME_IsEditorTemplate,
		FAssetRegistryTagMetadata()
		.SetDisplayName(LOCTEXT("IsEditorTemplate", "Is Editor Template"))
		.SetTooltip(LOCTEXT("IsEditorTemplateTooltip", "This asset appears in the template section when creating a new asset."))
	);
}
#endif

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

	Item->MutabilityFlags = EFaerieItemMutabilityFlags::None;

	if (AlwaysMutable)
	{
		EnumAddFlags(Item->MutabilityFlags, EFaerieItemMutabilityFlags::AlwaysTokenMutable);
	}

	Item->Tokens.Empty();
	TMultiMap<UClass*, UFaerieItemToken*> ClassMap;
	for (auto&& Token : Tokens)
	{
		ClassMap.Add(Token->GetClass(), Token);
	}
	TArray<UClass*> Keys;
	ClassMap.GetKeys(Keys);
	for (auto&& KeyClass : Keys)
	{
		TArray<UFaerieItemToken*, TInlineAllocator<1>> TokensOfClass;
		ClassMap.MultiFind(KeyClass, TokensOfClass);
		if (TokensOfClass.Num() == 1)
		{
			Item->Tokens.Add(DuplicateObject(TokensOfClass[0], Item, KeyClass->GetFName()));
		}
		else
		{
			for (auto&& It = TokensOfClass.CreateIterator(); It; ++It)
			{
				const FName TokenName(KeyClass->GetName() + TEXT("_") + LexToString(It.GetIndex()));
				Item->Tokens.Add(DuplicateObject(*It, Item, TokenName));
			}
		}
	}
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

#define LOCTEXT_NAMESPACE "ValidateFaerieItemAsset"

EDataValidationResult UFaerieItemAsset::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	if (!IsValid(Item))
	{
		Context.AddError(LOCTEXT("InvalidItemObject", "Item is invalid! Please try making sure Tokens are correctly configured and resave this asset."));
		Result = EDataValidationResult::Invalid;
	}
	else
	{
		Result = CombineDataValidationResults(Result, Item->IsDataValid(Context));
	}

	if (!IsValid(Template))
	{
		Context.AddWarning(LOCTEXT("InvalidTemplateObject", "Template is invalid! Unable to check Item for pattern-correctness."));
	}

	for (auto&& Token : Tokens)
	{
		Result = CombineDataValidationResults(Result, Token->IsDataValid(Context));
	}

	if (IsValid(Item) && IsValid(Template))
	{
		if (TArray<FText> TemplateMatchErrors;
			!Template->TryMatchWithDescriptions({Item, 1}, TemplateMatchErrors))
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
	if (IsValid(Item))
	{
		// Item is part of this asset, which will cannot have InstanceMutable set. Assets are never InstanceMutable, as
		// they are non-instanced templates that get duplicates made from them if they are supposed to be modifiable.
		// Checking for DataMutable lets us know if this item has tokens that can mutate.
		return Item->IsDataMutable();
	}
	return false;
}

FFaerieAssetInfo UFaerieItemAsset::GetSourceInfo() const
{
	if (!IsValidChecked(Item)) return FFaerieAssetInfo();

	if (auto&& InfoToken = Item->GetToken<UFaerieInfoToken>())
	{
		return InfoToken->GetAssetInfo();
	}
	return FFaerieAssetInfo();
}

const UFaerieItem* UFaerieItemAsset::CreateItemInstance(const FFaerieItemInstancingContext* Context) const
{
	if (!IsValidChecked(Item)) return nullptr;
	if (Context)
	{
		return Item->CreateInstance(Context->Flags);
	}
	return Item->CreateInstance();
}

const UFaerieItem* UFaerieItemAsset::GetItemInstance(const EFaerieItemInstancingMutability Mutability) const
{
	if (!IsValidChecked(Item)) return nullptr;
	return Item->CreateInstance(Mutability);
}

#undef LOCTEXT_NAMESPACE