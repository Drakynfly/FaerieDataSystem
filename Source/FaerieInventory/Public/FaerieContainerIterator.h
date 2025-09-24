// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItem.h"
#include "FaerieItemContainerStructs.h"
#include "LoopUtils.h"

class UFaerieItem;

namespace Faerie
{
	class FDefaultIteratorStorage;

	class IContainerIterator
	{
	public:
		virtual ~IContainerIterator() = default;

		virtual FDefaultIteratorStorage Copy() const = 0;
		virtual FEntryKey ResolveKey() const = 0;
		virtual FFaerieAddress ResolveAddress() const = 0;
		virtual const UFaerieItem* ResolveItem() const = 0;
		virtual void Advance() = 0;
		virtual bool IsValid() const = 0;
		virtual bool Equals(const TUniquePtr<IContainerIterator>& Other) const = 0;
	};

	class FDefaultIteratorStorage
	{
		template <typename ResolveType, typename ImplType> friend class TContainerIterator;

	public:
		FDefaultIteratorStorage(TUniquePtr<IContainerIterator>&& IteratorPtr) : IteratorPtr(MoveTemp(IteratorPtr)) {}
		FDefaultIteratorStorage(const FDefaultIteratorStorage& Other)
		  : IteratorPtr(Other.Copy().IteratorPtr) {}

		FORCEINLINE explicit operator bool() const { return IsValid(); }

		FORCEINLINE void operator++()
		{
			IteratorPtr->Advance();
		}

		[[nodiscard]] FORCEINLINE bool operator!=(const FDefaultIteratorStorage& Rhs) const
		{
			// If we are valid
			if (IteratorPtr.IsValid())
			{
				// If both are valid
				if (Rhs.IteratorPtr.IsValid())
				{
					// Run implementation Equals
					return !IteratorPtr->Equals(Rhs.IteratorPtr);
				}
				// Run
				return IteratorPtr->IsValid();
			}

			// If we are invalid, and the other is valid
			if (Rhs.IteratorPtr.IsValid())
			{
				return Rhs.IteratorPtr->IsValid();
			}

			// Both are invalid, so we are the same
			return false;
		}

		FDefaultIteratorStorage ToInterface() const
		{
			return Copy();
		}

	private:
		FORCEINLINE FDefaultIteratorStorage Copy() const
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

		TUniquePtr<IContainerIterator> IteratorPtr;
	};

	template <typename ResolveType, typename ImplType>
	class TContainerIterator
	{
	public:
		FORCEINLINE explicit TContainerIterator(ImplType&& Impl)
		  : IteratorImpl(MoveTemp(Impl))
		{
#if WITH_EDITOR
			UE_LOG(LogTemp, Display, TEXT("TContainerIterator::Move Ctor"));
#endif
		}

		FORCEINLINE explicit TContainerIterator(const ImplType& Impl)
		  : IteratorImpl(Impl)
		{
#if WITH_EDITOR
			UE_LOG(LogTemp, Display, TEXT("TContainerIterator::Copy Ctor"));
#endif
		}

		// Copies this iterator into a new iterator.
		TContainerIterator<ResolveType, FDefaultIteratorStorage> Copy()
		{
			return TContainerIterator<ResolveType, FDefaultIteratorStorage>(IteratorImpl.ToInterface());
		}

		FORCEINLINE ResolveType operator*() const
		{
#if WITH_EDITOR
			UE_LOG(LogTemp, Display, TEXT("TContainerIterator::operator*"));
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

		FORCEINLINE TContainerIterator& operator++()
		{
#if WITH_EDITOR
			UE_LOG(LogTemp, Display, TEXT("TContainerIterator::operator++"));
#endif
			++IteratorImpl;
			return *this;
		}

		FORCEINLINE explicit operator bool() const
		{
#if WITH_EDITOR
			const bool Result = static_cast<bool>(IteratorImpl);
			UE_LOG(LogTemp, Display, TEXT("TContainerIterator::operator bool - returning '%hs'"), Result ? "true" : "false");
#endif
			return static_cast<bool>(IteratorImpl);
		}

		[[nodiscard]] FORCEINLINE bool operator!=(const TContainerIterator& Rhs) const
		{
#if WITH_EDITOR
			const bool Result = IteratorImpl != Rhs.IteratorImpl;
			UE_LOG(LogTemp, Display, TEXT("TContainerIterator::operator!= (Other) - returning '%hs'"), Result ? "true" : "false");
#endif
			return IteratorImpl != Rhs.IteratorImpl;
		}

		[[nodiscard]] FORCEINLINE bool operator!=(EIteratorType) const
		{
#if WITH_EDITOR
			const bool Result = static_cast<bool>(*this);
			UE_LOG(LogTemp, Display, TEXT("TContainerIterator::operator!= (EIteratorType) - returning '%hs'"), Result ? "true" : "false");
#endif
			// As long we are valid, then we have not ended.
			return static_cast<bool>(*this);
		}

		FFaerieAddress GetAddress() const { return IteratorImpl.ResolveAddress(); }
		const UFaerieItem* GetItem() const { return IteratorImpl.ResolveItem(); }

		[[nodiscard]] FORCEINLINE TContainerIterator begin() const
		{
			return TContainerIterator(IteratorImpl);
		}
		[[nodiscard]] FORCEINLINE EIteratorType end () const { return End; }

	private:
		ImplType IteratorImpl;
	};

	using FDefaultKeyIterator = TContainerIterator<FEntryKey, FDefaultIteratorStorage>;
	using FDefaultAddressIterator = TContainerIterator<FFaerieAddress, FDefaultIteratorStorage>;
	using FDefaultItemIterator = TContainerIterator<UFaerieItem*, FDefaultIteratorStorage>;
	using FDefaultConstItemIterator = TContainerIterator<const UFaerieItem*, FDefaultIteratorStorage>;

	// Enables ranged for-loops through each key in the container. Simple range with no filtering.
	FAERIEINVENTORY_API FDefaultKeyIterator KeyRange(const UFaerieItemContainerBase* Container);

	// Enables ranged for-loops through each address in the container. Simple range with no filtering.
	FAERIEINVENTORY_API FDefaultAddressIterator AddressRange(const UFaerieItemContainerBase* Container);

	// Enables ranged for-loops through each item in the container. Simple range with no filtering.
	FAERIEINVENTORY_API FDefaultConstItemIterator ItemRange(const UFaerieItemContainerBase* Container);
}
