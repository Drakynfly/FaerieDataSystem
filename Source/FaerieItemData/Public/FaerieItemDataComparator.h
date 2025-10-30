// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "UObject/Object.h"
#include "FaerieItemProxy.h"
#include "FaerieItemDataComparator.generated.h"

/**
 * Compares two item proxies. Used to create sorting functionality.
 */
UCLASS(Abstract, BlueprintType, Const, EditInlineNew, DefaultToInstanced, CollapseCategories)
class FAERIEITEMDATA_API UFaerieItemDataComparator : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemDataComparator")
	virtual bool Exec(const FFaerieItemSnapshot& A, const FFaerieItemSnapshot& B) const PURE_VIRTUAL(UFaerieItemDataComparator::Exec, return false; )
};


/*
 * Base class for making blueprint comparators.
 */
UCLASS(Abstract, Blueprintable)
class UFaerieItemDataComparator_BlueprintBase final : public UFaerieItemDataComparator
{
	GENERATED_BODY()

public:
	virtual bool Exec(const FFaerieItemSnapshot& A, const FFaerieItemSnapshot& B) const override
	{
		return Execute(A, B);
	}

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Faerie|ItemDataComparator")
	bool Execute(const FFaerieItemSnapshot& A, const FFaerieItemSnapshot& B) const;
};