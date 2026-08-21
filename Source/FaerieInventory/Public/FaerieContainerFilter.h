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

	template <EFilterFlags Flags>
	consteval EIteratorMutabilityToggle DetermineMutability()
	{
		return EnumHasAnyFlags(Flags, EFilterFlags::MutableOnly) ? OnlyMutableInstances : AllInstances;
	}

	template <typename TPredicate>
	concept CFilterPredicate = requires(const TPredicate& Predicate, const FMassEntityManager* EntityManager, TValid<const FFaerieItemProxy&> Proxy)
	{
		{ Predicate.Exec(EntityManager, Proxy) } -> UE::CSameAs<bool>;
	};

	template <bool View, typename ResolveType, typename Interface, EFilterFlags Flags, CFilterPredicate... TPredicates>
	class TFilteringIterator
	{
		using InputType = std::conditional_t<View, const Utils::TPredicateTuple<TPredicates...>&, Utils::TPredicateTuple<TPredicates...>&&>;
		using FieldType = std::conditional_t<View, const Utils::TPredicateTuple<TPredicates...>&, Utils::TPredicateTuple<TPredicates...>>;

	public:
		explicit TFilteringIterator(InputType PredicateTuple, const FMassEntityManager* EntityManager, const TNotNull<const UFaerieItemContainerBase*> Container)
		  : EntityManager(EntityManager),
			PredicateTuple(MoveTempIfPossible(PredicateTuple)),
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
					return !PredicateTuple.TestAll(EntityManager, Iterator.GetSingleFrameProxy());
				}
				else
				{
					// Test for passing the predicates
					return PredicateTuple.TestAll(EntityManager, Iterator.GetSingleFrameProxy());
				}
			};

			// Advance while we are valid and failing the iterator tests
			while (static_cast<bool>(*this) && !TestIterator())
			{
				++Iterator;
			}
		}

		[[nodiscard]] UE_REWRITE bool operator!=(Utils::EIteratorType) const
		{
			// As long as we are valid, then we have not ended.
			return static_cast<bool>(*this);
		}

		[[nodiscard]] UE_REWRITE const TFilteringIterator& begin() const { return *this; }
		[[nodiscard]] UE_REWRITE Utils::EIteratorType end() const { return Utils::End; }

	private:
		const FMassEntityManager* EntityManager;
		FieldType PredicateTuple;

		// If MutableOnly has been enabled by a predicate, use the automatic skip feature in TIterator
		TIterator<ResolveType, DetermineMutability<Flags>(), Interface> Iterator;
	};

	template <EFilterFlags Flags, typename ResolveType, typename Interface, CFilterPredicate... TPredicates>
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
			return TFilter<FlagImmutableOnly<Flags>(), ResolveType, Interface, TPredicates...>(PredicateTuple);
		}

		[[nodiscard]] auto ByImmutable() &&
		{
			return TFilter<FlagImmutableOnly<Flags>(), ResolveType, Interface, TPredicates...>(MoveTemp(PredicateTuple));
		}

		[[nodiscard]] auto ByMutable() const &
		{
			return TFilter<FlagMutableOnly<Flags>(), ResolveType, Interface, TPredicates...>(PredicateTuple);
		}

		[[nodiscard]] auto ByMutable() &&
		{
			return TFilter<FlagMutableOnly<Flags>(), ResolveType, Interface, TPredicates...>(MoveTemp(PredicateTuple));
		}

		template <CFilterPredicate TPredicate>
		[[nodiscard]] auto By(TPredicate&& NewFilter) const &
		{
			return TFilter<Flags, ResolveType, Interface, TPredicates..., TPredicate>(PredicateTuple.template AddPredicateAndCopy<TPredicate>(MoveTemp(NewFilter)));
		}

		template <CFilterPredicate TPredicate, typename... TArgs>
		[[nodiscard]] auto By(TArgs&&... Args) const &
		{
			return TFilter<Flags, ResolveType, Interface, TPredicates..., TPredicate>(PredicateTuple.template AddPredicateAndCopy<TPredicate>(TPredicate(Args...)));
		}

		template <CFilterPredicate TPredicate>
		[[nodiscard]] auto By(TPredicate&& NewFilter) &&
		{
			return TFilter<Flags, ResolveType, Interface, TPredicates..., TPredicate>(PredicateTuple.template AddPredicateAndMove<TPredicate>(MoveTemp(NewFilter)));
		}

		template <CFilterPredicate TPredicate, typename... TArgs>
		[[nodiscard]] auto By(TArgs&&... Args) &&
		{
			return TFilter<Flags, ResolveType, Interface, TPredicates..., TPredicate>(PredicateTuple.template AddPredicateAndMove<TPredicate>(TPredicate(Args...)));
		}

		// Invert the filter direction so that excluded elements become included. Calling this function again will reset it.
		[[nodiscard]] auto Invert() const &
		{
			if constexpr (EnumHasAllFlags(Flags, EFilterFlags::Inverted))
			{
				return TFilter<Flags & ~EFilterFlags::Inverted, ResolveType, Interface, TPredicates...>(PredicateTuple);
			}
			else
			{
				return TFilter<Flags | EFilterFlags::Inverted, ResolveType, Interface, TPredicates...>(PredicateTuple);
			}
		}

		// Invert the filter direction so that excluded elements become included. Calling this function again will reset it.
		[[nodiscard]] auto Invert() &&
		{
			if constexpr (EnumHasAllFlags(Flags, EFilterFlags::Inverted))
			{
				return TFilter<Flags & ~EFilterFlags::Inverted, ResolveType, Interface, TPredicates...>(MoveTemp(PredicateTuple));
			}
			else
			{
				return TFilter<Flags | EFilterFlags::Inverted, ResolveType, Interface, TPredicates...>(MoveTemp(PredicateTuple));
			}
		}

		// Create an iterator from this filter.
		[[nodiscard]] UE_REWRITE auto Iterate(const FMassEntityManager* EntityManager, TNotNull<const UFaerieItemContainerBase*> Container) const &
		{
			return TFilteringIterator<true, ResolveType, Interface, Flags, TPredicates...>(PredicateTuple, EntityManager, Container);
		}

		// Create an iterator from this filter.
		[[nodiscard]] UE_REWRITE auto Iterate(const FMassEntityManager* EntityManager, TNotNull<const UFaerieItemContainerBase*> Container) &&
		{
			return TFilteringIterator<false, ResolveType, Interface, Flags, TPredicates...>(MoveTemp(PredicateTuple), EntityManager, Container);
		}

		[[nodiscard]] int32 Count(const FMassEntityManager* EntityManager, const TNotNull<const UFaerieItemContainerBase*> Container) const
		{
			int32 OutCount = 0;
			for (auto It = Iterate(EntityManager, Container); It; ++It)
			{
				OutCount++;
			}
			return OutCount;
		}

		template <typename OutContainerType>
		void Emit(const FMassEntityManager* EntityManager, const TNotNull<const UFaerieItemContainerBase*> Container, OutContainerType& OutContainer) const
		{
			for (auto It = Iterate(EntityManager, Container); It; ++It)
			{
				OutContainer.Add(*It);
			}
		}

		[[nodiscard]] ResolveType First(const FMassEntityManager* EntityManager, const TNotNull<const UFaerieItemContainerBase*> Container) const
		{
			if (auto It = Iterate(EntityManager, Container); It)
			{
				return *It;
			}
			return ResolveType();
		}

	private:
		Utils::TPredicateTuple<TPredicates...> PredicateTuple;
	};

	using FKeyFilter = TFilter<EFilterFlags::None, FFaerieEntryKey, IEntryIterator>;
	using FAddressFilter = TFilter<EFilterFlags::None, FFaerieAddress, IAddressIterator>;
	using FItemFilter = TFilter<EFilterFlags::None, const UFaerieItem*, IEntryIterator>;
	using FMutableItemFilter = TFilter<EFilterFlags::MutableOnly, UFaerieItem*, IEntryIterator>;
}
