﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieGridEnums.h"
#include "InventoryDataStructs.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "FaerieFastArraySerializerHack.h"

#include "FaerieGridStructs.generated.h"

USTRUCT(BlueprintType)
struct FFaerieGridPlacement
{
	GENERATED_BODY()

	FFaerieGridPlacement() = default;

	explicit FFaerieGridPlacement(const FIntPoint Origin)
	  : Origin(Origin) {}

	FFaerieGridPlacement(const FIntPoint Origin, const ESpatialItemRotation Rotation)
	  : Origin(Origin),
		Rotation(Rotation) {}

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "FaerieGridPlacement")
	FIntPoint Origin = FIntPoint::NoneValue;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "FaerieGridPlacement")
	ESpatialItemRotation Rotation = ESpatialItemRotation::None;

	friend bool operator==(const FFaerieGridPlacement& A, const FFaerieGridPlacement& B)
	{
		return A.Origin == B.Origin &&
			A.Rotation == B.Rotation;
	}

	friend bool operator<(const FFaerieGridPlacement& A, const FFaerieGridPlacement& B)
	{
		return A.Origin.X < B.Origin.X || (A.Origin.X == B.Origin.X && A.Origin.Y < B.Origin.Y);
	}
};

struct FFaerieGridContent;

USTRUCT(BlueprintType)
struct FFaerieGridKeyedStack : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FFaerieGridKeyedStack() = default;

	FFaerieGridKeyedStack(const FInventoryKey Key, const FFaerieGridPlacement& Value)
	  : Key(Key), Value(Value) {}

	UPROPERTY(VisibleInstanceOnly, Category = "GridKeyedStack")
	FInventoryKey Key;

	UPROPERTY(VisibleInstanceOnly, Category = "GridKeyedStack")
	FFaerieGridPlacement Value;

	void PreReplicatedRemove(const FFaerieGridContent& InArraySerializer);
	void PostReplicatedAdd(FFaerieGridContent& InArraySerializer);
	void PostReplicatedChange(const FFaerieGridContent& InArraySerializer);
};


USTRUCT(BlueprintType)
struct FFaerieGridContent : public FFastArraySerializer,
							public TBinarySearchOptimizedArray<FFaerieGridContent, FFaerieGridKeyedStack>
{
	GENERATED_BODY()

	friend TBinarySearchOptimizedArray;
	friend class UInventoryGridExtensionBase;

private:
	UPROPERTY(VisibleAnywhere, Category = "FaerieSpatialGridContent")
	TArray<FFaerieGridKeyedStack> Items;

	TArray<FFaerieGridKeyedStack>& GetArray() { return Items; }

	/** Owning extension to send Fast Array callbacks to */
	UPROPERTY()
	TWeakObjectPtr<UInventoryGridExtensionBase> ChangeListener;

public:
	bool EditItem(FInventoryKey Key, const TFunctionRef<bool(FFaerieGridPlacement&)>& Func);

	template <typename Predicate>
	const FFaerieGridKeyedStack* FindByPredicate(Predicate Pred) const
	{
		return Items.FindByPredicate(Pred);
	}

	void PreStackReplicatedRemove(const FFaerieGridKeyedStack& Stack) const;
	void PostStackReplicatedAdd(const FFaerieGridKeyedStack& Stack);
	void PostStackReplicatedChange(const FFaerieGridKeyedStack& Stack) const;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return Faerie::Hacks::FastArrayDeltaSerialize<FFaerieGridKeyedStack, FFaerieGridContent>(Items, DeltaParms, *this);
	}

	void Insert(FInventoryKey Key, const FFaerieGridPlacement& Value);

	void Remove(FInventoryKey Key);

	// Only const iteration is allowed.
	using TRangedForConstIterator = TArray<FFaerieGridKeyedStack>::RangedForConstIteratorType;
	FORCEINLINE TRangedForConstIterator begin() const { return TRangedForConstIterator(Items.begin()); }
	FORCEINLINE TRangedForConstIterator end() const { return TRangedForConstIterator(Items.end()); }
};

template <>
struct TStructOpsTypeTraits<FFaerieGridContent> : TStructOpsTypeTraitsBase2<FFaerieGridContent>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};