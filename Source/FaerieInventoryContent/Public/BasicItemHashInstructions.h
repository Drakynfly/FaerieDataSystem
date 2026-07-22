// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieHash.h"
#include "FaerieMassFragment.h"
#include "FaerieItemStackHashInstruction.h"

#include "Templates/SubclassOf.h"
#include "Templates/SubScriptStructOf.h"

#include "BasicItemHashInstructions.generated.h"

class UFaerieItemDataFilter;


/**
 * Inject a manually defined hash.
 */
UCLASS(DisplayName = "Literal (FISHI)")
class UFISHI_Literial : public UFaerieItemStackHashInstruction
{
	GENERATED_BODY()

public:
	virtual uint32 Hash(TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& View) const override;

protected:
	UPROPERTY(EditAnywhere, Category = "FISHI")
	FFaerieHash Value;
};

/**
 * Performs a Hash Combine of multiple instructions.
 */
UCLASS(DisplayName = "And (FISHI)")
class UFISHI_And : public UFaerieItemStackHashInstruction
{
	GENERATED_BODY()

public:
	virtual uint32 Hash(TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& View) const override;

protected:
	UPROPERTY(EditAnywhere, Instanced, Category = "FISHI")
	TArray<TObjectPtr<UFaerieItemStackHashInstruction>> Instructions;
};


/**
 * Returns the hash for the first instruction to not fail
 */
UCLASS(DisplayName = "Or (FISHI)")
class UFISHI_Or : public UFaerieItemStackHashInstruction
{
	GENERATED_BODY()

public:
	virtual uint32 Hash(TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& View) const override;

protected:
	UPROPERTY(EditAnywhere, Instanced, Category = "FISHI")
	TArray<TObjectPtr<UFaerieItemStackHashInstruction>> Instructions;
};


/**
 * Match against an ItemDataFilter, returning one of two predefined hashes.
 */
UCLASS(DisplayName = "Boolean Filter (FISHI)")
class UFISHI_BooleanFilter : public UFaerieItemStackHashInstruction
{
	GENERATED_BODY()

public:
	virtual uint32 Hash(TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& View) const override;

protected:
	// Pattern used to determine if an item qualifies as fitting this template.
	UPROPERTY(EditInstanceOnly, Category = "FISHI", meta = (DisplayThumbnail = false))
	TObjectPtr<UFaerieItemDataFilter> Pattern;
};

/**
 * Select between two other instructions to use, based on the result of an ItemDataFilter.
 */
UCLASS(DisplayName = "Boolean Select (FISHI)")
class UFISHI_BooleanSelect : public UFaerieItemStackHashInstruction
{
	GENERATED_BODY()

public:
	virtual uint32 Hash(TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& View) const override;

protected:
	// Pattern used to determine if an item qualifies as fitting this template.
	UPROPERTY(EditInstanceOnly, Category = "FISHI", meta = (DisplayThumbnail = false))
	TObjectPtr<UFaerieItemDataFilter> Pattern;

	UPROPERTY(EditAnywhere, Instanced, Category = "FISHI")
	TObjectPtr<UFaerieItemStackHashInstruction> False;

	UPROPERTY(EditAnywhere, Instanced, Category = "FISHI")
	TObjectPtr<UFaerieItemStackHashInstruction> True;
};


/**
 * Hashes a set of fragments together by their object hash.
 */
UCLASS(DisplayName = "Fragments (FISHI)")
class UFISHI_Fragments : public UFaerieItemStackHashInstruction
{
	GENERATED_BODY()

public:
	virtual uint32 Hash(TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& View) const override;

protected:
	UPROPERTY(EditInstanceOnly, Category = "FISHI", meta = (DisplayThumbnail = false))
	TArray<TSubScriptStructOf<FFaerieMassFragment>> FragmentTypes;
};