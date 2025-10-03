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

void FFaerieRecipeCraftRequest::Run(UFaerieCraftingRunner* Runner) const
{
	if (!IsValid(Config))
	{
		UE_LOG(LogItemGeneration, Warning, TEXT("%hs: Config is invalid!"), __FUNCTION__);
		return Runner->Fail();
	}

	FFaerieCraftingActionSlots RequestData;

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
					return Runner->Fail();
				}

				if (RequiredSlot.Value->TryMatch(FFaerieItemStackView(SlotPtr->ItemProxy)))
				{
					RequestData.FilledSlots.Add(RequiredSlot.Key, SlotPtr->ItemProxy);
				}
				else
				{
					UE_LOG(LogItemGeneration, Warning, TEXT("%hs: Required Slot '%s' failed with key: %s"),
						__FUNCTION__, *SlotPtr->SlotID.ToString(), *SlotPtr->ItemProxy.GetObject()->GetName());
					return Runner->Fail();
				}
			}
			else
			{
				UE_LOG(LogItemGeneration, Warning, TEXT("%hs: Request does contain required slot: %s!"),
					__FUNCTION__, *RequiredSlot.Key.ToString());
				return Runner->Fail();
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
					return Runner->Fail();
				}

				if (OptionalSlot.Value->TryMatch(FFaerieItemStackView(SlotPtr->ItemProxy)))
				{
					RequestData.FilledSlots.Add(OptionalSlot.Key, SlotPtr->ItemProxy);
				}
				else
				{
					UE_LOG(LogItemGeneration, Warning, TEXT("%hs: Optional Slot '%s' failed with key: %s"),
						__FUNCTION__, *SlotPtr->SlotID.ToString(), *SlotPtr->ItemProxy.GetObject()->GetName());
					return Runner->Fail();
				}
			}
		}
	}

	// Validation
	for (auto&& Element : RequestData.FilledSlots)
	{
		if (!Element.Value.IsValid() ||
			!IsValid(Element.Value->GetItemObject()) ||
			!Element.Value.IsInstanceMutable())
		{
			UE_LOG(LogItemGeneration, Error, TEXT("A filled slot [%s] is invalid!)"), *Element.Key.ToString())
			Runner->Fail();
		}
	}

	UE_LOG(LogItemGeneration, Log, TEXT("Running RecipeCraft"));

	if (!ensure(IsValid(Config->Recipe)))
	{
		return;
	}

	FFaerieItemInstancingContext_Crafting Context;
	Context.Squirrel = Squirrel.Get();
	Context.InputEntryData = RequestData.FilledSlots;

	const UFaerieItem* NewItem = Config->Recipe->GetItemSource()->CreateItemInstance(&Context);
	if (!IsValid(NewItem))
	{
		UE_LOG(LogItemGeneration, Error, TEXT("Item Instancing failed for Craft Item!"));
		return Runner->Fail();
	}

	RequestData.ProcessStacks.Add({NewItem, 1});

	if (RunConsumeStep && Config->Recipe->Implements<UFaerieItemSlotInterface>())
	{
		Faerie::ConsumeSlotCosts(RequestData.FilledSlots, Cast<IFaerieItemSlotInterface>(Config->Recipe));
	}

	Runner->RequestStorage.InitializeAs<FFaerieCraftingActionSlots>(RequestData);

	Runner->Complete();
}