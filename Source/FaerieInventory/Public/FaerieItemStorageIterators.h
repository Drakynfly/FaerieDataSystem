// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieContainerIterator.h"

struct FInventoryContent;
struct FInventoryEntry;
struct FKeyedStack;
class UFaerieItemStorage;

namespace Faerie::Storage
{
	class FStorageDataAccess
	{
	protected:
		static const FInventoryContent& ReadInventoryContent(const TNotNull<const UFaerieItemStorage*> Storage);
	};

	class FAERIEINVENTORY_API FIterator_AllEntries : FStorageDataAccess
	{
	public:
		FIterator_AllEntries(TNotNull<const UFaerieItemStorage*> Storage);
		~FIterator_AllEntries();

		TUniquePtr<Container::IIterator> ToInterface() const;

		void AdvanceEntry();

		UE_REWRITE FEntryKey operator*() const
		{
			return GetKey();
		}

		FEntryKey GetKey() const;
		const UFaerieItem* GetItem() const;
		FFaerieItemStackView GetView() const;

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
		[[nodiscard]] UE_REWRITE EIteratorType end() const { return End; }

	private:
		// Entry iteration
		TNotNull<const FInventoryContent*> Content;
		int32 EntryIndex = INDEX_NONE;
	};

	class FAERIEINVENTORY_API FIterator_AllAddresses : FStorageDataAccess
	{
	public:
		FIterator_AllAddresses(TNotNull<const UFaerieItemStorage*> Storage);

		~FIterator_AllAddresses();

		void AdvanceEntry();

		UE_REWRITE FFaerieAddress operator*() const
		{
			return GetAddress();
		}

		FEntryKey GetKey() const;
		FFaerieAddress GetAddress() const;
		const UFaerieItem* GetItem() const;
		FFaerieItemStackView GetView() const;

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
		TNotNull<const FInventoryContent*> Content;
		int32 EntryIndex = INDEX_NONE;

		// Stack iteration
		const FKeyedStack* StackPtr;
		int32 NumRemaining;
	};

	class FAERIEINVENTORY_API FIterator_SingleEntry : FStorageDataAccess
	{
	public:
		FIterator_SingleEntry(const FInventoryEntry& Entry);
		FIterator_SingleEntry(TNotNull<const UFaerieItemStorage*> Storage, const FEntryKey Key);
		FIterator_SingleEntry(TNotNull<const UFaerieItemStorage*> Storage, const int32 Index);

		FFaerieAddress operator*() const
		{
			return GetAddress();
		}

		FEntryKey GetKey() const;
		FFaerieAddress GetAddress() const;
		const UFaerieItem* GetItem() const;
		FFaerieItemStackView GetView() const;

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
		[[nodiscard]] UE_REWRITE EIteratorType end() const { return End; }

	private:
		TNotNull<const FInventoryEntry*> EntryPtr;
		const FKeyedStack* StackPtr;
		uint32 NumRemaining;
	};

	class FAERIEINVENTORY_API FIterator_AllEntries_ForInterface final : public Container::IIterator
	{
	public:
		FIterator_AllEntries_ForInterface(const TNotNull<const UFaerieItemStorage*> Storage)
		  : Storage(Storage), Inner(Storage) {}

		FIterator_AllEntries_ForInterface(const TNotNull<const UFaerieItemStorage*> Storage, const FIterator_AllEntries& Other)
		  : Storage(Storage), Inner(Other) {}

		//~ Container::IIterator
		virtual TUniquePtr<IIterator> Copy() const override;
		UE_REWRITE virtual void Advance() override { ++Inner; }
		UE_REWRITE virtual FEntryKey ResolveKey() const override { return Inner.GetKey(); }
		UE_REWRITE virtual FFaerieAddress ResolveAddress() const override { checkNoEntry(); return {}; }
		UE_REWRITE virtual const UFaerieItem* ResolveItem() const override { return Inner.GetItem(); }
		UE_REWRITE virtual FFaerieItemStackView ResolveView() const override { return Inner.GetView(); }
		virtual const IFaerieItemOwnerInterface* ResolveOwner() const override;
		UE_REWRITE virtual bool IsValid() const override { return static_cast<bool>(Inner); }
		//~ Container::IIterator

	private:
		const TNotNull<const UFaerieItemStorage*> Storage;
		FIterator_AllEntries Inner;
	};

	class FAERIEINVENTORY_API FIterator_AllAddresses_ForInterface final : public Container::IIterator
	{
	public:
		FIterator_AllAddresses_ForInterface(const TNotNull<const UFaerieItemStorage*> Storage)
		  : Storage(Storage), Inner(Storage) {}

		FIterator_AllAddresses_ForInterface(const TNotNull<const UFaerieItemStorage*> Storage, const FIterator_AllAddresses& Other)
		  : Storage(Storage), Inner(Other) {}

		//~ Container::IIterator
		virtual TUniquePtr<IIterator> Copy() const override;
		UE_REWRITE virtual void Advance() override { ++Inner; }
		UE_REWRITE virtual FEntryKey ResolveKey() const override { return Inner.GetKey(); }
		UE_REWRITE virtual FFaerieAddress ResolveAddress() const override { return Inner.GetAddress(); }
		UE_REWRITE virtual const UFaerieItem* ResolveItem() const override { return Inner.GetItem(); }
		UE_REWRITE virtual FFaerieItemStackView ResolveView() const override { return Inner.GetView(); }
		virtual const IFaerieItemOwnerInterface* ResolveOwner() const override;
		UE_REWRITE virtual bool IsValid() const override { return static_cast<bool>(Inner); }
		//~ Container::IIterator

	private:
		const TNotNull<const UFaerieItemStorage*> Storage;
		FIterator_AllAddresses Inner;
	};

	class FAERIEINVENTORY_API FIterator_SingleEntry_ForInterface final : public Container::IIterator
	{
	public:
		FIterator_SingleEntry_ForInterface(const TNotNull<const UFaerieItemStorage*> Storage, const FInventoryEntry& Entry)
		  : Storage(Storage), Inner(Entry) {}

		FIterator_SingleEntry_ForInterface(const TNotNull<const UFaerieItemStorage*> Storage, const FIterator_SingleEntry& Other)
		  : Storage(Storage), Inner(Other) {}

		//~ Container::IIterator
		virtual TUniquePtr<IIterator> Copy() const override;
		UE_REWRITE virtual void Advance() override { ++Inner; }
		UE_REWRITE virtual FEntryKey ResolveKey() const override { return Inner.GetKey(); }
		UE_REWRITE virtual FFaerieAddress ResolveAddress() const override { return Inner.GetAddress(); }
		UE_REWRITE virtual const UFaerieItem* ResolveItem() const override { return Inner.GetItem(); }
		UE_REWRITE virtual FFaerieItemStackView ResolveView() const override { return Inner.GetView(); }
		virtual const IFaerieItemOwnerInterface* ResolveOwner() const override;
		UE_REWRITE virtual bool IsValid() const override { return static_cast<bool>(Inner); }
		//~ Container::IIterator

	private:
		const TNotNull<const UFaerieItemStorage*> Storage;
		FIterator_SingleEntry Inner;
	};
}
