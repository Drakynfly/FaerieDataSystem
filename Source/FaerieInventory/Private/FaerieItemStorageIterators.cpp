// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemStorageIterators.h"
#include "FaerieInventoryLog.h"
#include "FaerieItemStorage.h"

namespace Faerie::Storage
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

	FIterator_AllEntries::FIterator_AllEntries(const UFaerieItemStorage* Storage)
	  : Content(&ReadInventoryContent(*Storage))
	{
		Content->LockWriteAccess();
		AdvanceEntry();
	}

	FIterator_AllEntries::~FIterator_AllEntries()
	{
		if (Content)
		{
			Content->UnlockWriteAccess();
		}
	}

	Container::FVirtualIterator FIterator_AllEntries::ToInterface() const
	{
		return Container::FVirtualIterator(MakeUnique<FIterator_AllEntries_ForInterface>(*this));
	}

	void FIterator_AllEntries::AdvanceEntry()
	{
		EntryIndex++;
		if (EntryIndex >= Content->Num())
		{
			EntryIndex = INDEX_NONE;
		}
	}

	FEntryKey FIterator_AllEntries::GetKey() const
	{
		return Content->GetKeyAt(EntryIndex);
	}

	const UFaerieItem* FIterator_AllEntries::GetItem() const
	{
		return Content->GetElementAt(EntryIndex).GetItem();
	}

	FIterator_AllAddresses::FIterator_AllAddresses(const UFaerieItemStorage* Storage)
	  : Content(&ReadInventoryContent(*Storage))
	{
		Content->LockWriteAccess();
		AdvanceEntry();
	}

	FIterator_AllAddresses::~FIterator_AllAddresses()
	{
		if (Content)
		{
			Content->UnlockWriteAccess();
		}
	}

	Container::FVirtualIterator FIterator_AllAddresses::ToInterface() const
	{
		return Container::FVirtualIterator(MakeUnique<FIterator_AllAddresses_ForInterface>(*this));
	}

	void FIterator_AllAddresses::AdvanceEntry()
	{
		EntryIndex++;
		if (EntryIndex >= Content->Num())
		{
			EntryIndex = INDEX_NONE;
			StackPtr = nullptr;
			return;
		}

		const FInventoryEntry& InvEntry = Content->GetElementAt(EntryIndex);
		const TConstArrayView<FKeyedStack> StackView = InvEntry.GetStacks();
		StackPtr = StackView.GetData();
		NumRemaining = StackView.Num()-1;
	}

	FEntryKey FIterator_AllAddresses::GetKey() const
	{
		return Content->GetKeyAt(EntryIndex);
	}

	FFaerieAddress FIterator_AllAddresses::GetAddress() const
	{
		return UFaerieItemStorage::MakeAddress(Content->GetKeyAt(EntryIndex), StackPtr->Key);
	}

	const UFaerieItem* FIterator_AllAddresses::GetItem() const
	{
		return Content->GetElementAt(EntryIndex).GetItem();
	}

	int32 FIterator_AllAddresses::GetStack() const
	{
		return Content->GetElementAt(EntryIndex).GetStack(StackPtr->Key);
	}

	FIterator_MaskedEntries::FIterator_MaskedEntries(const UFaerieItemStorage* Storage, const TBitArray<>& EntryMask)
	  : Content(&ReadInventoryContent(*Storage)),
		KeyMask(EntryMask),
		BitIterator(this->KeyMask)
	{
		Content->LockWriteAccess();
		AdvanceEntry();
	}

	FIterator_MaskedEntries::FIterator_MaskedEntries(const FInventoryContent* Content,
		const TBitArray<>& EntryMask)
	  : Content(Content),
		KeyMask(EntryMask),
		BitIterator(this->KeyMask)
	{
		Content->LockWriteAccess();
		AdvanceEntry();
	}

	FIterator_MaskedEntries::FIterator_MaskedEntries(const FIterator_MaskedEntries& Other)
	  : Content(Other.Content),
		KeyMask(Other.KeyMask),
		BitIterator(this->KeyMask)
	{
		Content->LockWriteAccess();
		AdvanceEntry();
	}

	FIterator_MaskedEntries::~FIterator_MaskedEntries()
	{
		if (Content)
		{
			Content->UnlockWriteAccess();
		}
	}

	Container::FVirtualIterator FIterator_MaskedEntries::ToInterface() const
	{
		return Container::FVirtualIterator(
			MakeUnique<FIterator_MaskedEntries_ForInterface>(FIterator_MaskedEntries(Content, KeyMask)));
	}

	void FIterator_MaskedEntries::AdvanceEntry()
	{
		UE_LOG(LogFaerieInventory, Verbose, TEXT("BitIterator Index: %i, KeyMask Num: %i, Content Num: %i"), BitIterator.GetIndex(), KeyMask.Num(), Content->Num())

		if (KeyMask.IsValidIndex(BitIterator.GetIndex()))
		{
			const FInventoryEntry& InvEntry = Content->GetElementAt(BitIterator.GetIndex());
			const TConstArrayView<FKeyedStack> StackView = InvEntry.GetStacks();
			StackPtr = StackView.GetData();
			NumRemaining = StackView.Num()-1;
		}
		else
		{
			StackPtr = nullptr;
		}
	}

	FEntryKey FIterator_MaskedEntries::GetKey() const
	{
		checkSlow(BitIterator);
		return Content->GetKeyAt(BitIterator.GetIndex());
	}

	FFaerieAddress FIterator_MaskedEntries::GetAddress() const
	{
		checkSlow(BitIterator);
		return UFaerieItemStorage::MakeAddress(Content->GetKeyAt(BitIterator.GetIndex()), StackPtr->Key);
	}

	const UFaerieItem* FIterator_MaskedEntries::GetItem() const
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

	FIterator_SingleEntry::FIterator_SingleEntry(const FInventoryEntry& Entry)
	  : EntryPtr(&Entry)
	{
		const TConstArrayView<FKeyedStack> StackView = EntryPtr->GetStacks();
		StackPtr = StackView.GetData();
		NumRemaining = StackView.Num()-1;
		checkSlow(EntryPtr);
		checkSlow(StackPtr);
	}

	FIterator_SingleEntry::FIterator_SingleEntry(const UFaerieItemStorage* Storage, const FEntryKey Key)
	{
		EntryPtr = &ReadInventoryContent(*Storage)[Key];
		const TConstArrayView<FKeyedStack> StackView = EntryPtr->GetStacks();
		StackPtr = StackView.GetData();
		NumRemaining = StackView.Num()-1;
		checkSlow(EntryPtr);
		checkSlow(StackPtr);
	}

	FIterator_SingleEntry::FIterator_SingleEntry(const UFaerieItemStorage* Storage, const int32 Index)
	{
		EntryPtr = &ReadInventoryContent(*Storage).GetElementAt(Index);
		const TConstArrayView<FKeyedStack> StackView = EntryPtr->GetStacks();
		StackPtr = StackView.GetData();
		NumRemaining = StackView.Num()-1;
		checkSlow(EntryPtr);
		checkSlow(StackPtr);
	}

	Container::FVirtualIterator FIterator_SingleEntry::ToInterface() const
	{
		return Container::FVirtualIterator(MakeUnique<FIterator_SingleEntry_ForInterface>(*this));
	}

	FEntryKey FIterator_SingleEntry::GetKey() const
	{
		return EntryPtr->Key;
	}

	FFaerieAddress FIterator_SingleEntry::GetAddress() const
	{
		return UFaerieItemStorage::MakeAddress(EntryPtr->Key, StackPtr->Key);
	}
}
