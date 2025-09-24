// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "BinarySearchOptimizedArray.h"
#include "FaerieGridEnums.h"
#include "FaerieDefinitions.h"
#include "FaerieFastArraySerializerHack.h"
#include "FaerieItemContainerStructs.h"

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

	FFaerieGridKeyedStack(const FFaerieAddress Key, const FFaerieGridPlacement& Value)
	  : Key(Key), Value(Value) {}

	UPROPERTY(VisibleInstanceOnly, Category = "GridKeyedStack")
	FFaerieAddress Key;

	UPROPERTY(VisibleInstanceOnly, Category = "GridKeyedStack")
	FFaerieGridPlacement Value;

	void PreReplicatedRemove(const FFaerieGridContent& InArraySerializer);
	void PostReplicatedAdd(FFaerieGridContent& InArraySerializer);
	void PostReplicatedChange(const FFaerieGridContent& InArraySerializer);
};

class UInventoryGridExtensionBase;

USTRUCT(BlueprintType)
struct FFaerieGridContent : public FFaerieFastArraySerializer,
							public TBinarySearchOptimizedArray<FFaerieGridContent, FFaerieGridKeyedStack>
{
	GENERATED_BODY()

	friend TBinarySearchOptimizedArray;
	friend UInventoryGridExtensionBase;

private:
	UPROPERTY(VisibleAnywhere, Category = "FaerieGridContent")
	TArray<FFaerieGridKeyedStack> Items;

	TArray<FFaerieGridKeyedStack>& GetArray() { return Items; }

	/** Owning extension to send Fast Array callbacks to */
	// UPROPERTY() Fast Arrays cannot have additional properties with Iris
	// ReSharper disable once CppUE4ProbableMemoryIssuesWithUObject
	TObjectPtr<UInventoryGridExtensionBase> ChangeListener;

	// Is writing to Items locked? Enabled while StackHandles are active.
	mutable uint32 WriteLock = 0;

public:
	template <typename Predicate>
	const FFaerieGridKeyedStack* FindByPredicate(Predicate Pred) const
	{
		return Items.FindByPredicate(Pred);
	}

	struct FScopedStackHandle : FNoncopyable
	{
		FScopedStackHandle(const FFaerieAddress Key, FFaerieGridContent& Source);
		~FScopedStackHandle();

	protected:
		FFaerieGridKeyedStack& Handle;

	private:
		FFaerieGridContent& Source;

	public:
		FFaerieGridPlacement* operator->() const { return &Handle.Value; }
		FFaerieGridPlacement& Get() const { return Handle.Value; }
	};

	FScopedStackHandle GetHandle(const FFaerieAddress Key)
	{
		return FScopedStackHandle(Key, *this);
	}

	void PreStackReplicatedRemove(const FFaerieGridKeyedStack& Stack) const;
	void PostStackReplicatedAdd(const FFaerieGridKeyedStack& Stack) const;
	void PostStackReplicatedChange(const FFaerieGridKeyedStack& Stack) const;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return Faerie::Hacks::FastArrayDeltaSerialize<FFaerieGridKeyedStack, FFaerieGridContent>(Items, DeltaParms, *this);
	}

	void Insert(FFaerieAddress Key, const FFaerieGridPlacement& Value);

	void Remove(FFaerieAddress Key);

	// Only const iteration is allowed.
	using TRangedForConstIterator = TArray<FFaerieGridKeyedStack>::RangedForConstIteratorType;
	TRangedForConstIterator begin() const;
	TRangedForConstIterator end() const;
};

template <>
struct TStructOpsTypeTraits<FFaerieGridContent> : TStructOpsTypeTraitsBase2<FFaerieGridContent>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};