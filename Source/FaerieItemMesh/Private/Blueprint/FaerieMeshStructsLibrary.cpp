// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieMeshStructsLibrary.h"
#include "FaerieMeshStructs.h"
#include "Materials/MaterialInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieMeshStructsLibrary)

TArray<UMaterialInterface*> UFaerieMeshStructsLibrary::FaerieItemMaterialsToObjectArray(
	const TArray<FFaerieItemMaterial>& Materials)
{
	TArray<UMaterialInterface*> OutArray;

	for (const FFaerieItemMaterial& ItemMaterial : Materials)
	{
		OutArray.Add(ItemMaterial.Material);
	}

	return OutArray;
}

TArray<TSoftObjectPtr<UMaterialInterface>> UFaerieMeshStructsLibrary::FaerieItemMaterialsToSoftObjectArray(
	const TArray<FFaerieItemMaterial>& Materials)
{
	TArray<TSoftObjectPtr<UMaterialInterface>> OutArray;

	for (const FFaerieItemMaterial& ItemMaterial : Materials)
	{
		OutArray.Add(ItemMaterial.Material);
	}

	return OutArray;
}