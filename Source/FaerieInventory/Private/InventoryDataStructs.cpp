// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "InventoryDataStructs.h"
#include "DebuggingFlags.h"
#include "FaerieInventoryLog.h"
#include "FaerieItemStorage.h"
#include "InventoryDataEnums.h"
#include "HAL/LowLevelMemStats.h"
#include "Tokens/FaerieStackLimiterToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryDataStructs)

DECLARE_LLM_MEMORY_STAT(TEXT("ItemStorage"), STAT_StorageLLM, STATGROUP_LLMFULL);
DECLARE_LLM_MEMORY_STAT(TEXT("ItemStorage"), STAT_StorageSummaryLLM, STATGROUP_LLM);
LLM_DEFINE_TAG(ItemStorage, NAME_None, NAME_None, GET_STATFNAME(STAT_StorageLLM), GET_STATFNAME(STAT_StorageSummaryLLM));

FEntryKey FEntryKey::InvalidKey;

// @TODO THIS IS A COPY OF ENCODE FROM FAERIEITEMSTORAGE.cpp fix at some point
[[nodiscard]] UE_REWRITE FFaerieAddress Encode(const FEntryKey Entry, const FStackKey Stack)
{
	return FFaerieAddress((static_cast<int64>(Entry.Value()) << 32) | static_cast<int64>(Stack.Value()));
}

FInventoryEntry::FInventoryEntry(FFaerieItemStackView InStack, TArray<FFaerieAddress>& OutNewAddresses)
{
	ItemObject = InStack.Item.Get();

	UpdateCachedStackLimit();

	if (Limit == Faerie::ItemData::UnlimitedStack)
	{
		const FStackKey NewKey = KeyGen.NextKey();
		Stacks.Emplace(NewKey, InStack.Copies);
		OutNewAddresses.Add(Encode(Key, NewKey));
	}
	else
	{
		// Split the incoming stack into as many more as are required
		while (InStack.Copies > 0)
		{
			const int32 NewStack = FMath::Min(InStack.Copies, Limit);
			InStack.Copies -= NewStack;

			const FStackKey NewKey = KeyGen.NextKey();
			Stacks.Emplace(NewKey, NewStack);
			OutNewAddresses.Add(Encode(Key, NewKey));
		}
	}
}

int32 FInventoryEntry::GetStackIndex(const FStackKey InKey) const
{
	return Algo::BinarySearchBy(Stacks, InKey, &FKeyedStack::Key);
}

const FKeyedStack* FInventoryEntry::GetStackPtr(const FStackKey InKey) const
{
	if (const int32 StackIndex = GetStackIndex(InKey);
		StackIndex != INDEX_NONE)
	{
		return &Stacks[StackIndex];
	}
	return nullptr;
}

void FInventoryEntry::UpdateCachedStackLimit()
{
	Limit = ItemObject ? UFaerieStackLimiterToken::GetItemStackLimit(ItemObject) : 0;
}

bool FInventoryEntry::Contains(const FStackKey InKey) const
{
	return GetStackIndex(InKey) != INDEX_NONE;
}

int32 FInventoryEntry::GetStack(const FStackKey InKey) const
{
	if (auto&& KeyedStack = GetStackPtr(InKey))
	{
		return KeyedStack->Stack;
	}
	return 0;
}

FStackKey FInventoryEntry::GetStackAt(const int32 Index) const
{
	return Stacks[Index].Key;
}

void FInventoryEntry::CopyKeys(TArray<FStackKey>& OutKeys) const
{
	Algo::Transform(Stacks, OutKeys, &FKeyedStack::Key);
}

void FInventoryEntry::CopyAddresses(TArray<FFaerieAddress>& OutAddresses) const
{
	for (auto&& Element : Stacks)
	{
		OutAddresses.Add(Encode(Key, Element.Key));
	}
}

void FInventoryEntry::CopyStacks(TArray<int32>& OutStacks) const
{
	Algo::Transform(Stacks, OutStacks, &FKeyedStack::Stack);
}

int32 FInventoryEntry::StackSum() const
{
	int32 Out = 0;

	for (auto&& KeyedStack : Stacks)
	{
		Out += KeyedStack.Stack;
	}

	return Out;
}

bool FInventoryEntry::IsValid() const
{
	// No item, obviously invalid
	if (!ItemObject) return false;

	// No stacks, invalid
	if (Stacks.IsEmpty()) return false;

	// Invalid limit
	if (!Faerie::ItemData::IsValidStackAmount(Limit)) return false;

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

FFaerieItemStackView FInventoryEntry::ToItemStackView() const
{
	return FFaerieItemStackView
	{
		ItemObject,
		StackSum()
	};
}

void FInventoryEntry::PostSerialize(const FArchive& Ar)
{
	if (Ar.IsLoading())
	{
		if (!Stacks.IsEmpty())
		{
			KeyGen.SetPosition(Stacks.Last().Key);
		}
	}
}

void FInventoryEntry::PostScriptConstruct()
{
	UpdateCachedStackLimit();
}

void FInventoryEntry::PreReplicatedRemove(const FInventoryContent& InArraySerializer)
{
	InArraySerializer.PreEntryReplicatedRemove(*this);
}

void FInventoryEntry::PostReplicatedAdd(const FInventoryContent& InArraySerializer)
{
	InArraySerializer.PostEntryReplicatedAdd(*this);
}

void FInventoryEntry::PostReplicatedChange(const FInventoryContent& InArraySerializer)
{
	InArraySerializer.PostEntryReplicatedChange_Client(*this);
}

bool FInventoryEntry::IsEqualTo(const FInventoryEntry& A, const FInventoryEntry& B, const EEntryEquivalencyFlags CheckFlags)
{
#define TEST_FLAG(Flag, Test)\
	if (EnumHasAnyFlags(CheckFlags, EEntryEquivalencyFlags::Test_##Flag)) if (!(Test)) return false;

	TEST_FLAG(Limit, A.Limit == B.Limit);
	TEST_FLAG(StackSum, A.StackSum() == B.StackSum());
	TEST_FLAG(ItemData, UFaerieItem::Compare(A.ItemObject, B.ItemObject, EFaerieItemComparisonFlags::Default));

#undef TEST_FLAG

	return true;
}

FInventoryEntry::FMutableAccess::FMutableAccess(FInventoryContent& Source, const FInventoryEntry& Entry)
	: Handle(const_cast<FInventoryEntry&>(Entry)),
	  Source(Source)
{
#if FAERIE_DEBUG
	if (Faerie::Debug::CVarEnableWriteLockTracking.GetValueOnGameThread())
	{
		UE_LOG(LogFaerieInventory, Warning, TEXT("WriteLock++ (FMutableAccess ctor 1)"))
	}
#endif
	Source.WriteLock++;
	ChangeMask.Init(false, Handle.NumStacks());
}

FInventoryEntry::FMutableAccess::FMutableAccess(FInventoryContent& Source, const int32 Index)
  : Handle(Source.Entries[Index]),
	Source(Source)
{
#if FAERIE_DEBUG
	if (Faerie::Debug::CVarEnableWriteLockTracking.GetValueOnGameThread())
	{
		UE_LOG(LogFaerieInventory, Warning, TEXT("WriteLock++ (FMutableAccess ctor 2)"))
	}
#endif
	Source.WriteLock++;
	ChangeMask.Init(false, Handle.NumStacks());
}

FInventoryEntry::FMutableAccess::FMutableAccess(FInventoryContent& Source, const FEntryKey Key)
  : Handle(Source.Entries[Source.IndexOf(Key)]),
	Source(Source)
{
#if FAERIE_DEBUG
	if (Faerie::Debug::CVarEnableWriteLockTracking.GetValueOnGameThread())
	{
		UE_LOG(LogFaerieInventory, Warning, TEXT("WriteLock++ (FMutableAccess ctor 3)"))
	}
#endif
	Source.WriteLock++;
	ChangeMask.Init(false, Handle.NumStacks());
}

FInventoryEntry::FMutableAccess::~FMutableAccess()
{
	checkSlow(Handle.NumStacks() == ChangeMask.Num());

#if FAERIE_DEBUG
	if (Faerie::Debug::CVarEnableWriteLockTracking.GetValueOnGameThread())
	{
		ensureAlways(Source.WriteLock > 0);
		UE_LOG(LogFaerieInventory, Warning, TEXT("WriteLock-- (FMutableAccess dtor)"))
	}
#endif

	Source.WriteLock--;

	// Propagate change to client
	Source.MarkItemDirty(Handle);

	// Broadcast change on server if anything was changed
	if (ChangeMask.CountSetBits() != 0)
	{
		Source.PostEntryReplicatedChange_Server(Handle, FInventoryContent::Server_ItemHandleClosed, ChangeMask);
	}
}

void FInventoryEntry::FMutableAccess::SetStack(const FStackKey InKey, const int32 Stack)
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
		Handle.Stacks.Emplace(InKey, Stack);
		ChangeMask.Add(true);
	}
}

void FInventoryEntry::FMutableAccess::RemoveStack(const FStackKey InKey)
{
	if (const int32 StackIndex = Handle.GetStackIndex(InKey);
		StackIndex != INDEX_NONE)
	{
		Handle.Stacks.RemoveAt(StackIndex);
		ChangeMask.RemoveAt(StackIndex);
	}
}

void FInventoryEntry::FMutableAccess::AddToAnyStack(int32 Amount, TArray<FFaerieAddress>& OutNewAddresses)
{
	// Fill existing stacks first
	for (auto It(Handle.Stacks.CreateIterator()); It; ++It)
	{
		FKeyedStack& KeyedStack = *It;

		if (Handle.Limit == Faerie::ItemData::UnlimitedStack)
		{
			// This stack can contain the rest, add and return
			KeyedStack.Stack += Amount;
			MarkStackDirty(It.GetIndex());
			return;
		}

		if (KeyedStack.Stack < Handle.Limit)
		{
			// Calculate how much we can add to this stack
			const int32 SpaceInStack = Handle.Limit - KeyedStack.Stack;
			// Add either the remaining amount or the available space, whichever is smaller
			const int32 AmountToAdd = FMath::Min(Amount, SpaceInStack);

			KeyedStack.Stack += AmountToAdd;
			MarkStackDirty(It.GetIndex());
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
		return AddToNewStacks(Amount, OutNewAddresses);
	}
}

void FInventoryEntry::FMutableAccess::AddToNewStacks(int32 Amount, TArray<FFaerieAddress>& OutNewAddresses)
{
	auto AddStack = [this, Amount, &OutNewAddresses]()
	{
		const FStackKey NewKey = Handle.KeyGen.NextKey();
		Handle.Stacks.Emplace(NewKey, Amount);
		ChangeMask.Add(true);
		OutNewAddresses.Add(Encode(Handle.Key, NewKey));
	};

	if (Handle.Limit == Faerie::ItemData::UnlimitedStack)
	{
		AddStack();
	}
	else
	{
		// Split the incoming stack into as many more as are required
		while (Amount > 0)
		{
			const int32 NewStack = FMath::Min(Amount, Handle.Limit);
			Amount -= NewStack;

			AddStack();
		}
	}
}

int32 FInventoryEntry::FMutableAccess::RemoveFromAnyStack(int32 Amount, TArray<FFaerieAddress>& OutAllModifiedAddresses)
{
	// Remove from tail stack first
	for (int32 i = Handle.Stacks.Num() - 1; i >= 0; --i)
	{
		if (FKeyedStack& KeyedStack = Handle.Stacks[i];
			Amount >= KeyedStack.Stack)
		{
			OutAllModifiedAddresses.Add(Encode(Handle.Key, KeyedStack.Key));
			Amount -= KeyedStack.Stack;
			Handle.Stacks.RemoveAt(i); // Remove the stack
			ChangeMask.RemoveAt(i); // Also remove the mask bit for this stack, so we don't get out of sync.

			if (Amount <= 0)
			{
				break;
			}
		}
		else
		{
			KeyedStack.Stack -= Amount;
			MarkStackDirty(i);
			OutAllModifiedAddresses.Add(Encode(Handle.Key, KeyedStack.Key));
			break;
		}
	}

	return Amount; // Return the remainder if we didn't remove it all.
}

int32 FInventoryEntry::FMutableAccess::MoveStack(const FStackKey From, const FStackKey To, const int32 Amount)
{
	const int32 FromIndex = Handle.GetStackIndex(From);
	const int32 ToIndex = Handle.GetStackIndex(To);
	FKeyedStack& FromStack = Handle.Stacks[FromIndex];
	FKeyedStack& ToStack = Handle.Stacks[ToIndex];

	const int32 MaxCanFix = Handle.Limit == Faerie::ItemData::UnlimitedStack ? Amount : Handle.Limit - ToStack.Stack;
	const int32 Moving = FMath::Min(Amount, FromStack.Stack, MaxCanFix);

	ToStack.Stack += Moving;
	MarkStackDirty(ToIndex);

	// Moving the whole stack
	if (FromStack.Stack == Moving)
	{
		Handle.Stacks.RemoveAt(FromIndex);
		ChangeMask.RemoveAt(FromIndex);
	}
	// Moving partial stack
	else
	{
		FromStack.Stack -= Moving;
		MarkStackDirty(FromIndex);
	}

	return Amount - Moving;
}

FStackKey FInventoryEntry::FMutableAccess::SplitStack(const FStackKey InKey, const int32 Amount)
{
	const int32 FromIndex = Handle.GetStackIndex(InKey);
	Handle.Stacks[FromIndex].Stack -= Amount;
	MarkStackDirty(FromIndex);

	const FStackKey NewKey = Handle.KeyGen.NextKey();
	Handle.Stacks.Emplace(FKeyedStack(NewKey, Amount));
	ChangeMask.Add(true);
	return NewKey;
}

bool FInventoryEntry::FMutableAccess::IsOnlyStack(const FStackKey InKey) const
{
	return Handle.Stacks.Num() == 1 && Handle.Stacks[0].Key == InKey;
}

void FInventoryEntry::FMutableAccess::MarkStackDirty(const int32 Index)
{
	ChangeMask[Index] |= true;
}

void FInventoryEntry::FMutableAccess::MarkAllStacksDirty()
{
	ChangeMask.Init(true, Handle.NumStacks());
}

FKeyedStack* FInventoryEntry::FMutableAccess::GetStackPtr(const FStackKey InKey)
{
	if (const int32 StackIndex = Handle.GetStackIndex(InKey);
		StackIndex != INDEX_NONE)
	{
		return &Handle.Stacks[StackIndex];
	}
	return nullptr;
}

void FInventoryContent::Append(const FInventoryEntry& Entry)
{
	check(Entry.Key.IsValid());
	check(WriteLock == 0);

	LLM_SCOPE_BYTAG(ItemStorage);

	// Quick validation that Key *should* be stuck at the end of the array.
	if (!Entries.IsEmpty())
	{
		checkf(Entries.Last().Key < Entry.Key,
			TEXT("If this is hit, then Key is not sequential and Append was not safe to use. Either use a validated Key, or use FInventoryContent::Insert"));
	}

	FInventoryEntry& NewItemRef = Entries.Emplace_GetRef(Entry);
	MarkItemDirty(NewItemRef);
	PostEntryReplicatedAdd(NewItemRef);
}

void FInventoryContent::AppendUnsafe(const FInventoryEntry& Entry)
{
	check(Entry.Key.IsValid());
	check(WriteLock == 0);

	LLM_SCOPE_BYTAG(ItemStorage);

	FInventoryEntry& NewItemRef = Entries.Emplace_GetRef(Entry);
	MarkItemDirty(NewItemRef);
	PostEntryReplicatedAdd(NewItemRef);
}

void FInventoryContent::Insert(const FInventoryEntry& Entry)
{
	check(Entry.Key.IsValid());
	check(WriteLock == 0);

	LLM_SCOPE_BYTAG(ItemStorage);

	FInventoryEntry& NewEntry = BSOA::Insert(Entry);

	PostEntryReplicatedAdd(NewEntry);
	MarkItemDirty(NewEntry);
}

void FInventoryContent::Remove(const FEntryKey Key)
{
	check(Key.IsValid());
	check(WriteLock == 0);

	if (BSOA::Remove(Key,
		[this](const FInventoryEntry& Entry)
		{
			// Notify owning server of this removal.
			PreEntryReplicatedRemove(Entry);
		}))
	{
		// Notify clients of this removal.
		MarkArrayDirty();
	}
}

void FInventoryContent::LockWriteAccess() const
{
#if FAERIE_DEBUG
	if (Faerie::Debug::CVarEnableWriteLockTracking.GetValueOnGameThread())
	{
		UE_LOG(LogFaerieInventory, Warning, TEXT("WriteLock++ (LockWriteAccess)"))
	}
#endif
	WriteLock++;
}

void FInventoryContent::UnlockWriteAccess() const
{
#if FAERIE_DEBUG
	if (Faerie::Debug::CVarEnableWriteLockTracking.GetValueOnGameThread())
	{
		ensureAlways(WriteLock > 0);
		UE_LOG(LogFaerieInventory, Warning, TEXT("WriteLock-- (UnlockWriteAccess)"))
	}
#endif
	WriteLock--;
}

void FInventoryContent::PreEntryReplicatedRemove(const FInventoryEntry& Entry) const
{
	if (IsValid(ChangeListener))
	{
		ChangeListener->PreContentRemoved(Entry);
	}
}

void FInventoryContent::PostEntryReplicatedAdd(const FInventoryEntry& Entry) const
{
	if (IsValid(ChangeListener))
	{
		ChangeListener->PostContentAdded(Entry);
	}
}

void FInventoryContent::PostEntryReplicatedChange_Server(const FInventoryEntry& Entry, const EChangeType ChangeType, const TBitArray<>& ChangeMask) const
{
	if (IsValid(ChangeListener))
	{
		ChangeListener->PostContentChanged(Entry, ChangeType, &ChangeMask);
	}
}

void FInventoryContent::PostEntryReplicatedChange_Client(const FInventoryEntry& Entry) const
{
	if (IsValid(ChangeListener))
	{
		ChangeListener->PostContentChanged(Entry, Client_SomethingReplicated, nullptr);
	}
}

FInventoryContent::TRangedForConstIterator FInventoryContent::begin() const
{
#if FAERIE_DEBUG
	if (Faerie::Debug::CVarEnableWriteLockTracking.GetValueOnGameThread())
	{
		UE_LOG(LogFaerieInventory, Warning, TEXT("WriteLock++ (iterator begin)"))
	}
#endif
	WriteLock++;
	return TRangedForConstIterator(Entries.begin());
}

FInventoryContent::TRangedForConstIterator FInventoryContent::end() const
{
#if FAERIE_DEBUG
	if (Faerie::Debug::CVarEnableWriteLockTracking.GetValueOnGameThread())
	{
		ensureAlways(WriteLock > 0);
		UE_LOG(LogFaerieInventory, Warning, TEXT("WriteLock-- (iterator end)"))
	}
#endif
	WriteLock--;
	return TRangedForConstIterator(Entries.end());
}
