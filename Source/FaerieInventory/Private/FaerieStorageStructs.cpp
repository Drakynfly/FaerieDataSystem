// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieStorageStructs.h"
#include "DebuggingFlags.h"
#include "FaerieInventoryLog.h"
#include "FaerieItemStorage.h"

#include "Fragments/FaerieStackLimitFragment.h"

#include "HAL/LowLevelMemStats.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieStorageStructs)

DECLARE_LLM_MEMORY_STAT(TEXT("ItemStorage"), STAT_StorageLLM, STATGROUP_LLMFULL);
DECLARE_LLM_MEMORY_STAT(TEXT("ItemStorage"), STAT_StorageSummaryLLM, STATGROUP_LLM);
LLM_DEFINE_TAG(ItemStorage, NAME_None, NAME_None, GET_STATFNAME(STAT_StorageLLM), GET_STATFNAME(STAT_StorageSummaryLLM));

FFaerieEntryKey FFaerieEntryKey::InvalidKey;

// @TODO THIS IS A COPY OF ENCODE FROM FAERIEITEMSTORAGE.cpp fix at some point
namespace LocalCopy
{
	[[nodiscard]] UE_REWRITE FFaerieAddress Encode(const FFaerieEntryKey Entry, const FFaerieStackKey Stack)
	{
		return FFaerieAddress((static_cast<int64>(Entry.Value()) << 32) | static_cast<int64>(Stack.Value()));
	}
}

FFaerieStorageEntry::FFaerieStorageEntry(const FFaerieEntryKey EntryKey, FFaerieItemInstance& Item, const int32 StackLimit, int32 Amount, TArray<FFaerieAddress>& OutNewAddresses)
  : Key(EntryKey), ItemInstance(MoveTemp(Item)), CachedLimit(StackLimit)
{
	if (StackLimit == Faerie::ItemData::UnlimitedStack)
	{
		const FFaerieStackKey NewKey = KeyGen.NextKey();
		Stacks.Emplace(NewKey, Amount);
		OutNewAddresses.Add(LocalCopy::Encode(Key, NewKey));
	}
	else
	{
		// Split the incoming stack into as many more as are required
		while (Amount > 0)
		{
			const int32 NewStack = FMath::Min(Amount, StackLimit);
			Amount -= NewStack;

			const FFaerieStackKey NewKey = KeyGen.NextKey();
			Stacks.Emplace(NewKey, NewStack);
			OutNewAddresses.Add(LocalCopy::Encode(Key, NewKey));
		}
	}
}

FFaerieStorageEntry::FFaerieStorageEntry(const FFaerieEntryKey EntryKey, FFaerieItemInstance& Item, const int32 StackLimit, const TConstArrayView<FFaerieKeyedStack> Stacks)
  : Key(EntryKey), ItemInstance(MoveTemp(Item)), Stacks(Stacks), CachedLimit(StackLimit)
{
	// @Todo split Stacks up if any are larger than StackLimit
}

int32 FFaerieStorageEntry::GetStackIndex(const FFaerieStackKey InKey) const
{
	return Algo::BinarySearchBy(Stacks, InKey, &FFaerieKeyedStack::Key);
}

const FFaerieKeyedStack* FFaerieStorageEntry::GetStackPtr(const FFaerieStackKey InKey) const
{
	if (const int32 StackIndex = GetStackIndex(InKey);
		StackIndex != INDEX_NONE)
	{
		return &Stacks[StackIndex];
	}
	return nullptr;
}

void FFaerieStorageEntry::UpdateCachedStackLimit(const FMassEntityManager& EntityManager)
{
	CachedLimit = Faerie::Container::GetItemStackLimit(&EntityManager, ItemInstance);
}

bool FFaerieStorageEntry::Contains(const FFaerieStackKey InKey) const
{
	return GetStackIndex(InKey) != INDEX_NONE;
}

int32 FFaerieStorageEntry::GetStack(const FFaerieStackKey InKey) const
{
	if (auto&& KeyedStack = GetStackPtr(InKey))
	{
		return KeyedStack->Stack;
	}
	return 0;
}

FFaerieStackKey FFaerieStorageEntry::GetStackAt(const int32 Index) const
{
	return Stacks[Index].Key;
}

FFaerieAddress FFaerieStorageEntry::FirstAddress() const
{
	return LocalCopy::Encode(Key, Stacks[0].Key);
}

void FFaerieStorageEntry::CopyKeys(TArray<FFaerieStackKey>& OutKeys) const
{
	Algo::Transform(Stacks, OutKeys, &FFaerieKeyedStack::Key);
}

void FFaerieStorageEntry::CopyAddresses(TArray<FFaerieAddress>& OutAddresses) const
{
	for (auto&& Element : Stacks)
	{
		OutAddresses.Add(LocalCopy::Encode(Key, Element.Key));
	}
}

void FFaerieStorageEntry::CopyStacks(TArray<int32>& OutStacks) const
{
	Algo::Transform(Stacks, OutStacks, &FFaerieKeyedStack::Stack);
}

int32 FFaerieStorageEntry::StackSum() const
{
	int32 Out = 0;

	for (auto&& KeyedStack : Stacks)
	{
		Out += KeyedStack.Stack;
	}

	return Out;
}

bool FFaerieStorageEntry::IsValid() const
{
	// No instance, obviously invalid
	if (ItemInstance.IsEmpty()) return false;

	// No stacks, invalid
	if (Stacks.IsEmpty()) return false;

	// Check that each stack is valid
	for (auto&& Element : Stacks)
	{
		if (!Element.Key.IsValid() || !Faerie::ItemData::IsValidStackAmount(Element.Stack))
		{
			return false;
		}
	}

	// Everything is good
	return true;
}

bool FFaerieStorageEntry::IsOnlyStack(const FFaerieStackKey InKey) const
{
	return Stacks.Num() == 1 && Stacks[0].Key == InKey;
}

void FFaerieStorageEntry::PostSerialize(const FArchive& Ar)
{
	if (Ar.IsLoading())
	{
		if (!Stacks.IsEmpty())
		{
			KeyGen.SetPosition(Stacks.Last().Key);
		}
	}
}

void FFaerieStorageEntry::PreReplicatedRemove(const FFaerieStorageContent& InArraySerializer)
{
	if (::IsValid(InArraySerializer.ChangeListener))
	{
		InArraySerializer.ChangeListener->Client_PreContentRemoved(*this);
	}
}

void FFaerieStorageEntry::PostReplicatedAdd(const FFaerieStorageContent& InArraySerializer)
{
	if (::IsValid(InArraySerializer.ChangeListener))
	{
		InArraySerializer.ChangeListener->Client_PostContentAdded(*this);
	}
}

void FFaerieStorageEntry::PostReplicatedChange(const FFaerieStorageContent& InArraySerializer)
{
	if (::IsValid(InArraySerializer.ChangeListener))
	{
		InArraySerializer.ChangeListener->Client_PostContentChanged(*this);
	}
}

const IFaerieItemOwnerInterface* FFaerieStorageEntry::FStorageContentAccess::GetListener(const FFaerieStorageContent& Source)
{
	return Source.ChangeListener;
}

#if FAERIE_DEBUG
uint32& FFaerieStorageEntry::FStorageContentAccess::GetWriteLock(const FFaerieStorageContent& Source)
{
	return Source.WriteLock;
}
#endif

const IFaerieItemOwnerInterface* FFaerieStorageEntry::FReadAccess::GetItemOwner() const
{
	return FStorageContentAccess::GetListener(Source);
}

const IFaerieItemOwnerInterface* FFaerieStorageEntry::FStackReadAccess::GetItemOwner() const
{
	return FStorageContentAccess::GetListener(Source);
}

FFaerieAddress FFaerieStorageEntry::FStackReadAccess::ResolveAddress() const
{
	return LocalCopy::Encode(Entry.Key, Stack.Key);
}

FFaerieStorageEntry::FReadWriteAccess::FReadWriteAccess(FFaerieStorageContent& Source, const FFaerieStorageEntry& Entry)
	: Entry(const_cast<FFaerieStorageEntry&>(Entry)),
	  Source(Source)
{
#if FAERIE_DEBUG
	if (Faerie::Debug::CVarEnableWriteLockTracking.GetValueOnGameThread())
	{
		UE_LOG(LogFaerieInventory, Warning, TEXT("WriteLock++ (FReadWriteAccess ctor 1)"))
	}
	FStorageContentAccess::GetWriteLock(Source)++;
#endif
	//ChangeMask.Init(false, Entry.NumStacks());
}

FFaerieStorageEntry::FReadWriteAccess::~FReadWriteAccess()
{
	//checkSlow(Entry.NumStacks() == ChangeMask.Num());

#if FAERIE_DEBUG
	if (Faerie::Debug::CVarEnableWriteLockTracking.GetValueOnGameThread())
	{
		ensureAlways(FStorageContentAccess::GetWriteLock(Source) > 0);
		UE_LOG(LogFaerieInventory, Warning, TEXT("WriteLock-- (FReadWriteAccess dtor)"))
	}
	FStorageContentAccess::GetWriteLock(Source)--;
#endif

	// Propagate change to client
	Source.MarkItemDirty(Entry);
}

void FFaerieStorageEntry::FReadWriteAccess::SetStack(const FFaerieStackKey InKey, const int32 Stack)
{
	if (Stack <= 0)
	{
		RemoveStack(InKey);
		return;
	}

	if (auto&& KeyedStack = GetStackPtr(InKey))
	{
		KeyedStack->Stack = Stack;
	}
	else
	{
		Entry.Stacks.Emplace(InKey, Stack);
		//ChangeMask.Add(true);
	}
}

void FFaerieStorageEntry::FReadWriteAccess::RemoveStack(const FFaerieStackKey InKey)
{
	if (const int32 StackIndex = Entry.GetStackIndex(InKey);
		StackIndex != INDEX_NONE)
	{
		Entry.Stacks.RemoveAt(StackIndex);
		//ChangeMask.RemoveAt(StackIndex);
	}
}

void FFaerieStorageEntry::FReadWriteAccess::AddToAnyStack(int32 Amount, TArray<FFaerieAddress>& OutAddressesTouched)
{
	// Fill existing stacks first
	for (auto It(Entry.Stacks.CreateIterator()); It; ++It)
	{
		FFaerieKeyedStack& KeyedStack = *It;

		if (Entry.CachedLimit == Faerie::ItemData::UnlimitedStack)
		{
			// This stack can contain the rest, add and return
			KeyedStack.Stack += Amount;
			//MarkStackDirty(It.GetIndex());
			OutAddressesTouched.Add(LocalCopy::Encode(Entry.Key, KeyedStack.Key));
			return;
		}

		if (KeyedStack.Stack < Entry.CachedLimit)
		{
			// Calculate how much we can add to this stack
			const int32 SpaceInStack = Entry.CachedLimit - KeyedStack.Stack;
			// Add either the remaining amount or the available space, whichever is smaller
			const int32 AmountToAdd = FMath::Min(Amount, SpaceInStack);

			KeyedStack.Stack += AmountToAdd;
			//MarkStackDirty(It.GetIndex());
			OutAddressesTouched.Add(LocalCopy::Encode(Entry.Key, KeyedStack.Key));

			Amount -= AmountToAdd;

			// If we've used up all the amount, we can return
			if (Amount <= 0)
			{
				return;
			}
		}
	}

	// We have dispersed the incoming stack among existing ones. If there is stack remaining, create new stacks.
	if (Amount > 0)
	{
		return AddToNewStacks(Amount, OutAddressesTouched);
	}
}

void FFaerieStorageEntry::FReadWriteAccess::AddToNewStacks(int32 Amount, TArray<FFaerieAddress>& OutNewAddresses)
{
	struct FLocal
	{
		static void AddStack(FFaerieStorageEntry& Entry, int32 Amount, TArray<FFaerieAddress>& OutNewAddresses)
		{
			const FFaerieStackKey NewKey = Entry.KeyGen.NextKey();
			Entry.Stacks.Emplace(NewKey, Amount);
			//ChangeMask.Add(true);
			OutNewAddresses.Add(LocalCopy::Encode(Entry.Key, NewKey));
		}
	};

	if (Entry.CachedLimit == Faerie::ItemData::UnlimitedStack)
	{
		FLocal::AddStack(Entry, Amount, OutNewAddresses);
	}
	else
	{
		// Split the incoming stack into as many more as are required
		while (Amount > 0)
		{
			const int32 NewStack = FMath::Min(Amount, Entry.CachedLimit);
			Amount -= NewStack;

			FLocal::AddStack(Entry, Amount, OutNewAddresses);
		}
	}
}

int32 FFaerieStorageEntry::FReadWriteAccess::RemoveFromAnyStack(int32 Amount, TArray<FFaerieAddress>& OutAllModifiedAddresses)
{
	// Remove from tail stack first
	for (int32 i = Entry.Stacks.Num() - 1; i >= 0; --i)
	{
		if (FFaerieKeyedStack& KeyedStack = Entry.Stacks[i];
			Amount >= KeyedStack.Stack)
		{
			OutAllModifiedAddresses.Add(LocalCopy::Encode(Entry.Key, KeyedStack.Key));
			Amount -= KeyedStack.Stack;
			Entry.Stacks.RemoveAt(i); // Remove the stack
			//ChangeMask.RemoveAt(i); // Also remove the mask bit for this stack, so we don't get out of sync.

			if (Amount <= 0)
			{
				break;
			}
		}
		else
		{
			KeyedStack.Stack -= Amount;
			//MarkStackDirty(i);
			OutAllModifiedAddresses.Add(LocalCopy::Encode(Entry.Key, KeyedStack.Key));
			break;
		}
	}

	return Amount; // Return the remainder if we didn't remove it all.
}

int32 FFaerieStorageEntry::FReadWriteAccess::MoveStack(const FFaerieStackKey From, const FFaerieStackKey To, const int32 Amount)
{
	const int32 FromIndex = Entry.GetStackIndex(From);
	const int32 ToIndex = Entry.GetStackIndex(To);
	FFaerieKeyedStack& FromStack = Entry.Stacks[FromIndex];
	FFaerieKeyedStack& ToStack = Entry.Stacks[ToIndex];

	const int32 MaxCanFix = Entry.CachedLimit == Faerie::ItemData::UnlimitedStack ? Amount : Entry.CachedLimit - ToStack.Stack;
	const int32 Moving = FMath::Min(Amount, FromStack.Stack, MaxCanFix);

	ToStack.Stack += Moving;
	//MarkStackDirty(ToIndex);

	// Moving the whole stack
	if (FromStack.Stack == Moving)
	{
		Entry.Stacks.RemoveAt(FromIndex);
		//ChangeMask.RemoveAt(FromIndex);
	}
	// Moving partial stack
	else
	{
		FromStack.Stack -= Moving;
		//MarkStackDirty(FromIndex);
	}

	return Amount - Moving;
}

FFaerieStackKey FFaerieStorageEntry::FReadWriteAccess::SplitStack(const FFaerieStackKey InKey, const int32 Amount)
{
	const int32 FromIndex = Entry.GetStackIndex(InKey);
	Entry.Stacks[FromIndex].Stack -= Amount;
	//MarkStackDirty(FromIndex);

	const FFaerieStackKey NewKey = Entry.KeyGen.NextKey();
	Entry.Stacks.Emplace(FFaerieKeyedStack(NewKey, Amount));
	//ChangeMask.Add(true);
	return NewKey;
}

/*
void FFaerieStorageEntry::FReadWriteAccess::MarkStackDirty(const int32 Index)
{
	ChangeMask[Index] |= true;
}

void FFaerieStorageEntry::FReadWriteAccess::MarkAllStacksDirty()
{
	ChangeMask.Init(true, Entry.NumStacks());
}
*/

FFaerieKeyedStack* FFaerieStorageEntry::FReadWriteAccess::GetStackPtr(const FFaerieStackKey InKey) const
{
	if (const int32 StackIndex = Entry.GetStackIndex(InKey);
		StackIndex != INDEX_NONE)
	{
		return &Entry.Stacks[StackIndex];
	}
	return nullptr;
}

void FFaerieStorageContent::Append(const FFaerieStorageEntry& Entry)
{
	check(Entry.Key.IsValid());
#if FAERIE_DEBUG
	check(WriteLock == 0);
#endif

	LLM_SCOPE_BYTAG(ItemStorage);

	// Quick validation that Key *should* be stuck at the end of the array.
	if (!Entries.IsEmpty())
	{
		checkf(Entries.Last().Key < Entry.Key,
			TEXT("If this is hit, then Key is not sequential and Append was not safe to use. Either use a validated Key, or use FFaerieStorageContent::Insert_GetRef"));
	}

	FFaerieStorageEntry& NewItemRef = Entries.Emplace_GetRef(Entry);
	MarkItemDirty(NewItemRef);
}

void FFaerieStorageContent::AppendUnsafe(const FFaerieStorageEntry& Entry)
{
	check(Entry.Key.IsValid());
#if FAERIE_DEBUG
	check(WriteLock == 0);
#endif

	LLM_SCOPE_BYTAG(ItemStorage);

	FFaerieStorageEntry& NewItemRef = Entries.Emplace_GetRef(Entry);
	MarkItemDirty(NewItemRef);
}

void FFaerieStorageContent::Insert(const FFaerieStorageEntry& Entry)
{
	check(Entry.Key.IsValid());
#if FAERIE_DEBUG
	check(WriteLock == 0);
#endif

	LLM_SCOPE_BYTAG(ItemStorage);

	FFaerieStorageEntry& NewEntry = BSOA::Insert_GetRef(Entry);

	MarkItemDirty(NewEntry);
}

void FFaerieStorageContent::Remove(const FFaerieEntryKey Key)
{
	check(Key.IsValid());
#if FAERIE_DEBUG
	check(WriteLock == 0);
#endif

	if (BSOA::Remove(Key))
	{
		// Notify clients of this removal.
		MarkArrayDirty();
	}
}

#if FAERIE_DEBUG
void FFaerieStorageContent::LockWriteAccess() const
{
	if (Faerie::Debug::CVarEnableWriteLockTracking.GetValueOnGameThread())
	{
		UE_LOG(LogFaerieInventory, Warning, TEXT("WriteLock++ (LockWriteAccess)"))
	}
	WriteLock++;
}

void FFaerieStorageContent::UnlockWriteAccess() const
{
	if (Faerie::Debug::CVarEnableWriteLockTracking.GetValueOnGameThread())
	{
		ensureAlways(WriteLock > 0);
		UE_LOG(LogFaerieInventory, Warning, TEXT("WriteLock-- (UnlockWriteAccess)"))
	}
	WriteLock--;
}
#endif

FFaerieStorageContent::TRangedForConstIterator FFaerieStorageContent::begin() const
{
#if FAERIE_DEBUG
	if (Faerie::Debug::CVarEnableWriteLockTracking.GetValueOnGameThread())
	{
		UE_LOG(LogFaerieInventory, Warning, TEXT("WriteLock++ (iterator begin)"))
	}

	WriteLock++;
#endif
	return TRangedForConstIterator(Entries.begin());
}

FFaerieStorageContent::TRangedForConstIterator FFaerieStorageContent::end() const
{
#if FAERIE_DEBUG
	if (Faerie::Debug::CVarEnableWriteLockTracking.GetValueOnGameThread())
	{
		ensureAlways(WriteLock > 0);
		UE_LOG(LogFaerieInventory, Warning, TEXT("WriteLock-- (iterator end)"))
	}
	WriteLock--;
#endif
	return TRangedForConstIterator(Entries.end());
}
