// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "LoopUtils.h"
#include "TypeCastingUtils.h"
#include "FaerieInventoryConcepts.h"
#include "FaerieItemContainerBase.h"
#include "PredicateTuple.h"

#include "Templates/SubclassOf.h"

class UFaerieItemContainerBase;

namespace Faerie::SubObject
{
	// Returns immutable default containers.
	FAERIEINVENTORY_API void GetTemplateContainersInInstanceDirect(const FFaerieItemInstance& Item, TAdderRef<TNotNull<const UFaerieItemContainerBase*>> Containers, TNotNull<const UClass*> Class = UFaerieItemContainerBase::StaticClass());

	// Returns immutable default containers.
	FAERIEINVENTORY_API void GetTemplateContainersInInstanceRecursive(const FFaerieItemInstance& Item, TArray<TNotNull<const UFaerieItemContainerBase*>>& Containers, TNotNull<const UClass*> Class = UFaerieItemContainerBase::StaticClass());

	// Returns runtime containers only. As such, requires an entity manager.
	FAERIEINVENTORY_API void GetContainersInInstanceDirect(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, TAdderRef<TNotNull<UFaerieItemContainerBase*>> Containers, TNotNull<const UClass*> Class = UFaerieItemContainerBase::StaticClass());

	// Returns runtime containers only. As such, requires an entity manager.
	FAERIEINVENTORY_API void GetContainersInInstanceRecursive(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, TArray<TNotNull<UFaerieItemContainerBase*>>& Containers, TNotNull<const UClass*> Class = UFaerieItemContainerBase::StaticClass());

	template <typename TFaerieItemContainerBase>
	[[nodiscard]] bool HasContainerInInstanceDirect(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, TNotNull<const TFaerieItemContainerBase*> TestContainer);

	template <typename TFaerieItemContainerBase>
	[[nodiscard]] bool HasContainerInInstanceRecursive(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, TNotNull<const TFaerieItemContainerBase*> TestContainer);

	FAERIEINVENTORY_API void GetChildrenInItem(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, TArray<FFaerieItemProxy>& OutProxies);
	FAERIEINVENTORY_API void GetChildrenInItemRecursive(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, TArray<FFaerieItemProxy>& OutProxies);

	namespace StaticPredicates
	{
		FAERIEINVENTORY_API bool ClassEquals(TNotNull<const UFaerieItemContainerBase*> Container, const TSubclassOf<UFaerieItemContainerBase>& Class);
		FAERIEINVENTORY_API bool ClassEqualsOrChildOf(TNotNull<const UFaerieItemContainerBase*> Container, const TSubclassOf<UFaerieItemContainerBase>& Class);
	}

	/**
	 * Iterates over all item containers in a Faerie Item
	 */
	class FContainerIterator
	{
		using FStorageType = TArray<TNotNull<UFaerieItemContainerBase*>>;

	public:
		explicit FContainerIterator(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item);
		~FContainerIterator() {}

		[[nodiscard]] UE_REWRITE UFaerieItemContainerBase* operator*() const { return Iterator.operator*(); }

		UE_REWRITE explicit operator bool() const { return static_cast<bool>(Iterator); }

		void operator++();

		[[nodiscard]] UE_REWRITE bool operator!=(Utils::EIteratorType) const
		{
			// As long as we are valid, then we have not ended.
			return static_cast<bool>(*this);
		}

		[[nodiscard]] UE_REWRITE const FContainerIterator& begin() const { return *this; }
		[[nodiscard]] UE_REWRITE Utils::EIteratorType end() const { return Utils::End; }

	protected:
		FStorageType Containers;
		FStorageType::TIterator Iterator;
	};

	/**
	 * Iterates over all Item Containers in a Faerie Item, and any found in sub-Items
	 */
	class FRecursiveContainerIterator
	{
		using FStorageType = TArray<UFaerieItemContainerBase*>;

	public:
		explicit FRecursiveContainerIterator(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item);

		[[nodiscard]] UE_REWRITE UFaerieItemContainerBase* operator*() const { return Iterator.operator*(); }

		UE_REWRITE explicit operator bool() const { return static_cast<bool>(Iterator); }

		UE_REWRITE void operator++() { ++Iterator; }

		[[nodiscard]] UE_REWRITE bool operator!=(Utils::EIteratorType) const
		{
			// As long as we are valid, then we have not ended.
			return static_cast<bool>(*this);
		}

		[[nodiscard]] UE_REWRITE const FRecursiveContainerIterator& begin() const { return *this; }
		[[nodiscard]] UE_REWRITE Utils::EIteratorType end() const { return Utils::End; }

	protected:
		FStorageType Containers;
		FStorageType::TIterator Iterator;
	};

	template <Container::CItemContainerBase TClass>
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
		[[nodiscard]] UE_REWRITE TNotNull<TClass*> operator*() const { return CastChecked<TClass>(Iterator.operator*()); }

		UE_REWRITE explicit operator bool() const { return static_cast<bool>(Iterator); }

		UE_REWRITE void operator++()
		{
			++Iterator;
		}

		[[nodiscard]] UE_REWRITE bool operator!=(Utils::EIteratorType) const
		{
			// As long as we are valid, then we have not ended.
			return static_cast<bool>(*this);
		}

		[[nodiscard]] UE_REWRITE const TFilteredArrayIterator& begin() const { return *this; }
		[[nodiscard]] UE_REWRITE Utils::EIteratorType end() const { return Utils::End; }

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
		bool Exec(const TNotNull<const UFaerieItemContainerBase*> Container) const
		{
			return StaticPredicates::ClassEqualsOrChildOf(Container, Class);
		}
		TSubclassOf<UFaerieItemContainerBase> Class;
	};

	struct FClassFilterExact
	{
		bool Exec(const TNotNull<const UFaerieItemContainerBase*> Container) const
		{
			return StaticPredicates::ClassEquals(Container, Class);
		}
		TSubclassOf<UFaerieItemContainerBase> Class;
	};

	template <Container::CItemContainerBase TClass, EFilterFlags Flags, typename... TPredicates>
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

		[[nodiscard]] TArray<TClass*> Emit(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item) const
		{
			TArray<TNotNull<UFaerieItemContainerBase*>> Containers;
			if constexpr (EnumHasAnyFlags(Flags, EFilterFlags::Recursive))
			{
				GetContainersInInstanceRecursive(EntityManager, Item, Containers);
			}
			else
			{
				GetContainersInInstanceDirect(EntityManager, Item, Containers);
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

				if (!PredicateTuple.TestAll(&EntityManager, *It))
				{
					It.RemoveCurrent();
				}
			}

			return Utils::Cast<TArray<TClass*>>(Containers);
		}

		// Create an iterator from this filter.
		[[nodiscard]] UE_REWRITE auto Iterate(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item) const &
		{
			// @TODO FIX THIS
			return TFilteredArrayIterator<TClass>(Emit(EntityManager, Item));
		}

		// Create an iterator from this filter.
		[[nodiscard]] UE_REWRITE auto Iterate(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item) &&
		{
			// @TODO FIX THIS
			return TFilteredArrayIterator<TClass>(Emit(EntityManager, Item));
		}

	private:
		Utils::TPredicateTuple<TPredicates...> PredicateTuple;
	};

	// Forward declare the default parameters of the template
	template <Container::CItemContainerBase TClass = UFaerieItemContainerBase, EFilterFlags Flags = EFilterFlags::None, typename... Filter>
	class TFilter;

	// Iterate over the direct containers in an item.
	UE_REWRITE FContainerIterator Iterate(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item)
	{
		return FContainerIterator(EntityManager, Item);
	}

	// Iterate over the all containers in an item recursively.
	UE_REWRITE FRecursiveContainerIterator IterateRecursive(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item)
	{
		return FRecursiveContainerIterator(EntityManager, Item);
	}

	UE_REWRITE TFilter<UFaerieItemContainerBase> Filter()
	{
		return TFilter<>();
	}
}
