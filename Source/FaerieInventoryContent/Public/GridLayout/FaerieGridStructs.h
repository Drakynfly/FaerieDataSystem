// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "BinarySearchOptimizedArray.h"
#include "DebuggingFlags.h"
#include "FaerieGridEnums.h"
#include "FaerieFastArraySerializer.h"
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

	FFaerieGridPlacement(const FIntPoint Origin, const EFaerieSpatialItemRotation Rotation)
	  : Origin(Origin),
		Rotation(Rotation) {}

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "FaerieGridPlacement")
	FIntPoint Origin = FIntPoint::NoneValue;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "FaerieGridPlacement")
	EFaerieSpatialItemRotation Rotation = EFaerieSpatialItemRotation::None;

	[[nodiscard]] UE_REWRITE bool UEOpEquals(const FFaerieGridPlacement& Other) const
	{
		return Origin == Other.Origin && Rotation == Other.Rotation;
	}

	[[nodiscard]] UE_REWRITE bool UEOpLessThan(const FFaerieGridPlacement& Other) const
	{
		return Origin.X < Other.Origin.X || (Origin.X == Other.Origin.X && Origin.Y < Other.Origin.Y);
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
	void PostReplicatedAdd(const FFaerieGridContent& InArraySerializer);
	void PostReplicatedChange(const FFaerieGridContent& InArraySerializer);
};

class UInventoryGridExtensionBase;

USTRUCT(BlueprintType)
struct FFaerieGridContent : public FFaerieFastArraySerializer
#if CPP
							, public TBinarySearchOptimizedArray<FFaerieGridContent, FFaerieGridKeyedStack>
#endif
{
	GENERATED_BODY()

	friend TBinarySearchOptimizedArray;
	friend UInventoryGridExtensionBase;

private:
	UPROPERTY(VisibleAnywhere, Category = "FaerieGridContent")
	TArray<FFaerieGridKeyedStack> Items;

	UE_REWRITE TArray<FFaerieGridKeyedStack>& GetArray() { return Items; }

	/** Owning extension to send Fast Array callbacks to */
	// UPROPERTY() Fast Arrays cannot have additional properties with Iris
	// ReSharper disable once CppUE4ProbableMemoryIssuesWithUObject
	TObjectPtr<UInventoryGridExtensionBase> ChangeListener;

#if FAERIE_DEBUG
	// Is writing to Items locked? Enabled while StackHandles are active.
	mutable uint32 WriteLock = 0;
#endif

public:
	template <typename Predicate>
	const FFaerieGridKeyedStack* FindByPredicate(Predicate Pred) const
	{
		return Items.FindByPredicate(Pred);
	}

	struct FScopedStackHandle
	{
		UE_NONCOPYABLE(FScopedStackHandle)

		FScopedStackHandle(const FFaerieAddress Key, FFaerieGridContent& Source);
		~FScopedStackHandle();

		FFaerieGridPlacement* operator->() const { return &Handle.Value; }
		FFaerieGridPlacement& operator*() const { return Handle.Value; }
		FFaerieGridPlacement& Get() const { return Handle.Value; }

	protected:
		FFaerieGridKeyedStack& Handle;

	private:
		FFaerieGridContent& Source;
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