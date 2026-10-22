// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Templates/Tuple.h"

namespace Faerie::Utils
{
	/*
	 * This class is a wrapper around a TTuple to contain Predicates which can be invoked on an object.
	 * Currently, this has the following abilities and restrictions:
	 * - The object predicate type must implement a function called Exec, which takes a single parameters of the object's type and return a bool,
	 * - Call TestAll to perform the && operation on all predicates,
	 * - Call TestAny to perform the || operation on all predicates.
	 */
	template <typename... Predicates>
	class TPredicateTuple
	{
	public:
		[[nodiscard]] TPredicateTuple() = default;

		[[nodiscard]] TPredicateTuple(TTuple<Predicates...>&& InPredicates)
		  : PredicateTuple(MoveTemp(InPredicates)) {}

		[[nodiscard]] TPredicateTuple(const TTuple<Predicates...>& InPredicates)
		  : PredicateTuple(InPredicates) {}

		// Copy our tuple into a new tuple with a new predicate appended.
		template<typename NewPredicate>
		[[nodiscard]] TPredicateTuple<Predicates..., NewPredicate> AddPredicateAndCopy(NewPredicate&& NextView) const &
		{
			return PredicateTuple.ApplyBefore([](const Predicates&... InViews, const NewPredicate& InNextView)
				{
					return TTuple<Predicates..., NewPredicate>(InViews..., InNextView);
				}, Forward<NewPredicate>(NextView));
		}

		// Move our predicates into a new tuple with a new predicate appended.
		template<typename NewPredicate>
		[[nodiscard]] TPredicateTuple<Predicates..., NewPredicate> AddPredicateAndMove(NewPredicate&& NextView) &
		{
			return PredicateTuple.ApplyBefore([](Predicates&&... InViews, NewPredicate&& InNextView)
				{
					return TTuple<Predicates..., NewPredicate>(Forward<Predicates>(InViews)..., Forward<NewPredicate>(InNextView));
				}, Forward<NewPredicate>(NextView));
		}

		// Returns true if the object passes every predicate
		template <typename ObjectType>
		[[nodiscard]] bool TestAll(ObjectType Object) const
		{
			return PredicateTuple.ApplyAfter([Object](const Predicates&... TupleValues)
				{
					auto Run = [](ObjectType InObject, const auto& InPredicate)
					{
						return InPredicate.Exec(InObject);
					};

					return (Run(Object, TupleValues) && ...);
				});
		}

		// Returns true if the object passes any predicate
		template <typename ObjectType>
		[[nodiscard]] bool TestAny(ObjectType Object) const
		{
			return PredicateTuple.ApplyAfter([Object](const Predicates&... TupleValues)
				{
					auto Run = [](ObjectType InObject, const auto& InPredicate)
					{
						return InPredicate.Exec(InObject);
					};

					return (Run(Object, TupleValues) || ...);
				});
		}

	private:
		TTuple<Predicates...> PredicateTuple{};
	};
}
