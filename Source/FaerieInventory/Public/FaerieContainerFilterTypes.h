// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieContainerIterator.h"
#include "GameplayTagContainer.h"

namespace Faerie::Container
{
	enum class EFilterFlags : uint32
	{
		None = 0,

		// @todo not yet supported
		ImmutableOnly = 1 << 0,

		// This filter is restricted to emitting mutable items
		MutableOnly = 1 << 1,

		// @todo not yet supported
		Static = 1 << 2,

		Inverted = 1 << 3
	};
	ENUM_CLASS_FLAGS(EFilterFlags)

	template <typename T>
	struct TPredicateTraits
	{
		static constexpr EFilterFlags GrantFlags = EFilterFlags::None;
		static constexpr EFilterFlags RemoveFlags = EFilterFlags::None;
	};

	template <typename T, EFilterFlags Flags>
	consteval EFilterFlags CombineFilterFlags()
	{
		return (Flags & ~TPredicateTraits<T>::RemoveFlags) | TPredicateTraits<T>::GrantFlags;
	}

	template <typename TPredicate>
	concept CFilterPredicate = requires(const TPredicate& Predicate, FIteratorPtr Iterator)
	{
		{ Predicate.Exec(Iterator) } -> UE::CSameAs<bool>;
	};

	struct FAERIEINVENTORY_API FMutablePredicate
	{
		static bool Exec(FIteratorPtr Iterator);
	};

	template <>
	struct TPredicateTraits<FMutablePredicate>
	{
		static constexpr EFilterFlags TypeFlags = EFilterFlags::Static;
		static constexpr EFilterFlags GrantFlags = EFilterFlags::MutableOnly;
		static constexpr EFilterFlags RemoveFlags = EFilterFlags::ImmutableOnly;
	};

	struct FAERIEINVENTORY_API FImmutablePredicate
	{
		static bool Exec(FIteratorPtr Iterator);
	};

	template <>
	struct TPredicateTraits<FImmutablePredicate>
	{
		static constexpr EFilterFlags TypeFlags = EFilterFlags::Static;
		static constexpr EFilterFlags GrantFlags = EFilterFlags::ImmutableOnly;
		static constexpr EFilterFlags RemoveFlags = EFilterFlags::MutableOnly;
	};

	// Compare the test Item's name against an FText.
	struct FAERIEINVENTORY_API FCompareName
	{
		bool Exec(FIteratorPtr Iterator) const;
		FText CompareText;
		ETextComparisonLevel::Type ComparisonType;
	};

	// Test for containing a gameplay tag.
	struct FAERIEINVENTORY_API FHasTag
	{
		bool Exec(FIteratorPtr Iterator) const;
		FGameplayTag Tag;
		bool HasTagExact;
	};

	// Run a callback on the iterator, allowing user code to run arbitrary selection logic.
	struct FAERIEINVENTORY_API FCallbackFilter
	{
		bool Exec(FIteratorPtr Iterator) const;
		FIteratorPredicate Callback;
	};
}