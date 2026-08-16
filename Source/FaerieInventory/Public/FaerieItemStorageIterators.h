// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieContainerIterator.h"

struct FFaerieStorageContent;
struct FFaerieStorageEntry;
struct FFaerieKeyedStack;
class UFaerieItemStorage;

namespace Faerie::Container
{
	class FStorageDataAccess
	{
	protected:
		static const FFaerieStorageContent& ReadInventoryContent(const TNotNull<const UFaerieItemStorage*> Storage);
	};

	class FAERIEINVENTORY_API FIterator_AllEntries : FStorageDataAccess
	{
	public:
		UE_NONCOPYABLE(FIterator_AllEntries)

		FIterator_AllEntries(TNotNull<const UFaerieItemStorage*> Storage);
		~FIterator_AllEntries();

		void AdvanceEntry();

		UE_REWRITE FFaerieEntryKey operator*() const
		{
			return GetKey();
		}

		FFaerieEntryKey GetKey() const;
		const FFaerieItemInstance& GetInstance() const;
		int32 GetCopies() const;

		UE_REWRITE void operator++()
		{
			AdvanceEntry();
		}

		UE_REWRITE explicit operator bool() const
		{
			return EntryIndex != INDEX_NONE;
		}

		[[nodiscard]] UE_REWRITE bool operator!=(Utils::EIteratorType) const
		{
			// As long as we are valid, then we have not ended.
			return static_cast<bool>(*this);
		}

		[[nodiscard]] UE_REWRITE const FIterator_AllEntries& begin() const { return *this; }
		[[nodiscard]] UE_REWRITE Utils::EIteratorType end() const { return Utils::End; }

	private:
		// Entry iteration
		const FFaerieStorageContent& Content;
		int32 EntryIndex = INDEX_NONE;
	};

	class FAERIEINVENTORY_API FIterator_AllAddresses : FStorageDataAccess
	{
	public:
		UE_NONCOPYABLE(FIterator_AllAddresses)

		FIterator_AllAddresses(TNotNull<const UFaerieItemStorage*> Storage);

		~FIterator_AllAddresses();

		void AdvanceEntry();

		UE_REWRITE FFaerieAddress operator*() const
		{
			return GetAddress();
		}

		FFaerieEntryKey GetKey() const;
		FFaerieAddress GetAddress() const;
		const FFaerieItemInstance& GetInstance() const;
		int32 GetCopies() const;

		void operator++();

		UE_REWRITE explicit operator bool() const
		{
			return EntryIndex != INDEX_NONE && StackPtr != nullptr;
		}

		[[nodiscard]] UE_REWRITE bool operator!=(Utils::EIteratorType) const
		{
			// As long as we are valid, then we have not ended.
			return static_cast<bool>(*this);
		}

		[[nodiscard]] UE_REWRITE const FIterator_AllAddresses& begin() const { return *this; }
		[[nodiscard]] UE_REWRITE Utils::EIteratorType end  () const { return Utils::End; }

	private:
		// Entry iteration
		const FFaerieStorageContent& Content;
		int32 EntryIndex = INDEX_NONE;

		// Stack iteration
		const FFaerieKeyedStack* StackPtr = nullptr;
		int32 NumRemaining;
	};

	class FAERIEINVENTORY_API FIterator_SingleEntry : FStorageDataAccess
	{
	public:
		UE_NONCOPYABLE(FIterator_SingleEntry)

		FIterator_SingleEntry(const FFaerieStorageEntry& InEntry);
		FIterator_SingleEntry(TNotNull<const UFaerieItemStorage*> Storage, const FFaerieEntryKey Key);
		FIterator_SingleEntry(TNotNull<const UFaerieItemStorage*> Storage, const int32 Index);

		const FFaerieKeyedStack& operator*() const
		{
			return *StackPtr;
		}

		FFaerieEntryKey GetKey() const;
		FFaerieAddress GetAddress() const;
		const FFaerieItemInstance& GetInstance() const;
		int32 GetCopies() const;

		void operator++();

		UE_REWRITE explicit operator bool() const { return !!StackPtr; }

		[[nodiscard]] UE_REWRITE bool operator!=(Utils::EIteratorType) const
		{
			// As long as we are valid, then we have not ended.
			return static_cast<bool>(*this);
		}

		[[nodiscard]] UE_REWRITE const FIterator_SingleEntry& begin() const { return *this; }
		[[nodiscard]] UE_REWRITE Utils::EIteratorType end() const { return Utils::End; }

	private:
		const FFaerieStorageEntry& Entry;
		const FFaerieKeyedStack* StackPtr = nullptr;
		uint32 NumRemaining;
	};

	class FAERIEINVENTORY_API FIterator_AllEntries_WithInterface final : public IEntryIterator
	{
	public:
		FIterator_AllEntries_WithInterface(const TNotNull<const UFaerieItemStorage*> Storage)
		  : Storage(Storage), Inner(Storage) {}

		//~ ItemData::ViewBase
		UE_REWRITE virtual FFaerieEntryKey ResolveKey() const override { return Inner.GetKey(); }
		UE_REWRITE virtual TOptional<FFaerieItemInstance> GetItemInstance() const override { return Inner.GetInstance(); }
		UE_REWRITE virtual int32 GetCopies() const override { return Inner.GetCopies(); }
		virtual const IFaerieItemOwnerInterface* GetItemOwner() const override;
		//~ ItemData::ViewBase

		//~ Container::IAddressIterator
		UE_REWRITE virtual void Advance() override { ++Inner; }
		UE_REWRITE virtual bool IsValid() const override { return static_cast<bool>(Inner); }
		//~ Container::IAddressIterator

	private:
		const TNotNull<const UFaerieItemStorage*> Storage;
		FIterator_AllEntries Inner;
	};

	class FAERIEINVENTORY_API FIterator_AllAddresses_WithInterface final : public IAddressIterator
	{
	public:
		FIterator_AllAddresses_WithInterface(const TNotNull<const UFaerieItemStorage*> Storage)
		  : Storage(Storage), Inner(Storage) {}

		//~ ItemData::ViewBase
		UE_REWRITE virtual FFaerieEntryKey ResolveKey() const override { return Inner.GetKey(); }
		UE_REWRITE virtual FFaerieAddress ResolveAddress() const override { return Inner.GetAddress(); }
		UE_REWRITE virtual TOptional<FFaerieItemInstance> GetItemInstance() const override { return Inner.GetInstance(); }
		UE_REWRITE virtual int32 GetCopies() const override { return Inner.GetCopies(); }
		virtual const IFaerieItemOwnerInterface* GetItemOwner() const override;
		//~ ItemData::ViewBase

		//~ Container::IAddressIterator
		UE_REWRITE virtual void Advance() override { ++Inner; }
		UE_REWRITE virtual bool IsValid() const override { return static_cast<bool>(Inner); }
		//~ Container::IAddressIterator

	private:
		const TNotNull<const UFaerieItemStorage*> Storage;
		FIterator_AllAddresses Inner;
	};

	class FAERIEINVENTORY_API FIterator_SingleEntry_WithInterface final : public IAddressIterator
	{
	public:
		FIterator_SingleEntry_WithInterface(const TNotNull<const UFaerieItemStorage*> Storage, const FFaerieStorageEntry& Entry)
		  : Storage(Storage), Inner(Entry) {}

		//~ ItemData::ViewBase
		UE_REWRITE virtual FFaerieEntryKey ResolveKey() const override { return Inner.GetKey(); }
		UE_REWRITE virtual FFaerieAddress ResolveAddress() const override { return Inner.GetAddress(); }
		UE_REWRITE virtual TOptional<FFaerieItemInstance> GetItemInstance() const override { return Inner.GetInstance(); }
		UE_REWRITE virtual int32 GetCopies() const override { return Inner.GetCopies(); }
		virtual const IFaerieItemOwnerInterface* GetItemOwner() const override;
		//~ ItemData::ViewBase

		//~ Container::IAddressIterator
		UE_REWRITE virtual void Advance() override { ++Inner; }
		UE_REWRITE virtual bool IsValid() const override { return static_cast<bool>(Inner); }
		//~ Container::IAddressIterator

	private:
		const TNotNull<const UFaerieItemStorage*> Storage;
		FIterator_SingleEntry Inner;
	};
}
