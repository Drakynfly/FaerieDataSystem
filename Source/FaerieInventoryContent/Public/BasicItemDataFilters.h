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
UCLASS(meta = (DisplayName = "Literal"))
class UFilterRule_Literal : public UFaerieItemDataFilter
{
	GENERATED_BODY()

public:
	virtual bool Exec(TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& View) const override { return true; }
};

/**
 * Matches when any of its rule succeeds
 */
UCLASS(meta = (DisplayName = "Logical Or"))
class UFilterRule_LogicalOr : public UFaerieItemDataFilter
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual EFaerieItemDataMutabilityStatus GetMutabilityStatus() const override;
#endif

	virtual bool ExecWithLog(TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& View, Faerie::ItemData::FFilterLogger& Logger) const override;
	virtual bool Exec(TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& View) const override;

protected:
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Instanced, Category = "LogicalOr")
	TArray<TObjectPtr<UFaerieItemDataFilter>> Rules;
};

/**
 * Matches when all of its rule succeeds
 */
UCLASS(meta = (DisplayName = "Logical And"))
class UFilterRule_LogicalAnd : public UFaerieItemDataFilter
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual EFaerieItemDataMutabilityStatus GetMutabilityStatus() const override;
#endif

	virtual bool ExecWithLog(TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& View, Faerie::ItemData::FFilterLogger& Logger) const override;
	virtual bool Exec(TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& View) const override;

protected:
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Instanced, Category = "LogicalAnd")
	TArray<TObjectPtr<UFaerieItemDataFilter>> Rules;
};

/**
 * Evaluates one filter, to determine if it runs another
 */
UCLASS(meta = (DisplayName = "Condition"))
class UFilterRule_Condition : public UFaerieItemDataFilter
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual EFaerieItemDataMutabilityStatus GetMutabilityStatus() const override;
#endif

	virtual bool Exec(TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& View) const override;

protected:
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Instanced, Category = "Condition")
	TObjectPtr<UFaerieItemDataFilter> ConditionRule;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Instanced, Category = "Condition")
	TObjectPtr<UFaerieItemDataFilter> TrueBranch;

	// Result if Condition fails.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Condition")
	bool FalseBranch = true;
};

/**
 * Evaluates one filter, to determine which of two others to run
 */
UCLASS(meta = (DisplayName = "Ternary Condition"))
class UFilterRule_Ternary : public UFaerieItemDataFilter
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual EFaerieItemDataMutabilityStatus GetMutabilityStatus() const override;
#endif

	virtual bool Exec(TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& View) const override;

protected:
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Instanced, Category = "Ternary")
	TObjectPtr<UFaerieItemDataFilter> ConditionRule;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Instanced, Category = "Ternary")
	TObjectPtr<UFaerieItemDataFilter> TrueBranch;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Instanced, Category = "Ternary")
	TObjectPtr<UFaerieItemDataFilter> FalseBranch;
};

/**
 * Matches when its child does not.
 */
UCLASS(meta = (DisplayName = "Logical Not"))
class UFilterRule_LogicalNot : public UFaerieItemDataFilter
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual EFaerieItemDataMutabilityStatus GetMutabilityStatus() const override;
#endif

	virtual bool Exec(TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& View) const override;

protected:
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Instanced, Category = "LogicalNot")
	TObjectPtr<UFaerieItemDataFilter> InvertedRule;
};

/**
 * Filter rule that checks against the item's data mutability status
 */
UCLASS(meta = (DisplayName = "Mutability"))
class UFilterRule_Mutability : public UFaerieItemDataFilter
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual EFaerieItemDataMutabilityStatus GetMutabilityStatus() const override;
#endif

	virtual bool Exec(TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& View) const override;

protected:
	// Enable to require a mutable entry. Leave disabled to only allow immutable entries.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Mutability")
	bool RequireMutable;
};

/**
 * Filter rule for matching a Template Asset
 */
UCLASS(meta = (DisplayName = "Match Template"))
class UFilterRule_MatchTemplate : public UFaerieItemDataFilter
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual EFaerieItemDataMutabilityStatus GetMutabilityStatus() const override;
#endif

	virtual bool ExecWithLog(TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& View, Faerie::ItemData::FFilterLogger& Logger) const override;
	virtual bool Exec(TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& View) const override;

protected:
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "MatchTemplate", meta = (AllowAbstract))
	TObjectPtr<UFaerieItemTemplate> Template;
};

/**
 * Filter entries by their fragments
 */
UCLASS(meta = (DisplayName = "Has Fragment(s)"))
class UFilterRule_HasFragments : public UFaerieItemDataFilter
{
	GENERATED_BODY()

public:
	UFilterRule_HasFragments();

#if WITH_EDITOR
	virtual EFaerieItemDataMutabilityStatus GetMutabilityStatus() const override;
#endif

	virtual bool ExecWithLog(TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& View, Faerie::ItemData::FFilterLogger& Logger) const override;
	virtual bool Exec(TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& View) const override;

protected:
	UPROPERTY(EditAnywhere, Category = "HasFragments")
	TArray<TSubScriptStructOf<FFaerieMassFragment>> FragmentTypes;

	// Search for referenced fragments under this tag.
	UPROPERTY(EditAnywhere, Category = "HasFragments")
	FGameplayTag ReferenceTag;
};

/**
 * Filter entries by imposing requirements on its Stack.
 */
UCLASS(meta = (DisplayName = "Compare Copies"))
class UFilterRule_Copies : public UFaerieItemDataFilter
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual EFaerieItemDataMutabilityStatus GetMutabilityStatus() const override;
#endif

	virtual bool Exec(TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& View) const override;

protected:
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "CompareCopies")
	EFaerieCopiesCompareOperator Operator;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "CompareCopies", meta = (ClampMin = 1))
	int32 AmountToCompare = 1;
};

/**
 * Filter entries by its stack limit
 */
UCLASS(meta = (DisplayName = "Compare Limit"))
class UFilterRule_StackLimit : public UFaerieItemDataFilter
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual EFaerieItemDataMutabilityStatus GetMutabilityStatus() const override;
#endif

	virtual bool Exec(TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& View) const override;

protected:
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "CompareLimit")
	EFaerieStackCompareOperator Operator;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "CompareLimit",
		meta = (ClampMin = 1, EditCondition = "Operator != EFaerieStackCompareOperator::HasLimit && Operator != EFaerieStackCompareOperator::HasNoLimit", EditConditionHides))
	int32 AmountToCompare = 1;
};

/**
 * Filter by gameplay tag "any" query
 */
UCLASS(meta = (DisplayName = "Any Tags"))
class UFilterRule_GameplayTagAny : public UFaerieItemDataFilter
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	//virtual EFaerieItemDataMutabilityStatus GetMutabilityStatus() const override;
#endif

	virtual bool Exec(TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& View) const override;

protected:
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GameplayTagAny")
	FGameplayTagContainer Tags;
};

/**
 * Filter by gameplay tag "all" query
 */
UCLASS(meta = (DisplayName = "All Tags"))
class UFilterRule_GameplayTagAll : public UFaerieItemDataFilter
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	//virtual EFaerieItemDataMutabilityStatus GetMutabilityStatus() const override;
#endif

	virtual bool Exec(TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& View) const override;

protected:
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GameplayTagAll")
	FGameplayTagContainer Tags;
};