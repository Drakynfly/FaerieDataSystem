// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "FaerieGridLibrary.generated.h"

enum class EFaerieSpatialItemRotation : uint8;
struct FFaerieGridShape;

/**
 *
 */
UCLASS()
class UFaerieGridLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Faerie|GridLibrary")
	static FFaerieGridShape RotateShape(const FFaerieGridShape& InShape, EFaerieSpatialItemRotation Rotation);

	UFUNCTION(BlueprintPure, Category = "Faerie|GridLibrary")
	static FIntPoint GetSize(const FFaerieGridShape& InShape);
};