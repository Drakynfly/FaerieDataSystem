// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieContainerFilter.h"
#include "FaerieContainerIterator.h"
#include "FaerieItemProxy.h"

class UFaerieItemContainerBase;

namespace Faerie
{
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
	};
	ENUM_CLASS_FLAGS(EFilterFlags)

	template <typename T>
	struct TFilterProperties
	{
		static constexpr EFilterFlags GrantFlags = EFilterFlags::None;
		static constexpr EFilterFlags RemoveFlags = EFilterFlags::None;
	};

	template <typename T, EFilterFlags Flags>
	consteval EFilterFlags CombineFilterFlags()
	{
		return (Flags & ~TFilterProperties<T>::RemoveFlags) | TFilterProperties<T>::GrantFlags;
	}

	struct FAERIEINVENTORY_API IEntryKeyFilter
	{
		virtual ~IEntryKeyFilter() = default;
		virtual bool Passes(FEntryKey Key) = 0;
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

	template <typename T>
	concept CFilterType =
		TIsDerivedFrom<typename TRemoveReference<T>::Type, IEntryKeyFilter>::Value ||
		TIsDerivedFrom<typename TRemoveReference<T>::Type, IItemDataFilter>::Value ||
		TIsDerivedFrom<typename TRemoveReference<T>::Type, ISnapshotFilter>::Value;

	class IContainerFilter
	{
	public:
		virtual ~IContainerFilter() = default;

		virtual void Run_Impl(IItemDataFilter&& Filter) = 0;
		virtual void Run_Impl(IEntryKeyFilter&& Filter) = 0;
		virtual void Run_Impl(ISnapshotFilter&& Filter) = 0;
		virtual void Invert_Impl() = 0;

		virtual void Reset() = 0;
		virtual int32 Num() const = 0;

		virtual FDefaultKeyIterator KeyRange() const = 0;
		virtual FDefaultAddressIterator AddressRange() const = 0;
		virtual FDefaultItemIterator ItemRange() const = 0;
		virtual FDefaultConstItemIterator ConstItemRange() const = 0;
	};

	class FDefaultFilterStorage
	{
		template <EFilterFlags Flags, typename ImplType> friend class TContainerFilter;

	public:
		FDefaultFilterStorage(TUniquePtr<IContainerFilter>&& FilterPtr) : FilterPtr(MoveTemp(FilterPtr)) {}

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

		TUniquePtr<IContainerFilter> FilterPtr;
	};

	template <EFilterFlags Flags, typename ImplType>
	class TContainerFilter
	{
		template <CFilterType T> using TReturnFilterType =
			std::conditional_t<
				CombineFilterFlags<T, Flags>() == Flags,
				TContainerFilter&,
				TContainerFilter<CombineFilterFlags<T, Flags>(), ImplType>>;

	public:
		TContainerFilter(const ImplType& Storage)
		  : Impl(Storage) {}

		TContainerFilter(ImplType&& Storage)
		  : Impl(MoveTemp(Storage)) {}

		// Filter functions
		TContainerFilter& Run(IItemDataFilter&& Filter)
		{
			Impl.Run(MoveTemp(Filter));
			return *this;
		}

		TContainerFilter& Run(IEntryKeyFilter&& Filter)
		{
			Impl.Run(MoveTemp(Filter));
			return *this;
		}

		TContainerFilter& Run(ISnapshotFilter&& Filter)
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
				return TContainerFilter<CombineFilterFlags<T, Flags>(), ImplType>(MoveTemp(Impl));
			}
		}

		template <CFilterType T>
		[[nodiscard]] TReturnFilterType<T> Run()
		{
			if constexpr (EnumHasAnyFlags(TFilterProperties<T>::TypeFlags, EFilterFlags::Static))
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
				return TContainerFilter<CombineFilterFlags<T, Flags>(), ImplType>(MoveTemp(Impl));
			}
		}

		TContainerFilter& Invert()
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
			for (ResolveType Item : Impl.template Range<ResolveType>())
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

	private:
		ImplType Impl;
	};

	using FContainerKeyFilter = TContainerFilter<EFilterFlags::KeyFilter, FDefaultFilterStorage>;
	using FContainerAddressFilter = TContainerFilter<EFilterFlags::AddressFilter, FDefaultFilterStorage>;

	using FSnapshotFilter = TDelegate<bool(const FFaerieItemSnapshot&)>;
	using FItemComparator = TDelegate<bool(const FFaerieItemSnapshot&, const FFaerieItemSnapshot&)>;

	FAERIEINVENTORY_API FContainerKeyFilter KeyFilter(const UFaerieItemContainerBase* Container);
	FAERIEINVENTORY_API FContainerAddressFilter AddressFilter(const UFaerieItemContainerBase* Container);
}
