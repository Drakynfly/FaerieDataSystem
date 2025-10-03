// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "BasicItemDataFilters.h"
#include "FaerieItem.h"
#include "Tokens/FaerieStackLimiterToken.h"
#include "Tokens/FaerieTagToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BasicItemDataFilters)

#define LOCTEXT_NAMESPACE "BasicItemDataFilters"

#if WITH_EDITOR
EItemDataMutabilityStatus UFilterRule_LogicalOr::GetMutabilityStatus() const
{
	for (auto&& Rule : Rules)
	{
		if (!Rule) continue;

		switch (Rule->GetMutabilityStatus())
		{
		case EItemDataMutabilityStatus::Unknown: break;
		case EItemDataMutabilityStatus::KnownMutable: return EItemDataMutabilityStatus::KnownMutable;
		case EItemDataMutabilityStatus::KnownImmutable: return EItemDataMutabilityStatus::KnownMutable;
		default: ;
		}
	}

	return Super::GetMutabilityStatus();
}
#endif

bool UFilterRule_LogicalOr::ExecWithLog(const FFaerieItemStackView View, Faerie::ItemData::FFilterLogger& Logger) const
{
	for (auto&& Rule : Rules)
	{
		if (!Rule->ExecWithLog(View, Logger))
		{
			return false;
		}
	}

	return true;
}

bool UFilterRule_LogicalOr::Exec(const FFaerieItemStackView View) const
{
	for (auto&& Rule : Rules)
	{
		if (Rule->Exec(View))
		{
			return true;
		}
	}

	return false;
}

#if WITH_EDITOR
EItemDataMutabilityStatus UFilterRule_LogicalAnd::GetMutabilityStatus() const
{
	for (auto&& Rule : Rules)
	{
		if (!Rule) continue;

		switch (Rule->GetMutabilityStatus())
		{
		case EItemDataMutabilityStatus::Unknown: break;
		case EItemDataMutabilityStatus::KnownMutable: return EItemDataMutabilityStatus::KnownMutable;
		case EItemDataMutabilityStatus::KnownImmutable: return EItemDataMutabilityStatus::KnownMutable;
		default: ;
		}
	}

	return Super::GetMutabilityStatus();
}

#endif

bool UFilterRule_LogicalAnd::ExecWithLog(const FFaerieItemStackView View, Faerie::ItemData::FFilterLogger& Logger) const
{
	for (auto&& Rule : Rules)
	{
		if (!Rule->ExecWithLog(View, Logger))
		{
			return false;
		}
	}

	return true;
}

bool UFilterRule_LogicalAnd::Exec(const FFaerieItemStackView View) const
{
	for (auto&& Rule : Rules)
	{
		if (!Rule->Exec(View))
		{
			return false;
		}
	}

	return true;
}

#if WITH_EDITOR
EItemDataMutabilityStatus UFilterRule_Condition::GetMutabilityStatus() const
{
	// @TODO
	return Super::GetMutabilityStatus();
}
#endif

bool UFilterRule_Condition::Exec(const FFaerieItemStackView View) const
{
	if (!ConditionRule) return false;
	if (ConditionRule->Exec(View))
	{
		if (!TrueBranch) return false;
		return TrueBranch->Exec(View);
	}
	return FalseBranch;
}

#if WITH_EDITOR
EItemDataMutabilityStatus UFilterRule_Ternary::GetMutabilityStatus() const
{
	// @TODO
	return Super::GetMutabilityStatus();
}
#endif

bool UFilterRule_Ternary::Exec(const FFaerieItemStackView View) const
{
	if (!ConditionRule) return false;
	if (ConditionRule->Exec(View))
	{
		if (!TrueBranch) return false;
		return TrueBranch->Exec(View);
	}
	if (!FalseBranch) return false;
	return FalseBranch->Exec(View);
}

#if WITH_EDITOR
EItemDataMutabilityStatus UFilterRule_LogicalNot::GetMutabilityStatus() const
{
	if (InvertedRule)
	{
		return InvertedRule->GetMutabilityStatus();
	}
	return Super::GetMutabilityStatus();
}
#endif

bool UFilterRule_LogicalNot::Exec(const FFaerieItemStackView View) const
{
	return !InvertedRule->Exec(View);
}

#if WITH_EDITOR
EItemDataMutabilityStatus UFilterRule_Mutability::GetMutabilityStatus() const
{
	return RequireMutable ? EItemDataMutabilityStatus::KnownMutable : EItemDataMutabilityStatus::KnownImmutable;
}
#endif

bool UFilterRule_Mutability::Exec(const FFaerieItemStackView View) const
{
	return View.Item->CanMutate() == RequireMutable;
}

#if WITH_EDITOR
EItemDataMutabilityStatus UFilterRule_MatchTemplate::GetMutabilityStatus() const
{
	if (IsValid(Template))
	{
		return Template->GetPattern()->GetMutabilityStatus();
	}
	return Super::GetMutabilityStatus();
}
#endif

bool UFilterRule_MatchTemplate::ExecWithLog(const FFaerieItemStackView View,
	Faerie::ItemData::FFilterLogger& Logger) const
{
	if (IsValid(Template))
	{
		return Template->TryMatchWithDescriptions(View, Logger.Errors);
	}
	return false;
}

bool UFilterRule_MatchTemplate::Exec(const FFaerieItemStackView View) const
{
	if (IsValid(Template))
	{
		return Template->TryMatch(View);
	}
	return false;
}

#if WITH_EDITOR
EItemDataMutabilityStatus UFilterRule_HasTokens::GetMutabilityStatus() const
{
	for (auto&& TokenClass : TokenClasses)
	{
		if (GetDefault<UFaerieItemToken>(TokenClass)->IsMutable())
		{
			return EItemDataMutabilityStatus::KnownMutable;
		}
	}

	return Super::GetMutabilityStatus();
}
#endif

bool UFilterRule_HasTokens::ExecWithLog(const FFaerieItemStackView View, Faerie::ItemData::FFilterLogger& Logger) const
{
	TArray<TSubclassOf<UFaerieItemToken>> TokenClassesCopy = TokenClasses;

	for (auto&& Tokens = View.Item->GetTokens();
		const TObjectPtr<UFaerieItemToken>& Token : Tokens)
	{
		if (!IsValid(Token)) continue;

		TokenClassesCopy.RemoveAllSwap([Token](const TSubclassOf<UFaerieItemToken>& TokenClass)
			{
				return Token->IsA(TokenClass);
			});
	}

	static const FTextFormat ErrorFormat = LOCTEXT("HasTokens_MissingClassError", "Missing required token of class: '{0}'");

	for (auto&& MissingClass : TokenClassesCopy)
	{
		FFormatOrderedArguments Args;
#if WITH_EDITOR
		Args.Add(MissingClass->GetDisplayNameText());
#else
		Args.Add(FText::FromString(MissingClass->GetName()));
#endif
		Logger.Errors.Add(FText::Format(ErrorFormat, Args));
	}

	return TokenClassesCopy.IsEmpty();
}

bool UFilterRule_HasTokens::Exec(const FFaerieItemStackView View) const
{
	TArray<TSubclassOf<UFaerieItemToken>> TokenClassesCopy = TokenClasses;

	for (auto&& Tokens = View.Item->GetTokens();
		const TObjectPtr<UFaerieItemToken>& Token : Tokens)
	{
		if (!IsValid(Token)) continue;

		TokenClassesCopy.RemoveAllSwap([Token](const TSubclassOf<UFaerieItemToken>& TokenClass)
			{
				return Token->IsA(TokenClass);
			});
	}

	return TokenClassesCopy.IsEmpty();
}

#if WITH_EDITOR
EItemDataMutabilityStatus UFilterRule_Copies::GetMutabilityStatus() const
{
	auto&& Default = Super::GetMutabilityStatus();

	// In cases where only stacks are allowed through, we know that we are only passing immutable data.

	switch (Operator) {
	case ECopiesCompareOperator::Less:				return Default;
	case ECopiesCompareOperator::LessOrEqual:		return Default;
	case ECopiesCompareOperator::Greater:			return EItemDataMutabilityStatus::KnownImmutable;
	case ECopiesCompareOperator::GreaterOrEqual:	return AmountToCompare > 1 ? EItemDataMutabilityStatus::KnownImmutable : Default;
	case ECopiesCompareOperator::Equal:				return AmountToCompare > 1 ? EItemDataMutabilityStatus::KnownImmutable : Default;
	case ECopiesCompareOperator::NotEqual:			return AmountToCompare == 1 ? EItemDataMutabilityStatus::KnownImmutable : Default;
	default: return Default;
	}
}
#endif

bool UFilterRule_Copies::Exec(const FFaerieItemStackView View) const
{
	switch (Operator) {
	case ECopiesCompareOperator::Less:				return View.Copies < AmountToCompare;
	case ECopiesCompareOperator::LessOrEqual:		return View.Copies <= AmountToCompare;
	case ECopiesCompareOperator::Greater:			return View.Copies > AmountToCompare;
	case ECopiesCompareOperator::GreaterOrEqual:	return View.Copies >= AmountToCompare;
	case ECopiesCompareOperator::Equal:				return View.Copies == AmountToCompare;
	case ECopiesCompareOperator::NotEqual:			return View.Copies != AmountToCompare;
	default: return false;
	}
}

#if WITH_EDITOR
EItemDataMutabilityStatus UFilterRule_StackLimit::GetMutabilityStatus() const
{
	auto&& Default = Super::GetMutabilityStatus();

	// In cases where only stacks are allowed through, we know that we are only passing immutable data.

	switch (Operator) {
	case EStackCompareOperator::Less:			return Default;
	case EStackCompareOperator::LessOrEqual:	return Default;
	case EStackCompareOperator::Greater:		return EItemDataMutabilityStatus::KnownImmutable;
	case EStackCompareOperator::GreaterOrEqual:	return AmountToCompare > 1 ? EItemDataMutabilityStatus::KnownImmutable : Default;
	case EStackCompareOperator::Equal:			return AmountToCompare > 1 ? EItemDataMutabilityStatus::KnownImmutable : Default;
	case EStackCompareOperator::NotEqual:		return AmountToCompare == 1 ? EItemDataMutabilityStatus::KnownImmutable : Default;
	case EStackCompareOperator::HasLimit:		return Default;
	case EStackCompareOperator::HasNoLimit:		return EItemDataMutabilityStatus::KnownImmutable;;
	default: return Default;
	}
}
#endif

bool UFilterRule_StackLimit::Exec(const FFaerieItemStackView View) const
{
	if (const int32 Limit = UFaerieStackLimiterToken::GetItemStackLimit(View.Item.Get());
		Limit == Faerie::ItemData::UnlimitedStack)
	{
		switch (Operator) {
		case EStackCompareOperator::Less:			return false;
		case EStackCompareOperator::LessOrEqual:	return false;
		case EStackCompareOperator::Greater:		return true;
		case EStackCompareOperator::GreaterOrEqual:	return true;
		case EStackCompareOperator::Equal:			return false;
		case EStackCompareOperator::NotEqual:		return true;
		case EStackCompareOperator::HasLimit:		return false;
		case EStackCompareOperator::HasNoLimit:		return true;
		default: return false;
		}
	}
	else
	{
		switch (Operator) {
		case EStackCompareOperator::Less:			return Limit < AmountToCompare;
		case EStackCompareOperator::LessOrEqual:	return Limit <= AmountToCompare;
		case EStackCompareOperator::Greater:		return Limit > AmountToCompare;
		case EStackCompareOperator::GreaterOrEqual:	return Limit >= AmountToCompare;
		case EStackCompareOperator::Equal:			return Limit == AmountToCompare;
		case EStackCompareOperator::NotEqual:		return Limit != AmountToCompare;
		case EStackCompareOperator::HasLimit:		return true;
		case EStackCompareOperator::HasNoLimit:		return false;
		default: return false;
		}
	}
}

bool UFilterRule_GameplayTagAny::Exec(const FFaerieItemStackView View) const
{
	if (const UFaerieTagToken* TagToken = View.Item->GetToken<UFaerieTagToken>())
	{
		return TagToken->GetTags().HasAny(Tags);
	}
	return false;
}

bool UFilterRule_GameplayTagAll::Exec(const FFaerieItemStackView View) const
{
	if (const UFaerieTagToken* TagToken = View.Item->GetToken<UFaerieTagToken>())
	{
		return TagToken->GetTags().HasAll(Tags);
	}
	return false;
}

#undef LOCTEXT_NAMESPACE