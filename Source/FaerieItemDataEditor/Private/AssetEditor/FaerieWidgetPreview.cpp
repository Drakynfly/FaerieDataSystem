// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "AssetEditor/FaerieWidgetPreview.h"
#include "FaerieItemAsset.h"

const UFaerieItem* UFaerieWidgetPreview::GetItemObject() const
{
	return Asset->GetEditorItemView();
}

TScriptInterface<IFaerieItemOwnerInterface> UFaerieWidgetPreview::GetOwner() const
{
	return nullptr;
}

void UFaerieWidgetPreview::InitFaerieWidgetPreview(UFaerieItemAsset* InAsset)
{
	Asset = InAsset;
}
