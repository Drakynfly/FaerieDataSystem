// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemProxy.h"
#include "ValidParameter.h"

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
	virtual bool Exec(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> ProxyA, Faerie::TValid<const FFaerieItemProxy&> ProxyB) const
		PURE_VIRTUAL(UFaerieItemDataComparator::Exec, return false; )

protected:
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemDataComparator", DisplayName = "Exec")
	bool K2_Exec(const FFaerieItemProxy& A, const FFaerieItemProxy& B) const;
};

/*
 * Base class for making blueprint comparators.
 */
UCLASS(Abstract, Blueprintable)
class UFaerieItemDataComparator_BlueprintBase final : public UFaerieItemDataComparator
{
	GENERATED_BODY()

public:
	virtual bool Exec(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> ProxyA,
					  Faerie::TValid<const FFaerieItemProxy&> ProxyB) const override;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Faerie|ItemDataComparator")
	bool Execute(const FFaerieItemProxy& A, const FFaerieItemProxy& B) const;
};