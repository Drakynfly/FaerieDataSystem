// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemStorageIterators.h"
#include "FaerieItemStorage.h"

namespace Faerie::Storage
{
	// ReSharper disable once CppMemberFunctionMayBeStatic
	const FFaerieStorageContent& FStorageDataAccess::ReadInventoryContent(const TNotNull<const UFaerieItemStorage*> Storage)
	{
		return Storage->EntryMap;
	}

	FIterator_AllEntries::FIterator_AllEntries(const TNotNull<const UFaerieItemStorage*> Storage)
	  : Content(ReadInventoryContent(Storage))
	{
#if FAERIE_DEBUG
		Content.LockWriteAccess();
#endif
		AdvanceEntry();
	}

	FIterator_AllEntries::~FIterator_AllEntries()
	{
#if FAERIE_DEBUG
		Content.UnlockWriteAccess();
#endif
	}

	void FIterator_AllEntries::AdvanceEntry()
	{
		EntryIndex++;
		if (EntryIndex >= Content.Num())
		{
			EntryIndex = INDEX_NONE;
		}
	}

	FFaerieEntryKey FIterator_AllEntries::GetKey() const
	{
		return Content.GetKeyAt(EntryIndex);
	}

	FFaerieItemInstance FIterator_AllEntries::GetInstance() const
	{
		return Content.GetElementAt(EntryIndex).GetInstance();
	}

	int32 FIterator_AllEntries::GetCopies() const
	{
		return Content.GetElementAt(EntryIndex).StackSum();
	}

	FIterator_AllAddresses::FIterator_AllAddresses(const TNotNull<const UFaerieItemStorage*> Storage)
	  : Content(ReadInventoryContent(Storage))
	{
#if FAERIE_DEBUG
		Content.LockWriteAccess();
#endif
		AdvanceEntry();
	}

	FIterator_AllAddresses::~FIterator_AllAddresses()
	{
#if FAERIE_DEBUG
		Content.UnlockWriteAccess();
#endif
	}

	void FIterator_AllAddresses::AdvanceEntry()
	{
		EntryIndex++;
		if (EntryIndex >= Content.Num())
		{
			EntryIndex = INDEX_NONE;
			StackPtr = nullptr;
			return;
		}

		const FFaerieStorageEntry& InvEntry = Content.GetElementAt(EntryIndex);
		const TConstArrayView<FFaerieKeyedStack> StackView = InvEntry.GetStacks();
		StackPtr = StackView.GetData();
		NumRemaining = StackView.Num()-1;
	}

	FFaerieEntryKey FIterator_AllAddresses::GetKey() const
	{
		return Content.GetKeyAt(EntryIndex);
	}

	FFaerieAddress FIterator_AllAddresses::GetAddress() const
	{
		return UFaerieItemStorage::MakeAddress(Content.GetKeyAt(EntryIndex), StackPtr->Key);
	}

	FFaerieItemInstance FIterator_AllAddresses::GetInstance() const
	{
		return Content.GetElementAt(EntryIndex).GetInstance();
	}

	int32 FIterator_AllAddresses::GetCopies() const
	{
		return Content.GetElementAt(EntryIndex).GetStack(StackPtr->Key);
	}

	void FIterator_AllAddresses::operator++()
	{
		if (NumRemaining > 0)
		{
			NumRemaining--;
			StackPtr++;
		}
		else
		{
			AdvanceEntry();
		}
	}

	FIterator_SingleEntry::FIterator_SingleEntry(const FFaerieStorageEntry& Entry)
	  : Entry(Entry)
	{
		const TConstArrayView<FFaerieKeyedStack> StackView = Entry.GetStacks();
		StackPtr = StackView.GetData();
		NumRemaining = StackView.Num()-1;
		checkSlow(StackPtr);
	}

	FIterator_SingleEntry::FIterator_SingleEntry(const TNotNull<const UFaerieItemStorage*> Storage, const FFaerieEntryKey Key)
	  : Entry(ReadInventoryContent(Storage)[Key])
	{
		const TConstArrayView<FFaerieKeyedStack> StackView = Entry.GetStacks();
		StackPtr = StackView.GetData();
		NumRemaining = StackView.Num()-1;
		checkSlow(StackPtr);
	}

	FIterator_SingleEntry::FIterator_SingleEntry(const TNotNull<const UFaerieItemStorage*> Storage, const int32 Index)
	  : Entry(ReadInventoryContent(Storage).GetElementAt(Index))
	{
		const TConstArrayView<FFaerieKeyedStack> StackView = Entry.GetStacks();
		StackPtr = StackView.GetData();
		NumRemaining = StackView.Num()-1;
		checkSlow(StackPtr);
	}

	FFaerieEntryKey FIterator_SingleEntry::GetKey() const
	{
		return Entry.GetKey();
	}

	FFaerieAddress FIterator_SingleEntry::GetAddress() const
	{
		return UFaerieItemStorage::MakeAddress(Entry.GetKey(), StackPtr->Key);
	}

	FFaerieItemInstance FIterator_SingleEntry::GetInstance() const
	{
		return Entry.GetInstance();
	}

	int32 FIterator_SingleEntry::GetCopies() const
	{
		return Entry.GetStack(StackPtr->Key);
	}

	void FIterator_SingleEntry::operator++()
	{
		if (NumRemaining > 0)
		{
			NumRemaining--;
			StackPtr++;
		}
		else
		{
			StackPtr = nullptr;
		}
	}

	const IFaerieItemOwnerInterface* FIterator_AllEntries_WithInterface::ResolveOwner() const
	{
		return Storage;
	}

	const IFaerieItemOwnerInterface* FIterator_AllAddresses_WithInterface::ResolveOwner() const
	{
		return Storage;
	}

	const IFaerieItemOwnerInterface* FIterator_SingleEntry_WithInterface::ResolveOwner() const
	{
		return Storage;
	}
}
