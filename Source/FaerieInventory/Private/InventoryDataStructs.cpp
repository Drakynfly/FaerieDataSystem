// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

// ReSharper disable CppMemberFunctionMayBeConst
#include "InventoryDataStructs.h"
#include "FaerieItemStorage.h"
#include "InventoryDataEnums.h"
#include "HAL/LowLevelMemStats.h"
#include "Tokens/FaerieStackLimiterToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryDataStructs)

DECLARE_LLM_MEMORY_STAT(TEXT("ItemStorage"), STAT_StorageLLM, STATGROUP_LLMFULL);
DECLARE_LLM_MEMORY_STAT(TEXT("ItemStorage"), STAT_StorageSummaryLLM, STATGROUP_LLM);
LLM_DEFINE_TAG(ItemStorage, NAME_None, NAME_None, GET_STATFNAME(STAT_StorageLLM), GET_STATFNAME(STAT_StorageSummaryLLM));

FEntryKey FEntryKey::InvalidKey;

FInventoryEntry::FInventoryEntry(const UFaerieItem* InItem)
	: ItemObject(const_cast<UFaerieItem*>(InItem))
{
	UpdateCachedStackLimit();
}

int32 FInventoryEntry::GetStackIndex(const FStackKey InKey) const
{
	return Algo::BinarySearchBy(Stacks, InKey, &FKeyedStack::Key);
}

FKeyedStack* FInventoryEntry::GetStackPtr(const FStackKey InKey)
{
	if (const int32 StackIndex = GetStackIndex(InKey);
		StackIndex != INDEX_NONE)
	{
		return &Stacks[StackIndex];
	}
	return nullptr;
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

TArray<FStackKey> FInventoryEntry::CopyKeys() const
{
	TArray<FStackKey> Out;
	Algo::Transform(Stacks, Out, &FKeyedStack::Key);
	return Out;
}

TArray<int32> FInventoryEntry::CopyStacks() const
{
	TArray<int32> Out;
	Algo::Transform(Stacks, Out, &FKeyedStack::Stack);
	return Out;
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

void FInventoryEntry::SetStack(const FStackKey InKey, const int32 Stack)
{
	if (Stack <= 0)
	{
		if (const int32 StackIndex = GetStackIndex(InKey);
			StackIndex != INDEX_NONE)
		{
			Stacks.RemoveAt(StackIndex);
		}
		return;
	}

	if (auto&& KeyedStack = GetStackPtr(InKey))
	{
		KeyedStack->Stack = Stack;
	}
	else
	{
		Stacks.Add({InKey, Stack});
	}
}

void FInventoryEntry::AddToAnyStack(int32 Amount, TArray<FStackKey>* OutAddedKeys)
{
	// Fill existing stacks first
	for (auto& KeyedStack : Stacks)
	{
		if (Limit == Faerie::ItemData::UnlimitedStack)
		{
			// This stack can contain the rest, add and return
			KeyedStack.Stack += Amount;
			return;
		}

		if (KeyedStack.Stack < Limit)
		{
			// Calculate how much we can add to this stack
			const int32 SpaceInStack = Limit - KeyedStack.Stack;
			// Add either the remaining amount or the available space, whichever is smaller
			const int32 AmountToAdd = FMath::Min(Amount, SpaceInStack);

			KeyedStack.Stack += AmountToAdd;
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
		return AddToNewStacks(Amount, OutAddedKeys);
	}
}

void FInventoryEntry::AddToNewStacks(int32 Amount, TArray<FStackKey>* OutAddedKeys)
{
	TArray<FStackKey> AddedStacks;

	if (Limit == Faerie::ItemData::UnlimitedStack)
	{
		const FStackKey NewKey = AddedStacks.Add_GetRef(KeyGen.NextKey());
		Stacks.Add({NewKey, Amount});
	}
	else
	{
		// Split the incoming stack into as many more as are required
		while (Amount > 0)
		{
			const FStackKey NewKey = AddedStacks.Add_GetRef(KeyGen.NextKey());
			const int32 NewStack = FMath::Min(Amount, Limit);
			Amount -= NewStack;
			Stacks.Add({NewKey, NewStack});
		}
	}

	if (OutAddedKeys)
	{
		OutAddedKeys->Append(AddedStacks);
	}
}

int32 FInventoryEntry::RemoveFromAnyStack(int32 Amount, TArray<FStackKey>* OutAllModifiedKeys, TArray<FStackKey>* OutRemovedKeys)
{
	TArray<FStackKey> RemovedStacks;

	// Remove from tail stack first
	for (int32 i = Stacks.Num() - 1; i >= 0; --i)
	{
		if (FKeyedStack& KeyedStack = Stacks[i];
			Amount >= KeyedStack.Stack)
		{
			RemovedStacks.Add(KeyedStack.Key);
			Amount -= KeyedStack.Stack;
			Stacks.RemoveAt(i);

			if (Amount <= 0)
			{
				break;
			}
		}
		else
		{
			KeyedStack.Stack -= Amount;
			if (OutAllModifiedKeys)
			{
				OutAllModifiedKeys->Add(KeyedStack.Key);
				OutAllModifiedKeys->Append(RemovedStacks);
			}
			break;
		}
	}

	if (OutRemovedKeys)
	{
		*OutRemovedKeys = RemovedStacks;
	}

	return Amount; // Return the remainder if we didn't remove it all.
}

int32 FInventoryEntry::MoveStack(const FStackKey From, const FStackKey To, const int32 Amount)
{
	const int32 StackIndexA = GetStackIndex(From);
	FKeyedStack& FromStack = Stacks[StackIndexA];
	FKeyedStack& ToStack = *GetStackPtr(To);
	const int32 Moving = FMath::Min(FMath::Min(Amount, FromStack.Stack), Limit - ToStack.Stack);
	FromStack.Stack -= Moving;
	ToStack.Stack += Moving;
	if (FromStack.Stack == 0)
	{
		Stacks.RemoveAt(StackIndexA);
		return 0;
	}
	return FromStack.Stack;
}

FStackKey FInventoryEntry::SplitStack(const FStackKey InKey, const int32 Amount)
{
	GetStackPtr(InKey)->Stack -= Amount;
	const FStackKey NewKey = KeyGen.NextKey();
	Stacks.Emplace(FKeyedStack(NewKey, Amount));
	return NewKey;
}

bool FInventoryEntry::IsValid() const
{
	// No item, obviously invalid
	if (!ItemObject) return false;

	// No stacks, invalid
	if (Stacks.IsEmpty()) return false;

	// Invalid limit
	if (!Faerie::ItemData::IsValidStack(Limit)) return false;

	// Check that each stack is valid
	for (auto&& Element : Stacks)
	{
		if (!Element.Key.IsValid() || !Faerie::ItemData::IsValidStack(Element.Stack))
		{
			return false;
		}
	}

	// Everything is good
	return true;
}

FFaerieItemStackView FInventoryEntry::ToItemStackView() const
{
	FFaerieItemStackView Stack;
	Stack.Item = ItemObject;
	Stack.Copies = StackSum();
	return Stack;
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
	InArraySerializer.PostEntryReplicatedChange(*this, FInventoryContent::Client_SomethingReplicated);
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
	WriteLock++;
}

void FInventoryContent::UnlockWriteAccess() const
{
	WriteLock--;
}

FInventoryContent::FScopedItemHandle::FScopedItemHandle(const FEntryKey Key, FInventoryContent& Source)
  : Handle(Source.Entries[Source.IndexOf(Key)]),
	Source(Source)
{
	Source.WriteLock++;
}

FInventoryContent::FScopedItemHandle::~FScopedItemHandle()
{
	Source.WriteLock--;

	// Propagate change to client
	Source.MarkItemDirty(Handle);

	// Broadcast change on server
	Source.PostEntryReplicatedChange(Handle, Server_ItemHandleClosed);
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

void FInventoryContent::PostEntryReplicatedChange(const FInventoryEntry& Entry, const EChangeType ChangeType) const
{
	if (IsValid(ChangeListener))
	{
		ChangeListener->PostContentChanged(Entry, ChangeType);
	}
}

FInventoryContent::TRangedForConstIterator FInventoryContent::begin() const
{
	WriteLock++;
	return TRangedForConstIterator(Entries.begin());
}

FInventoryContent::TRangedForConstIterator FInventoryContent::end() const
{
	WriteLock--;
	return TRangedForConstIterator(Entries.end());
}
