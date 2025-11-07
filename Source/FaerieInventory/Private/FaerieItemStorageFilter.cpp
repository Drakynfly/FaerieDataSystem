// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemStorageFilter.h"
#include "FaerieContainerFilter.h"
#include "FaerieItemContainerBase.h"
#include "FaerieItemStorage.h"

namespace Faerie::Storage
{
	template <typename Pred>
	void FilterEntriesByEntry(TBitArray<>& KeyBits, const FInventoryContent& Content, Pred&& Func)
	{
		for (TConstSetBitIterator<> It(KeyBits); It; ++It)
		{
			// Disable keys that fail the predicate.
			if (const FEntryKey Key = Content.GetKeyAt(It.GetIndex());
				!Func(Key))
			{
				KeyBits.AccessCorrespondingBit(It) = false;
			}
		}
	}

	template <typename Pred>
	void FilterEntriesByItem(TBitArray<>& KeyBits, const FInventoryContent& Content, Pred&& Func)
	{
		for (TConstSetBitIterator<> It(KeyBits); It; ++It)
		{
			// Disable keys that fail the predicate.
			if (const UFaerieItem* Item = Content.GetElementAt(It.GetIndex()).GetItem();
				!Func(Item))
			{
				KeyBits.AccessCorrespondingBit(It) = false;
			}
		}
	}

	FEntryFilter::FEntryFilter(const UFaerieItemStorage* Storage)
	  : Storage(Storage)
	{
		KeyBits.Init(true, Storage->GetEntryCount());
	}

	FEntryFilter& FEntryFilter::Run(Container::IItemDataFilter&& Filter)
	{
		FilterEntriesByItem(KeyBits, ReadInventoryContent(*Storage),
			[&Filter](const UFaerieItem* Item)
			{
				return Filter.Passes(Item);
			});
		return *this;
	}

	FEntryFilter& FEntryFilter::Run(Container::IEntryKeyFilter&& Filter)
	{
		FilterEntriesByEntry(KeyBits, ReadInventoryContent(*Storage),
			[&Filter](const FEntryKey Key)
			{
				return Filter.Passes(Key);
			});
		return *this;
	}

	FEntryFilter& FEntryFilter::Run(Container::ISnapshotFilter&& Filter)
	{
		FilterEntriesByEntry(KeyBits, ReadInventoryContent(*Storage), [this, &Filter](const FEntryKey Key)
			{
				const FFaerieItemStackView View = Storage->View(Key);
				FFaerieItemSnapshot Snapshot;
				Snapshot.Owner = Storage;
				Snapshot.ItemObject = View.Item.Get();
				Snapshot.Copies = View.Copies;
				return Filter.Passes(Snapshot);
			});
		return *this;
	}

	void FEntryFilter::Reset()
	{
		KeyBits.Init(true, Storage->GetEntryCount());
	}

	/*
	FFaerieAddress IndexToAddress(const int32 Index, const TConstArrayView<int32> Converter)
	{
		const int32 ConverterIndex = Algo::LowerBound(Converter, Index);
		const int32 EntryValue = Converter[ConverterIndex];
		return UFaerieItemStorage::MakeAddress(FEntryKey(EntryValue), FStackKey(Index - EntryValue));
	}

	int32 AddressToIndex(const FEntryKey Entry, const FStackKey Stack, const TConstArrayView<int32> Converter)
	{
		return Converter[Entry.Value()] + Stack.Value();
	}

	template <typename Pred>
	void FilterAddressesByAddress(TBitArray<>& AddressBits, Pred&& Func, const TConstArrayView<int32> Converter)
	{
		for (TConstSetBitIterator<> It(AddressBits); It; ++It)
		{
			const FFaerieAddress Address = IndexToAddress(It.GetIndex(), Converter);

			// Disable keys that are invalid, or fail the filter.
			if (!Func(Address))
			{
				AddressBits.AccessCorrespondingBit(It) = false;
			}
		}
	}

	FAddressFilter::FAddressFilter(const UFaerieItemStorage* Storage)
	  : Storage(Storage)
	{
		const int32 StackCount = Storage->GetStackCount();
		AddressBits.Init(true, StackCount);

		// Create shortcut to jump from index back to address and key.
		EntryKeys.Reserve(StackCount);
		for (const FEntryKey EntryKey : FStorageIterator_AllEntries(Storage))
		{
			EntryKeys.Add(EntryKey.Value());
		}
	}

	FAddressFilter& FAddressFilter::Run(IItemDataFilter& Filter)
	{
		// @todo ItemDataFilters only need to iterator once per item, not per each stack
		FilterAddressesByAddress(AddressBits,
			[this, &Filter](const FFaerieAddress Address)
			{
				return Filter.Passes(Storage->ViewItem(Address));
			}, EntryKeys);
		return *this;
	}

	FAddressFilter& FAddressFilter::Run(IEntryKeyFilter& Filter)
	{
		const TArray<FStackKey> Stacks = Storage->BreakEntryIntoKeys(Key);

		AddressBits.Init(false, AddressBits.Num());
		for (const FStackKey Stack : Stacks)
		{
			const int32 Index = AddressToIndex(Key, Stack, EntryKeys);
			AddressBits[Index] = true;
		}

		return *this;
	}

	void FAddressFilter::Reset()
	{
		AddressBits.Init(true, Storage->GetStackCount());
	}

	FStorageAddressMask FAddressFilter::AddressRange() const
	{
		if (AddressBits.CountSetBits() == 0)
		{
			// No bits set, return invalid iterator.
			return FStorageAddressMask(End);
		}
		return FStorageAddressMask(FStorageIterator_MaskedAddresses(Storage, AddressBits));
	}

	FStorageItemMask FAddressFilter::ItemRange() const
	{
		if (AddressBits.CountSetBits() == 0)
		{
			// No bits set, return invalid iterator.
			return FStorageItemMask(End);
		}
		return FStorageItemMask(FStorageIterator_MaskedAddresses(Storage, AddressBits));
	}

	FStorageConstItemMask FAddressFilter::ConstItemRange() const
	{
		if (AddressBits.CountSetBits() == 0)
		{
			// No bits set, return invalid iterator.
			return FStorageConstItemMask(End);
		}
		return FStorageConstItemMask(FStorageIterator_MaskedAddresses(Storage, AddressBits));
	}

	FAddressFilter::FStorageSingleAddressIterator FAddressFilter::AddressRange(const FEntryKey Key) const
	{
		const int32 Index = Storage->FindIndexOfKey(Key);
		if (!AddressBits[Index])
		{
			// This bit has been filtered out, do create an iterator for it.
			return FStorageSingleAddressIterator(End);
		}

		return FStorageSingleAddressIterator(FStorageIterator_SingleEntry(Storage, Index));
	}

	TArray<FEntryKey> FAddressFilter::EmitKeys() const
	{

	}

	TArray<FFaerieAddress> FAddressFilter::EmitAddresses() const
	{
		TArray<FFaerieAddress> Out;
		Out.Reserve(Num());
		for (const FFaerieAddress Address : AddressRange())
		{
			Out.Add(Address);
		}
		return Out;
	}

	TArray<FFaerieAddress> FAddressFilter::EmitAddresses(const FEntryKey Key) const
	{
		TArray<FFaerieAddress> Out;
		for (const FFaerieAddress Address : AddressRange(Key))
		{
			Out.Add(Address);
		}
		return Out;
	}

	TArray<const UFaerieItem*> FAddressFilter::EmitItems() const
	{
		TArray<const UFaerieItem*> Out;
		for (const UFaerieItem* Item : ItemRange())
		{
			Out.Add(Item);
		}
		return Out;
	}
	*/
}
