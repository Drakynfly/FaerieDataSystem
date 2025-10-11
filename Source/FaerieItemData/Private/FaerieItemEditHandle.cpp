// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemEditHandle.h"
#include "FaerieItem.h"
#include "FaerieItemProxy.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemEditHandle)

FFaerieItemEditHandle::FFaerieItemEditHandle(const UFaerieItem* InItem)
{
	Item = InItem->MutateCast();
}

FFaerieItemEditHandle::FFaerieItemEditHandle(const IFaerieItemDataProxy* InProxy)
{
	const UFaerieItem* ItemObject = InProxy->GetItemObject();
	if (!::IsValid(ItemObject))
	{
		return;
	}
	Item = ItemObject->MutateCast();
}

bool FFaerieItemEditHandle::IsValid() const
{
	return Item.IsValid();
}
