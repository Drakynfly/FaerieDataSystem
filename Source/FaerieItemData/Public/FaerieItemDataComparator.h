// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataView.h"
#include "UObject/Object.h"
#include "FaerieItemDataComparator.generated.h"

/**
 * Compares two item views. Used to create sorting functionality.
 */
UCLASS(Abstract, Const, BlueprintType, EditInlineNew, DefaultToInstanced, CollapseCategories)
class FAERIEITEMDATA_API UFaerieItemDataComparator : public UObject
{
	GENERATED_BODY()

public:
	virtual bool Exec(TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& ViewA, const Faerie::ItemData::FValidatedDataView& ViewB) const
		PURE_VIRTUAL(UFaerieItemDataComparator::Exec, return false; )

protected:
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemDataComparator", DisplayName = "Exec", meta = (WorldContext = "WorldContextObj"))
	bool K2_Exec(UObject* WorldContextObj, const FFaerieItemDataView& A, const FFaerieItemDataView& B) const;
};

/*
 * Base class for making blueprint comparators.
 */
UCLASS(Abstract, Blueprintable)
class UFaerieItemDataComparator_BlueprintBase final : public UFaerieItemDataComparator
{
	GENERATED_BODY()

public:
	virtual bool Exec(TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& ViewA, const Faerie::ItemData::FValidatedDataView& ViewB) const override;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Faerie|ItemDataComparator")
	bool Execute(const FFaerieItemDataView& A, const FFaerieItemDataView& B) const;
};