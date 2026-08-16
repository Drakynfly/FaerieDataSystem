// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemTemplate.h"
#include "FaerieItemDataFilter.h"
#include "GameplayTagContainer.h"

#include "Templates/SubclassOf.h"
#include "Templates/SubScriptStructOf.h"

#include "BasicItemDataFilters.generated.h"

struct FFaerieMassFragment;

/**
 * Automatic success when not inverted. Automatic failure when inverted.
 */
USTRUCT()
struct FFaerieItemFilter_Literal final : public FFaerieItemDataFilterBase
{
	GENERATED_BODY()

	virtual bool Exec(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy) const override { return true; }
};

/**
 * Matches when its child does not.
 */
USTRUCT()
struct FFaerieItemFilter_Not final : public FFaerieItemDataFilterBase
{
	GENERATED_BODY()

	virtual bool Exec(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy) const override;

#if WITH_EDITOR
	virtual bool ExecWithLog(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy, Faerie::ItemData::FFilterLogger& Logger) const override;
	virtual EFaerieItemDataMutabilityStatus GetMutabilityStatus() const override;
#endif

	UPROPERTY(EditAnywhere, Category = "Ternary")
	TInstancedStruct<FFaerieItemDataFilterBase> InvertedFilter;
};

/**
 * Matches when one of its rules succeeds
 */
USTRUCT()
struct FFaerieItemFilter_Or final : public FFaerieItemDataFilterBase
{
	GENERATED_BODY()

	virtual bool Exec(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy) const override;

#if WITH_EDITOR
	virtual bool ExecWithLog(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy, Faerie::ItemData::FFilterLogger& Logger) const override;
	virtual EFaerieItemDataMutabilityStatus GetMutabilityStatus() const override;
#endif

	UPROPERTY(EditAnywhere, Category = "FilterOr")
	TArray<TInstancedStruct<FFaerieItemDataFilterBase>> Rules;
};

/**
 * Matches when all of its rules succeeds
 */
USTRUCT()
struct FFaerieItemFilter_And final : public FFaerieItemDataFilterBase
{
	GENERATED_BODY()

	virtual bool Exec(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy) const override;

#if WITH_EDITOR
	virtual bool ExecWithLog(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy, Faerie::ItemData::FFilterLogger& Logger) const override;
	virtual EFaerieItemDataMutabilityStatus GetMutabilityStatus() const override;
#endif

	UPROPERTY(EditAnywhere, Category = "FilterAnd")
	TArray<TInstancedStruct<FFaerieItemDataFilterBase>> Rules;
};

/**
 * Evaluates one filter, to determine if it runs another
 */
USTRUCT()
struct FFaerieItemFilter_Conditional final : public FFaerieItemDataFilterBase
{
	GENERATED_BODY()

	virtual bool Exec(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy) const override;

#if WITH_EDITOR
	virtual EFaerieItemDataMutabilityStatus GetMutabilityStatus() const override;
#endif

	UPROPERTY(EditAnywhere, Category = "Condition")
	TInstancedStruct<FFaerieItemDataFilterBase> ConditionRule;

	UPROPERTY(EditAnywhere, Category = "Condition")
	TInstancedStruct<FFaerieItemDataFilterBase> TrueBranch;

	// Result if Condition fails.
	UPROPERTY(EditAnywhere, Category = "Condition")
	bool FalseBranch = true;
};

/**
 * Evaluates one filter, to determine which of two others to run
 */
USTRUCT()
struct FFaerieItemFilter_Ternary final : public FFaerieItemDataFilterBase
{
	GENERATED_BODY()

	virtual bool Exec(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy) const override;

#if WITH_EDITOR
	virtual EFaerieItemDataMutabilityStatus GetMutabilityStatus() const override;
#endif

	UPROPERTY(EditAnywhere, Category = "Ternary")
	TInstancedStruct<FFaerieItemDataFilterBase> ConditionRule;

	UPROPERTY(EditAnywhere, Category = "Ternary")
	TInstancedStruct<FFaerieItemDataFilterBase> TrueBranch;

	UPROPERTY(EditAnywhere, Category = "Ternary")
	TInstancedStruct<FFaerieItemDataFilterBase> FalseBranch;
};

/**
 * Filter rule that checks against the item's data mutability status
 */
USTRUCT()
struct FFaerieItemFilter_Mutability final : public FFaerieItemDataFilterBase
{
	GENERATED_BODY()

	virtual bool Exec(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy) const override;

#if WITH_EDITOR
	virtual EFaerieItemDataMutabilityStatus GetMutabilityStatus() const override;
#endif

	// Enable to require a mutable entry. Leave disabled to only allow immutable entries.
	UPROPERTY(EditAnywhere, Category = "Mutability")
	bool RequireMutable = true;
};

/**
 * Filter rule for matching a Template Asset
 */
USTRUCT()
struct FFaerieItemFilter_MatchTemplate final : public FFaerieItemDataFilterBase
{
	GENERATED_BODY()

	virtual bool Exec(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy) const override;

#if WITH_EDITOR
	virtual bool ExecWithLog(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy, Faerie::ItemData::FFilterLogger& Logger) const override;
	virtual EFaerieItemDataMutabilityStatus GetMutabilityStatus() const override;
#endif

	UPROPERTY(EditAnywhere, Category = "MatchTemplate", meta = (AllowAbstract))
	TObjectPtr<UFaerieItemTemplate> Template;
};

/**
 * Filter entries by their fragments
 */
USTRUCT()
struct FFaerieItemFilter_HasFragments final : public FFaerieItemDataFilterBase
{
	GENERATED_BODY()

	FFaerieItemFilter_HasFragments();

	virtual bool Exec(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy) const override;

#if WITH_EDITOR
	virtual bool ExecWithLog(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy, Faerie::ItemData::FFilterLogger& Logger) const override;
	virtual EFaerieItemDataMutabilityStatus GetMutabilityStatus() const override;
#endif

	UPROPERTY(EditAnywhere, Category = "HasFragments")
	TArray<TSubScriptStructOf<FFaerieMassFragment>> FragmentTypes;

	// Search for referenced fragments under this tag.
	UPROPERTY(EditAnywhere, Category = "HasFragments")
	FGameplayTag ReferenceTag;
};

/**
 * Filter entries by imposing requirements on its Stack.
 */
USTRUCT()
struct FFaerieItemFilter_Copies final : public FFaerieItemDataFilterBase
{
	GENERATED_BODY()

	virtual bool Exec(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy) const override;

#if WITH_EDITOR
	virtual EFaerieItemDataMutabilityStatus GetMutabilityStatus() const override;
#endif

	UPROPERTY(EditAnywhere, Category = "CompareCopies")
	EFaerieCopiesCompareOperator Operator;

	UPROPERTY(EditAnywhere, Category = "CompareCopies", meta = (ClampMin = 1))
	int32 AmountToCompare = 1;
};

/**
 * Filter entries by its stack limit
 */
USTRUCT()
struct FFaerieItemFilter_StackLimit final : public FFaerieItemDataFilterBase
{
	GENERATED_BODY()

	virtual bool Exec(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy) const override;

#if WITH_EDITOR
	virtual EFaerieItemDataMutabilityStatus GetMutabilityStatus() const override;
#endif

	UPROPERTY(EditAnywhere, Category = "CompareLimit")
	EFaerieStackCompareOperator Operator;

	UPROPERTY(EditAnywhere, Category = "CompareLimit",
		meta = (ClampMin = 1, EditCondition = "Operator != EFaerieStackCompareOperator::HasLimit && Operator != EFaerieStackCompareOperator::HasNoLimit", EditConditionHides))
	int32 AmountToCompare = 1;
};

/**
 * Filter by a specific asset name.
 */
USTRUCT()
struct FFaerieItemFilter_CompareName final : public FFaerieItemDataFilterBase
{
	GENERATED_BODY()

	virtual bool Exec(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy) const override;

	UPROPERTY(EditAnywhere, Category = "CompareName")
	FText CompareText;

	ETextComparisonLevel::Type ComparisonType = ETextComparisonLevel::Default;
};

/**
 * Filter by gameplay tag "any" query
 */
USTRUCT()
struct FFaerieItemFilter_GameplayTagAny final : public FFaerieItemDataFilterBase
{
	GENERATED_BODY()

	virtual bool Exec(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy) const override;

	UPROPERTY(EditAnywhere, Category = "GameplayTagAny")
	FGameplayTagContainer Tags;

	UPROPERTY(EditAnywhere, Category = "GameplayTagAny")
	bool Exact = false;
};

/**
 * Filter by gameplay tag "all" query
 */
USTRUCT()
struct FFaerieItemFilter_GameplayTagAll final : public FFaerieItemDataFilterBase
{
	GENERATED_BODY()

	virtual bool Exec(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy) const override;

	UPROPERTY(EditAnywhere, Category = "GameplayTagAll")
	FGameplayTagContainer Tags;

	UPROPERTY(EditAnywhere, Category = "GameplayTagAll")
	bool Exact = false;
};