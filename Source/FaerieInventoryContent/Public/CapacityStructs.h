// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "CapacityStructs.generated.h"

/**
 * A wrapper around an int32 to allow the editor to provide a struct customization
 */
USTRUCT(BlueprintType)
struct FAERIEINVENTORYCONTENT_API FWeightEditor
{
    GENERATED_BODY()

	friend class FItemCapacityCustomization;

    FWeightEditor()
      : Weight(0)
    {}

    FWeightEditor(const int32 Weight)
      : Weight(Weight)
    {}

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weight", meta = (ForceUnits = g, ClampMin = 0, UIMax = 5000))
    int32 Weight;

public:
    operator int32() const { return Weight; }
    const int32& operator*() const { return Weight; }
    int32* operator->() { return &Weight; }
    int32& GetMutable() { return Weight; }
};

/**
 * A wrapper around a float to allow the editor to provide a struct customization
 */
USTRUCT(BlueprintType)
struct FAERIEINVENTORYCONTENT_API FWeightEditor_Float
{
	GENERATED_BODY()

	friend class FInventoryWeightCustomization;
	friend class FItemCapacityCustomization;

	FWeightEditor_Float()
	  : Weight(0)
	{}

	FWeightEditor_Float(const float Weight)
	  : Weight(Weight)
	{}

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weight", meta = (ForceUnits = g, ClampMin = 0, UIMax = 5000))
	float Weight;

public:
	operator float() const { return Weight; }
	const float& operator*() const { return Weight; }
	float* operator->() { return &Weight; }
	float& GetMutable() { return Weight; }
};

USTRUCT(BlueprintType)
struct FAERIEINVENTORYCONTENT_API FItemCapacity
{
    GENERATED_BODY()

    // The weight in grams for a single instance of this entry.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Capacity", meta = (ClampMin = 0, UIMax = 5000))
    FWeightEditor Weight = 100;

    // Physical dimensions in centimeters.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Capacity", meta = (Units = cm, ClampMin = 1))
    FIntVector Bounds = FIntVector(10);

    /**
    * Falloff of size when stacking. A value of 1 means that stack size = size*amount, while a value of 0 means that
    * stacking this item only increases weight, not size.
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Capacity", meta = (ClampMin = 0.f, ClampMax = 1.f, UIMin = 0.01f))
    float Efficiency = 1;

    bool IsInsignificant() const
    {
        return Weight == 0 || FMath::IsNearlyZero(Bounds.GetMin(), 0.01);
    }

    // Get the product of the physical dimensions of this entry.
    int64 GetVolume() const
    {
        return Bounds.X * Bounds.Y * Bounds.Z;
    }

	double GetEfficientVolume() const
    {
    	return GetVolume() * Efficiency;
    }

	// Get the approximate weight for one square centimeter of this capacity.
	double WeightOfSquareCentimeter() const
    {
    	return static_cast<double>(Weight) / GetEfficientVolume();
    }
};


USTRUCT(BlueprintType)
struct FWeightAndVolume
{
	GENERATED_BODY()

	FWeightAndVolume() = default;

	FWeightAndVolume(const int32 Weight, const int64 Volume)
	  : GramWeight(Weight),
		Volume(Volume)
	{}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Units = g))
	int32 GramWeight = 0;

	// Physical dimensions as a product of the bounds in cube centimeters.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0))
	int64 Volume = 0;

	bool IsInsignificant() const
	{
		return GramWeight == 0 && Volume == 0;
	}

	friend bool operator==(const FWeightAndVolume& Lhs, const FWeightAndVolume& Rhs)
	{
		return Lhs.GramWeight == Rhs.GramWeight
			   && Lhs.Volume == Rhs.Volume;
	}

	friend bool operator!=(const FWeightAndVolume& Lhs, const FWeightAndVolume& Rhs)
	{
		return !(Lhs == Rhs);
	}

	FWeightAndVolume& operator+=(const FWeightAndVolume& Other)
	{
		GramWeight += Other.GramWeight;
		Volume += Other.Volume;
		return *this;
	}

	FWeightAndVolume& operator-=(const FWeightAndVolume& Other)
	{
		GramWeight -= Other.GramWeight;
		Volume -= Other.Volume;
		return *this;
	}

	friend FWeightAndVolume operator+(const FWeightAndVolume& A, const FWeightAndVolume& B)
	{
		FWeightAndVolume Out;
		Out.GramWeight = A.GramWeight + B.GramWeight;
		Out.Volume = A.Volume + B.Volume;
		return Out;
	}

	friend FWeightAndVolume operator-(const FWeightAndVolume& A, const FWeightAndVolume& B)
	{
		FWeightAndVolume Out;
		Out.GramWeight = A.GramWeight - B.GramWeight;
		Out.Volume = A.Volume - B.Volume;
		return Out;
	}

	// Negation operator
	friend FWeightAndVolume operator-(const FWeightAndVolume& Other)
	{
		FWeightAndVolume Out;
		Out.GramWeight = -Other.GramWeight;
		Out.Volume = -Other.Volume;
		return Out;
	}
};