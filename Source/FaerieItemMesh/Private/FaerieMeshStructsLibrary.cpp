﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieMeshStructsLibrary.h"
#include "FaerieMeshStructs.h"

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
