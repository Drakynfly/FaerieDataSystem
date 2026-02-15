// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Extensions/InventoryContentFilterExtension.h"
#include "FaerieItemDataFilter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryContentFilterExtension)

EEventExtensionResponse UInventoryContentFilterExtension::AllowsAddition(const TNotNull<const UFaerieItemContainerBase*>,
                                                                         const TConstArrayView<FFaerieItemStackView> Views,
                                                                         FFaerieExtensionAllowsAdditionArgs) const
{
	if (ensure(IsValid(Filter)))
	{
		for (auto&& View : Views)
		{
			if (!Filter->Exec(View))
			{
				return EEventExtensionResponse::Disallowed;
			}
		}

		return EEventExtensionResponse::Allowed;
	}

	return EEventExtensionResponse::NoExplicitResponse;
}