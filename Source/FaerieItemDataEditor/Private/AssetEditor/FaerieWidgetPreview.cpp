// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "AssetEditor/FaerieWidgetPreview.h"
#include "FaerieItemAsset.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieWidgetPreview)

FMassEntityHandle UFaerieWidgetPreview::GetEntityHandle() const
{
	return FMassEntityHandle();
}

TOptional<FFaerieItemInstance> UFaerieWidgetPreview::GetItemInstance() const
{
	if (Asset.IsValid())
	{
		return Asset->GetTemplateInstance();
	}
	return NullOpt;
}

UObject* UFaerieWidgetPreview::GetItemOwner() const
{
	return nullptr;
}

void UFaerieWidgetPreview::InitFaerieWidgetPreview(UFaerieItemAsset* InAsset)
{
	Asset = InAsset;
}
