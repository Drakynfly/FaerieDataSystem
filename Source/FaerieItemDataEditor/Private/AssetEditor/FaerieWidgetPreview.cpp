// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "AssetEditor/FaerieWidgetPreview.h"
#include "FaerieItemAsset.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieWidgetPreview)

const UFaerieItem* UFaerieWidgetPreview::GetItemObject() const
{
	return Asset->GetEditorItemView();
}

TScriptInterface<IFaerieItemOwnerInterface> UFaerieWidgetPreview::GetItemOwner() const
{
	return nullptr;
}

FFaerieItemStack UFaerieWidgetPreview::Release(int32 Copies) const
{
	// Release doesn't make sense here, since we are only ever "owning" an asset.
	return FFaerieItemStack();
}

void UFaerieWidgetPreview::InitFaerieWidgetPreview(UFaerieItemAsset* InAsset)
{
	Asset = InAsset;
}
