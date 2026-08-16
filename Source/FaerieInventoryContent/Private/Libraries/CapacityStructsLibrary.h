// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Capacity/CapacityStructs.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CapacityStructsLibrary.generated.h"

struct FFaerieItemProxy;

/**
 *
 */
UCLASS()
class UFaerieCapacityStructsUtilities : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Faerie|Capacity")
	static FFaerieItemCapacity GetCapacity(const FFaerieItemProxy& Proxy);

	UFUNCTION(BlueprintPure, Category = "Faerie|Capacity")
	static int32 GetWeightOfStack(const FFaerieItemProxy& Proxy, const int32 Stack);

	// Gets the volume of an entire stack. Volume == X + (X * (Stack - 1) * Efficiency)
	UFUNCTION(BlueprintPure, Category = "Faerie|Capacity")
	static int64 GetVolumeOfStack(const FFaerieItemProxy& Proxy, const int32 Stack);

	// Gets the volume of a partial stack. Volume == X * Stack * Efficiency
	UFUNCTION(BlueprintPure, Category = "Faerie|Capacity")
	static int64 GetEfficientVolume(const FFaerieItemProxy& Proxy, const int32 Stack);

	// Gets the weight and volume of an entire stack.
	UFUNCTION(BlueprintPure, Category = "Faerie|Capacity")
	static FFaerieWeightAndVolume GetWeightAndVolumeOfStack(const FFaerieItemProxy& Proxy, const int32 Stack);

	// Gets the weight and volume for a portion of a stack. Uses EfficientVolume rather than full volume.
	UFUNCTION(BlueprintPure, Category = "Faerie|Capacity")
	static FFaerieWeightAndVolume GetWeightAndVolumeOfPartialStack(const FFaerieItemProxy& Proxy, const int32 Stack);

	UFUNCTION(BlueprintPure, Category = "Faerie|Capacity")
	static FFaerieItemCapacity WeightOfScaledComparison(const FFaerieItemCapacity& Original, const FFaerieItemCapacity& Comparison);

	UFUNCTION(BlueprintPure, Category = "Faerie|Capacity", meta = (DisplayName = "WeightAndVolume + WeightAndVolume", CompactNodeTitle = "+", ScriptMethod = "Add",
		ScriptOperator = "+;+=", Keywords = "+ add plus", CommutativeAssociativeBinaryOperator = "true"))
	static FFaerieWeightAndVolume Add_WeightAndVolume(const FFaerieWeightAndVolume& A, const FFaerieWeightAndVolume& B) { return A + B; }

	UFUNCTION(BlueprintPure, Category = "Faerie|Capacity", meta = (DisplayName = "WeightAndVolume - WeightAndVolume", CompactNodeTitle = "-", ScriptMethod = "Subtract",
		ScriptOperator = "-;-=", Keywords = "- subtract minus", CommutativeAssociativeBinaryOperator = "true"))
	static FFaerieWeightAndVolume Subtract_WeightAndVolume(const FFaerieWeightAndVolume& A, const FFaerieWeightAndVolume& B) { return A - B; }

	UFUNCTION(BlueprintPure, Category = "Faerie|Capacity")
	static FFaerieWeightAndVolume ToWeightAndVolume_ItemCapacity(const FFaerieItemCapacity& ItemCapacity);
};