// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Extensions/InventoryContentFilterExtension.h"
#include "FaerieItemContainerBase.h"
#include "FaerieItemFilter.h"
#include "FaerieItemTemplate.h"
#include "EntityManagerHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryContentFilterExtension)

using namespace Faerie;

EFaerieExtensionResponse FFaerieItemContainerContentFilter::AllowsAddition(const TNotNull<const UFaerieItemContainerBase*> Container,
	const Utils::TArrayAdapter<FFaerieItemProxy>& Proxies, FFaerieExtensionAllowsAdditionArgs Args) const
{
	if (ensure(IsValid(Filter)))
	{
		const FMassEntityManager* EntityManager = ItemData::GetFaerieEntityManager();
		for (int32 i = 0; i < Proxies.Num(); ++i)
		{
			const FFaerieItemProxy& Proxy = Proxies[i];
			if (!Filter->TryMatch(EntityManager, Proxy))
			{
				return EFaerieExtensionResponse::Disallowed;
			}
		}
	}

	return EFaerieExtensionResponse::NoExplicitResponse;
}