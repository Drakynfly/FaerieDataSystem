// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Consumable/FaerieConsumableFragment.h"
#include "Consumable/FaerieItemUsesFragment.h"
#include "FaerieItem.h"
#include "FaerieItemProxy.h"
#include "EntityManagerHelpers.h"

#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieConsumableFragment)

namespace Faerie::Generation
{
	bool CanConsume(const FFaerieItemProxy& Proxy, const TNotNull<const UScriptStruct*> FragmentType,
		const TNotNull<const AActor*> Consumer, const int32 Cost)
	{
		ItemData::FOptionalEntityManager EntityManager(Consumer);
		TConstStructView<FFaerieMassFragment> Fragment = ItemData::GetEntityFragmentOrDefault(EntityManager, Proxy->GetItemInstance().GetValue(), FragmentType);
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
		ItemData::FOptionalEntityManager EntityManager(Consumer);
		TConstStructView<FFaerieMassFragment> Fragment = ItemData::GetEntityFragmentOrDefault(EntityManager, Proxy->GetItemInstance().GetValue(), FragmentType);
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

	bool CanRemoveUses(const FFaerieItemProxy& Proxy,
	const ItemData::FRequireEntityManager& EntityManager, const int32 Cost, const bool ResultIfNoUsesFragment)
	{
		const TOptional<FFaerieItemInstance> Item = Proxy->GetItemInstance();
		ItemData::FUsesHelper Uses(EntityManager, Item.GetValue());
		if (Uses.HasFragmentValue())
		{
			return Uses.HasUsesRemaining(Cost);
		}

		return ResultIfNoUsesFragment;
	}

	void RemoveUses(const FFaerieItemProxy& Proxy, const ItemData::FRequireEntityManager& EntityManager, const int32 Cost)
	{
		const TOptional<FFaerieItemInstance> Item = Proxy->GetItemInstance();
		ItemData::FUsesHelper Uses(EntityManager, Item.GetValue());
		if (Uses.HasFragmentValue())
		{
			// Remove a usage.
			Uses.RemoveUses(Proxy, Cost);
		}
	}
}

using namespace Faerie;

bool UFaerieConsumableLogicBase::TestConsumable(const TConstStructView<FFaerieMassFragment>& Fragment,
	const FFaerieItemProxy& Proxy, const TNotNull<const AActor*> Consumer, const int32 Cost) const
{
	const ItemData::FRequireEntityManager EntityManager(Consumer);
	return Generation::CanRemoveUses(Proxy, EntityManager, Cost, true);
}
