// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "ContainerFilters.h"
#include "Fragments/FaerieStackLimitFragment.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ContainerFilters)

using namespace Faerie;

// ReSharper disable CppBadColonSpaces

bool FFaerieItemFilter_StackLimit::Exec(const FMassEntityManager* EntityManager, const TValid<const FFaerieItemProxy&> Proxy) const
{
	if (const int32 Limit = Container::GetItemStackLimit(EntityManager, ValidGet(Proxy).GetItemInstance().GetValue());
		Limit == ItemData::UnlimitedStack)
	{
		switch (Operator)
		{
		case EFaerieStackCompareOperator::Less:				return false;
		case EFaerieStackCompareOperator::LessOrEqual:		return false;
		case EFaerieStackCompareOperator::Greater:			return true;
		case EFaerieStackCompareOperator::GreaterOrEqual:	return true;
		case EFaerieStackCompareOperator::Equal:			return false;
		case EFaerieStackCompareOperator::NotEqual:			return true;
		case EFaerieStackCompareOperator::HasLimit:			return false;
		case EFaerieStackCompareOperator::HasNoLimit:		return true;
		default: return false;
		}
	}
	else
	{
		switch (Operator)
		{
		case EFaerieStackCompareOperator::Less:				return Limit < AmountToCompare;
		case EFaerieStackCompareOperator::LessOrEqual:		return Limit <= AmountToCompare;
		case EFaerieStackCompareOperator::Greater:			return Limit > AmountToCompare;
		case EFaerieStackCompareOperator::GreaterOrEqual:	return Limit >= AmountToCompare;
		case EFaerieStackCompareOperator::Equal:			return Limit == AmountToCompare;
		case EFaerieStackCompareOperator::NotEqual:			return Limit != AmountToCompare;
		case EFaerieStackCompareOperator::HasLimit:			return true;
		case EFaerieStackCompareOperator::HasNoLimit:		return false;
		default: return false;
		}
	}
}

#if WITH_EDITOR
EFaerieItemFilterMutabilityStatus FFaerieItemFilter_StackLimit::GetMutabilityStatus() const
{
	auto&& Default = Super::GetMutabilityStatus();

	// In cases where only stacks are allowed through, we know that we are only passing immutable data.
	switch (Operator)
	{
	case EFaerieStackCompareOperator::Less:				return Default;
	case EFaerieStackCompareOperator::LessOrEqual:		return Default;
	case EFaerieStackCompareOperator::Greater:			return EFaerieItemFilterMutabilityStatus::KnownImmutable;
	case EFaerieStackCompareOperator::GreaterOrEqual:	return AmountToCompare > 1 ? EFaerieItemFilterMutabilityStatus::KnownImmutable : Default;
	case EFaerieStackCompareOperator::Equal:			return AmountToCompare > 1 ? EFaerieItemFilterMutabilityStatus::KnownImmutable : Default;
	case EFaerieStackCompareOperator::NotEqual:			return AmountToCompare == 1 ? EFaerieItemFilterMutabilityStatus::KnownImmutable : Default;
	case EFaerieStackCompareOperator::HasLimit:			return Default;
	case EFaerieStackCompareOperator::HasNoLimit:		return EFaerieItemFilterMutabilityStatus::KnownImmutable;;
	default: return Default;
	}
}
#endif