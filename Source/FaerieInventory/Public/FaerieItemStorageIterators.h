// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieContainerIterator.h"
#include "FaerieItemProxy.h"
#include "InventoryDataStructs.h"

struct FInventoryContent;
struct FInventoryEntry;
struct FKeyedStack;
class UFaerieItemStorage;

namespace Faerie
{
	class FStorageDataAccess
	{
	protected:
		const FInventoryContent& ReadInventoryContent(const UFaerieItemStorage& Storage);
		FFaerieItemSnapshot MakeSnapshot(const UFaerieItemStorage& Storage, const int32 Index);
	};

	class FAERIEINVENTORY_API FStorageIterator_AllEntries : FStorageDataAccess
	{
	public:
		FStorageIterator_AllEntries(const UFaerieItemStorage* Storage);

		~FStorageIterator_AllEntries();

		//FDefaultIteratorStorage ToInterface() const;

		void AdvanceEntry();

		FORCEINLINE FEntryKey operator*() const
		{
			return GetKey();
		}

		FEntryKey GetKey() const;
		const UFaerieItem* GetItem() const;

		FStorageIterator_AllEntries& operator++()
		{
			AdvanceEntry();
			return *this;
		}

		FORCEINLINE explicit operator bool() const
		{
			return EntryIndex != INDEX_NONE;
		}

		[[nodiscard]] FORCEINLINE bool operator!=(const FStorageIterator_AllEntries& Rhs) const
		{
			return EntryIndex != Rhs.EntryIndex;
		}

		[[nodiscard]] FORCEINLINE bool operator!=(EIteratorType) const
		{
			// As long we are valid, then we have not ended.
			return static_cast<bool>(*this);
		}

		[[nodiscard]] FORCEINLINE FStorageIterator_AllEntries begin() const { return *this; }
		[[nodiscard]] FORCEINLINE EIteratorType end () const { return End; }

	private:
		// Entry iteration
		const FInventoryContent* Content;
		int32 EntryIndex = -1;
	};

	class FAERIEINVENTORY_API FStorageIterator_AllAddresses : FStorageDataAccess
	{
	public:
		FStorageIterator_AllAddresses(const UFaerieItemStorage* Storage);

		~FStorageIterator_AllAddresses();

		FDefaultIteratorStorage ToInterface() const;

		void AdvanceEntry();

		FORCEINLINE FFaerieAddress operator*() const
		{
			return GetAddress();
		}

		FEntryKey GetKey() const;
		FFaerieAddress GetAddress() const;
		const UFaerieItem* GetItem() const;
		int32 GetStack() const;

		FStorageIterator_AllAddresses& operator++()
		{
			if (NumRemaining)
			{
				NumRemaining--;
				StackPtr++;
			}
			else
			{
				AdvanceEntry();
			}
			return *this;
		}

		FORCEINLINE explicit operator bool() const
		{
			return EntryIndex != INDEX_NONE && StackPtr != nullptr;
		}

		[[nodiscard]] FORCEINLINE bool operator!=(const FStorageIterator_AllAddresses& Rhs) const
		{
			return StackPtr != Rhs.StackPtr;
		}

		[[nodiscard]] FORCEINLINE bool operator!=(EIteratorType) const
		{
			// As long we are valid, then we have not ended.
			return static_cast<bool>(*this);
		}

		[[nodiscard]] FORCEINLINE FStorageIterator_AllAddresses begin() const { return *this; }
		[[nodiscard]] FORCEINLINE EIteratorType end  () const { return End; }

	private:
		// Entry iteration
		const FInventoryContent* Content;
		int32 EntryIndex = -1;

		// Stack iteration
		const FKeyedStack* StackPtr;
		int32 NumRemaining;
	};

	class FAERIEINVENTORY_API FStorageIterator_MaskedEntries : FStorageDataAccess
	{
	public:
		FStorageIterator_MaskedEntries(const UFaerieItemStorage* Storage, const TBitArray<>& EntryMask);
		FStorageIterator_MaskedEntries(const FInventoryContent* Content, const TBitArray<>& EntryMask);
		FStorageIterator_MaskedEntries(const FStorageIterator_MaskedEntries& Other);

		~FStorageIterator_MaskedEntries();

		FDefaultIteratorStorage ToInterface() const;

		void AdvanceEntry();

		FORCEINLINE FFaerieAddress operator*() const
		{
			return GetAddress();
		}

		FEntryKey GetKey() const;
		FFaerieAddress GetAddress() const;
		const UFaerieItem* GetItem() const;

		FStorageIterator_MaskedEntries& operator++()
		{
			if (NumRemaining)
			{
				NumRemaining--;
				StackPtr++;
			}
			else
			{
				++BitIterator;
				AdvanceEntry();
			}
			return *this;
		}

		FORCEINLINE explicit operator bool() const
		{
			return StackPtr != nullptr && KeyMask.IsValidIndex(BitIterator.GetIndex());
		}

		[[nodiscard]] FORCEINLINE bool operator!=(const FStorageIterator_MaskedEntries& Rhs) const
		{
			return StackPtr != Rhs.StackPtr;
		}

		[[nodiscard]] FORCEINLINE bool operator!=(EIteratorType) const
		{
			// As long we are valid, then we have not ended.
			return static_cast<bool>(*this);
		}

		[[nodiscard]] FORCEINLINE FStorageIterator_MaskedEntries begin() const { return FStorageIterator_MaskedEntries(Content, KeyMask); }
		[[nodiscard]] FORCEINLINE EIteratorType end () const { return End; }

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

	class FAERIEINVENTORY_API FStorageIterator_SingleEntry : FStorageDataAccess
	{
	public:
		FStorageIterator_SingleEntry(const UFaerieItemStorage* Storage, const FEntryKey Key);
		FStorageIterator_SingleEntry(const UFaerieItemStorage* Storage, const int32 Index);

		FDefaultIteratorStorage ToInterface() const;

		FFaerieAddress operator*() const
		{
			return GetAddress();
		}

		FEntryKey GetKey() const;
		FFaerieAddress GetAddress() const;

		FORCEINLINE const UFaerieItem* GetItem() const
		{
			return EntryPtr->GetItem();
		}

		FORCEINLINE FStorageIterator_SingleEntry& operator++()
		{
			if (NumRemaining)
			{
				NumRemaining--;
				StackPtr++;
			}
			else
			{
				StackPtr = nullptr;
			}
			return *this;
		}

		FORCEINLINE explicit operator bool() const
		{
			return StackPtr && NumRemaining;
		}

		[[nodiscard]] FORCEINLINE bool operator!=(const FStorageIterator_SingleEntry& Rhs) const
		{
			return StackPtr != Rhs.StackPtr;
		}

		[[nodiscard]] FORCEINLINE bool operator!=(EIteratorType) const
		{
			// As long we are valid, then we have not ended.
			return static_cast<bool>(*this);
		}

		[[nodiscard]] FORCEINLINE FStorageIterator_SingleEntry begin() const { return *this; }
		[[nodiscard]] FORCEINLINE EIteratorType end  () const { return End; }

	private:
		const FInventoryEntry* EntryPtr;
		const FKeyedStack* StackPtr;
		uint32 NumRemaining;
	};

	class FAERIEINVENTORY_API FStorageIterator_MaskedEntries_ForInterface final : public IContainerIterator
	{
	public:
		FStorageIterator_MaskedEntries_ForInterface(FStorageIterator_MaskedEntries&& Inner) : Inner(Inner) {}

		//~ IContainerIterator
		FORCEINLINE virtual FDefaultIteratorStorage Copy() const override { return Inner.ToInterface(); }
		FORCEINLINE virtual void Advance() override { ++Inner; }
		FORCEINLINE virtual FEntryKey ResolveKey() const override { return Inner.GetKey(); }
		FORCEINLINE virtual FFaerieAddress ResolveAddress() const override { return Inner.GetAddress(); }
		FORCEINLINE virtual const UFaerieItem* ResolveItem() const override { return Inner.GetItem(); }
		FORCEINLINE virtual bool IsValid() const override { return static_cast<bool>(Inner); }
		FORCEINLINE virtual bool Equals(const TUniquePtr<IContainerIterator>& Other) const override
		{
			return !(Inner != reinterpret_cast<FStorageIterator_MaskedEntries_ForInterface*>(Other.Get())->Inner);
		}
		//~ IContainerIterator

	private:
		FStorageIterator_MaskedEntries Inner;
	};

	class FAERIEINVENTORY_API FStorageIterator_AllAddresses_ForInterface final : public IContainerIterator
	{
	public:
		FStorageIterator_AllAddresses_ForInterface(const FStorageIterator_AllAddresses& Inner) : Inner(Inner) {}

		//~ IContainerIterator
		FORCEINLINE virtual FDefaultIteratorStorage Copy() const override { return Inner.ToInterface(); }
		FORCEINLINE virtual void Advance() override { ++Inner; }
		FORCEINLINE virtual FEntryKey ResolveKey() const override { return Inner.GetKey(); }
		FORCEINLINE virtual FFaerieAddress ResolveAddress() const override { return Inner.GetAddress(); }
		FORCEINLINE virtual const UFaerieItem* ResolveItem() const override { return Inner.GetItem(); }
		FORCEINLINE virtual bool IsValid() const override { return static_cast<bool>(Inner); }
		FORCEINLINE virtual bool Equals(const TUniquePtr<IContainerIterator>& Other) const override
		{
			return !(Inner != reinterpret_cast<FStorageIterator_AllAddresses_ForInterface*>(Other.Get())->Inner);
		}
		//~ IContainerIterator

	private:
		FStorageIterator_AllAddresses Inner;
	};

	class FAERIEINVENTORY_API FStorageIterator_SingleEntry_ForInterface final : public IContainerIterator
	{
	public:
		FStorageIterator_SingleEntry_ForInterface(const FStorageIterator_SingleEntry& Inner) : Inner(Inner) {}

		//~ IContainerIterator
		FORCEINLINE virtual FDefaultIteratorStorage Copy() const override { return Inner.ToInterface(); }
		FORCEINLINE virtual void Advance() override { ++Inner; }
		FORCEINLINE virtual FEntryKey ResolveKey() const override { return Inner.GetKey(); }
		FORCEINLINE virtual FFaerieAddress ResolveAddress() const override { return Inner.GetAddress(); }
		FORCEINLINE virtual const UFaerieItem* ResolveItem() const override { return Inner.GetItem(); }
		FORCEINLINE virtual bool IsValid() const override { return static_cast<bool>(Inner); }
		FORCEINLINE virtual bool Equals(const TUniquePtr<IContainerIterator>& Other) const override
		{
			return !(Inner != reinterpret_cast<FStorageIterator_SingleEntry_ForInterface*>(Other.Get())->Inner);
		}
		//~ IContainerIterator

	private:
		FStorageIterator_SingleEntry Inner;
	};
}
