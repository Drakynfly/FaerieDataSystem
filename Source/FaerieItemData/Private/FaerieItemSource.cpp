// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemSource.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemSource)

FFaerieUnownedItemStack Faerie::ItemData::FGetInstanceResult::WithInitialization(FMassEntityManager& EntityManager) const
{
	// Setup new instance for runtime.
	FFaerieUnownedItemStack ItemStack = Stack.GetValue();
	ItemStack.Instance.InitializeMassEntityIfInvalid(EntityManager);
	return ItemStack;
}

const FName IFaerieItemSource::MutableSourceTag(TEXT("MutableSource"));
