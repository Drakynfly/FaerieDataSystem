// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemFilter.h"
#include "ContainerFilters.generated.h"

UENUM()
enum class EFaerieStackCompareOperator : uint8
{
	Less			UMETA(DisplayName = "<"),
	LessOrEqual		UMETA(DisplayName = "<="),
	Greater			UMETA(DisplayName = ">"),
	GreaterOrEqual	UMETA(DisplayName = ">="),
	Equal			UMETA(DisplayName = "=="),
	NotEqual		UMETA(DisplayName = "!="),
	HasLimit		UMETA(DisplayName = "Limited"),
	HasNoLimit		UMETA(DisplayName = "Unlimited")
};

/**
 * Filter entries by its stack limit
 */
USTRUCT()
struct FFaerieItemFilter_StackLimit final : public FFaerieItemFilterBase
{
	GENERATED_BODY()

	virtual bool Exec(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy) const override;

#if WITH_EDITOR
	virtual EFaerieItemFilterMutabilityStatus GetMutabilityStatus() const override;
#endif

	UPROPERTY(EditAnywhere, Category = "CompareLimit")
	EFaerieStackCompareOperator Operator = EFaerieStackCompareOperator::Less;

	UPROPERTY(EditAnywhere, Category = "CompareLimit",
		meta = (ClampMin = 1, EditCondition = "Operator != EFaerieStackCompareOperator::HasLimit && Operator != EFaerieStackCompareOperator::HasNoLimit", EditConditionHides))
	int32 AmountToCompare = 1;
};
