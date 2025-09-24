// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "InventoryStructsLibrary.h"
#include "FaerieItemStorage.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryStructsLibrary)

FString UInventoryStructsLibrary::ToString_Address(const FFaerieAddress Address)
{
	return UFaerieItemStorage::FStorageKey(Address).ToString();
}

bool UInventoryStructsLibrary::EqualEqual_StackKey(const FStackKey A, const FStackKey B)
{
	return A == B;
}