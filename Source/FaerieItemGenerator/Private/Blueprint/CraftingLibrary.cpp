// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "CraftingLibrary.h"
#include "EntityManagerHelpers.h"
#include "FaerieItem.h"
#include "FaerieItemSlotInterface.h"

#include "Consumable/FaerieConsumableFragment.h"
#include "Generation/FaerieItemGenerationConfig.h"

#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CraftingLibrary)

void UFaerieGenerationLibrary::GetCraftingSlots(const TScriptInterface<IFaerieItemSlotInterface> Interface, FFaerieItemCraftingSlots& Slots)
{
    if (!Interface.GetInterface())
    {
        FFrame::KismetExecutionMessage(TEXT("Invalid Interface passed to UFaerieGenerationLibrary::TestCraftingSlots"), ELogVerbosity::Error);
        return;
    }

    Slots = FFaerieItemCraftingSlots();

    if (const IFaerieItemSlotInterface* InterfacePtr = Interface.GetInterface())
    {
       Slots = *InterfacePtr->GetCraftingSlots();
    }
}

void UFaerieGenerationLibrary::GetCraftingSlots_Message(UObject* Object, FFaerieItemCraftingSlots& Slots)
{
    if (Object && Object->Implements<UFaerieItemSlotInterface>())
    {
        GetCraftingSlots(TScriptInterface<IFaerieItemSlotInterface>(Object), Slots);
    }
}

bool UFaerieGenerationLibrary::TestCraftingSlots(const UObject* WorldContextObj, const TScriptInterface<IFaerieItemSlotInterface> Interface,
    const FFaerieCraftingFilledSlots& FilledSlots)
{
    if (!IsValid(WorldContextObj))
    {
        FFrame::KismetExecutionMessage(TEXT("Invalid WorldContextObj passed to UFaerieGenerationLibrary::TestCraftingSlots"), ELogVerbosity::Error);
        return false;
    }

    if (!Interface.GetInterface())
    {
        FFrame::KismetExecutionMessage(TEXT("Invalid Interface passed to UFaerieGenerationLibrary::TestCraftingSlots"), ELogVerbosity::Error);
        return false;
    }

    if (const IFaerieItemSlotInterface* InterfacePtr = Interface.GetInterface())
    {
        if (const FFaerieItemCraftingSlots* SlotsPtr = InterfacePtr->GetCraftingSlots())
        {
            return Faerie::Generation::ValidateFilledSlots(WorldContextObj, FilledSlots, *SlotsPtr);
        }
    }
    return false;
}

bool UFaerieGenerationLibrary::ConsumeSlotCosts(const UObject* WorldContextObj, const FFaerieCraftingFilledSlots& FilledSlots,
    const TScriptInterface<IFaerieItemSlotInterface>& CraftingSlots)
{
    if (!IsValid(WorldContextObj))
    {
        FFrame::KismetExecutionMessage(TEXT("Invalid WorldContextObj passed to UFaerieGenerationLibrary::ConsumeSlotCosts"), ELogVerbosity::Error);
        return false;
    }

    if (!CraftingSlots.GetInterface())
    {
        FFrame::KismetExecutionMessage(TEXT("Invalid Interface passed to UFaerieGenerationLibrary::ConsumeSlotCosts"), ELogVerbosity::Error);
        return false;
    }

    if (const IFaerieItemSlotInterface* InterfacePtr = CraftingSlots.GetInterface())
    {
        if (const FFaerieItemCraftingSlots* SlotsPtr = InterfacePtr->GetCraftingSlots())
        {
            Faerie::ItemData::FOptionalEntityManager EntityManager(WorldContextObj);
            return Faerie::Generation::ConsumeSlotCosts(EntityManager, FilledSlots, *SlotsPtr);
        }
    }
    return false;
}

bool UFaerieGenerationLibrary::IsSlotOptional(const TScriptInterface<IFaerieItemSlotInterface> Interface, const FFaerieItemSlotHandle& Name)
{
    if (!Interface.GetInterface())
    {
        FFrame::KismetExecutionMessage(TEXT("Invalid Interface passed to UFaerieGenerationLibrary::TestCraftingSlots"), ELogVerbosity::Error);
        return false;
    }

    if (const IFaerieItemSlotInterface* InterfacePtr = Interface.GetInterface())
    {
        if (const FFaerieItemCraftingSlots* SlotsPtr = InterfacePtr->GetCraftingSlots())
        {
            return Faerie::Generation::IsSlotOptional(*SlotsPtr, Name);
        }
    }
    return false;
}

bool UFaerieGenerationLibrary::FindSlot(const TScriptInterface<IFaerieItemSlotInterface> Interface,
                                      const FFaerieItemSlotHandle& Name, FFaerieItemCraftingCostElement& OutSlot)
{
    if (!Interface.GetInterface())
    {
        FFrame::KismetExecutionMessage(TEXT("Invalid Interface passed to UFaerieGenerationLibrary::TestCraftingSlots"), ELogVerbosity::Error);
        return false;
    }

    if (const IFaerieItemSlotInterface* InterfacePtr = Interface.GetInterface())
    {
        if (const FFaerieItemCraftingSlots* SlotsPtr = InterfacePtr->GetCraftingSlots())
        {
            if (const FFaerieItemCraftingCostElement* Slot = Faerie::Generation::FindSlot(*SlotsPtr, Name))
            {
                OutSlot = *Slot;
                return true;
            }
        }
    }
    return false;
}

bool UFaerieGenerationLibrary::CanConsume(const FFaerieItemProxy& Proxy, UScriptStruct* ConsumableType,
    const AActor* Consumer, const int32 Cost)
{
    if (!Proxy.IsValid())
    {
        FFrame::KismetExecutionMessage(TEXT("Invalid Proxy passed to UFaerieGenerationLibrary::CanConsume"), ELogVerbosity::Error);
        return false;
    }

    if (!IsValid(ConsumableType))
    {
        FFrame::KismetExecutionMessage(TEXT("Invalid ConsumableType passed to UFaerieGenerationLibrary::CanConsume"), ELogVerbosity::Error);
        return false;
    }

    if (!IsValid(Consumer))
    {
        FFrame::KismetExecutionMessage(TEXT("Invalid Consumer passed to UFaerieGenerationLibrary::CanConsume"), ELogVerbosity::Error);
        return false;
    }

    return Faerie::Generation::CanConsume(Proxy, ConsumableType, Consumer, Cost);
}

bool UFaerieGenerationLibrary::TryConsume(const FFaerieItemProxy& Proxy, UScriptStruct* ConsumableType,
    AActor* Consumer, const int32 Cost)
{
    if (!Proxy.IsValid())
    {
        FFrame::KismetExecutionMessage(TEXT("Invalid Proxy passed to UFaerieGenerationLibrary::TryConsume"), ELogVerbosity::Error);
        return false;
    }

    if (!IsValid(ConsumableType))
    {
        FFrame::KismetExecutionMessage(TEXT("Invalid ConsumableType passed to UFaerieGenerationLibrary::TryConsume"), ELogVerbosity::Error);
        return false;
    }

    if (!IsValid(Consumer))
    {
        FFrame::KismetExecutionMessage(TEXT("Invalid Consumer passed to UFaerieGenerationLibrary::TryConsume"), ELogVerbosity::Error);
        return false;
    }

    return Faerie::Generation::TryConsume(Proxy, ConsumableType, Consumer, Cost);
}
