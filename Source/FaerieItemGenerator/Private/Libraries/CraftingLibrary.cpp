// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "CraftingLibrary.h"
#include "FaerieItemSlotInterface.h"
#include "Generation/FaerieItemGenerationConfig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CraftingLibrary)

void UFaerieCraftingLibrary::GetCraftingSlots(const TScriptInterface<IFaerieItemSlotInterface> Interface, FFaerieItemCraftingSlots& Slots)
{
    Slots = FFaerieItemCraftingSlots();

    if (Interface.GetInterface())
    {
       Slots = Interface->GetCraftingSlots();
    }
}

void UFaerieCraftingLibrary::GetCraftingSlots_Message(UObject* Object, FFaerieItemCraftingSlots& Slots)
{
    if (Object && Object->Implements<UFaerieItemSlotInterface>())
    {
        GetCraftingSlots(TScriptInterface<IFaerieItemSlotInterface>(Object), Slots);
    }
}

bool UFaerieCraftingLibrary::TestCraftingSlots(const TScriptInterface<IFaerieItemSlotInterface> Interface,
    const FFaerieCraftingFilledSlots& FilledSlots)
{
    if (Interface.GetInterface())
    {
        return Faerie::Generation::ValidateFilledSlots(FilledSlots, Interface->GetCraftingSlots());
    }
    return false;
}

bool UFaerieCraftingLibrary::IsSlotOptional(const TScriptInterface<IFaerieItemSlotInterface> Interface, const FFaerieItemSlotHandle& Name)
{
    if (const IFaerieItemSlotInterface* InterfacePtr = Interface.GetInterface())
    {
        return Faerie::Generation::IsSlotOptional(InterfacePtr, Name);
    }
    return false;
}

bool UFaerieCraftingLibrary::FindSlot(const TScriptInterface<IFaerieItemSlotInterface> Interface,
                                      const FFaerieItemSlotHandle& Name, FFaerieItemCraftingCostElement& OutSlot)
{
    if (const IFaerieItemSlotInterface* InterfacePtr = Interface.GetInterface())
    {
        if (const FFaerieItemCraftingCostElement* Slot = Faerie::Generation::FindSlot(InterfacePtr, Name))
        {
            OutSlot = *Slot;
            return true;
        }
    }
    return false;
}