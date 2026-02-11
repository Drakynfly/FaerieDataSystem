// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Containers/BitArray.h"

#include "FaerieItem.h"
#include "FaerieItemDataConcepts.h"
#include "FaerieItemToken.h"
#include "FaerieItemTokenFilterFlags.h"
#include "LoopUtils.h"
#include "PredicateTuple.h"
#include "TypeCastingUtils.h"

class UFaerieItemDataLibrary;

namespace Faerie::Token
{
	namespace Private
	{
		class FAERIEITEMDATA_API FIteratorAccess
		{
		protected:
			static const TArray<TObjectPtr<UFaerieItemToken>>& ReadTokenArray(TNotNull<const UFaerieItem*> Item);

			static bool StaticClassFilter(TNotNull<const UFaerieItemToken*> Token, TNotNull<UClass*> Class);
			static bool StaticIsMutableFilter(TNotNull<const UFaerieItemToken*> Token);

			static void AddWriteLock(TNotNull<const UFaerieItem*> Item);
			static void RemoveWriteLock(TNotNull<const UFaerieItem*> Item);

			static void HashCombineToken(TNotNull<const UFaerieItemToken*> Token, uint32& Hash);

			static bool CompareTokenMasks(const TBitArray<>& MaskA, TNotNull<const UFaerieItem*> ItemA, const TBitArray<>& MaskB, TNotNull<const UFaerieItem*> ItemB);
		};
	}

	class FTokenIterator : Private::FIteratorAccess
	{
	public:
		FTokenIterator(const TNotNull<const UFaerieItem*> Item)
		  : Iterator(ReadTokenArray(Item).CreateConstIterator()) {}

		[[nodiscard]] UE_REWRITE auto operator*() const
		{
			return Iterator.operator*();
		}

		UE_REWRITE explicit operator bool() const { return static_cast<bool>(Iterator); }

		UE_REWRITE void operator++()
		{
			++Iterator;
		}

		[[nodiscard]] UE_REWRITE bool operator!=(EIteratorType) const
		{
			// As long as we are valid, then we have not ended.
			return static_cast<bool>(*this);
		}

		[[nodiscard]] UE_REWRITE const FTokenIterator& begin() const { return *this; }
		[[nodiscard]] UE_REWRITE EIteratorType end() const { return End; }

	private:
		TArray<TObjectPtr<UFaerieItemToken>>::TConstIterator Iterator;
	};

	template <typename TPredicate>
	concept CTokenPredicate = requires(const TPredicate& Predicate, TNotNull<const UFaerieItemToken*> Token)
	{
		{ Predicate.Exec(Token) } -> UE::CSameAs<bool>;
	};

	template <bool View, typename TTokenClass, EFilterFlags Flags, CTokenPredicate... TPredicates>
	class TFilteringIterator : Private::FIteratorAccess
	{
		using InputType = std::conditional_t<View, const Utils::TPredicateTuple<TPredicates...>&, Utils::TPredicateTuple<TPredicates...>&&>;
		using FieldType = std::conditional_t<View, const Utils::TPredicateTuple<TPredicates...>&, Utils::TPredicateTuple<TPredicates...>>;

	public:
		TFilteringIterator(InputType&& PredicateTuple, const TNotNull<const UFaerieItem*> Item)
		  : PredicateTuple(MoveTempIfPossible(PredicateTuple)), Iterator(Item)
		{
			SkipInvalid();
		}

		[[nodiscard]] UE_REWRITE TTokenClass* operator*() const { return CastChecked<TTokenClass>(Iterator.operator*(), ECastCheckedType::NullAllowed); }

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
				if constexpr (!std::is_same_v<TTokenClass, UFaerieItemToken>)
				{
					if (!StaticClassFilter(*Iterator, TTokenClass::StaticClass()))
					{
						return false;
					}
				}

				if constexpr (EnumHasAnyFlags(Flags, EFilterFlags::MutableOnly))
				{
					if (!StaticIsMutableFilter(*Iterator))
					{
						return false;
					}
				}

				if constexpr (EnumHasAnyFlags(Flags, EFilterFlags::ImmutableOnly))
				{
					if (StaticIsMutableFilter(*Iterator))
					{
						return false;
					}
				}

				if constexpr (EnumHasAnyFlags(Flags, EFilterFlags::Inverted))
				{
					// Test for not passing the predicates
					return !PredicateTuple.TestAll(*Iterator);
				}
				else
				{
					// Test for passing the predicates
					return PredicateTuple.TestAll(*Iterator);
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
		FTokenIterator Iterator;
	};

	template <CItemTokenBase FilterClass, EFilterFlags Flags, CTokenPredicate... TPredicates>
	class TFilter : Private::FIteratorAccess
	{
		// Let this library use BlueprintOnlyAccess;
		friend UFaerieItemDataLibrary;

	public:
		static constexpr bool Const = !EnumHasAnyFlags(Flags, EFilterFlags::MutableOnly);
		using FElementType = std::conditional_t<Const, const FilterClass, FilterClass>;

		TFilter() = default;

		TFilter(const Utils::TPredicateTuple<TPredicates...>&& PredicateTuple)
		 : PredicateTuple(PredicateTuple) {}

		TFilter(Utils::TPredicateTuple<TPredicates...>&& PredicateTuple)
		 : PredicateTuple(MoveTemp(PredicateTuple)) {}

		[[nodiscard]] UE_REWRITE auto Invert() &&
		{
			if constexpr (EnumHasAllFlags(Flags, EFilterFlags::Inverted))
			{
				return TFilter<FilterClass, Flags & ~EFilterFlags::Inverted, TPredicates...>(MoveTemp(PredicateTuple));
			}
			else
			{
				return TFilter<FilterClass, Flags | EFilterFlags::Inverted, TPredicates...>(MoveTemp(PredicateTuple));
			}
		}

		[[nodiscard]] auto ByImmutable() const &
		{
			return TFilter<FilterClass, FlagImmutableOnly<Flags>(), TPredicates...>(PredicateTuple);
		}

		[[nodiscard]] auto ByImmutable() &&
		{
			return TFilter<FilterClass, FlagImmutableOnly<Flags>(), TPredicates...>(MoveTemp(PredicateTuple));
		}

		[[nodiscard]] auto ByMutable() const &
		{
			return TFilter<FilterClass, FlagMutableOnly<Flags>(), TPredicates...>(PredicateTuple);
		}

		[[nodiscard]] auto ByMutable() &&
		{
			return TFilter<FilterClass, FlagMutableOnly<Flags>(), TPredicates...>(MoveTemp(PredicateTuple));
		}

		template<
			CItemTokenImpl T
			UE_REQUIRES(TIsDerivedFrom<T, FilterClass>::Value && !std::is_same_v<FilterClass, T>)
		>
		[[nodiscard]] auto ByClass() const &
		{
			return TFilter<T, Flags, TPredicates...>(PredicateTuple);
		}

		template<
			CItemTokenImpl T
			UE_REQUIRES(TIsDerivedFrom<T, FilterClass>::Value && !std::is_same_v<FilterClass, T>)
		>
		[[nodiscard]] auto ByClass() &&
		{
			return TFilter<T, Flags, TPredicates...>(MoveTemp(PredicateTuple));
		}

		template <CTokenPredicate T>
		[[nodiscard]] auto By(T&& Predicate) const &
		{
			return TFilter<FilterClass, Flags, TPredicates..., T>(PredicateTuple.template AddPredicateAndCopy<T>(MoveTemp(Predicate)));
		}

		template <CTokenPredicate T, typename... TArgs>
		[[nodiscard]] auto By(TArgs&&... Args) const &
		{
			return TFilter<FilterClass, Flags, TPredicates..., T>(PredicateTuple.template AddPredicateAndCopy<T>(T(Args...)));
		}

		template <CTokenPredicate T>
		[[nodiscard]] auto By(T&& Predicate) &&
		{
			return TFilter<FilterClass, Flags, TPredicates..., T>(PredicateTuple.template AddPredicateAndMove<T>(MoveTemp(Predicate)));
		}

		template <CTokenPredicate T, typename... TArgs>
		[[nodiscard]] auto By(TArgs&&... Args) &&
		{
			return TFilter<FilterClass, Flags, TPredicates..., T>(PredicateTuple.template AddPredicateAndMove<T>(T(Args...)));
		}

		[[nodiscard]] auto Iterate(const TNotNull<const UFaerieItem*> Item) const &
		{
			return TFilteringIterator<true, FilterClass, Flags, TPredicates...>(PredicateTuple, Item);
		}

		[[nodiscard]] auto Iterate(const TNotNull<const UFaerieItem*> Item) &&
		{
			return TFilteringIterator<false, FilterClass, Flags, TPredicates...>(MoveTemp(PredicateTuple), Item);
		}

		[[nodiscard]] auto First(const TNotNull<const UFaerieItem*> Item) const
		{
			for (auto&& Token : Iterate(Item))
			{
				return Token;
			}
			return nullptr;
		}

		[[nodiscard]] TArray<FElementType*> Emit(const TNotNull<const UFaerieItem*> Item) const
		{
			TArray<FElementType*> Out;
			for (auto&& Token : Iterate(Item))
			{
				Out.Add(Token);
			}
			return Out;
		}

		[[nodiscard]] int32 Count(const TNotNull<const UFaerieItem*> Item) const
		{
			int32 OutCount = 0;
			for (auto&& Token : Iterate(Item))
			{
				OutCount++;
			}
			return OutCount;
		}

		[[nodiscard]] uint32 Hash(const TNotNull<const UFaerieItem*> Item) const
		{
			uint32 Hash = 0;
			for (auto&& Token : Iterate(Item))
			{
				HashCombineToken(Token, Hash);
			}
			return Hash;
		}

		// Create a bitmask representing which tokens pass the predicates.
		[[nodiscard]] TBitArray<> Mask(const TNotNull<const UFaerieItem*> Item) const
		{
			TBitArray<> Mask;
			Mask.Init(false, Item->GetOwnedTokens().Num());
			for (int32 i = 0; i < Item->GetOwnedTokens().Num(); ++i)
			{
				if (PredicateTuple.TestAll(Item->GetTokenAtIndex(i)))
				{
					Mask[i] = true;
				}
			}
			return Mask;
		}

		template <typename TOther>
		[[nodiscard]] bool CompareTokens(const TOther& OtherFilter, const TNotNull<const UFaerieItem*> ItemA, TNotNull<const UFaerieItem*> ItemB) const
		{
			return CompareTokenMasks(Mask(ItemA), ItemA, OtherFilter.Mask(ItemB), ItemB);
		}

	private:
		[[nodiscard]] TArray<UFaerieItemToken*> BlueprintOnlyAccess(const TNotNull<const UFaerieItem*> Item) const
		{
			return Type::Cast<TArray<UFaerieItemToken*>>(Emit(Item));
		}

		Utils::TPredicateTuple<TPredicates...> PredicateTuple;
	};

	// Forward declare the default parameters of the template
	template <CItemTokenBase FilterClass = UFaerieItemToken, EFilterFlags Flags = EFilterFlags::None, CTokenPredicate... TPredicates>
	class TFilter;

	// Create a new token filter
	[[nodiscard]] UE_REWRITE TFilter<> Filter() { return TFilter<>(); }
}