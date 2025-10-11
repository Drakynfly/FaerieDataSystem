// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "LoopUtils.h"
#include "TypeCastingUtils.h"
#include "FaerieInventoryConcepts.h"

class UFaerieItem;
class UFaerieItemContainerBase;

namespace Faerie
{
	// Get all container objects from inside a FaerieItem.
	FAERIEINVENTORY_API TArray<UFaerieItemContainerBase*> GetAllContainersInItem(UFaerieItem* Item);
	FAERIEINVENTORY_API TArray<UFaerieItemContainerBase*> GetAllContainersInItemRecursive(UFaerieItem* Item);

	FAERIEINVENTORY_API TArray<const UFaerieItem*> GetChildrenInItem(UFaerieItem* Item);
	FAERIEINVENTORY_API TArray<const UFaerieItem*> GetChildrenInItemRecursive(UFaerieItem* Item);

	namespace SubObject
	{
		namespace Filters
		{
			FAERIEINVENTORY_API void ByClass(TArray<UFaerieItemContainerBase*>& Containers, const TSubclassOf<UFaerieItemContainerBase>& Class);
		}

		class FIterator
		{
			using FStorageType = TArray<UFaerieItemContainerBase*>;

		public:
			explicit FIterator(FStorageType&& Array)
			  : Containers(MoveTemp(Array)),
				Iterator(Containers.CreateIterator()) {}

			explicit FIterator(const FStorageType& Array)
			  : Containers(Array),
				Iterator(Containers.CreateIterator()) {}

			explicit FIterator(UFaerieItem* Item);

			[[nodiscard]] FORCEINLINE UFaerieItemContainerBase* operator*() const { return Iterator.operator*(); }

			FORCEINLINE explicit operator bool() const { return static_cast<bool>(Iterator); }

			FORCEINLINE void operator++()
			{
				++Iterator;
			}

			[[nodiscard]] FORCEINLINE bool operator!=(EIteratorType) const
			{
				// As long as we are valid, then we have not ended.
				return static_cast<bool>(*this);
			}

			[[nodiscard]] FORCEINLINE FIterator begin() const { return *this; }
			[[nodiscard]] FORCEINLINE EIteratorType end () const { return End; }

		protected:
			FStorageType Containers;
			FStorageType::TIterator Iterator;
		};

		template <CItemContainerBase TClass>
		class TFilteredIterator
		{
			using FStorageType = TArray<UFaerieItemContainerBase*>;

		public:
			explicit TFilteredIterator(FStorageType&& Array)
			  : Containers(MoveTemp(Array)),
				Iterator(Containers.CreateIterator()) {}

			explicit TFilteredIterator(const FStorageType& Array)
			  : Containers(Array),
				Iterator(Containers.CreateIterator()) {}

		public:
			[[nodiscard]] FORCEINLINE TClass* operator*() const { return CastChecked<TClass>(Iterator.operator*()); }

			FORCEINLINE explicit operator bool() const { return static_cast<bool>(Iterator); }

			FORCEINLINE void operator++()
			{
				++Iterator;
			}

			[[nodiscard]] FORCEINLINE bool operator!=(EIteratorType) const
			{
				// As long as we are valid, then we have not ended.
				return static_cast<bool>(*this);
			}

			[[nodiscard]] FORCEINLINE TFilteredIterator begin() { return *this; }
			[[nodiscard]] FORCEINLINE EIteratorType end () const { return End; }

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

		template <typename... Filters>
		struct TFilterStorage
		{
			TFilterStorage() = default;

			TFilterStorage(TTuple<Filters...>&& InFilters)
			  : FilterTuple(MoveTemp(InFilters)) {}

			TFilterStorage(const TTuple<Filters...>& InFilters)
			  : FilterTuple(InFilters) {}

			template<typename NewFilter>
			TFilterStorage<Filters..., NewFilter> AddFilter(NewFilter&& NextView)
			{
				return FilterTuple.ApplyBefore([](const Filters&... InViews, const NewFilter& InNextView)
					{
						return TTuple<Filters..., NewFilter>(InViews..., InNextView);
					}, Forward<NewFilter>(NextView));
			}

			void Exec(TArray<UFaerieItemContainerBase*>& Containers) const
			{
				FilterTuple.ApplyBefore([&Containers](auto&... Filter)
				{
					(Filter.Exec(Containers), ...);
				});
			}

			TTuple<Filters...> FilterTuple{};
		};

		struct FClassFilter
		{
			void Exec(TArray<UFaerieItemContainerBase*>& Containers) const
			{
				Filters::ByClass(Containers, Class);
			}
			TSubclassOf<UFaerieItemContainerBase> Class;
		};

		template <CItemContainerBase TClass, EFilterFlags Flags, typename... TFilters>
		class TFilter
		{
		public:
			TFilter() = default;
			TFilter(TFilterStorage<TFilters...>&& FilterTuple)
			 : FilterStorage(MoveTemp(FilterTuple)) {}

			// @todo fix return types

			//TEnableIf<!EnumHasAnyFlags(Flags, EFilterFlags::Recursive),
			//	TFilter<TClass, Flags | EFilterFlags::Recursive>>::Type Recursive()
			auto Recursive()
			{
				return TFilter<TClass, Flags | EFilterFlags::Recursive>(MoveTemp(FilterStorage));
			}

			[[nodiscard]] auto ByClass(const TSubclassOf<UFaerieItemContainerBase>& Class)
			{
				FClassFilter NewFilter(Class);
				return TFilter<UFaerieItemContainerBase, Flags, TFilters..., FClassFilter>(FilterStorage.template AddFilter<FClassFilter>(MoveTemp(NewFilter)));
			}

			template <typename T UE_REQUIRES(TIsDerivedFrom<T, TClass>::Value)>
			[[nodiscard]] auto ByClass(const TSubclassOf<T>& Class)
			{
				FClassFilter NewFilter(Class);
				return TFilter<T, Flags, TFilters..., FClassFilter>(FilterStorage.AddFilter(NewFilter));
			}

			template <typename T UE_REQUIRES(TIsDerivedFrom<T, TClass>::Value)>
			[[nodiscard]] auto ByClass()
			{
				return TFilter<T, Flags, TFilters...>(MoveTemp(FilterStorage));
			}

			TArray<TClass*> Emit(UFaerieItem* Item) const
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
				if constexpr (!std::is_same_v<TClass, UFaerieItemContainerBase>)
				{
					Filters::ByClass(Containers, TClass::StaticClass());
				}
				FilterStorage.Exec(Containers);
				return Type::Cast<TArray<TClass*>>(Containers);
			}

			// Create an iterator from this filter.
			[[nodiscard]] FORCEINLINE auto Iterate(UFaerieItem* Item) const
			{
				return TFilteredIterator<TClass>(Type::Cast<TArray<UFaerieItemContainerBase*>>(Emit(Item)));
			}

		private:
			TFilterStorage<TFilters...> FilterStorage;
		};

		// Forward declare the default parameters of the template
		template <CItemContainerBase TClass = UFaerieItemContainerBase, EFilterFlags Flags = EFilterFlags::None, typename... Filter>
		class TFilter;

		// Iterate over the direct subobject containers in an item.
		FAERIEINVENTORY_API FORCEINLINE FIterator Iterate(UFaerieItem* Item)
		{
			return FIterator(Item);
		}

		FAERIEINVENTORY_API FORCEINLINE TFilter<UFaerieItemContainerBase> Filter()
		{
			return TFilter<>();
		}
	}
}
