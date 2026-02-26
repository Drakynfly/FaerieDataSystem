// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemSlotInterface.h"
#include "FaerieItemGenerationLog.h"
#include "FaerieItemProxy.h"
#include "FaerieItemStackView.h"
#include "FaerieItemTemplate.h"
#include "Tokens/FaerieItemUsesToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemSlotInterface)

namespace Faerie::Generation
{
	bool ValidateFilledSlots(const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemCraftingSlots& CraftingSlots)
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

		for (auto&& RequiredSlot : CraftingSlots.RequiredSlots)
		{
			if (auto&& ItemProxy = FilledSlots.Slots.Find(RequiredSlot.Name))
			{
				if (!IsValid(ItemProxy->GetObject()))
				{
					UE_LOG(LogItemGeneration, Warning, TEXT("ValidateFilledSlots: Proxy is invalid for slot: %s!"),
						*RequiredSlot.Name.ToString());
					return false;
				}

				if (!RequiredSlot.Template->TryMatch(FFaerieItemStackView(*ItemProxy)))
				{
					UE_LOG(LogItemGeneration, Warning, TEXT("ValidateFilledSlots: Slot '%s' failed with key: %s"),
						   *RequiredSlot.Name.ToString(), *ItemProxy->GetObject()->GetName());
					return false;
				}

				if (RequiredSlot.PayInConsumableUses)
				{
					if (!ItemProxy->GetItemObject()->IsDataMutable())
					{
						UE_LOG(LogItemGeneration, Warning, TEXT("ValidateFilledSlots: Slot '%s' cannot pay consumable cost with immutable item: %s"),
							*RequiredSlot.Name.ToString(), *ItemProxy->GetObject()->GetName());
						return false;
					}

					const UFaerieItemUsesToken* ItemUses = ItemProxy->GetItemObject()->GetToken<UFaerieItemUsesToken>();
					if (!ItemUses->HasUses(RequiredSlot.Amount))
					{
						UE_LOG(LogItemGeneration, Warning, TEXT("ValidateFilledSlots: Slot '%s' insufficient uses to pay cost with item: %s"),
							*RequiredSlot.Name.ToString(), *ItemProxy->GetObject()->GetName());
						return false;
					}
				}
				else
				{
					if (ItemProxy->GetCopies() < RequiredSlot.Amount)
					{
						UE_LOG(LogItemGeneration, Warning, TEXT("ValidateFilledSlots: Slot '%s' insufficient uses to pay cost with item: %s"),
							*RequiredSlot.Name.ToString(), *ItemProxy->GetObject()->GetName());
						return false;
					}
				}
			}
			else
			{
				if (!RequiredSlot.Optional)
				{
					UE_LOG(LogItemGeneration, Warning, TEXT("ValidateFilledSlots: Does not contain required slot: %s!"),
						*RequiredSlot.Name.ToString());
					return false;
				}
			}
		}

		return true;
	}

	bool ConsumeSlotCosts(const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemCraftingSlots& CraftingSlots)
	{
		for (auto&& Slot : CraftingSlots.RequiredSlots)
		{
			auto&& SlotPaymentPtr = FilledSlots.Slots.Find(Slot.Name);
			if (!ensure(SlotPaymentPtr))
			{
				UE_LOG(LogItemGeneration, Error, TEXT("ConsumeSlotCosts is unable to find a filled slot [%s]!"), *Slot.Name.ToString())
				return false;
			}

			const FFaerieItemProxy& SlotPayment = *SlotPaymentPtr;

			const UFaerieItem* Item = SlotPayment->GetItemObject();
			if (!ensure(IsValid(Item)))
			{
				return false;
			}

			if (Slot.PayInConsumableUses)
			{
                // If the item can be used as a resource multiple times.
                if (UFaerieItem* Mutable = Item->MutateCast())
                {
                	if (auto&& Uses = Mutable->GetMutableToken<UFaerieItemUsesToken>())
                	{
                		if (Uses->HasUses(Slot.Amount))
                		{
                			Uses->RemoveUses(Slot.Amount);
                		}
                		return false;
                	}
                }
			}
			else
			{
                (void)SlotPayment->Release(Slot.Amount);
			}
		}

		return true;
	}

	const FFaerieItemCraftingCostElement* FindSlot(const TNotNull<const IFaerieItemSlotInterface*> Interface, const FFaerieItemSlotHandle& Name)
	{
		const FFaerieItemCraftingSlots Slots = Interface->GetCraftingSlots();
		if (const FFaerieItemCraftingCostElement* Slot = Slots.RequiredSlots.FindByKey(Name))
		{
			return Slot;
		}

		return nullptr;
	}

	bool IsSlotOptional(const TNotNull<const IFaerieItemSlotInterface*> Interface, const FFaerieItemSlotHandle& Name)
	{
		if (const FFaerieItemCraftingCostElement* Slot = FindSlot(Interface, Name))
		{
			return Slot->Optional;
		}

		return false;
	}
}