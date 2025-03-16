// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemEditHandle.h"
#include "FaerieItem.h"
#include "FaerieItemDataProxy.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemEditHandle)

FFaerieItemEditHandle::FFaerieItemEditHandle(const UFaerieItem* InItem)
{
	Item = InItem->MutateCast();
}

FFaerieItemEditHandle::FFaerieItemEditHandle(const IFaerieItemDataProxy* InProxy)
{
	Item = InProxy->GetItemObject()->MutateCast();
}

bool FFaerieItemEditHandle::IsValid() const
{
	return Item.IsValid();
}
