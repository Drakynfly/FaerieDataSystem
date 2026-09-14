// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "BasicItemFilters.h"
#include "FaerieItem.h"
#include "FaerieItemDataView.h"

#include "Fragments/FaerieTagFragment.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BasicItemFilters)

#define LOCTEXT_NAMESPACE "BasicItemDataFilters"

using namespace Faerie;

bool FFaerieItemFilter_Not::Exec(const FMassEntityManager* EntityManager, const TValid<const FFaerieItemProxy&> Proxy) const
{
	if (!InvertedFilter.IsValid()) return false;
	return !InvertedFilter->Exec(EntityManager, Proxy);
}

#if WITH_EDITOR
bool FFaerieItemFilter_Not::ExecWithLog(const FMassEntityManager* EntityManager, const TValid<const FFaerieItemProxy&> Proxy,
	ItemData::FFilterLogger& Logger) const
{
	if (!InvertedFilter.IsValid())
	{
		static const FText ErrorFormat = NSLOCTEXT("FFaerieItemFilter_Not", "FFaerieItemFilter_Not_InvalidFilter", "InvertedFilter invalid for FFaerieItemFilter_Not!");
		Logger.Errors.Add(ErrorFormat);
	}

	return !InvertedFilter->ExecWithLog(EntityManager, Proxy, Logger);
}

EFaerieItemFilterMutabilityStatus FFaerieItemFilter_Not::GetMutabilityStatus() const
{
	// We can't really determine anything about this filter's mutability. We cannot use the inverted filter's status,
	// nor can we 'invert' it's status.
	return EFaerieItemFilterMutabilityStatus::Unknown;
}
#endif

bool FFaerieItemFilter_Or::Exec(const FMassEntityManager* EntityManager, const TValid<const FFaerieItemProxy&> Proxy) const
{
	for (auto&& Rule : Rules)
	{
		if (Rule->Exec(EntityManager, Proxy))
		{
			return true;
		}
	}

	return false;
}

#if WITH_EDITOR
bool FFaerieItemFilter_Or::ExecWithLog(const FMassEntityManager* EntityManager, const TValid<const FFaerieItemProxy&> Proxy,
	ItemData::FFilterLogger& Logger) const
{
	for (auto&& Rule : Rules)
	{
		if (!Rule->ExecWithLog(EntityManager, Proxy, Logger))
		{
			return false;
		}
	}

	return true;
}

EFaerieItemFilterMutabilityStatus FFaerieItemFilter_Or::GetMutabilityStatus() const
{
	EFaerieItemFilterMutabilityStatus OutStatus = EFaerieItemFilterMutabilityStatus::Unknown;
	for (auto&& Rule : Rules)
	{
		if (!Rule.IsValid()) continue;
		OutStatus = ItemData::CombineStatuses(OutStatus, Rule->GetMutabilityStatus());
	}

	return OutStatus;
}
#endif

bool FFaerieItemFilter_And::Exec(const FMassEntityManager* EntityManager, const TValid<const FFaerieItemProxy&> Proxy) const
{
	for (auto&& Rule : Rules)
	{
		if (!Rule->Exec(EntityManager, Proxy))
		{
			return false;
		}
	}

	return true;
}

#if WITH_EDITOR
bool FFaerieItemFilter_And::ExecWithLog(const FMassEntityManager* EntityManager, const TValid<const FFaerieItemProxy&> Proxy,
	ItemData::FFilterLogger& Logger) const
{
	for (auto&& Rule : Rules)
	{
		if (!Rule->ExecWithLog(EntityManager, Proxy, Logger))
		{
			return false;
		}
	}

	return true;
}

EFaerieItemFilterMutabilityStatus FFaerieItemFilter_And::GetMutabilityStatus() const
{
	EFaerieItemFilterMutabilityStatus OutStatus = EFaerieItemFilterMutabilityStatus::Unknown;
	for (auto&& Rule : Rules)
	{
		if (!Rule.IsValid()) continue;
		OutStatus = ItemData::CombineStatuses(OutStatus, Rule->GetMutabilityStatus());
	}

	return OutStatus;
}

#endif

bool FFaerieItemFilter_Conditional::Exec(const FMassEntityManager* EntityManager, const TValid<const FFaerieItemProxy&> Proxy) const
{
	if (!ConditionRule.IsValid()) return false;
	if (ConditionRule->Exec(EntityManager, Proxy))
	{
		if (!TrueBranch.IsValid()) return false;
		return TrueBranch->Exec(EntityManager, Proxy);
	}
	return FalseBranch;
}

#if WITH_EDITOR
EFaerieItemFilterMutabilityStatus FFaerieItemFilter_Conditional::GetMutabilityStatus() const
{
	const EFaerieItemFilterMutabilityStatus ConditionalStatus = ConditionRule.IsValid() ? ConditionRule->GetMutabilityStatus() : EFaerieItemFilterMutabilityStatus::Unknown;
	const EFaerieItemFilterMutabilityStatus TrueBranchStatus = TrueBranch.IsValid() ? TrueBranch->GetMutabilityStatus() : EFaerieItemFilterMutabilityStatus::Unknown;
	return ItemData::CombineStatuses(ConditionalStatus, TrueBranchStatus);
}
#endif

bool FFaerieItemFilter_Ternary::Exec(const FMassEntityManager* EntityManager, const TValid<const FFaerieItemProxy&> Proxy) const
{
	if (!ConditionRule.IsValid()) return false;
	if (ConditionRule->Exec(EntityManager, Proxy))
	{
		if (!TrueBranch.IsValid()) return false;
		return TrueBranch->Exec(EntityManager, Proxy);
	}
	if (!FalseBranch.IsValid()) return false;
	return FalseBranch->Exec(EntityManager, Proxy);
}

#if WITH_EDITOR
EFaerieItemFilterMutabilityStatus FFaerieItemFilter_Ternary::GetMutabilityStatus() const
{
	const EFaerieItemFilterMutabilityStatus ConditionalStatus = ConditionRule.IsValid() ? ConditionRule->GetMutabilityStatus() : EFaerieItemFilterMutabilityStatus::Unknown;
	const EFaerieItemFilterMutabilityStatus TrueBranchStatus = TrueBranch.IsValid() ? TrueBranch->GetMutabilityStatus() : EFaerieItemFilterMutabilityStatus::Unknown;
	const EFaerieItemFilterMutabilityStatus FalseBranchStatus = FalseBranch.IsValid() ? FalseBranch->GetMutabilityStatus() : EFaerieItemFilterMutabilityStatus::Unknown;
	return ItemData::CombineStatuses(ConditionalStatus, ItemData::CombineStatuses(TrueBranchStatus, FalseBranchStatus));
}
#endif

bool FFaerieItemFilter_Mutability::Exec(const FMassEntityManager* EntityManager, const TValid<const FFaerieItemProxy&> Proxy) const
{
	return ValidGet(Proxy).GetItemInstanceOrInvalid().IsMutable() == RequireMutable;
}

#if WITH_EDITOR
EFaerieItemFilterMutabilityStatus FFaerieItemFilter_Mutability::GetMutabilityStatus() const
{
	return RequireMutable ? EFaerieItemFilterMutabilityStatus::KnownMutable : EFaerieItemFilterMutabilityStatus::KnownImmutable;
}
#endif

bool FFaerieItemFilter_MatchTemplate::Exec(const FMassEntityManager* EntityManager, const TValid<const FFaerieItemProxy&> Proxy) const
{
	if (IsValid(Template))
	{
		return Template->TryMatch(EntityManager, Proxy);
	}
	return false;
}

#if WITH_EDITOR
bool FFaerieItemFilter_MatchTemplate::ExecWithLog(const FMassEntityManager* EntityManager, const TValid<const FFaerieItemProxy&> Proxy,
	ItemData::FFilterLogger& Logger) const
{
	if (IsValid(Template))
	{
		return Template->TryMatchWithDescriptions(EntityManager, Proxy, Logger.Errors);
	}
	return false;
}

EFaerieItemFilterMutabilityStatus FFaerieItemFilter_MatchTemplate::GetMutabilityStatus() const
{
	if (IsValid(Template) && Template->GetFilter().IsValid())
	{
		return Template->GetFilter()->GetMutabilityStatus();
	}
	return EFaerieItemFilterMutabilityStatus::Unknown;
}
#endif

FFaerieItemFilter_HasFragments::FFaerieItemFilter_HasFragments()
{
	ReferenceTag= ItemData::Tags::ReferenceDefaults;
}

bool FFaerieItemFilter_HasFragments::Exec(const FMassEntityManager* EntityManager, const TValid<const FFaerieItemProxy&> Proxy) const
{
	const FFaerieItemInstance Instance = ValidGet(Proxy).GetItemInstanceOrInvalid();

	// @todo cache the bitset?
	FMassElementBitSet BitSet;
	for (auto&& FragmentType : FragmentTypes)
	{
		if (FragmentType)
		{
			BitSet.Add(FragmentType);
		}
	}
	for (auto&& FragmentType : Fragments)
	{
		if (FragmentType)
		{
			BitSet.Add(FragmentType);
		}
	}

	for (auto&& It = BitSet.GetIndexIterator(); It; ++It)
	{
		const UScriptStruct* FragmentType = BitSet.GetTypeAtIndex(*It);
		if (ItemData::HasEntityFragmentOrDefault(EntityManager, Instance, FragmentType, ReferenceTag))
		{
			BitSet.RemoveAtIndex(*It);
		}
	}

	return BitSet.IsEmpty();
}

#if WITH_EDITOR
bool FFaerieItemFilter_HasFragments::ExecWithLog(const FMassEntityManager* EntityManager, const TValid<const FFaerieItemProxy&> Proxy,
	ItemData::FFilterLogger& Logger) const
{
	static const FTextFormat MissingStructErrorFormat = LOCTEXT("HasFragments_MissingClassError", "Missing required fragment of type: '{0}'");
	static const FTextFormat InvalidTypeErrorFormat = LOCTEXT("HasFragments_InvalidType", "Invalid fragment type at index '{0}'");

	const FFaerieItemInstance Instance = ValidGet(Proxy).GetItemInstanceOrInvalid();

	FMassElementBitSet BitSet;
	for (int32 i = 0; i < FragmentTypes.Num(); ++i)
	{
		auto FragmentType = FragmentTypes[i];
		if (!FragmentType)
		{
			Logger.Errors.Add(FText::Format(InvalidTypeErrorFormat, i));
			continue;
		}

		BitSet.Add(FragmentType);
	}
	for (int32 i = 0; i < FragmentTypes.Num(); ++i)
	{
		auto FragmentType = FragmentTypes[i];
		if (!FragmentType)
		{
			Logger.Errors.Add(FText::Format(InvalidTypeErrorFormat, i));
			continue;
		}

		BitSet.Add(FragmentType);
	}

	for (auto&& It = BitSet.GetIndexIterator(); It; ++It)
	{
		const UScriptStruct* FragmentType = BitSet.GetTypeAtIndex(*It);

		if (ItemData::HasEntityFragmentOrDefault(EntityManager, Instance, FragmentType, ReferenceTag))
		{
			BitSet.RemoveAtIndex(*It);
		}
	}

	for (auto&& It = BitSet.GetIndexIterator(); It; ++It)
	{
		const UScriptStruct* FragmentType = BitSet.GetTypeAtIndex(*It);

		FFormatOrderedArguments Args;
#if WITH_EDITOR
		Args.Add(FragmentType->GetDisplayNameText());
#else
		Args.Add(FText::FromString(FragmentType->GetName()));
#endif
		Logger.Errors.Add(FText::Format(MissingStructErrorFormat, Args));
	}

	return BitSet.IsEmpty();
}

EFaerieItemFilterMutabilityStatus FFaerieItemFilter_HasFragments::GetMutabilityStatus() const
{
	// @Todo we could search the traits for mutability info...
	return Super::GetMutabilityStatus();
}
#endif

bool FFaerieItemFilter_Copies::Exec(const FMassEntityManager* EntityManager, const TValid<const FFaerieItemProxy&> Proxy) const
{
	const int32 Copies = ValidGet(Proxy).GetCopies();
	switch (Operator)
	{
	case EFaerieCopiesCompareOperator::Less:			return Copies < AmountToCompare;
	case EFaerieCopiesCompareOperator::LessOrEqual:		return Copies <= AmountToCompare;
	case EFaerieCopiesCompareOperator::Greater:			return Copies > AmountToCompare;
	case EFaerieCopiesCompareOperator::GreaterOrEqual:	return Copies >= AmountToCompare;
	case EFaerieCopiesCompareOperator::Equal:			return Copies == AmountToCompare;
	case EFaerieCopiesCompareOperator::NotEqual:		return Copies != AmountToCompare;
	default: return false;
	}
}

#if WITH_EDITOR
EFaerieItemFilterMutabilityStatus FFaerieItemFilter_Copies::GetMutabilityStatus() const
{
	auto&& Default = Super::GetMutabilityStatus();

	// In cases where only stacks are allowed through, we know that we are only passing immutable data.
	switch (Operator)
	{
	case EFaerieCopiesCompareOperator::Less:			return Default;
	case EFaerieCopiesCompareOperator::LessOrEqual:		return Default;
	case EFaerieCopiesCompareOperator::Greater:			return EFaerieItemFilterMutabilityStatus::KnownImmutable;
	case EFaerieCopiesCompareOperator::GreaterOrEqual:	return AmountToCompare > 1 ? EFaerieItemFilterMutabilityStatus::KnownImmutable : Default;
	case EFaerieCopiesCompareOperator::Equal:			return AmountToCompare > 1 ? EFaerieItemFilterMutabilityStatus::KnownImmutable : Default;
	case EFaerieCopiesCompareOperator::NotEqual:		return AmountToCompare == 1 ? EFaerieItemFilterMutabilityStatus::KnownImmutable : Default;
	default: return Default;
	}
}

#endif

bool FFaerieItemFilter_CompareName::Exec(const FMassEntityManager* EntityManager, const TValid<const FFaerieItemProxy&> Proxy) const
{
	auto AssetInfo = Faerie::ItemData::GetEntityFragmentOrDefault<FFaerieAssetInfo>(EntityManager, ValidGet(Proxy).GetItemInstanceOrInvalid());
	if (AssetInfo.IsValid())
	{
		return AssetInfo->ObjectName.CompareTo(CompareText, ComparisonType) == 0;
	}
	return false;
}

bool FFaerieItemFilter_GameplayTagAny::Exec(const FMassEntityManager* EntityManager, const TValid<const FFaerieItemProxy&> Proxy) const
{
	auto TagFragment = Faerie::ItemData::GetEntityFragmentOrDefault<FFaerieTagFragment>(EntityManager, ValidGet(Proxy).GetItemInstanceOrInvalid());
	if (TagFragment.IsValid())
	{
		if (Exact)
		{
			return TagFragment->Tags.HasAnyExact(Tags);
		}
		return TagFragment->Tags.HasAny(Tags);
	}
	return false;
}

bool FFaerieItemFilter_GameplayTagAll::Exec(const FMassEntityManager* EntityManager, const TValid<const FFaerieItemProxy&> Proxy) const
{
	auto TagFragment = Faerie::ItemData::GetEntityFragmentOrDefault<FFaerieTagFragment>(EntityManager, ValidGet(Proxy).GetItemInstanceOrInvalid());
	if (TagFragment.IsValid())
	{
		if (Exact)
		{
			return TagFragment->Tags.HasAllExact(Tags);
		}
		return TagFragment->Tags.HasAll(Tags);
	}
	return false;
}

bool FFaerieItemFilter_GameplayTagQuery::Exec(const FMassEntityManager* EntityManager, const TValid<const FFaerieItemProxy&> Proxy) const
{
	auto TagFragment = Faerie::ItemData::GetEntityFragmentOrDefault<FFaerieTagFragment>(EntityManager, ValidGet(Proxy).GetItemInstanceOrInvalid());
	if (TagFragment.IsValid())
	{
		return Query.Matches(TagFragment->Tags);
	}
	return false;
}

#undef LOCTEXT_NAMESPACE
