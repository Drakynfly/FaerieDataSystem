// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemDataLibrary.h"
#include "FaerieItem.h"
#include "FaerieItemAsset.h"
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

UFaerieItem* UFaerieItemDataLibrary::GetItemInstance(const UFaerieItemAsset* Asset, const bool MutableInstance)
{
	if (IsValid(Asset))
	{
		return Asset->GetItemInstance(MutableInstance);
	}
	return nullptr;
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
