// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Capacity/CapacityStructs.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CapacityStructsLibrary.generated.h"

struct FFaerieCapacityExtensionConfig;
struct FFaerieCapacityExtensionState;
struct FFaerieItemProxy;

/**
 *
 */
UCLASS()
class UFaerieCapacityStructsUtilities : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Faerie|Capacity", meta = (BlueprintThreadSafe))
	static FFaerieItemCapacity GetCapacity(const FFaerieItemProxy& Proxy);

	UFUNCTION(BlueprintPure, Category = "Faerie|Capacity", meta = (BlueprintThreadSafe))
	static int32 GetWeightOfStack(const FFaerieItemProxy& Proxy, const int32 Stack);

	// Gets the volume of an entire stack. Volume == X + (X * (Stack - 1) * Efficiency)
	UFUNCTION(BlueprintPure, Category = "Faerie|Capacity", meta = (BlueprintThreadSafe))
	static int64 GetVolumeOfStack(const FFaerieItemProxy& Proxy, const int32 Stack);

	// Gets the volume of a partial stack. Volume == X * Stack * Efficiency
	UFUNCTION(BlueprintPure, Category = "Faerie|Capacity", meta = (BlueprintThreadSafe))
	static int64 GetEfficientVolume(const FFaerieItemProxy& Proxy, const int32 Stack);

	// Gets the weight and volume of an entire stack.
	UFUNCTION(BlueprintPure, Category = "Faerie|Capacity", meta = (BlueprintThreadSafe))
	static FFaerieWeightAndVolume GetWeightAndVolumeOfStack(const FFaerieItemProxy& Proxy, const int32 Stack);

	// Gets the weight and volume for a portion of a stack. Uses EfficientVolume rather than full volume.
	UFUNCTION(BlueprintPure, Category = "Faerie|Capacity", meta = (BlueprintThreadSafe))
	static FFaerieWeightAndVolume GetWeightAndVolumeOfPartialStack(const FFaerieItemProxy& Proxy, const int32 Stack);

	UFUNCTION(BlueprintPure, Category = "Faerie|Capacity", meta = (BlueprintThreadSafe))
	static FFaerieItemCapacity WeightOfScaledComparison(const FFaerieItemCapacity& Original, const FFaerieItemCapacity& Comparison);

	UFUNCTION(BlueprintPure, Category = "Faerie|Capacity", meta = (DisplayName = "WeightAndVolume + WeightAndVolume", CompactNodeTitle = "+", ScriptMethod = "Add",
		ScriptOperator = "+;+=", Keywords = "+ add plus", CommutativeAssociativeBinaryOperator = "true", BlueprintThreadSafe))
	static FFaerieWeightAndVolume Add_WeightAndVolume(const FFaerieWeightAndVolume& A, const FFaerieWeightAndVolume& B) { return A + B; }

	UFUNCTION(BlueprintPure, Category = "Faerie|Capacity", meta = (DisplayName = "WeightAndVolume - WeightAndVolume", CompactNodeTitle = "-", ScriptMethod = "Subtract",
		ScriptOperator = "-;-=", Keywords = "- subtract minus", CommutativeAssociativeBinaryOperator = "true", BlueprintThreadSafe))
	static FFaerieWeightAndVolume Subtract_WeightAndVolume(const FFaerieWeightAndVolume& A, const FFaerieWeightAndVolume& B) { return A - B; }

	UFUNCTION(BlueprintPure, Category = "Faerie|Capacity", meta = (BlueprintThreadSafe))
	static FFaerieWeightAndVolume ToWeightAndVolume_ItemCapacity(const FFaerieItemCapacity& ItemCapacity);

	// Get the current amount filled.
	UFUNCTION(BlueprintPure, Category = "Faerie|Capacity", meta = (BlueprintThreadSafe))
	static FFaerieWeightAndVolume GetCurrentCapacity(const FFaerieCapacityExtensionState& State);

	// Get the maximum amount that this manager can hold.
	UFUNCTION(BlueprintPure, Category = "Faerie|Capacity", meta = (BlueprintThreadSafe))
	static FFaerieWeightAndVolume GetMaxCapacity(const FFaerieCapacityExtensionConfig& Config);

	UFUNCTION(BlueprintPure, Category = "Faerie|Capacity", meta = (BlueprintThreadSafe))
	static bool IsOverMaxWeight(const FFaerieCapacityExtensionState& State, const FFaerieCapacityExtensionConfig& Config);

	UFUNCTION(BlueprintPure, Category = "Faerie|Capacity", meta = (BlueprintThreadSafe))
	static bool IsOverMaxVolume(const FFaerieCapacityExtensionState& State, const FFaerieCapacityExtensionConfig& Config);

	UFUNCTION(BlueprintPure, Category = "Faerie|Capacity", meta = (BlueprintThreadSafe))
	static float GetPercentageFullForWeightAndVolume(const FFaerieWeightAndVolume& WeightAndVolume, const FFaerieCapacityExtensionConfig& Config);

	// Get our current percentage "fullness"
	UFUNCTION(BlueprintPure, Category = "Faerie|Capacity", meta = (BlueprintThreadSafe))
	static float GetPercentageFull(const FFaerieCapacityExtensionState& State, const FFaerieCapacityExtensionConfig& Config);
};