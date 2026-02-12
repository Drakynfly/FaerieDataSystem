// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Curves/CurveFloat.h"
#include "FaerieItemSource.h"
#include "StructUtils/InstancedStruct.h"
#include "ItemSlotHandle.h"

#include "FaerieGenerationStructs.generated.h"

USTRUCT(BlueprintType)
struct FAERIEITEMGENERATOR_API FFaerieTableDrop
{
	GENERATED_BODY()

	// Base asset to draw parameters from.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TableDrop")
	FFaerieItemSourceObject Asset;

	// Used to fill Required/Optional Slots for graph-based instances.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TableDrop", meta = (ForceInlineRow))
	TMap<FFaerieItemSlotHandle, TInstancedStruct<struct FFaerieTableDrop>> StaticResourceSlots;

	TOptional<FFaerieItemStack> Resolve(const struct FFaerieItemInstancingContext_Crafting& Context) const;

	bool IsValid() const
	{
		return !Asset.Object.IsNull();
	}

	// Only checks if asset is the same, ignores mutators.
	[[nodiscard]] UE_REWRITE bool UEOpEquals(const FFaerieTableDrop& Other) const
	{
		return Asset.Object == Other.Asset.Object;
	}
};

USTRUCT(BlueprintType)
struct FAERIEITEMGENERATOR_API FFaerieWeightedDrop
{
	GENERATED_BODY()

#if WITH_EDITORONLY_DATA
	// Weight value used in the editor to calculate AdjustedWeight. See FFaerieWeightedDropPool::CalculatePercentages
	UPROPERTY(EditAnywhere, meta = (ClampMin = 1))
	int32 Weight = 1;
#endif

	// Weight to select this drop from table of weighted drops. Used to binary search function.
	UPROPERTY(VisibleAnywhere)
	double AdjustedWeight = 0.0;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Transient)
	float PercentageChanceToDrop = 0.f;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FFaerieTableDrop Drop;

	[[nodiscard]] UE_REWRITE bool UEOpEquals(const FFaerieWeightedDrop& Other) const
	{
		return Drop == Other.Drop;
	}
};


USTRUCT()
struct FFaerieWeightedPool
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Table")
	TArray<FFaerieWeightedDrop> DropList;

	// Generates a drop from this pool, using the provided random weight, which must be a value between 0 and 1.
	const FFaerieTableDrop* GetDrop(double RanWeight) const;

#if WITH_EDITOR
	// Calculate the percentage each drop has to be chosen.
	void CalculatePercentages();

	// Keeps the table sorted by Weight.
	void SortTable();
#endif
};

namespace Faerie
{
	struct FPendingItemGeneration
	{
		// The drop that was generated.
		const FFaerieTableDrop* Drop = nullptr;

		// Initial amount of items dropped. May be split later into multiple entries, if this number is larger than stack
		// limit on the asset.
		int32 Count = 0;

		bool IsValid() const
		{
			return Drop && Drop->IsValid() && Count > 0;
		}
	};
}

class USquirrel;

USTRUCT(BlueprintType, meta = (HideDropdown))
struct FAERIEITEMGENERATOR_API FFaerieGenerationProcedureBase
{
	GENERATED_BODY()

	virtual ~FFaerieGenerationProcedureBase() = default;

	virtual void Resolve(const FFaerieWeightedPool& Pool, USquirrel* Squirrel,
		TArray<Faerie::FPendingItemGeneration>& Pending, int32 Amount) const
		PURE_VIRTUAL(FGeneratorProcedureBase::Resolve, )
};

/*
 * Generate X items of a single type.
 */
USTRUCT(BlueprintType, meta = (DisplayName = "X of 1"))
struct FAERIEITEMGENERATOR_API FFaerieGenerationProcedure_OfOne final : public FFaerieGenerationProcedureBase
{
	GENERATED_BODY()

	virtual void Resolve(const FFaerieWeightedPool& Pool, USquirrel* Squirrel,
		TArray<Faerie::FPendingItemGeneration>& Pending, int32 Amount) const override;
};

/*
 * Generate X total items of any number of types, random selected from the pool.
 */
USTRUCT(BlueprintType, meta = (DisplayName = "X of Any"))
struct FAERIEITEMGENERATOR_API FFaerieGenerationProcedure_OfAny final : public FFaerieGenerationProcedureBase
{
	GENERATED_BODY()

	virtual void Resolve(const FFaerieWeightedPool& Pool, USquirrel* Squirrel,
		TArray<Faerie::FPendingItemGeneration>& Pending, int32 Amount) const override;
};

/*
 * Generate X total items of any number of types, preferring to create stacks.
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Chunked"))
struct FAERIEITEMGENERATOR_API FFaerieGenerationProcedure_Chunked final : public FFaerieGenerationProcedureBase
{
	GENERATED_BODY()

	virtual void Resolve(const FFaerieWeightedPool& Pool, USquirrel* Squirrel,
		TArray<Faerie::FPendingItemGeneration>& Pending, int32 Amount) const override;
};

/*
 * Generate X items of each type in the pool.
 */
USTRUCT(BlueprintType, meta = (DisplayName = "X of All"))
struct FAERIEITEMGENERATOR_API FFaerieGenerationProcedure_OfAll final : public FFaerieGenerationProcedureBase
{
	GENERATED_BODY()

	virtual void Resolve(const FFaerieWeightedPool& Pool, USquirrel* Squirrel,
		TArray<Faerie::FPendingItemGeneration>& Pending, int32 Amount) const override;
};


USTRUCT(BlueprintType, meta = (HideDropdown))
struct FAERIEITEMGENERATOR_API FFaerieGeneratorAmountBase
{
	GENERATED_BODY()

	virtual ~FFaerieGeneratorAmountBase() = default;

	virtual int32 Resolve(USquirrel* Squirrel) const
		PURE_VIRTUAL(FGeneratorAmountBase::Resolve, return -1; )
};

/** Fixed amount of items to generate from this drop. */
USTRUCT(BlueprintType, meta = (DisplayName = "Fixed"))
struct FFaerieGeneratorAmount_Fixed final : public FFaerieGeneratorAmountBase
{
	GENERATED_BODY()

	virtual int32 Resolve(USquirrel* Squirrel) const override { return AmountInt; }

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Fixed Amount", meta = (ClampMin = "1"))
	int32 AmountInt = 1;
};

/** Random value between a min and max. */
USTRUCT(BlueprintType, meta = (DisplayName = "Range"))
struct FFaerieGeneratorAmount_Range final : public FFaerieGeneratorAmountBase
{
	GENERATED_BODY()

	virtual int32 Resolve(USquirrel* Squirrel) const override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Range Amount", meta = (ClampMin = "0"))
	int32 AmountMin = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Range Amount", meta = (ClampMin = "1"))
	int32 AmountMax = 3;
};

/**
* Graph to determine the number of drops.
* A single point on a whole number will guarantee the amount.
* A single point on a fractional point will generate an amount equal to the whole amount, or the whole amount +1, biased by the fraction.
* Multiple points will cause it to pull a number from any time on the curve.
*/
USTRUCT(BlueprintType, meta = (DisplayName = "Curve"))
struct FFaerieGeneratorAmount_Curve final : public FFaerieGeneratorAmountBase
{
	GENERATED_BODY()

	virtual int32 Resolve(USquirrel* Squirrel) const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generator", meta = (XAxisName = "Chance", YAxisName = "Amount", TimeLineLength = "1.0"))
	FRuntimeFloatCurve AmountCurve;
};