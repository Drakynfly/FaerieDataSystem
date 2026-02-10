// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemContainerStructs.h"
#include "FaerieItemDataViewBase.h"
#include "LoopUtils.h"

namespace Faerie::Container
{
	class IIterator : public ItemData::IViewBase
	{
	public:
		virtual FEntryKey ResolveKey() const = 0;
		virtual FFaerieAddress ResolveAddress() const = 0;

		virtual TUniquePtr<IIterator> Copy() const = 0;
		virtual void Advance() = 0;
	};

	namespace Private
	{
		class FIteratorAccess
		{
		public:
			FAERIEINVENTORY_API static TUniquePtr<IIterator> CreateEntryIteratorImpl(const TNotNull<const UFaerieItemContainerBase*> Container);
			FAERIEINVENTORY_API static TUniquePtr<IIterator> CreateAddressIteratorImpl(const TNotNull<const UFaerieItemContainerBase*> Container);
			FAERIEINVENTORY_API static TUniquePtr<IIterator> CreateSingleEntryIteratorImpl(const TNotNull<const UFaerieItemContainerBase*> Container, const FEntryKey Key);
		};

		template <bool IterateAddresses>
		TUniquePtr<IIterator> CreateIteratorImpl(const TNotNull<const UFaerieItemContainerBase*> Container)
		{
			if constexpr (IterateAddresses)
			{
				return FIteratorAccess::CreateAddressIteratorImpl(Container);
			}
			else
			{
				return FIteratorAccess::CreateEntryIteratorImpl(Container);
			}
		}
	}

	template <typename ResolveType, bool SkipToNextMutable>
	class TIterator
	{
	public:
		UE_REWRITE TIterator(const TNotNull<const UFaerieItemContainerBase*> Container)
		  : IteratorPtr(Private::CreateIteratorImpl<std::is_same_v<ResolveType, FFaerieAddress>>(Container))
		{
#if WITH_EDITOR
			UE_LOG(LogTemp, Verbose, TEXT("TIterator::Ctor from Container"));
#endif

			// When in non-const mode, jump to next mutable item
			if constexpr (SkipToNextMutable)
			{
				AdvanceWhileMutable();
			}
		}

		UE_REWRITE TIterator(TIterator&& Other)
		  : IteratorPtr(MoveTemp(Other.IteratorPtr))
		{
#if WITH_EDITOR
			UE_LOG(LogTemp, Verbose, TEXT("TIterator::Move Ctor"));
#endif

			// When in non-const mode, jump to next mutable item
			if constexpr (SkipToNextMutable)
			{
				AdvanceWhileMutable();
			}
		}

		UE_REWRITE TIterator(TUniquePtr<IIterator>&& Iterator)
		  : IteratorPtr(MoveTemp(Iterator))
		{
#if WITH_EDITOR
			UE_LOG(LogTemp, Verbose, TEXT("TIterator::Move Ctor"));
#endif

			// When in non-const mode, jump to next mutable item
			if constexpr (SkipToNextMutable)
			{
				AdvanceWhileMutable();
			}
		}

		UE_REWRITE TIterator(const TIterator& Other)
		  : IteratorPtr(Other.IteratorPtr ? Other.IteratorPtr->Copy() : TUniquePtr<IIterator>())
		{
#if WITH_EDITOR
			UE_LOG(LogTemp, Verbose, TEXT("TIterator::Copy Ctor"));
#endif

			// When in non-const mode, jump to next mutable item
			if constexpr (SkipToNextMutable)
			{
				AdvanceWhileMutable();
			}
		}

		UE_REWRITE explicit TIterator(const TUniquePtr<IIterator>& Iterator)
		  : IteratorPtr(Iterator ? Iterator->Copy() : TUniquePtr<IIterator>())
		{
#if WITH_EDITOR
			UE_LOG(LogTemp, Verbose, TEXT("TIterator::Copy Ctor"));
#endif

			// When in non-const mode, jump to next mutable item
			if constexpr (SkipToNextMutable)
			{
				AdvanceWhileMutable();
			}
		}

		const IIterator* GetPtr() const { return IteratorPtr.Get(); }

		[[nodiscard]] UE_REWRITE ResolveType operator*() const
		{
#if WITH_EDITOR
			UE_LOG(LogTemp, Verbose, TEXT("TIterator::operator*"));
#endif
			if constexpr (std::is_same_v<ResolveType, FEntryKey>)
			{
				return IteratorPtr->ResolveKey();
			}
			else if constexpr (std::is_same_v<ResolveType, FFaerieAddress>)
			{
				return IteratorPtr->ResolveAddress();
			}
			else if constexpr (std::is_same_v<ResolveType, const UFaerieItem*>)
			{
				return IteratorPtr->ResolveItem();
			}
			else if constexpr (std::is_same_v<ResolveType, UFaerieItem*>)
			{
				return IteratorPtr->ResolveItem()->MutateCast();
			}
			else
			{
				return ResolveType();
			}
		}

		void AdvanceWhileMutable()
		{
			while (static_cast<bool>(*this) && !IteratorPtr->ResolveItem()->CanMutate())
			{
				IteratorPtr->Advance();
			}
		}

		[[nodiscard]] UE_REWRITE FEntryKey GetKey() const { return IteratorPtr->ResolveKey(); }
		[[nodiscard]] UE_REWRITE FFaerieAddress GetAddress() const { return IteratorPtr->ResolveAddress(); }
		[[nodiscard]] UE_REWRITE const UFaerieItem* GetItem() const { return IteratorPtr->ResolveItem(); }

		UE_REWRITE void operator++()
		{
#if WITH_EDITOR
			UE_LOG(LogTemp, Verbose, TEXT("TIterator::operator++"));
#endif

			IteratorPtr->Advance();

			if constexpr (SkipToNextMutable)
			{
				// Then, when in non-const mode, jump to next mutable item
				AdvanceWhileMutable();
			}
		}

		UE_REWRITE explicit operator bool() const
		{
#if WITH_EDITOR
			const bool Result = IteratorPtr && IteratorPtr->IsValid();
			UE_LOG(LogTemp, Verbose, TEXT("TIterator::operator bool - returning '%hs'"), Result ? "true" : "false");
#endif
			return IteratorPtr && IteratorPtr->IsValid();
		}

		[[nodiscard]] UE_REWRITE bool operator!=(EIteratorType) const
		{
#if WITH_EDITOR
			const bool Result = static_cast<bool>(*this);
			UE_LOG(LogTemp, Verbose, TEXT("TIterator::operator!= (EIteratorType) - returning '%hs'"), Result ? "true" : "false");
#endif
			// As long as we are valid, then we have not ended.
			return static_cast<bool>(*this);
		}

		[[nodiscard]] UE_REWRITE const TIterator& begin() const
		{
			return *this;
		}
		[[nodiscard]] UE_REWRITE EIteratorType end() const { return End; }

	private:
		TUniquePtr<IIterator> IteratorPtr;
	};

	// Typedef for the rather ungainly parameter for filter predicates.
	using FIteratorPtr = const TNotNull<const IIterator*>;

	// Typedef for delegates that consume iterator predicate functions.
	using FIteratorPredicate = TDelegate<bool(FIteratorPtr)>;

	using FKeyIterator = TIterator<FEntryKey, false>;
	using FAddressIterator = TIterator<FFaerieAddress, false>;
	using FItemIterator = TIterator<UFaerieItem*, true>;
	using FConstItemIterator = TIterator<const UFaerieItem*, false>;

	// Enables ranged for-loops through each key in the container. Simple range with no filtering.
	FAERIEINVENTORY_API FKeyIterator KeyRange(TNotNull<const UFaerieItemContainerBase*> Container);

	// Enables ranged for-loops through each address in the container. Simple range with no filtering.
	FAERIEINVENTORY_API FAddressIterator AddressRange(TNotNull<const UFaerieItemContainerBase*> Container);

	// Enables ranged for-loops through each address in one entry. Simple range with no filtering.
	FAERIEINVENTORY_API FAddressIterator SingleKeyRange(TNotNull<const UFaerieItemContainerBase*> Container, FEntryKey Key);

	// Enables ranged for-loops through each item in the container. Simple range with no filtering.
	FAERIEINVENTORY_API FConstItemIterator ConstItemRange(TNotNull<const UFaerieItemContainerBase*> Container);

	// Enables ranged for-loops through each mutable item in the container. Automatically filtered to only return mutable instances.
	FAERIEINVENTORY_API FItemIterator ItemRange(TNotNull<const UFaerieItemContainerBase*> Container);
}
