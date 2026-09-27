// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Consumable/FaerieConsumableFragment.h"
#include "Consumable/FaerieItemUsesFragment.h"
#include "FaerieItem.h"
#include "FaerieItemProxy.h"

#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieConsumableFragment)

namespace Faerie::Generation
{
	bool CanConsume(const FMassEntityManager& EntityManager, const FFaerieItemProxy& Proxy, const TNotNull<const UScriptStruct*> FragmentType,
		const TNotNull<const AActor*> Consumer, const int32 Cost)
	{
		TConstStructView<FFaerieMassFragment> Fragment = ItemData::GetEntityFragmentOrDefault(&EntityManager, Proxy.GetItemInstanceOrInvalid(), FragmentType);
		if (Fragment.IsValid())
		{
			if (const FFaerieConsumableFragment* ConsumableFragment = Fragment.GetPtr<FFaerieConsumableFragment>())
			{
				if (const UFaerieConsumableLogicBase* Logic = ConsumableFragment->GetConsumableLogic())
				{
					return Logic->TestConsumable(EntityManager, Fragment, Proxy, Consumer, Cost);
				}
			}
		}

		return false;
	}

	bool TryConsume(FMassEntityManager& EntityManager, const FFaerieItemProxy& Proxy, const TNotNull<const UScriptStruct*> FragmentType, const TNotNull<AActor*> Consumer, const int32 Cost)
	{
		TConstStructView<FFaerieMassFragment> Fragment = ItemData::GetEntityFragmentOrDefault(&EntityManager, Proxy.GetItemInstanceOrInvalid(), FragmentType);
		if (Fragment.IsValid())
		{
			if (const FFaerieConsumableFragment* ConsumableFragment = Fragment.GetPtr<FFaerieConsumableFragment>())
			{
				if (const UFaerieConsumableLogicBase* Logic = ConsumableFragment->GetConsumableLogic())
				{
					Logic->OnConsumed(EntityManager, Fragment, Proxy, Consumer, Cost);
					return true;
				}
			}
		}
		return false;
	}

	bool CanRemoveUses(const FMassEntityManager& EntityManager, const FFaerieItemProxy& Proxy, const int32 Cost,
		const bool ResultIfNoUsesFragment)
	{
		const TOptional<FFaerieItemInstance> Item = Proxy.GetItemInstance();
		ItemData::FUsesHelper Uses(EntityManager, Item.GetValue());
		if (Uses.HasFragmentValue())
		{
			return Uses.HasUsesRemaining(Cost);
		}

		return ResultIfNoUsesFragment;
	}

	void RemoveUses(FMassEntityManager& EntityManager, const FFaerieItemProxy& Proxy, const int32 Cost)
	{
		const TOptional<FFaerieItemInstance> Item = Proxy.GetItemInstance();
		ItemData::FUsesHelper Uses(EntityManager, Item.GetValue());
		if (Uses.HasFragmentValue())
		{
			// Remove a usage.
			Uses.RemoveUses(Proxy, Cost);
		}
	}
}

using namespace Faerie;

bool UFaerieConsumableLogicBase::TestConsumable(const FMassEntityManager& EntityManager, const TConstStructView<FFaerieMassFragment>& Fragment,
	const FFaerieItemProxy& Proxy, const TNotNull<const AActor*> Consumer, const int32 Cost) const
{
	return Generation::CanRemoveUses(EntityManager, Proxy, Cost, true);
}
