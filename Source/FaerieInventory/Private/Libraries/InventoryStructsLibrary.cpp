// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "InventoryStructsLibrary.h"
#include "FaerieItemStorage.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryStructsLibrary)

FString UInventoryStructsLibrary::ToString_Address(const FFaerieAddress Address)
{
	auto Keys = UFaerieItemStorage::BreakAddress(Address);
	return Keys.Get<0>().ToString() + TEXT(":") + Keys.Get<1>().ToString();
}

bool UInventoryStructsLibrary::EqualEqual_StackKey(const FStackKey A, const FStackKey B)
{
	return A == B;
}