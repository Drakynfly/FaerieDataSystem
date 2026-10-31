// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Consumable/FaerieConsumableFragment.h"
#include "Consumable/FaerieItemUsesFragment.h"
#include "FaerieMassFragmentAccessors.h"

#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieConsumableFragment)

namespace Faerie::Generation
{
	bool CanConsume(const FFaerieItemProxy& Proxy, const TNotNull<const UScriptStruct*> FragmentType,
		const TNotNull<const AActor*> Consumer, const int32 Cost)
	{
		const FConstStructView Fragment = ItemData::GetItemFragmentOrDefault(Proxy, FragmentType);
		if (Fragment.IsValid())
		{
			if (const FFaerieConsumableFragment* ConsumableFragment = Fragment.GetPtr<FFaerieConsumableFragment>())
			{
				if (const UFaerieConsumableLogicBase* Logic = ConsumableFragment->GetConsumableLogic())
				{
					return Logic->TestConsumable(Fragment, Proxy, Consumer, Cost);
				}
			}
		}

		return false;
	}

	bool TryConsume(const FFaerieItemProxy& Proxy, const TNotNull<const UScriptStruct*> FragmentType, const TNotNull<AActor*> Consumer, const int32 Cost)
	{
		const FConstStructView Fragment = ItemData::GetItemFragmentOrDefault(Proxy, FragmentType);
		if (Fragment.IsValid())
		{
			if (const FFaerieConsumableFragment* ConsumableFragment = Fragment.GetPtr<FFaerieConsumableFragment>())
			{
				if (const UFaerieConsumableLogicBase* Logic = ConsumableFragment->GetConsumableLogic())
				{
					Logic->OnConsumed(Fragment, Proxy, Consumer, Cost);
					return true;
				}
			}
		}
		return false;
	}

	bool CanRemoveUses(const FFaerieItemProxy& Proxy, const int32 Cost,
		const bool ResultIfNoUsesFragment)
	{
		const ItemData::FUsesHelper Uses(Proxy);
		if (Uses.HasFragmentValue())
		{
			return Uses.HasUsesRemaining(Cost);
		}

		return ResultIfNoUsesFragment;
	}

	void RemoveUses(const FFaerieItemProxy& Proxy, const int32 Cost)
	{
		ItemData::FUsesHelper Uses(Proxy);
		if (Uses.HasFragmentValue())
		{
			// Remove a usage.
			Uses.RemoveUses(Proxy, Cost);
		}
	}
}

using namespace Faerie;

bool UFaerieConsumableLogicBase::TestConsumable(const FConstStructView& Fragment, const FFaerieItemProxy& Proxy,
	const TNotNull<const AActor*> Consumer, const int32 Cost) const
{
	return Generation::CanRemoveUses(Proxy, Cost, true);
}
