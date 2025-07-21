// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemStorage.h"
#include "FaerieInventorySettings.h"

#include "FaerieItem.h"
#include "InventoryStorageProxy.h"
#include "ItemContainerExtensionBase.h"
#include "Tokens/FaerieItemStorageToken.h"
#include "Tokens/FaerieStackLimiterToken.h"

#include "Net/UnrealNetwork.h"
#include "Providers/FlakesBinarySerializer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemStorage)

DECLARE_STATS_GROUP(TEXT("FaerieItemStorage"), STATGROUP_FaerieItemStorage, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Query (First)"), STAT_Storage_QueryFirst, STATGROUP_FaerieItemStorage);
DECLARE_CYCLE_STAT(TEXT("Query (All)"), STAT_Storage_QueryAll, STATGROUP_FaerieItemStorage);

DEFINE_LOG_CATEGORY(LogFaerieItemStorage);

bool IfOnlyNewStacks(const EFaerieStorageAddStackBehavior Behavior)
{
	return Behavior == EFaerieStorageAddStackBehavior::OnlyNewStacks;
};

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

FFaerieContainerSaveData UFaerieItemStorage::MakeSaveData() const
{
	ensureMsgf(GetDefault<UFaerieInventorySettings>()->ContainerMutableBehavior == EFaerieContainerOwnershipBehavior::Rename,
		TEXT("Flakes relies on ownership of sub-objects. Rename must be enabled! (ProjectSettings -> Faerie Inventory -> Container Mutable Behavior)"));

	FFaerieContainerSaveData SaveData;
	SaveData.ItemData = FInstancedStruct::Make(Flakes::MakeFlake<Flakes::Binary::Type>(FConstStructView::Make(EntryMap), this));
	RavelExtensionData(SaveData.ExtensionData);
	return SaveData;
}

void UFaerieItemStorage::LoadSaveData(const FFaerieContainerSaveData& SaveData)
{
	// Clear out state

	Clear(Faerie::Inventory::Tags::RemovalDeletion);
	KeyGen.Reset();

	Extensions->DeinitializeExtension(this);

	// Load in save data

	EntryMap = Flakes::CreateStruct<Flakes::Binary::Type, FInventoryContent>(SaveData.ItemData.Get<FFlake>(), this);
	TArray<FEntryKey, TInlineAllocator<4>> InvalidKeys;
	for (const FKeyedInventoryEntry& Entry : EntryMap)
	{
		if (!ValidateLoadedItem(Entry.Value.ItemObject))
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

	UnravelExtensionData(SaveData.ExtensionData);
}

bool UFaerieItemStorage::Contains(const FEntryKey Key) const
{
	return EntryMap.Contains(Key);
}

FFaerieItemStackView UFaerieItemStorage::View(const FEntryKey Key) const
{
	return EntryMap[Key].ToItemStackView();
}

FFaerieItemProxy UFaerieItemStorage::Proxy(const FEntryKey Key) const
{
	return GetEntryProxyImpl(Key);
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

void UFaerieItemStorage::ForEachKey(const TFunctionRef<void(FEntryKey)>& Func) const
{
	for (const FKeyedInventoryEntry& Element : EntryMap)
	{
		Func(Element.Key);
	}
}

int32 UFaerieItemStorage::GetStack(const FEntryKey Key) const
{
	if (!Contains(Key)) return 0;

	// Return the total items stored by this key, across all stacks, since this API doesn't know about stacks.
	return GetEntryViewImpl(Key).Get().StackSum();
}

TArray<FFaerieAddress> UFaerieItemStorage::Switchover_GetAddresses(const FEntryKey Key) const
{
	TArray<FFaerieAddress> Out;

	if (auto&& EntryPtr = EntryMap.Find(Key))
	{
		for (auto&& Stack : EntryPtr->Stacks)
		{
			Out.Add(Encode(Key, Stack.Key));
		}
	}

	return Out;
}

bool UFaerieItemStorage::Contains(const FFaerieAddress Address) const
{
	FEntryKey Entry;
	FStackKey Stack;
	Decode(Address, Entry, Stack);
	if (auto&& EntryPtr = EntryMap.Find(Entry))
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
	if (auto&& EntryPtr = EntryMap.Find(Entry))
	{
		return EntryPtr->GetStack(Stack);
	}
	return 0;
}

const UFaerieItem* UFaerieItemStorage::ViewItem(const FFaerieAddress Address) const
{
	FEntryKey Entry;
	FStackKey Stack;
	Decode(Address, Entry, Stack);
	if (auto&& EntryPtr = EntryMap.Find(Entry))
	{
		return EntryPtr->ItemObject;
	}
	return nullptr;
}

FFaerieItemStackView UFaerieItemStorage::ViewStack(const FFaerieAddress Address) const
{
	FEntryKey Entry;
	FStackKey Stack;
	Decode(Address, Entry, Stack);
	if (auto&& EntryPtr = EntryMap.Find(Entry))
	{
		if (const int32 StackValue = EntryPtr->GetStack(Stack);
			0 < StackValue)
		{
			return FFaerieItemStackView(EntryPtr->ItemObject, StackValue);
		}
	}
	return FFaerieItemStackView();
}

FFaerieItemProxy UFaerieItemStorage::Proxy(const FFaerieAddress Address) const
{
	return GetStackProxyImpl(FInventoryKey(Address));
}

FFaerieItemStack UFaerieItemStorage::Release(const FFaerieAddress Address, const int32 Copies)
{
	if (FFaerieItemStack OutStack;
		TakeStack(FInventoryKey(Address), OutStack, Faerie::Inventory::Tags::RemovalMoving, Copies))
	{
		return OutStack;
	}
	return FFaerieItemStack();
}

void UFaerieItemStorage::ForEachAddress(const TFunctionRef<void(FFaerieAddress)>& Func) const
{
	for (const FKeyedInventoryEntry& Element : EntryMap)
	{
		for (auto&& Stack : Element.Value.Stacks)
		{
			Func(Encode(Element.Key, Stack.Key));
		}
	}
}

void UFaerieItemStorage::ForEachItem(const TFunctionRef<void(const UFaerieItem*)>& Func) const
{
	for (const FKeyedInventoryEntry& Element : EntryMap)
	{
		Func(Element.Value.ItemObject);
	}
}

void UFaerieItemStorage::OnItemMutated(const UFaerieItem* Item, const UFaerieItemToken* Token, const FGameplayTag EditTag)
{
	Super::OnItemMutated(Item, Token, EditTag);

	// @todo annoying but acceptable
	for (const FKeyedInventoryEntry& Element : EntryMap)
	{
		if (Element.Value.ItemObject == Item)
		{
			PostContentChanged(Element, ItemMutation);
			return;
		}
	}
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

FFaerieAddress UFaerieItemStorage::Encode(const FEntryKey Entry, const FStackKey Stack)
{
	return FFaerieAddress((static_cast<int64>(Entry.Value()) << 32) | static_cast<int64>(Stack.Value()));
}

void UFaerieItemStorage::Decode(const FFaerieAddress Address, FEntryKey& Entry, FStackKey& Stack)
{
	constexpr int64 Mask = 0x00000000FFFFFFFF;
	Stack = FStackKey(Address.Address & Mask);
	Entry = FEntryKey(Address.Address >> 32);
}

void UFaerieItemStorage::PostContentAdded(const FKeyedInventoryEntry& Entry)
{
	if (!Entry.Key.IsValid())
	{
		UE_LOG(LogFaerieItemStorage, Warning, TEXT("PostContentAdded: Received Invalid Key"))
		return;
	}

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	OnKeyAddedCallback.Broadcast(this, Entry.Key);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	OnKeyAdded.Broadcast(this, Entry.Key);

	// Proxies may already exist for keys on the client if they are replicated by extensions or other means, and
	// happened to arrive before we got them.

	auto Addresses = Switchover_GetAddresses(Entry.Key);
	for (const FFaerieAddress& Address : Addresses)
	{
		OnAddressAddedCallback.Broadcast(this, Address);
	}

	if (auto&& EntryProxy = LocalEntryProxies.Find(Entry.Key))
	{
		if (EntryProxy->IsValid())
		{
			EntryProxy->Get()->NotifyCreation();
		}
	}

	for (auto&& Keys = GetInvKeysForEntry(Entry.Key);
		auto&& Key : Keys)
	{
		if (auto&& StackProxy = LocalStackProxies.Find(Key))
		{
			if (StackProxy->IsValid())
			{
				StackProxy->Get()->NotifyCreation();
			}
		}
	}
}

void UFaerieItemStorage::PostContentChanged(const FKeyedInventoryEntry& Entry, EContentChangeType ChangeType)
{
	if (!ensure(Entry.Key.IsValid()))
	{
		UE_LOG(LogFaerieItemStorage, Warning, TEXT("PostContentChanged: Received Invalid Key"))
		return;
	}

	if (!Entry.Value.IsValid())
	{
		return;
	}

	// Call updates on any entry and stack proxies
	if (Contains(Entry.Key))
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		OnKeyUpdatedCallback.Broadcast(this, Entry.Key);
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
		OnKeyUpdated.Broadcast(this, Entry.Key);

		switch (ChangeType)
		{
		case StackChange:
			{
				// @todo it's overkill to update all addresses for a stack change... but we dont know how to determine which stack actually changed, so blast them all :/
				auto Addresses = Switchover_GetAddresses(Entry.Key);
				for (const FFaerieAddress& Address : Addresses)
				{
					OnAddressUpdatedCallback.Broadcast(this, Address);
				}
			}
			break;
		case ItemMutation:
			{
				auto Addresses = Switchover_GetAddresses(Entry.Key);
				for (const FFaerieAddress& Address : Addresses)
				{
					OnAddressUpdatedCallback.Broadcast(this, Address);
				}
			}
			break;
		}

		// Call update on the entry proxy
		if (auto&& EntryProxy = LocalEntryProxies.Find(Entry.Key))
		{
			if (EntryProxy->IsValid())
			{
				EntryProxy->Get()->NotifyUpdate();
			}
		}

		// Call updates on any stack proxies.
		// PostContentChanged is called when stacks are removed as well, so let's do some cleanup here.
		// Start by getting all the Keys that we could have proxies for.
		const TSet<FInventoryKey> Keys(GetInvKeysForEntry(Entry.Key));
		for (auto It = LocalStackProxies.CreateIterator(); It; ++It)
		{
			auto&& LocalStackProxy = *It;

			// Check for local proxies that match this entry
			if (!LocalStackProxy.Value.IsValid() ||
				LocalStackProxy.Key.EntryKey != Entry.Key)
			{
				continue;
			}

			// If this is a key we are supposed to have update it.
			if (Keys.Contains(LocalStackProxy.Key))
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

void UFaerieItemStorage::PreContentRemoved(const FKeyedInventoryEntry& Entry)
{
	if (!Entry.Key.IsValid())
	{
		UE_LOG(LogFaerieItemStorage, Warning, TEXT("PreContentRemoved: Received Invalid Key"))
		return;
	}

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	OnKeyRemovedCallback.Broadcast(this, Entry.Key);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	OnKeyRemoved.Broadcast(this, Entry.Key);

	auto Addresses = Switchover_GetAddresses(Entry.Key);
	for (const FFaerieAddress& Address : Addresses)
	{
		OnAddressRemovedCallback.Broadcast(this, Address);
	}

	// Cleanup local views.

	TWeakObjectPtr<UInventoryEntryProxy> EntryProxy;
	LocalEntryProxies.RemoveAndCopyValue(Entry.Key, EntryProxy);
	if (EntryProxy.IsValid())
	{
		EntryProxy->NotifyRemoval();
	}

	for (auto&& Stack : Entry.Value.Stacks)
	{
		TWeakObjectPtr<UInventoryStackProxy> StackProxy;
		LocalStackProxies.RemoveAndCopyValue({Entry.Key, Stack.Key}, StackProxy);
		if (StackProxy.IsValid())
		{
			StackProxy->NotifyRemoval();
		}
	}
}


	/**------------------------------*/
	/*	  INTERNAL IMPLEMENTATIONS	 */
	/**------------------------------*/

TConstStructView<FInventoryEntry> UFaerieItemStorage::GetEntryViewImpl(const FEntryKey Key) const
{
	return TConstStructView<FInventoryEntry>(EntryMap[Key]);
}

UInventoryEntryProxy* UFaerieItemStorage::GetEntryProxyImpl(const FEntryKey Key) const
{
	// Don't create proxies for invalid keys.
	if (!Key.IsValid()) return nullptr;

	if (auto&& ExistingProxy = LocalEntryProxies.Find(Key))
	{
		if (ExistingProxy && ExistingProxy->IsValid())
		{
			return ExistingProxy->Get();
		}
	}

	ThisClass* This = const_cast<ThisClass*>(this);

	const FName ProxyName = MakeUniqueObjectName(This, UInventoryEntryProxy::StaticClass(),
												 *FString::Printf(TEXT("ENTRY_PROXY_%s"), *Key.ToString()));
	UInventoryEntryProxy* NewEntryProxy = NewObject<UInventoryEntryProxy>(This, UInventoryEntryProxy::StaticClass(),
																	ProxyName);
	check(IsValid(NewEntryProxy));

	NewEntryProxy->Key = Key;
	NewEntryProxy->ItemStorage = This;

	if (Contains(Key))
	{
		NewEntryProxy->NotifyCreation();
	}

	This->LocalEntryProxies.Add(Key, NewEntryProxy);

	return NewEntryProxy;
}

UInventoryStackProxy* UFaerieItemStorage::GetStackProxyImpl(const FInventoryKey Key) const
{
	// Don't create proxies for invalid keys.
	if (!Key.IsValid()) return nullptr;

	if (auto&& ExistingProxy = LocalStackProxies.Find(Key))
	{
		if (ExistingProxy && ExistingProxy->IsValid())
		{
			return ExistingProxy->Get();
		}
	}

	ThisClass* This = const_cast<ThisClass*>(this);

	const FName ProxyName = MakeUniqueObjectName(This, UInventoryStackProxy::StaticClass(),
												 *FString::Printf(TEXT("STACK_PROXY_%s_%s"),
												 *Key.EntryKey.ToString(), *Key.StackKey.ToString()));
	UInventoryStackProxy* NewEntryProxy = NewObject<UInventoryStackProxy>(This, UInventoryStackProxy::StaticClass(), ProxyName);
	check(IsValid(NewEntryProxy));

	NewEntryProxy->ItemStorage = This;
	NewEntryProxy->Key = Key;

	if (IsValidKey(Key))
	{
		NewEntryProxy->NotifyCreation();
	}

	This->LocalStackProxies.Add(Key, NewEntryProxy);

	return NewEntryProxy;
}


void UFaerieItemStorage::GetEntryImpl(const FEntryKey Key, FInventoryEntry& Entry) const
{
	check(Contains(Key))
	Entry = EntryMap[Key];
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

	Faerie::Inventory::FEventLog Event;

	// Setup Log for this event
	Event.Type = Faerie::Inventory::Tags::Addition;
	Event.Item = InStack.Item;
	Event.Amount = InStack.Copies;

	// Mutables cannot stack, due to, well, being mutable, meaning that each individual retains the ability to
	// uniquely mutate from others.
	if (!InStack.Item->CanMutate())
	{
		Event.EntryTouched = QueryFirst(
			[InStack](const FFaerieItemProxy& Other)
			{
				return InStack.Item->CompareWith(Other.GetItemObject(), EFaerieItemComparisonFlags::Default);
			}).Key;
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
		// NextKey() is guaranteed to have a greater value than all currently existing keys, so simply appending is fine, and
		// will keep the EntryMap sorted.
		Event.EntryTouched = KeyGen.NextKey();

		TakeOwnership(InStack.Item);

		FInventoryEntry NewEntry;
		NewEntry.ItemObject = InStack.Item;
		NewEntry.Limit = UFaerieStackLimiterToken::GetItemStackLimit(NewEntry.ItemObject);
		NewEntry.AddToNewStacks(InStack.Copies);

		/*FKeyedInventoryEntry& AddedEntry =*/ EntryMap.Append(Event.EntryTouched, NewEntry);
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

		Event.Item = Handle->ItemObject;
		const int32 Sum = Handle->StackSum();

		if (Amount == Faerie::ItemData::UnlimitedStack || Amount >= Sum) // Remove the entire entry
		{
			Event.Amount = Sum;
			Event.StackKeys = Handle->CopyKeys();
			ReleaseOwnership(Handle->ItemObject);
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
		UE_LOG(LogFaerieItemStorage, Log, TEXT("Removing entire entry at: '%s'"), *Key.ToString());
		EntryMap.Remove(Key);
	}

	Event.Success = true;

	Extensions->PostRemoval(this, Event);

	return Event;
}

Faerie::Inventory::FEventLog UFaerieItemStorage::RemoveFromStackImpl(const FInventoryKey Key, const int32 Amount,
																	 const FFaerieInventoryTag Reason)
{
	// RemoveEntryImpl should not be called with unvalidated parameters.
	check(Faerie::ItemData::IsValidStack(Amount));
	check(Contains(Key.EntryKey));
	check(Reason.MatchesTag(Faerie::Inventory::Tags::RemovalBase))

	Faerie::Inventory::FEventLog Event;

	Extensions->PreRemoval(this, Key.EntryKey, Amount);

	// Log for this event
	Event.Type = Reason;
	Event.EntryTouched = Key.EntryKey;
	Event.StackKeys.Add(Key.StackKey);

	bool Remove = false;

	// Open Mutable Scope
	{
		const FInventoryContent::FScopedItemHandle Handle = EntryMap.GetHandle(Key.EntryKey);

		Event.Item = Handle->ItemObject;

		if (auto&& Stack = Handle->GetStack(Key.StackKey);
			Amount == Faerie::ItemData::UnlimitedStack || Amount >= Stack) // Remove the entire stack
		{
			Event.Amount = Stack;

			Handle->SetStack(Key.StackKey, 0);

			if (Handle->Stacks.IsEmpty())
			{
				ReleaseOwnership(Handle->ItemObject);
				Remove = true;
			}
		}
		else // Remove part of the stack
		{
			check(Amount == FMath::Clamp(Amount, 1, Stack-1));

			Event.Amount = Amount;

			auto&& NewAmount = Stack - Amount;
			Handle->SetStack(Key.StackKey, NewAmount);
		}
	}
	// Close Mutable scope

	if (Remove)
	{
		UE_LOG(LogFaerieItemStorage, Log, TEXT("Removing entire stack at: '%s'"), *Key.ToString());
		EntryMap.Remove(Key.EntryKey);
	}

	// De-tally from total items.
	Event.Success = true;

	Extensions->PostRemoval(this, Event);

	return Event;
}


	/**------------------------------*/
	/*	 STORAGE API - ALL USERS   */
	/**------------------------------*/

TConstStructView<FInventoryEntry> UFaerieItemStorage::GetEntryView(const FEntryKey Key) const
{
	return GetEntryViewImpl(Key);
}

FFaerieItemStackView UFaerieItemStorage::GetStackView(const FInventoryKey Key) const
{
	if (const TConstStructView<FInventoryEntry> EntryView = GetEntryViewImpl(Key.EntryKey);
		EntryView.IsValid())
	{
		FFaerieItemStackView View;
		View.Item = EntryView.Get().ItemObject;
		View.Copies = EntryView.Get().GetStack(Key.StackKey);
		return View;
	}
	return FFaerieItemStackView();
}

TArray<FInventoryKey> UFaerieItemStorage::GetInvKeysForEntry(const FEntryKey Key) const
{
	TArray<FInventoryKey> Out;

	if (!Contains(Key)) return Out;

	const TConstStructView<FInventoryEntry> Entry = GetEntryViewImpl(Key);
	Out.Reserve(Entry.Get().Stacks.Num());
	for (const FKeyedStack& Stack : Entry.Get().Stacks)
	{
		Out.Add({Key, Stack.Key});
	}

	check(!Out.IsEmpty())

	return Out;
}

void UFaerieItemStorage::GetAllKeys(TArray<FEntryKey>& Keys) const
{
	Keys.Empty(EntryMap.Num());
	Algo::Transform(EntryMap, Keys, &FKeyedInventoryEntry::Key);
}

int32 UFaerieItemStorage::GetStackCount() const
{
	return EntryMap.Num();
}

bool UFaerieItemStorage::ContainsKey(const FEntryKey Key) const
{
	return Contains(Key);
}

bool UFaerieItemStorage::IsValidKey(const FInventoryKey Key) const
{
	if (!Contains(Key.EntryKey)) return false;
	return GetEntryViewImpl(Key.EntryKey).Get().Contains(Key.StackKey);
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
		for (const FKeyedInventoryEntry& Entry : EntryMap)
		{
			if (Entry.Value.ItemObject == Item)
			{
				return Entry.Key;
			}
		}
		break;
	case EFaerieItemEqualsCheck::UseCompareWith:
		for (const FKeyedInventoryEntry& Entry : EntryMap)
		{
			if (Entry.Value.ItemObject->CompareWith(Item, EFaerieItemComparisonFlags::Default))
			{
				return Entry.Key;
			}
		}
		break;
	}

	return FEntryKey();
}

FInventoryKey UFaerieItemStorage::GetFirstKey() const
{
	if (EntryMap.IsEmpty()) return FInventoryKey();
	auto&& FirstEntry = EntryMap.Entries[0];

	return { FirstEntry.Key, FirstEntry.Value.Stacks[0].Key };
}

bool UFaerieItemStorage::GetEntry(const FEntryKey Key, FInventoryEntry& Entry) const
{
	if (!Contains(Key)) return false;
	GetEntryImpl(Key, Entry);
	return true;
}

UInventoryEntryProxy* UFaerieItemStorage::GetEntryProxy(const FEntryKey Key) const
{
	return GetEntryProxyImpl(Key);
}

UInventoryStackProxy* UFaerieItemStorage::GetStackProxy_New(const FInventoryKey Key) const
{
	return GetStackProxyImpl(Key);
}

bool UFaerieItemStorage::GetStackProxy(const FInventoryKey Key, FFaerieItemProxy& Proxy)
{
	if (UInventoryStackProxy* StackProxy = GetStackProxy_New(Key))
	{
		Proxy = {StackProxy};
		return Proxy.IsValid();
	}
	return false;
}

void UFaerieItemStorage::GetEntryArray(const TArray<FEntryKey>& Keys, TArray<FInventoryEntry>& Entries) const
{
	// Allocate memory once
	Entries.SetNumUninitialized(Keys.Num(), EAllowShrinking::Yes);
	for (int32 i = 0; i < Keys.Num(); ++i)
	{
		GetEntryImpl(Keys[i], Entries[i]);
	}
}

FKeyedInventoryEntry UFaerieItemStorage::QueryFirst(const Faerie::FStorageFilterFunc& Filter) const
{
	SCOPE_CYCLE_COUNTER(STAT_Storage_QueryFirst);

	for (const FKeyedInventoryEntry& Item : EntryMap)
	{
		if (Filter(Proxy(Item.Key)))
		{
			return Item;
		}
	}

	return FKeyedInventoryEntry();
}

void UFaerieItemStorage::QueryAll(const Faerie::FStorageQuery& Query, TArray<FKeyedInventoryEntry>& OutKeys) const
{
	SCOPE_CYCLE_COUNTER(STAT_Storage_QueryAll);

	// Ensure we are starting with a blank slate.
	OutKeys.Empty();

	if (Query.Filter.IsBound())
	{
		if (Query.InvertFilter)
		{
			Algo::CopyIf(EntryMap, OutKeys,
				[&Query, this](const FKeyedInventoryEntry& Item)
				{
					return !Query.Filter.Execute(Proxy(Item.Key));
				});
		}
		else
		{
			Algo::CopyIf(EntryMap, OutKeys,
				[&Query, this](const FKeyedInventoryEntry& Item)
				{
					return Query.Filter.Execute(Proxy(Item.Key));
				});
		}
	}
	else
	{
		OutKeys = EntryMap.Entries;
	}

	if (Query.Sort.IsBound())
	{
		if (Query.InvertSort)
		{
			Algo::Sort(OutKeys,
				[&, Sort = Query.Sort](const FKeyedInventoryEntry& A, const FKeyedInventoryEntry& B)
				{
					return !Sort.Execute(Proxy(A.Key), Proxy(B.Key));
				});
		}
		else
		{
			Algo::Sort(OutKeys,
				[&, Sort = Query.Sort](const FKeyedInventoryEntry& A, const FKeyedInventoryEntry& B)
				{
					return Sort.Execute(Proxy(A.Key), Proxy(B.Key));
				});
		}
	}
}

void UFaerieItemStorage::QueryAll(const Faerie::FStorageQuery& Query,
	TArray<FFaerieAddress>& OutAddresses) const
{
	SCOPE_CYCLE_COUNTER(STAT_Storage_QueryAll);

	// Ensure we are starting with a blank slate.
	OutAddresses.Empty();

	if (Query.Filter.IsBound())
	{
		if (Query.InvertFilter)
		{
			ForEachAddress([&](const FFaerieAddress A)
				{
					if (!Query.Filter.Execute(Proxy(A)))
					{
						OutAddresses.Add(A);
					}
				});
		}
		else
		{
			ForEachAddress([&](const FFaerieAddress A)
				{
					if (Query.Filter.Execute(Proxy(A)))
					{
						OutAddresses.Add(A);
					}
				});
		}
	}
	else
	{
		ForEachAddress([&](const FFaerieAddress A)
			{
				OutAddresses.Add(A);
			});
	}

	if (Query.Sort.IsBound())
	{
		if (Query.InvertSort)
		{
			Algo::Sort(OutAddresses,
				[&, Sort = Query.Sort](const FFaerieAddress& A, const FFaerieAddress& B)
				{
					return !Sort.Execute(Proxy(A), Proxy(B));
				});
		}
		else
		{
			Algo::Sort(OutAddresses,
				[&, Sort = Query.Sort](const FFaerieAddress& A, const FFaerieAddress& B)
				{
					return Sort.Execute(Proxy(A), Proxy(B));
				});
		}
	}
}

FEntryKey UFaerieItemStorage::QueryFirst(const FBlueprintStorageFilter& Filter) const
{
	if (!Filter.IsBound()) return FEntryKey();

	return QueryFirst(
		[Filter](const FFaerieItemProxy& Proxy)
		{
			return Filter.Execute(Proxy);
		}).Key;
}

void UFaerieItemStorage::QueryAll(const FFaerieItemStorageBlueprintQuery& Query, TArray<FEntryKey>& OutKeys) const
{
	Faerie::FStorageQuery NativeQuery;
	if (Query.Filter.IsBound())
	{
		NativeQuery.Filter.BindLambda(
			[Filter = Query.Filter](const FFaerieItemProxy& Proxy)
			{
				return Filter.Execute(Proxy);
			});
		NativeQuery.InvertFilter = Query.InvertFilter;
	}

	if (Query.Sort.IsBound())
	{
		NativeQuery.Sort.BindLambda(
			[Sort = Query.Sort](const FFaerieItemProxy& A, const FFaerieItemProxy& B)
			{
				return Sort.Execute(A, B);
			});
		NativeQuery.InvertSort = Query.Reverse;
	}

	TArray<FKeyedInventoryEntry> Entries;
	QueryAll(NativeQuery, Entries);
	OutKeys.Reserve(Entries.Num());
	Algo::Transform(Entries, OutKeys, &FKeyedInventoryEntry::Key);
}

bool UFaerieItemStorage::CanAddStack(const FFaerieItemStackView Stack, const EFaerieStorageAddStackBehavior AddStackBehavior) const
{
	if (!Stack.Item.IsValid() ||
		Stack.Copies < 1)
	{
		return false;
	}

	if (Stack.Item->CanMutate())
	{
		// Prevent recursive storage for mutable items
		// @todo this only checks one layer of depth. Theoretically, these storage tokens could point to other ItemStorages,
		// which in turn has an item that points to us, which will crash the Extensions code when the item is possessed.
		// But honestly, I don't feel like fixing that unless it becomes a problem.
		const TSet<UFaerieItemContainerBase*> ContainerSet = UFaerieItemContainerToken::GetAllContainersInItem(Stack.Item.Get());
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

		if (Stack.Item->CanMutate())
		{
			// Prevent recursive storage for mutable items
			// @todo this only checks one layer of depth. Theoretically, these storage tokens could point to other ItemStorages,
			// which in turn has an item that points to us, which will crash the Extensions code when the item is possessed.
			// But honestly, I don't feel like fixing that unless it becomes a problem.
			const TSet<UFaerieItemContainerBase*> ContainerSet = UFaerieItemContainerToken::GetAllContainersInItem(Stack.Item.Get());
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

bool UFaerieItemStorage::CanEditStack(const FInventoryKey StackKey, const FFaerieInventoryTag EditTag) const
{
	return CanEditEntry(StackKey.EntryKey, EditTag);
}

bool UFaerieItemStorage::CanRemoveEntry(const FEntryKey Key, const FFaerieInventoryTag Reason) const
{
	// By default, some removal reasons are allowed, unless an extension explicitly disallows it.
	const bool DefaultAllowed = Faerie::Inventory::Tags::RemovalTagsAllowedByDefault().Contains(Reason);

	switch (Extensions->AllowsRemoval(this, Key, Reason))
	{
	case EEventExtensionResponse::NoExplicitResponse:	return DefaultAllowed;
	case EEventExtensionResponse::Allowed:				return true;
	case EEventExtensionResponse::Disallowed:			return false;
	default: return false;
	}
}

bool UFaerieItemStorage::CanRemoveStack(const FInventoryKey Key, const FFaerieInventoryTag Reason) const
{
	return CanRemoveEntry(Key.EntryKey, Reason);
}


	/**---------------------------------*/
	/*	 STORAGE API - AUTHORITY ONLY   */
	/**---------------------------------*/

bool UFaerieItemStorage::AddEntryFromItemObject(UFaerieItem* ItemObject, const EFaerieStorageAddStackBehavior AddStackBehavior)
{
	if (!CanAddStack({ItemObject, 1}, AddStackBehavior))
	{
		return false;
	}

	FFaerieItemStack Stack;
	Stack.Item = ItemObject;
	Stack.Copies = 1;

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

FLoggedInventoryEvent UFaerieItemStorage::AddItemStackWithLog(const FFaerieItemStack& ItemStack, const EFaerieStorageAddStackBehavior AddStackBehavior)
{
	if (!CanAddStack(ItemStack, AddStackBehavior))
	{
		return { this, Faerie::Inventory::FEventLog::AdditionFailed("Refused by CanAddStack") };
	}

	return {this, AddStackImpl(ItemStack, IfOnlyNewStacks(AddStackBehavior)) };
}

bool UFaerieItemStorage::RemoveEntry(const FEntryKey Key, const FFaerieInventoryTag RemovalTag, const int32 Amount)
{
	if (Amount == 0 || Amount < -1) return false;
	if (!Contains(Key)) return false;
	if (!CanRemoveEntry(Key, RemovalTag)) return false;

	return RemoveFromEntryImpl(Key, Amount, RemovalTag).Success;
}

bool UFaerieItemStorage::RemoveStack(const FInventoryKey Key, const FFaerieInventoryTag RemovalTag, const int32 Amount)
{
	if (Amount == 0 || Amount < -1) return false;
	if (!Contains(Key.EntryKey)) return false;

	if (!RemovalTag.IsValid()) return false;

	if (!CanRemoveStack(Key, RemovalTag)) return false;

	return RemoveFromStackImpl(Key, Amount, RemovalTag).Success;
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

bool UFaerieItemStorage::TakeStack(const FInventoryKey Key, FFaerieItemStack& OutStack,
								   const FFaerieInventoryTag RemovalTag, const int32 Amount)
{
	if (Amount == 0 || Amount < -1) return false;
	if (!Contains(Key.EntryKey)) return false;

	if (!RemovalTag.IsValid()) return false;

	if (!CanRemoveStack(Key, RemovalTag)) return false;

	auto&& Event = RemoveFromStackImpl(Key, Amount, RemovalTag);

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

	for (const FKeyedInventoryEntry& Element : EntryMap)
	{
		RemoveFromEntryImpl(Element.Key, Faerie::ItemData::UnlimitedStack, RemovalTag);
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

FEntryKey UFaerieItemStorage::MoveStack(UFaerieItemStorage* ToStorage, const FInventoryKey Key, const int32 Amount, const EFaerieStorageAddStackBehavior AddStackBehavior)
{
	if (!IsValid(ToStorage) ||
		ToStorage == this ||
		!Contains(Key.EntryKey) ||
		!Faerie::ItemData::IsValidStack(Amount) ||
		!CanRemoveStack(Key, Faerie::Inventory::Tags::RemovalMoving))
	{
		return FEntryKey::InvalidKey;
	}

	FFaerieItemStackView View = GetStackView(Key);
	if (Amount > 0)
	{
		View.Copies = FMath::Min(View.Copies, Amount);
	}

	if (!ToStorage->CanAddStack(View, AddStackBehavior))
	{
		return FEntryKey::InvalidKey;
	}

	FFaerieItemStack ItemStack;
	if (!TakeStack(Key, ItemStack, Faerie::Inventory::Tags::RemovalMoving, Amount))
	{
		return FEntryKey::InvalidKey;
	}

	return ToStorage->AddStackImpl(ItemStack, IfOnlyNewStacks(AddStackBehavior)).EntryTouched;
}

FEntryKey UFaerieItemStorage::MoveEntry(UFaerieItemStorage* ToStorage, const FEntryKey Key, const EFaerieStorageAddStackBehavior AddStackBehavior)
{
	if (!IsValid(ToStorage) ||
		ToStorage == this ||
		!Contains(Key) ||
		!CanRemoveEntry(Key, Faerie::Inventory::Tags::RemovalMoving))
	{
		return FEntryKey::InvalidKey;
	}

	const TConstStructView<FInventoryEntry> EntryView = GetEntryViewImpl(Key);
	if (!ensure(EntryView.IsValid()))
	{
		return FEntryKey::InvalidKey;
	}

	if (!ToStorage->CanAddStack(EntryView.Get().ToItemStackView(), AddStackBehavior))
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
	if (!Contains(Entry) ||
		!CanEditStack({Entry, FromStack}, Faerie::Inventory::Tags::Merge) ||
		!CanEditStack({Entry, ToStack}, Faerie::Inventory::Tags::Merge))
	{
		return false;
	}

	auto&& EntryView = GetEntryViewImpl(Entry);
	const int32 AmountB = EntryView.Get().GetStack(ToStack);

	// Ensure both stacks exist and B isn't already full
	if (EntryView.Get().Contains(FromStack) ||
		AmountB != INDEX_NONE ||
		AmountB == EntryView.Get().Limit)
	{
		return false;
	}

	Faerie::Inventory::FEventLog Event;
	Event.Amount = AmountB; // Initially store the amount in stack B here.
	Event.Item = EntryView.Get().ItemObject;
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

bool UFaerieItemStorage::SplitStack(const FEntryKey Entry, const FStackKey Stack, const int32 Amount)
{
	if (!Contains(Entry) ||
		!CanEditStack({Entry, Stack}, Faerie::Inventory::Tags::Split))
	{
		return false;
	}

	auto&& EntryView = GetEntryViewImpl(Entry);
	const int32 StackAmount = EntryView.Get().GetStack(Stack);

	// Validate that the requested amount is less than what's in the stack
	if (Amount >= StackAmount)
	{
		return false;
	}

	Faerie::Inventory::FEventLog Event;
	Event.Item = EntryView.Get().ItemObject;
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
	// @todo this is not very optimized...

	if (!IsValid(ToStorage) ||
		ToStorage == this)
	{
		return;
	}

	for (const FKeyedInventoryEntry& Element : EntryMap)
	{
		MoveEntry(ToStorage, Element.Key, EFaerieStorageAddStackBehavior::AddToAnyStack);
	}
}


/*
 * Footnote1: You might think that even at runtime we could reset the key during Clear, since all items are removed,
 * and therefor no entries exist, making 100 a valid starting point again, *except* that other entities might still
 * be holding onto FEntryKeys, which could be cached in at some point later when potentially the entry is once more
 * valid, but with a completely different item. So during runtime, the Key must always increment.
 */