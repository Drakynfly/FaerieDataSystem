﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataProxy.h"
#include "UObject/Object.h"
#include "FaerieItemMutator.generated.h"

class UFaerieItemTemplate;

/**
 * Base class for mutation behavior. This is essentially a 'command' class.
 */
UCLASS(Abstract)
class FAERIEITEMGENERATOR_API UFaerieItemMutator : public UObject
{
	GENERATED_BODY()

protected:
	virtual bool CanApply(FFaerieItemProxy Proxy) const;
	virtual bool Apply(FFaerieItemStack Stack) PURE_VIRTUAL(UFaerieItemMutator::Apply, return false; )

public:
	bool TryApply(const FFaerieItemStack& Stack);

	// Any soft assets required to be loaded when Apply is called should be registered here.
	UFUNCTION(BlueprintNativeEvent, Category = "Mutator")
	void GetRequiredAssets(TArray<TSoftObjectPtr<UObject>>& RequiredAssets) const;

protected:
	// The filter that selects valid entries that this mutator can apply to.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Instanced, Category = "Resource Slot Filter")
	TObjectPtr<UFaerieItemTemplate> ApplicationFilter = nullptr;
};