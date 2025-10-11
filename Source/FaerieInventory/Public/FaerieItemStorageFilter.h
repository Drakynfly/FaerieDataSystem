// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieContainerFilter.h"
#include "FaerieContainerIterator.h"
#include "FaerieItemProxy.h"
#include "FaerieItemStorageIterators.h"

class UFaerieItemStorage;

namespace Faerie::Storage
{
	using FKeyMask = Container::TIterator<FEntryKey, FIterator_MaskedEntries>;
	//using FAddressMask = TIterator<FFaerieAddress, FIterator_MaskedAddresses>; // @todo
	using FItemMask = Container::TIterator<UFaerieItem*, FIterator_MaskedEntries>;
	using FConstItemMask = Container::TIterator<const UFaerieItem*, FIterator_MaskedEntries>;

	class FAERIEINVENTORY_API FEntryFilter : FStorageDataAccess
	{
	public:
		FEntryFilter(const UFaerieItemStorage* Storage);

		const UFaerieItemStorage* GetContainer() const { return Storage; }

		FEntryFilter& Run(Container::IItemDataFilter&& Filter);
		FEntryFilter& Run(Container::IEntryKeyFilter&& Filter);
		FEntryFilter& Run(Container::ISnapshotFilter&& Filter);

		template <Container::CFilterType T>
		void RunStatic()
		{
			for (TConstSetBitIterator<> It(KeyBits); It; ++It)
			{
				if constexpr (TIsDerivedFrom<T, Container::IEntryKeyFilter>::Value)
				{
					// Disable keys that fail the filter.
                    if (const FEntryKey Key = ReadInventoryContent(*Storage).GetKeyAt(It.GetIndex());
                    	!T::StaticPasses(Key))
                    {
                    	KeyBits.AccessCorrespondingBit(It) = false;
                    }
				}
				else if constexpr (TIsDerivedFrom<T, Container::IItemDataFilter>::Value)
				{
					// Disable keys that fail the filter.
					if (const UFaerieItem* Item = ReadInventoryContent(*Storage).GetElementAt(It.GetIndex()).GetItem();
						!T::StaticPasses(Item))
					{
						KeyBits.AccessCorrespondingBit(It) = false;
					}
				}
				else if constexpr (TIsDerivedFrom<T, Container::ISnapshotFilter>::Value)
				{
					// Disable keys that fail the filter.
					if (const FFaerieItemSnapshot Snapshot = MakeSnapshot(*Storage, It.GetIndex());
						!T::StaticPasses(Snapshot))
					{
						KeyBits.AccessCorrespondingBit(It) = false;
					}
				}
				else if constexpr (TIsDerivedFrom<T, Container::ICustomFilter>::Value)
				{
					T::StaticPasses(*this);
				}
				else
				{
					unimplemented()
				}
			}
		}

		// Invert the filter to reverse keys from disabled to enabled, and vice versa.
		FEntryFilter& Invert()
		{
			KeyBits.BitwiseNOT();
			return *this;
		}

		void Reset();

		bool IsEmpty() const { return KeyBits.CountSetBits() == 0; }
		int32 Num() const { return KeyBits.CountSetBits(); }

		template <typename ResolveType>
		Container::TIterator<ResolveType, FIterator_MaskedEntries> Range() const
		{
			if (KeyBits.CountSetBits() == 0)
			{
				// No bits set, return invalid iterator.
				return Container::TIterator<ResolveType, FIterator_MaskedEntries>(FIterator_MaskedEntries(Storage, {}));
			}
			return Container::TIterator<ResolveType, FIterator_MaskedEntries>(FIterator_MaskedEntries(Storage, KeyBits));
		}

		[[nodiscard]] FORCEINLINE FKeyMask begin() const { return Range<FEntryKey>(); }
		[[nodiscard]] FORCEINLINE EIteratorType end () const { return End; }

	protected:
		const UFaerieItemStorage* Storage;

		// A bit for each EntryKey
		TBitArray<> KeyBits;
	};

	// @todo this is a complex filter to implement, come back to this
	/*
	class FAERIEINVENTORY_API FAddressFilter : FStorageDataAccess
	{
	public:
		FAddressFilter(const UFaerieItemStorage* Storage);

		FAddressFilter& Run(Container::IItemDataFilter& Filter);
		FAddressFilter& Run(Container::IEntryKeyFilter& Filter);
		FAddressFilter& Run(Container::IAddressFilter&& Filter);
		FAddressFilter& Run(Container::ISnapshotFilter&& Filter);

		// Invert the filter to reverse keys from disabled to enabled, and vice versa.
		FAddressFilter& Invert()
		{
			AddressBits.BitwiseNOT();
			return *this;
		}

		void Reset();

		bool IsEmpty() const { return AddressBits.CountSetBits() == 0; }
		int32 Num() const { return AddressBits.CountSetBits(); }

		FAddressMask AddressRange() const;
		FItemMask ItemRange() const;
		FConstItemMask ConstItemRange() const;

		// Enables ranged for-loops through each address for a key in the container.
		using FSingleAddressIterator = TIterator<FFaerieAddress, FIterator_SingleEntry>;
		FSingleAddressIterator AddressRange(FEntryKey Key) const;

		// Emits all keys for which the filter contains any address.
		TArray<FEntryKey> EmitKeys() const;
		TArray<FFaerieAddress> EmitAddresses() const;
		TArray<FFaerieAddress> EmitAddresses(FEntryKey Key) const;
		TArray<const UFaerieItem*> EmitItems() const;

		[[nodiscard]] FORCEINLINE FAddressMask begin() const { return AddressRange(); }
		[[nodiscard]] FORCEINLINE FAddressMask end  () const { return End; }

	protected:
		const UFaerieItemStorage* Storage;

		// A bit for each stack/address
		TBitArray<> AddressBits;
		TArray<int32> EntryKeys;
	};
	*/

	using FItemFilter_Key = Container::TFilter<Container::EFilterFlags::KeyFilter, FEntryFilter>;
	//using FItemFilter_Address = Container::TFilter<Container::EFilterFlags::AddressFilter, FAddressFilter>;
}
