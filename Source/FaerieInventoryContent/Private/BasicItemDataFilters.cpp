// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "BasicItemDataFilters.h"
#include "EntityManagerHelpers.h"
#include "FaerieContainerFilterTypes.h"
#include "FaerieItem.h"
#include "FaerieItemDataView.h"

#include "Fragments/FaerieStackLimitFragment.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BasicItemDataFilters)

#define LOCTEXT_NAMESPACE "BasicItemDataFilters"

using namespace Faerie;

#if WITH_EDITOR
EFaerieItemDataMutabilityStatus UFilterRule_LogicalOr::GetMutabilityStatus() const
{
	for (auto&& Rule : Rules)
	{
		if (!Rule) continue;

		switch (Rule->GetMutabilityStatus())
		{
		case EFaerieItemDataMutabilityStatus::Unknown: break;
		case EFaerieItemDataMutabilityStatus::KnownMutable: return EFaerieItemDataMutabilityStatus::KnownMutable;
		case EFaerieItemDataMutabilityStatus::KnownImmutable: return EFaerieItemDataMutabilityStatus::KnownMutable;
		default: ;
		}
	}

	return Super::GetMutabilityStatus();
}
#endif

bool UFilterRule_LogicalOr::ExecWithLog(const TNotNull<const UObject*> WorldContextObj, const ItemData::FValidatedDataView& View, ItemData::FFilterLogger& Logger) const
{
	for (auto&& Rule : Rules)
	{
		if (!Rule->ExecWithLog(WorldContextObj, View, Logger))
		{
			return false;
		}
	}

	return true;
}

bool UFilterRule_LogicalOr::Exec(const TNotNull<const UObject*> WorldContextObj, const ItemData::FValidatedDataView& View) const
{
	for (auto&& Rule : Rules)
	{
		if (Rule->Exec(WorldContextObj, View))
		{
			return true;
		}
	}

	return false;
}

#if WITH_EDITOR
EFaerieItemDataMutabilityStatus UFilterRule_LogicalAnd::GetMutabilityStatus() const
{
	for (auto&& Rule : Rules)
	{
		if (!Rule) continue;

		switch (Rule->GetMutabilityStatus())
		{
		case EFaerieItemDataMutabilityStatus::Unknown: break;
		case EFaerieItemDataMutabilityStatus::KnownMutable: return EFaerieItemDataMutabilityStatus::KnownMutable;
		case EFaerieItemDataMutabilityStatus::KnownImmutable: return EFaerieItemDataMutabilityStatus::KnownMutable;
		default: ;
		}
	}

	return Super::GetMutabilityStatus();
}

#endif

bool UFilterRule_LogicalAnd::ExecWithLog(const TNotNull<const UObject*> WorldContextObj, const ItemData::FValidatedDataView& View, ItemData::FFilterLogger& Logger) const
{
	for (auto&& Rule : Rules)
	{
		if (!Rule->ExecWithLog(WorldContextObj, View, Logger))
		{
			return false;
		}
	}

	return true;
}

bool UFilterRule_LogicalAnd::Exec(const TNotNull<const UObject*> WorldContextObj, const ItemData::FValidatedDataView& View) const
{
	for (auto&& Rule : Rules)
	{
		if (!Rule->Exec(WorldContextObj, View))
		{
			return false;
		}
	}

	return true;
}

#if WITH_EDITOR
EFaerieItemDataMutabilityStatus UFilterRule_Condition::GetMutabilityStatus() const
{
	// @TODO
	return Super::GetMutabilityStatus();
}
#endif

bool UFilterRule_Condition::Exec(const TNotNull<const UObject*> WorldContextObj, const ItemData::FValidatedDataView& View) const
{
	if (!ConditionRule) return false;
	if (ConditionRule->Exec(WorldContextObj, View))
	{
		if (!TrueBranch) return false;
		return TrueBranch->Exec(WorldContextObj, View);
	}
	return FalseBranch;
}

#if WITH_EDITOR
EFaerieItemDataMutabilityStatus UFilterRule_Ternary::GetMutabilityStatus() const
{
	// @TODO
	return Super::GetMutabilityStatus();
}
#endif

bool UFilterRule_Ternary::Exec(const TNotNull<const UObject*> WorldContextObj, const ItemData::FValidatedDataView& View) const
{
	if (!ConditionRule) return false;
	if (ConditionRule->Exec(WorldContextObj, View))
	{
		if (!TrueBranch) return false;
		return TrueBranch->Exec(WorldContextObj, View);
	}
	if (!FalseBranch) return false;
	return FalseBranch->Exec(WorldContextObj, View);
}

#if WITH_EDITOR
EFaerieItemDataMutabilityStatus UFilterRule_LogicalNot::GetMutabilityStatus() const
{
	if (InvertedRule)
	{
		return InvertedRule->GetMutabilityStatus();
	}
	return Super::GetMutabilityStatus();
}
#endif

bool UFilterRule_LogicalNot::Exec(const TNotNull<const UObject*> WorldContextObj, const ItemData::FValidatedDataView& View) const
{
	return !InvertedRule->Exec(WorldContextObj, View);
}

#if WITH_EDITOR
EFaerieItemDataMutabilityStatus UFilterRule_Mutability::GetMutabilityStatus() const
{
	return RequireMutable ? EFaerieItemDataMutabilityStatus::KnownMutable : EFaerieItemDataMutabilityStatus::KnownImmutable;
}
#endif

bool UFilterRule_Mutability::Exec(const TNotNull<const UObject*> WorldContextObj, const ItemData::FValidatedDataView& View) const
{
	return View->GetInstance().IsMutable() == RequireMutable;
}

#if WITH_EDITOR
EFaerieItemDataMutabilityStatus UFilterRule_MatchTemplate::GetMutabilityStatus() const
{
	if (IsValid(Template))
	{
		return Template->GetPattern()->GetMutabilityStatus();
	}
	return Super::GetMutabilityStatus();
}
#endif

bool UFilterRule_MatchTemplate::ExecWithLog(const TNotNull<const UObject*> WorldContextObj, const ItemData::FValidatedDataView& View,
	ItemData::FFilterLogger& Logger) const
{
	if (IsValid(Template))
	{
		return Template->TryMatchWithDescriptions(WorldContextObj, View, Logger.Errors);
	}
	return false;
}

bool UFilterRule_MatchTemplate::Exec(const TNotNull<const UObject*> WorldContextObj, const ItemData::FValidatedDataView& View) const
{
	if (IsValid(Template))
	{
		return Template->TryMatch(WorldContextObj, View);
	}
	return false;
}

UFilterRule_HasFragments::UFilterRule_HasFragments()
{
	ReferenceTag= ItemData::Tags::ReferenceDefaults;
}

#if WITH_EDITOR
EFaerieItemDataMutabilityStatus UFilterRule_HasFragments::GetMutabilityStatus() const
{
	// @Todo we could search the traits for mutability info...
	return Super::GetMutabilityStatus();
}
#endif

bool UFilterRule_HasFragments::ExecWithLog(const TNotNull<const UObject*> WorldContextObj, const ItemData::FValidatedDataView& View, ItemData::FFilterLogger& Logger) const
{
	static const FText InvalidViewError = LOCTEXT("HasFragments_InvalidViewError", "View is invalid");
	static const FTextFormat MissingStructErrorFormat = LOCTEXT("HasFragments_MissingClassError", "Missing required fragment of type: '{0}'");

	const FFaerieItemInstance Instance = View->GetInstance();
	if (!Instance.IsValid())
	{
		Logger.Errors.Add(InvalidViewError);
		return false;
	}

	TSet<TSubScriptStructOf<FFaerieMassFragment>> FragmentTypesCopy(FragmentTypes);

	const ItemData::FOptionalEntityManager EntityManager(WorldContextObj);
	for (auto&& StructType : FragmentTypes)
	{
		auto FragmentView = ItemData::GetEntityFragmentOrDefault(EntityManager, Instance, StructType, ReferenceTag);
		if (FragmentView.IsValid())
		{
			FragmentTypesCopy.Remove(StructType);
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

bool UFilterRule_HasFragments::Exec(const TNotNull<const UObject*> WorldContextObj, const ItemData::FValidatedDataView& View) const
{
	const FFaerieItemInstance Instance = View->GetInstance();
	if (!Instance.IsValid())
	{
		return false;
	}

	TSet<TSubScriptStructOf<FFaerieMassFragment>> FragmentTypesCopy(FragmentTypes);

	const ItemData::FOptionalEntityManager EntityManager(WorldContextObj);
	for (auto&& StructType : FragmentTypes)
	{
		auto FragmentView = ItemData::GetEntityFragmentOrDefault(EntityManager, Instance, StructType, ReferenceTag);
		if (FragmentView.IsValid())
		{
			FragmentTypesCopy.Remove(StructType);
		}
	}

	return FragmentTypesCopy.IsEmpty();
}

#if WITH_EDITOR
EFaerieItemDataMutabilityStatus UFilterRule_Copies::GetMutabilityStatus() const
{
	auto&& Default = Super::GetMutabilityStatus();

	// In cases where only stacks are allowed through, we know that we are only passing immutable data.

	switch (Operator)
	{
	case EFaerieCopiesCompareOperator::Less:				return Default;
	case EFaerieCopiesCompareOperator::LessOrEqual:		return Default;
	case EFaerieCopiesCompareOperator::Greater:			return EFaerieItemDataMutabilityStatus::KnownImmutable;
	case EFaerieCopiesCompareOperator::GreaterOrEqual:	return AmountToCompare > 1 ? EFaerieItemDataMutabilityStatus::KnownImmutable : Default;
	case EFaerieCopiesCompareOperator::Equal:				return AmountToCompare > 1 ? EFaerieItemDataMutabilityStatus::KnownImmutable : Default;
	case EFaerieCopiesCompareOperator::NotEqual:			return AmountToCompare == 1 ? EFaerieItemDataMutabilityStatus::KnownImmutable : Default;
	default: return Default;
	}
}
#endif

bool UFilterRule_Copies::Exec(const TNotNull<const UObject*> WorldContextObj, const ItemData::FValidatedDataView& View) const
{
	const int32 Copies = View->GetCopies();
	switch (Operator)
	{
	case EFaerieCopiesCompareOperator::Less:				return Copies < AmountToCompare;
	case EFaerieCopiesCompareOperator::LessOrEqual:			return Copies <= AmountToCompare;
	case EFaerieCopiesCompareOperator::Greater:				return Copies > AmountToCompare;
	case EFaerieCopiesCompareOperator::GreaterOrEqual:		return Copies >= AmountToCompare;
	case EFaerieCopiesCompareOperator::Equal:				return Copies == AmountToCompare;
	case EFaerieCopiesCompareOperator::NotEqual:			return Copies != AmountToCompare;
	default: return false;
	}
}

#if WITH_EDITOR
EFaerieItemDataMutabilityStatus UFilterRule_StackLimit::GetMutabilityStatus() const
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

bool UFilterRule_StackLimit::Exec(const TNotNull<const UObject*> WorldContextObj, const ItemData::FValidatedDataView& View) const
{
	ItemData::FOptionalEntityManager EntityManager(WorldContextObj);
	if (const int32 Limit = Container::GetItemStackLimit(EntityManager, View->GetInstance());
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

bool UFilterRule_GameplayTagAny::Exec(const TNotNull<const UObject*> WorldContextObj, const ItemData::FValidatedDataView& View) const
{
	Container::FHasAnyTags HasTag;
	HasTag.Tags = Tags;
	return HasTag.Exec(WorldContextObj, View);
}

bool UFilterRule_GameplayTagAll::Exec(const TNotNull<const UObject*> WorldContextObj, const ItemData::FValidatedDataView& View) const
{
	Container::FHasAllTags HasTag;
	HasTag.Tags = Tags;
	return HasTag.Exec(WorldContextObj, View);
}

#undef LOCTEXT_NAMESPACE