// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieContainerIterator.h"
#include "FaerieItemProxy.h"
#include "InventoryDataStructs.h"

struct FInventoryContent;
struct FInventoryEntry;
struct FKeyedStack;
class UFaerieItemStorage;

namespace Faerie::Storage
{
	class FStorageDataAccess
	{
	protected:
		static const FInventoryContent& ReadInventoryContent(const UFaerieItemStorage& Storage);
		static FFaerieItemSnapshot MakeSnapshot(const UFaerieItemStorage& Storage, const int32 Index);
	};

	class FAERIEINVENTORY_API FIterator_AllEntries : FStorageDataAccess
	{
	public:
		FIterator_AllEntries(const UFaerieItemStorage& Storage);
		~FIterator_AllEntries();

		Container::FVirtualIterator ToInterface() const;

		void AdvanceEntry();

		UE_REWRITE FEntryKey operator*() const
		{
			return GetKey();
		}

		FEntryKey GetKey() const;
		const UFaerieItem* GetItem() const;

		UE_REWRITE void operator++()
		{
			AdvanceEntry();
		}

		UE_REWRITE explicit operator bool() const
		{
			return EntryIndex != INDEX_NONE;
		}

		[[nodiscard]] UE_REWRITE bool operator!=(EIteratorType) const
		{
			// As long as we are valid, then we have not ended.
			return static_cast<bool>(*this);
		}

		[[nodiscard]] UE_REWRITE const FIterator_AllEntries& begin() const { return *this; }
		[[nodiscard]] UE_REWRITE EIteratorType end () const { return End; }

	private:
		// Entry iteration
		const FInventoryContent* Content;
		int32 EntryIndex = INDEX_NONE;
	};

	class FAERIEINVENTORY_API FIterator_AllAddresses : FStorageDataAccess
	{
	public:
		FIterator_AllAddresses(const UFaerieItemStorage& Storage);

		~FIterator_AllAddresses();

		Container::FVirtualIterator ToInterface() const;

		void AdvanceEntry();

		UE_REWRITE FFaerieAddress operator*() const
		{
			return GetAddress();
		}

		FEntryKey GetKey() const;
		FFaerieAddress GetAddress() const;
		const UFaerieItem* GetItem() const;
		int32 GetStack() const;

		void operator++();

		UE_REWRITE explicit operator bool() const
		{
			return EntryIndex != INDEX_NONE && StackPtr != nullptr;
		}

		[[nodiscard]] UE_REWRITE bool operator!=(EIteratorType) const
		{
			// As long as we are valid, then we have not ended.
			return static_cast<bool>(*this);
		}

		[[nodiscard]] UE_REWRITE const FIterator_AllAddresses& begin() const { return *this; }
		[[nodiscard]] UE_REWRITE EIteratorType end  () const { return End; }

	private:
		// Entry iteration
		const FInventoryContent* Content;
		int32 EntryIndex = INDEX_NONE;

		// Stack iteration
		const FKeyedStack* StackPtr;
		int32 NumRemaining;
	};

	class FAERIEINVENTORY_API FIterator_MaskedEntries : FStorageDataAccess
	{
	public:
		FIterator_MaskedEntries(const UFaerieItemStorage& Storage, const TBitArray<>& EntryMask);
		FIterator_MaskedEntries(const FInventoryContent& Content, const TBitArray<>& EntryMask);
		FIterator_MaskedEntries(const FIterator_MaskedEntries& Other);

		~FIterator_MaskedEntries();

		Container::FVirtualIterator ToInterface() const;

		void AdvanceEntry();

		UE_REWRITE FFaerieAddress operator*() const
		{
			return GetAddress();
		}

		FEntryKey GetKey() const;
		FFaerieAddress GetAddress() const;
		const UFaerieItem* GetItem() const;

		void operator++();

		UE_REWRITE explicit operator bool() const
		{
			return StackPtr != nullptr && KeyMask.IsValidIndex(BitIterator.GetIndex());
		}

		[[nodiscard]] UE_REWRITE bool operator!=(EIteratorType) const
		{
			// As long as we are valid, then we have not ended.
			return static_cast<bool>(*this);
		}

		[[nodiscard]] UE_REWRITE FIterator_MaskedEntries begin() const { return FIterator_MaskedEntries(*Content, KeyMask); }
		[[nodiscard]] UE_REWRITE EIteratorType end () const { return End; }

	private:
		// Entry iteration
		const FInventoryContent* Content;
		const TBitArray<> KeyMask;
		TConstSetBitIterator<> BitIterator;

		// Stack iteration
		const FKeyedStack* StackPtr;
		int32 NumRemaining;
	};

	/*
	class FAERIEINVENTORY_API FStorageIterator_MaskedAddresses : FStorageDataAccess
	{
	public:
		FStorageIterator_MaskedAddresses(const UFaerieItemStorage* Storage, const TBitArray<>& AddressMask);

	private:
		// Entry iteration
		const FInventoryContent* Content;
		const TBitArray<> AddressMask;
	};
	*/

	class FAERIEINVENTORY_API FIterator_SingleEntry : FStorageDataAccess
	{
	public:
		FIterator_SingleEntry(const FInventoryEntry& Entry);
		FIterator_SingleEntry(const UFaerieItemStorage* Storage, const FEntryKey Key);
		FIterator_SingleEntry(const UFaerieItemStorage* Storage, const int32 Index);

		Container::FVirtualIterator ToInterface() const;

		FFaerieAddress operator*() const
		{
			return GetAddress();
		}

		FEntryKey GetKey() const;
		FFaerieAddress GetAddress() const;

		UE_REWRITE const UFaerieItem* GetItem() const
		{
			return EntryPtr->GetItem();
		}

		void operator++();

		UE_REWRITE explicit operator bool() const
		{
			return StackPtr && NumRemaining;
		}

		[[nodiscard]] UE_REWRITE bool operator!=(EIteratorType) const
		{
			// As long as we are valid, then we have not ended.
			return static_cast<bool>(*this);
		}

		[[nodiscard]] UE_REWRITE const FIterator_SingleEntry& begin() const { return *this; }
		[[nodiscard]] UE_REWRITE EIteratorType end () const { return End; }

	private:
		const FInventoryEntry* EntryPtr;
		const FKeyedStack* StackPtr;
		uint32 NumRemaining;
	};

	class FAERIEINVENTORY_API FIterator_MaskedEntries_ForInterface final : public Container::IIterator
	{
	public:
		FIterator_MaskedEntries_ForInterface(FIterator_MaskedEntries&& Inner) : Inner(Inner) {}

		//~ Container::IIterator
		UE_REWRITE virtual Container::FVirtualIterator Copy() const override { return Inner.ToInterface(); }
		UE_REWRITE virtual void Advance() override { ++Inner; }
		UE_REWRITE virtual FEntryKey ResolveKey() const override { return Inner.GetKey(); }
		UE_REWRITE virtual FFaerieAddress ResolveAddress() const override { return Inner.GetAddress(); }
		UE_REWRITE virtual const UFaerieItem* ResolveItem() const override { return Inner.GetItem(); }
		UE_REWRITE virtual bool IsValid() const override { return static_cast<bool>(Inner); }
		//~ Container::IIterator

	private:
		FIterator_MaskedEntries Inner;
	};

	class FAERIEINVENTORY_API FIterator_AllEntries_ForInterface final : public Container::IIterator
	{
	public:
		FIterator_AllEntries_ForInterface(const UFaerieItemStorage& Storage) : Inner(Storage) {}

		//~ Container::IIterator
		UE_REWRITE virtual Container::FVirtualIterator Copy() const override { return Inner.ToInterface(); }
		UE_REWRITE virtual void Advance() override { ++Inner; }
		UE_REWRITE virtual FEntryKey ResolveKey() const override { return Inner.GetKey(); }
		UE_REWRITE virtual FFaerieAddress ResolveAddress() const override { checkNoEntry(); return {}; }
		UE_REWRITE virtual const UFaerieItem* ResolveItem() const override { return Inner.GetItem(); }
		UE_REWRITE virtual bool IsValid() const override { return static_cast<bool>(Inner); }
		//~ Container::IIterator

	private:
		FIterator_AllEntries Inner;
	};

	class FAERIEINVENTORY_API FIterator_AllAddresses_ForInterface final : public Container::IIterator
	{
	public:
		FIterator_AllAddresses_ForInterface(const UFaerieItemStorage& Storage) : Inner(Storage) {}

		//~ Container::IIterator
		UE_REWRITE virtual Container::FVirtualIterator Copy() const override { return Inner.ToInterface(); }
		UE_REWRITE virtual void Advance() override { ++Inner; }
		UE_REWRITE virtual FEntryKey ResolveKey() const override { return Inner.GetKey(); }
		UE_REWRITE virtual FFaerieAddress ResolveAddress() const override { return Inner.GetAddress(); }
		UE_REWRITE virtual const UFaerieItem* ResolveItem() const override { return Inner.GetItem(); }
		UE_REWRITE virtual bool IsValid() const override { return static_cast<bool>(Inner); }
		//~ Container::IIterator

	private:
		FIterator_AllAddresses Inner;
	};

	class FAERIEINVENTORY_API FIterator_SingleEntry_ForInterface final : public Container::IIterator
	{
	public:
		FIterator_SingleEntry_ForInterface(const FIterator_SingleEntry& Inner) : Inner(Inner) {}

		//~ Container::IIterator
		UE_REWRITE virtual Container::FVirtualIterator Copy() const override { return Inner.ToInterface(); }
		UE_REWRITE virtual void Advance() override { ++Inner; }
		UE_REWRITE virtual FEntryKey ResolveKey() const override { return Inner.GetKey(); }
		UE_REWRITE virtual FFaerieAddress ResolveAddress() const override { return Inner.GetAddress(); }
		UE_REWRITE virtual const UFaerieItem* ResolveItem() const override { return Inner.GetItem(); }
		UE_REWRITE virtual bool IsValid() const override { return static_cast<bool>(Inner); }
		//~ Container::IIterator

	private:
		FIterator_SingleEntry Inner;
	};
}
