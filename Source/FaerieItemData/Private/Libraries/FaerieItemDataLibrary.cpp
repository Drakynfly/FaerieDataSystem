// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemDataLibrary.h"
#include "FaerieItem.h"
#include "FaerieItemAsset.h"
#include "FaerieItemEditHandle.h"
#include "FaerieItemStackView.h"
#include "FaerieItemToken.h"
#include "FaerieItemTokenFilter.h"
#include "FlakesJsonSerializer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemDataLibrary)

bool UFaerieItemDataLibrary::Equal_ItemData(const UFaerieItem* A, const UFaerieItem* B)
{
	return UFaerieItem::Compare(A, B, EFaerieItemComparisonFlags::Default);
}

bool UFaerieItemDataLibrary::Equal_ItemToken(const UFaerieItemToken* A, const UFaerieItemToken* B)
{
	return A->CompareWith(B);
}

const UFaerieItem* UFaerieItemDataLibrary::GetItemInstance(const UFaerieItemAsset* Asset, const EFaerieItemInstancingMutability Mutability)
{
	if (IsValid(Asset))
	{
		return Asset->GetItemInstance(Mutability);
	}
	return nullptr;
}

UFaerieItem* UFaerieItemDataLibrary::NewItemInstance(const TArray<UFaerieItemToken*>& Tokens, const EFaerieItemInstancingMutability Mutability)
{
	return UFaerieItem::CreateNewInstance(Tokens, Mutability);
}

bool UFaerieItemDataLibrary::TryGetEditHandle(const UFaerieItem* Item, FFaerieItemEditHandle& Handle)
{
	Handle = FFaerieItemEditHandle(Item);
	return Handle.IsValid();
}

bool UFaerieItemDataLibrary::IsValidHandle(const FFaerieItemEditHandle& Handle)
{
	return Handle.IsValid();
}

UFaerieItem* UFaerieItemDataLibrary::GetItem(const FFaerieItemEditHandle& Handle)
{
	return Handle.GetItem();
}

bool UFaerieItemDataLibrary::AddToken(const FFaerieItemEditHandle& Handle, UFaerieItemToken* Token)
{
	if (Handle.IsValid())
	{
		return Handle->AddToken(Token);
	}
	return false;
}

bool UFaerieItemDataLibrary::RemoveToken(const FFaerieItemEditHandle& Handle, UFaerieItemToken* Token)
{
	if (Handle.IsValid())
	{
		return Handle->RemoveToken(Token);
	}
	return false;
}

int32 UFaerieItemDataLibrary::RemoveTokensByClass(const FFaerieItemEditHandle& Handle, const TSubclassOf<UFaerieItemToken> Class)
{
	if (Handle.IsValid())
	{
		return Handle->RemoveTokensByClass(Class);
	}
	return false;
}

void UFaerieItemDataLibrary::FindTokensByClass(const UFaerieItem* Item, const TSubclassOf<UFaerieItemToken> Class,
											   TArray<UFaerieItemToken*>& FoundTokens)
{
	using namespace Faerie::Token;
	if (!IsValid(Item)) return;
	FoundTokens = FTokenFilter(Item).ByClass(Class).BlueprintOnlyAccess();
}

TArray<UFaerieItemToken*> UFaerieItemDataLibrary::FindTokensByTag(const UFaerieItem* Item, const FGameplayTag& Tag,
																  const bool Exact)
{
	using namespace Faerie::Token;
	if (!IsValid(Item)) return {};
	return FTokenFilter(Item).By<FTagFilter>(Tag, Exact).BlueprintOnlyAccess();
}

TArray<UFaerieItemToken*> UFaerieItemDataLibrary::FindTokensByTags(const UFaerieItem* Item, const FGameplayTagContainer& Tags,
	const bool All, const bool Exact)
{
	using namespace Faerie::Token;
	if (!IsValid(Item)) return {};
	return FTokenFilter(Item).By<FTagsFilter>(Tags, All, Exact).BlueprintOnlyAccess();
}

TArray<UFaerieItemToken*> UFaerieItemDataLibrary::FindTokensByTagQuery(const UFaerieItem* Item, const FGameplayTagQuery& Query)
{
	using namespace Faerie::Token;
	if (!IsValid(Item)) return {};
	return FTokenFilter(Item).By<FTagQueryFilter>(Query).BlueprintOnlyAccess();
}

FFaerieItemStackView UFaerieItemDataLibrary::StackToView(const FFaerieItemStack& Stack)
{
	return Stack;
}

FString UFaerieItemDataLibrary::DebugEmitItemJson(const UFaerieItem* Item, const bool Pretty)
{
	const FJsonObjectWrapper Json = UFlakesJsonLibrary::CreateFlake_Json(Item);
	return UFlakesJsonLibrary::ToString(Json, Pretty);
}

bool UFaerieItemDataLibrary::DebugCompareItemsByJson(const UFaerieItem* ItemA, const UFaerieItem* ItemB)
{
	const FJsonObjectWrapper JsonA = UFlakesJsonLibrary::CreateFlake_Json(ItemA);
	const FJsonObjectWrapper JsonB = UFlakesJsonLibrary::CreateFlake_Json(ItemB);
	return UFlakesJsonLibrary::ToString(JsonA, false) == UFlakesJsonLibrary::ToString(JsonB, false);
}
