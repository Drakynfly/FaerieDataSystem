// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Extensions/InventoryContentFilterExtension.h"
#include "FaerieItemContainerBase.h"
#include "FaerieItemFilter.h"
#include "FaerieItemTemplate.h"
#include "FaerieInventoryLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryContentFilterExtension)

using namespace Faerie;

EFaerieExtensionResponse FFaerieItemContainerContentFilter::AllowsAddition(const FMassEntityManager& EntityManager,
	const TNotNull<const UFaerieItemContainerBase*> Container, const Utils::TArrayAdapter<FFaerieItemProxy>& Proxies,
	const FFaerieExtensionAllowsAdditionArgs Args, const bool RecursingCall) const
{
	if (!(ensure(IsValid(Filter))))
	{
		return EFaerieExtensionResponse::NoExplicitResponse;
	}

	if (RecursingCall && !ApplyToChildren)
	{
		return EFaerieExtensionResponse::NoExplicitResponse;
	}

	for (int32 i = 0; i < Proxies.Num(); ++i)
	{
		const FFaerieItemProxy& Proxy = Proxies[i];

#if WITH_EDITOR
		if (Container::FScopedExpectedResponse::Get().IsSet())
		{
			TArray<FText> Errors;
			if (!Filter->TryMatchWithDescriptions(&EntityManager, Proxy, Errors))
			{
				if (Container::FScopedExpectedResponse::Get().GetValue() != EFaerieExtensionResponse::Disallowed)
				{
					for (FText& Error : Errors)
					{
						UE_LOGF(LogFaerieInventory, Error, "FaerieItemContainerContentFilter::AllowsAddition: Unexpected Disallowed result for Proxy[%i]. Error message: '%ls'", i, *Error.ToString())
					}
				}
				return EFaerieExtensionResponse::Disallowed;
			}
		}
		else
#endif
		{
			if (!Filter->TryMatch(&EntityManager, Proxy))
			{
				return EFaerieExtensionResponse::Disallowed;
			}
		}
	}

	return EFaerieExtensionResponse::NoExplicitResponse;
}