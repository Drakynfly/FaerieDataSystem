// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "LoopUtils.h"
#include "TypeCastingUtils.h"
#include "FaerieInventoryConcepts.h"
#include "FaerieItemTokenFilter.h"
#include "Templates/SubclassOf.h"
#include "Tokens/FaerieItemStorageToken.h"

class UFaerieItem;
class UFaerieItemContainerBase;

namespace Faerie
{
	// Get all container objects from inside a FaerieItem.
	FAERIEINVENTORY_API TArray<UFaerieItemContainerBase*> GetAllContainersInItem(const TNotNull<UFaerieItem*> Item);
	FAERIEINVENTORY_API TArray<UFaerieItemContainerBase*> GetAllContainersInItemRecursive(const TNotNull<UFaerieItem*> Item);

	FAERIEINVENTORY_API TArray<const UFaerieItem*> GetChildrenInItem(const TNotNull<UFaerieItem*> Item);
	FAERIEINVENTORY_API TArray<const UFaerieItem*> GetChildrenInItemRecursive(const TNotNull<UFaerieItem*> Item);

	namespace SubObject
	{
		namespace StaticPredicates
		{
			FAERIEINVENTORY_API bool ClassEquals(TNotNull<const UFaerieItemContainerBase*> Container, const TSubclassOf<UFaerieItemContainerBase>& Class);
			FAERIEINVENTORY_API bool ClassEqualsOrChildOf(TNotNull<const UFaerieItemContainerBase*> Container, const TSubclassOf<UFaerieItemContainerBase>& Class);
		}

		/**
		 * Iterates over all Item Containers in a Faerie Item that are directly owned
		 */
		class FContainerIterator
		{
		public:
			explicit FContainerIterator(const TNotNull<UFaerieItem*> Item);
			~FContainerIterator() {}

			[[nodiscard]] UE_REWRITE UFaerieItemContainerBase* operator*() const { return Iterator.operator*()->GetItemContainer(); }

			UE_REWRITE explicit operator bool() const { return static_cast<bool>(Iterator); }

			UE_REWRITE void operator++() { ++Iterator; }

			[[nodiscard]] UE_REWRITE bool operator!=(EIteratorType) const
			{
				// As long as we are valid, then we have not ended.
				return static_cast<bool>(*this);
			}

			[[nodiscard]] UE_REWRITE const FContainerIterator& begin() const { return *this; }
			[[nodiscard]] UE_REWRITE EIteratorType end() const { return End; }

		protected:
			Token::TFilteringIterator<false, UFaerieItemContainerToken, Token::EFilterFlags::MutableOnly> Iterator;
		};

		/**
		 * Iterates over all Item Containers in a Faerie Item, and any found in sub-Items
		 */
		class FRecursiveContainerIterator
		{
			using FStorageType = TArray<UFaerieItemContainerBase*>;

		public:
			explicit FRecursiveContainerIterator(TNotNull<UFaerieItem*> Item);

			[[nodiscard]] UE_REWRITE UFaerieItemContainerBase* operator*() const { return Iterator.operator*(); }

			UE_REWRITE explicit operator bool() const { return static_cast<bool>(Iterator); }

			UE_REWRITE void operator++() { ++Iterator; }

			[[nodiscard]] UE_REWRITE bool operator!=(EIteratorType) const
			{
				// As long as we are valid, then we have not ended.
				return static_cast<bool>(*this);
			}

			[[nodiscard]] UE_REWRITE const FRecursiveContainerIterator& begin() const { return *this; }
			[[nodiscard]] UE_REWRITE EIteratorType end() const { return End; }

		protected:
			FStorageType Containers;
			FStorageType::TIterator Iterator;
		};

		template <CItemContainerBase TClass>
		class TFilteredArrayIterator
		{
			using FStorageType = TArray<TClass*>;

		public:
			explicit TFilteredArrayIterator(FStorageType&& Array)
			  : Containers(MoveTemp(Array)),
				Iterator(Containers.CreateIterator()) {}

			explicit TFilteredArrayIterator(const FStorageType& Array)
			  : Containers(Array),
				Iterator(Containers.CreateIterator()) {}

		public:
			[[nodiscard]] UE_REWRITE TClass* operator*() const { return CastChecked<TClass>(Iterator.operator*()); }

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

			[[nodiscard]] UE_REWRITE const TFilteredArrayIterator& begin() const { return *this; }
			[[nodiscard]] UE_REWRITE EIteratorType end() const { return End; }

		protected:
			FStorageType Containers;
			FStorageType::TIterator Iterator;
		};

		enum class EFilterFlags : uint32
		{
			None = 0,

			Recursive = 1 << 0,
		};
		ENUM_CLASS_FLAGS(EFilterFlags)

		struct FClassFilter
		{
			bool Exec(TNotNull<const UFaerieItemContainerBase*> Container) const
			{
				return StaticPredicates::ClassEqualsOrChildOf(Container, Class);
			}
			TSubclassOf<UFaerieItemContainerBase> Class;
		};

		struct FClassFilterExact
		{
			bool Exec(TNotNull<const UFaerieItemContainerBase*> Container) const
			{
				return StaticPredicates::ClassEquals(Container, Class);
			}
			TSubclassOf<UFaerieItemContainerBase> Class;
		};

		template <CItemContainerBase TClass, EFilterFlags Flags, typename... TPredicates>
		class TFilter
		{
		public:
			TFilter() = default;

			TFilter(const Utils::TPredicateTuple<TPredicates...>& PredicateTuple)
			  : PredicateTuple(PredicateTuple) {}

			TFilter(Utils::TPredicateTuple<TPredicates...>&& PredicateTuple)
			  : PredicateTuple(MoveTemp(PredicateTuple)) {}

			// Mark this filter as searching recursively through all children.
			// @Note: The awkward template here is to prevent calling this on a filter that is already recursive.
			template <
				EFilterFlags Flag = EFilterFlags::Recursive
				UE_REQUIRES(Flag == EFilterFlags::Recursive && !EnumHasAnyFlags(Flags, EFilterFlags::Recursive))
			>
			[[nodiscard]] auto Recursive() const &
			{
				return TFilter<TClass, Flags | EFilterFlags::Recursive>(PredicateTuple);
			}

			// Mark this filter as searching recursively through all children.
			// @Note: The awkward template here is to prevent calling this on a filter that is already recursive.
			template <
				EFilterFlags Flag = EFilterFlags::Recursive
				UE_REQUIRES(Flag == EFilterFlags::Recursive && !EnumHasAnyFlags(Flags, EFilterFlags::Recursive))
			>
			[[nodiscard]] auto Recursive() &&
			{
				return TFilter<TClass, Flags | EFilterFlags::Recursive>(MoveTemp(PredicateTuple));
			}

			template <typename T UE_REQUIRES(TIsDerivedFrom<T, TClass>::Value)>
			[[nodiscard]] auto ByClass() const &
			{
				return TFilter<T, Flags, TPredicates...>(PredicateTuple);
			}

			template <typename T UE_REQUIRES(TIsDerivedFrom<T, TClass>::Value)>
			[[nodiscard]] auto ByClass() &&
			{
				return TFilter<T, Flags, TPredicates...>(MoveTemp(PredicateTuple));
			}

			// @todo restrict TPredicate to only allow filter structs
			template <typename TPredicate>
			[[nodiscard]] auto By(TPredicate&& NewFilter) const &
			{
				return TFilter<UFaerieItemContainerBase, Flags, TPredicates..., TPredicate>(PredicateTuple.template AddPredicateAndCopy<TPredicate>(MoveTemp(NewFilter)));
			}

			// @todo restrict TPredicate to only allow filter structs
			template <typename TPredicate, typename... TArgs>
			[[nodiscard]] auto By(TArgs&&... Args) const &
			{
				return TFilter<UFaerieItemContainerBase, Flags, TPredicates..., TPredicate>(PredicateTuple.template AddPredicateAndCopy<TPredicate>(TPredicate(Args...)));
			}

			// @todo restrict TPredicate to only allow filter structs
			template <typename TPredicate>
			[[nodiscard]] auto By(TPredicate&& NewFilter) &&
			{
				return TFilter<UFaerieItemContainerBase, Flags, TPredicates..., TPredicate>(PredicateTuple.template AddPredicateAndMove<TPredicate>(MoveTemp(NewFilter)));
			}

			// @todo restrict TPredicate to only allow filter structs
			template <typename TPredicate, typename... TArgs>
			[[nodiscard]] auto By(TArgs&&... Args) &&
			{
				return TFilter<UFaerieItemContainerBase, Flags, TPredicates..., TPredicate>(PredicateTuple.template AddPredicateAndMove<TPredicate>(TPredicate(Args...)));
			}

			[[nodiscard]] TArray<TClass*> Emit(const TNotNull<UFaerieItem*> Item) const
			{
				TArray<UFaerieItemContainerBase*> Containers;
				if constexpr (EnumHasAnyFlags(Flags, EFilterFlags::Recursive))
				{
					Containers = GetAllContainersInItemRecursive(Item);
				}
				else
				{
					Containers = GetAllContainersInItem(Item);
				}

				for (auto It(Containers.CreateIterator()); It; ++It)
				{
					if constexpr (!std::is_same_v<TClass, UFaerieItemContainerBase>)
					{
						if (!StaticPredicates::ClassEqualsOrChildOf(*It, TClass::StaticClass()))
						{
							It.RemoveCurrent();
							continue;
						}
					}

					if (!PredicateTuple.TestAll(*It))
					{
						It.RemoveCurrent();
					}
				}

				return Type::Cast<TArray<TClass*>>(Containers);
			}

			// Create an iterator from this filter.
			[[nodiscard]] UE_REWRITE auto Iterate(const TNotNull<UFaerieItem*> Item) const &
			{
				// @TODO FIX THIS
				return TFilteredArrayIterator<TClass>(Emit(Item));
			}

			// Create an iterator from this filter.
			[[nodiscard]] UE_REWRITE auto Iterate(const TNotNull<UFaerieItem*> Item) &&
			{
				// @TODO FIX THIS
				return TFilteredArrayIterator<TClass>(Emit(Item));
			}

		private:
			Utils::TPredicateTuple<TPredicates...> PredicateTuple;
		};

		// Forward declare the default parameters of the template
		template <CItemContainerBase TClass = UFaerieItemContainerBase, EFilterFlags Flags = EFilterFlags::None, typename... Filter>
		class TFilter;

		// Iterate over the direct subobject containers in an item.
		UE_REWRITE FContainerIterator Iterate(const TNotNull<UFaerieItem*> Item)
		{
			return FContainerIterator(Item);
		}

		// Iterate over the all subobject containers in an item recursively.
		UE_REWRITE FRecursiveContainerIterator IterateRecursive(const TNotNull<UFaerieItem*> Item)
		{
			return FRecursiveContainerIterator(Item);
		}

		UE_REWRITE TFilter<UFaerieItemContainerBase> Filter()
		{
			return TFilter<>();
		}
	}
}
