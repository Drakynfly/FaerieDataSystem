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

	UFUNCTION(BlueprintCallable, Category = "CapacityToken")
	int32 GetVolumeOfStack(const int32 Stack) const;

	UFUNCTION(BlueprintCallable, Category = "CapacityToken")
	FWeightAndVolume GetWeightAndVolumeOfStack(const int32 Stack) const;

	UFUNCTION(BlueprintCallable, Category = "CapacityToken")
	FWeightAndVolume GetWeightAndVolumeOfStackForRemoval(const int32 Current, const int32 Removal) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, meta = (ShowOnlyInnerProperties, ExposeOnSpawn))
	FItemCapacity Capacity;
};