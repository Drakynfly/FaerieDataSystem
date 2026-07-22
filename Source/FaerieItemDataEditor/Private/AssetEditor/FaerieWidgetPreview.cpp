// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "AssetEditor/FaerieWidgetPreview.h"
#include "FaerieItemAsset.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieWidgetPreview)

TOptional<FFaerieItemInstance> UFaerieWidgetPreview::GetItemInstance() const
{
	if (Asset.IsValid())
	{
		return Asset->GetTemplateInstance();
	}
	return NullOpt;
}

IFaerieItemOwnerInterface* UFaerieWidgetPreview::GetItemOwner() const
{
	return nullptr;
}

Faerie::ItemData::FProxyChangeEvent::RegistrationType& UFaerieWidgetPreview::GetOnProxyChangeEvent()
{
	return OnChangeEvent;
}

void UFaerieWidgetPreview::InitFaerieWidgetPreview(UFaerieItemAsset* InAsset)
{
	Asset = InAsset;
}
