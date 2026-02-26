// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemStorage.h"
#include "FaerieInventoryLog.h"
#include "FaerieInventorySettings.h"
#include "FaerieItem.h"
#include "FaerieItemStorageIterators.h"
#include "FaerieItemStorageStatics.h"
#include "FaerieSubObjectFilter.h"
#include "ItemStackProxy.h"
#include "ItemContainerExtensionBase.h"

#include "Algo/Transform.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"

#if WITH_EDITOR
#include "Engine/Engine.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemStorage)

DECLARE_STATS_GROUP(TEXT("FaerieItemStorage"), STATGROUP_FaerieItemStorage, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Add to Storage"), STAT_Storage_Add, STATGROUP_FaerieItemStorage);
DECLARE_CYCLE_STAT(TEXT("Add to Storage (multi)"), STAT_Storage_AddMulti, STATGROUP_FaerieItemStorage);
DECLARE_CYCLE_STAT(TEXT("Remove from Storage"), STAT_Storage_Remove, STATGROUP_FaerieItemStorage);
DECLARE_CYCLE_STAT(TEXT("Remove from Storage (multi)"), STAT_Storage_RemoveMulti, STATGROUP_FaerieItemStorage);

#define LOCTEXT_NAMESPACE "FaerieItemStorage"

using namespace Faerie;

namespace Faerie::Storage
{
	namespace Address
	{
		[[nodiscard]] UE_REWRITE FFaerieAddress Encode(const FEntryKey Entry, const FStackKey Stack)
		{
			return FFaerieAddress((static_cast<int64>(Entry.Value()) << 32) | static_cast<int64>(Stack.Value()));
		}

		UE_REWRITE void Decode(const FFaerieAddress Address, FEntryKey& Entry, FStackKey& Stack)
		{
			constexpr int64 Mask = 0x00000000FFFFFFFF;
			Stack = FStackKey(Address.Address & Mask);
			Entry = FEntryKey(Address.Address >> 32);
		}

		UE_REWRITE void Decode_Entry(const FFaerieAddress Address, FEntryKey& Entry)
		{
			Entry = FEntryKey(Address.Address >> 32);
		}

		UE_REWRITE void Decode_Stack(const FFaerieAddress Address, FStackKey& Stack)
		{
			constexpr int64 Mask = 0x00000000FFFFFFFF;
			Stack = FStackKey(Address.Address & Mask);
		}
	}

	constexpr bool IfOnlyNewStacks(const EFaerieStorageAddStackBehavior Behavior)
	{
		return Behavior == EFaerieStorageAddStackBehavior::OnlyNewStacks;
	}

	static const FText AdditionFailure_FailedCanAddStack = LOCTEXT("AdditionFailure_FailedCanAddStack", "Refused by CanAddStack");
}

void UFaerieItemStorage::PostInitProperties()
{
	Super::PostInitProperties();

	// Bind replication functions out into this class.
	EntryMap.ChangeListener = this;
}

void UFaerieItemStorage::PostDuplicate(const EDuplicateMode::Type DuplicateMode)
{
	Super::PostDuplicate(DuplicateMode);

	// Rebind replication functions out into this class.
	EntryMap.ChangeListener = this;
}

void UFaerieItemStorage::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, EntryMap, SharedParams)
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

	Clear(Inventory::Tags::RemovalDeletion);
	KeyGen.Reset();

	Extensions->DeinitializeExtension(this);

	// Load in save data
	EntryMap = ItemData.Get<const FInventoryContent>();
	TArray<FEntryKey, TInlineAllocator<4>> InvalidKeys;
	for (const FInventoryEntry& Entry : EntryMap)
	{
		if (!ItemData::ValidateItemData(Entry.GetItem()))
		{
			InvalidKeys.Add(Entry.GetKey());
		}
	}

	for (const FEntryKey InvalidKey : InvalidKeys)
	{
		EntryMap.Remove(InvalidKey);
	}

	EntryMap.MarkArrayDirty();

	// Rebind replication functions out into this class.
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
		TakeEntry(Key, OutStack, Inventory::Tags::RemovalMoving, Copies))
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

void UFaerieItemStorage::GetAllAddresses(TArray<FFaerieAddress>& Addresses) const
{
	Addresses.Reset(Algo::TransformAccumulate(EntryMap, &FInventoryEntry::NumStacks, 0));
	for (auto&& Entry : EntryMap)
	{
		for (auto&& Stack : Entry.GetStacks())
		{
			Addresses.Add(Storage::Address::Encode(Entry.GetKey(), Stack.Key));
		}
	}
}

bool UFaerieItemStorage::Contains(const FFaerieAddress Address) const
{
	FEntryKey Entry;
	FStackKey Stack;
	Storage::Address::Decode(Address, Entry, Stack);
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
	Storage::Address::Decode(Address, Entry, Stack);
	if (auto&& EntryPtr = GetEntrySafe(Entry))
	{
		return EntryPtr->GetStack(Stack);
	}
	return 0;
}

const UFaerieItem* UFaerieItemStorage::ViewItem(const FEntryKey Key) const
{
	if (auto&& EntryPtr = GetEntrySafe(Key))
	{
		return EntryPtr->GetItem();
	}
	return nullptr;
}

const UFaerieItem* UFaerieItemStorage::ViewItem(const FFaerieAddress Address) const
{
	FEntryKey Entry;
	Storage::Address::Decode_Entry(Address, Entry);
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
	Storage::Address::Decode(Address, Entry, Stack);
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
		TakeStack(Address, OutStack, Inventory::Tags::RemovalMoving, Copies))
	{
		return OutStack;
	}
	return FFaerieItemStack();
}

TUniquePtr<Container::IIterator> UFaerieItemStorage::CreateEntryIterator() const
{
	// Don't provide an iterator if we are empty...
	if (EntryMap.IsEmpty()) return nullptr;
	return MakeUnique<Storage::FIterator_AllEntries_ForInterface>(this);
}

TUniquePtr<Container::IIterator> UFaerieItemStorage::CreateAddressIterator() const
{
	// Don't provide an iterator if we are empty...
	if (EntryMap.IsEmpty()) return nullptr;
	return MakeUnique<Storage::FIterator_AllAddresses_ForInterface>(this);
}

TUniquePtr<Container::IIterator> UFaerieItemStorage::CreateSingleEntryIterator(const FEntryKey Key) const
{
	// Don't provide an iterator if the key is invalid...
	if (const FInventoryEntry* Entry = GetEntrySafe(Key))
	{
		return MakeUnique<Storage::FIterator_SingleEntry_ForInterface>(this, *Entry);
	}
	return nullptr;
}

FFaerieItemStack UFaerieItemStorage::Release(const FFaerieItemStackView Stack)
{
	// This being set to ComparePointers means that the Stack must contain the *exact* same UFaerieItem pointer as the
	// one contained in our entry. This ensures that the stack was reliably sourced, and is faster.
	// @todo expose this to configuration somewhere???
	static constexpr EFaerieItemEqualsCheck CheckLevelForStorageRelease = EFaerieItemEqualsCheck::ComparePointers;

	const FEntryKey Key = FindItem(Stack.Item.Get(), CheckLevelForStorageRelease);
	return Release(Key, Stack.Copies);
}

bool UFaerieItemStorage::Possess(const FFaerieItemStack Stack)
{
	if (!CanAddStack(Stack, EFaerieStorageAddStackBehavior::AddToAnyStack)) return false;
	(void)AddStackImpl(Stack, false);
	return true;
}

void UFaerieItemStorage::OnItemMutated(const TNotNull<const UFaerieItem*> Item, const TNotNull<const UFaerieItemToken*> Token, const FGameplayTag EditTag)
{
	Super::OnItemMutated(Item, Token, EditTag);

	if (const FInventoryEntry* Entry = FindEntry(Item, EFaerieItemEqualsCheck::ComparePointers))
	{
		PostContentChanged(*Entry, FInventoryContent::ItemMutated, nullptr);
	}
}

void UFaerieItemStorage::PostContentAdded(const FInventoryEntry& Entry)
{
	if (!Entry.IsValid())
	{
		UE_LOG(LogFaerieInventory, Error, TEXT("PostContentAdded: Received Invalid Entry"))
		return;
	}

	// Proxies may already exist for keys on the client if they are replicated by extensions or other means, and
	// happened to arrive before we got them.
	TArray<FFaerieAddress, TInlineAllocator<1>> Addresses;
	Addresses.Reserve(Entry.NumStacks());
	for (const FKeyedStack& Stack : Entry.GetStacks())
	{
		Addresses.Emplace(Storage::Address::Encode(Entry.GetKey(), Stack.Key));
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
	if (!Entry.IsValid())
	{
		UE_LOG(LogFaerieInventory, Error, TEXT("PreContentRemoved: Received Invalid Entry"))
		return;
	}

	// Collate addresses
	TArray<FFaerieAddress, TInlineAllocator<1>> Addresses;
	Addresses.Reserve(Entry.NumStacks());
	for (const FKeyedStack& Stack : Entry.GetStacks())
	{
		Addresses.Emplace(Storage::Address::Encode(Entry.GetKey(), Stack.Key));
	}

	// Cleanup local views.
	for (const FFaerieAddress Address : Addresses)
	{
		TWeakObjectPtr<UFaerieItemStackProxy> StackProxy;
		LocalStackProxies.RemoveAndCopyValue(Address, StackProxy);
		if (StackProxy.IsValid())
		{
			StackProxy->NotifyRemoval();
		}
	}
}

void UFaerieItemStorage::PostContentChanged(const FInventoryEntry& Entry, const FInventoryContent::EChangeType ChangeType, const TBitArray<>* EntryChangeMask)
{
	if (!ensure(Entry.IsValid()))
	{
		UE_LOG(LogFaerieInventory, Error, TEXT("PostContentChanged: Received Invalid Entry"))
		return;
	}

	if (!ensure(Contains(Entry.GetKey())))
	{
		// Do nothing, PreContentRemoved should handle this ...
		return;
	}

	/*
		switch (ChangeType)
		{
		case FInventoryContent::Server_ItemHandleClosed:
			{
				if (EntryChangeMask)
				{
					for (TConstSetBitIterator<> It(*EntryChangeMask); It; ++It)
					{
						const FStackKey StackKey = Entry.GetStackAt(It.GetIndex());
						BroadcastAddressEvent(EFaerieAddressEventType::Edit, MakeAddress(Entry.Key, StackKey));
					}
				}
				else
				{
					for (const FFaerieAddress& Address : Faerie::Storage::FIterator_SingleEntry(Entry))
					{
						BroadcastAddressEvent(EFaerieAddressEventType::Edit, Address);
					}
				}
			}
			break;
		case FInventoryContent::Client_SomethingReplicated:
			{
				// @todo it's overkill to update all addresses for a stack change... but we dont know how to determine which stack actually changed, so blast them all :/
				for (const FFaerieAddress& Address : Faerie::Storage::FIterator_SingleEntry(Entry))
				{
					BroadcastAddressEvent(EFaerieAddressEventType::Edit, Address);
				}
			}
			break;
		case FInventoryContent::ItemMutated:
			{
				// Update all addresses for the item after a mutation.
				for (const FFaerieAddress& Address : Faerie::Storage::FIterator_SingleEntry(Entry))
				{
					BroadcastAddressEvent(EFaerieAddressEventType::Edit, Address);
				}
			}
			break;
		}
		*/

	// Call updates on any stack proxies.
	// PostContentChanged is called when stacks are removed as well, so let's do some cleanup here.
	// Start by getting all the Addresses that we could have proxies for.
	TSet<FFaerieAddress> Addresses;
	Addresses.Reserve(Entry.NumStacks());
	for (const FKeyedStack& Stack : Entry.GetStacks())
	{
		Addresses.Add(Storage::Address::Encode(Entry.GetKey(), Stack.Key));
	}

	for (auto It = LocalStackProxies.CreateIterator(); It; ++It)
	{
		auto&& LocalStackProxy = *It;

		// Check for local proxies that match this entry
		if (!LocalStackProxy.Value.IsValid() ||
			LocalStackProxy.Value->GetKey() != Entry.GetKey())
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


	/**------------------------------*/
	/*	  INTERNAL IMPLEMENTATIONS	 */
	/**------------------------------*/

TArray<FEntryKey> UFaerieItemStorage::CopyEntryKeys() const
{
	TArray<FEntryKey> Keys;
	Keys.Reserve(EntryMap.Num());
	Algo::Transform(EntryMap, Keys, &FInventoryEntry::GetKey);
	return Keys;
}

const FInventoryEntry* UFaerieItemStorage::GetEntrySafe(const FEntryKey Key) const
{
	return EntryMap.Find(Key);
}

const FInventoryEntry* UFaerieItemStorage::FindEntry(const TNotNull<const UFaerieItem*> Item, const EFaerieItemEqualsCheck Method) const
{
	switch (Method)
	{
	case EFaerieItemEqualsCheck::ComparePointers:
		for (const FInventoryEntry& Entry : EntryMap)
		{
			if (Item == Entry.GetItem())
			{
				return &Entry;
			}
		}
		break;
	case EFaerieItemEqualsCheck::UseCompareWith:
		for (const FInventoryEntry& Entry : EntryMap)
		{
			if (Item->CompareWith(Entry.GetItem(), EFaerieItemComparisonFlags::Default))
			{
				return &Entry;
			}
		}
		break;
	}

	return nullptr;
}

UFaerieItemStackProxy* UFaerieItemStorage::GetStackProxyImpl(const FFaerieAddress Address) const
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
	Storage::Address::Decode(Address, Entry, Stack);
	const FName ProxyName = MakeUniqueObjectName(This, UFaerieItemStackProxy::StaticClass(),
												 *FString::Printf(TEXT("STACK_PROXY_%s_%s"),
												 *Entry.ToString(), *Stack.ToString()));
	UFaerieItemStackProxy* NewEntryProxy = NewObject<UFaerieItemStackProxy>(This, UFaerieItemStackProxy::StaticClass(), ProxyName);
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

Inventory::FEventData UFaerieItemStorage::AddStackImplNoBroadcast(const FFaerieItemStack& InStack, const bool ForceNewStack)
{
	Inventory::FEventData Event;

	// Setup Log for this event
	Event.Item = InStack.Item;
	Event.Amount = InStack.Copies;

	static constexpr EFaerieItemEqualsCheck CheckLevelForStorageAddition = EFaerieItemEqualsCheck::UseCompareWith;

	const FInventoryEntry* const ExistingEntry = [&]() -> const FInventoryEntry*
		{
			// Mutables cannot stack, due to, well, being mutable, meaning that each instance retains the ability to
			// uniquely mutate from others.
			if (!InStack.Item->CanMutate())
			{
				return FindEntry(InStack.Item, CheckLevelForStorageAddition);
			}
			return nullptr;
		}();

	if (ExistingEntry)
	{
		Event.EntryTouched = ExistingEntry->GetKey();

		// Try to fill up the stacks of existing entries first, before creating a new entry.
		FInventoryEntry::FMutableAccess Entry = ExistingEntry->GetMutableAccess(EntryMap);
		if (ForceNewStack)
		{
			Entry.AddToNewStacks(Event.Amount, Event.AddressesTouched);
		}
		else
		{
			Entry.AddToAnyStack(Event.Amount, Event.AddressesTouched);
		}
	}
	else
	{
		if (UFaerieItem* Mutable = InStack.Item->MutateCast())
		{
			ItemData::TakeOwnership(this, Mutable);
		}

		// NextKey() is guaranteed to have a greater value than all currently existing keys, so simply appending is fine, and
		// will keep the EntryMap sorted.
		const FInventoryEntry NewEntry ( InStack, KeyGen.NextKey(), Event.AddressesTouched );

		EntryMap.AppendUnsafe(NewEntry);
		Event.EntryTouched = NewEntry.GetKey();
	}

	return Event;
}

Inventory::FEventData UFaerieItemStorage::AddStackImpl(const FFaerieItemStack& InStack, const bool ForceNewStack)
{
	SCOPE_CYCLE_COUNTER(STAT_Storage_Add);

	// Execute PreAddition on all extensions
	Extensions->PreAddition(this, InStack);

	const Inventory::FEventData Event = AddStackImplNoBroadcast(InStack, ForceNewStack);

	// Execute PostEventBatch on all extensions with the finished Event
	Extensions->PostEvent(this, Event, Inventory::Tags::Addition);

	return Event;
}

Inventory::FEventData UFaerieItemStorage::RemoveFromEntryImplNoBroadcast(const FInventoryEntry& InEntry, const int32 Amount)
{
	Inventory::FEventData Event;

	// Log for this event
	Event.EntryTouched = InEntry.GetKey();

	bool RemoveEntry = false;

	// Open Mutable Scope
	{
		FInventoryEntry::FMutableAccess Entry = InEntry.GetMutableAccess(EntryMap);

		Event.Item = Entry->GetItem();
		const int32 Sum = Entry->StackSum();

		if (Amount == ItemData::EntireStack || Amount >= Sum) // Remove the entire entry
		{
			Event.Amount = Sum;
			Entry->CopyAddresses(Event.AddressesTouched);
			if (UFaerieItem* Mutable = Entry->GetItem()->MutateCast())
			{
				ItemData::ReleaseOwnership(this, Mutable);
			}
			RemoveEntry = true;
		}
		else // Remove part of the entry
		{
			Event.Amount = FMath::Clamp(Amount, 1, Sum-1);
			Entry.RemoveFromAnyStack(Event.Amount, Event.AddressesTouched);
		}
	}
	// Close Mutable scope

	if (RemoveEntry)
	{
		UE_LOG(LogFaerieInventory, Verbose, TEXT("Removing entire entry at: '%s'"), *InEntry.GetKey().ToString());
		EntryMap.Remove(InEntry.GetKey());
	}

	return Event;
}

Inventory::FEventData UFaerieItemStorage::RemoveFromEntryImpl(const FInventoryEntry& Entry, const int32 Amount,
																			const FFaerieInventoryTag Reason)
{
	SCOPE_CYCLE_COUNTER(STAT_Storage_Remove);

	// RemoveEntryImpl should not be called with unvalidated parameters.
	check(Faerie::ItemData::IsValidStackAmount(Amount));
	check(Reason.MatchesTag(Faerie::Inventory::Tags::RemovalBase))

	Extensions->PreRemoval(this, Entry.GetKey(), Amount);

	const Inventory::FEventData Event = RemoveFromEntryImplNoBroadcast(Entry, Amount);

	Extensions->PostEvent(this, Event, Reason);

	return Event;
}

Inventory::FEventData UFaerieItemStorage::RemoveFromStackImpl(const FFaerieAddress Address, const int32 Amount,
																			const FFaerieInventoryTag Reason)
{
	SCOPE_CYCLE_COUNTER(STAT_Storage_Remove);

	// RemoveFromStackImpl should not be called with unvalidated parameters.
	check(Contains(Address));
	check(Faerie::ItemData::IsValidStackAmount(Amount));
	check(Reason.MatchesTag(Faerie::Inventory::Tags::RemovalBase))

	Inventory::FEventData Event;

	FEntryKey EntryKey;
	FStackKey StackKey;
	Storage::Address::Decode(Address, EntryKey, StackKey);

	Extensions->PreRemoval(this, EntryKey, Amount);

	// Log for this event
	Event.EntryTouched = EntryKey;
	Event.AddressesTouched.Add(Address);

	bool RemoveEntry = false;

	// Open Mutable Scope
	{
		FInventoryEntry::FMutableAccess Entry = EntryMap[EntryKey].GetMutableAccess(EntryMap);;

		Event.Item = Entry->GetItem();

		if (auto&& Stack = Entry->GetStack(StackKey);
			Amount == ItemData::EntireStack || Amount >= Stack) // Remove the entire stack
		{
			Event.Amount = Stack;

			// If removing this stack would remove all reference to this item, release and exit so we can remove the entry.
			if (Entry.IsOnlyStack(StackKey))
			{
				if (UFaerieItem* Mutable = Entry->GetItem()->MutateCast())
				{
					ItemData::ReleaseOwnership(this, Mutable);
				}
				RemoveEntry = true;
			}
			else
			{
				Entry.RemoveStack(StackKey);
			}
		}
		else // Remove part of the stack
		{
			check(Amount == FMath::Clamp(Amount, 1, Stack-1));

			Event.Amount = Amount;

			auto&& NewAmount = Stack - Amount;
			Entry.SetStack(StackKey, NewAmount);
		}
	}
	// Close Mutable scope

	if (RemoveEntry)
	{
		UE_LOG(LogFaerieInventory, Verbose, TEXT("Removing entire stack at: '%s_%s'"), *EntryKey.ToString(), *StackKey.ToString());
		EntryMap.Remove(EntryKey);
	}

	Extensions->PostEvent(this, Event, Reason);

	return Event;
}

bool UFaerieItemStorage::CanRemoveEntryImpl(const FInventoryEntry& Entry, const FFaerieInventoryTag Reason) const
{
	// By default, some removal reasons are allowed, unless an extension explicitly disallows it.
	bool Allowed = Inventory::Tags::RemovalTagsAllowedByDefault().Contains(Reason);

	for (auto It = Storage::FIterator_SingleEntry(Entry); It; ++It)
	{
		switch (Extensions->AllowsRemoval(this, *It, Reason))
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


	/**------------------------------*/
	/*	 STORAGE API - ALL USERS   */
	/**------------------------------*/

FFaerieAddress UFaerieItemStorage::MakeAddress(const FEntryKey Entry, const FStackKey Stack)
{
	return Storage::Address::Encode(Entry, Stack);
}

FEntryKey UFaerieItemStorage::GetAddressEntry(const FFaerieAddress Address)
{
	FEntryKey Key;
	Storage::Address::Decode_Entry(Address, Key);
	return Key;
}

FStackKey UFaerieItemStorage::GetAddressStack(const FFaerieAddress Address)
{
	FStackKey Key;
	Storage::Address::Decode_Stack(Address, Key);
	return Key;
}

TTuple<FEntryKey, FStackKey> UFaerieItemStorage::BreakAddress(const FFaerieAddress Address)
{
	FEntryKey Entry;
	FStackKey Stack;
	Storage::Address::Decode(Address, Entry, Stack);
	return MakeTuple(Entry, Stack);
}

bool UFaerieItemStorage::BreakAddressIntoKeys(const FFaerieAddress Address, FEntryKey& Entry, FStackKey& Stack) const
{
	Storage::Address::Decode(Address, Entry, Stack);
	if (auto&& EntryPtr = GetEntrySafe(Entry))
	{
		return EntryPtr->Contains(Stack);
	}
	return false;
}

TArray<FStackKey> UFaerieItemStorage::BreakEntryIntoKeys(const FEntryKey Key) const
{
	TArray<FStackKey> Out;
	if (const FInventoryEntry* Entry = GetEntrySafe(Key))
	{
		Entry->CopyKeys(Out);
	}
	return Out;
}

TArray<int32> UFaerieItemStorage::GetStacksInEntry(const FEntryKey Key) const
{
	TArray<int32> Out;
	if (const FInventoryEntry* Entry = GetEntrySafe(Key))
	{
		Entry->CopyStacks(Out);
	}
	return Out;
}

TArray<FFaerieAddress> UFaerieItemStorage::GetAddressesForEntry(const FEntryKey Key) const
{
	TArray<FFaerieAddress> Out;

	if (const FInventoryEntry* Entry = GetEntrySafe(Key))
	{
		Out.Reserve(Entry->NumStacks());
		for (const FKeyedStack& Stack : Entry->GetStacks())
		{
			Out.Add(Storage::Address::Encode(Key, Stack.Key));
		}

		check(!Out.IsEmpty())
	}

	return Out;
}

void UFaerieItemStorage::GetAllKeys(TArray<FEntryKey>& Keys) const
{
	Keys = CopyEntryKeys();
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
		Stacks += Entry.NumStacks();
	}
	return Stacks;
}

bool UFaerieItemStorage::ContainsKey(const FEntryKey Key) const
{
	return Contains(Key);
}

bool UFaerieItemStorage::ContainsItem(const UFaerieItem* Item, const EFaerieItemEqualsCheck Method) const
{
	return !!FindEntry(Item, Method);
}

FEntryKey UFaerieItemStorage::FindItem(const UFaerieItem* Item, const EFaerieItemEqualsCheck Method) const
{
	if (!IsValid(Item))
	{
		return FEntryKey();
	}

	if (const FInventoryEntry* Entry = FindEntry(Item, Method))
	{
		return Entry->GetKey();
	}

	return FEntryKey();
}

FFaerieAddress UFaerieItemStorage::GetFirstAddress() const
{
	if (EntryMap.IsEmpty()) return FFaerieAddress();
	const FInventoryEntry& FirstEntry = EntryMap.Entries[0];
	return Storage::Address::Encode(FirstEntry.GetKey(), FirstEntry.GetStacks()[0].Key);
}

const UFaerieItem* UFaerieItemStorage::GetEntryItem(const FEntryKey Key) const
{
	if (const FInventoryEntry* Entry = GetEntrySafe(Key))
	{
		return Entry->GetItem();
	}
	return nullptr;
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
		if (SubObject::GetAllContainersInItemRecursive(Mutable).Contains(this))
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
			if (SubObject::GetAllContainersInItemRecursive(Mutable).Contains(this))
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
	const bool DefaultAllowed = Inventory::Tags::EditTagsAllowedByDefault().Contains(EditTag);

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
	Storage::Address::Decode_Entry(Address, Entry);
	return CanEditEntry(Entry, EditTag);
}

bool UFaerieItemStorage::CanRemoveEntry(const FEntryKey Key, const FFaerieInventoryTag Reason) const
{
	if (const FInventoryEntry* EntryPtr = GetEntrySafe(Key))
	{
		return CanRemoveEntryImpl(*EntryPtr, Reason);
	}
	return false;
}

bool UFaerieItemStorage::CanRemoveStack(const FFaerieAddress Address, const FFaerieInventoryTag Reason) const
{
	if (!Contains(Address)) return false;

	// By default, some removal reasons are allowed, unless an extension explicitly disallows it.
	const bool DefaultAllowed = Inventory::Tags::RemovalTagsAllowedByDefault().Contains(Reason);

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

	(void)AddStackImpl(Stack, Storage::IfOnlyNewStacks(AddStackBehavior));
	return true;
}

bool UFaerieItemStorage::AddItemStack(const FFaerieItemStack& ItemStack, const EFaerieStorageAddStackBehavior AddStackBehavior)
{
	if (!CanAddStack(ItemStack, AddStackBehavior))
	{
		return false;
	}

	(void)AddStackImpl(ItemStack, Storage::IfOnlyNewStacks(AddStackBehavior));
	return true;
}

void UFaerieItemStorage::AddItemStack(const FFaerieItemStack& ItemStack,
	const EFaerieStorageAddStackBehavior AddStackBehavior, TValueOrError<Inventory::FEventData, FText>& OutResult)
{
	if (!CanAddStack(ItemStack, AddStackBehavior))
	{
		OutResult = MakeError(Storage::AdditionFailure_FailedCanAddStack);
		return;
	}

	OutResult = MakeValue(AddStackImpl(ItemStack, Storage::IfOnlyNewStacks(AddStackBehavior)));
}

void UFaerieItemStorage::AddItemStacks(const TConstArrayView<FFaerieItemStack> ItemStacks, const EFaerieStorageAddStackBehavior AddStackBehavior)
{
	SCOPE_CYCLE_COUNTER(STAT_Storage_AddMulti);

	const bool ForceNewStack = Storage::IfOnlyNewStacks(AddStackBehavior);

	Inventory::FEventLogBatch Batch;
	Batch.Type = Inventory::Tags::Addition;
	TArray<Inventory::FEventData> Events;
	Events.Reserve(ItemStacks.Num());

	for (auto&& ItemStack : ItemStacks)
	{
		if (!ensureAlwaysMsgf(
			ItemStack.IsValid(),
			TEXT("AddStackImpl was passed an invalid stack.")))
		{
			continue;
		}

		if (!CanAddStack(ItemStack, AddStackBehavior))
		{
			// @todo how do we want to handle this failure?
			continue;
		}

		// Execute PreAddition on all extensions
		Extensions->PreAddition(this, ItemStack);

		Events.Add(AddStackImplNoBroadcast(ItemStack, ForceNewStack));
	}

	// Execute PostEventBatch on all extensions with the finished Event
	Batch.Data = Events;
	Extensions->PostEventBatch(this, Batch);
}

void UFaerieItemStorage::AddItemStackBulk(const TArray<FFaerieItemStack>& ItemStacks, const EFaerieStorageAddStackBehavior AddStackBehavior)
{
	AddItemStacks(ItemStacks, AddStackBehavior);
}

bool UFaerieItemStorage::AddItemStackWithLog(const FFaerieItemStack& ItemStack, const EFaerieStorageAddStackBehavior AddStackBehavior, FLoggedInventoryEvent& Event)
{
	TValueOrError<Inventory::FEventData, FText> Result = MakeError(FText::GetEmpty());
	AddItemStack(ItemStack, AddStackBehavior, Result);
	if (Result.HasValue())
	{
		const Inventory::FEventLogSingle Single(Inventory::Tags::Addition, Result.GetValue());
		Event = {this, Single };
	}
	return false;
}

bool UFaerieItemStorage::RemoveEntry(const FEntryKey Key, const FFaerieInventoryTag RemovalTag, const int32 Amount)
{
	if (Amount == 0 || Amount < ItemData::EntireStack) return false;
	if (!RemovalTag.IsValid()) return false;

	if (const FInventoryEntry* EntryPtr = GetEntrySafe(Key))
	{
		if (CanRemoveEntryImpl(*EntryPtr, RemovalTag))
		{
			(void)RemoveFromEntryImpl(*EntryPtr, Amount, RemovalTag);
			return true;
		}
	}
	return false;
}

bool UFaerieItemStorage::RemoveStack(const FFaerieAddress Address, const FFaerieInventoryTag RemovalTag, const int32 Amount)
{
	if (Amount == 0 || Amount < ItemData::EntireStack) return false;
	if (!RemovalTag.IsValid()) return false;

	if (!CanRemoveStack(Address, RemovalTag)) return false;

	(void)RemoveFromStackImpl(Address, Amount, RemovalTag);
	return true;
}

bool UFaerieItemStorage::TakeEntry(const FEntryKey Key, FFaerieItemStack& OutStack,
								   const FFaerieInventoryTag RemovalTag, const int32 Amount)
{
	if (Amount == 0 || Amount < ItemData::EntireStack) return false;
	if (!RemovalTag.IsValid()) return false;

	if (const FInventoryEntry* EntryPtr = GetEntrySafe(Key))
	{
		if (!CanRemoveEntryImpl(*EntryPtr, RemovalTag)) return false;

		auto&& Event = RemoveFromEntryImpl(*EntryPtr, Amount, RemovalTag);
		OutStack = { Event.Item.Get(), Event.Amount };
		return true;
	}

	return false;
}

bool UFaerieItemStorage::TakeStack(const FFaerieAddress Address, FFaerieItemStack& OutStack,
								   const FFaerieInventoryTag RemovalTag, const int32 Amount)
{
	if (Amount == 0 || Amount < ItemData::EntireStack) return false;
	if (!RemovalTag.IsValid()) return false;

	if (!CanRemoveStack(Address, RemovalTag)) return false;

	auto&& Event = RemoveFromStackImpl(Address, Amount, RemovalTag);
	OutStack = { Event.Item.Get(), Event.Amount };
	return true;
}

void UFaerieItemStorage::Clear(FFaerieInventoryTag RemovalTag)
{
	if (!RemovalTag.IsValid() || !RemovalTag.MatchesTag(Inventory::Tags::RemovalBase))
	{
		RemovalTag = Inventory::Tags::RemovalDeletion;
	}

	const TArray<FEntryKey> Entries = CopyEntryKeys();
	Inventory::FEventLogBatch Batch;
	Batch.Type = RemovalTag;
	TArray<Inventory::FEventData> Events;
	Events.Reserve(Entries.Num());
	for (const FEntryKey EntryKey : Entries)
	{
		const FInventoryEntry& Entry = EntryMap[EntryKey];

		if (!CanRemoveEntryImpl(Entry, Inventory::Tags::RemovalMoving))
		{
			continue;
		}

		Extensions->PreRemoval(this, EntryKey, ItemData::EntireStack);

		// RemoveFromEntryImplNoBroadcast should not be called with unvalidated parameters.
		Events.Add(RemoveFromEntryImplNoBroadcast(Entry, ItemData::EntireStack));
	}

	Batch.Data = Events;
	Extensions->PostEventBatch(this, Batch);

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
		!ItemData::IsValidStackAmount(Amount))
	{
		return FEntryKey::InvalidKey;
	}

	// Verify the stack exists.
	FEntryKey Entry;
	Storage::Address::Decode_Entry(Address, Entry);
	const FInventoryEntry* EntryPtr = GetEntrySafe(Entry);
	if (!EntryPtr)
	{
		return FEntryKey::InvalidKey;
	}

	if (!CanRemoveStack(Address, Inventory::Tags::RemovalMoving))
	{
		return FEntryKey::InvalidKey;
	}

	FStackKey Stack;
	Storage::Address::Decode_Stack(Address, Stack);
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
	if (!TakeStack(Address, ItemStack, Inventory::Tags::RemovalMoving, Amount))
	{
		return FEntryKey::InvalidKey;
	}

	return ToStorage->AddStackImpl(ItemStack, Storage::IfOnlyNewStacks(AddStackBehavior)).EntryTouched;
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

	if (!CanRemoveEntryImpl(*EntryPtr, Inventory::Tags::RemovalMoving))
	{
		return FEntryKey::InvalidKey;
	}

	if (!ToStorage->CanAddStack(EntryPtr->ToItemStackView(), AddStackBehavior))
	{
		return FEntryKey::InvalidKey;
	}

	auto&& RemoveResult = RemoveFromEntryImpl(*EntryPtr, ItemData::EntireStack, Inventory::Tags::RemovalMoving);

	const FFaerieItemStack Stack { RemoveResult.Item.Get(), RemoveResult.Amount };

	return ToStorage->AddStackImpl(Stack, Storage::IfOnlyNewStacks(AddStackBehavior)).EntryTouched;
}

bool UFaerieItemStorage::MergeStacks(const FEntryKey Entry, const FStackKey FromStack, const FStackKey ToStack, const int32 Amount)
{
	const FFaerieAddress FromAddress = Storage::Address::Encode(Entry, FromStack);
	const FFaerieAddress ToAddress = Storage::Address::Encode(Entry, ToStack);

	// Verify the stack exists.
	const FInventoryEntry* EntryPtr = GetEntrySafe(Entry);
	if (!EntryPtr)
	{
		return false;
	}

	if (!CanEditStack(FromAddress, Inventory::Tags::Merge) ||
		!CanEditStack(ToAddress, Inventory::Tags::Merge))
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

	Inventory::FEventData Event;
	Event.Amount = AmountB; // Initially store the amount in stack B here.
	Event.Item = EntryPtr->GetItem();
	Event.EntryTouched = Entry;
	Event.AddressesTouched.Add(FromAddress);
	Event.AddressesTouched.Add(ToAddress);

	// Open Mutable Scope
	{
		FInventoryEntry::FMutableAccess Handle = EntryPtr->GetMutableAccess(EntryMap);
		const int32 Remainder = Handle.MoveStack(FromStack, ToStack, Amount);

		// We didn't move this many.
		Event.Amount -= Remainder;
	}
	// Close Mutable scope

	Extensions->PostEvent(this, Event, Inventory::Tags::Merge);

	return true;
}

bool UFaerieItemStorage::SplitStack(const FFaerieAddress Address, const int32 Amount)
{
	// Decode and verify the stack exists.
	FEntryKey Entry;
	FStackKey Stack;
	Storage::Address::Decode(Address, Entry, Stack);
	const FInventoryEntry* EntryPtr = GetEntrySafe(Entry);
	if (!EntryPtr)
	{
		return false;
	}

	// Check if we can edit the amount requested
	if (!CanEditStack(Address, Inventory::Tags::Split))
	{
		return false;
	}

	// Validate that the requested amount is less than what's in the stack
	if (Amount >= EntryPtr->GetStack(Stack))
	{
		return false;
	}

	Inventory::FEventData Event;
	Event.Item = EntryPtr->GetItem();
	Event.Amount = Amount;
	Event.EntryTouched = Entry;
	Event.AddressesTouched.Add(Address);

	// Split the stack
	{
		FInventoryEntry::FMutableAccess Handle = EntryPtr->GetMutableAccess(EntryMap);
		const FStackKey SplitStack = Handle.SplitStack(Stack, Amount);

		// Update event with final Address information
		Event.AddressesTouched.Add(Storage::Address::Encode(Entry, SplitStack));
	}

	Extensions->PostEvent(this, Event, Inventory::Tags::Split);

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

	const TArray<FEntryKey> Entries = CopyEntryKeys();
	Inventory::FEventLogBatch EventBatch;
	EventBatch.Type = Inventory::Tags::RemovalMoving;

	TArray<Inventory::FEventData> Events;
	Events.Reserve(Entries.Num());

	TArray<FFaerieItemStack> Stacks;
	Stacks.Reserve(Entries.Num());

	for (const FEntryKey EntryKey : Entries)
	{
		const FInventoryEntry& Entry = EntryMap[EntryKey];

		if (!CanRemoveEntryImpl(Entry, Inventory::Tags::RemovalMoving))
		{
			continue;
		}

		if (!ToStorage->CanAddStack(Entry.ToItemStackView(), DumpBehavior))
		{
			continue;
		}

		Extensions->PreRemoval(this, EntryKey, ItemData::EntireStack);

		auto&& Event = Events.Add_GetRef(RemoveFromEntryImplNoBroadcast(Entry, ItemData::EntireStack));

		Stacks.Emplace(Event.Item.Get(), Event.Amount);
	}

	EventBatch.Data = Events;
	Extensions->PostEventBatch(this, EventBatch);

	ToStorage->AddItemStacks(Stacks, DumpBehavior);
}

#undef LOCTEXT_NAMESPACE

/*
 * Footnote1: You might think that even at runtime we could reset the key during Clear, since all items are removed,
 * and therefor no entries exist, making 100 a valid starting point again, *except* that other entities might still
 * be holding onto FEntryKeys, which could be cached in at some point later when potentially the entry is once more
 * valid, but with a completely different item. So during runtime, the Key must always increment.
 */