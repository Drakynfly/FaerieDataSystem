// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "AssetEditor/FaerieWidgetPreview.h"
#include "FaerieItemAsset.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieWidgetPreview)

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
