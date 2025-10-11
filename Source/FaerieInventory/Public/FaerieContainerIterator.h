// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItem.h"
#include "FaerieItemContainerStructs.h"
#include "LoopUtils.h"

class UFaerieItem;

namespace Faerie::Container
{
	class FVirtualIterator;

	class IIterator
	{
	public:
		virtual ~IIterator() = default;

		virtual FVirtualIterator Copy() const = 0;
		virtual FEntryKey ResolveKey() const = 0;
		virtual FFaerieAddress ResolveAddress() const = 0;
		virtual const UFaerieItem* ResolveItem() const = 0;
		virtual void Advance() = 0;
		virtual bool IsValid() const = 0;
	};

	class FVirtualIterator
	{
		template <typename ResolveType, typename ImplType> friend class TIterator;

	public:
		FVirtualIterator(TUniquePtr<IIterator>&& IteratorPtr) : IteratorPtr(MoveTemp(IteratorPtr)) {}
		FVirtualIterator(const FVirtualIterator& Other)
		  : IteratorPtr(Other.Copy().IteratorPtr) {}

		FORCEINLINE explicit operator bool() const { return IsValid(); }

		FORCEINLINE void operator++()
		{
			IteratorPtr->Advance();
		}

		FVirtualIterator ToInterface() const
		{
			return Copy();
		}

	private:
		FORCEINLINE FVirtualIterator Copy() const
		{
			if (IteratorPtr.IsValid())
			{
				return IteratorPtr->Copy();
			}
			return {nullptr};
		}
		FORCEINLINE FEntryKey GetKey() const { return IteratorPtr->ResolveKey(); }
		FORCEINLINE FFaerieAddress GetAddress() const { return IteratorPtr->ResolveAddress(); }
		FORCEINLINE const UFaerieItem* GetItem() const { return IteratorPtr->ResolveItem(); }
		FORCEINLINE bool IsValid() const { return IteratorPtr.IsValid() && IteratorPtr->IsValid(); }

		TUniquePtr<IIterator> IteratorPtr;
	};

	template <typename ResolveType, typename ImplType>
	class TIterator
	{
	public:
		FORCEINLINE explicit TIterator(ImplType&& Impl)
		  : IteratorImpl(MoveTemp(Impl))
		{
#if WITH_EDITOR
			UE_LOG(LogTemp, Verbose, TEXT("TIterator::Move Ctor"));
#endif
		}

		FORCEINLINE explicit TIterator(const ImplType& Impl)
		  : IteratorImpl(Impl)
		{
#if WITH_EDITOR
			UE_LOG(LogTemp, Verbose, TEXT("TIterator::Copy Ctor"));
#endif
		}

		// Copies this iterator into a new iterator.
		[[nodiscard]] TIterator<ResolveType, FVirtualIterator> Copy()
		{
			return TIterator<ResolveType, FVirtualIterator>(IteratorImpl.ToInterface());
		}

		[[nodiscard]] FORCEINLINE ResolveType operator*() const
		{
#if WITH_EDITOR
			UE_LOG(LogTemp, Verbose, TEXT("TIterator::operator*"));
#endif
			if constexpr (std::is_same_v<ResolveType, FEntryKey>)
			{
				return IteratorImpl.GetKey();
			}
			else if constexpr (std::is_same_v<ResolveType, FFaerieAddress>)
			{
				return IteratorImpl.GetAddress();
			}
			else if constexpr (std::is_same_v<ResolveType, const UFaerieItem*>)
			{
				return IteratorImpl.GetItem();
			}
			else if constexpr (std::is_same_v<ResolveType, UFaerieItem*>)
			{
				return IteratorImpl.GetItem()->MutateCast();
			}
			else
			{
				return ResolveType();
			}
		}

		FORCEINLINE TIterator& operator++()
		{
#if WITH_EDITOR
			UE_LOG(LogTemp, Verbose, TEXT("TIterator::operator++"));
#endif
			++IteratorImpl;
			return *this;
		}

		FORCEINLINE explicit operator bool() const
		{
#if WITH_EDITOR
			const bool Result = static_cast<bool>(IteratorImpl);
			UE_LOG(LogTemp, Verbose, TEXT("TIterator::operator bool - returning '%hs'"), Result ? "true" : "false");
#endif
			return static_cast<bool>(IteratorImpl);
		}

		[[nodiscard]] FORCEINLINE bool operator!=(EIteratorType) const
		{
#if WITH_EDITOR
			const bool Result = static_cast<bool>(*this);
			UE_LOG(LogTemp, Verbose, TEXT("TIterator::operator!= (EIteratorType) - returning '%hs'"), Result ? "true" : "false");
#endif
			// As long as we are valid, then we have not ended.
			return static_cast<bool>(*this);
		}

		FFaerieAddress GetAddress() const { return IteratorImpl.ResolveAddress(); }
		const UFaerieItem* GetItem() const { return IteratorImpl.ResolveItem(); }

		[[nodiscard]] FORCEINLINE TIterator begin() const
		{
			return TIterator(IteratorImpl);
		}
		[[nodiscard]] FORCEINLINE EIteratorType end () const { return End; }

	private:
		ImplType IteratorImpl;
	};

	using FVirtualKeyIterator = TIterator<FEntryKey, FVirtualIterator>;
	using FVirtualAddressIterator = TIterator<FFaerieAddress, FVirtualIterator>;
	using FVirtualItemIterator = TIterator<UFaerieItem*, FVirtualIterator>;
	using FVirtualConstItemIterator = TIterator<const UFaerieItem*, FVirtualIterator>;

	// Enables ranged for-loops through each key in the container. Simple range with no filtering.
	FAERIEINVENTORY_API FVirtualKeyIterator KeyRange(const UFaerieItemContainerBase* Container);

	// Enables ranged for-loops through each address in the container. Simple range with no filtering.
	FAERIEINVENTORY_API FVirtualAddressIterator AddressRange(const UFaerieItemContainerBase* Container);

	// Enables ranged for-loops through each item in the container. Simple range with no filtering.
	FAERIEINVENTORY_API FVirtualConstItemIterator ItemRange(const UFaerieItemContainerBase* Container);
}
