// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Recipes/FaerieRecipeCraftRequest.h"
#include "FaerieItem.h"
#include "FaerieItemGenerationLog.h"
#include "FaerieItemRecipe.h"
#include "FaerieItemSource.h"
#include "FaerieItemStackView.h"
#include "FaerieItemTemplate.h"
#include "ItemInstancingContext_Crafting.h"
#include "Recipes/FaerieRecipeCraftConfig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieRecipeCraftRequest)

bool FFaerieRecipeCraftRequest::Configure(UFaerieCraftingRunner* Runner) const
{
	if (!IsValid(Config))
	{
		UE_LOG(LogItemGeneration, Warning, TEXT("%hs: Config is invalid!"), __FUNCTION__);
		return false;
	}

	FFaerieCraftingActionSlots SlotMemory;

	if (const FFaerieCraftingSlotsView SlotsView = Faerie::Crafting::GetCraftingSlots(Config);
		SlotsView.IsValid())
	{
		for (auto&& RequiredSlot : SlotsView.Get().RequiredSlots)
		{
			if (auto&& SlotPtr = Slots.FindByPredicate(
				[RequiredSlot](const FFaerieCraftingRequestSlot& Slot)
				{
					return Slot.SlotID == RequiredSlot.Key;
				}))
			{
				if (!IsValid(SlotPtr->ItemProxy.GetObject()))
				{
					UE_LOG(LogItemGeneration, Warning, TEXT("%hs: Entry is invalid for slot: %s!"),
						__FUNCTION__, *RequiredSlot.Key.ToString());
					return false;
				}

				if (RequiredSlot.Value->TryMatch(SlotPtr->ItemProxy))
				{
					SlotMemory.FilledSlots.Add(RequiredSlot.Key, SlotPtr->ItemProxy);
				}
				else
				{
					UE_LOG(LogItemGeneration, Warning, TEXT("%hs: Required Slot '%s' failed with key: %s"),
						__FUNCTION__, *SlotPtr->SlotID.ToString(), *SlotPtr->ItemProxy.GetObject()->GetName());
					return false;
				}
			}
			else
			{
				UE_LOG(LogItemGeneration, Warning, TEXT("%hs: Request does contain required slot: %s!"),
					__FUNCTION__, *RequiredSlot.Key.ToString());
				return false;
			}
		}

		for (auto&& OptionalSlot : SlotsView.Get().OptionalSlots)
		{
			if (auto&& SlotPtr = Slots.FindByPredicate(
				[OptionalSlot](const FFaerieCraftingRequestSlot& Slot)
				{
					return Slot.SlotID == OptionalSlot.Key;
				}))
			{
				if (!IsValid(SlotPtr->ItemProxy.GetObject()))
				{
					UE_LOG(LogItemGeneration, Warning, TEXT("%hs: Entry is invalid for slot: %s!"),
						__FUNCTION__, *OptionalSlot.Key.ToString());
					return false;
				}

				if (OptionalSlot.Value->TryMatch(SlotPtr->ItemProxy))
				{
					SlotMemory.FilledSlots.Add(OptionalSlot.Key, SlotPtr->ItemProxy);
				}
				else
				{
					UE_LOG(LogItemGeneration, Warning, TEXT("%hs: Optional Slot '%s' failed with key: %s"),
						__FUNCTION__, *SlotPtr->SlotID.ToString(), *SlotPtr->ItemProxy.GetObject()->GetName());
					return false;
				}
			}
		}
	}

	Runner->RequestStorage.InitializeAs<FFaerieCraftingActionSlots>(SlotMemory);

	return true;
}

void FFaerieRecipeCraftRequest::Run(UFaerieCraftingRunner* Runner) const
{
	const FFaerieCraftingActionSlots& SlotMemory = Runner->RequestStorage.Get<FFaerieCraftingActionSlots>();

	// Execute parent Run, as it validates some stuff, and then early out if it fails.
	for (auto&& Element : SlotMemory.FilledSlots)
	{
		if (!Element.Value.IsValid() ||
			!IsValid(Element.Value->GetItemObject()) ||
			!Element.Value.IsInstanceMutable())
		{
			UE_LOG(LogItemGeneration, Error, TEXT("A filled slot [%s] is invalid!)"), *Element.Key.ToString())
			Runner->Fail();
		}
	}

	UE_LOG(LogItemGeneration, Log, TEXT("Running CraftEntries"));

	if (!ensure(IsValid(Config->Recipe)))
	{
		return;
	}

	FFaerieItemInstancingContext_Crafting Context;
	Context.Squirrel = Squirrel.Get();
	Context.InputEntryData = SlotMemory.FilledSlots;

	const UFaerieItem* NewItem = Config->Recipe->GetItemSource()->CreateItemInstance(&Context);
	if (!IsValid(NewItem))
	{
		UE_LOG(LogItemGeneration, Error, TEXT("Item Instancing failed for Craft Item!"));
		return Runner->Fail();
	}

	Runner->ProcessStacks.Add({NewItem, 1});

	if (RunConsumeStep && Config->Recipe->Implements<UFaerieItemSlotInterface>())
	{
		Faerie::ConsumeSlotCosts(SlotMemory.FilledSlots, Cast<IFaerieItemSlotInterface>(Config->Recipe));
	}

	Runner->Complete();
}