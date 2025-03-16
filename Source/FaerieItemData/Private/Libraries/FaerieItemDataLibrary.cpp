// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemDataLibrary.h"
#include "FaerieItem.h"
#include "FaerieItemAsset.h"
#include "FaerieItemEditHandle.h"
#include "FaerieItemToken.h"
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

UFaerieItem* UFaerieItemDataLibrary::GetItemInstance(const UFaerieItemAsset* Asset, const EFaerieItemInstancingMutability Mutability)
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

bool UFaerieItemDataLibrary::EditToken(const FFaerieItemEditHandle& Handle, UFaerieItemToken* Token,
	const FBlueprintTokenEdit& Edit)
{
	if (!Handle.IsValid())
	{
		return false;
	}

	if (!ensure(Edit.IsBound()))
	{
		return false;
	}

	if (!ensure(Token->GetOuterItem() == Handle.GetItem()))
	{
		return false;
	}

	Token->EditToken(
		[Edit](UFaerieItemToken* PassThrough)
		{
			return Edit.Execute(PassThrough);
		});
	return true;
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
