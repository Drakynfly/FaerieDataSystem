// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "LoopUtils.h"
#include "DebuggingFlags.h"
#include "FaerieContainerViewBases.h"
#include "FaerieInventoryLog.h"
#include "FaerieItemContainerBase.h"
#include "FaerieItemOwnership.h"
#include "FaerieItemProxy.h"
#include "MassEntityManager.h"

#define FAE_API FAERIEINVENTORY_API

namespace Faerie::Container
{
#if FAERIE_DEBUG
	constexpr bool EnableDebugLogs = false;
#define LOG_ITERATOR_MESSAGE(Message) if constexpr (EnableDebugLogs) { UE_LOGF(LogFaerieInventory, Warning, Message); }
#define LOG_ITERATOR_MESSAGE_FMT(Message, ...) if constexpr (EnableDebugLogs) { UE_LOGF(LogFaerieInventory, Warning, Message, __VA_ARGS__); }
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
			FAE_API static TUniquePtr<IEntryIterator> CreateEntryIteratorImpl(const TNotNull<const UFaerieItemContainerBase*> Container);
			FAE_API static TUniquePtr<IAddressIterator> CreateAddressIteratorImpl(const TNotNull<const UFaerieItemContainerBase*> Container);
			FAE_API static TUniquePtr<IAddressIterator> CreateSingleEntryIteratorImpl(const TNotNull<const UFaerieItemContainerBase*> Container, const FFaerieEntryKey Key);
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
		UE_REWRITE explicit TIterator(const FMassEntityManager* EntityManager, const TNotNull<const UFaerieItemContainerBase*> Container)
		  : EntityManager(EntityManager), IteratorPtr(Private::CreateIteratorImpl<std::is_same_v<ResolveType, FFaerieAddress>>(Container))
		{
			LOG_ITERATOR_MESSAGE("TIterator::Ctor from Container")

			// When in non-const mode, jump to next mutable item
			if constexpr (Toggle == OnlyMutableInstances)
			{
				AdvanceWhileImmutable();
			}
		}

		UE_REWRITE explicit TIterator(const FMassEntityManager* EntityManager, TIterator&& Other)
		  : EntityManager(EntityManager), IteratorPtr(MoveTemp(Other.IteratorPtr))
		{
			LOG_ITERATOR_MESSAGE("TIterator::Move Ctor");

			// When in non-const mode, jump to next mutable item
			if constexpr (Toggle == OnlyMutableInstances)
			{
				AdvanceWhileImmutable();
			}
		}

		UE_REWRITE explicit TIterator(const FMassEntityManager* EntityManager, TUniquePtr<ViewInterface>&& Iterator)
		  : EntityManager(EntityManager), IteratorPtr(MoveTemp(Iterator))
		{
			LOG_ITERATOR_MESSAGE("TIterator::Move Ctor");

			// When in non-const mode, jump to next mutable item
			if constexpr (Toggle == OnlyMutableInstances)
			{
				AdvanceWhileImmutable();
			}
		}

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
			else if constexpr (std::is_same_v<ResolveType, FMassEntityHandle>)
			{
				return IteratorPtr->GetEntityHandle();
			}
			else
			{
				return ResolveType();
			}
		}

		void AdvanceWhileImmutable()
		{
			while (static_cast<bool>(*this) && !GetSingleFrameProxy().IsMutable())
			{
				IteratorPtr->Advance();
			}
		}

		[[nodiscard]] UE_REWRITE FFaerieEntryKey GetKey() const { return IteratorPtr->ResolveKey(); }
		[[nodiscard]] UE_REWRITE FFaerieAddress GetAddress() const { return IteratorPtr->ResolveAddress(); }
		[[nodiscard]] UE_REWRITE int32 GetCopies() const { return IteratorPtr->GetCopies(); }
		[[nodiscard]] UE_REWRITE FMassEntityHandle GetEntityHandle() const { return IteratorPtr->GetEntityHandle(); }
		[[nodiscard]] UE_REWRITE const UFaerieItemContainerBase* GetOwner() const
		{
			return GetItemOwner(*EntityManager, GetEntityHandle());
		}

		[[nodiscard]] FFaerieItemProxy GetSingleFrameProxy() const UE_LIFETIMEBOUND
		{
			return FFaerieItemProxy(FFaerieItemProxy::ESingleFrame, EntityManager ? EntityManager->AsShared().ToSharedPtr() : nullptr, IteratorPtr.Get());
		}

		[[nodiscard]] UE_REWRITE FFaerieItemProxy GetPersistentProxy() const
		{
			if constexpr (std::is_same_v<ResolveType, FFaerieAddress>)
			{
				return GetOwner()->Proxy(*EntityManager, GetAddress());
			}
			else
			{
				return GetOwner()->Proxy(*EntityManager, GetKey());
			}
		}

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
		const FMassEntityManager* EntityManager;
		TUniquePtr<ViewInterface> IteratorPtr;
	};

	using FKeyIterator = TIterator<FFaerieEntryKey, AllInstances, IEntryIterator>;
	using FAddressIterator = TIterator<FFaerieAddress, AllInstances, IAddressIterator>;
	using FItemIterator = TIterator<FMassEntityHandle, AllInstances, IEntryIterator>;
	using FMutableItemIterator = TIterator<FMassEntityHandle, OnlyMutableInstances, IEntryIterator>;

	// Enables ranged for-loops through each key in the container. Simple range with no filtering.
	FAE_API FKeyIterator KeyRange(const FMassEntityManager* EntityManager, TNotNull<const UFaerieItemContainerBase*> Container);

	// Enables ranged for-loops through each address in the container. Simple range with no filtering.
	FAE_API FAddressIterator AddressRange(const FMassEntityManager* EntityManager, TNotNull<const UFaerieItemContainerBase*> Container);

	// Enables ranged for-loops through each address in one entry. Simple range with no filtering.
	FAE_API FAddressIterator SingleKeyRange(const FMassEntityManager* EntityManager, TNotNull<const UFaerieItemContainerBase*> Container, FFaerieEntryKey Key);

	// Enables ranged for-loops through each item in the container. Simple range with no filtering.
	FAE_API FItemIterator ItemRange(const FMassEntityManager* EntityManager, TNotNull<const UFaerieItemContainerBase*> Container);

	// Enables ranged for-loops through each mutable item in the container. Automatically filtered to only return mutable instances.
	FAE_API FMutableItemIterator MutableItemRange(const FMassEntityManager* EntityManager, TNotNull<const UFaerieItemContainerBase*> Container);
}

#undef FAE_API