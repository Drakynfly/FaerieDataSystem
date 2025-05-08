// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemTemplate.h"
#include "FaerieItemDataFilter.h"
#include "GameplayTagContainer.h"

#include "BasicItemDataFilters.generated.h"

/**
 * Automatic success when not inverted. Automatic failure when inverted.
 */
UCLASS(meta = (DisplayName = "Literal"))
class UFilterRule_Literal : public UFaerieItemDataFilter
{
	GENERATED_BODY()

public:
	virtual bool Exec(FFaerieItemStackView View) const override { return true; }
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
	virtual EItemDataMutabilityStatus GetMutabilityStatus() const override;
#endif

	virtual bool ExecWithLog(FFaerieItemStackView View, Faerie::ItemData::FFilterLogger& Logger) const override;
	virtual bool Exec(FFaerieItemStackView View) const override;

protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Instanced, Category = "Inventory Filter")
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
	virtual EItemDataMutabilityStatus GetMutabilityStatus() const override;
#endif

	virtual bool ExecWithLog(FFaerieItemStackView View, Faerie::ItemData::FFilterLogger& Logger) const override;
	virtual bool Exec(FFaerieItemStackView View) const override;

protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Instanced, Category = "Inventory Filter")
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
	virtual EItemDataMutabilityStatus GetMutabilityStatus() const override;
#endif

	virtual bool Exec(FFaerieItemStackView View) const override;

protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Instanced, Category = "Condition")
	TObjectPtr<UFaerieItemDataFilter> ConditionRule;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Instanced, Category = "Condition")
	TObjectPtr<UFaerieItemDataFilter> TrueBranch;

	// Result if Condition fails.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Condition")
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
	virtual EItemDataMutabilityStatus GetMutabilityStatus() const override;
#endif

	virtual bool Exec(FFaerieItemStackView View) const override;

protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Instanced, Category = "Ternary")
	TObjectPtr<UFaerieItemDataFilter> ConditionRule;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Instanced, Category = "Ternary")
	TObjectPtr<UFaerieItemDataFilter> TrueBranch;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Instanced, Category = "Ternary")
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
	virtual EItemDataMutabilityStatus GetMutabilityStatus() const override;
#endif

	virtual bool Exec(FFaerieItemStackView View) const override;

protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Instanced, Category = "LogicalNot")
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
	virtual EItemDataMutabilityStatus GetMutabilityStatus() const override;
#endif

	virtual bool Exec(FFaerieItemStackView View) const override;

protected:
	// Enable to require a mutable entry. Leave disabled to only allow immutable entries.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Stack Compare")
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
	virtual EItemDataMutabilityStatus GetMutabilityStatus() const override;
#endif

	virtual bool ExecWithLog(const FFaerieItemStackView View, Faerie::ItemData::FFilterLogger& Logger) const override;
	virtual bool Exec(FFaerieItemStackView View) const override;

protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Stack Compare", meta = (AllowAbstract))
	TObjectPtr<UFaerieItemTemplate> Template;
};


class UFaerieItemToken;

/**
 * Filter entries by their tokens
 */
UCLASS(meta = (DisplayName = "Has Token(s)"))
class UFilterRule_HasTokens : public UFaerieItemDataFilter
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual EItemDataMutabilityStatus GetMutabilityStatus() const override;
#endif

	virtual bool ExecWithLog(const FFaerieItemStackView View, Faerie::ItemData::FFilterLogger& Logger) const override;
	virtual bool Exec(FFaerieItemStackView View) const override;

protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Find Token", meta = (AllowAbstract = "true"))
	TArray<TSubclassOf<UFaerieItemToken>> TokenClasses;
};

UENUM()
enum class ECopiesCompareOperator : uint8
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
UCLASS(meta = (DisplayName = "Compare Copies"))
class UFilterRule_Copies : public UFaerieItemDataFilter
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual EItemDataMutabilityStatus GetMutabilityStatus() const override;
#endif

	virtual bool Exec(FFaerieItemStackView View) const override;

protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "CompareCopies")
	ECopiesCompareOperator Operator;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "CompareCopies", meta = (ClampMin = 1))
	int32 AmountToCompare = 1;
};


UENUM()
enum class EStackCompareOperator : uint8
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
UCLASS(meta = (DisplayName = "Compare Limit"))
class UFilterRule_StackLimit : public UFaerieItemDataFilter
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual EItemDataMutabilityStatus GetMutabilityStatus() const override;
#endif

	virtual bool Exec(FFaerieItemStackView View) const override;

protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "CompareLimit")
	EStackCompareOperator Operator;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "CompareLimit",
		meta = (ClampMin = 1, EditCondition = "Operator != EStackCompareOperator::HasLimit && Operator != EStackCompareOperator::HasNoLimit", EditConditionHides))
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
	//virtual EItemDataMutabilityStatus GetMutabilityStatus() const override;
#endif

	virtual bool Exec(FFaerieItemStackView View) const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faerie|TagToken")
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
	//virtual EItemDataMutabilityStatus GetMutabilityStatus() const override;
#endif

	virtual bool Exec(FFaerieItemStackView View) const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faerie|TagToken")
	FGameplayTagContainer Tags;
};