// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "FaerieMeshStructsLibrary.generated.h"

struct FFaerieItemMaterial;
class UMaterialInterface;

/**
 *
 */
UCLASS()
class UFaerieMeshStructsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Faerie|Inventory|Utils")
	static TArray<UMaterialInterface*> FaerieItemMaterialsToObjectArray(const TArray<FFaerieItemMaterial>& Materials);

	UFUNCTION(BlueprintPure, Category = "Faerie|Inventory|Utils")
	static TArray<TSoftObjectPtr<UMaterialInterface>> FaerieItemMaterialsToSoftObjectArray(const TArray<FFaerieItemMaterial>& Materials);
};