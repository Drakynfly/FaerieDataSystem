// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemSource.h"
#include "FaerieItemAsset.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemSource)

FFaerieUnownedItemStack Faerie::ItemData::FItemDataBuilder::GetForRuntime() const
{
	return FFaerieUnownedItemStack(RuntimeItem, Copies);
}

bool Faerie::ItemData::FItemDataBuilder::IsValid() const
{
#if WITH_EDITOR
	if (::IsValid(ItemAsset))
	{
		return true;
	}
#endif
	return RuntimeItem.IsSet();
}

const FName IFaerieItemSource::MutableSourceTag(TEXT("MutableSource"));
