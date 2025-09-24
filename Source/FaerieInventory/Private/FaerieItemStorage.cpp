// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemStorage.h"
#include "FaerieInventoryLog.h"
#include "FaerieInventorySettings.h"
#include "FaerieItem.h"
#include "FaerieItemStorageStatics.h"
#include "InventoryStorageProxy.h"
#include "ItemContainerExtensionBase.h"
#include "Tokens/FaerieItemStorageToken.h"
#include "Tokens/FaerieStackLimiterToken.h"

#include "Algo/Copy.h"
#include "Net/UnrealNetwork.h"

#if WITH_EDITOR
#include "Engine/Engine.h"
#endif

#include "FaerieContainerFilter.h"
#include "FaerieContainerFilterTypes.h"
#include "FaerieItemDataComparator.h"
#include "FaerieItemStorageIterators.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemStorage)

DECLARE_STATS_GROUP(TEXT("FaerieItemStorage"), STATGROUP_FaerieItemStorage, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Query (First)"), STAT_Storage_QueryFirst, STATGROUP_FaerieItemStorage);
DECLARE_CYCLE_STAT(TEXT("Query (All)"), STAT_Storage_QueryAll, STATGROUP_FaerieItemStorage);
DECLARE_CYCLE_STAT(TEXT("Add to Storage"), STAT_Storage_Add, STATGROUP_FaerieItemStorage);
DECLARE_CYCLE_STAT(TEXT("Remove from Storage"), STAT_Storage_Remove, STATGROUP_FaerieItemStorage);

namespace Faerie
{
	namespace Address
	{
		// @todo
		/**UE_REWRITE**/ [[nodiscard]] inline FFaerieAddress Encode(const FEntryKey Entry, const FStackKey Stack)
		{
			return FFaerieAddress((static_cast<int64>(Entry.Value()) << 32) | static_cast<int64>(Stack.Value()));
		}

		/**UE_REWRITE**/ inline void Decode(const FFaerieAddress Address, FEntryKey& Entry, FStackKey& Stack)
		{
			constexpr int64 Mask = 0x00000000FFFFFFFF;
			Stack = FStackKey(Address.Address & Mask);
			Entry = FEntryKey(Address.Address >> 32);
		}

		/**UE_REWRITE**/ inline void Decode_Entry(const FFaerieAddress Address, FEntryKey& Entry)
		{
			Entry = FEntryKey(Address.Address >> 32);
		}

		/**UE_REWRITE**/ inline void Decode_Stack(const FFaerieAddress Address, FStackKey& Stack)
		{
			constexpr int64 Mask = 0x00000000FFFFFFFF;
			Stack = FStackKey(Address.Address & Mask);
		}

		constexpr bool IfOnlyNewStacks(const EFaerieStorageAddStackBehavior Behavior)
		{
			return Behavior == EFaerieStorageAddStackBehavior::OnlyNewStacks;
		}
	}

	class FItemStorageEntryFilter_ForInterface final : public IContainerFilter
	{
	public:
		FItemStorageEntryFilter_ForInterface(const UFaerieItemStorage* Storage)
		  : Impl(Storage) {}

	protected:
		//* IContainerFilter
		FORCEINLINE virtual void Run_Impl(IItemDataFilter&& Filter) override { Impl.Run(MoveTemp(Filter)); }
		FORCEINLINE virtual void Run_Impl(IEntryKeyFilter&& Filter) override { Impl.Run(MoveTemp(Filter)); }
		FORCEINLINE virtual void Run_Impl(ISnapshotFilter&& Filter) override { Impl.Run(MoveTemp(Filter)); }
		FORCEINLINE virtual void Invert_Impl() override { Impl.Invert(); }
		FORCEINLINE virtual void Reset() override { Impl.Reset(); }
		FORCEINLINE virtual int32 Num() const override { return Impl.Num(); }
		FORCEINLINE virtual FDefaultKeyIterator KeyRange() const override { return Impl.Range<FEntryKey>().Copy(); }
		FORCEINLINE virtual FDefaultAddressIterator AddressRange() const override { return Impl.Range<FFaerieAddress>().Copy(); }
		FORCEINLINE virtual FDefaultItemIterator ItemRange() const override { return Impl.Range<UFaerieItem*>().Copy(); }
		FORCEINLINE virtual FDefaultConstItemIterator ConstItemRange() const override { return Impl.Range<const UFaerieItem*>().Copy(); }
		//* IContainerFilter

	private:
		FItemStorageEntryFilter Impl;
	};

	// @todo re-enable when this filter is implemented
	/*
	class FItemStorageAddressFilter_ForInterface final : public IContainerFilter
	{
	public:
		FItemStorageAddressFilter_ForInterface(const UFaerieItemStorage* Storage)
		  : Impl(Storage) {}

	protected:
		//* IContainerFilter
		FORCEINLINE virtual void Run_Impl(IItemDataFilter&& Filter) override { Impl.Run(MoveTemp(Filter)); }
		FORCEINLINE virtual void Run_Impl(IEntryKeyFilter&& Filter) override { Impl.Run(MoveTemp(Filter)); }
		FORCEINLINE virtual void Run_Impl(ISnapshotFilter&& Filter) override { Impl.Run(MoveTemp(Filter)); }
		FORCEINLINE virtual TUniquePtr<IContainerFilter> Invert_Impl() override
		FORCEINLINE virtual void Invert_Impl() override { Impl.Invert(); }
		FORCEINLINE virtual void Reset() override { Impl.Reset(); }
		FORCEINLINE virtual int32 Num() const override { return Impl.Num(); }
		FORCEINLINE virtual FDefaultKeyIterator KeyRange() const override { return Impl.Range<FEntryKey>().Copy(); }
		FORCEINLINE virtual FDefaultAddressIterator AddressRange() const override { return Impl.Range<FFaerieAddress>().Copy(); }
		FORCEINLINE virtual FDefaultItemIterator ItemRange() const override { return Impl.Range<UFaerieItem*>().Copy(); }
		FORCEINLINE virtual FDefaultConstItemIterator ConstItemRange() const override { return Impl.Range<const UFaerieItem*>().Copy(); }
		//* IContainerFilter

	private:
		FItemStorageAddressFilter Impl;
	};
	*/
}

using namespace Faerie::Address;


void UFaerieItemStorage::PostInitProperties()
{
	Super::PostInitProperties();

	// Bind replication functions out into this class.
	EntryMap.ChangeListener = this;
}

void UFaerieItemStorage::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, EntryMap, SharedParams);
}

void UFaerieItemStorage::PostLoad()
{
	Super::PostLoad();

	// Determine the next valid key to use.
	if (!EntryMap.IsEmpty())
	{
		KeyGen.SetPosition(EntryMap.GetKeyAt(EntryMap.Num()-1));
	}
	// See Footnote1
}

void UFaerieItemStorage::InitializeNetObject(AActor* Actor)
{
	Super::InitializeNetObject(Actor);
	Actor->AddReplicatedSubObject(Extensions);
	Extensions->InitializeNetObject(Actor);
}

void UFaerieItemStorage::DeinitializeNetObject(AActor* Actor)
{
	Extensions->DeinitializeNetObject(Actor);
	Actor->RemoveReplicatedSubObject(Extensions);
	Super::DeinitializeNetObject(Actor);
}

FInstancedStruct UFaerieItemStorage::MakeSaveData(TMap<FGuid, FInstancedStruct>& ExtensionData) const
{
	ensureMsgf(GetDefault<UFaerieInventorySettings>()->ContainerMutableBehavior == EFaerieContainerOwnershipBehavior::Rename,
		TEXT("Flakes relies on ownership of sub-objects. Rename must be enabled! (ProjectSettings -> Faerie Inventory -> Container Mutable Behavior)"));

	RavelExtensionData(ExtensionData);
	return FInstancedStruct::Make(EntryMap);
}

void UFaerieItemStorage::LoadSaveData(const FConstStructView ItemData, UFaerieItemContainerExtensionData* ExtensionData)
{
	// Clear out state

	Clear(Faerie::Inventory::Tags::RemovalDeletion);
	KeyGen.Reset();

	Extensions->DeinitializeExtension(this);

	// Load in save data
	EntryMap = ItemData.Get<const FInventoryContent>();
	TArray<FEntryKey, TInlineAllocator<4>> InvalidKeys;
	for (const FInventoryEntry& Entry : EntryMap)
	{
		if (!Faerie::ValidateItemData(Entry.GetItem()))
		{
			InvalidKeys.Add(Entry.Key);
		}
	}

	for (const FEntryKey InvalidKey : InvalidKeys)
	{
		EntryMap.Remove(InvalidKey);
	}

	EntryMap.MarkArrayDirty();
	EntryMap.ChangeListener = this;

	// Determine the next valid key to use.
	if (!EntryMap.IsEmpty())
	{
		KeyGen.SetPosition(EntryMap.GetKeyAt(EntryMap.Num()-1));
	}

	// Rebuild extension state

	//@todo broadcast full refresh event?

	Extensions->InitializeExtension(this);

	UnravelExtensionData(ExtensionData);
}

bool UFaerieItemStorage::Contains(const FEntryKey Key) const
{
	return EntryMap.Contains(Key);
}

FFaerieItemStackView UFaerieItemStorage::View(const FEntryKey Key) const
{
	return EntryMap[Key].ToItemStackView();
}

FFaerieItemStack UFaerieItemStorage::Release(const FEntryKey Key, const int32 Copies)
{
	if (FFaerieItemStack OutStack;
		TakeEntry(Key, OutStack, Faerie::Inventory::Tags::RemovalMoving, Copies))
	{
		return OutStack;
	}
	return FFaerieItemStack();
}

int32 UFaerieItemStorage::GetStack(const FEntryKey Key) const
{
	if (const FInventoryEntry* EntryPtr = GetEntrySafe(Key))
	{
		// Return the total items stored by this key, across all stacks, since this API doesn't know about stacks.
		return EntryPtr->StackSum();
	}
	return 0;
}

bool UFaerieItemStorage::Contains(const FFaerieAddress Address) const
{
	FEntryKey Entry;
	FStackKey Stack;
	Decode(Address, Entry, Stack);
	if (auto&& EntryPtr = GetEntrySafe(Entry))
	{
		return EntryPtr->Contains(Stack);
	}
	return false;
}

int32 UFaerieItemStorage::GetStack(const FFaerieAddress Address) const
{
	FEntryKey Entry;
	FStackKey Stack;
	Decode(Address, Entry, Stack);
	if (auto&& EntryPtr = GetEntrySafe(Entry))
	{
		return EntryPtr->GetStack(Stack);
	}
	return 0;
}

const UFaerieItem* UFaerieItemStorage::ViewItem(const FFaerieAddress Address) const
{
	FEntryKey Entry;
	Decode_Entry(Address, Entry);
	if (auto&& EntryPtr = GetEntrySafe(Entry))
	{
		return EntryPtr->GetItem();
	}
	return nullptr;
}

FFaerieItemStackView UFaerieItemStorage::ViewStack(const FFaerieAddress Address) const
{
	FEntryKey Entry;
	FStackKey Stack;
	Decode(Address, Entry, Stack);
	if (auto&& EntryPtr = GetEntrySafe(Entry))
	{
		if (const int32 StackValue = EntryPtr->GetStack(Stack);
			0 < StackValue)
		{
			return FFaerieItemStackView(EntryPtr->GetItem(), StackValue);
		}
	}
	return FFaerieItemStackView();
}

FFaerieItemProxy UFaerieItemStorage::Proxy(const FFaerieAddress Address) const
{
	return GetStackProxyImpl(Address);
}

FFaerieItemStack UFaerieItemStorage::Release(const FFaerieAddress Address, const int32 Copies)
{
	if (FFaerieItemStack OutStack;
		TakeStack(Address, OutStack, Faerie::Inventory::Tags::RemovalMoving, Copies))
	{
		return OutStack;
	}
	return FFaerieItemStack();
}

TUniquePtr<Faerie::IContainerIterator> UFaerieItemStorage::CreateIterator() const
{
	// Don't provide an iterator if we are empty...
	if (EntryMap.IsEmpty()) return nullptr;

	// Otherwise, use the full iterator.
	return MakeUnique<Faerie::FStorageIterator_AllAddresses_ForInterface>(this);
}

TUniquePtr<Faerie::IContainerFilter> UFaerieItemStorage::CreateFilter(const bool FilterByAddresses) const
{
	if (FilterByAddresses)
	{
		unimplemented();
		return nullptr;
		//return MakeUnique<Faerie::FItemStorageAddressFilter_ForInterface>(this);
	}
	return MakeUnique<Faerie::FItemStorageEntryFilter_ForInterface>(this);
}

FFaerieItemStack UFaerieItemStorage::Release(const FFaerieItemStackView Stack)
{
	const FEntryKey Key = FindItem(Stack.Item.Get(), EFaerieItemEqualsCheck::ComparePointers);
	return Release(Key, Stack.Copies);
}

bool UFaerieItemStorage::Possess(const FFaerieItemStack Stack)
{
	if (!IsValid(Stack.Item) ||
		Stack.Copies < 1) return false;

	return AddStackImpl(Stack, false).Success;
}

void UFaerieItemStorage::OnItemMutated(const UFaerieItem* Item, const UFaerieItemToken* Token, const FGameplayTag EditTag)
{
	Super::OnItemMutated(Item, Token, EditTag);

	// Iterating entire map is annoying but unavoidable.
	for (const FInventoryEntry& Element : EntryMap)
	{
		if (Element.GetItem() == Item)
		{
			PostContentChanged(Element, FInventoryContent::ItemMutated);
			return;
		}
	}
}

TArray<FEntryKey> UFaerieItemStorage::GetAllEntries() const
{
	TArray<FEntryKey> Keys;
	Keys.Reserve(EntryMap.Num());
	Algo::Transform(EntryMap, Keys, &FInventoryEntry::Key);
	return Keys;
}

void UFaerieItemStorage::PostContentAdded(const FInventoryEntry& Entry)
{
	if (!Entry.Key.IsValid())
	{
		UE_LOG(LogFaerieInventory, Warning, TEXT("PostContentAdded: Received Invalid Key"))
		return;
	}

	OnKeyAdded.Broadcast(this, Entry.Key);

	// Proxies may already exist for keys on the client if they are replicated by extensions or other means, and
	// happened to arrive before we got them.
	TArray<FFaerieAddress> Addresses;
	Addresses.Reserve(Entry.GetStacks().Num());
	for (const FKeyedStack& Stack : Entry.GetStacks())
	{
		Addresses.Add(Encode(Entry.Key, Stack.Key));
	}

	for (const FFaerieAddress Address : Addresses)
	{
		BroadcastAddressEvent(EFaerieAddressEventType::PostAdd, Address);
	}

	for (const FFaerieAddress Address : Addresses)
	{
		if (auto&& StackProxy = LocalStackProxies.Find(Address))
		{
			if (StackProxy->IsValid())
			{
				StackProxy->Get()->NotifyCreation();
			}
		}
	}
}

void UFaerieItemStorage::PreContentRemoved(const FInventoryEntry& Entry)
{
	if (!Entry.Key.IsValid())
	{
		UE_LOG(LogFaerieInventory, Warning, TEXT("PreContentRemoved: Received Invalid Key"))
		return;
	}

	OnKeyRemoved.Broadcast(this, Entry.Key);

	for (const FFaerieAddress Address : Faerie::FStorageIterator_SingleEntry(this, Entry.Key))
	{
		BroadcastAddressEvent(EFaerieAddressEventType::PreRemove, Address);
	}

	// Cleanup local views.
	for (auto&& Stack : Entry.GetStacks())
	{
		TWeakObjectPtr<UInventoryStackProxy> StackProxy;
		LocalStackProxies.RemoveAndCopyValue(Encode(Entry.Key, Stack.Key), StackProxy);
		if (StackProxy.IsValid())
		{
			StackProxy->NotifyRemoval();
		}
	}
}

void UFaerieItemStorage::PostContentChanged(const FInventoryEntry& Entry, const FInventoryContent::EChangeType ChangeType)
{
	if (!ensure(Entry.Key.IsValid()))
	{
		UE_LOG(LogFaerieInventory, Warning, TEXT("PostContentChanged: Received Invalid Key"))
		return;
	}

	if (!Entry.IsValid())
	{
		return;
	}

	// Call updates on any entry and stack proxies
	if (Contains(Entry.Key))
	{
		OnKeyUpdated.Broadcast(this, Entry.Key);

		switch (ChangeType)
		{
		case FInventoryContent::Server_ItemHandleClosed:
		case FInventoryContent::Client_SomethingReplicated:
			{
				// @todo it's overkill to update all addresses for a stack change... but we dont know how to determine which stack actually changed, so blast them all :/
				for (const FFaerieAddress& Address : Faerie::FStorageIterator_SingleEntry(this, Entry.Key))
				{
					BroadcastAddressEvent(EFaerieAddressEventType::Edit, Address);
				}
			}
			break;
		case FInventoryContent::ItemMutated:
			{
				// Update all addresses for the item after a mutation.
				for (const FFaerieAddress& Address : Faerie::FStorageIterator_SingleEntry(this, Entry.Key))
				{
					BroadcastAddressEvent(EFaerieAddressEventType::Edit, Address);
				}
			}
			break;
		}

		// Call updates on any stack proxies.
		// PostContentChanged is called when stacks are removed as well, so let's do some cleanup here.
		// Start by getting all the Addresses that we could have proxies for.
		TSet<FFaerieAddress> Addresses;
		Addresses.Reserve(Entry.GetStacks().Num());
		for (const FKeyedStack& Stack : Entry.GetStacks())
		{
			Addresses.Add(Encode(Entry.Key, Stack.Key));
		}

		for (auto It = LocalStackProxies.CreateIterator(); It; ++It)
		{
			auto&& LocalStackProxy = *It;

			// Check for local proxies that match this entry
			if (!LocalStackProxy.Value.IsValid() ||
				LocalStackProxy.Value->GetKey() != Entry.Key)
			{
				continue;
			}

			// If we are supposed to have this key, update it.
			if (Addresses.Contains(LocalStackProxy.Key))
			{
				LocalStackProxy.Value->NotifyUpdate();
			}
			// Otherwise, discard it.
			else
			{
				It.RemoveCurrent();
				LocalStackProxy.Value->NotifyRemoval();
			}
		}
	}
	else
	{
		// Do nothing, PreContentRemoved should handle this ...
	}
}

void UFaerieItemStorage::BroadcastAddressEvent(const EFaerieAddressEventType Type, const FFaerieAddress Address)
{
	OnAddressEventCallback.Broadcast(this, Type, Address);
	OnAddressEvent.Broadcast(this, Type, Address);
}


	/**------------------------------*/
	/*	  INTERNAL IMPLEMENTATIONS	 */
	/**------------------------------*/

const FInventoryEntry* UFaerieItemStorage::GetEntrySafe(const FEntryKey Key) const
{
	return EntryMap.Find(Key);
}

UInventoryStackProxy* UFaerieItemStorage::GetStackProxyImpl(const FFaerieAddress Address) const
{
	// Don't create proxies for invalid keys.
	if (!Address.IsValid()) return nullptr;

	if (auto&& ExistingProxy = LocalStackProxies.Find(Address))
	{
		if (ExistingProxy && ExistingProxy->IsValid())
		{
			return ExistingProxy->Get();
		}
	}

	ThisClass* This = const_cast<ThisClass*>(this);

	FEntryKey Entry;
	FStackKey Stack;
	Decode(Address, Entry, Stack);
	const FName ProxyName = MakeUniqueObjectName(This, UInventoryStackProxy::StaticClass(),
												 *FString::Printf(TEXT("STACK_PROXY_%s_%s"),
												 *Entry.ToString(), *Stack.ToString()));
	UInventoryStackProxy* NewEntryProxy = NewObject<UInventoryStackProxy>(This, UInventoryStackProxy::StaticClass(), ProxyName);
	check(IsValid(NewEntryProxy));

	NewEntryProxy->ItemStorage = This;
	NewEntryProxy->Address = Address;

	if (Contains(Address))
	{
		NewEntryProxy->NotifyCreation();
	}

	This->LocalStackProxies.Add(Address, NewEntryProxy);

	return NewEntryProxy;
}

Faerie::Inventory::FEventLog UFaerieItemStorage::AddStackImpl(const FFaerieItemStack& InStack, const bool ForceNewStack)
{
	if (!ensureAlwaysMsgf(
			IsValid(InStack.Item) &&
			Faerie::ItemData::IsValidStack(InStack.Copies),
			TEXT("AddStackImpl was passed an invalid stack.")))
	{
		return Faerie::Inventory::FEventLog::AdditionFailed("AddStackImpl was passed an invalid stack.");
	}

	SCOPE_CYCLE_COUNTER(STAT_Storage_Add);

	Faerie::Inventory::FEventLog Event;

	// Setup Log for this event
	Event.Type = Faerie::Inventory::Tags::Addition;
	Event.Item = InStack.Item;
	Event.Amount = InStack.Copies;

	// Mutables cannot stack, due to, well, being mutable, meaning that each individual retains the ability to
	// uniquely mutate from others.
	if (!InStack.Item->CanMutate())
	{
		Event.EntryTouched = FindItem(InStack.Item, EFaerieItemEqualsCheck::UseCompareWith);
	}

	// Execute PreAddition on all extensions
	Extensions->PreAddition(this, {InStack.Item, Event.Amount });

	// Try to fill up the stacks of existing entries first, before creating a new entry.
	if (Event.EntryTouched.IsValid())
	{
		const FInventoryContent::FScopedItemHandle Entry = EntryMap.GetHandle(Event.EntryTouched);
		if (ForceNewStack)
		{
			Entry->AddToNewStacks(Event.Amount, &Event.StackKeys);
		}
		else
		{
			Entry->AddToAnyStack(Event.Amount, &Event.StackKeys);
		}
	}
	else
	{
		Faerie::TakeOwnership(this, InStack.Item);

		FInventoryEntry NewEntry { InStack.Item };

		// NextKey() is guaranteed to have a greater value than all currently existing keys, so simply appending is fine, and
		// will keep the EntryMap sorted.
		NewEntry.Key = KeyGen.NextKey();
		NewEntry.AddToNewStacks(InStack.Copies);

		EntryMap.AppendUnsafe(NewEntry);
		Event.EntryTouched = NewEntry.Key;
		Event.StackKeys = NewEntry.CopyKeys();
	}

	Event.Success = true;

	// Execute PostAddition on all extensions with the finished Event
	Extensions->PostAddition(this, Event);

	return Event;
}

Faerie::Inventory::FEventLog UFaerieItemStorage::RemoveFromEntryImpl(const FEntryKey Key, const int32 Amount,
																	 const FFaerieInventoryTag Reason)
{
	SCOPE_CYCLE_COUNTER(STAT_Storage_Remove);

	// RemoveEntryImpl should not be called with unvalidated parameters.
	check(Contains(Key));
	check(Faerie::ItemData::IsValidStack(Amount));
	check(Reason.MatchesTag(Faerie::Inventory::Tags::RemovalBase))

	Faerie::Inventory::FEventLog Event;

	Extensions->PreRemoval(this, Key, Amount);

	// Log for this event
	Event.Type = Reason;
	Event.EntryTouched = Key;

	bool Remove = false;

	// Open Mutable Scope
	{
		const FInventoryContent::FScopedItemHandle Handle = EntryMap.GetHandle(Key);

		Event.Item = Handle->GetItem();
		const int32 Sum = Handle->StackSum();

		if (Amount == Faerie::ItemData::UnlimitedStack || Amount >= Sum) // Remove the entire entry
		{
			Event.Amount = Sum;
			Event.StackKeys = Handle->CopyKeys();
			Faerie::ReleaseOwnership(this, Handle->GetItem());
			Remove = true;
		}
		else // Remove part of the entry
		{
			Event.Amount = FMath::Clamp(Amount, 1, Sum-1);
			Handle->RemoveFromAnyStack(Event.Amount, &Event.StackKeys);
		}
	}
	// Close Mutable scope

	if (Remove)
	{
		UE_LOG(LogFaerieInventory, Log, TEXT("Removing entire entry at: '%s'"), *Key.ToString());
		EntryMap.Remove(Key);
	}

	Event.Success = true;

	Extensions->PostRemoval(this, Event);

	return Event;
}

Faerie::Inventory::FEventLog UFaerieItemStorage::RemoveFromStackImpl(const FFaerieAddress Address, const int32 Amount,
																	 const FFaerieInventoryTag Reason)
{
	SCOPE_CYCLE_COUNTER(STAT_Storage_Remove);

	// RemoveFromStackImpl should not be called with unvalidated parameters.
	check(Contains(Address));
	check(Faerie::ItemData::IsValidStack(Amount));
	check(Reason.MatchesTag(Faerie::Inventory::Tags::RemovalBase))

	Faerie::Inventory::FEventLog Event;

	FEntryKey EntryKey;
	FStackKey StackKey;
	Decode(Address, EntryKey, StackKey);

	Extensions->PreRemoval(this, EntryKey, Amount);

	// Log for this event
	Event.Type = Reason;
	Event.EntryTouched = EntryKey;
	Event.StackKeys.Add(StackKey);

	bool Remove = false;

	// Open Mutable Scope
	{
		const FInventoryContent::FScopedItemHandle Handle = EntryMap.GetHandle(EntryKey);

		Event.Item = Handle->GetItem();

		if (auto&& Stack = Handle->GetStack(StackKey);
			Amount == Faerie::ItemData::UnlimitedStack || Amount >= Stack) // Remove the entire stack
		{
			Event.Amount = Stack;

			Handle->SetStack(StackKey, 0);

			if (Handle->GetStacks().IsEmpty())
			{
				Faerie::ReleaseOwnership(this, Handle->GetItem());
				Remove = true;
			}
		}
		else // Remove part of the stack
		{
			check(Amount == FMath::Clamp(Amount, 1, Stack-1));

			Event.Amount = Amount;

			auto&& NewAmount = Stack - Amount;
			Handle->SetStack(StackKey, NewAmount);
		}
	}
	// Close Mutable scope

	if (Remove)
	{
		UE_LOG(LogFaerieInventory, Log, TEXT("Removing entire stack at: '%s_%s'"), *EntryKey.ToString(), *StackKey.ToString());
		EntryMap.Remove(EntryKey);
	}

	// De-tally from total items.
	Event.Success = true;

	Extensions->PostRemoval(this, Event);

	return Event;
}


	/**------------------------------*/
	/*	 STORAGE API - ALL USERS   */
	/**------------------------------*/

UFaerieItemStorage::FStorageKey::FStorageKey(const FFaerieAddress& Address)
{
	Decode(Address, EntryKey, StackKey);
}

FFaerieAddress UFaerieItemStorage::FStorageKey::ToAddress() const
{
	return Encode(EntryKey, StackKey);
}

FEntryKey UFaerieItemStorage::FStorageKey::GetEntryKey(const FFaerieAddress FaerieAddress)
{
	FEntryKey Key;
	Decode_Entry(FaerieAddress, Key);
	return Key;
}

FStackKey UFaerieItemStorage::FStorageKey::GetStackKey(const FFaerieAddress FaerieAddress)
{
	FStackKey Key;
	Decode_Stack(FaerieAddress, Key);
	return Key;
}

void UFaerieItemStorage::BreakAddressIntoKeys(const FFaerieAddress Address, FEntryKey& Entry, FStackKey& Stack) const
{
	Decode(Address, Entry, Stack);
}

TArray<FStackKey> UFaerieItemStorage::BreakEntryIntoKeys(const FEntryKey Key) const
{
	if (const FInventoryEntry* Entry = GetEntrySafe(Key))
	{
		return Entry->CopyKeys();
	}
	return TArray<FStackKey>();
}

TArray<int32> UFaerieItemStorage::GetStacksInEntry(const FEntryKey Key) const
{
	if (const FInventoryEntry* Entry = GetEntrySafe(Key))
	{
		return Entry->CopyStacks();
	}
	return TArray<int32>();
}

void UFaerieItemStorage::GetAllAddresses(TArray<FFaerieAddress>& Addresses) const
{
	for (const FFaerieAddress Address : Faerie::AddressRange(this))
	{
		Addresses.Add(Address);
	}
}

TArray<FFaerieAddress> UFaerieItemStorage::GetAddressesForEntry(const FEntryKey Key) const
{
	TArray<FFaerieAddress> Out;

	if (const FInventoryEntry* Entry = GetEntrySafe(Key))
	{
		Out.Reserve(Entry->GetStacks().Num());
		for (const FKeyedStack& Stack : Entry->GetStacks())
		{
			Out.Add(Encode(Key, Stack.Key));
		}

		check(!Out.IsEmpty())
	}

	return Out;
}

void UFaerieItemStorage::GetAllKeys(TArray<FEntryKey>& Keys) const
{
	Keys = GetAllEntries();
}

int32 UFaerieItemStorage::GetEntryCount() const
{
	return EntryMap.Num();
}

int32 UFaerieItemStorage::GetStackCount() const
{
	int32 Stacks = 0;
	for (const FInventoryEntry& Entry : EntryMap)
	{
		Stacks += Entry.GetStacks().Num();
	}
	return Stacks;
}

bool UFaerieItemStorage::ContainsKey(const FEntryKey Key) const
{
	return Contains(Key);
}

bool UFaerieItemStorage::ContainsItem(const UFaerieItem* Item, const EFaerieItemEqualsCheck Method) const
{
	return FindItem(Item, Method).IsValid();
}

FEntryKey UFaerieItemStorage::FindItem(const UFaerieItem* Item, const EFaerieItemEqualsCheck Method) const
{
	if (!IsValid(Item))
	{
		return FEntryKey();
	}

	switch (Method)
	{
	case EFaerieItemEqualsCheck::ComparePointers:
		for (const FInventoryEntry& Entry : EntryMap)
		{
			if (Item == Entry.GetItem())
			{
				return Entry.Key;
			}
		}
		break;
	case EFaerieItemEqualsCheck::UseCompareWith:
		for (const FInventoryEntry& Entry : EntryMap)
		{
			if (Item->CompareWith(Entry.GetItem(), EFaerieItemComparisonFlags::Default))
			{
				return Entry.Key;
			}
		}
		break;
	}

	return FEntryKey();
}

FFaerieAddress UFaerieItemStorage::GetFirstAddress() const
{
	if (EntryMap.IsEmpty()) return FFaerieAddress();
	const FInventoryEntry& FirstEntry = EntryMap.Entries[0];
	return Encode(FirstEntry.Key, FirstEntry.GetStacks()[0].Key);
}

const UFaerieItem* UFaerieItemStorage::GetEntryItem(const FEntryKey Key) const
{
	if (const FInventoryEntry* Entry = GetEntrySafe(Key))
	{
		return Entry->GetItem();
	}
	return nullptr;
}

FFaerieAddress UFaerieItemStorage::QueryFirst(const Faerie::FStorageFilterFunc& Filter) const
{
	SCOPE_CYCLE_COUNTER(STAT_Storage_QueryFirst);

	FFaerieItemSnapshot Snap;
	Snap.Owner = this;

	for (const FInventoryEntry& Item : EntryMap)
	{
		Snap.ItemObject = Item.GetItem();

		for (auto&& Stack : Item.GetStacks())
		{
			Snap.Copies = Stack.Stack;

			if (const FFaerieAddress Address = Encode(Item.Key, Stack.Key);
				Filter(Snap))
			{
				return Address;
			}
		}
	}

	return FFaerieAddress();
}

void UFaerieItemStorage::QueryAll(const Faerie::FStorageQuery& Query, TArray<FFaerieAddress>& OutAddresses) const
{
	SCOPE_CYCLE_COUNTER(STAT_Storage_QueryAll);

	// Ensure we are starting with a blank slate.
	OutAddresses.Empty();

	if (!Query.Filter.IsBound() && !Query.Sort.IsBound())
	{
		if (!Query.InvertFilter)
		{
			for (const FFaerieAddress Address : Faerie::AddressRange(this))
			{
				OutAddresses.Add(Address);
			}

			if (Query.InvertSort)
			{
				Algo::Reverse(OutAddresses);
			}
		}
		return;
	}

	//Faerie::FItemStorageAddressFilter Res(this);
	Faerie::FItemStorageFilter_Key Res(this);

	{
		Faerie::FSnapshotFilterCallback CallbackFilter;
		CallbackFilter.Callback = Query.Filter;
		Res.Run(MoveTemp(CallbackFilter));
	}

	if (Query.InvertFilter)
	{
		Res.Invert();
	}

	if (Query.Sort.IsBound())
	{
		if (Query.InvertSort)
		{
			// @todo
			//Res.SortBySnapshot<Faerie::Backward>(Query.Sort);
		}
		else
		{
			// @todo
			//Res.SortBySnapshot(Query.Sort);
		}
	}

	OutAddresses = Res.EmitAddresses();
}

FFaerieAddress UFaerieItemStorage::QueryFirst(const FBlueprintStorageFilter& Filter) const
{
	if (!Filter.IsBound()) return FFaerieAddress();

	return QueryFirst(
		[Filter](const FFaerieItemSnapshot& Proxy)
		{
			return Filter.Execute(Proxy);
		});
}

void UFaerieItemStorage::QueryAll(const FFaerieItemStorageBlueprintQuery& Query, TArray<FFaerieAddress>& OutAddresses) const
{
	Faerie::FStorageQuery NativeQuery;
	if (Query.Filter.IsBound())
	{
		NativeQuery.Filter.BindLambda(
			[Filter = Query.Filter](const FFaerieItemSnapshot& Proxy)
			{
				return Filter.Execute(Proxy);
			});
	}

	if (Query.Sort.IsBound())
	{
		NativeQuery.Sort.BindLambda(
			[Sort = Query.Sort](const FFaerieItemSnapshot& A, const FFaerieItemSnapshot& B)
			{
				return Sort.Execute(A, B);
			});
	}

	NativeQuery.InvertFilter = Query.InvertFilter;
	NativeQuery.InvertSort = Query.ReverseSort;

	QueryAll(NativeQuery, OutAddresses);
}

bool UFaerieItemStorage::CanAddStack(const FFaerieItemStackView Stack, const EFaerieStorageAddStackBehavior AddStackBehavior) const
{
	if (!Stack.Item.IsValid() ||
		Stack.Copies < 1)
	{
		return false;
	}

	if (UFaerieItem* Mutable = Stack.Item->MutateCast())
	{
		// Prevent recursive storage for mutable items
		// @todo this only checks one layer of depth. Theoretically, these storage tokens could point to other ItemStorages,
		// which in turn has an item that points to us, which will crash the Extensions code when the item is possessed.
		// But honestly, I don't feel like fixing that unless it becomes a problem.
		const TSet<UFaerieItemContainerBase*> ContainerSet = UFaerieItemContainerToken::GetAllContainersInItem(Mutable);
		if (ContainerSet.Contains(this))
		{
			return false;
		}
	}

	const FFaerieExtensionAllowsAdditionArgs CanAddStackArgs { AddStackBehavior };

	switch (Extensions->AllowsAddition(this, MakeArrayView(&Stack, 1), CanAddStackArgs))
	{
	case EEventExtensionResponse::NoExplicitResponse:
	case EEventExtensionResponse::Allowed:				return true;
	case EEventExtensionResponse::Disallowed:			return false;
	default: return false;
	}
}

bool UFaerieItemStorage::CanAddStacks(const TArray<FFaerieItemStackView>& Stacks, const FFaerieExtensionAllowsAdditionArgs Args) const
{
	for (auto&& Stack : Stacks)
	{
		if (!Stack.Item.IsValid() ||
			Stack.Copies < 1)
		{
			return false;
		}

		if (UFaerieItem* Mutable = Stack.Item->MutateCast())
		{
			// Prevent recursive storage for mutable items
			// @todo this only checks one layer of depth. Theoretically, these storage tokens could point to other ItemStorages,
			// which in turn has an item that points to us, which will crash the Extensions code when the item is possessed.
			// But honestly, I don't feel like fixing that unless it becomes a problem.
			const TSet<UFaerieItemContainerBase*> ContainerSet = UFaerieItemContainerToken::GetAllContainersInItem(Mutable);
			if (ContainerSet.Contains(this))
			{
				return false;
			}
		}
	}

	switch (Extensions->AllowsAddition(this, Stacks, Args))
	{
	case EEventExtensionResponse::NoExplicitResponse:
	case EEventExtensionResponse::Allowed:				return true;
	case EEventExtensionResponse::Disallowed:			return false;
	default: return false;
	}
}

bool UFaerieItemStorage::CanEditEntry(const FEntryKey Key, const FFaerieInventoryTag EditTag) const
{
	// By default, some removal reasons are allowed, unless an extension explicitly disallows it.
	const bool DefaultAllowed = Faerie::Inventory::Tags::EditTagsAllowedByDefault().Contains(EditTag);

	switch (Extensions->AllowsEdit(this, Key, EditTag))
	{
	case EEventExtensionResponse::NoExplicitResponse:	return DefaultAllowed;
	case EEventExtensionResponse::Allowed:				return true;
	case EEventExtensionResponse::Disallowed:			return false;
	default: return false;
	}
}

bool UFaerieItemStorage::CanEditStack(const FFaerieAddress Address, const FFaerieInventoryTag EditTag) const
{
	FEntryKey Entry;
	Decode_Entry(Address, Entry);
	return CanEditEntry(Entry, EditTag);
}

bool UFaerieItemStorage::CanRemoveEntry(const FEntryKey Key, const FFaerieInventoryTag Reason) const
{
	// By default, some removal reasons are allowed, unless an extension explicitly disallows it.
	bool Allowed = Faerie::Inventory::Tags::RemovalTagsAllowedByDefault().Contains(Reason);

	for (const FFaerieAddress Address : Faerie::FStorageIterator_SingleEntry(this, Key))
	{
		switch (Extensions->AllowsRemoval(this, Address, Reason))
		{
		case EEventExtensionResponse::Disallowed:
			return false;
		case EEventExtensionResponse::Allowed:
			Allowed = true;
		case EEventExtensionResponse::NoExplicitResponse:
		default:
			break;
		}
	}

	return Allowed;
}

bool UFaerieItemStorage::CanRemoveStack(const FFaerieAddress Address, const FFaerieInventoryTag Reason) const
{
	// By default, some removal reasons are allowed, unless an extension explicitly disallows it.
	const bool DefaultAllowed = Faerie::Inventory::Tags::RemovalTagsAllowedByDefault().Contains(Reason);

	switch (Extensions->AllowsRemoval(this, Address, Reason))
	{
	case EEventExtensionResponse::NoExplicitResponse:	return DefaultAllowed;
	case EEventExtensionResponse::Allowed:				return true;
	case EEventExtensionResponse::Disallowed:			return false;
	default: return false;
	}
}


	/**---------------------------------*/
	/*	 STORAGE API - AUTHORITY ONLY   */
	/**---------------------------------*/

bool UFaerieItemStorage::AddEntryFromItemObject(const UFaerieItem* ItemObject, const EFaerieStorageAddStackBehavior AddStackBehavior)
{
	if (!CanAddStack({ItemObject, 1}, AddStackBehavior))
	{
		return false;
	}

	const FFaerieItemStack Stack{
		ItemObject,
		1
	};

	return AddStackImpl(Stack, IfOnlyNewStacks(AddStackBehavior)).Success;
}

bool UFaerieItemStorage::AddItemStack(const FFaerieItemStack& ItemStack, const EFaerieStorageAddStackBehavior AddStackBehavior)
{
	if (!CanAddStack(ItemStack, AddStackBehavior))
	{
		return false;
	}

	return AddStackImpl(ItemStack, IfOnlyNewStacks(AddStackBehavior)).Success;
}

void UFaerieItemStorage::AddItemStack(const FFaerieItemStack& ItemStack,
	const EFaerieStorageAddStackBehavior AddStackBehavior, Faerie::Inventory::FEventLog& OutLog)
{
	if (!CanAddStack(ItemStack, AddStackBehavior))
	{
		OutLog = Faerie::Inventory::FEventLog::AdditionFailed("Refused by CanAddStack");
		return;
	}

	OutLog = AddStackImpl(ItemStack, IfOnlyNewStacks(AddStackBehavior));
}

FLoggedInventoryEvent UFaerieItemStorage::AddItemStackWithLog(const FFaerieItemStack& ItemStack, const EFaerieStorageAddStackBehavior AddStackBehavior)
{
	Faerie::Inventory::FEventLog Log;
	AddItemStack(ItemStack, AddStackBehavior, Log);
	return {this, Log };
}

bool UFaerieItemStorage::RemoveEntry(const FEntryKey Key, const FFaerieInventoryTag RemovalTag, const int32 Amount)
{
	if (Amount == 0 || Amount < -1) return false;
	if (!Contains(Key)) return false;
	if (!CanRemoveEntry(Key, RemovalTag)) return false;

	return RemoveFromEntryImpl(Key, Amount, RemovalTag).Success;
}

bool UFaerieItemStorage::RemoveStack(const FFaerieAddress Address, const FFaerieInventoryTag RemovalTag, const int32 Amount)
{
	if (Amount == 0 || Amount < -1) return false;
	if (!Contains(Address)) return false;

	if (!RemovalTag.IsValid()) return false;

	if (!CanRemoveStack(Address, RemovalTag)) return false;

	return RemoveFromStackImpl(Address, Amount, RemovalTag).Success;
}

bool UFaerieItemStorage::TakeEntry(const FEntryKey Key, FFaerieItemStack& OutStack,
								   const FFaerieInventoryTag RemovalTag, const int32 Amount)
{
	if (Amount == 0 || Amount < -1) return false;
	if (!Contains(Key)) return false;

	if (!RemovalTag.IsValid()) return false;

	if (!CanRemoveEntry(Key, RemovalTag)) return false;

	auto&& Event = RemoveFromEntryImpl(Key, Amount, RemovalTag);

	if (Event.Success)
	{
		OutStack = { Event.Item.Get(), Event.Amount };
	}

	return Event.Success;
}

bool UFaerieItemStorage::TakeStack(const FFaerieAddress Address, FFaerieItemStack& OutStack,
								   const FFaerieInventoryTag RemovalTag, const int32 Amount)
{
	if (Amount == 0 || Amount < -1) return false;
	if (!Contains(Address)) return false;

	if (!RemovalTag.IsValid()) return false;

	if (!CanRemoveStack(Address, RemovalTag)) return false;

	auto&& Event = RemoveFromStackImpl(Address, Amount, RemovalTag);

	if (Event.Success)
	{
		OutStack = { Event.Item.Get(), Event.Amount };
	}

	return Event.Success;
}

void UFaerieItemStorage::Clear(FFaerieInventoryTag RemovalTag)
{
	if (!RemovalTag.IsValid())
	{
		RemovalTag = Faerie::Inventory::Tags::RemovalDeletion;
	}

	const TArray<FEntryKey> Entries = GetAllEntries();
	for (const FEntryKey Entry : Entries)
	{
		RemoveFromEntryImpl(Entry, Faerie::ItemData::UnlimitedStack, RemovalTag);
	}

	checkf(EntryMap.IsEmpty(), TEXT("Clear failed to empty EntryMap"));

#if WITH_EDITOR
	// The editor should reset the KeyGen, so that clearing and generating new content doesn't rack up the key endlessly.
	if (GEngine->IsEditor())
	{
		KeyGen.Reset();
	}

	// See Footnote1
#endif
}

FEntryKey UFaerieItemStorage::MoveStack(UFaerieItemStorage* ToStorage, const FFaerieAddress Address, const int32 Amount, const EFaerieStorageAddStackBehavior AddStackBehavior)
{
	if (!IsValid(ToStorage) ||
		ToStorage == this ||
		!Faerie::ItemData::IsValidStack(Amount) ||
		!CanRemoveStack(Address, Faerie::Inventory::Tags::RemovalMoving))
	{
		return FEntryKey::InvalidKey;
	}

	// Verify the stack exists.
	FEntryKey Entry;
	Decode_Entry(Address, Entry);
	const FInventoryEntry* EntryPtr = GetEntrySafe(Entry);
	if (!EntryPtr)
	{
		return FEntryKey::InvalidKey;
	}

	if (!CanRemoveStack(Address, Faerie::Inventory::Tags::RemovalMoving))
	{
		return FEntryKey::InvalidKey;
	}

	FStackKey Stack;
	Decode_Stack(Address, Stack);
	const int32 StackValue = EntryPtr->GetStack(Stack);
	if (0 >= StackValue)
	{
		return FEntryKey::InvalidKey;
	}

	FFaerieItemStackView View{ EntryPtr->GetItem(), StackValue };
	if (Amount > 0)
	{
		View.Copies = FMath::Min(View.Copies, Amount);
	}

	if (!ToStorage->CanAddStack(View, AddStackBehavior))
	{
		return FEntryKey::InvalidKey;
	}

	FFaerieItemStack ItemStack;
	if (!TakeStack(Address, ItemStack, Faerie::Inventory::Tags::RemovalMoving, Amount))
	{
		return FEntryKey::InvalidKey;
	}

	return ToStorage->AddStackImpl(ItemStack, IfOnlyNewStacks(AddStackBehavior)).EntryTouched;
}

FEntryKey UFaerieItemStorage::MoveEntry(UFaerieItemStorage* ToStorage, const FEntryKey Key, const EFaerieStorageAddStackBehavior AddStackBehavior)
{
	if (!IsValid(ToStorage) ||
		ToStorage == this)
	{
		return FEntryKey::InvalidKey;
	}

	// Verify the stack exists.
	const FInventoryEntry* EntryPtr = GetEntrySafe(Key);
	if (!EntryPtr)
	{
		return FEntryKey::InvalidKey;
	}

	if (!CanRemoveEntry(Key, Faerie::Inventory::Tags::RemovalMoving))
	{
		return FEntryKey::InvalidKey;
	}

	if (!ToStorage->CanAddStack(EntryPtr->ToItemStackView(), AddStackBehavior))
	{
		return FEntryKey::InvalidKey;
	}

	auto&& Result = RemoveFromEntryImpl(Key, Faerie::ItemData::UnlimitedStack, Faerie::Inventory::Tags::RemovalMoving);

	if (!ensure(Result.Success))
	{
		return FEntryKey::InvalidKey;
	}

	const FFaerieItemStack Stack { Result.Item.Get(), Result.Amount };
	return ToStorage->AddStackImpl(Stack, IfOnlyNewStacks(AddStackBehavior)).EntryTouched;
}

bool UFaerieItemStorage::MergeStacks(const FEntryKey Entry, const FStackKey FromStack, const FStackKey ToStack, const int32 Amount)
{
	const FFaerieAddress FromAddress = Encode(Entry, FromStack);
	const FFaerieAddress ToAddress = Encode(Entry, ToStack);

	// Verify the stack exists.
	const FInventoryEntry* EntryPtr = GetEntrySafe(Entry);
	if (!EntryPtr)
	{
		return false;
	}

	if (!CanEditStack(FromAddress, Faerie::Inventory::Tags::Merge) ||
		!CanEditStack(ToAddress, Faerie::Inventory::Tags::Merge))
	{
		return false;
	}

	const int32 AmountB = EntryPtr->GetStack(ToStack);

	// Ensure both stacks exist and B isn't already full
	if (EntryPtr->Contains(FromStack) ||
		AmountB != INDEX_NONE ||
		AmountB == EntryPtr->GetCachedStackLimit())
	{
		return false;
	}

	Faerie::Inventory::FEventLog Event;
	Event.Amount = AmountB; // Initially store the amount in stack B here.
	Event.Item = EntryPtr->GetItem();
	Event.EntryTouched = Entry;
	Event.StackKeys.Add(FromStack);
	Event.StackKeys.Add(ToStack);
	Event.Type = Faerie::Inventory::Tags::Merge;
	Event.Success = true;

	// Open Mutable Scope
	{
		const FInventoryContent::FScopedItemHandle Handle = EntryMap.GetHandle(Entry);
		const int32 Remainder = Handle->MoveStack(FromStack, ToStack, Amount);

		// We didn't move this many.
		Event.Amount -= Remainder;
	}
	// Close Mutable scope

	Extensions->PostEntryChanged(this, Event);

	return true;
}

bool UFaerieItemStorage::SplitStack(const FFaerieAddress Address, const int32 Amount)
{
	// Decode and verify the stack exists.
	FEntryKey Entry;
	FStackKey Stack;
	Decode(Address, Entry, Stack);
	const FInventoryEntry* EntryPtr = GetEntrySafe(Entry);
	if (!EntryPtr)
	{
		return false;
	}

	// Check if we can edit the amount requested
	if (!CanEditStack(Address, Faerie::Inventory::Tags::Split))
	{
		return false;
	}

	// Validate that the requested amount is less than what's in the stack
	if (Amount >= EntryPtr->GetStack(Stack))
	{
		return false;
	}

	Faerie::Inventory::FEventLog Event;
	Event.Item = EntryPtr->GetItem();
	Event.Amount = Amount;
	Event.EntryTouched = Entry;
	Event.StackKeys.Add(Stack);
	Event.Type = Faerie::Inventory::Tags::Split;
	Event.Success = true;

	// Split the stack
	{
		const FInventoryContent::FScopedItemHandle Handle = EntryMap.GetHandle(Entry);
		const FStackKey SplitStack = Handle->SplitStack(Stack, Amount);

		// Update event with final stack information
		Event.StackKeys.Add(SplitStack);
	}

	Extensions->PostEntryChanged(this, Event);

	return true;
}

void UFaerieItemStorage::Dump(UFaerieItemStorage* ToStorage)
{
	if (!IsValid(ToStorage) ||
		ToStorage == this)
	{
		return;
	}

	static constexpr EFaerieStorageAddStackBehavior DumpBehavior = EFaerieStorageAddStackBehavior::AddToAnyStack;

	const TArray<FEntryKey> Entries = GetAllEntries();
	for (const FEntryKey Entry : Entries)
	{
		if (!CanRemoveEntry(Entry, Faerie::Inventory::Tags::RemovalMoving))
		{
			continue;
		}

		const FFaerieItemStackView StackView = View(Entry);
		if (!ToStorage->CanAddStack(StackView, DumpBehavior))
		{
			continue;
		}

		Faerie::Inventory::FEventLog Result = RemoveFromEntryImpl(Entry,
			Faerie::ItemData::UnlimitedStack, Faerie::Inventory::Tags::RemovalMoving);

		if (!ensure(Result.Success))
		{
			continue;
		}

		const FFaerieItemStack Stack { Result.Item.Get(), Result.Amount };
		ToStorage->AddStackImpl(Stack, IfOnlyNewStacks(DumpBehavior)).EntryTouched;
	}
}


/*
 * Footnote1: You might think that even at runtime we could reset the key during Clear, since all items are removed,
 * and therefor no entries exist, making 100 a valid starting point again, *except* that other entities might still
 * be holding onto FEntryKeys, which could be cached in at some point later when potentially the entry is once more
 * valid, but with a completely different item. So during runtime, the Key must always increment.
 */