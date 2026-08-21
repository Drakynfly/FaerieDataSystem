// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "BasicItemDataFilters.h"
#include "FaerieItem.h"
#include "FaerieItemDataView.h"

#include "Fragments/FaerieStackLimitFragment.h"
#include "Fragments/FaerieTagFragment.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BasicItemDataFilters)

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

EFaerieItemDataMutabilityStatus FFaerieItemFilter_Not::GetMutabilityStatus() const
{
	// We can't really determine anything about this filter's mutability. We cannot use the inverted filter's status,
	// nor can we 'invert' it's status.
	return EFaerieItemDataMutabilityStatus::Unknown;
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

EFaerieItemDataMutabilityStatus FFaerieItemFilter_Or::GetMutabilityStatus() const
{
	EFaerieItemDataMutabilityStatus OutStatus = EFaerieItemDataMutabilityStatus::Unknown;
	for (auto&& Rule : Rules)
	{
		if (!Rule.IsValid()) continue;
		OutStatus = CombineStatuses(OutStatus, Rule->GetMutabilityStatus());
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

EFaerieItemDataMutabilityStatus FFaerieItemFilter_And::GetMutabilityStatus() const
{
	EFaerieItemDataMutabilityStatus OutStatus = EFaerieItemDataMutabilityStatus::Unknown;
	for (auto&& Rule : Rules)
	{
		if (!Rule.IsValid()) continue;
		OutStatus = CombineStatuses(OutStatus, Rule->GetMutabilityStatus());
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
EFaerieItemDataMutabilityStatus FFaerieItemFilter_Conditional::GetMutabilityStatus() const
{
	const EFaerieItemDataMutabilityStatus ConditionalStatus = ConditionRule.IsValid() ? ConditionRule->GetMutabilityStatus() : EFaerieItemDataMutabilityStatus::Unknown;
	const EFaerieItemDataMutabilityStatus TrueBranchStatus = TrueBranch.IsValid() ? TrueBranch->GetMutabilityStatus() : EFaerieItemDataMutabilityStatus::Unknown;
	return CombineStatuses(ConditionalStatus, TrueBranchStatus);
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
EFaerieItemDataMutabilityStatus FFaerieItemFilter_Ternary::GetMutabilityStatus() const
{
	const EFaerieItemDataMutabilityStatus ConditionalStatus = ConditionRule.IsValid() ? ConditionRule->GetMutabilityStatus() : EFaerieItemDataMutabilityStatus::Unknown;
	const EFaerieItemDataMutabilityStatus TrueBranchStatus = TrueBranch.IsValid() ? TrueBranch->GetMutabilityStatus() : EFaerieItemDataMutabilityStatus::Unknown;
	const EFaerieItemDataMutabilityStatus FalseBranchStatus = FalseBranch.IsValid() ? FalseBranch->GetMutabilityStatus() : EFaerieItemDataMutabilityStatus::Unknown;
	return CombineStatuses(ConditionalStatus, CombineStatuses(TrueBranchStatus, FalseBranchStatus));
}
#endif

bool FFaerieItemFilter_Mutability::Exec(const FMassEntityManager* EntityManager, const TValid<const FFaerieItemProxy&> Proxy) const
{
	return ValidGet(Proxy).GetItemInstanceOrInvalid().IsMutable() == RequireMutable;
}

#if WITH_EDITOR
EFaerieItemDataMutabilityStatus FFaerieItemFilter_Mutability::GetMutabilityStatus() const
{
	return RequireMutable ? EFaerieItemDataMutabilityStatus::KnownMutable : EFaerieItemDataMutabilityStatus::KnownImmutable;
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

EFaerieItemDataMutabilityStatus FFaerieItemFilter_MatchTemplate::GetMutabilityStatus() const
{
	if (IsValid(Template) && Template->GetFilter().IsValid())
	{
		return Template->GetFilter()->GetMutabilityStatus();
	}
	return EFaerieItemDataMutabilityStatus::Unknown;
}
#endif

FFaerieItemFilter_HasFragments::FFaerieItemFilter_HasFragments()
{
	ReferenceTag= ItemData::Tags::ReferenceDefaults;
}

bool FFaerieItemFilter_HasFragments::Exec(const FMassEntityManager* EntityManager, const TValid<const FFaerieItemProxy&> Proxy) const
{
	TOptional<FFaerieItemInstance> Instance = ValidGet(Proxy).GetItemInstance();
	if (!Instance.IsSet())
	{
		return false;
	}

	TSet<TSubScriptStructOf<FFaerieMassFragment>> FragmentTypesCopy(FragmentTypes);

	for (auto&& StructType : FragmentTypes)
	{
		if (!StructType)
		{
			continue;
		}

		auto FragmentView = ItemData::GetEntityFragmentOrDefault(EntityManager, Instance.GetValue(), StructType, ReferenceTag);
		if (FragmentView.IsValid())
		{
			FragmentTypesCopy.Remove(StructType);
		}
	}

	return FragmentTypesCopy.IsEmpty();
}

#if WITH_EDITOR
bool FFaerieItemFilter_HasFragments::ExecWithLog(const FMassEntityManager* EntityManager, const TValid<const FFaerieItemProxy&> Proxy,
	ItemData::FFilterLogger& Logger) const
{
	static const FText InvalidViewError = LOCTEXT("HasFragments_InvalidViewError", "View is invalid");
	static const FTextFormat MissingStructErrorFormat = LOCTEXT("HasFragments_MissingClassError", "Missing required fragment of type: '{0}'");
	static const FTextFormat InvalidTypeErrorFormat = LOCTEXT("HasFragments_InvalidType", "Invalid fragment type at index '{0}'");

	const TOptional<FFaerieItemInstance> Instance = ValidGet(Proxy).GetItemInstance();
	if (!Instance.IsSet())
	{
		Logger.Errors.Add(InvalidViewError);
		return false;
	}

	TSet<TSubScriptStructOf<FFaerieMassFragment>> FragmentTypesCopy(FragmentTypes);

	for (auto&& It = FragmentTypes.CreateConstIterator(); It; ++It)
	{
		if (!*It)
		{
			Logger.Errors.Add(FText::Format(InvalidTypeErrorFormat, It.GetIndex()));
			continue;
		}

		auto FragmentView = ItemData::GetEntityFragmentOrDefault(EntityManager, Instance.GetValue(), *It, ReferenceTag);
		if (FragmentView.IsValid())
		{
			FragmentTypesCopy.Remove(*It);
		}
	}

	for (auto&& FragmentType : FragmentTypesCopy)
	{
		FFormatOrderedArguments Args;
#if WITH_EDITOR
		Args.Add(FragmentType->GetDisplayNameText());
#else
		Args.Add(FText::FromString(FragmentType->GetName()));
#endif
		Logger.Errors.Add(FText::Format(MissingStructErrorFormat, Args));
	}

	return FragmentTypesCopy.IsEmpty();
}

EFaerieItemDataMutabilityStatus FFaerieItemFilter_HasFragments::GetMutabilityStatus() const
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
EFaerieItemDataMutabilityStatus FFaerieItemFilter_Copies::GetMutabilityStatus() const
{
	auto&& Default = Super::GetMutabilityStatus();

	// In cases where only stacks are allowed through, we know that we are only passing immutable data.
	switch (Operator)
	{
	case EFaerieCopiesCompareOperator::Less:			return Default;
	case EFaerieCopiesCompareOperator::LessOrEqual:		return Default;
	case EFaerieCopiesCompareOperator::Greater:			return EFaerieItemDataMutabilityStatus::KnownImmutable;
	case EFaerieCopiesCompareOperator::GreaterOrEqual:	return AmountToCompare > 1 ? EFaerieItemDataMutabilityStatus::KnownImmutable : Default;
	case EFaerieCopiesCompareOperator::Equal:			return AmountToCompare > 1 ? EFaerieItemDataMutabilityStatus::KnownImmutable : Default;
	case EFaerieCopiesCompareOperator::NotEqual:		return AmountToCompare == 1 ? EFaerieItemDataMutabilityStatus::KnownImmutable : Default;
	default: return Default;
	}
}

#endif

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
EFaerieItemDataMutabilityStatus FFaerieItemFilter_StackLimit::GetMutabilityStatus() const
{
	auto&& Default = Super::GetMutabilityStatus();

	// In cases where only stacks are allowed through, we know that we are only passing immutable data.
	switch (Operator)
	{
	case EFaerieStackCompareOperator::Less:				return Default;
	case EFaerieStackCompareOperator::LessOrEqual:		return Default;
	case EFaerieStackCompareOperator::Greater:			return EFaerieItemDataMutabilityStatus::KnownImmutable;
	case EFaerieStackCompareOperator::GreaterOrEqual:	return AmountToCompare > 1 ? EFaerieItemDataMutabilityStatus::KnownImmutable : Default;
	case EFaerieStackCompareOperator::Equal:			return AmountToCompare > 1 ? EFaerieItemDataMutabilityStatus::KnownImmutable : Default;
	case EFaerieStackCompareOperator::NotEqual:			return AmountToCompare == 1 ? EFaerieItemDataMutabilityStatus::KnownImmutable : Default;
	case EFaerieStackCompareOperator::HasLimit:			return Default;
	case EFaerieStackCompareOperator::HasNoLimit:		return EFaerieItemDataMutabilityStatus::KnownImmutable;;
	default: return Default;
	}
}
#endif

bool FFaerieItemFilter_CompareName::Exec(const FMassEntityManager* EntityManager, const TValid<const FFaerieItemProxy&> Proxy) const
{
	auto AssetInfo = Faerie::ItemData::GetEntityFragmentOrDefault<FFaerieAssetInfo>(EntityManager, ValidGet(Proxy).GetItemInstance().GetValue());
	if (AssetInfo.IsValid())
	{
		return AssetInfo->ObjectName.CompareTo(CompareText, ComparisonType) == 0;
	}
	return false;
}

bool FFaerieItemFilter_GameplayTagAny::Exec(const FMassEntityManager* EntityManager, const TValid<const FFaerieItemProxy&> Proxy) const
{
	auto TagFragment = Faerie::ItemData::GetEntityFragmentOrDefault<FFaerieTagFragment>(EntityManager, ValidGet(Proxy).GetItemInstance().GetValue());
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
	auto TagFragment = Faerie::ItemData::GetEntityFragmentOrDefault<FFaerieTagFragment>(EntityManager, ValidGet(Proxy).GetItemInstance().GetValue());
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

#undef LOCTEXT_NAMESPACE