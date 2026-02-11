// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieContainerIterator.h"
#include "LoopUtils.h"
#include "PredicateTuple.h"

class UFaerieItem;
class UFaerieItemContainerBase;

namespace Faerie::Container
{
	enum class EFilterFlags : uint32
	{
		None = 0,

		// @todo not yet supported
		ImmutableOnly = 1 << 0,

		// This filter is restricted to emitting mutable items
		MutableOnly = 1 << 1,

		Inverted = 1 << 2
	};
	ENUM_CLASS_FLAGS(EFilterFlags)

	template <EFilterFlags Flags>
	consteval EFilterFlags FlagMutableOnly()
	{
		return Flags & ~EFilterFlags::ImmutableOnly | EFilterFlags::MutableOnly;
	}

	template <EFilterFlags Flags>
	consteval EFilterFlags FlagImmutableOnly()
	{
		return Flags & ~EFilterFlags::MutableOnly | EFilterFlags::ImmutableOnly;
	}

	template <typename TPredicate>
	concept CFilterPredicate = requires(const TPredicate& Predicate, FIteratorPtr Iterator)
	{
		{ Predicate.Exec(Iterator) } -> UE::CSameAs<bool>;
	};

	template <bool View, typename ResolveType, EFilterFlags Flags, CFilterPredicate... TPredicates>
	class TFilteringIterator
	{
		using InputType = std::conditional_t<View, const Utils::TPredicateTuple<TPredicates...>&, Utils::TPredicateTuple<TPredicates...>&&>;
		using FieldType = std::conditional_t<View, const Utils::TPredicateTuple<TPredicates...>&, Utils::TPredicateTuple<TPredicates...>>;

	public:
		explicit TFilteringIterator(InputType PredicateTuple, const TNotNull<const UFaerieItemContainerBase*> Container)
		  : PredicateTuple(MoveTempIfPossible(PredicateTuple)),
			Iterator(Container)
		{
			SkipInvalid();
		}

		[[nodiscard]] UE_REWRITE ResolveType operator*() const { return Iterator.operator*(); }

		UE_REWRITE explicit operator bool() const { return static_cast<bool>(Iterator); }

		UE_REWRITE void operator++()
		{
			++Iterator;
			SkipInvalid();
		}

		// Advance to the next that we allow
		void SkipInvalid()
		{
			auto TestIterator = [&]() -> bool
			{
				if constexpr (EnumHasAnyFlags(Flags, EFilterFlags::Inverted))
				{
					// Test for not passing the predicates
					return !PredicateTuple.TestAll(Iterator.GetPtr());
				}
				else
				{
					// Test for passing the predicates
					return PredicateTuple.TestAll(Iterator.GetPtr());
				}
			};

			// Advance while we are valid and failing the iterator tests
			while (static_cast<bool>(*this) && !TestIterator())
			{
				++Iterator;
			}
		}

		[[nodiscard]] UE_REWRITE bool operator!=(EIteratorType) const
		{
			// As long as we are valid, then we have not ended.
			return static_cast<bool>(*this);
		}

		[[nodiscard]] UE_REWRITE const TFilteringIterator& begin() const { return *this; }
		[[nodiscard]] UE_REWRITE EIteratorType end() const { return End; }

	private:
		FieldType PredicateTuple;

		// If MutableOnly has been enabled by a predicate, use the automatic skip feature in TIterator
		TIterator<ResolveType, EnumHasAnyFlags(Flags, EFilterFlags::MutableOnly)> Iterator;
	};

	template <EFilterFlags Flags, typename ResolveType, CFilterPredicate... TPredicates>
	class TFilter
	{
	public:
		TFilter() = default;

		TFilter(const Utils::TPredicateTuple<TPredicates...>& PredicateTuple)
		  : PredicateTuple(PredicateTuple) {}

		TFilter(Utils::TPredicateTuple<TPredicates...>&& PredicateTuple)
		  : PredicateTuple(MoveTemp(PredicateTuple)) {}

		[[nodiscard]] auto ByImmutable() const &
		{
			return TFilter<FlagImmutableOnly<Flags>(), ResolveType, TPredicates...>(PredicateTuple);
		}

		[[nodiscard]] auto ByImmutable() &&
		{
			return TFilter<FlagImmutableOnly<Flags>(), ResolveType, TPredicates...>(MoveTemp(PredicateTuple));
		}

		[[nodiscard]] auto ByMutable() const &
		{
			return TFilter<FlagMutableOnly<Flags>(), ResolveType, TPredicates...>(PredicateTuple);
		}

		[[nodiscard]] auto ByMutable() &&
		{
			return TFilter<FlagMutableOnly<Flags>(), ResolveType, TPredicates...>(MoveTemp(PredicateTuple));
		}

		template <CFilterPredicate TPredicate>
		[[nodiscard]] auto By(TPredicate&& NewFilter) const &
		{
			return TFilter<Flags, ResolveType, TPredicates..., TPredicate>(PredicateTuple.template AddPredicateAndCopy<TPredicate>(MoveTemp(NewFilter)));
		}

		template <CFilterPredicate TPredicate, typename... TArgs>
		[[nodiscard]] auto By(TArgs&&... Args) const &
		{
			return TFilter<Flags, ResolveType, TPredicates..., TPredicate>(PredicateTuple.template AddPredicateAndCopy<TPredicate>(TPredicate(Args...)));
		}

		template <CFilterPredicate TPredicate>
		[[nodiscard]] auto By(TPredicate&& NewFilter) &&
		{
			return TFilter<Flags, ResolveType, TPredicates..., TPredicate>(PredicateTuple.template AddPredicateAndMove<TPredicate>(MoveTemp(NewFilter)));
		}

		template <CFilterPredicate TPredicate, typename... TArgs>
		[[nodiscard]] auto By(TArgs&&... Args) &&
		{
			return TFilter<Flags, ResolveType, TPredicates..., TPredicate>(PredicateTuple.template AddPredicateAndMove<TPredicate>(TPredicate(Args...)));
		}

		// Invert the filter direction so that excluded elements become included. Calling this function again will reset it.
		[[nodiscard]] auto Invert() const &
		{
			if constexpr (EnumHasAllFlags(Flags, EFilterFlags::Inverted))
			{
				return TFilter<Flags & ~EFilterFlags::Inverted, ResolveType, TPredicates...>(PredicateTuple);
			}
			else
			{
				return TFilter<Flags | EFilterFlags::Inverted, ResolveType, TPredicates...>(PredicateTuple);
			}
		}

		// Invert the filter direction so that excluded elements become included. Calling this function again will reset it.
		[[nodiscard]] auto Invert() &&
		{
			if constexpr (EnumHasAllFlags(Flags, EFilterFlags::Inverted))
			{
				return TFilter<Flags & ~EFilterFlags::Inverted, ResolveType, TPredicates...>(MoveTemp(PredicateTuple));
			}
			else
			{
				return TFilter<Flags | EFilterFlags::Inverted, ResolveType, TPredicates...>(MoveTemp(PredicateTuple));
			}
		}

		// Create an iterator from this filter.
		[[nodiscard]] UE_REWRITE auto Iterate(TNotNull<const UFaerieItemContainerBase*> Container) const &
		{
			return TFilteringIterator<true, ResolveType, Flags, TPredicates...>(PredicateTuple, Container);
		}

		// Create an iterator from this filter.
		[[nodiscard]] UE_REWRITE auto Iterate(TNotNull<const UFaerieItemContainerBase*> Container) &&
		{
			return TFilteringIterator<false, ResolveType, Flags, TPredicates...>(MoveTemp(PredicateTuple), Container);
		}

		[[nodiscard]] int32 Count(const TNotNull<const UFaerieItemContainerBase*> Container) const
		{
			int32 OutCount = 0;
			for (auto&& Value : Iterate(Container))
			{
				OutCount++;
			}
			return OutCount;
		}

		[[nodiscard]] TArray<ResolveType> Emit(const TNotNull<const UFaerieItemContainerBase*> Container) const
		{
			TArray<ResolveType> OutItems;
			for (auto&& Value : Iterate(Container))
			{
				OutItems.Add(Value);
			}
			return OutItems;
		}

		[[nodiscard]] ResolveType First(const TNotNull<const UFaerieItemContainerBase*> Container) const
		{
			for (auto&& Value : Iterate(Container))
			{
				return Value;
			}
			return ResolveType();
		}

	private:
		Utils::TPredicateTuple<TPredicates...> PredicateTuple;
	};

	using FKeyFilter = TFilter<EFilterFlags::None, FEntryKey>;
	using FAddressFilter = TFilter<EFilterFlags::None, FFaerieAddress>;
	using FItemFilter = TFilter<EFilterFlags::None, const UFaerieItem*>;
	using FMutableItemFilter = TFilter<EFilterFlags::MutableOnly, UFaerieItem*>;
}
