// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "CraftingLibrary.h"
#include "FaerieItemSlotInterface.h"
#include "Generation/FaerieItemGenerationConfig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CraftingLibrary)

UFaerieItemGenerationConfig* UFaerieCraftingLibrary::CreateGenerationDriver(const TArray<FFaerieWeightedDrop>& DropList, const FFaerieGeneratorAmountBase& Amount)
{
    UFaerieItemGenerationConfig* NewDriver = NewObject<UFaerieItemGenerationConfig>();
    NewDriver->DropPool.DropList = DropList;
    NewDriver->AmountResolver = TInstancedStruct<FFaerieGeneratorAmountBase>::Make(Amount);
    return NewDriver;
}

void UFaerieCraftingLibrary::GetCraftingSlots(const TScriptInterface<IFaerieItemSlotInterface> Interface, FFaerieItemCraftingSlots& Slots)
{
    Slots = FFaerieItemCraftingSlots();

    if (Interface.GetInterface())
    {
        if (const FFaerieCraftingSlotsView SlotsView = Interface->GetCraftingSlots();
            SlotsView.IsValid())
        {
            Slots = SlotsView.Get();
        }
    }
}

void UFaerieCraftingLibrary::GetCraftingSlots_Message(UObject* Object, FFaerieItemCraftingSlots& Slots)
{
    if (Object && Object->Implements<UFaerieItemSlotInterface>())
    {
        GetCraftingSlots(TScriptInterface<IFaerieItemSlotInterface>(Object), Slots);
    }
}

bool UFaerieCraftingLibrary::IsSlotOptional(const TScriptInterface<IFaerieItemSlotInterface> Interface, const FFaerieItemSlotHandle& Name)
{
    return Faerie::Crafting::IsSlotOptional(Interface.GetInterface(), Name);
}

bool UFaerieCraftingLibrary::FindSlot(const TScriptInterface<IFaerieItemSlotInterface> Interface,
                                      const FFaerieItemSlotHandle& Name, UFaerieItemTemplate*& OutSlot)
{
    return Faerie::Crafting::FindSlot(Interface.GetInterface(), Name, OutSlot);
}