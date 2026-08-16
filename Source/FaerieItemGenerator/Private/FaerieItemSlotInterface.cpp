// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemSlotInterface.h"
#include "FaerieItemContainerBase.h"
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

	template<bool LogFailure>
	bool ValidateFilledSlots(const FMassEntityManager* EntityManager, const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemCraftingSlots& Slots)
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

		return ForEachCraftingSlot(Slots, [&EntityManager, &FilledSlots](const FFaerieItemCraftingCostElement& Slot)
			{
				if (const FFaerieItemProxy* ItemProxy = FilledSlots.Slots.Find(Slot.Name))
				{
					const UObject* ProxyObj = ItemProxy->GetProxyObject();
					if (!IsValid(ProxyObj))
					{
						if constexpr (LogFailure)
						{
							UE_LOG(LogItemGeneration, Warning, TEXT("ValidateFilledSlots: Proxy is invalid for slot: %s!"),
                            	*Slot.Name.ToString());
						}
						return false;
					}

					if (!Slot.Template->TryMatch(EntityManager, *ItemProxy))
					{
						if constexpr (LogFailure)
						{
							UE_LOG(LogItemGeneration, Warning, TEXT("ValidateFilledSlots: Slot '%s' failed with key: %s"),
								  *Slot.Name.ToString(), *ItemProxy->GetProxyObject()->GetName());
						}
						return false;
					}

					if (Slot.PayInConsumableUses)
					{
						if (!ItemProxy->GetItemInstance().GetValue().IsMutable())
						{
							if constexpr (LogFailure)
							{
								UE_LOG(LogItemGeneration, Warning, TEXT("ValidateFilledSlots: Slot '%s' cannot pay consumable cost with immutable item: %s"),
								   *Slot.Name.ToString(), *ItemProxy->GetProxyObject()->GetName());
							}
							return false;
						}

						const ItemData::FUsesHelper Uses(*EntityManager, ItemProxy->GetItemInstance().GetValue());

						if (!Uses.HasUsesRemaining(Slot.Amount))
						{
							if constexpr (LogFailure)
							{
								UE_LOG(LogItemGeneration, Warning, TEXT("ValidateFilledSlots: Slot '%s' insufficient uses to pay cost with item: %s"),
								   *Slot.Name.ToString(), *ProxyObj->GetName());
							}
							return false;
						}
					}
					else
					{
						if (ItemProxy->GetCopies() < Slot.Amount)
						{
							if constexpr (LogFailure)
							{
								UE_LOG(LogItemGeneration, Warning, TEXT("ValidateFilledSlots: Slot '%s' insufficient uses to pay cost with item: %s"),
								   *Slot.Name.ToString(), *ProxyObj->GetName());
							}
							return false;
						}
					}
				}
				else
				{
					if (!Slot.Optional)
					{
						if constexpr (LogFailure)
						{
							UE_LOG(LogItemGeneration, Warning, TEXT("ValidateFilledSlots: Does not contain required slot: %s!"),
							   *Slot.Name.ToString());
						}
						return false;
					}
				}

				// All slots validated!
				return true;
			});
	}

	// Instantiate this function
	template FAERIEITEMGENERATOR_API bool ValidateFilledSlots<true>(const FMassEntityManager*, const FFaerieCraftingFilledSlots&, const FFaerieItemCraftingSlots&);
	template FAERIEITEMGENERATOR_API bool ValidateFilledSlots<false>(const FMassEntityManager*, const FFaerieCraftingFilledSlots&, const FFaerieItemCraftingSlots&);

	bool ConsumeSlotCosts(FMassEntityManager& EntityManager, const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemCraftingSlots& Slots)
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

			const TOptional<FFaerieItemInstance> InstanceOpt = SlotPayment.GetItemInstance();
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
                		return true;
                	}
                	return false;
                }
			}
			else
			{
				if (UFaerieItemContainerBase* Owner = Cast<UFaerieItemContainerBase>(SlotPayment.GetItemOwner()))
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