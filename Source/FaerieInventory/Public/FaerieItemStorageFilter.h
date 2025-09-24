// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieContainerFilter.h"
#include "FaerieContainerIterator.h"
#include "FaerieItemProxy.h"
#include "FaerieItemStorageIterators.h"

class UFaerieItemStorage;

namespace Faerie
{
	struct IItemDataFilter;

	using FStorageKeyMask = TContainerIterator<FEntryKey, FStorageIterator_MaskedEntries>;
	//using FStorageAddressMask = TContainerIterator<FFaerieAddress, FStorageIterator_MaskedAddresses>; // @todo
	using FStorageItemMask = TContainerIterator<UFaerieItem*, FStorageIterator_MaskedEntries>;
	using FStorageConstItemMask = TContainerIterator<const UFaerieItem*, FStorageIterator_MaskedEntries>;

	enum ESortDirection
	{
		Forward,
		Backward,
	};

	class FAERIEINVENTORY_API FItemStorageEntryFilter : FStorageDataAccess
	{
	public:
		FItemStorageEntryFilter(const UFaerieItemStorage* Storage);

		FItemStorageEntryFilter& Run(IItemDataFilter&& Filter);
		FItemStorageEntryFilter& Run(IEntryKeyFilter&& Filter);
		FItemStorageEntryFilter& Run(ISnapshotFilter&& Filter);

		template <CFilterType T>
		void RunStatic()
		{
			for (TConstSetBitIterator<> It(KeyBits); It; ++It)
			{
				if constexpr (TIsDerivedFrom<T, IEntryKeyFilter>::Value)
				{
					// Disable keys that fail the predicate.
                    if (const FEntryKey Key = ReadInventoryContent(*Storage).GetKeyAt(It.GetIndex());
                    	!T::StaticPasses(Key))
                    {
                    	KeyBits.AccessCorrespondingBit(It) = false;
                    }
				}
				else if constexpr (TIsDerivedFrom<T, IItemDataFilter>::Value)
				{
					// Disable keys that the predicate.
					if (const UFaerieItem* Item = ReadInventoryContent(*Storage).GetElementAt(It.GetIndex()).GetItem();
						!T::StaticPasses(Item))
					{
						KeyBits.AccessCorrespondingBit(It) = false;
					}
				}
				else if constexpr (TIsDerivedFrom<T, ISnapshotFilter>::Value)
				{
					// Disable keys that the predicate.
					if (const FFaerieItemSnapshot Snapshot = MakeSnapshot(*Storage, It.GetIndex());
						!T::StaticPasses(Snapshot))
					{
						KeyBits.AccessCorrespondingBit(It) = false;
					}
				}
			}
		}

		// Invert the filter to reverse keys from disabled to enabled, and vice versa.
		FItemStorageEntryFilter& Invert()
		{
			KeyBits.BitwiseNOT();
			return *this;
		}

		void Reset();

		bool IsEmpty() const { return KeyBits.CountSetBits() == 0; }
		int32 Num() const { return KeyBits.CountSetBits(); }

		template <typename ResolveType>
		TContainerIterator<ResolveType, FStorageIterator_MaskedEntries> Range() const
		{
			if (KeyBits.CountSetBits() == 0)
			{
				// No bits set, return invalid iterator.
				return TContainerIterator<ResolveType, FStorageIterator_MaskedEntries>(FStorageIterator_MaskedEntries(Storage, {}));
			}
			return TContainerIterator<ResolveType, FStorageIterator_MaskedEntries>(FStorageIterator_MaskedEntries(Storage, KeyBits));
		}

		template <ESortDirection Direction = Forward>
		void SortBySnapshot(const FItemComparator& Sort)
		{
			unimplemented();
		}

		[[nodiscard]] FORCEINLINE FStorageKeyMask begin() const { return Range<FEntryKey>(); }
		[[nodiscard]] FORCEINLINE EIteratorType end () const { return End; }

	protected:
		const UFaerieItemStorage* Storage;

		// A bit for each EntryKey
		TBitArray<> KeyBits;
	};

	// @todo this is a complex filter to implement, come back to this
	/*
	class FAERIEINVENTORY_API FItemStorageAddressFilter : FStorageDataAccess
	{
	public:
		FItemStorageAddressFilter(const UFaerieItemStorage* Storage);

		FItemStorageAddressFilter& Run(IItemDataFilter& Filter);
		FItemStorageAddressFilter& Run(IEntryKeyFilter& Filter);

		// Invert the filter to reverse keys from disabled to enabled, and vice versa.
		FItemStorageAddressFilter& Invert()
		{
			AddressBits.BitwiseNOT();
			return *this;
		}

		void Reset();

		bool IsEmpty() const { return AddressBits.CountSetBits() == 0; }
		int32 Num() const { return AddressBits.CountSetBits(); }

		FStorageAddressMask AddressRange() const;
		FStorageItemMask ItemRange() const;
		FStorageConstItemMask ConstItemRange() const;

		// Enables ranged for-loops through each address for a key in the container.
		using FStorageSingleAddressIterator = TContainerIterator<FFaerieAddress, FStorageIterator_SingleEntry>;
		FStorageSingleAddressIterator AddressRange(FEntryKey Key) const;

		// Emits all keys for which the filter contains any address.
		TArray<FEntryKey> EmitKeys() const;
		TArray<FFaerieAddress> EmitAddresses() const;
		TArray<FFaerieAddress> EmitAddresses(FEntryKey Key) const;
		TArray<const UFaerieItem*> EmitItems() const;

		template <ESortDirection Direction = Forward>
		void SortBySnapshot(const FItemComparator& Sort)
		{
			unimplemented();
		}

		[[nodiscard]] FORCEINLINE FStorageAddressMask begin() const { return AddressRange(); }
		[[nodiscard]] FORCEINLINE FStorageAddressMask end  () const { return End; }

	protected:
		const UFaerieItemStorage* Storage;

		// A bit for each stack/address
		TBitArray<> AddressBits;
		TArray<int32> EntryKeys;
	};
	*/

	using FItemStorageFilter_Key = TContainerFilter<EFilterFlags::KeyFilter, FItemStorageEntryFilter>;
	//using FItemStorageFilter_Address = TContainerFilter<EFilterFlags::AddressFilter, FItemStorageAddressFilter>;
}
