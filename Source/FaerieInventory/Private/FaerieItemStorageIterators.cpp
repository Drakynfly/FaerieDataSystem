// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemStorageIterators.h"
#include "FaerieItemStorage.h"

namespace Faerie
{
	// ReSharper disable once CppMemberFunctionMayBeStatic
	const FInventoryContent& FStorageDataAccess::ReadInventoryContent(const UFaerieItemStorage& Storage)
	{
		return Storage.EntryMap;
	}

	FFaerieItemSnapshot FStorageDataAccess::MakeSnapshot(const UFaerieItemStorage& Storage, const int32 Index)
	{
		auto&& Entry = ReadInventoryContent(Storage).GetElementAt(Index);
		FFaerieItemSnapshot Snapshot;
		Snapshot.Owner = &Storage;
		Snapshot.ItemObject = Entry.GetItem();
		Snapshot.Copies = Entry.StackSum();
		return Snapshot;
	}

	FStorageIterator_AllEntries::FStorageIterator_AllEntries(const UFaerieItemStorage* Storage)
	  : Content(&ReadInventoryContent(*Storage))
	{
		Content->LockWriteAccess();
		AdvanceEntry();
	}

	FStorageIterator_AllEntries::~FStorageIterator_AllEntries()
	{
		if (Content)
		{
			Content->UnlockWriteAccess();
		}
	}

	void FStorageIterator_AllEntries::AdvanceEntry()
	{
		EntryIndex++;
		if (EntryIndex >= Content->Num()-1)
		{
			EntryIndex = INDEX_NONE;
		}
	}

	FEntryKey FStorageIterator_AllEntries::GetKey() const
	{
		return Content->GetKeyAt(EntryIndex);
	}

	const UFaerieItem* FStorageIterator_AllEntries::GetItem() const
	{
		return Content->GetElementAt(EntryIndex).GetItem();
	}

	FStorageIterator_AllAddresses::FStorageIterator_AllAddresses(const UFaerieItemStorage* Storage)
	  : Content(&ReadInventoryContent(*Storage))
	{
		Content->LockWriteAccess();
		AdvanceEntry();
	}

	FStorageIterator_AllAddresses::~FStorageIterator_AllAddresses()
	{
		if (Content)
		{
			Content->UnlockWriteAccess();
		}
	}

	FDefaultIteratorStorage FStorageIterator_AllAddresses::ToInterface() const
	{
		return FDefaultIteratorStorage(
			MakeUnique<FStorageIterator_AllAddresses_ForInterface>(FStorageIterator_AllAddresses(*this)));
	}

	void FStorageIterator_AllAddresses::AdvanceEntry()
	{
		EntryIndex++;
		if (EntryIndex >= Content->Num()-1)
		{
			EntryIndex = INDEX_NONE;
			StackPtr = nullptr;
			return;
		}

		const FInventoryEntry& InvEntry = Content->GetElementAt(EntryIndex);
		const TConstArrayView<FKeyedStack> StackView = InvEntry.GetStacks();
		StackPtr = StackView.GetData();
		NumRemaining = StackView.Num();
	}

	FEntryKey FStorageIterator_AllAddresses::GetKey() const
	{
		return Content->GetKeyAt(EntryIndex);
	}

	FFaerieAddress FStorageIterator_AllAddresses::GetAddress() const
	{
		return UFaerieItemStorage::FStorageKey(Content->GetKeyAt(EntryIndex), StackPtr->Key).ToAddress();
	}

	const UFaerieItem* FStorageIterator_AllAddresses::GetItem() const
	{
		return Content->GetElementAt(EntryIndex).GetItem();
	}

	int32 FStorageIterator_AllAddresses::GetStack() const
	{
		return Content->GetElementAt(EntryIndex).GetStack(StackPtr->Key);
	}

	FStorageIterator_MaskedEntries::FStorageIterator_MaskedEntries(const UFaerieItemStorage* Storage,
																   const TBitArray<>& EntryMask)
	  : Content(&ReadInventoryContent(*Storage)),
		KeyMask(EntryMask),
		BitIterator(this->KeyMask)
	{
		Content->LockWriteAccess();
		AdvanceEntry();
	}

	FStorageIterator_MaskedEntries::FStorageIterator_MaskedEntries(const FInventoryContent* Content,
		const TBitArray<>& EntryMask)
	  : Content(Content),
		KeyMask(EntryMask),
		BitIterator(this->KeyMask)
	{
		Content->LockWriteAccess();
		AdvanceEntry();
	}

	FStorageIterator_MaskedEntries::FStorageIterator_MaskedEntries(const FStorageIterator_MaskedEntries& Other)
	  : Content(Other.Content),
		KeyMask(Other.KeyMask),
		BitIterator(this->KeyMask)
	{
		Content->LockWriteAccess();
		AdvanceEntry();
	}

	FStorageIterator_MaskedEntries::~FStorageIterator_MaskedEntries()
	{
		if (Content)
		{
			Content->UnlockWriteAccess();
		}
	}

	FDefaultIteratorStorage FStorageIterator_MaskedEntries::ToInterface() const
	{
		return FDefaultIteratorStorage(
			MakeUnique<FStorageIterator_MaskedEntries_ForInterface>(FStorageIterator_MaskedEntries(Content, KeyMask)));
	}

	void FStorageIterator_MaskedEntries::AdvanceEntry()
	{
		UE_LOG(LogTemp, Verbose, TEXT("BitIterator Index: %i, KeyMask Num: %i, Content Num: %i"), BitIterator.GetIndex(), KeyMask.Num(), Content->Num())

		if (KeyMask.IsValidIndex(BitIterator.GetIndex()))
		{
			UE_LOG(LogTemp, Log, TEXT("Advanced entry"));

			const FInventoryEntry& InvEntry = Content->GetElementAt(BitIterator.GetIndex());
			const TConstArrayView<FKeyedStack> StackView = InvEntry.GetStacks();
			StackPtr = StackView.GetData();
			NumRemaining = StackView.Num()-1;
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("Quitting iteration"));
			StackPtr = nullptr;
		}
	}

	FEntryKey FStorageIterator_MaskedEntries::GetKey() const
	{
		checkSlow(BitIterator);
		return Content->GetKeyAt(BitIterator.GetIndex());
	}

	FFaerieAddress FStorageIterator_MaskedEntries::GetAddress() const
	{
		checkSlow(BitIterator);
		return UFaerieItemStorage::FStorageKey(Content->GetKeyAt(BitIterator.GetIndex()), StackPtr->Key).ToAddress();
	}

	const UFaerieItem* FStorageIterator_MaskedEntries::GetItem() const
	{
		checkSlow(BitIterator);
		check(Content);
		check(StackPtr);
		return Content->GetElementAt(BitIterator.GetIndex()).GetItem();
	}

	/*
	FStorageIterator_MaskedAddresses::FStorageIterator_MaskedAddresses(const UFaerieItemStorage* Storage,
		const TBitArray<>& AddressMask)
	  : Content(&ReadInventoryContent(*Storage)),
		AddressMask(AddressMask),
		//BitIterator(AddressMask)
	{
		Content->LockWriteAccess();
		AdvanceEntry();
	}
	*/

	FStorageIterator_SingleEntry::FStorageIterator_SingleEntry(const UFaerieItemStorage* Storage, const FEntryKey Key)
	{
		EntryPtr = &ReadInventoryContent(*Storage)[Key];
		const TConstArrayView<FKeyedStack> StackView = EntryPtr->GetStacks();
		StackPtr = StackView.GetData();
		NumRemaining = StackView.Num();
		checkSlow(EntryPtr);
		checkSlow(StackPtr);
	}

	FStorageIterator_SingleEntry::FStorageIterator_SingleEntry(const UFaerieItemStorage* Storage, const int32 Index)
	{
		EntryPtr = &ReadInventoryContent(*Storage).GetElementAt(Index);
		const TConstArrayView<FKeyedStack> StackView = EntryPtr->GetStacks();
		StackPtr = StackView.GetData();
		NumRemaining = StackView.Num();
		checkSlow(EntryPtr);
		checkSlow(StackPtr);
	}

	FDefaultIteratorStorage FStorageIterator_SingleEntry::ToInterface() const
	{
		return FDefaultIteratorStorage(
			MakeUnique<FStorageIterator_SingleEntry_ForInterface>(FStorageIterator_SingleEntry(*this)));
	}

	FEntryKey FStorageIterator_SingleEntry::GetKey() const
	{
		return EntryPtr->Key;
	}

	FFaerieAddress FStorageIterator_SingleEntry::GetAddress() const
	{
		return UFaerieItemStorage::FStorageKey(EntryPtr->Key, StackPtr->Key).ToAddress();
	}
}
