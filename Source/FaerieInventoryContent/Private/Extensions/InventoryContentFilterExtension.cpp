// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Extensions/InventoryContentFilterExtension.h"
#include "FaerieItemContainerBase.h"
#include "FaerieItemDataFilter.h"
#include "FaerieItemDataView.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryContentFilterExtension)

EEventExtensionResponse UInventoryContentFilterExtension::AllowsAddition(const TNotNull<const UFaerieItemContainerBase*> Container,
                                                                         const TConstArrayView<FFaerieItemDataView> Views,
                                                                         FFaerieExtensionAllowsAdditionArgs) const
{
	if (ensure(IsValid(Filter)))
	{
		for (const FFaerieItemDataView& View : Views)
		{
			if (!Filter->Exec(Container, View))
			{
				return EEventExtensionResponse::Disallowed;
			}
		}

		return EEventExtensionResponse::Allowed;
	}

	return EEventExtensionResponse::NoExplicitResponse;
}