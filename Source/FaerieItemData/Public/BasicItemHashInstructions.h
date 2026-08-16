// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieHash.h"
#include "FaerieMassFragment.h"
#include "FaerieItemStackHashInstruction.h"

#include "Templates/SubclassOf.h"
#include "Templates/SubScriptStructOf.h"

#include "BasicItemHashInstructions.generated.h"

struct FFaerieItemDataFilterBase;

/**
 * Inject a manually defined hash.
 */
USTRUCT()
struct FAERIEITEMDATA_API FISHI_Literal : public FFaerieItemDataHashInstruction
{
	GENERATED_BODY()

	virtual uint32 Hash(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> View) const override;

protected:
	UPROPERTY(EditAnywhere, Category = "FISHI")
	FFaerieHash Value;
};

/**
 * Performs a Hash Combine of multiple instructions.
 */
USTRUCT()
struct FAERIEITEMDATA_API FISHI_And : public FFaerieItemDataHashInstruction
{
	GENERATED_BODY()

	virtual uint32 Hash(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> View) const override;

protected:
	UPROPERTY(EditAnywhere, Category = "FISHI")
	TArray<TInstancedStruct<FFaerieItemDataHashInstruction>> Instructions;
};

/**
 * Returns the hash for the first instruction to not fail
 */
USTRUCT()
struct FAERIEITEMDATA_API FISHI_Or : public FFaerieItemDataHashInstruction
{
	GENERATED_BODY()

	virtual uint32 Hash(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> View) const override;

protected:
	UPROPERTY(EditAnywhere, Category = "FISHI")
	TArray<TInstancedStruct<FFaerieItemDataHashInstruction>> Instructions;
};

/**
 * Match against an ItemDataFilter, returning one of two predefined hashes.
 */
USTRUCT()
struct FAERIEITEMDATA_API FISHI_BooleanFilter : public FFaerieItemDataHashInstruction
{
	GENERATED_BODY()

	virtual uint32 Hash(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> View) const override;

protected:
	// Pattern used to determine if an item qualifies as fitting this template.
	UPROPERTY(EditInstanceOnly, Category = "FISHI", meta = (DisplayThumbnail = false))
	TInstancedStruct<FFaerieItemDataFilterBase> Pattern;
};

/**
 * Select between two other instructions to use, based on the result of an ItemDataFilter.
 */
USTRUCT()
struct FAERIEITEMDATA_API FISHI_BooleanSelect : public FFaerieItemDataHashInstruction
{
	GENERATED_BODY()

	virtual uint32 Hash(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> View) const override;

protected:
	// Pattern used to determine if an item qualifies as fitting this template.
	UPROPERTY(EditInstanceOnly, Category = "FISHI", meta = (DisplayThumbnail = false))
	TInstancedStruct<FFaerieItemDataFilterBase> Pattern;

	UPROPERTY(EditAnywhere, Category = "FISHI")
	TInstancedStruct<FFaerieItemDataHashInstruction> False;

	UPROPERTY(EditAnywhere, Category = "FISHI")
	TInstancedStruct<FFaerieItemDataHashInstruction> True;
};

/**
 * Hashes a set of fragments together by their object hash.
 */
USTRUCT()
struct FAERIEITEMDATA_API FISHI_Fragments : public FFaerieItemDataHashInstruction
{
	GENERATED_BODY()

	virtual uint32 Hash(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> View) const override;

protected:
	UPROPERTY(EditInstanceOnly, Category = "FISHI", meta = (DisplayThumbnail = false))
	TArray<TSubScriptStructOf<FFaerieMassFragment>> FragmentTypes;
};