// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemSlotInterface.h"
#include "EntityManagerHelpers.h"
#include "FaerieItem.h"
#include "FaerieItemDataView.h"
#include "FaerieItemGenerationLog.h"
#include "FaerieItemOwnerInterface.h"
#include "FaerieItemProxy.h"
#include "FaerieItemTemplate.h"

#include "Consumable/FaerieItemUsesFragment.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemSlotInterface)

namespace Faerie::Generation
{
	bool ForEachCraftingSlot(const FFaerieItemCraftingSlots& Slots, const TFunctionRef<bool(const FFaerieItemCraftingCostElement& Slot)>& Predicate)
	{
		for (const FFaerieItemCraftingCostElement& Element : Slots.Slots)
		{
			if (!Predicate(Element))
			{
				return false;
			}
		}
		return true;
	}

	bool ValidateFilledSlots(const TNotNull<const UObject*> WorldContext, const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemCraftingSlots& Slots)
	{
		// Validation
		for (auto&& Element : FilledSlots.Slots)
		{
			if (!Element.Value.IsValid())
			{
				UE_LOG(LogItemGeneration, Error, TEXT("ValidateFilledSlots: A filled slot [%s] is invalid!)"), *Element.Key.ToString())
				return false;
			}
		}

		ItemData::FOptionalEntityManager EntityManager(WorldContext);

		return ForEachCraftingSlot(Slots, [&EntityManager, &FilledSlots, WorldContext](const FFaerieItemCraftingCostElement& Slot)
			{
				if (const FFaerieItemProxy* ItemProxy = FilledSlots.Slots.Find(Slot.Name))
				{
					const UObject* ProxyObj = ItemProxy->GetProxyObject();
					if (!IsValid(ProxyObj))
					{
						UE_LOG(LogItemGeneration, Warning, TEXT("ValidateFilledSlots: Proxy is invalid for slot: %s!"),
							*Slot.Name.ToString());
						return false;
					}

					const FFaerieItemDataView DataView(*ItemProxy);

					if (!Slot.Template->TryMatch(WorldContext, DataView))
					{
						UE_LOG(LogItemGeneration, Warning, TEXT("ValidateFilledSlots: Slot '%s' failed with key: %s"),
							   *Slot.Name.ToString(), *ItemProxy->GetProxyObject()->GetName());
						return false;
					}

					if (Slot.PayInConsumableUses)
					{
						if (!DataView.GetInstance().IsMutable())
						{
							UE_LOG(LogItemGeneration, Warning, TEXT("ValidateFilledSlots: Slot '%s' cannot pay consumable cost with immutable item: %s"),
								*Slot.Name.ToString(), *ItemProxy->GetProxyObject()->GetName());
							return false;
						}

						const ItemData::FUsesHelper Uses(EntityManager, DataView.GetInstance());
						if (!Uses.HasUsesRemaining(Slot.Amount))
						{
							UE_LOG(LogItemGeneration, Warning, TEXT("ValidateFilledSlots: Slot '%s' insufficient uses to pay cost with item: %s"),
								*Slot.Name.ToString(), *ProxyObj->GetName());
							return false;
						}
					}
					else
					{
						if (DataView.GetCopies() < Slot.Amount)
						{
							UE_LOG(LogItemGeneration, Warning, TEXT("ValidateFilledSlots: Slot '%s' insufficient uses to pay cost with item: %s"),
								*Slot.Name.ToString(), *ProxyObj->GetName());
							return false;
						}
					}
				}
				else
				{
					if (!Slot.Optional)
					{
						UE_LOG(LogItemGeneration, Warning, TEXT("ValidateFilledSlots: Does not contain required slot: %s!"),
							*Slot.Name.ToString());
						return false;
					}
				}

				// All slots validated!
				return true;
			});
	}

	bool ConsumeSlotCosts(const ItemData::FOptionalEntityManager& EntityManager, const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemCraftingSlots& Slots)
	{
		return ForEachCraftingSlot(Slots, [&EntityManager, &FilledSlots](const FFaerieItemCraftingCostElement& Slot)
		{
			auto&& SlotPaymentPtr = FilledSlots.Slots.Find(Slot.Name);
			if (!ensure(SlotPaymentPtr))
			{
				UE_LOG(LogItemGeneration, Error, TEXT("ConsumeSlotCosts is unable to find a filled slot [%s]!"), *Slot.Name.ToString())
				return false;
			}

			const FFaerieItemProxy& SlotPayment = *SlotPaymentPtr;

			const TOptional<FFaerieItemInstance> InstanceOpt = SlotPayment->GetItemInstance();
			if (!ensure(InstanceOpt.IsSet()))
			{
				return false;
			}

			if (Slot.PayInConsumableUses)
			{
				ItemData::FUsesHelper Uses(EntityManager, InstanceOpt.GetValue());

                // If the item can be used as a resource multiple times.
                {
                	if (Uses.HasUsesRemaining(Slot.Amount))
                	{
                		Uses.RemoveUses(SlotPayment, Slot.Amount);
                	}
                	return false;
                }
			}
			else
			{
				if (IFaerieItemOwnerInterface* Owner = SlotPayment->GetItemOwner())
				{
					Owner->DestroyStack(SlotPayment, Slot.Amount);
				}
			}

			// All slots consumed!
			return true;
		});
	}

	const FFaerieItemCraftingCostElement* FindSlot(const FFaerieItemCraftingSlots& Slots, const FFaerieItemSlotHandle& Name)
	{
		if (const FFaerieItemCraftingCostElement* Slot = Slots.Slots.FindByKey(Name))
		{
			return Slot;
		}

		return nullptr;
	}

	bool IsSlotOptional(const FFaerieItemCraftingSlots& Slots, const FFaerieItemSlotHandle& Name)
	{
		if (const FFaerieItemCraftingCostElement* Slot = FindSlot(Slots, Name))
		{
			return Slot->Optional;
		}

		return false;
	}
}