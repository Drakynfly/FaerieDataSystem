﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemAsset.h"

#include "FaerieItem.h"
#include "FaerieItemTemplate.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include "FaerieItemDataProxy.h"
#include "Tokens/FaerieInfoToken.h"
#include "UObject/ObjectSaveContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemAsset)

void UFaerieItemAsset::PreSave(FObjectPreSaveContext SaveContext)
{
#if WITH_EDITOR
	if (!IsValid(Item))
	{
		Item = NewObject<UFaerieItem>(this);
	}

	Item->MutabilityFlags = EFaerieItemMutabilityFlags::None;

	if (AlwaysMutable)
	{
		EnumAddFlags(Item->MutabilityFlags, EFaerieItemMutabilityFlags::AlwaysTokenMutable);
	}

	Item->Tokens.Empty();
	for (auto&& Token : Tokens)
	{
		Item->Tokens.Add(DuplicateObject(Token, Item));
	}
#endif

	Super::PreSave(SaveContext);
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

UFaerieItem* UFaerieItemAsset::CreateItemInstance(const UItemInstancingContext* Context) const
{
	if (!IsValidChecked(Item)) return nullptr;
	return Item->CreateInstance(Context->Flags);
}

UFaerieItem* UFaerieItemAsset::GetItemInstance(const EFaerieItemInstancingMutability Mutability) const
{
	if (!IsValidChecked(Item)) return nullptr;
	return Item->CreateInstance(Mutability);
}