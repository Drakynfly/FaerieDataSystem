// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemStorageIterators.h"
#include "FaerieItemStorage.h"

namespace Faerie::Storage
{
	// ReSharper disable once CppMemberFunctionMayBeStatic
	const FInventoryContent& FStorageDataAccess::ReadInventoryContent(const TNotNull<const UFaerieItemStorage*> Storage)
	{
		return Storage->EntryMap;
	}

	FIterator_AllEntries::FIterator_AllEntries(const TNotNull<const UFaerieItemStorage*> Storage)
	  : Content(&ReadInventoryContent(Storage))
	{
		Content->LockWriteAccess();
		AdvanceEntry();
	}

	FIterator_AllEntries::~FIterator_AllEntries()
	{
		Content->UnlockWriteAccess();
	}

	TUniquePtr<Container::IIterator> FIterator_AllEntries::ToInterface() const
	{
		return TUniquePtr<Container::IIterator>(MakeUnique<FIterator_AllEntries_ForInterface>(Content->GetOuterItemStorage()));
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

	FFaerieItemStackView FIterator_AllEntries::GetView() const
	{
		return Content->GetElementAt(EntryIndex).ToItemStackView();
	}

	FIterator_AllAddresses::FIterator_AllAddresses(const TNotNull<const UFaerieItemStorage*> Storage)
	  : Content(&ReadInventoryContent(Storage))
	{
		Content->LockWriteAccess();
		AdvanceEntry();
	}

	FIterator_AllAddresses::~FIterator_AllAddresses()
	{
		Content->UnlockWriteAccess();
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

	FFaerieItemStackView FIterator_AllAddresses::GetView() const
	{
		auto& Entry = Content->GetElementAt(EntryIndex);
		return FFaerieItemStackView(Entry.GetItem(), Entry.GetStack(StackPtr->Key));
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

	FIterator_SingleEntry::FIterator_SingleEntry(const FInventoryEntry& Entry)
	  : EntryPtr(&Entry)
	{
		const TConstArrayView<FKeyedStack> StackView = EntryPtr->GetStacks();
		StackPtr = StackView.GetData();
		NumRemaining = StackView.Num()-1;
		checkSlow(StackPtr);
	}

	FIterator_SingleEntry::FIterator_SingleEntry(const TNotNull<const UFaerieItemStorage*> Storage, const FEntryKey Key)
	  : EntryPtr(&ReadInventoryContent(Storage)[Key])
	{
		const TConstArrayView<FKeyedStack> StackView = EntryPtr->GetStacks();
		StackPtr = StackView.GetData();
		NumRemaining = StackView.Num()-1;
		checkSlow(StackPtr);
	}

	FIterator_SingleEntry::FIterator_SingleEntry(const TNotNull<const UFaerieItemStorage*> Storage, const int32 Index)
	  : EntryPtr(&ReadInventoryContent(Storage).GetElementAt(Index))
	{
		const TConstArrayView<FKeyedStack> StackView = EntryPtr->GetStacks();
		StackPtr = StackView.GetData();
		NumRemaining = StackView.Num()-1;
		checkSlow(StackPtr);
	}

	FEntryKey FIterator_SingleEntry::GetKey() const
	{
		return EntryPtr->GetKey();
	}

	FFaerieAddress FIterator_SingleEntry::GetAddress() const
	{
		return UFaerieItemStorage::MakeAddress(EntryPtr->GetKey(), StackPtr->Key);
	}

	const UFaerieItem* FIterator_SingleEntry::GetItem() const
	{
		return EntryPtr->GetItem();
	}

	FFaerieItemStackView FIterator_SingleEntry::GetView() const
	{
		return FFaerieItemStackView(EntryPtr->GetItem(), EntryPtr->GetStack(StackPtr->Key));
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

	const IFaerieItemOwnerInterface* FIterator_AllEntries_ForInterface::ResolveOwner() const
	{
		return Storage;
	}

	const IFaerieItemOwnerInterface* FIterator_AllAddresses_ForInterface::ResolveOwner() const
	{
		return Storage;
	}

	const IFaerieItemOwnerInterface* FIterator_SingleEntry_ForInterface::ResolveOwner() const
	{
		return Storage;
	}
}
