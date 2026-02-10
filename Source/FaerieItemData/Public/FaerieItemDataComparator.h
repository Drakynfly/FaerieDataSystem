// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataViewBase.h"
#include "UObject/Object.h"
#include "FaerieItemDataComparator.generated.h"

namespace Faerie::ItemData
{
	class IViewBase;
}

struct FFaerieItemDataViewWrapper;

/**
 * Compares two item views. Used to create sorting functionality.
 */
UCLASS(Abstract, BlueprintType, Const, EditInlineNew, DefaultToInstanced, CollapseCategories)
class FAERIEITEMDATA_API UFaerieItemDataComparator : public UObject
{
	GENERATED_BODY()

public:
	virtual bool Exec(Faerie::ItemData::FViewPtr ViewA, Faerie::ItemData::FViewPtr ViewB) const
		PURE_VIRTUAL(UFaerieItemDataComparator::Exec, return false; )

protected:
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemDataComparator", DisplayName = "Exec")
	bool K2_Exec(const FFaerieItemDataViewWrapper& A, const FFaerieItemDataViewWrapper& B) const;
};

/*
 * Base class for making blueprint comparators.
 */
UCLASS(Abstract, Blueprintable)
class UFaerieItemDataComparator_BlueprintBase final : public UFaerieItemDataComparator
{
	GENERATED_BODY()

public:
	virtual bool Exec(Faerie::ItemData::FViewPtr ViewA, Faerie::ItemData::FViewPtr ViewB) const override;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Faerie|ItemDataComparator")
	bool Execute(const FFaerieItemDataViewWrapper& A, const FFaerieItemDataViewWrapper& B) const;
};