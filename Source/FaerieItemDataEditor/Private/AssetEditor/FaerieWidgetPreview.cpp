// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "AssetEditor/FaerieWidgetPreview.h"
#include "FaerieItemAsset.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieWidgetPreview)

const UFaerieItemAsset* UFaerieWidgetPreview::GetItemAsset() const
{
	if (Asset.IsValid())
	{
		return Asset.Get();
	}
	return nullptr;
}

FMassEntityHandle UFaerieWidgetPreview::GetEntityHandle() const
{
	return FMassEntityHandle();
}

void UFaerieWidgetPreview::InitFaerieWidgetPreview(UFaerieItemAsset* InAsset)
{
	Asset = InAsset;
}
