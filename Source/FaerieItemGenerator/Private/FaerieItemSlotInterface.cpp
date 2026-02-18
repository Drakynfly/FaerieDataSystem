// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemSlotInterface.h"
#include "FaerieItemGenerationLog.h"
#include "FaerieItemProxy.h"
#include "FaerieItemStackView.h"
#include "FaerieItemTemplate.h"
#include "Algo/ForEach.h"
#include "Tokens/FaerieItemUsesToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemSlotInterface)

namespace Faerie::Generation
{
	bool ValidateFilledSlots(const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemCraftingSlots& CraftingSlots)
	{
		// Validation
		for (auto&& Element : FilledSlots.Slots)
		{
			if (!Element.Value.IsValid() ||
				!IsValid(Element.Value->GetItemObject()) ||
				!Element.Value.IsInstanceMutable())
			{
				UE_LOG(LogItemGeneration, Error, TEXT("A filled slot [%s] is invalid!)"), *Element.Key.ToString())
				return false;
			}
		}

		for (auto&& RequiredSlot : CraftingSlots.RequiredSlots)
		{
			if (auto&& ItemProxy = FilledSlots.Slots.Find(RequiredSlot.Key))
			{
				if (!IsValid(ItemProxy->GetObject()))
				{
					UE_LOG(LogItemGeneration, Warning, TEXT("%hs: Proxy is invalid for slot: %s!"),
						__FUNCTION__, *RequiredSlot.Key.ToString());
					return false;
				}

				if (!RequiredSlot.Value->TryMatch(FFaerieItemStackView(*ItemProxy)))
				{
					UE_LOG(LogItemGeneration, Warning, TEXT("%hs: Required Slot '%s' failed with key: %s"),
						   __FUNCTION__, *RequiredSlot.Key.ToString(), *ItemProxy->GetObject()->GetName());
					return false;
				}
			}
			else
			{
				UE_LOG(LogItemGeneration, Warning, TEXT("%hs: Request does not contain required slot: %s!"),
					__FUNCTION__, *RequiredSlot.Key.ToString());
				return false;
			}
		}

		for (auto&& OptionalSlot : CraftingSlots.OptionalSlots)
		{
			if (auto&& ItemProxy = FilledSlots.Slots.Find(OptionalSlot.Key))
			{
				if (!IsValid(ItemProxy->GetObject()))
				{
					UE_LOG(LogItemGeneration, Warning, TEXT("%hs: Entry is invalid for slot: %s!"),
						__FUNCTION__, *OptionalSlot.Key.ToString());
					return false;
				}

				if (!OptionalSlot.Value->TryMatch(FFaerieItemStackView(*ItemProxy)))
				{
					UE_LOG(LogItemGeneration, Warning, TEXT("%hs: Optional Slot '%s' failed with key: %s"),
						   __FUNCTION__, *OptionalSlot.Key.ToString(), *ItemProxy->GetObject()->GetName());
					return false;
				}
			}
		}

		return true;
	}

	void ConsumeSlotCosts(const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemCraftingSlots& CraftingSlots)
	{
		auto CanEat = [&FilledSlots](const TPair<FFaerieItemSlotHandle, TObjectPtr<UFaerieItemTemplate>>& Slot)
			{
				auto&& ItemProxy = *FilledSlots.Slots.Find(Slot.Key);

				if (!ensure(ItemProxy.IsValid()))
				{
					UE_LOG(LogItemGeneration, Error, TEXT("ConsumeSlotCosts is unable to find a filled slot [%s]!"), *Slot.Key.ToString())
					return false;
				}

				if (!ItemProxy.IsInstanceMutable())
				{
					UE_LOG(LogItemGeneration, Error, TEXT("ConsumeSlotCosts is unable to mutate the item in slot [%s]!"), *Slot.Key.ToString())
					return false;
				}

				return true;
			};

		auto EatUse = [&FilledSlots](const TPair<FFaerieItemSlotHandle, TObjectPtr<UFaerieItemTemplate>>& Slot)
			{
				const FFaerieItemProxy& ItemProxy = *FilledSlots.Slots.Find(Slot.Key);

				const UFaerieItem* Item = ItemProxy->GetItemObject();
				if (!ensure(IsValid(Item)))
				{
					return;
				}

				bool RemovedUse = false;

				// If the item can be used as a resource multiple times.
				if (UFaerieItem* Mutable = Item->MutateCast())
				{
					if (auto&& Uses = Mutable->GetMutableToken<UFaerieItemUsesToken>())
					{
						RemovedUse = Uses->RemoveUses(1);
					}
				}

				// Otherwise, consume the item itself
				if (!RemovedUse)
				{
					(void)ItemProxy->Release(1);
				}
			};

		Algo::ForEachIf(CraftingSlots.RequiredSlots, CanEat, EatUse);
		Algo::ForEachIf(CraftingSlots.OptionalSlots, CanEat, EatUse);
	}

	FFaerieCraftingSlotsView GetCraftingSlots(const IFaerieItemSlotInterface* Interface)
	{
		if (Interface != nullptr)
		{
			return Interface->GetCraftingSlots();
		}
		return FFaerieCraftingSlotsView();
	}

	bool IsSlotOptional(const IFaerieItemSlotInterface* Interface, const FFaerieItemSlotHandle& Name)
	{
		if (Interface == nullptr) return false;
		const FFaerieCraftingSlotsView SlotsView = GetCraftingSlots(Interface);
		return SlotsView.Get().OptionalSlots.Contains(Name);
	}

	bool FindSlot(const IFaerieItemSlotInterface* Interface, const FFaerieItemSlotHandle& Name, UFaerieItemTemplate*& OutSlot)
	{
		if (Interface == nullptr) return false;

		const FFaerieCraftingSlotsView SlotsView = GetCraftingSlots(Interface);
		const FFaerieItemCraftingSlots& SlotsPtr = SlotsView.Get();

		if (SlotsPtr.RequiredSlots.Contains(Name))
		{
			OutSlot = SlotsPtr.RequiredSlots[Name];
			return true;
		}

		if (SlotsPtr.OptionalSlots.Contains(Name))
		{
			OutSlot = SlotsPtr.OptionalSlots[Name];
			return true;
		}

		return false;
	}
}