// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemStorage.h"
#include "FaerieContainerEvent.h"
#include "FaerieInventoryLog.h"
#include "FaerieMassFragmentAccessors.h"
#include "FaerieItemDestructionProcessor.h"
#include "FaerieItemStorageIterators.h"
#include "FaerieItemOwnership.h"
#include "FaerieSubObjectFilter.h"
#include "ItemEntryProxy.h"
#include "ItemStackProxy.h"
#include "ItemContainerExtensionBase.h"
#include "EntityManagerHelpers.h"
#include "ValidParameter.h"

#include "Fragments/FaerieStackLimitFragment.h"

#include "Algo/Accumulate.h"
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

namespace Faerie::Container
{
	namespace Tags
	{
		UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryTag, Merge,
			"Fae.Inventory.Edit.Merge", "An entry was edited to merge two stacks")
		UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryTag, Split,
			"Fae.Inventory.Edit.Split", "An entry was edited to split an amount off onto a new stack")

		const TSet<FFaerieInventoryTag>& StorageEditTagsAllowedByDefault()
		{
			static const TSet<FFaerieInventoryTag> StaticTags
			{
				Merge,
				Split
			};

			return StaticTags;
		}

		const TSet<FFaerieInventoryTag>& StorageRemovalTagsAllowedByDefault()
		{
			static const TSet<FFaerieInventoryTag> StaticTags
			{
				Inventory::Tags::RemovalDeletion,
				Inventory::Tags::RemovalMoving
			};

			return StaticTags;
		}
	}

	namespace Address
	{
		[[nodiscard]] UE_REWRITE FFaerieAddress Encode(const FFaerieEntryKey Entry, const FFaerieStackKey Stack)
		{
			return FFaerieAddress((static_cast<int64>(Entry.Value()) << 32) | static_cast<int64>(Stack.Value()));
		}

		UE_REWRITE void Decode(const FFaerieAddress Address, FFaerieEntryKey& Entry, FFaerieStackKey& Stack)
		{
			static constexpr int64 Mask = 0x00000000FFFFFFFF;
			Stack = FFaerieStackKey(Address.Address & Mask);
			Entry = FFaerieEntryKey(Address.Address >> 32);
		}

		UE_REWRITE void Decode_Entry(const FFaerieAddress Address, FFaerieEntryKey& Entry)
		{
			Entry = FFaerieEntryKey(Address.Address >> 32);
		}

		UE_REWRITE void Decode_Stack(const FFaerieAddress Address, FFaerieStackKey& Stack)
		{
			static constexpr int64 Mask = 0x00000000FFFFFFFF;
			Stack = FFaerieStackKey(Address.Address & Mask);
		}
	}

	constexpr bool IfOnlyNewStacks(const EFaerieStorageAddStackBehavior Behavior)
	{
		return Behavior == EFaerieStorageAddStackBehavior::OnlyNewStacks;
	}

	static const FText AddItemStackFailure_FailedCanAddStack = LOCTEXT("AdditionFailure_FailedCanAddStack", "Refused by CanAddStack");
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

FInstancedStruct UFaerieItemStorage::MakeSaveData(const FMassEntityManager& EntityManager, const Container::FSaveParams Params) const
{
	FFaerieStorageExportData ExportData;
	MakeSaveData(EntityManager, ExportData, Params);
	return FInstancedStruct::Make(ExportData);
}

bool UFaerieItemStorage::LoadSaveData(FMassEntityManager& EntityManager, const FConstStructView ItemData, const Container::FLoadParams Params)
{
	if (ItemData.GetScriptStruct() != FFaerieStorageExportData::StaticStruct())
	{
		UE_LOGF(LogFaerieInventory, Error, "Invalid data to load from. Must be FFaerieStorageExportData.")
		return false;
	}
	LoadSaveData(EntityManager, ItemData.Get<const FFaerieStorageExportData>(), Params);
	return true;
}

bool UFaerieItemStorage::Contains(const FFaerieAddress Address) const
{
	return ContainsAddress(Address);
}

FMassEntityHandle UFaerieItemStorage::GetItemHandle(const FFaerieEntryKey Key) const
{
	if (const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Key))
	{
		return EntryPtr->GetEntityHandle();
	}
	return FMassEntityHandle();
}

FMassEntityHandle UFaerieItemStorage::GetItemHandle(const FFaerieAddress Address) const
{
	FFaerieEntryKey Entry;
	Container::Address::Decode_Entry(Address, Entry);
	if (const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Entry))
	{
		return EntryPtr->GetEntityHandle();
	}
	return FMassEntityHandle();
}

ItemData::FScopeProxy UFaerieItemStorage::ViewEntry(const FFaerieEntryKey Key) const
{
	if (const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Key))
	{
		return ItemData::FScopeProxy(EntryPtr->GetEntityHandle(), EntryPtr->StackSum());
	}
	return nullptr;
}

ItemData::FScopeProxy UFaerieItemStorage::ViewAddress(const FFaerieAddress Address) const
{
	FFaerieEntryKey Entry;
	FFaerieStackKey Stack;
	Container::Address::Decode(Address, Entry, Stack);
	if (const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Entry))
	{
		return ItemData::FScopeProxy(EntryPtr->GetEntityHandle(), EntryPtr->GetStack(Stack));
	}
	return nullptr;
}

FFaerieItemProxy UFaerieItemStorage::Proxy(const FMassEntityManager& EntityManager, const FMassEntityHandle Item) const
{
	if (const FFaerieStorageEntry* Entry = FindEntry(Item))
	{
		// @todo optimize to remove double lookup
		return FFaerieItemProxy(EntityManager.AsShared(), GetEntryProxyImpl(EntityManager, Entry->GetKey()));
	}
	return FFaerieItemProxy();
}

FFaerieItemProxy UFaerieItemStorage::Proxy(const FMassEntityManager& EntityManager, const FFaerieEntryKey Key) const
{
	return FFaerieItemProxy(EntityManager.AsShared(), GetEntryProxyImpl(EntityManager, Key));
}

FFaerieItemProxy UFaerieItemStorage::Proxy(const FMassEntityManager& EntityManager, const FFaerieAddress Address) const
{
	return FFaerieItemProxy(EntityManager.AsShared(), GetStackProxyImpl(Address));
}

bool UFaerieItemStorage::Possess(FMassEntityManager& EntityManager, const FFaerieUnownedItemStack& Stack)
{
	const ItemData::FScopeProxy Proxy(Stack.Item, Stack.Copies);
	if (!CanAddStack(EntityManager, FFaerieItemProxy(FFaerieItemProxy::ESingleFrame, EntityManager.AsShared(), &Proxy), EFaerieStorageAddStackBehavior::AddToAnyStack)) return false;

	(void)AddStackImpl(EntityManager, Stack, false);
	return true;
}

TOptional<FFaerieUnownedItemStack> UFaerieItemStorage::Release(FMassEntityManager& EntityManager, const FFaerieEntryKey Key, const int32 Copies,
	const FFaerieInventoryTag Reason)
{
	if (FFaerieUnownedItemStack OutStack;
		TakeEntry(EntityManager, Key, OutStack, Reason, Copies))
	{
		return OutStack;
	}
	return NullOpt;
}

TOptional<FFaerieUnownedItemStack> UFaerieItemStorage::Release(FMassEntityManager& EntityManager, const FFaerieAddress Address, const int32 Copies,
	const FFaerieInventoryTag Reason)
{
	if (FFaerieUnownedItemStack OutStack;
		TakeStack(EntityManager, Address, OutStack, Reason, Copies))
	{
		return OutStack;
	}
	return NullOpt;
}

TOptional<FFaerieUnownedItemStack> UFaerieItemStorage::Release(FMassEntityManager& EntityManager, const FFaerieItemProxy& Proxy, const int32 Copies,
	const FFaerieInventoryTag Reason)
{
	if (const UFaerieItemStackProxy* StackProxy = Cast<UFaerieItemStackProxy>(Proxy.GetProxyObject()))
	{
		if (StackProxy->GetOuter() != this)
		{
			UE_LOGF(LogFaerieInventory, Error, "This isn't our proxy! We cannot release copies from it.")
			return NullOpt;
		}

		FFaerieUnownedItemStack OutStack;
		TakeStack(EntityManager, StackProxy->Address, OutStack, Reason, Copies);
		return OutStack;
	}
	if (const UFaerieItemEntryProxy* EntryProxy = Cast<UFaerieItemEntryProxy>(Proxy.GetProxyObject()))
	{
		if (EntryProxy->GetOuter() != this)
		{
			UE_LOGF(LogFaerieInventory, Error, "This isn't our proxy! We cannot release copies from it.")
			return NullOpt;
		}

		FFaerieUnownedItemStack OutStack;
		TakeEntry(EntityManager, EntryProxy->Key, OutStack, Reason, Copies);
	}
	return NullOpt;
}

bool UFaerieItemStorage::CanPossess(const FMassEntityManager& EntityManager, const FFaerieItemProxy& Proxy) const
{
	return CanAddStack(EntityManager, Proxy, EFaerieStorageAddStackBehavior::AddToAnyStack);
}

bool UFaerieItemStorage::CanRelease(const FMassEntityManager& EntityManager, const FFaerieItemProxy& Proxy, const FFaerieInventoryTag Reason) const
{
	if (const UFaerieItemStackProxy* StackProxy = Cast<UFaerieItemStackProxy>(Proxy.GetProxyObject()))
	{
		return CanRemoveStack(EntityManager, StackProxy->Address, Reason);
	}
	if (const UFaerieItemEntryProxy* EntryProxy = Cast<UFaerieItemEntryProxy>(Proxy.GetProxyObject()))
	{
		return CanRemoveEntry(EntityManager, EntryProxy->Key, Reason);
	}
	return false;
}

void UFaerieItemStorage::GetAllAddresses(const TAdderReserverRef<FFaerieAddress> Addresses) const
{
	Addresses.Reserve(Algo::TransformAccumulate(EntryMap, &FFaerieStorageEntry::NumStacks, 0));
	for (const FFaerieStorageEntry& Entry : EntryMap)
	{
		for (const FFaerieKeyedStack& Stack : Entry.GetStacks())
		{
			Addresses.Add(Container::Address::Encode(Entry.GetKey(), Stack.Key));
		}
	}
}

bool UFaerieItemStorage::AckItemCreatedByNetwork(FMassEntityManager& EntityManager, const FMassEntityHandle Entity, const FFaerieNetItemID NetID)
{
	for (const FFaerieStorageEntry& Entry : EntryMap)
	{
		if (Entry.GetNetID() == NetID)
		{
			Entry.GetReadWriteAccess(EntryMap).AssignEntityFromReplication(Entity);
			return true;
		}
	}
	return false;
}

TUniquePtr<Container::IEntryIterator> UFaerieItemStorage::CreateEntryIterator() const
{
	// Don't provide an iterator if we are empty...
	if (EntryMap.IsEmpty()) return nullptr;
	return MakeUnique<Container::FIterator_AllEntries_WithInterface>(this);
}

TUniquePtr<Container::IAddressIterator> UFaerieItemStorage::CreateAddressIterator() const
{
	// Don't provide an iterator if we are empty...
	if (EntryMap.IsEmpty()) return nullptr;
	return MakeUnique<Container::FIterator_AllAddresses_WithInterface>(this);
}

TUniquePtr<Container::IAddressIterator> UFaerieItemStorage::CreateSingleEntryIterator(const FFaerieEntryKey Key) const
{
	// Don't provide an iterator if the key is invalid...
	if (const FFaerieStorageEntry* Entry = GetEntrySafe(Key))
	{
		return MakeUnique<Container::FIterator_SingleEntry_WithInterface>(this, *Entry);
	}
	return nullptr;
}


	/**------------------------------*/
	/*	  INTERNAL IMPLEMENTATIONS	 */
	/**------------------------------*/

void UFaerieItemStorage::CopyEntryKeys(TAdderReserverRef<FFaerieEntryKey> OutKeys) const
{
	OutKeys.Reserve(OutKeys.Num() + EntryMap.Num());
	Algo::Transform(EntryMap, OutKeys, &FFaerieStorageEntry::GetKey);
}

void UFaerieItemStorage::CollateAddresses(const FFaerieStorageEntry& Entry, TArray<FFaerieAddress>& OutAddresses)
{
	OutAddresses.Reserve(Entry.NumStacks());
	for (const FFaerieKeyedStack& Stack : Entry.GetStacks())
	{
		OutAddresses.Emplace(Container::Address::Encode(Entry.GetKey(), Stack.Key));
	}
}

const FFaerieStorageEntry* UFaerieItemStorage::GetEntrySafe(const FFaerieEntryKey Key) const
{
	return EntryMap.Find(Key);
}

const FFaerieStorageEntry* UFaerieItemStorage::FindEntry(const FMassEntityHandle Item) const
{
	for (const FFaerieStorageEntry& Entry : EntryMap)
	{
		if (Item == Entry.GetEntityHandle())
		{
			return &Entry;
		}
	}

	return nullptr;
}

UObject* UFaerieItemStorage::GetEntryProxyImpl(const FMassEntityManager& EntityManager, const FFaerieEntryKey Key) const
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

	// Note: Optimization
	// If the entry is mutable, then there is only ever one stack proxy, and we can use that as our entry proxy,
	// saving an extra object creation, and providing extra data to the receiving end.
	// We also can then safely restrict EntryProxies to only describing immutable items, lightening their API surface.
	if (const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Key))
	{
		if (ItemData::IsMutable(EntityManager, EntryPtr->GetEntityHandle()))
		{
			return GetStackProxyImpl(EntryPtr->FirstAddress());
		}
	}

	ThisClass* This = const_cast<ThisClass*>(this);

	const FName ProxyName = MakeUniqueObjectName(This, UFaerieItemEntryProxy::StaticClass(),
												 *FString::Printf(TEXT("ENTRY_PROXY_%s"),
												 *Key.ToString()));
	const TNotNull<UFaerieItemEntryProxy*> NewEntryProxy =
		NewObject<UFaerieItemEntryProxy>(This, UFaerieItemEntryProxy::StaticClass(), ProxyName);

	NewEntryProxy->Key = Key;

	This->LocalEntryProxies.Add(Key, NewEntryProxy);

	if (ContainsKey(Key))
	{
		NewEntryProxy->NotifyLocalCreation();
	}

	return NewEntryProxy;
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

	FFaerieEntryKey Entry;
	FFaerieStackKey Stack;
	Container::Address::Decode(Address, Entry, Stack);
	const FName ProxyName = MakeUniqueObjectName(This, UFaerieItemStackProxy::StaticClass(),
												 *FString::Printf(TEXT("STACK_PROXY_%s_%s"),
												 *Entry.ToString(), *Stack.ToString()));
	const TNotNull<UFaerieItemStackProxy*> NewStackProxy =
		NewObject<UFaerieItemStackProxy>(This, UFaerieItemStackProxy::StaticClass(), ProxyName);

	NewStackProxy->Address = Address;

	This->LocalStackProxies.Add(Address, NewStackProxy);

	if (const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Entry))
	{
		if (EntryPtr->Contains(Stack))
		{
			NewStackProxy->NotifyLocalCreation();
		}
	}

	return NewStackProxy;
}

Container::FContainerEventPayload UFaerieItemStorage::AddStackImplNoBroadcast(FMassEntityManager& EntityManager, const TValid<const FFaerieUnownedItemStack&> ItemStack, const bool ForceNewStack)
{
	static auto FindExistingEntry = [](FMassEntityManager& EntityManager, const TNotNull<const UFaerieItemStorage*> Storage, const FMassEntityHandle Item) -> const FFaerieStorageEntry*
		{
			if (!ItemData::IsMutable(EntityManager, Item))
			{
				return Storage->FindEntry(Item);
			}
			return nullptr;
		};

	const FMassEntityHandle Item = ValidGet(ItemStack).Item;
	const int32 Copies = ValidGet(ItemStack).Copies;

	if (const FFaerieStorageEntry* const ExistingEntry = FindExistingEntry(EntityManager, this, Item))
	{
		// Setup Log for this event
		Container::FContainerEventPayload Event = Container::FContainerEventPayload::MakeAddedCopies(this, Item, Copies, ExistingEntry->GetKey(), {});

		// Try to fill up the stacks of existing entries first, before creating a new entry.
		FFaerieStorageEntry::FReadWriteAccess Handle = ExistingEntry->GetReadWriteAccess(EntryMap);
		if (ForceNewStack)
		{
			Handle.AddToNewStacks(Copies, Event.AddressesTouched);
		}
		else
		{
			Handle.AddToAnyStack(Copies, Event.AddressesTouched);
		}
		Server_PostContentChanged(Handle.Get(), Event);
		return Event;
	}

	// Setup Log for this event
	Container::FContainerEventPayload Event = Container::FContainerEventPayload::MakeNewStack(this, Item, Copies, KeyGen.NextKey(), {});

	if (ItemData::IsMutable(EntityManager, Item))
	{
		Container::TakeOwnership(EntityManager, this, Item);
	}

	const int32 StackLimit = Container::GetItemStackLimit(EntityManager, Item);
	const FFaerieStorageEntry NewEntry ( Event.EntryTouched, Item, StackLimit, Copies, Event.AddressesTouched );

	// NextKey() is guaranteed to have a greater value than all currently existing keys, so simply appending is fine, and
	// will keep the EntryMap sorted.
	EntryMap.AppendUnsafe(NewEntry);
	Server_PostContentAdded(NewEntry, Event);
	return Event;
}

Container::FContainerEventPayload UFaerieItemStorage::AddStackImpl(FMassEntityManager& EntityManager, const TValid<const FFaerieUnownedItemStack&> ItemStack, const bool ForceNewStack)
{
	SCOPE_CYCLE_COUNTER(STAT_Storage_Add);

	Container::FContainerEventPayload Event = AddStackImplNoBroadcast(EntityManager, ItemStack, ForceNewStack);

	// Execute PostEventBatch with the finished Event
	PostEvent(EntityManager, Event);

	return Event;
}

Container::FContainerEventPayload UFaerieItemStorage::RemoveFromEntryImplNoBroadcast(FMassEntityManager& EntityManager, const FFaerieStorageEntry& Entry, const int32 Amount, const FFaerieInventoryTag Reason)
{
	// Log for this event
	Container::FContainerEventPayload Event = Container::FContainerEventPayload::MakeNew(this, Entry.GetKey(), Reason);

	// Open Mutable Scope
	{
		FFaerieStorageEntry::FReadWriteAccess Handle = Entry.GetReadWriteAccess(EntryMap);

		Event.Item = Handle->GetEntityHandle();
		const int32 Sum = Handle->StackSum();

		if (Amount == ItemData::EntireStack || Amount >= Sum) // Remove the entire entry
		{
			Event.Copies = Sum;
			Handle->CopyAddresses(Event.AddressesTouched);
			if (ItemData::IsMutable(EntityManager, Event.Item))
			{
				Container::ReleaseOwnership(EntityManager, this, Event.Item);
			}
			Event.EntryRemoved = true;
		}
		else // Remove part of the entry
		{
			Event.Copies = FMath::Clamp(Amount, 1, Sum-1);
			Handle.RemoveFromAnyStack(Event.Copies, Event.AddressesTouched);
		}
	}
	// Close Mutable scope

	if (Event.EntryRemoved)
	{
		UE_LOGF(LogFaerieInventory, Verbose, "Removing entire entry at: '%ls'", *Entry.GetKey().ToString());
		Server_PreContentRemoved(Entry, Event);
		EntryMap.Remove(Entry.GetKey());
	}
	else
	{
		// If the entry was not removed, update local proxies that counts were decremented.
		Server_PostContentChanged(Entry, Event);
	}

	return Event;
}

Container::FContainerEventPayload UFaerieItemStorage::RemoveFromStackImplNoBroadcast(FMassEntityManager& EntityManager,
	const FFaerieStorageEntry& Entry, const FFaerieStackKey Stack, const int32 Amount, const FFaerieInventoryTag Reason)
{
	// Log for this event
	Container::FContainerEventPayload Event = Container::FContainerEventPayload::MakeNew(this, Entry.GetKey(), Reason);
	Event.AddressesTouched.Add(Container::Address::Encode(Entry.GetKey(), Stack));

	// Open Mutable Scope
	{
		FFaerieStorageEntry::FReadWriteAccess Handle = Entry.GetReadWriteAccess(EntryMap);

		Event.Item = Handle->GetEntityHandle();

		if (const int32 Copies = Handle->GetStack(Stack);
			Amount == ItemData::EntireStack || Amount >= Copies) // Remove the entire stack
		{
			Event.Copies = Copies;

			// If removing this stack would remove all reference to this item, release and exit so we can remove the entry.
			if (Handle->IsOnlyStack(Stack))
			{
				if (ItemData::IsMutable(EntityManager, Handle->GetEntityHandle()))
				{
					Container::ReleaseOwnership(EntityManager, this, Handle->GetEntityHandle());
				}
				Event.EntryRemoved = true;
			}
			else
			{
				Handle.RemoveStack(Stack);
			}
		}
		else // Remove part of the stack
		{
			checkSlow(Amount == FMath::Clamp(Amount, 1, Copies-1));

			Event.Copies = Amount;

			const int32 NewAmount = Copies - Amount;
			Handle.SetStack(Stack, NewAmount);
		}
	}
	// Close Mutable scope

	if (Event.EntryRemoved)
	{
		UE_LOGF(LogFaerieInventory, Verbose, "Removing entire stack at: '%ls_%ls'", *Entry.GetKey().ToString(), *Stack.ToString());
		Server_PreContentRemoved(Entry, Event);
		EntryMap.Remove(Entry.GetKey());
	}
	else
	{
		// If the entry was not removed, update local proxies that counts were decremented.
		Server_PostContentChanged(Entry, Event);
	}

	return Event;
}

Container::FContainerEventPayload UFaerieItemStorage::RemoveFromEntryImpl(FMassEntityManager& EntityManager, const FFaerieStorageEntry& Entry,
														  const int32 Amount, const FFaerieInventoryTag Reason)
{
	SCOPE_CYCLE_COUNTER(STAT_Storage_Remove);

	// RemoveEntryImpl should not be called with unvalidated parameters.
	checkSlow(Faerie::ItemData::IsValidStackAmount(Amount));
	checkSlow(Reason.MatchesTag(Faerie::Inventory::Tags::RemovalBase))

	const Container::FContainerEventPayload Event = RemoveFromEntryImplNoBroadcast(EntityManager, Entry, Amount, Reason);

	PostEvent(EntityManager, Event);

	// Destroy the mass entity, if this stack is being deleted.
	if (Reason == Inventory::Tags::RemovalDeletion && Event.EntryRemoved)
	{
		ItemData::MarkItemAsGarbage(EntityManager, Event.Item);
	}

	return Event;
}

Container::FContainerEventPayload UFaerieItemStorage::RemoveFromStackImpl(FMassEntityManager& EntityManager, const FFaerieStorageEntry& Entry,
	const FFaerieStackKey Stack, const int32 Amount, const FFaerieInventoryTag Reason)
{
	SCOPE_CYCLE_COUNTER(STAT_Storage_Remove);

	// RemoveFromStackImpl should not be called with unvalidated parameters.
	checkSlow(Faerie::ItemData::IsValidStackAmount(Amount));
	checkSlow(Reason.MatchesTag(Faerie::Inventory::Tags::RemovalBase))

	const Container::FContainerEventPayload Event = RemoveFromStackImplNoBroadcast(EntityManager, Entry, Stack, Amount, Reason);

	PostEvent(EntityManager, Event);

	// Destroy the mass entity, if this stack is being deleted.
	if (Reason == Inventory::Tags::RemovalDeletion && Event.EntryRemoved)
	{
		ItemData::MarkItemAsGarbage(EntityManager, Event.Item);
	}

	return Event;
}

bool UFaerieItemStorage::CanEditStackImpl(const FMassEntityManager& EntityManager, const FFaerieStorageEntry& Entry, const FFaerieStackKey Stack, const FFaerieInventoryTag EditTag) const
{
	const FFaerieStorageEntry::FStackReadAccess ReadAccess = Entry.GetStackReadAccess(EntryMap, Stack);

	// By default, some edit types are allowed, unless an extension explicitly disallows it.
	const bool Default = Container::Tags::StorageEditTagsAllowedByDefault().Contains(EditTag);
	return AllowsEdit(EntityManager, &ReadAccess, EditTag, Default);
}

bool UFaerieItemStorage::CanRemoveEntryImpl(const FMassEntityManager& EntityManager, const FFaerieStorageEntry& Entry, const FFaerieInventoryTag Reason) const
{
	// By default, some removal reasons are allowed, unless an extension explicitly disallows it.
	const bool Default = Container::Tags::StorageRemovalTagsAllowedByDefault().Contains(Reason);

	for (auto It = Container::FIterator_SingleEntry_WithInterface(this, Entry); It.IsValid(); It.Advance())
	{
		if (!AllowsRemoval(EntityManager, &It, Reason, Default))
		{
			// Exit if an extension forbids removal.
			return false;
		}
	}

	return true;
}

bool UFaerieItemStorage::CanRemoveStackImpl(const FMassEntityManager& EntityManager, const FFaerieStorageEntry& Entry, const FFaerieStackKey Stack, const FFaerieInventoryTag Reason) const
{
	const FFaerieStorageEntry::FStackReadAccess ReadAccess = Entry.GetStackReadAccess(EntryMap, Stack);

	// By default, some removal reasons are allowed, unless an extension explicitly disallows it.
	const bool Default = Container::Tags::StorageRemovalTagsAllowedByDefault().Contains(Reason);
	return AllowsRemoval(EntityManager, &ReadAccess, Reason, Default);
}

void UFaerieItemStorage::Server_PostContentAdded(const FFaerieStorageEntry& Entry, const Container::FContainerEventPayload& Event) {}

void UFaerieItemStorage::Server_PreContentRemoved(const FFaerieStorageEntry& Entry, const Container::FContainerEventPayload& Event)
{
	// We should only be called when the entry is being deleted.
	check(Event.EntryRemoved);

	// Cleanup local views.
	TWeakObjectPtr<UFaerieItemEntryProxy> EntryProxy;
	LocalEntryProxies.RemoveAndCopyValue(Event.EntryTouched, EntryProxy);
	if (UFaerieItemEntryProxy* EntryProxyObject = EntryProxy.Get())
	{
		EntryProxyObject->NotifyRemoval();
	}

	for (const FFaerieAddress Address : Event.AddressesTouched)
	{
		TWeakObjectPtr<UFaerieItemStackProxy> StackProxy;
		LocalStackProxies.RemoveAndCopyValue(Address, StackProxy);
		if (UFaerieItemStackProxy* StackProxyObject = StackProxy.Get())
		{
			StackProxyObject->NotifyRemoval();
		}
	}
}

void UFaerieItemStorage::Server_PostContentChanged(const FFaerieStorageEntry& Entry, const Container::FContainerEventPayload& Event)
{
	if (const TWeakObjectPtr<UFaerieItemEntryProxy>* FoundEntry = LocalEntryProxies.Find(Event.EntryTouched))
	{
		TWeakObjectPtr<UFaerieItemEntryProxy> LocalEntryProxy = *FoundEntry;

		// Cleanup invalid entries.
		if (LocalEntryProxy.IsStale() || LocalEntryProxy.IsExplicitlyNull())
		{
			LocalEntryProxies.Remove(Event.EntryTouched);
		}
		else
		{
			LocalEntryProxy->NotifyUpdate();
		}
	}

	// Call updates on any stack proxies.
	for (const FFaerieAddress Address : Event.AddressesTouched)
	{
		if (const TWeakObjectPtr<UFaerieItemStackProxy>* FoundEntry = LocalStackProxies.Find(Address))
		{
			TWeakObjectPtr<UFaerieItemStackProxy> LocalStackProxy = *FoundEntry;

			// Cleanup invalid entries.
			if (LocalStackProxy.IsStale() || LocalStackProxy.IsExplicitlyNull())
			{
				LocalStackProxies.Remove(Address);
				continue;
			}

			FFaerieStackKey Stack;
			Container::Address::Decode_Stack(Address, Stack);

			// If we are supposed to have this key, update it.
			if (Entry.Contains(Stack))
			{
				LocalStackProxy->NotifyUpdate();
			}
			// Otherwise, discard it.
			else
			{
				LocalStackProxies.Remove(Address);
				LocalStackProxy->NotifyRemoval();
			}
		}
	}
}

void UFaerieItemStorage::Client_PostContentAdded(const FFaerieStorageEntry& Entry)
{
	if (!Entry.IsValid())
	{
		UE_LOGF(LogFaerieInventory, Error, "Client_PostContentAdded: Received Invalid Entry")
		return;
	}

	// @Todo we may want to move most/all this logic to the AckItemCreatedByNetwork function.

	// Prepare event data.
	Container::FContainerEventPayload Event = Container::FContainerEventPayload::MakeNewStack(this,
		Entry.GetEntityHandle(), Entry.StackSum(), Entry.GetKey(), {});
	CollateAddresses(Entry, Event.AddressesTouched);

	// Proxies may already exist for keys on the client if they are replicated by extensions or other means, and
	// happened to arrive before we got them.
	if (auto&& EntryProxy = LocalEntryProxies.Find(Event.EntryTouched))
	{
		if (EntryProxy->IsValid())
		{
			EntryProxy->Get()->NotifyDelayedCreation();
		}
	}

	for (FFaerieAddress Address : Event.AddressesTouched)
	{
		if (auto&& StackProxy = LocalStackProxies.Find(Address))
		{
			if (StackProxy->IsValid())
			{
				StackProxy->Get()->NotifyDelayedCreation();
			}
		}
	}

	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());
	PostEvent(EntityManager, Event);
}

void UFaerieItemStorage::Client_PreContentRemoved(const FFaerieStorageEntry& Entry)
{
	// We cannot check Entry.IsValid(), because the MassEntity may already be destroyed before this storage received this
	// removal notice.

	// Prepare event data. Item may not be valid.
	// Client_PreContentRemoved is only called by replication callback on FFaerieStorageEntry, so it only gets called
	// when the entry is being removed.
	Container::FContainerEventPayload Event = Container::FContainerEventPayload::MakeRemoval(this, Entry.GetEntityHandle(), Entry.StackSum(),
		Inventory::Tags::RemovalDeletion, Entry.GetKey(), {}, true);
	CollateAddresses(Entry, Event.AddressesTouched);

	// Cleanup local proxy objects.
	TWeakObjectPtr<UFaerieItemEntryProxy> EntryProxy;
	LocalEntryProxies.RemoveAndCopyValue(Event.EntryTouched, EntryProxy);
	if (UFaerieItemEntryProxy* EntryProxyObject = EntryProxy.Get())
	{
		EntryProxyObject->NotifyRemoval();
	}

	for (const FFaerieAddress Address : Event.AddressesTouched)
	{
		TWeakObjectPtr<UFaerieItemStackProxy> StackProxy;
		LocalStackProxies.RemoveAndCopyValue(Address, StackProxy);
		if (UFaerieItemStackProxy* StackProxyObject = StackProxy.Get())
		{
			StackProxyObject->NotifyRemoval();
		}
	}

	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());
	PostEvent(EntityManager, Event);
}

void UFaerieItemStorage::Client_PostContentChanged(const FFaerieStorageEntry& Entry)
{
	if (!ensure(Entry.IsValid()))
	{
		UE_LOGF(LogFaerieInventory, Error, "Client_PostContentChanged: Received Invalid Entry")
		return;
	}

	if (!ensure(ContainsKey(Entry.GetKey())))
	{
		// Do nothing, Client_PreContentRemoved should handle this ...
		return;
	}

	Container::FContainerEventPayload Event = Container::FContainerEventPayload::MakeNew(this, Entry.GetKey(), Inventory::Tags::ReplicationEdit);
	Event.Type = Inventory::Tags::ReplicationEdit;
	Event.Item = Entry.GetEntityHandle();
	Event.AddressesTouched.Reserve(Entry.NumStacks());

	// Call updates on all proxies for the entry.
	// @Note: We don't know what addresses got changed, only that one or more on this entry were, so we need to update
	// them all.
	//
	// Client_PostContentChanged is called when stacks are removed as well, so let's do some cleanup here.
	// Start by getting all the Addresses that we could have proxies for.
	TSet<FFaerieAddress> Addresses;
	Addresses.Reserve(Entry.NumStacks());
	for (const FFaerieKeyedStack& Stack : Entry.GetStacks())
	{
		const FFaerieAddress Address = Container::Address::Encode(Entry.GetKey(), Stack.Key);
		Addresses.Add(Address);
		Event.AddressesTouched.Add(Address);
	}

	if (TWeakObjectPtr<UFaerieItemEntryProxy>* LocalEntryProxy = LocalEntryProxies.Find(Event.EntryTouched))
	{
		// Cleanup invalid entries.
		if (LocalEntryProxy->IsStale() || LocalEntryProxy->IsExplicitlyNull())
		{
			LocalEntryProxies.Remove(Event.EntryTouched);
		}
		else
		{
			// Check for local proxies that match this entry
			if ((*LocalEntryProxy)->GetKey() == Entry.GetKey())
			{
				(*LocalEntryProxy)->NotifyUpdate();
			}
		}
	}

	for (auto It = LocalStackProxies.CreateIterator(); It; ++It)
	{
		TWeakObjectPtr<UFaerieItemStackProxy> LocalStackProxy = It.Value();

		// Cleanup invalid entries.
		if (LocalStackProxy.IsStale() || LocalStackProxy.IsExplicitlyNull())
		{
			It.RemoveCurrent();
			continue;
		}

		// Check for local proxies that match this entry
		if (LocalStackProxy->GetKey() != Entry.GetKey())
		{
			continue;
		}

		// If we are supposed to have this key, update it.
		if (Addresses.Contains(It.Key()))
		{
			LocalStackProxy->NotifyUpdate();
		}
		// Otherwise, discard it.
		else
		{
			It.RemoveCurrent();
			LocalStackProxy->NotifyRemoval();
		}
	}

	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());
	PostEvent(EntityManager, Event);
}


	/**------------------------------*/
	/*	 STORAGE API - ALL USERS   */
	/**------------------------------*/

void UFaerieItemStorage::MakeSaveData(const FMassEntityManager& EntityManager, FFaerieStorageExportData& OutItemData, const Container::FSaveParams Params) const
{
	if (Params.ExportItemData)
	{
		for (const FFaerieStorageEntry& Entry : EntryMap)
		{
			FFaerieStorageEntryExportData& ExportEntry = OutItemData.Entries.AddDefaulted_GetRef();
			ExportEntry.ExportData = ExportItemData(EntityManager, Entry.GetEntityHandle());
			Entry.CopyStacks(ExportEntry.Counts);
		}
	}

	if (Params.ExportExtensionData)
	{
		RavelExtensionData(EntityManager, OutItemData.ExtensionData);
	}
}

void UFaerieItemStorage::LoadSaveData(FMassEntityManager& EntityManager, const FFaerieStorageExportData& InItemData, const Container::FLoadParams Params)
{
	// Clear out state
	if (Params.ClearExtensionsBeforeImport)
	{
		ExtensionData.Reset();
	}

	if (Params.ClearItemDataBeforeImport)
	{
		Clear(EntityManager, Inventory::Tags::RemovalDeletion);
	}

	// See Footnote1
	//KeyGen.Reset();

	TArray<Container::FContainerEventPayload> Events;
	Events.Reserve(InItemData.Entries.Num());

	// Load in save data
	for (const FFaerieStorageEntryExportData& ExportEntry : InItemData.Entries)
	{
		FMassEntityHandle Item;
		{
			Container::FBlockTakeOwnershipScope BlockTakeOwnershipDuringImport;
			Item = ImportItemData(EntityManager, ExportEntry.ExportData);
		}

		if (!Item.IsSet())
		{
			UE_LOGF(LogFaerieInventory, Error, "Item failed to import!")
			continue;
		}

		for (const int32 Count : ExportEntry.Counts)
		{
			const FFaerieUnownedItemStack ItemStack{ Item, Count };
			Events.Add(AddStackImplNoBroadcast(EntityManager, ItemStack, true));
		}
	}

	// Rebuild extension state
	UnravelExtensionData(EntityManager, InItemData.ExtensionData);
	InitializeExtensions(EntityManager);

	PostEventBatch(EntityManager, Events);
}

FFaerieAddress UFaerieItemStorage::MakeAddress(const FFaerieEntryKey Entry, const FFaerieStackKey Stack)
{
	return Container::Address::Encode(Entry, Stack);
}

FFaerieEntryKey UFaerieItemStorage::GetAddressEntry(const FFaerieAddress Address)
{
	FFaerieEntryKey Key;
	Container::Address::Decode_Entry(Address, Key);
	return Key;
}

FFaerieStackKey UFaerieItemStorage::GetAddressStack(const FFaerieAddress Address)
{
	FFaerieStackKey Key;
	Container::Address::Decode_Stack(Address, Key);
	return Key;
}

TTuple<FFaerieEntryKey, FFaerieStackKey> UFaerieItemStorage::BreakAddress(const FFaerieAddress Address)
{
	FFaerieEntryKey Entry;
	FFaerieStackKey Stack;
	Container::Address::Decode(Address, Entry, Stack);
	return MakeTuple(Entry, Stack);
}

int32 UFaerieItemStorage::GetEntryCopies(const FFaerieEntryKey Key) const
{
	if (const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Key))
	{
		return EntryPtr->StackSum();
	}
	return 0;
}

int32 UFaerieItemStorage::GetStackCopies(const FFaerieAddress Address) const
{
	FFaerieEntryKey Entry;
	FFaerieStackKey Stack;
	Container::Address::Decode(Address, Entry, Stack);
	if (const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Entry))
	{
		return EntryPtr->GetStack(Stack);
	}
	return 0;
}

const UFaerieItemStackProxy* UFaerieItemStorage::GetProxy(const FFaerieAddress Address) const
{
	return GetStackProxyImpl(Address);
}

bool UFaerieItemStorage::BreakAddressIntoKeys(const FFaerieAddress Address, FFaerieEntryKey& Entry, FFaerieStackKey& Stack) const
{
	Container::Address::Decode(Address, Entry, Stack);
	if (const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Entry))
	{
		return EntryPtr->Contains(Stack);
	}
	return false;
}

TArray<FFaerieStackKey> UFaerieItemStorage::BreakEntryIntoKeys(const FFaerieEntryKey Key) const
{
	TArray<FFaerieStackKey> Out;
	if (const FFaerieStorageEntry* Entry = GetEntrySafe(Key))
	{
		Entry->CopyKeys(Out);
	}
	return Out;
}

TArray<int32> UFaerieItemStorage::GetStacksInEntry(const FFaerieEntryKey Key) const
{
	TArray<int32> Out;
	if (const FFaerieStorageEntry* Entry = GetEntrySafe(Key))
	{
		Entry->CopyStacks(Out);
	}
	return Out;
}

TArray<FFaerieAddress> UFaerieItemStorage::GetAddressesForEntry(const FFaerieEntryKey Key) const
{
	TArray<FFaerieAddress> Out;

	if (const FFaerieStorageEntry* Entry = GetEntrySafe(Key))
	{
		Out.Reserve(Entry->NumStacks());
		for (const FFaerieKeyedStack& Stack : Entry->GetStacks())
		{
			Out.Add(Container::Address::Encode(Key, Stack.Key));
		}

		checkSlow(!Out.IsEmpty())
	}

	return Out;
}

void UFaerieItemStorage::GetAllKeys(TArray<FFaerieEntryKey>& Keys) const
{
	Keys.Reset();
	CopyEntryKeys(Keys);
}

int32 UFaerieItemStorage::GetEntryCount() const
{
	return EntryMap.Num();
}

int32 UFaerieItemStorage::GetStackCount() const
{
	int32 Stacks = 0;
	for (const FFaerieStorageEntry& Entry : EntryMap)
	{
		Stacks += Entry.NumStacks();
	}
	return Stacks;
}

bool UFaerieItemStorage::ContainsKey(const FFaerieEntryKey Key) const
{
	return EntryMap.Contains(Key);
}

bool UFaerieItemStorage::ContainsAddress(const FFaerieAddress Address) const
{
	FFaerieEntryKey Entry;
	FFaerieStackKey Stack;
	Container::Address::Decode(Address, Entry, Stack);
	if (const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Entry))
	{
		return EntryPtr->Contains(Stack);
	}
	return false;
}

bool UFaerieItemStorage::ContainsItem(const FFaerieItemProxy& Item) const
{
	if (const UFaerieItemStackProxy* StackProxy = Cast<UFaerieItemStackProxy>(Item.GetProxyObject()))
	{
		if (StackProxy->GetOuter() != this)
		{
			// This proxy does not belong to this storage.
			return false;
		}

		return ContainsAddress(StackProxy->GetAddress());
	}

	if (const UFaerieItemEntryProxy* EntryProxy = Cast<UFaerieItemEntryProxy>(Item.GetProxyObject()))
	{
		if (EntryProxy->GetOuter() != this)
		{
			// This proxy does not belong to this storage.
			return false;
		}

		return ContainsKey(EntryProxy->GetKey());
	}

	return false;
}

FFaerieEntryKey UFaerieItemStorage::FindItem(const FFaerieItemProxy& Item) const
{
	if (const UFaerieItemStackProxy* StackProxy = Cast<UFaerieItemStackProxy>(Item.GetProxyObject()))
	{
		if (StackProxy->GetOuter() != this)
		{
			// This proxy does not belong to this storage.
			return FFaerieEntryKey::InvalidKey;
		}

		const FFaerieEntryKey EntryKey = StackProxy->GetKey();
		if (ContainsKey(EntryKey))
		{
			return EntryKey;
		}
		return FFaerieEntryKey::InvalidKey;
	}

	if (const UFaerieItemEntryProxy* EntryProxy = Cast<UFaerieItemEntryProxy>(Item.GetProxyObject()))
	{
		if (EntryProxy->GetOuter() != this)
		{
			// This proxy does not belong to this storage.
			return FFaerieEntryKey::InvalidKey;
		}

		const FFaerieEntryKey EntryKey = EntryProxy->GetKey();
		if (ContainsKey(EntryKey))
		{
			return EntryKey;
		}
		return FFaerieEntryKey::InvalidKey;
	}

	return FFaerieEntryKey::InvalidKey;
}

FFaerieAddress UFaerieItemStorage::GetFirstAddress() const
{
	if (EntryMap.IsEmpty()) return FFaerieAddress();
	const FFaerieStorageEntry& FirstEntry = EntryMap.Entries[0];
	return Container::Address::Encode(FirstEntry.GetKey(), FirstEntry.GetStacks()[0].Key);
}

bool UFaerieItemStorage::CanAddStack(const FMassEntityManager& EntityManager, const FFaerieItemProxy& Proxy, const EFaerieStorageAddStackBehavior AddStackBehavior) const
{
	const FMassEntityHandle Item = Proxy.GetEntityHandle();
	if (!Item.IsSet())
	{
		return false;
	}

	// Prevent recursive storage: don't allow adding an item that contains ourself.
	if (SubObject::HasContainerInInstanceRecursive_Storage(EntityManager, Item, this))
	{
		return false;
	}

	const FFaerieExtensionAllowsAdditionArgs CanAddStackArgs {
		.AddStackBehavior = AddStackBehavior
	};

	return AllowsAddition(EntityManager, MakeConstArrayView(&Proxy, 1), CanAddStackArgs, true);
}

bool UFaerieItemStorage::CanAddStacks(const FMassEntityManager& EntityManager, const TArray<FFaerieItemProxy>& Proxies, const FFaerieExtensionAllowsAdditionArgs Args) const
{
	if (Proxies.IsEmpty())
	{
		return false;
	}

	for (const FFaerieItemProxy& Proxy : Proxies)
	{
		const FMassEntityHandle Item = Proxy.GetEntityHandle();
		if (!Item.IsSet())
		{
			return false;
		}

		// Prevent recursive storage: don't allow adding an item that contains ourself.
		if (SubObject::HasContainerInInstanceRecursive_Storage(EntityManager, Item, this))
		{
			return false;
		}
	}

	return AllowsAddition(EntityManager, Proxies, Args, true);
}

bool UFaerieItemStorage::CanAddStacks(const FMassEntityManager& EntityManager,
	const Utils::TArrayAdapter<FFaerieItemProxy>& Proxies, const FFaerieExtensionAllowsAdditionArgs Args) const
{
	if (Proxies.Num() == 0)
	{
		return false;
	}

	for (int32 i = 0; i < Proxies.Num(); ++i)
	{
		const FFaerieItemProxy Proxy = Proxies[i];

		const FMassEntityHandle Item = Proxy.GetEntityHandle();
		if (!Item.IsSet())
		{
			return false;
		}

		// Prevent recursive storage: don't allow adding an item that contains ourself.
		if (SubObject::HasContainerInInstanceRecursive_Storage(EntityManager, Item, this))
		{
			return false;
		}
	}

	return AllowsAddition(EntityManager, Proxies, Args, true);
}

bool UFaerieItemStorage::CanEditStack(const FMassEntityManager& EntityManager, const FFaerieAddress Address, const FFaerieInventoryTag EditTag) const
{
	FFaerieEntryKey Entry;
	FFaerieStackKey Stack;
	Container::Address::Decode(Address, Entry, Stack);
	if (const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Entry))
	{
		return CanEditStackImpl(EntityManager, *EntryPtr, Stack, EditTag);
	}
	return false;
}

bool UFaerieItemStorage::CanRemoveEntry(const FMassEntityManager& EntityManager, const FFaerieEntryKey Key, const FFaerieInventoryTag Reason) const
{
	if (const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Key))
	{
		return CanRemoveEntryImpl(EntityManager, *EntryPtr, Reason);
	}
	return false;
}

bool UFaerieItemStorage::CanRemoveStack(const FMassEntityManager& EntityManager, const FFaerieAddress Address, const FFaerieInventoryTag Reason) const
{
	// By default, some removal reasons are allowed, unless an extension explicitly disallows it.
	const bool Default = Container::Tags::StorageRemovalTagsAllowedByDefault().Contains(Reason);

	FFaerieEntryKey Entry;
	FFaerieStackKey Stack;
	Container::Address::Decode(Address, Entry, Stack);
	if (const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Entry))
	{
		const FFaerieStorageEntry::FStackReadAccess ReadAccess = EntryPtr->GetStackReadAccess(EntryMap, Stack);

		if (!AllowsRemoval(EntityManager, &ReadAccess, Reason, Default))
		{
			return false;
		}
	}

	return true;
}


	/**---------------------------------*/
	/*	 STORAGE API - AUTHORITY ONLY   */
	/**---------------------------------*/

bool UFaerieItemStorage::AddItemStack(FMassEntityManager& EntityManager, const FFaerieUnownedItemStack& Stack,
	const EFaerieStorageAddStackBehavior AddStackBehavior)
{
	if (!Stack.IsValid()) return false;

	const ItemData::FScopeProxy Proxy(Stack.Item, Stack.Copies);
	if (!CanAddStack(EntityManager, FFaerieItemProxy(FFaerieItemProxy::ESingleFrame, EntityManager.AsShared(), &Proxy), AddStackBehavior))
	{
		return false;
	}

	(void)AddStackImpl(EntityManager, Stack, Container::IfOnlyNewStacks(AddStackBehavior));
	return true;
}

void UFaerieItemStorage::AddItemStack(FMassEntityManager& EntityManager, const TValid<const FFaerieUnownedItemStack&> Stack,
	const EFaerieStorageAddStackBehavior AddStackBehavior, TValueOrError<Container::FContainerEventPayload, FText>& OutResult)
{
	const ItemData::FScopeProxy Proxy(ValidGet(Stack).Item, ValidGet(Stack).Copies);
	if (!CanAddStack(EntityManager, FFaerieItemProxy(FFaerieItemProxy::ESingleFrame, EntityManager.AsShared(), &Proxy), AddStackBehavior))
	{
		OutResult = MakeError(Container::AddItemStackFailure_FailedCanAddStack);
		return;
	}

	OutResult = MakeValue(AddStackImpl(EntityManager, Stack, Container::IfOnlyNewStacks(AddStackBehavior)));
}

bool UFaerieItemStorage::AddItemStacks(FMassEntityManager& EntityManager, const Utils::TArrayAdapter<FFaerieUnownedItemStack>& Adapter,
	const EFaerieStorageAddStackBehavior AddStackBehavior, const bool StopAfterFailure)
{
	SCOPE_CYCLE_COUNTER(STAT_Storage_AddMulti);

	const bool ForceNewStack = Container::IfOnlyNewStacks(AddStackBehavior);

	TArray<Container::FContainerEventPayload> Events;
	Events.Reserve(Adapter.Num());

	ON_SCOPE_EXIT
	{
		if (!Events.IsEmpty())
		{
			// Execute PostEventBatch on all extensions with the finished Event
			PostEventBatch(EntityManager, Events);
		}
	};

	for (int32 i = 0; i < Adapter.Num(); ++i)
	{
		const FFaerieUnownedItemStack& Stack = Adapter[i];

		if (!ensureAlwaysMsgf(
			Stack.IsValid(),
			TEXT("AddStackImpl was passed an invalid item view.")))
		{
			continue;
		}

		const ItemData::FScopeProxy Proxy(Stack.Item, Stack.Copies);
		if (!CanAddStack(EntityManager, FFaerieItemProxy(FFaerieItemProxy::ESingleFrame, EntityManager.AsShared(), &Proxy), AddStackBehavior))
		{
			if (StopAfterFailure)
			{
				return false;
			}
			continue;
		}

		Events.Add(AddStackImplNoBroadcast(EntityManager, Stack, ForceNewStack));
	}

	return true;
}

bool UFaerieItemStorage::RemoveEntry(FMassEntityManager& EntityManager, const FFaerieEntryKey Key,
	const FFaerieInventoryTag RemovalTag, const int32 Amount)
{
	if (Amount == 0 || Amount < ItemData::EntireStack) return false;
	if (!RemovalTag.IsValid()) return false;

	if (const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Key))
	{
		if (CanRemoveEntryImpl(EntityManager, *EntryPtr, RemovalTag))
		{
			(void)RemoveFromEntryImpl(EntityManager, *EntryPtr, Amount, RemovalTag);
			return true;
		}
	}
	return false;
}

bool UFaerieItemStorage::RemoveStack(FMassEntityManager& EntityManager, const FFaerieAddress Address, const FFaerieInventoryTag RemovalTag, const int32 Amount)
{
	if (Amount == 0 || Amount < ItemData::EntireStack) return false;
	if (!RemovalTag.IsValid()) return false;

	FFaerieEntryKey Entry;
	FFaerieStackKey Stack;
	Container::Address::Decode(Address, Entry, Stack);
	if (const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Entry))
	{
		if (CanRemoveStackImpl(EntityManager, *EntryPtr, Stack, RemovalTag))
		{
			(void)RemoveFromStackImpl(EntityManager, *EntryPtr, Stack, Amount, RemovalTag);
			return true;
		}
	}
	return false;
}

bool UFaerieItemStorage::TakeEntry(FMassEntityManager& EntityManager, const FFaerieEntryKey Key,
	FFaerieUnownedItemStack& OutStack, const  FFaerieInventoryTag RemovalTag, const  int32 Amount)
{
	if (Amount == 0 || Amount < ItemData::EntireStack) return false;
	if (!RemovalTag.IsValid()) return false;

	if (const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Key))
	{
		if (CanRemoveEntryImpl(EntityManager, *EntryPtr, RemovalTag))
		{
			const Container::FContainerEventPayload Event = RemoveFromEntryImpl(EntityManager, *EntryPtr, Amount, RemovalTag);
			OutStack = FFaerieUnownedItemStack(Event.Item, Event.Copies);
			return true;
		}
	}

	return false;
}

bool UFaerieItemStorage::TakeStack(FMassEntityManager& EntityManager, const FFaerieAddress Address,
	FFaerieUnownedItemStack& OutStack, const FFaerieInventoryTag RemovalTag, const int32 Amount)
{
	if (Amount == 0 || Amount < ItemData::EntireStack) return false;
	if (!RemovalTag.IsValid()) return false;

	FFaerieEntryKey Entry;
	FFaerieStackKey Stack;
	Container::Address::Decode(Address, Entry, Stack);
	if (const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Entry))
	{
		if (CanRemoveStackImpl(EntityManager, *EntryPtr, Stack, RemovalTag))
		{
			const Container::FContainerEventPayload Event = RemoveFromStackImpl(EntityManager, *EntryPtr, Stack, Amount, RemovalTag);
			OutStack = FFaerieUnownedItemStack(Event.Item, Event.Copies);
			return true;
		}
	}

	return false;
}

FFaerieEntryKey UFaerieItemStorage::MoveStack(FMassEntityManager& EntityManager, UFaerieItemStorage* ToStorage,
	const FFaerieAddress Address, const int32 Amount, const EFaerieStorageAddStackBehavior AddStackBehavior)
{
	if (!IsValid(ToStorage) ||
		ToStorage == this ||
		!ItemData::IsValidStackAmount(Amount))
	{
		return FFaerieEntryKey::InvalidKey;
	}

	// Verify the stack exists.
	FFaerieEntryKey Entry;
	FFaerieStackKey Stack;
	Container::Address::Decode(Address, Entry, Stack);
	const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Entry);
	if (!EntryPtr)
	{
		return FFaerieEntryKey::InvalidKey;
	}

	if (!CanRemoveStackImpl(EntityManager, *EntryPtr, Stack, Inventory::Tags::RemovalMoving))
	{
		return FFaerieEntryKey::InvalidKey;
	}

	const int32 StackValue = EntryPtr->GetStack(Stack);
	if (0 >= StackValue)
	{
		return FFaerieEntryKey::InvalidKey;
	}

	ItemData::FScopeProxy View(EntryPtr->GetEntityHandle(), StackValue);
	if (Amount > 0)
	{
		View.SetCopies(FMath::Min(StackValue, Amount));
	}

	if (!ToStorage->CanAddStack(EntityManager, FFaerieItemProxy(FFaerieItemProxy::ESingleFrame, EntityManager.AsShared(), &View), AddStackBehavior))
	{
		return FFaerieEntryKey::InvalidKey;
	}

	const Container::FContainerEventPayload Event = RemoveFromStackImpl(EntityManager, *EntryPtr, Stack, Amount, Inventory::Tags::RemovalMoving);

	const FFaerieUnownedItemStack ItemStack(Event.Item, Event.Copies);
	return ToStorage->AddStackImpl(EntityManager, ItemStack, Container::IfOnlyNewStacks(AddStackBehavior)).EntryTouched;
}

FFaerieEntryKey UFaerieItemStorage::MoveEntry(FMassEntityManager& EntityManager, UFaerieItemStorage* ToStorage, const FFaerieEntryKey Key, const EFaerieStorageAddStackBehavior AddStackBehavior)
{
	if (!IsValid(ToStorage) ||
	ToStorage == this)
	{
		return FFaerieEntryKey::InvalidKey;
	}

	// Verify the stack exists.
	const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Key);
	if (!EntryPtr)
	{
		return FFaerieEntryKey::InvalidKey;
	}

	if (!CanRemoveEntryImpl(EntityManager, *EntryPtr, Inventory::Tags::RemovalMoving))
	{
		return FFaerieEntryKey::InvalidKey;
	}

	const FFaerieStorageEntry::FReadAccess ReadAccess = EntryPtr->GetReadAccess(EntryMap);
	if (!ToStorage->CanAddStack(EntityManager, FFaerieItemProxy(FFaerieItemProxy::ESingleFrame, EntityManager.AsShared(), &ReadAccess), AddStackBehavior))
	{
		return FFaerieEntryKey::InvalidKey;
	}

	const Container::FContainerEventPayload RemoveResult = RemoveFromEntryImpl(EntityManager, *EntryPtr,
		ItemData::EntireStack, Inventory::Tags::RemovalMoving);

	const FFaerieUnownedItemStack ItemStack(RemoveResult.Item, RemoveResult.Copies);
	return ToStorage->AddStackImpl(EntityManager, ItemStack, Container::IfOnlyNewStacks(AddStackBehavior)).EntryTouched;
}

bool UFaerieItemStorage::MergeStacks(FMassEntityManager& EntityManager, const FFaerieEntryKey Entry,
	const FFaerieStackKey FromStack, const FFaerieStackKey ToStack, const int32 Amount)
{
	const FFaerieAddress FromAddress = Container::Address::Encode(Entry, FromStack);
	const FFaerieAddress ToAddress = Container::Address::Encode(Entry, ToStack);

	// Verify the stack exists.
	const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Entry);
	if (!EntryPtr)
	{
		return false;
	}

	if (!CanEditStackImpl(EntityManager, *EntryPtr, FromStack, Container::Tags::Merge) ||
		!CanEditStackImpl(EntityManager, *EntryPtr, ToStack, Container::Tags::Merge))
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

	Container::FContainerEventPayload Event = Container::FContainerEventPayload::MakeNew(this, Entry, Container::Tags::Merge);
	Event.Copies = AmountB; // Initially store the amount in stack B here.
	Event.Item = EntryPtr->GetEntityHandle();
	Event.AddressesTouched.Add(FromAddress);
	Event.AddressesTouched.Add(ToAddress);

	// Open Mutable Scope
	{
		FFaerieStorageEntry::FReadWriteAccess Handle = EntryPtr->GetReadWriteAccess(EntryMap);
		const int32 Remainder = Handle.MoveStack(FromStack, ToStack, Amount);

		// We didn't move this many.
		Event.Copies -= Remainder;

		Server_PostContentChanged(Handle.Get(), Event);
	}
	// Close Mutable scope

	PostEvent(EntityManager, Event);

	return true;
}

bool UFaerieItemStorage::SplitStack(FMassEntityManager& EntityManager, const FFaerieAddress Address, const int32 Amount)
{
	// Decode and verify the stack exists.
	FFaerieEntryKey Entry;
	FFaerieStackKey Stack;
	Container::Address::Decode(Address, Entry, Stack);
	const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Entry);
	if (!EntryPtr)
	{
		return false;
	}

	// Check if we can edit the amount requested
	if (!CanEditStackImpl(EntityManager, *EntryPtr, Stack, Container::Tags::Split))
	{
		return false;
	}

	// Validate that the requested amount is less than what's in the stack
	if (Amount >= EntryPtr->GetStack(Stack))
	{
		return false;
	}

	Container::FContainerEventPayload Event = Container::FContainerEventPayload::MakeNew(this, Entry, Container::Tags::Split);
	Event.Item = EntryPtr->GetEntityHandle();
	Event.Copies = Amount;
	Event.AddressesTouched.Add(Address);

	// Split the stack
	{
		FFaerieStorageEntry::FReadWriteAccess Handle = EntryPtr->GetReadWriteAccess(EntryMap);
		const FFaerieStackKey SplitStack = Handle.SplitStack(Stack, Amount);

		// Update event with final Address information
		Event.AddressesTouched.Add(Container::Address::Encode(Entry, SplitStack));

		Server_PostContentChanged(Handle.Get(), Event);
	}

	PostEvent(EntityManager, Event);

	return true;
}

void UFaerieItemStorage::Clear(FMassEntityManager& EntityManager, FFaerieInventoryTag Reason)
{
	if (!Reason.IsValid() || !Reason.MatchesTag(Inventory::Tags::RemovalBase))
	{
		Reason = Inventory::Tags::RemovalDeletion;
	}

	TArray<FFaerieEntryKey> Entries;
	CopyEntryKeys(Entries);

	TArray<Container::FContainerEventPayload> Events;
	Events.Reserve(Entries.Num());

	for (const FFaerieEntryKey EntryKey : Entries)
	{
		const FFaerieStorageEntry& Entry = EntryMap[EntryKey];

		if (!CanRemoveEntryImpl(EntityManager, Entry, Inventory::Tags::RemovalMoving))
		{
			continue;
		}

		// RemoveFromEntryImplNoBroadcast should not be called with unvalidated parameters.
		Events.Add(RemoveFromEntryImplNoBroadcast(EntityManager, Entry, ItemData::EntireStack, Reason));
	}

	PostEventBatch(EntityManager, Events);

	if (Reason == Inventory::Tags::RemovalDeletion)
	{
		for (const Container::FContainerEventPayload& Event : Events)
		{
			ItemData::MarkItemAsGarbage(EntityManager, Event.Item);
		}
	}

#if WITH_EDITOR
	// The editor should reset the KeyGen, so that clearing and generating new content doesn't rack up the key endlessly.
	if (GEngine->IsEditor())
	{
		KeyGen.Reset();
	}

	// See Footnote1
#endif
}

void UFaerieItemStorage::Dump(FMassEntityManager& EntityManager, UFaerieItemStorage* ToStorage)
{
	if (!IsValid(ToStorage) ||
		ToStorage == this)
	{
		return;
	}

	static constexpr EFaerieStorageAddStackBehavior DumpBehavior = EFaerieStorageAddStackBehavior::AddToAnyStack;

	TArray<FFaerieEntryKey> Entries;
	CopyEntryKeys(Entries);

	TArray<Container::FContainerEventPayload> Events;
	Events.Reserve(Entries.Num());

	TArray<FFaerieUnownedItemStack> Stacks;
	Stacks.Reserve(Entries.Num());

	for (const FFaerieEntryKey EntryKey : Entries)
	{
		const FFaerieStorageEntry& Entry = EntryMap[EntryKey];

		if (!CanRemoveEntryImpl(EntityManager, Entry, Inventory::Tags::RemovalMoving))
		{
			continue;
		}

		const FFaerieStorageEntry::FReadAccess ReadAccess = Entry.GetReadAccess(EntryMap);
		if (!ToStorage->CanAddStack(EntityManager, FFaerieItemProxy(FFaerieItemProxy::ESingleFrame, EntityManager.AsShared(), &ReadAccess), DumpBehavior))
		{
			continue;
		}

		Container::FContainerEventPayload& Event = Events.Add_GetRef(RemoveFromEntryImplNoBroadcast(EntityManager, Entry, ItemData::EntireStack, Inventory::Tags::RemovalMoving));

		Stacks.Emplace(Event.Item, Event.Copies);
	}

	PostEventBatch(EntityManager, Events);

	ToStorage->AddItemStacks(EntityManager, Stacks, DumpBehavior, false);
}

	/**---------------------------------*/
	/*			BLUEPRINT API			*/
	/**---------------------------------*/

bool UFaerieItemStorage::CanAddStack(const FFaerieItemProxy& Proxy, const EFaerieStorageAddStackBehavior AddStackBehavior) const
{
	return CanAddStack(ItemData::GetFaerieEntityManagerChecked(GetWorld()), Proxy, AddStackBehavior);
}

bool UFaerieItemStorage::CanAddStacks(const TArray<FFaerieItemProxy>& Proxies, const FFaerieExtensionAllowsAdditionArgs Args) const
{
	return CanAddStacks(ItemData::GetFaerieEntityManagerChecked(GetWorld()), Proxies, Args);
}

bool UFaerieItemStorage::CanEditStack(const FFaerieAddress Address, const FFaerieInventoryTag EditTag) const
{
	return CanEditStack(ItemData::GetFaerieEntityManagerChecked(GetWorld()), Address, EditTag);
}

bool UFaerieItemStorage::CanRemoveEntry(const FFaerieEntryKey Key, const FFaerieInventoryTag Reason) const
{
	return CanRemoveEntry(ItemData::GetFaerieEntityManagerChecked(GetWorld()), Key, Reason);
}

bool UFaerieItemStorage::CanRemoveStack(const FFaerieAddress Address, const FFaerieInventoryTag Reason) const
{
	return CanRemoveStack(ItemData::GetFaerieEntityManagerChecked(GetWorld()), Address, Reason);
}

bool UFaerieItemStorage::AddItemStack(const FFaerieUnownedItemStack& Stack, const EFaerieStorageAddStackBehavior AddStackBehavior)
{
	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());
	return AddItemStack(EntityManager, Stack, AddStackBehavior);
}

void UFaerieItemStorage::AddItemStackBulk(const TArray<FFaerieUnownedItemStack>& Stacks, const EFaerieStorageAddStackBehavior AddStackBehavior, const bool StopAfterFailure)
{
	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());
	AddItemStacks(EntityManager, Stacks, AddStackBehavior, StopAfterFailure);
}

bool UFaerieItemStorage::AddItemStackWithLog(const FFaerieUnownedItemStack& Stack, const EFaerieStorageAddStackBehavior AddStackBehavior, FFaerieBlueprintInventoryEvent& Event)
{
	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());
	TValueOrError<Container::FContainerEventPayload, FText> Result = MakeError(FText::GetEmpty());
	AddItemStack(EntityManager, Stack, AddStackBehavior, Result);
	if (Result.HasValue())
	{
		Event = FFaerieBlueprintInventoryEvent::FromNativeEvent(Result.GetValue());
	}
	return false;
}

bool UFaerieItemStorage::RemoveEntry(const FFaerieEntryKey Key, const FFaerieInventoryTag RemovalTag, const int32 Amount)
{
	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());
	return RemoveEntry(EntityManager, Key, RemovalTag, Amount);
}

bool UFaerieItemStorage::RemoveStack(const FFaerieAddress Address, const FFaerieInventoryTag RemovalTag, const int32 Amount)
{
	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());
	return RemoveStack(EntityManager, Address, RemovalTag, Amount);
}

bool UFaerieItemStorage::TakeEntry(const FFaerieEntryKey Key, FFaerieUnownedItemStack& OutStack,
								   const FFaerieInventoryTag RemovalTag, const int32 Amount)
{
	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());
	return TakeEntry(EntityManager, Key, OutStack, RemovalTag, Amount);
}

bool UFaerieItemStorage::TakeStack(const FFaerieAddress Address, FFaerieUnownedItemStack& OutStack,
								   const FFaerieInventoryTag RemovalTag, const int32 Amount)
{
	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());
	return TakeStack(EntityManager, Address, OutStack, RemovalTag, Amount);
}

FFaerieEntryKey UFaerieItemStorage::MoveStack(UFaerieItemStorage* ToStorage, const FFaerieAddress Address, const int32 Amount, const EFaerieStorageAddStackBehavior AddStackBehavior)
{
	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());
	return MoveStack(EntityManager, ToStorage, Address, Amount, AddStackBehavior);
}

FFaerieEntryKey UFaerieItemStorage::MoveEntry(UFaerieItemStorage* ToStorage, const FFaerieEntryKey Key, const EFaerieStorageAddStackBehavior AddStackBehavior)
{
	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());
	return MoveEntry(EntityManager, ToStorage, Key, AddStackBehavior);
}

bool UFaerieItemStorage::MergeStacks(const FFaerieEntryKey Entry, const FFaerieStackKey FromStack, const FFaerieStackKey ToStack, const int32 Amount)
{
	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());
	return MergeStacks(EntityManager, Entry, FromStack, ToStack, Amount);
}

bool UFaerieItemStorage::SplitStack(const FFaerieAddress Address, const int32 Amount)
{
	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());
	return SplitStack(EntityManager, Address, Amount);
}

void UFaerieItemStorage::Clear(const FFaerieInventoryTag RemovalTag)
{
	Clear(ItemData::GetFaerieEntityManagerChecked(GetWorld()), RemovalTag);
}

void UFaerieItemStorage::Dump(UFaerieItemStorage* ToStorage)
{
	Dump(ItemData::GetFaerieEntityManagerChecked(GetWorld()), ToStorage);
}

#undef LOCTEXT_NAMESPACE

/*
 * Footnote1: You might think that even at runtime we could reset the key during Clear, since all items are removed,
 * and therefor no entries exist, making 100 a valid starting point again, *except* that other entities might still
 * be holding onto FFaerieEntryKeys, which could be cached in at some point later when potentially the entry is once more
 * valid, but with a completely different item. So during runtime, the Key must always increment.
 */