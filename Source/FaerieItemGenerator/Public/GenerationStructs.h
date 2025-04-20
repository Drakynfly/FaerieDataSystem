// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemSource.h"
#include "StructUtils/InstancedStruct.h"
#include "ItemSlotHandle.h"

#include "GenerationStructs.generated.h"

USTRUCT(BlueprintType)
struct FRecursiveTableDrop
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RecursiveTableDrop")
	TInstancedStruct<struct FTableDrop> Drop;
};

USTRUCT(BlueprintType)
struct FAERIEITEMGENERATOR_API FTableDrop
{
	GENERATED_BODY()

	// Base asset to draw parameters from.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TableDrop")
	FFaerieItemSourceObject Asset;

	// Used to fill Required/Optional Slots for graph-based instances.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TableDrop", meta = (ForceInlineRow))
	TMap<FFaerieItemSlotHandle, FRecursiveTableDrop> StaticResourceSlots;

	// @todo make this callable via blueprints at some point...
	UFaerieItem* Resolve(const class UItemInstancingContext_Crafting* Context) const;

	bool IsValid() const
	{
		return !Asset.Object.IsNull();
	}

	// Only checks if asset is the same, ignores mutators.
	friend bool operator==(const FTableDrop& Lhs, const FTableDrop& Rhs)
	{
		return Lhs.Asset.Object == Rhs.Asset.Object;
	}

	friend bool operator!=(const FTableDrop& Lhs, const FTableDrop& Rhs)
	{
		return !(Lhs == Rhs);
	}
};

USTRUCT(BlueprintType)
struct FAERIEITEMGENERATOR_API FWeightedDrop
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
	FTableDrop Drop;

	friend bool operator==(const FWeightedDrop& Lhs, const FWeightedDrop& Rhs)
	{
		return Lhs.Drop == Rhs.Drop;
	}

	friend bool operator!=(const FWeightedDrop& Lhs, const FWeightedDrop& Rhs)
	{
		return !(Lhs == Rhs);
	}
};

class USquirrel;

USTRUCT(BlueprintType, meta = (HideDropdown))
struct FAERIEITEMGENERATOR_API FGeneratorAmountBase
{
	GENERATED_BODY()

	virtual ~FGeneratorAmountBase() = default;

	virtual int32 Resolve(USquirrel* Squirrel) const
		PURE_VIRTUAL(FGeneratorAmountBase::Resolve, return -1; )
};

/** Fixed amount of items to generate from this drop. */
USTRUCT(BlueprintType, meta = (DisplayName = "Fixed"))
struct FGeneratorAmount_Fixed final : public FGeneratorAmountBase
{
	GENERATED_BODY()

	virtual int32 Resolve(USquirrel* Squirrel) const override { return AmountInt; }

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Fixed Amount", meta = (ClampMin = "1"))
	int32 AmountInt = 1;
};

/** Random value between a min and max. */
USTRUCT(BlueprintType, meta = (DisplayName = "Range"))
struct FGeneratorAmount_Range final : public FGeneratorAmountBase
{
	GENERATED_BODY()

	virtual int32 Resolve(USquirrel* Squirrel) const override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Range Amount", meta = (ClampMin = "1"))
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
struct FGeneratorAmount_Curve final : public FGeneratorAmountBase
{
	GENERATED_BODY()

	virtual int32 Resolve(USquirrel* Squirrel) const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generator", meta = (XAxisName = "Chance", YAxisName = "Amount", TimeLineLength = "1.0"))
	FRuntimeFloatCurve AmountCurve;
};