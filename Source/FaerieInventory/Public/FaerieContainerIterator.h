// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "LoopUtils.h"
#include "DebuggingFlags.h"
#include "FaerieItem.h"
#include "FaerieItemContainerBase.h"

namespace Faerie::Container
{
#if FAERIE_DEBUG
	constexpr bool EnableDebugLogs = false;
#define LOG_ITERATOR_MESSAGE(Message) if constexpr (EnableDebugLogs) { UE_LOG(LogTemp, Warning, TEXT(Message)); }
#define LOG_ITERATOR_MESSAGE_FMT(Message, ...) if constexpr (EnableDebugLogs) { UE_LOG(LogTemp, Warning, TEXT(Message), __VA_ARGS__); }
#else
#define LOG_ITERATOR_MESSAGE(Message)
#define LOG_ITERATOR_MESSAGE_FMT(Message, ...)
#endif

	/*
	 * Iterators are a special kind of Data View, that can step to the "next" valid view.
	 */
	class IEntryIterator : public IEntryView
	{
	public:
		// Advance this iterator to the next valid view.
		virtual void Advance() = 0;

		// Is the iterator valid
		virtual bool IsValid() const = 0;
	};

	/*
	 * Iterators are a special kind of Data View, that can step to the "next" valid view.
	 */
	class IAddressIterator : public IAddressView
	{
	public:
		// Advance this iterator to the next valid view.
		virtual void Advance() = 0;

		// Is the iterator valid
		virtual bool IsValid() const = 0;
	};

	namespace Private
	{
		class FIteratorAccess
		{
		public:
			FAERIEINVENTORY_API static TUniquePtr<IEntryIterator> CreateEntryIteratorImpl(const TNotNull<const UFaerieItemContainerBase*> Container);
			FAERIEINVENTORY_API static TUniquePtr<IAddressIterator> CreateAddressIteratorImpl(const TNotNull<const UFaerieItemContainerBase*> Container);
			FAERIEINVENTORY_API static TUniquePtr<IAddressIterator> CreateSingleEntryIteratorImpl(const TNotNull<const UFaerieItemContainerBase*> Container, const FFaerieEntryKey Key);
		};

		template <bool IterateAddresses>
		auto CreateIteratorImpl(const TNotNull<const UFaerieItemContainerBase*> Container)
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

	enum EIteratorMutabilityToggle
	{
		OnlyMutableInstances,
		AllInstances
	};

	template <typename ResolveType, EIteratorMutabilityToggle Toggle, typename ViewInterface>
	class TIterator
	{
	public:
		UE_REWRITE explicit TIterator(const TNotNull<const UFaerieItemContainerBase*> Container)
		  : IteratorPtr(Private::CreateIteratorImpl<std::is_same_v<ResolveType, FFaerieAddress>>(Container))
		{
			LOG_ITERATOR_MESSAGE("TIterator::Ctor from Container")

			// When in non-const mode, jump to next mutable item
			if constexpr (Toggle == OnlyMutableInstances)
			{
				AdvanceWhileImmutable();
			}
		}

		UE_REWRITE explicit TIterator(TIterator&& Other)
		  : IteratorPtr(MoveTemp(Other.IteratorPtr))
		{
			LOG_ITERATOR_MESSAGE("TIterator::Move Ctor");

			// When in non-const mode, jump to next mutable item
			if constexpr (Toggle == OnlyMutableInstances)
			{
				AdvanceWhileImmutable();
			}
		}

		UE_REWRITE explicit TIterator(TUniquePtr<ViewInterface>&& Iterator)
		  : IteratorPtr(MoveTemp(Iterator))
		{
			LOG_ITERATOR_MESSAGE("TIterator::Move Ctor");

			// When in non-const mode, jump to next mutable item
			if constexpr (Toggle == OnlyMutableInstances)
			{
				AdvanceWhileImmutable();
			}
		}

		// @todo if this compiles, move the FFaerieItemDataView to a member
		[[nodiscard]] UE_REWRITE ItemData::FValidatedDataView GetPtr() const { return FFaerieItemDataView(IteratorPtr.Get()); }

		[[nodiscard]] UE_REWRITE ResolveType operator*() const
		{
			LOG_ITERATOR_MESSAGE("TIterator::operator*");

			if constexpr (std::is_same_v<ResolveType, FFaerieEntryKey>)
			{
				return IteratorPtr->ResolveKey();
			}
			else if constexpr (std::is_same_v<ResolveType, FFaerieAddress>)
			{
				return IteratorPtr->ResolveAddress();
			}
			else if constexpr (std::is_same_v<ResolveType, ItemData::FReference>)
			{
				return IteratorPtr->ResolveItem();
			}
			else if constexpr (std::is_same_v<ResolveType, ItemData::FMutableReference>)
			{
				// Implicit conversion to FReference
				return IteratorPtr->ResolveItem();
			}
			else
			{
				return *reinterpret_cast<ResolveType*>(nullptr);
			}
		}

		void AdvanceWhileImmutable()
		{
			while (static_cast<bool>(*this) && !IteratorPtr->ResolveItem()->IsMutable())
			{
				IteratorPtr->Advance();
			}
		}

		[[nodiscard]] UE_REWRITE FFaerieEntryKey GetKey() const { return IteratorPtr->ResolveKey(); }
		[[nodiscard]] UE_REWRITE FFaerieAddress GetAddress() const { return IteratorPtr->ResolveAddress(); }
		[[nodiscard]] UE_REWRITE int32 GetCopies() const { return IteratorPtr->ResolveCopies(); }
		[[nodiscard]] UE_REWRITE auto GetReference() const
		{
			if constexpr (Toggle == OnlyMutableInstances)
			{
				return ItemData::FMutableReference(IteratorPtr->ResolveItem());
			}
			else
			{
				return ItemData::FReference(IteratorPtr->ResolveItem());
			}
		}

		// As we are in FaerieInventory, we can cast to the actual UObject type.
		[[nodiscard]] UE_REWRITE const UFaerieItemContainerBase* GetOwner() const { return CastChecked<UFaerieItemContainerBase>(IteratorPtr->ResolveOwner()); }

		[[nodiscard]] UE_REWRITE FFaerieItemProxy GetProxy() const { return GetOwner()->Proxy(GetAddress()); }

		UE_REWRITE void operator++()
		{
			LOG_ITERATOR_MESSAGE("TIterator::operator++");

			IteratorPtr->Advance();

			if constexpr (Toggle == OnlyMutableInstances)
			{
				// Then, when in non-const mode, jump to next mutable item
				AdvanceWhileImmutable();
			}
		}

		UE_REWRITE explicit operator bool() const
		{
			LOG_ITERATOR_MESSAGE_FMT("TIterator::operator bool - returning '%hs'", IteratorPtr && IteratorPtr->IsValid() ? "true" : "false")
			return IteratorPtr && IteratorPtr->IsValid();
		}

		[[nodiscard]] UE_REWRITE bool operator!=(Utils::EIteratorType) const
		{
			LOG_ITERATOR_MESSAGE_FMT("TIterator::operator!= (EIteratorType) - returning '%hs'", IteratorPtr && IteratorPtr->IsValid() ? "true" : "false")

			// As long as we are valid, then we have not ended.
			return static_cast<bool>(*this);
		}

		[[nodiscard]] UE_REWRITE const TIterator& begin() const { return *this; }
		[[nodiscard]] UE_REWRITE Utils::EIteratorType end() const { return Utils::End; }

	private:
		TUniquePtr<ViewInterface> IteratorPtr;
	};

	using FKeyIterator = TIterator<FFaerieEntryKey, AllInstances, IEntryIterator>;
	using FAddressIterator = TIterator<FFaerieAddress, AllInstances, IAddressIterator>;
	using FItemIterator = TIterator<ItemData::FReference, AllInstances, IEntryIterator>;
	using FMutableItemIterator = TIterator<ItemData::FMutableReference, OnlyMutableInstances, IEntryIterator>;

	// Enables ranged for-loops through each key in the container. Simple range with no filtering.
	FAERIEINVENTORY_API FKeyIterator KeyRange(TNotNull<const UFaerieItemContainerBase*> Container);

	// Enables ranged for-loops through each address in the container. Simple range with no filtering.
	FAERIEINVENTORY_API FAddressIterator AddressRange(TNotNull<const UFaerieItemContainerBase*> Container);

	// Enables ranged for-loops through each address in one entry. Simple range with no filtering.
	FAERIEINVENTORY_API FAddressIterator SingleKeyRange(TNotNull<const UFaerieItemContainerBase*> Container, FFaerieEntryKey Key);

	// Enables ranged for-loops through each item in the container. Simple range with no filtering.
	FAERIEINVENTORY_API FItemIterator ItemRange(TNotNull<const UFaerieItemContainerBase*> Container);

	// Enables ranged for-loops through each mutable item in the container. Automatically filtered to only return mutable instances.
	FAERIEINVENTORY_API FMutableItemIterator MutableItemRange(TNotNull<const UFaerieItemContainerBase*> Container);
}
