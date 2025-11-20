// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieContainerFilter.h"
#include "FaerieContainerIterator.h"
#include "FaerieItemProxy.h"
#include "Misc/TVariant.h"

class UFaerieItemContainerBase;

namespace Faerie::Container
{
	template <typename T>
	using TStoragePredicate = TFunction<bool(T)>;

	using FItemPredicate = TStoragePredicate<const UFaerieItem*>;
	using FStackPredicate = TStoragePredicate<const FFaerieItemStackView&>;
	using FSnapshotPredicate = TStoragePredicate<const FFaerieItemSnapshot&>;
	using FVariantPredicate = TVariant<FEmptyVariantState, FItemPredicate, FStackPredicate, FSnapshotPredicate>;

	using FAddressPredicate = TStoragePredicate<const FFaerieAddress&>;

	template <typename T>
	using TStorageComparator = TFunction<bool(T, T)>;

	using FItemComparator = TStorageComparator<const UFaerieItem*>;
	using FStackComparator = TStorageComparator<const FFaerieItemStackView&>;
	using FSnapshotComparator = TStorageComparator<const FFaerieItemSnapshot&>;
	using FVariantComparator = TVariant<FEmptyVariantState, FItemComparator, FStackComparator, FSnapshotComparator>;

	using FAddressComparator = TStorageComparator<const FFaerieAddress&>;

	namespace Private
	{
		class FContainerReader
		{
		protected:
			static UFaerieItem* GetItem(const UFaerieItemContainerBase* Container, FEntryKey Key);
			static UFaerieItem* GetItem(const UFaerieItemContainerBase* Container, FFaerieAddress Address);
			static const UFaerieItem* ConstGetItem(const UFaerieItemContainerBase* Container, FEntryKey Key);
			static const UFaerieItem* ConstGetItem(const UFaerieItemContainerBase* Container, FFaerieAddress Address);
			static FFaerieItemStackView GetStackView(const UFaerieItemContainerBase* Container, FEntryKey Key);
			static FFaerieItemStackView GetStackView(const UFaerieItemContainerBase* Container, FFaerieAddress Address);
			static FFaerieItemSnapshot MakeSnapshot(const UFaerieItemContainerBase* Container, FEntryKey Key);
			static FFaerieItemSnapshot MakeSnapshot(const UFaerieItemContainerBase* Container, FFaerieAddress Address);

			// #@todo temporary accessors while implementing filters
			static FEntryKey GetAddressEntry(const UFaerieItemContainerBase* Container, FFaerieAddress Address);
			static TArray<FFaerieAddress> GetEntryAddresses(const UFaerieItemContainerBase* Container, FEntryKey Key);
		};
	}

	enum class EFilterFlags : uint32
	{
		None = 0,

		// @todo not yet supported
		ImmutableOnly = 1 << 0,

		// This filter is restricted to emitting mutable items
		MutableOnly = 1 << 1,

		// This filter masks out results by Entry Keys
		KeyFilter = 1 << 2,

		// This filter masks out results by Addresses
		AddressFilter = 1 << 3,

		// This filter can be invoked statically
		Static = 1 << 4,

		// This filter stores its results in internal memory
		InMemory = 1 << 5,
	};
	ENUM_CLASS_FLAGS(EFilterFlags)

	enum ESortDirection
	{
		Forward,
		Backward,
	};

	template <typename T>
	struct TFilterTraits
	{
		static constexpr EFilterFlags GrantFlags = EFilterFlags::None;
		static constexpr EFilterFlags RemoveFlags = EFilterFlags::None;
	};

	template <typename T, EFilterFlags Flags>
	consteval EFilterFlags CombineFilterFlags()
	{
		return (Flags & ~TFilterTraits<T>::RemoveFlags) | TFilterTraits<T>::GrantFlags;
	}

	struct FAERIEINVENTORY_API IEntryKeyFilter
	{
		virtual ~IEntryKeyFilter() = default;
		virtual bool Passes(FEntryKey Key) = 0;
	};

	struct FAERIEINVENTORY_API IAddressFilter
	{
		virtual ~IAddressFilter() = default;
		virtual bool Passes(FFaerieAddress Address) = 0;
	};

	struct FAERIEINVENTORY_API IItemDataFilter
	{
		virtual ~IItemDataFilter() = default;
		virtual bool Passes(const UFaerieItem* Item) = 0;
	};

	struct FAERIEINVENTORY_API ISnapshotFilter
	{
		virtual ~ISnapshotFilter() = default;
		virtual bool Passes(const FFaerieItemSnapshot& Snapshot) = 0;
	};

	class IFilter;

	struct FAERIEINVENTORY_API ICustomFilter
	{
		virtual ~ICustomFilter() = default;
		virtual bool Passes(IFilter& Filter) = 0;
	};

	template <typename T>
	concept CFilterType =
		TIsDerivedFrom<typename TRemoveReference<T>::Type, IEntryKeyFilter>::Value ||
		TIsDerivedFrom<typename TRemoveReference<T>::Type, IItemDataFilter>::Value ||
		TIsDerivedFrom<typename TRemoveReference<T>::Type, IAddressFilter>::Value ||
		TIsDerivedFrom<typename TRemoveReference<T>::Type, ISnapshotFilter>::Value ||
		TIsDerivedFrom<typename TRemoveReference<T>::Type, ICustomFilter>::Value;

	template <typename T>
	concept CAddressFilter =
		TIsDerivedFrom<typename TRemoveReference<T>::Type, IAddressFilter>::Value;

	class IFilter
	{
	public:
		virtual ~IFilter() = default;

		virtual void Run_Impl(IItemDataFilter&& Filter) = 0;
		virtual void Run_Impl(IEntryKeyFilter&& Filter) = 0;
		virtual void Run_Impl(IAddressFilter&& Filter) = 0;
		virtual void Run_Impl(ISnapshotFilter&& Filter) = 0;
		void Run_Impl(ICustomFilter&& Filter)
		{
			Filter.Passes(*this);
		}
		virtual void Invert_Impl() = 0;

		virtual void Reset() = 0;
		virtual int32 Num() const = 0;

		virtual FVirtualKeyIterator KeyRange() const = 0;
		virtual FVirtualAddressIterator AddressRange() const = 0;
		virtual FVirtualItemIterator ItemRange() const = 0;
		virtual FVirtualConstItemIterator ConstItemRange() const = 0;
	};

	class FVirtualFilter
	{
		template <EFilterFlags Flags, typename ImplType> friend class TFilter;

	public:
		FVirtualFilter(TUniquePtr<IFilter>&& FilterPtr) : FilterPtr(MoveTemp(FilterPtr)) {}

	private:
		template <CFilterType T>
		void Run(T&& Filter) { FilterPtr->Run_Impl(MoveTemp(Filter)); }

		// Implement the template version for compatibility, even though we cannot actually resolve this statically.
		template <CFilterType T>
		void RunStatic()
		{
			T Instance;
			FilterPtr->Run_Impl(MoveTemp(Instance));
		}

		void Invert() { FilterPtr->Invert_Impl(); }
		void Reset() { FilterPtr->Reset(); }
		int32 Num() const { return FilterPtr->Num(); }

		template <typename ResolveType>
		FORCEINLINE auto Range() const
		{
			if constexpr (std::is_same_v<ResolveType, FEntryKey>)
			{
				return FilterPtr->KeyRange();
			}
			else if constexpr (std::is_same_v<ResolveType, FFaerieAddress>)
			{
				return FilterPtr->AddressRange();
			}
			else if constexpr (std::is_same_v<ResolveType, const UFaerieItem*>)
			{
				return FilterPtr->ConstItemRange();
			}
			else if constexpr (std::is_same_v<ResolveType, UFaerieItem*>)
			{
				return FilterPtr->ItemRange();
			}
			else
			{
				return;
			}
		}

		TUniquePtr<IFilter> FilterPtr;
	};

	struct FIteratorSwitchThunk_Keys : Private::FContainerReader
	{
		FIteratorSwitchThunk_Keys(const UFaerieItemContainerBase* Container, const TArray<FEntryKey>& Array)
		  : Container(Container), Iterator(Array.CreateConstIterator())
		{
			if (static_cast<bool>(Iterator))
			{
				Addresses = GetEntryAddresses(Container, *Iterator);
				AddressIndex = INDEX_NONE;
				operator++();
				return;
			}
			else
			{
				Address = FFaerieAddress();
			}
		}

		FFaerieAddress operator*() const
		{
			return Address;
		}

		operator bool() const { return Address.IsValid(); }

		void operator++()
		{
			AddressIndex++;
			if (Addresses.IsValidIndex(AddressIndex))
			{
				Address = Addresses[AddressIndex];
			}
			else
			{
				++Iterator;
				if (static_cast<bool>(Iterator))
				{
					Addresses = GetEntryAddresses(Container, *Iterator);
					AddressIndex = INDEX_NONE;
					operator++();
					return;
				}
				else
				{
					Address = FFaerieAddress();
				}
			}
		}

		[[nodiscard]] FORCEINLINE bool operator!=(EIteratorType) const
		{
			// As long as we are valid, then we have not ended.
			return static_cast<bool>(*this);
		}

		const UFaerieItemContainerBase* Container;
		TArray<FEntryKey>::TConstIterator Iterator;
		TArray<FFaerieAddress> Addresses;
		int32 AddressIndex = INDEX_NONE;
		FFaerieAddress Address;
	};

	struct FIteratorSwitchThunk_Address : Private::FContainerReader
	{
		FIteratorSwitchThunk_Address(const UFaerieItemContainerBase* Container, const TArray<FFaerieAddress>& Array)
		  : Container(Container), Iterator(Array.CreateConstIterator())
		{
			operator++();
		}

		FEntryKey operator*() const
		{
			return Key;
		}

		operator bool() const { return Key != FEntryKey::InvalidKey; }

		void operator++()
		{
			const FEntryKey LastKey = Key;
			do
			{
				++Iterator;
				Key = GetAddressEntry(Container, *Iterator);
			}
			while (Key == LastKey && static_cast<bool>(Iterator));
		}

		[[nodiscard]] FORCEINLINE bool operator!=(EIteratorType) const
		{
			// As long as we are valid, then we have not ended.
			return static_cast<bool>(*this);
		}

		const UFaerieItemContainerBase* Container;
		TArray<FFaerieAddress>::TConstIterator Iterator;
		FEntryKey Key;
	};

	template <bool bAddress>
	class TMemoryFilter : Private::FContainerReader
	{
		template <EFilterFlags Flags, typename ImplType> friend class TFilter;
		using FFilterElement = std::conditional_t<bAddress, FFaerieAddress, FEntryKey>;
		using FFilterArray = TArray<FFilterElement>;

	public:
		TMemoryFilter(TArray<FFilterElement>&& FilterArray, const UFaerieItemContainerBase* Container)
		  : FilterMemory(MoveTemp(FilterArray)),
		  	Container(Container) {}

	private:
		template <CFilterType T>
		void Run(T&& Filter)
		{
			for (auto It(FilterMemory.CreateIterator()); It; ++It)
			{
				if constexpr (TIsDerivedFrom<T, IEntryKeyFilter>::Value)
				{
					if (bAddress)
					{
						unimplemented()
					}
					else
					{
						// Remove elements that fail the filter.
						if (!Filter.Passes(*It))
						{
							It.RemoveCurrent();
						}
					}
				}
				else if constexpr (TIsDerivedFrom<T, IAddressFilter>::Value)
				{
					if (bAddress)
					{
						// Remove elements that fail the filter.
						if (!Filter.Passes(*It))
						{
							It.RemoveCurrent();
						}
					}
					else
					{
						unimplemented()
					}
				}
				else if constexpr (TIsDerivedFrom<T, IItemDataFilter>::Value)
				{
					// Remove elements that fail the filter.
					if (const UFaerieItem* Item = ConstGetItem(Container, *It);
						!Filter.Passes(Item))
					{
						It.RemoveCurrent();
					}
				}
				else if constexpr (TIsDerivedFrom<T, ISnapshotFilter>::Value)
				{
					// Remove elements that fail the filter.
					if (const FFaerieItemSnapshot Snapshot = MakeSnapshot(Container, *It);
						!Filter.Passes(Snapshot))
					{
						It.RemoveCurrent();
					}
				}
				else if constexpr (TIsDerivedFrom<T, ICustomFilter>::Value)
				{
					Filter.Passes(*this);
				}
				else
				{
					unimplemented()
				}
			}
		}

		// Implement the template version for compatibility, even though we cannot actually resolve this statically.
		template <CFilterType T>
		void RunStatic()
		{
			for (auto It(FilterMemory.CreateIterator()); It; ++It)
			{
				if constexpr (TIsDerivedFrom<T, IEntryKeyFilter>::Value)
				{
					if constexpr (bAddress)
					{
						unimplemented()
					}
					else
					{
						// Remove elements that fail the filter.
						if (!T::StaticPasses(*It))
						{
							It.RemoveCurrent();
						}
					}
				}
				else if constexpr (TIsDerivedFrom<T, IAddressFilter>::Value)
				{
					if constexpr (bAddress)
					{
						// Remove elements that fail the filter.
						if (!T::StaticPasses(*It))
						{
							It.RemoveCurrent();
						}
					}
					else
					{
						unimplemented()
					}
				}
				else if constexpr (TIsDerivedFrom<T, IItemDataFilter>::Value)
				{
					// Remove elements that fail the filter.
					if (const UFaerieItem* Item = ConstGetItem(Container, *It);
						!T::StaticPasses(Item))
					{
						It.RemoveCurrent();
					}
				}
				else if constexpr (TIsDerivedFrom<T, ISnapshotFilter>::Value)
				{
					// Remove elements that fail the filter.
					if (const FFaerieItemSnapshot Snapshot = MakeSnapshot(Container, *It);
						!T::StaticPasses(Snapshot))
					{
						It.RemoveCurrent();
					}
				}
				else if constexpr (TIsDerivedFrom<T, ICustomFilter>::Value)
				{
					T::StaticPasses(*this);
				}
				else
				{
					unimplemented()
				}
			}
		}

		void Invert();
		void Reset();
		int32 Num() const { return FilterMemory.Num(); }

		struct FRangeSwitchThunk
		{
			FRangeSwitchThunk(const FFilterArray& Array, const UFaerieItemContainerBase* Container)
			  : Array(Array), Container(Container) {}

			[[nodiscard]] FORCEINLINE auto begin() const
			{
				if constexpr (std::is_same_v<FFilterElement, FFaerieAddress>)
				{
					return FIteratorSwitchThunk_Address(Container, Array);
				}
				else
				{
					return FIteratorSwitchThunk_Keys(Container, Array);
				}
			}
			[[nodiscard]] FORCEINLINE EIteratorType end () const { return End; }

			const FFilterArray& Array;
			const UFaerieItemContainerBase* Container;
		};

		template <bool Const>
		struct TItemIteratorThunk
		{
			TItemIteratorThunk(FFilterArray::TConstIterator&& It, const UFaerieItemContainerBase* Container)
				: Iterator(MoveTemp(It)), Container(Container) {}

			auto operator*()
			{
				if constexpr (Const)
				{
					return ConstGetItem(Container, *Iterator);
				}
				else
				{
					return GetItem(Container, *Iterator);
				}
			}

			operator bool() const { return static_cast<bool>(Iterator); }

			void operator++() { ++Iterator; }

			[[nodiscard]] FORCEINLINE bool operator!=(EIteratorType) const
			{
				// As long as we are valid, then we have not ended.
				return static_cast<bool>(*this);
			}

			FFilterArray::TConstIterator Iterator;
			const UFaerieItemContainerBase* Container;
		};

		template <bool Const>
		struct TItemRangeThunk
		{
			TItemRangeThunk(const FFilterArray& Array, const UFaerieItemContainerBase* Container)
			  : Array(Array), Container(Container) {}

			[[nodiscard]] FORCEINLINE TItemIteratorThunk<Const> begin() const
			{
				return TItemIteratorThunk(Array.CreateConstIterator(), Container);
			}
			[[nodiscard]] FORCEINLINE EIteratorType end () const { return End; }

			const FFilterArray& Array;
			const UFaerieItemContainerBase* Container;
		};

		template <typename ResolveType>
		FORCEINLINE auto Range() const
		{
			if constexpr (std::is_same_v<ResolveType, FEntryKey>)
			{
				if constexpr (bAddress)
				{
					unimplemented();
				}
				else
				{
					return FRangeSwitchThunk(FilterMemory, Container);
				}
			}
			else if constexpr (std::is_same_v<ResolveType, FFaerieAddress>)
			{
				if constexpr (bAddress)
				{
					return FilterMemory.CreateConstIterator();
				}
				else
				{
					return FRangeSwitchThunk(FilterMemory, Container);
				}
			}
			else if constexpr (std::is_same_v<ResolveType, const UFaerieItem*>)
			{
				return TItemRangeThunk<true>(FilterMemory, Container);
			}
			else if constexpr (std::is_same_v<ResolveType, UFaerieItem*>)
			{
				return TItemRangeThunk<false>(FilterMemory, Container);
			}
			else
			{
				return;
			}
		}

		template <ESortDirection Direction>
		void SortByItem(const FItemComparator& Sort)
		{
			if constexpr (Direction == ESortDirection::Forward)
			{
				Algo::Sort(FilterMemory,
					[Sort, this](const FFilterElement& A, const FFilterElement& B)
					{
						return Sort(ConstGetItem(Container, A), ConstGetItem(Container, B));
					});
			}
			else
			{
				Algo::Sort(FilterMemory,
					[Sort, this](const FFilterElement& A, const FFilterElement& B)
					{
						return !Sort(ConstGetItem(Container, A), ConstGetItem(Container, B));
					});
			}
		}

		template <ESortDirection Direction>
		void SortByStack(const FStackComparator& Sort)
		{
			if constexpr (Direction == ESortDirection::Forward)
			{
				Algo::Sort(FilterMemory,
					[Sort, this](const FFilterElement& A, const FFilterElement& B)
					{
						return Sort(GetStackView(Container, A), GetStackView(Container, B));
					});
			}
			else
			{
				Algo::Sort(FilterMemory,
					[Sort, this](const FFilterElement& A, const FFilterElement& B)
					{
						return !Sort(GetStackView(Container, A), GetStackView(Container, B));
					});
			}
		}

		template <ESortDirection Direction>
		void SortBySnapshot(const FSnapshotComparator& Sort)
		{
			if constexpr (Direction == ESortDirection::Forward)
			{
				Algo::Sort(FilterMemory,
					[Sort, this](const FFilterElement& A, const FFilterElement& B)
					{
						return Sort(MakeSnapshot(Container, A), MakeSnapshot(Container, B));
					});
			}
			else
			{
				Algo::Sort(FilterMemory,
					[Sort, this](const FFilterElement& A, const FFilterElement& B)
					{
						return !Sort(MakeSnapshot(Container, A), MakeSnapshot(Container, B));
					});
			}
		}

		template <ESortDirection Direction>
		void SortBy(const FVariantComparator& Sort)
		{
			switch (Sort.GetIndex())
			{
			case 1:
				// Compare Items
				SortByItem<Direction>(Sort.Get<FItemComparator>());
				break;
			case 2:
				// Compare Stacks
				SortByStack<Direction>(Sort.Get<FStackComparator>());
				break;
			case 3:
				// Compare Snapshots
				SortBySnapshot<Direction>(Sort.Get<FSnapshotComparator>());
				break;
			default:
				break;
			}
		}

		FFilterArray FilterMemory;
		const UFaerieItemContainerBase* Container;
	};

	template <EFilterFlags Flags, typename ImplType>
	class TFilter
	{
		template <CFilterType T> using TReturnFilterType =
			std::conditional_t<
				CombineFilterFlags<T, Flags>() == Flags,
				TFilter&,
				TFilter<CombineFilterFlags<T, Flags>(), ImplType>>;

	public:
		TFilter(const ImplType& Storage)
		  : Impl(Storage) {}

		TFilter(ImplType&& Storage)
		  : Impl(MoveTemp(Storage)) {}

		// Filter functions
		TFilter& Run(IItemDataFilter&& Filter)
		{
			Impl.Run(MoveTemp(Filter));
			return *this;
		}

		TFilter& Run(IEntryKeyFilter&& Filter)
		{
			Impl.Run(MoveTemp(Filter));
			return *this;
		}

		template <CAddressFilter FilterType UE_REQUIRES(EnumHasAnyFlags(Flags, EFilterFlags::AddressFilter))>
		TFilter& Run(FilterType&& Filter)
		{
			Impl.Run(MoveTemp(Filter));
			return *this;
		}

		TFilter& Run(ISnapshotFilter&& Filter)
		{
			Impl.Run(MoveTemp(Filter));
			return *this;
		}

		template <CFilterType T, typename... TArgs>
		[[nodiscard]] TReturnFilterType<T> Run(const TArgs&... Args)
		{
			T StructInstance(Args...);
			Impl.Run(MoveTemp(StructInstance));

			if constexpr (CombineFilterFlags<T, Flags>() == Flags)
			{
				return *this;
			}
			else
			{
				return TFilter<CombineFilterFlags<T, Flags>(), ImplType>(MoveTemp(Impl));
			}
		}

		template <CFilterType T>
		[[nodiscard]] TReturnFilterType<T> Run()
		{
			if constexpr (EnumHasAnyFlags(TFilterTraits<T>::TypeFlags, EFilterFlags::Static))
			{
				Impl.template RunStatic<T>();
			}
			else
			{
				T StructInstance;
				Impl.Run(MoveTemp(StructInstance));
			}

			if constexpr (CombineFilterFlags<T, Flags>() == Flags)
			{
				return *this;
			}
			else
			{
				return TFilter<CombineFilterFlags<T, Flags>(), ImplType>(MoveTemp(Impl));
			}
		}

		TFilter& Invert()
		{
			Impl.Invert();
			return *this;
		}

		// Get the number of items in this filter.
		int32 Num() const
		{
			return Impl.Num();
		}

	private:
		template <typename ResolveType>
		FORCEINLINE auto Range_Impl() const
		{
			return Impl.template Range<ResolveType>();
		}

		template <typename ResolveType>
		TArray<ResolveType> Emit_Impl() const
		{
			TArray<ResolveType> Out;
			Out.Reserve(Impl.Num());
			for (const ResolveType& Item : Impl.template Range<ResolveType>())
			{
				Out.Add(Item);
			}
			return Out;
		}

	public:
		// Range functions

		// Converts this filter into a ranged-loop of the Entry keys in this filter
		FORCEINLINE auto KeyRange() const
		{
			return Range_Impl<FEntryKey>();
		}

		// Converts this filter into a ranged-loop of the Addresses in this filter
		FORCEINLINE auto AddressRange() const
		{
			return Range_Impl<FFaerieAddress>();
		}

		// Converts this filter into a ranged-loop of the Items in this filter. Will emit const pointers unless first
		// filtered by FMutableFilter
		FORCEINLINE auto Items() const
		{
			if constexpr (EnumHasAnyFlags(Flags, EFilterFlags::MutableOnly))
			{
				return Range_Impl<UFaerieItem*>();
			}
			else
			{
				return Range_Impl<const UFaerieItem*>();
			}
		}

		FORCEINLINE TArray<FEntryKey> EmitKeys() const
		{
			return Emit_Impl<FEntryKey>();
		}

		FORCEINLINE TArray<FFaerieAddress> EmitAddresses() const
		{
			return Emit_Impl<FFaerieAddress>();
		}

		FORCEINLINE auto EmitItems() const
		{
			if constexpr (EnumHasAnyFlags(Flags, EFilterFlags::MutableOnly))
			{
				return Emit_Impl<UFaerieItem*>();
			}
			else
			{
				return Emit_Impl<const UFaerieItem*>();
			}
		}

		template <
			ESortDirection Direction = Forward
			UE_REQUIRES(!EnumHasAnyFlags(Flags, EFilterFlags::InMemory))
		>
		[[nodiscard]] auto SortBy(const FVariantComparator& Sort)
		{
			auto MemoryFilter = [this]()
			{
				// Switchover to in-filter memory
				if constexpr (EnumHasAnyFlags(Flags, EFilterFlags::AddressFilter))
				{
					return TMemoryFilter<true>(EmitAddresses(), Impl.GetContainer());
				}
				else
				{
					return TMemoryFilter<false>(EmitKeys(), Impl.GetContainer());
				}
			}();

			// Apply sort
			MemoryFilter.template SortBy<Direction>(Sort);

			// Move into new wrapper, marked with InMemory
			return TFilter<Flags | EFilterFlags::InMemory, TMemoryFilter<EnumHasAnyFlags(Flags, EFilterFlags::AddressFilter)>>(MoveTemp(MemoryFilter));
		}

		template <
			ESortDirection Direction = Forward
			UE_REQUIRES(EnumHasAnyFlags(Flags, EFilterFlags::InMemory))
		>
		TFilter& SortBy(const FVariantComparator& Sort)
		{
			Impl.template SortBy<Direction>(Sort);
			return *this;
		}

	private:
		ImplType Impl;
	};

	using FKeyFilter = TFilter<EFilterFlags::KeyFilter, FVirtualFilter>;
	using FAddressFilter = TFilter<EFilterFlags::AddressFilter, FVirtualFilter>;

	FAERIEINVENTORY_API FKeyFilter KeyFilter(const UFaerieItemContainerBase* Container);
	FAERIEINVENTORY_API FAddressFilter AddressFilter(const UFaerieItemContainerBase* Container);
}
