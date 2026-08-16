// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Extensions/InventoryContentFilterExtension.h"
#include "FaerieItemContainerBase.h"
#include "FaerieItemDataFilter.h"
#include "EntityManagerHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryContentFilterExtension)

EEventExtensionResponse UInventoryContentFilterExtension::AllowsAddition(const TNotNull<const UFaerieItemContainerBase*> Container,
                                                                         const Faerie::Utils::TArrayAdapter<FFaerieItemProxy>& Proxies,
                                                                         FFaerieExtensionAllowsAdditionArgs) const
{
	if (ensure(Filter.IsValid()))
	{
		const FMassEntityManager* EntityManager = Faerie::ItemData::GetFaerieEntityManager();
		for (int32 i = 0; i < Proxies.Num(); ++i)
		{
			const FFaerieItemProxy Proxy = Proxies[i];
			if (!Filter->Exec(EntityManager, Proxy))
			{
				return EEventExtensionResponse::Disallowed;
			}
		}

		return EEventExtensionResponse::Allowed;
	}

	return EEventExtensionResponse::NoExplicitResponse;
}