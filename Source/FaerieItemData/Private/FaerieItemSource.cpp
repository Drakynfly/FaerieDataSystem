// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemSource.h"
#include "EntityManagerHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemSource)

FFaerieUnownedItemStack Faerie::ItemData::FGetInstanceResult::WithInitialization() const
{
	// Setup new instance for runtime.
	FFaerieUnownedItemStack ItemStack = Stack.GetValue();
	ItemStack.Instance.InitializeMassEntityIfInvalid(GetFaerieEntityManagerChecked());
	return ItemStack;
}

const FName IFaerieItemSource::MutableSourceTag(TEXT("MutableSource"));
