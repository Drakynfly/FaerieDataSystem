// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "LoopUtils.h"
#include "FaerieInventoryConcepts.h"
#include "FaerieItemContainerBase.h"
#include "PredicateTuple.h"

#include "Templates/SubclassOf.h"

class UFaerieItemContainerBase;
class UFaerieItemStackContainer;
class UFaerieItemStorage;

// Too many functions here, but merging them
namespace Faerie::SubObject
{
	// Returns immutable default containers.
	FAERIEINVENTORY_API void GetTemplateContainersInInstanceDirect(const FFaerieItemInstance& Item, TAdderRef<TNotNull<const UFaerieItemContainerBase*>> Containers, TNotNull<const UClass*> Class = UFaerieItemContainerBase::StaticClass());
	FAERIEINVENTORY_API void GetTemplateContainersInInstanceRecursive(const FFaerieItemInstance& Item, TAdderRef<TNotNull<const UFaerieItemContainerBase*>> Containers, TNotNull<const UClass*> Class = UFaerieItemContainerBase::StaticClass());

	// Looks for runtime containers only. As such, requires an entity manager.
	FAERIEINVENTORY_API bool HasContainerInInstanceDirect_Stack(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, TNotNull<const UFaerieItemStackContainer*> TestContainer);
	FAERIEINVENTORY_API bool HasContainerInInstanceDirect_Storage(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, TNotNull<const UFaerieItemStorage*> TestContainer);
	FAERIEINVENTORY_API bool HasContainerInInstanceRecursive_Stack(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, TNotNull<const UFaerieItemStackContainer*> TestContainer);
	FAERIEINVENTORY_API bool HasContainerInInstanceRecursive_Storage(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, TNotNull<const UFaerieItemStorage*> TestContainer);

	// Returns runtime containers only. As such, requires an entity manager.
	FAERIEINVENTORY_API void GetContainersInInstanceDirect_All(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, TAdderRef<TNotNull<UFaerieItemContainerBase*>> Containers);
	FAERIEINVENTORY_API void GetContainersInInstanceDirect_Stack(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, TAdderRef<TNotNull<UFaerieItemStackContainer*>> Containers);
	FAERIEINVENTORY_API void GetContainersInInstanceDirect_Storage(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, TAdderRef<TNotNull<UFaerieItemStorage*>> Containers);
	FAERIEINVENTORY_API void GetContainersInInstanceRecursive_All(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, TAdderRef<TNotNull<UFaerieItemContainerBase*>> Containers);
	FAERIEINVENTORY_API void GetContainersInInstanceRecursive_Stack(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, TAdderRef<TNotNull<UFaerieItemStackContainer*>> Containers);
	FAERIEINVENTORY_API void GetContainersInInstanceRecursive_Storage(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, TAdderRef<TNotNull<UFaerieItemStorage*>> Containers);

	FAERIEINVENTORY_API void GetContainersInInstanceDirect(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, TAdderRef<TNotNull<UFaerieItemContainerBase*>> Containers, TNotNull<const UClass*> Class);
	FAERIEINVENTORY_API void GetContainersInInstanceRecursive(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, TAdderRef<TNotNull<UFaerieItemContainerBase*>> Containers, TNotNull<const UClass*> Class);

	// Returns runtime containers only. As such, requires an entity manager.
	template <typename TFaerieItemContainerBase>
	void GetContainersInInstanceDirect(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, TAdderRef<TNotNull<TFaerieItemContainerBase*>> Containers)
	{
		if constexpr (std::is_same_v<UFaerieItemContainerBase, TFaerieItemContainerBase>)
		{
			GetContainersInInstanceDirect_All(EntityManager, Item, Containers);
		}
		else if constexpr (std::is_same_v<UFaerieItemStackContainer, TFaerieItemContainerBase>)
		{
			GetContainersInInstanceDirect_Stack(EntityManager, Item, Containers);
		}
		else if constexpr (std::is_same_v<UFaerieItemStorage, TFaerieItemContainerBase>)
		{
			GetContainersInInstanceDirect_Storage(EntityManager, Item, Containers);
		}
	}

	// Returns runtime containers only. As such, requires an entity manager.
	template <typename TFaerieItemContainerBase>
	void GetContainersInInstanceRecursive(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, TAdderRef<TNotNull<TFaerieItemContainerBase*>> Containers)
	{
		if constexpr (std::is_same_v<UFaerieItemContainerBase, TFaerieItemContainerBase>)
		{
			GetContainersInInstanceRecursive_All(EntityManager, Item, Containers);
		}
		else if constexpr (std::is_same_v<UFaerieItemStackContainer, TFaerieItemContainerBase>)
		{
			GetContainersInInstanceRecursive_Stack(EntityManager, Item, Containers);
		}
		else if constexpr (std::is_same_v<UFaerieItemStorage, TFaerieItemContainerBase>)
		{
			GetContainersInInstanceRecursive_Storage(EntityManager, Item, Containers);
		}
	}

	FAERIEINVENTORY_API void GetChildrenInItem(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, TAdderRef<FFaerieItemProxy> OutProxies);
	FAERIEINVENTORY_API void GetChildrenInItemRecursive(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, TAdderRef<FFaerieItemProxy> OutProxies);

	/**
	 * Iterates over all item containers in a Faerie Item
	 */
	class FContainerIterator
	{
		using FStorageType = TArray<TNotNull<UFaerieItemContainerBase*>>;

	public:
		explicit FContainerIterator(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item);

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
		using FStorageType = TArray<TNotNull<TClass*>>;

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

		void Emit(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, TArray<TNotNull<TClass*>>& OutContainer) const
		{
			if constexpr (EnumHasAnyFlags(Flags, EFilterFlags::Recursive))
			{
				GetContainersInInstanceRecursive<TClass>(EntityManager, Item, OutContainer);
			}
			else
			{
				GetContainersInInstanceDirect<TClass>(EntityManager, Item, OutContainer);
			}

			for (auto It(OutContainer.CreateIterator()); It; ++It)
			{
				if (!PredicateTuple.TestAll(&EntityManager, *It))
				{
					It.RemoveCurrent();
				}
			}
		}

		// Create an iterator from this filter.
		[[nodiscard]] UE_REWRITE auto Iterate(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item) const &
		{
			// @TODO FIX THIS
			TArray<TNotNull<TClass*>> Containers;
			Emit(EntityManager, Item, Containers);
			return TFilteredArrayIterator<TClass>(MoveTemp(Containers));
		}

		// Create an iterator from this filter.
		[[nodiscard]] UE_REWRITE auto Iterate(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item) &&
		{
			// @TODO FIX THIS
			TArray<TNotNull<TClass*>> Containers;
			Emit(EntityManager, Item, Containers);
			return TFilteredArrayIterator<TClass>(MoveTemp(Containers));
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
