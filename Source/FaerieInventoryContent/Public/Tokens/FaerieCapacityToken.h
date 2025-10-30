// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemToken.h"
#include "CapacityStructs.h"

#include "FaerieCapacityToken.generated.h"

/**
 *
 */
UCLASS(DisplayName = "Token - Capacity")
class FAERIEINVENTORYCONTENT_API UFaerieCapacityToken : public UFaerieItemToken
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	static UFaerieCapacityToken* CreateInstance(const FItemCapacity& Capacity);

	FItemCapacity GetCapacity() const { return Capacity; }

	UFUNCTION(BlueprintCallable, Category = "CapacityToken")
	int32 GetWeightOfStack(const int32 Stack) const;

	// Gets the volume of an entire stack. Volume == X + (X * (Stack - 1) * Efficiency)
	UFUNCTION(BlueprintCallable, Category = "CapacityToken")
	int64 GetVolumeOfStack(const int32 Stack) const;

	// Gets the volume of a partial stack. Volume == X * Stack * Efficiency
	UFUNCTION(BlueprintCallable, Category = "CapacityToken")
	int64 GetEfficientVolume(const int32 Stack) const;

	// Gets the weight and volume of a entire stack.
	UFUNCTION(BlueprintCallable, Category = "CapacityToken")
	FWeightAndVolume GetWeightAndVolumeOfStack(const int32 Stack) const;

	// Gets the weight and volume for a portion of a stack. Uses EfficientVolume rather than full volume.
	UFUNCTION(BlueprintCallable, Category = "CapacityToken")
	FWeightAndVolume GetWeightAndVolumeOfPartialStack(const int32 Stack) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, meta = (ShowOnlyInnerProperties, ExposeOnSpawn))
	FItemCapacity Capacity;
};