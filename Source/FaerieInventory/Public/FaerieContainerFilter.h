// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieContainerFilterTypes.h"
#include "FaerieContainerIterator.h"
#include "LoopUtils.h"
#include "PredicateTuple.h"

class UFaerieItem;
class UFaerieItemContainerBase;

namespace Faerie::Container
{
	template <typename TEmitType, bool SkipMode, typename... TPredicates>
	class TFilteringIterator
	{
	public:
		TFilteringIterator(Utils::TPredicateTuple<TPredicates...>&& PredicateTuple, const TNotNull<const UFaerieItemContainerBase*> Container)
		  : PredicateTuple(MoveTemp(PredicateTuple)),
		 	Iterator(Container) {}

		[[nodiscard]] UE_REWRITE TEmitType operator*() const { return Iterator.operator*(); }

		UE_REWRITE explicit operator bool() const { return static_cast<bool>(Iterator); }

		UE_REWRITE void operator++()
		{
			do
			{
				++Iterator;
			}
			while (static_cast<bool>(Iterator) && !PredicateTuple.TestAll(Iterator.GetPtr()));
		}

		[[nodiscard]] UE_REWRITE bool operator!=(EIteratorType) const
		{
			// As long as we are valid, then we have not ended.
			return static_cast<bool>(*this);
		}

		[[nodiscard]] UE_REWRITE const TFilteringIterator& begin() { return *this; }
		[[nodiscard]] UE_REWRITE EIteratorType end () const { return End; }

	private:
		Utils::TPredicateTuple<TPredicates...> PredicateTuple;
		TIterator<TEmitType, SkipMode> Iterator;
	};

	template <typename TEmitType, bool SkipMode, typename... TPredicates>
	class TFilteringIterator_Ref
	{
	public:
		TFilteringIterator_Ref(const Utils::TPredicateTuple<TPredicates...>& PredicateTuple, const TNotNull<const UFaerieItemContainerBase*> Container)
		  : PredicateTuple(PredicateTuple),
		 	Iterator(Container) {}

		[[nodiscard]] UE_REWRITE TEmitType operator*() const { return Iterator.operator*(); }

		UE_REWRITE explicit operator bool() const { return static_cast<bool>(Iterator); }

		UE_REWRITE void operator++()
		{
			do
			{
				++Iterator;
			}
			while (static_cast<bool>(Iterator) && !PredicateTuple.TestAll(Iterator.GetPtr()));
		}

		[[nodiscard]] UE_REWRITE bool operator!=(EIteratorType) const
		{
			// As long as we are valid, then we have not ended.
			return static_cast<bool>(*this);
		}

		[[nodiscard]] UE_REWRITE const TFilteringIterator_Ref& begin() { return *this; }
		[[nodiscard]] UE_REWRITE EIteratorType end () const { return End; }

	private:
		const Utils::TPredicateTuple<TPredicates...>& PredicateTuple;
		TIterator<TEmitType, SkipMode> Iterator;
	};

	template <EFilterFlags Flags, typename TEmitType, typename... TPredicates>
	class TFilter
	{
	public:
		TFilter() = default;

		TFilter(const Utils::TPredicateTuple<TPredicates...>& PredicateTuple)
		  : PredicateTuple(PredicateTuple) {}

		TFilter(Utils::TPredicateTuple<TPredicates...>&& PredicateTuple)
		  : PredicateTuple(MoveTemp(PredicateTuple)) {}

		template <CFilterPredicate TPredicate>
		[[nodiscard]] auto By(TPredicate&& NewFilter) const &
		{
			return TFilter<CombineFilterFlags<TPredicate, Flags>(), TEmitType, TPredicates..., TPredicate>(PredicateTuple.template AddPredicateAndCopy<TPredicate>(MoveTemp(NewFilter)));
		}

		template <CFilterPredicate TPredicate, typename... TArgs>
		[[nodiscard]] auto By(TArgs&&... Args) const &
		{
			return TFilter<CombineFilterFlags<TPredicate, Flags>(), TEmitType, TPredicates..., TPredicate>(PredicateTuple.template AddPredicateAndCopy<TPredicate>(TPredicate(Args...)));
		}

		template <CFilterPredicate TPredicate>
		[[nodiscard]] auto By(TPredicate&& NewFilter) &&
		{
			return TFilter<CombineFilterFlags<TPredicate, Flags>(), TEmitType, TPredicates..., TPredicate>(PredicateTuple.template AddPredicateAndMove<TPredicate>(MoveTemp(NewFilter)));
		}

		template <CFilterPredicate TPredicate, typename... TArgs>
		[[nodiscard]] auto By(TArgs&&... Args) &&
		{
			return TFilter<CombineFilterFlags<TPredicate, Flags>(), TEmitType, TPredicates..., TPredicate>(PredicateTuple.template AddPredicateAndMove<TPredicate>(TPredicate(Args...)));
		}

		// Invert the filter direction so that excluded elements become included. Calling this function again will reset it.
		[[nodiscard]] auto Invert() const &
		{
			if constexpr (EnumHasAllFlags(Flags, EFilterFlags::Inverted))
			{
				return TFilter<Flags & ~EFilterFlags::Inverted, TEmitType, TPredicates...>(PredicateTuple);
			}
			else
			{
				return TFilter<Flags | EFilterFlags::Inverted, TEmitType, TPredicates...>(PredicateTuple);
			}
		}

		// Invert the filter direction so that excluded elements become included. Calling this function again will reset it.
		[[nodiscard]] auto Invert() &&
		{
			if constexpr (EnumHasAllFlags(Flags, EFilterFlags::Inverted))
			{
				return TFilter<Flags & ~EFilterFlags::Inverted, TEmitType, TPredicates...>(MoveTemp(PredicateTuple));
			}
			else
			{
				return TFilter<Flags | EFilterFlags::Inverted, TEmitType, TPredicates...>(MoveTemp(PredicateTuple));
			}
		}

		// Create an iterator from this filter.
		[[nodiscard]] UE_REWRITE auto Iterate(TNotNull<const UFaerieItemContainerBase*> Container) const
		{
			// If MutableOnly has been enabled by a predicate, use the automatic skip feature in TIterator
			return TFilteringIterator_Ref<TEmitType,
				EnumHasAnyFlags(Flags, EFilterFlags::MutableOnly),
				TPredicates...>(PredicateTuple, Container);
		}

		[[nodiscard]] int32 Count(TNotNull<const UFaerieItemContainerBase*> Container) const
		{
			int32 OutCount = 0;
			for (TIterator<TEmitType, EnumHasAnyFlags(Flags, EFilterFlags::MutableOnly)> It(Container); It; ++It)
			{
				if constexpr (EnumHasAnyFlags(Flags, EFilterFlags::Inverted))
				{
					if (!PredicateTuple.TestAll(It.GetPtr()))
					{
						OutCount++;
					}
				}
				else
				{
					if (PredicateTuple.TestAll(It.GetPtr()))
					{
						OutCount++;
					}
				}
			}

			return OutCount;
		}

		[[nodiscard]] TArray<TEmitType> Emit(TNotNull<const UFaerieItemContainerBase*> Container) const
		{
			TArray<TEmitType> OutItems;
			for (TIterator<TEmitType, EnumHasAnyFlags(Flags, EFilterFlags::MutableOnly)> It(Container); It; ++It)
			{
				if constexpr (EnumHasAnyFlags(Flags, EFilterFlags::Inverted))
				{
					if (!PredicateTuple.TestAll(It.GetPtr()))
					{
						OutItems.Add(*It);
					}
				}
				else
				{
					if (PredicateTuple.TestAll(It.GetPtr()))
					{
						OutItems.Add(*It);
					}
				}
			}

			return OutItems;
		}

		[[nodiscard]] TEmitType First(TNotNull<const UFaerieItemContainerBase*> Container) const
		{
			for (TIterator<TEmitType, EnumHasAnyFlags(Flags, EFilterFlags::MutableOnly)> It(Container); It; ++It)
			{
				if constexpr (EnumHasAnyFlags(Flags, EFilterFlags::Inverted))
				{
					if (!PredicateTuple.TestAll(It.GetPtr()))
					{
						return *It;
					}
				}
				else
				{
					if (PredicateTuple.TestAll(It.GetPtr()))
					{
						return *It;
					}
				}
			}
			return TEmitType();
		}

	private:
		Utils::TPredicateTuple<TPredicates...> PredicateTuple;
	};

	using FKeyFilter = TFilter<EFilterFlags::None, FEntryKey>;
	using FAddressFilter = TFilter<EFilterFlags::None, FFaerieAddress>;
	using FItemFilter = TFilter<EFilterFlags::None, const UFaerieItem*>;
	using FMutableItemFilter = TFilter<EFilterFlags::MutableOnly, UFaerieItem*>;
}
