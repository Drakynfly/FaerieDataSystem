// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemTemplate.h"
#include "FaerieItemFilter.h"
#include "GameplayTagContainer.h"
#include "SubScriptStructOfMulti.h"

#include "Templates/SubclassOf.h"
#include "Templates/SubScriptStructOf.h"

#include "BasicItemFilters.generated.h"

struct FFaerieMassFragment;

/**
 * Automatic success when not inverted. Automatic failure when inverted.
 */
USTRUCT()
struct FFaerieItemFilter_Literal final : public FFaerieItemFilterBase
{
	GENERATED_BODY()

	virtual bool Exec(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy) const override { return true; }
};

/**
 * Matches when its child does not.
 */
USTRUCT()
struct FFaerieItemFilter_Not final : public FFaerieItemFilterBase
{
	GENERATED_BODY()

	virtual bool Exec(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy) const override;

#if WITH_EDITOR
	virtual bool ExecWithLog(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy, Faerie::ItemData::FFilterLogger& Logger) const override;
	virtual EFaerieItemFilterMutabilityStatus GetMutabilityStatus() const override;
#endif

	UPROPERTY(EditAnywhere, Category = "Ternary", meta = (ExcludeBaseStruct))
	TInstancedStruct<FFaerieItemFilterBase> InvertedFilter;
};

/**
 * Matches when one of its rules succeeds
 */
USTRUCT()
struct FFaerieItemFilter_Or final : public FFaerieItemFilterBase
{
	GENERATED_BODY()

	virtual bool Exec(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy) const override;

#if WITH_EDITOR
	virtual bool ExecWithLog(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy, Faerie::ItemData::FFilterLogger& Logger) const override;
	virtual EFaerieItemFilterMutabilityStatus GetMutabilityStatus() const override;
#endif

	UPROPERTY(EditAnywhere, Category = "FilterOr", meta = (ExcludeBaseStruct))
	TArray<TInstancedStruct<FFaerieItemFilterBase>> Rules;
};

/**
 * Matches when all of its rules succeeds
 */
USTRUCT()
struct FFaerieItemFilter_And final : public FFaerieItemFilterBase
{
	GENERATED_BODY()

	virtual bool Exec(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy) const override;

#if WITH_EDITOR
	virtual bool ExecWithLog(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy, Faerie::ItemData::FFilterLogger& Logger) const override;
	virtual EFaerieItemFilterMutabilityStatus GetMutabilityStatus() const override;
#endif

	UPROPERTY(EditAnywhere, Category = "FilterAnd", meta = (ExcludeBaseStruct))
	TArray<TInstancedStruct<FFaerieItemFilterBase>> Rules;
};

/**
 * Evaluates one filter, to determine if it runs another
 */
USTRUCT()
struct FFaerieItemFilter_Conditional final : public FFaerieItemFilterBase
{
	GENERATED_BODY()

	virtual bool Exec(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy) const override;

#if WITH_EDITOR
	virtual EFaerieItemFilterMutabilityStatus GetMutabilityStatus() const override;
#endif

	UPROPERTY(EditAnywhere, Category = "Condition", meta = (ExcludeBaseStruct))
	TInstancedStruct<FFaerieItemFilterBase> ConditionRule;

	UPROPERTY(EditAnywhere, Category = "Condition", meta = (ExcludeBaseStruct))
	TInstancedStruct<FFaerieItemFilterBase> TrueBranch;

	// Result if Condition fails.
	UPROPERTY(EditAnywhere, Category = "Condition")
	bool FalseBranch = true;
};

/**
 * Evaluates one filter, to determine which of two others to run
 */
USTRUCT()
struct FFaerieItemFilter_Ternary final : public FFaerieItemFilterBase
{
	GENERATED_BODY()

	virtual bool Exec(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy) const override;

#if WITH_EDITOR
	virtual EFaerieItemFilterMutabilityStatus GetMutabilityStatus() const override;
#endif

	UPROPERTY(EditAnywhere, Category = "Ternary", meta = (ExcludeBaseStruct))
	TInstancedStruct<FFaerieItemFilterBase> ConditionRule;

	UPROPERTY(EditAnywhere, Category = "Ternary", meta = (ExcludeBaseStruct))
	TInstancedStruct<FFaerieItemFilterBase> TrueBranch;

	UPROPERTY(EditAnywhere, Category = "Ternary", meta = (ExcludeBaseStruct))
	TInstancedStruct<FFaerieItemFilterBase> FalseBranch;
};

/**
 * Filter rule that checks against the item's data mutability status
 */
USTRUCT()
struct FFaerieItemFilter_Mutability final : public FFaerieItemFilterBase
{
	GENERATED_BODY()

	virtual bool Exec(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy) const override;

#if WITH_EDITOR
	virtual EFaerieItemFilterMutabilityStatus GetMutabilityStatus() const override;
#endif

	// Enable to require a mutable entry. Leave disabled to only allow immutable entries.
	UPROPERTY(EditAnywhere, Category = "Mutability")
	bool RequireMutable = true;
};

/**
 * Filter rule for matching a Template Asset
 */
USTRUCT()
struct FFaerieItemFilter_MatchTemplate final : public FFaerieItemFilterBase
{
	GENERATED_BODY()

	virtual bool Exec(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy) const override;

#if WITH_EDITOR
	virtual bool ExecWithLog(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy, Faerie::ItemData::FFilterLogger& Logger) const override;
	virtual EFaerieItemFilterMutabilityStatus GetMutabilityStatus() const override;
#endif

	UPROPERTY(EditAnywhere, Category = "MatchTemplate", meta = (AllowAbstract))
	TObjectPtr<UFaerieItemTemplate> Template;
};

/**
 * Filter entries by their fragments
 */
USTRUCT()
struct FFaerieItemFilter_HasFragments final : public FFaerieItemFilterBase
{
	GENERATED_BODY()

	FFaerieItemFilter_HasFragments();

	virtual bool Exec(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy) const override;

#if WITH_EDITOR
	virtual bool ExecWithLog(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy, Faerie::ItemData::FFilterLogger& Logger) const override;
	virtual EFaerieItemFilterMutabilityStatus GetMutabilityStatus() const override;
#endif

	// @Todo move to Fragments
	UPROPERTY(EditAnywhere, Category = "HasFragments")
	TArray<TSubScriptStructOf<FFaerieMassFragment>> FragmentTypes;

	// @todo we also need to check for normal Tags, Chunk Fragments etc
	UPROPERTY(EditAnywhere, Category = "HasFragments", meta = (ExcludeBaseStructs, BaseStructs =
		"/Script/FaerieItemData.FaerieMassFragment,/Script/FaerieItemData.FaerieMassSparseFragment,/Script/FaerieItemData.FaerieMassSparseTag"))
	TArray<FSubScriptStructOfMulti> Fragments;

	// Search for referenced fragments under this tag.
	UPROPERTY(EditAnywhere, Category = "HasFragments")
	FGameplayTag ReferenceTag;
};

UENUM()
enum class EFaerieCopiesCompareOperator : uint8
{
	Less			UMETA(DisplayName = "<"),
	LessOrEqual		UMETA(DisplayName = "<="),
	Greater			UMETA(DisplayName = ">"),
	GreaterOrEqual	UMETA(DisplayName = ">="),
	Equal			UMETA(DisplayName = "=="),
	NotEqual		UMETA(DisplayName = "!="),
};

/**
 * Filter entries by imposing requirements on its Stack.
 */
USTRUCT()
struct FFaerieItemFilter_Copies final : public FFaerieItemFilterBase
{
	GENERATED_BODY()

	virtual bool Exec(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy) const override;

#if WITH_EDITOR
	virtual EFaerieItemFilterMutabilityStatus GetMutabilityStatus() const override;
#endif

	UPROPERTY(EditAnywhere, Category = "CompareCopies")
	EFaerieCopiesCompareOperator Operator = EFaerieCopiesCompareOperator::Less;

	UPROPERTY(EditAnywhere, Category = "CompareCopies", meta = (ClampMin = 1))
	int32 AmountToCompare = 1;
};

/**
 * Filter by a specific asset name.
 */
USTRUCT()
struct FFaerieItemFilter_CompareName final : public FFaerieItemFilterBase
{
	GENERATED_BODY()

	virtual bool Exec(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy) const override;

	UPROPERTY(EditAnywhere, Category = "CompareName")
	FText CompareText;

	ETextComparisonLevel::Type ComparisonType = ETextComparisonLevel::Default;
};

/**
 * Filter by 'HasAny' gameplay tag function
 */
USTRUCT()
struct FFaerieItemFilter_GameplayTagAny final : public FFaerieItemFilterBase
{
	GENERATED_BODY()

	virtual bool Exec(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy) const override;

	UPROPERTY(EditAnywhere, Category = "GameplayTagAny")
	FGameplayTagContainer Tags;

	UPROPERTY(EditAnywhere, Category = "GameplayTagAny")
	bool Exact = false;
};

/**
 * Filter by 'HasAll' gameplay tag function
 */
USTRUCT()
struct FFaerieItemFilter_GameplayTagAll final : public FFaerieItemFilterBase
{
	GENERATED_BODY()

	virtual bool Exec(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy) const override;

	UPROPERTY(EditAnywhere, Category = "GameplayTagAll")
	FGameplayTagContainer Tags;

	UPROPERTY(EditAnywhere, Category = "GameplayTagAll")
	bool Exact = false;
};

/**
 * Filter by a full gameplay tag query
 */
USTRUCT()
struct FFaerieItemFilter_GameplayTagQuery final : public FFaerieItemFilterBase
{
	GENERATED_BODY()

	virtual bool Exec(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy) const override;

	UPROPERTY(EditAnywhere, Category = "GameplayTagAll")
	FGameplayTagQuery Query;
};