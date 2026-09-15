// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemStorage.h"
#include "FaerieContainerEvent.h"
#include "FaerieInventoryLog.h"
#include "FaerieItem.h"
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

	static const FText AdditionFailure_FailedCanAddStack = LOCTEXT("AdditionFailure_FailedCanAddStack", "Refused by CanAddStack");
}

namespace Faerie::Inventory
{
	UE_DEFINE_GAMEPLAY_TAG_TYPED(FFaerieInventoryTag, ProxyCreated, "Fae.Inventory.ProxyCreated")
	UE_DEFINE_GAMEPLAY_TAG_TYPED(FFaerieInventoryTag, ProxyUpdated, "Fae.Inventory.ProxyUpdated")
	UE_DEFINE_GAMEPLAY_TAG_TYPED(FFaerieInventoryTag, ProxyRemoved, "Fae.Inventory.ProxyRemoved")
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

FInstancedStruct UFaerieItemStorage::MakeSaveData(const Container::FSaveParams Params) const
{
	FFaerieStorageExportData ExportData;
	MakeSaveData(ExportData, Params);
	return FInstancedStruct::Make(ExportData);
}

void UFaerieItemStorage::LoadSaveData(const FConstStructView ItemData, const Container::FLoadParams Params)
{
	if (ItemData.GetScriptStruct() != FFaerieStorageExportData::StaticStruct())
	{
		UE_LOGF(LogFaerieInventory, Error, "Invalid data to load from. Must be FFaerieStorageExportData.")
		return;
	}
	LoadSaveData(ItemData.Get<const FFaerieStorageExportData>(), Params);
}

bool UFaerieItemStorage::Contains(const FFaerieAddress Address) const
{
	return ContainsAddress(Address);
}

TOptional<FFaerieItemInstance> UFaerieItemStorage::ViewInstance(const FFaerieEntryKey Key) const
{
	if (const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Key))
	{
		return EntryPtr->GetInstance();
	}
	return NullOpt;
}

TOptional<FFaerieItemInstance> UFaerieItemStorage::ViewInstance(const FFaerieAddress Address) const
{
	FFaerieEntryKey Entry;
	Container::Address::Decode_Entry(Address, Entry);
	if (const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Entry))
	{
		return EntryPtr->GetInstance();
	}
	return NullOpt;
}

ItemData::FScopeProxy UFaerieItemStorage::ViewEntry(const FFaerieEntryKey Key) const
{
	if (const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Key))
	{
		return ItemData::FScopeProxy(EntryPtr->GetInstance(), EntryPtr->StackSum(), this);
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
		return ItemData::FScopeProxy(EntryPtr->GetInstance(), EntryPtr->GetStack(Stack), this);
	}
	return nullptr;
}

FFaerieItemProxy UFaerieItemStorage::Proxy(const FFaerieEntryKey Key) const
{
	return FFaerieItemProxy(GetEntryProxyImpl(Key));
}

FFaerieItemProxy UFaerieItemStorage::Proxy(const FFaerieAddress Address) const
{
	return FFaerieItemProxy(GetStackProxyImpl(Address));
}

bool UFaerieItemStorage::Possess(const FFaerieUnownedItemStack& Stack)
{
	const ItemData::FScopeProxy Proxy(Stack.Instance, Stack.Copies, nullptr);
	if (!CanAddStack(FFaerieItemProxy(FFaerieItemProxy::ESingleFrame, &Proxy), EFaerieStorageAddStackBehavior::AddToAnyStack)) return false;

	(void)AddStackImpl(Stack, false);
	return true;
}

void UFaerieItemStorage::DestroyStack(const FFaerieEntryKey Key, const int32 Copies)
{
	FFaerieUnownedItemStack OutStack;
	TakeEntry(Key, OutStack, Inventory::Tags::RemovalDeletion, Copies);
}

void UFaerieItemStorage::DestroyStack(const FFaerieAddress Address, const int32 Copies)
{
	FFaerieUnownedItemStack OutStack;
	TakeStack(Address, OutStack, Inventory::Tags::RemovalDeletion, Copies);
}

void UFaerieItemStorage::DestroyStack(const FFaerieItemProxy& Proxy, const int32 Copies)
{
	if (const UFaerieItemStackProxy* StackProxy = Cast<UFaerieItemStackProxy>(Proxy.GetProxyObject()))
	{
		if (StackProxy->GetOuter() != this)
		{
			UE_LOGF(LogFaerieInventory, Error, "This isn't our proxy! We cannot release copies from it.")
			return;
		}

		FFaerieUnownedItemStack OutStack;
		TakeStack(StackProxy->Address, OutStack, Inventory::Tags::RemovalDeletion, Copies);
	}
}

TOptional<FFaerieUnownedItemStack> UFaerieItemStorage::Release(const FFaerieEntryKey Key, const int32 Copies, const FFaerieInventoryTag Reason)
{
	if (FFaerieUnownedItemStack OutStack;
		TakeEntry(Key, OutStack, Reason, Copies))
	{
		return OutStack;
	}
	return NullOpt;
}

TOptional<FFaerieUnownedItemStack> UFaerieItemStorage::Release(const FFaerieAddress Address, const int32 Copies, const FFaerieInventoryTag Reason)
{
	if (FFaerieUnownedItemStack OutStack;
		TakeStack(Address, OutStack, Reason, Copies))
	{
		return OutStack;
	}
	return NullOpt;
}

bool UFaerieItemStorage::CanPossess(const FFaerieItemProxy& Proxy) const
{
	return CanAddStack(Proxy, EFaerieStorageAddStackBehavior::AddToAnyStack);
}

bool UFaerieItemStorage::CanRelease(const FFaerieItemProxy& Proxy, const FFaerieInventoryTag Reason) const
{
	if (const UFaerieItemStackProxy* StackProxy = Cast<UFaerieItemStackProxy>(Proxy.GetProxyObject()))
	{
		return CanRemoveStack(StackProxy->Address, Reason);
	}
	if (const UFaerieItemEntryProxy* EntryProxy = Cast<UFaerieItemEntryProxy>(Proxy.GetProxyObject()))
	{
		return CanRemoveEntry(EntryProxy->Key, Reason);
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

void UFaerieItemStorage::OnItemDataChanged(const FFaerieItemInstance& Instance, const FGameplayTag EditTag)
{
	Super::OnItemDataChanged(Instance, EditTag);

	if (const FFaerieStorageEntry* Entry = FindEntry(Instance))
	{
		// Mutable entries should not generate Entry Proxies, see GetEntryProxyImpl
		check(LocalEntryProxies.Contains(Entry->GetKey()))

		// There should only be a single address for a mutable item, so only try to update one.
		const FFaerieAddress Address = Entry->FirstAddress();

		if (const TWeakObjectPtr<UFaerieItemStackProxy>* Proxy = LocalStackProxies.Find(Address);
			Proxy && Proxy->IsValid())
		{
			Proxy->Get()->NotifyItemDataChanged(EditTag);
		}
	}
}


	/**------------------------------*/
	/*	  INTERNAL IMPLEMENTATIONS	 */
	/**------------------------------*/

void UFaerieItemStorage::CopyEntryKeys(TAdderReserverRef<FFaerieEntryKey> OutKeys) const
{
	OutKeys.Reserve(OutKeys.Num() + EntryMap.Num());
	Algo::Transform(EntryMap, OutKeys, &FFaerieStorageEntry::GetKey);
}

TArray<FFaerieAddress> UFaerieItemStorage::CollateAddresses(const FFaerieStorageEntry& Entry)
{
	TArray<FFaerieAddress> Addresses;
	Addresses.Reserve(Entry.NumStacks());
	for (const FFaerieKeyedStack& Stack : Entry.GetStacks())
	{
		Addresses.Emplace(Container::Address::Encode(Entry.GetKey(), Stack.Key));
	}
	return Addresses;
}

const FFaerieStorageEntry* UFaerieItemStorage::GetEntrySafe(const FFaerieEntryKey Key) const
{
	return EntryMap.Find(Key);
}

const FFaerieStorageEntry* UFaerieItemStorage::FindEntry(const FFaerieItemInstance& Item) const
{
	for (const FFaerieStorageEntry& Entry : EntryMap)
	{
		if (Item == Entry.GetInstance())
		{
			return &Entry;
		}
	}

	return nullptr;
}

UObject* UFaerieItemStorage::GetEntryProxyImpl(const FFaerieEntryKey Key) const
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

	// If the entry is mutable, then there is only ever one stack proxy, and we can use that as our entry proxy,
	// saving an extra object creation, and providing extra data to the receiving end.
	// We also can then safely restrict EntryProxies to only describing immutable instances, lightening their API surface.
	if (const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Key))
	{
		if (EntryPtr->GetInstance().IsMutable())
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

Container::FEvent UFaerieItemStorage::AddStackImplNoBroadcast(const TValid<const FFaerieUnownedItemStack&> ItemStack, const bool ForceNewStack)
{
	static auto FindExistingEntry = [](const TNotNull<const UFaerieItemStorage*> Storage, const FFaerieItemInstance& Item) -> const FFaerieStorageEntry*
		{
			if (Item.CanStack())
			{
				return Storage->FindEntry(Item);
			}
			return nullptr;
		};

	FFaerieItemInstance ItemInstance = ValidGet(ItemStack).Instance;
	const int32 Copies = ValidGet(ItemStack).Copies;

	if (const FFaerieStorageEntry* const ExistingEntry = FindExistingEntry(this, ItemInstance))
	{
		// Setup Log for this event
		Container::FEvent Event;
		Event.EntryTouched = ExistingEntry->GetKey();
		Event.Timestamp = FDateTime::UtcNow();
		Event.Type = Inventory::Tags::Addition;
		Event.Instance = ItemInstance;
		Event.Copies = Copies;

		// Try to fill up the stacks of existing entries first, before creating a new entry.
		FFaerieStorageEntry::FReadWriteAccess Entry = ExistingEntry->GetReadWriteAccess(EntryMap);
		if (ForceNewStack)
		{
			Entry.AddToNewStacks(Copies, Event.AddressesTouched);
		}
		else
		{
			Entry.AddToAnyStack(Copies, Event.AddressesTouched);
		}
		Server_PostContentChanged(Entry.Get(), Event);
		return Event;
	}

	// Setup Log for this event
	Container::FEvent Event;
	Event.EntryTouched = KeyGen.NextKey();
	Event.Timestamp = FDateTime::UtcNow();
	Event.Type = Inventory::Tags::Addition;
	Event.Instance = ItemInstance;
	Event.Copies = Copies;

	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked();
	if (ItemInstance.IsMutable())
	{
		Container::TakeOwnership(EntityManager, this, ItemInstance);
	}

	const int32 StackLimit = Container::GetItemStackLimit(&EntityManager, ItemInstance);
	const FFaerieStorageEntry NewEntry ( Event.EntryTouched, ItemInstance, StackLimit, Copies, Event.AddressesTouched );

	// NextKey() is guaranteed to have a greater value than all currently existing keys, so simply appending is fine, and
	// will keep the EntryMap sorted.
	EntryMap.AppendUnsafe(NewEntry);
	Server_PostContentAdded(NewEntry, Event);
	return Event;
}

Container::FEvent UFaerieItemStorage::AddStackImpl(const TValid<const FFaerieUnownedItemStack&> ItemStack, const bool ForceNewStack)
{
	SCOPE_CYCLE_COUNTER(STAT_Storage_Add);

	Container::FEvent Event = AddStackImplNoBroadcast(ItemStack, ForceNewStack);

	// Execute PostEventBatch on all extensions with the finished Event
	PostEvent(Event);

	return Event;
}

Container::FEvent UFaerieItemStorage::RemoveFromEntryImplNoBroadcast(const FFaerieStorageEntry& InEntry, const int32 Amount, const FFaerieInventoryTag Reason)
{
	// Log for this event
	Container::FEvent Event = Container::FEvent::MakeBlank(this, InEntry.GetKey());
	Event.Type = Reason;

	// Open Mutable Scope
	{
		FFaerieStorageEntry::FReadWriteAccess Entry = InEntry.GetReadWriteAccess(EntryMap);

		Event.Instance = Entry->GetInstance();
		const int32 Sum = Entry->StackSum();

		if (Amount == ItemData::EntireStack || Amount >= Sum) // Remove the entire entry
		{
			Event.Copies = Sum;
			Entry->CopyAddresses(Event.AddressesTouched);
			if (Event.Instance.IsMutable())
			{
				FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked();
				Container::ReleaseOwnership(EntityManager, this, Event.Instance);
			}
			Event.EntryRemoved = true;
		}
		else // Remove part of the entry
		{
			Event.Copies = FMath::Clamp(Amount, 1, Sum-1);
			Entry.RemoveFromAnyStack(Event.Copies, Event.AddressesTouched);
		}
	}
	// Close Mutable scope

	if (Event.EntryRemoved)
	{
		UE_LOGF(LogFaerieInventory, Verbose, "Removing entire entry at: '%ls'", *InEntry.GetKey().ToString());
		Server_PreContentRemoved(InEntry, Event);
		EntryMap.Remove(InEntry.GetKey());
	}
	else
	{
		// If the entry was not removed, update local proxies that counts were decremented.
		Server_PostContentChanged(InEntry, Event);
	}

	return Event;
}

Faerie::Container::FEvent UFaerieItemStorage::RemoveFromStackImplNoBroadcast(const FFaerieStorageEntry& InEntry, const FFaerieStackKey Stack, const int32 Amount, const FFaerieInventoryTag Reason)
{
	// Log for this event
	Container::FEvent Event = Container::FEvent::MakeBlank(this, InEntry.GetKey());
	Event.Type = Reason;
	Event.AddressesTouched.Add(Container::Address::Encode(InEntry.GetKey(), Stack));

	// Open Mutable Scope
	{
		FFaerieStorageEntry::FReadWriteAccess Entry = InEntry.GetReadWriteAccess(EntryMap);

		Event.Instance = Entry->GetInstance();

		if (const int32 Copies = Entry->GetStack(Stack);
			Amount == ItemData::EntireStack || Amount >= Copies) // Remove the entire stack
		{
			Event.Copies = Copies;

			// If removing this stack would remove all reference to this item, release and exit so we can remove the entry.
			if (Entry->IsOnlyStack(Stack))
			{
				if (Entry->GetInstance().IsMutable())
				{
					FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked();
					Container::ReleaseOwnership(EntityManager, this, Entry->GetInstance());
				}
				Event.EntryRemoved = true;
			}
			else
			{
				Entry.RemoveStack(Stack);
			}
		}
		else // Remove part of the stack
		{
			checkSlow(Amount == FMath::Clamp(Amount, 1, Copies-1));

			Event.Copies = Amount;

			const int32 NewAmount = Copies - Amount;
			Entry.SetStack(Stack, NewAmount);
		}
	}
	// Close Mutable scope

	if (Event.EntryRemoved)
	{
		UE_LOGF(LogFaerieInventory, Verbose, "Removing entire stack at: '%ls_%ls'", *InEntry.GetKey().ToString(), *Stack.ToString());
		Server_PreContentRemoved(InEntry, Event);
		EntryMap.Remove(InEntry.GetKey());
	}
	else
	{
		// If the entry was not removed, update local proxies that counts were decremented.
		Server_PostContentChanged(InEntry, Event);
	}

	return Event;
}

Container::FEvent UFaerieItemStorage::RemoveFromEntryImpl(const FFaerieStorageEntry& Entry, const int32 Amount,
														  const FFaerieInventoryTag Reason)
{
	SCOPE_CYCLE_COUNTER(STAT_Storage_Remove);

	// RemoveEntryImpl should not be called with unvalidated parameters.
	checkSlow(Faerie::ItemData::IsValidStackAmount(Amount));
	checkSlow(Reason.MatchesTag(Faerie::Inventory::Tags::RemovalBase))

	Container::FEvent Event = RemoveFromEntryImplNoBroadcast(Entry, Amount, Reason);

	PostEvent(Event);

	// Destroy the mass entity, if this stack is being deleted.
	if (Reason == Inventory::Tags::RemovalDeletion && Event.EntryRemoved)
	{
		FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked();
		Event.Instance.DestroyMassEntity(EntityManager);
	}

	return Event;
}

Container::FEvent UFaerieItemStorage::RemoveFromStackImpl(const FFaerieStorageEntry& InEntry, const FFaerieStackKey Stack,
															  const int32 Amount, const FFaerieInventoryTag Reason)
{
	SCOPE_CYCLE_COUNTER(STAT_Storage_Remove);

	// RemoveFromStackImpl should not be called with unvalidated parameters.
	checkSlow(Faerie::ItemData::IsValidStackAmount(Amount));
	checkSlow(Reason.MatchesTag(Faerie::Inventory::Tags::RemovalBase))

	Container::FEvent Event = RemoveFromStackImplNoBroadcast(InEntry, Stack, Amount, Reason);

	PostEvent(Event);

	// Destroy the mass entity, if this stack is being deleted.
	if (Reason == Inventory::Tags::RemovalDeletion && Event.EntryRemoved)
	{
		FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked();
		Event.Instance.DestroyMassEntity(EntityManager);
	}

	return Event;
}

bool UFaerieItemStorage::CanEditStackImpl(const FFaerieStorageEntry& Entry, const FFaerieStackKey Stack, const FFaerieInventoryTag EditTag) const
{
	const FFaerieStorageEntry::FStackReadAccess ReadAccess = Entry.GetStackReadAccess(EntryMap, Stack);

	// By default, some edit types are allowed, unless an extension explicitly disallows it.
	const bool Default = Inventory::Tags::EditTagsAllowedByDefault().Contains(EditTag);
	return AllowsEdit(&ReadAccess, EditTag, Default);
}

bool UFaerieItemStorage::CanRemoveEntryImpl(const FFaerieStorageEntry& Entry, const FFaerieInventoryTag Reason) const
{
	// By default, some removal reasons are allowed, unless an extension explicitly disallows it.
	const bool Default = Inventory::Tags::RemovalTagsAllowedByDefault().Contains(Reason);

	for (auto It = Container::FIterator_SingleEntry_WithInterface(this, Entry); It.IsValid(); It.Advance())
	{
		if (!AllowsRemoval(&It, Reason, Default))
		{
			// Exit if an extension forbids removal.
			return false;
		}
	}

	return true;
}

bool UFaerieItemStorage::CanRemoveStackImpl(const FFaerieStorageEntry& Entry, const FFaerieStackKey Stack, const FFaerieInventoryTag Reason) const
{
	const FFaerieStorageEntry::FStackReadAccess ReadAccess = Entry.GetStackReadAccess(EntryMap, Stack);

	// By default, some removal reasons are allowed, unless an extension explicitly disallows it.
	const bool Default = Inventory::Tags::RemovalTagsAllowedByDefault().Contains(Reason);
	return AllowsRemoval(&ReadAccess, Reason, Default);
}

void UFaerieItemStorage::Server_PostContentAdded(const FFaerieStorageEntry& Entry, const Container::FEvent& Event) {}

void UFaerieItemStorage::Server_PreContentRemoved(const FFaerieStorageEntry& Entry, const Container::FEvent& Event)
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

void UFaerieItemStorage::Server_PostContentChanged(const FFaerieStorageEntry& Entry, const Container::FEvent& Event)
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

	// Prepare event data.
	const Container::FEvent Event = Container::FEvent::MakeAddition(this, Entry.GetInstance(), Entry.StackSum(), Entry.GetKey(), CollateAddresses(Entry));

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

	PostEvent(Event);
}

void UFaerieItemStorage::Client_PreContentRemoved(const FFaerieStorageEntry& Entry)
{
	// We cannot check Entry.IsValid(), because the MassEntity may already be destroyed before this storage received this
	// removal notice.

	// Prepare event data. Item may not be valid.
	// Client_PreContentRemoved is only called by replication callback on FFaerieStorageEntry, so it only gets called
	// when the entry is being removed.
	const Container::FEvent Event = Container::FEvent::MakeRemoval(this, Entry.GetInstance(), Entry.StackSum(),
		Inventory::Tags::RemovalDeletion, Entry.GetKey(), CollateAddresses(Entry), true);

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

	PostEvent(Event);
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

	Container::FEvent Event = Container::FEvent::MakeBlank(this, Entry.GetKey());
	Event.Type = Inventory::Tags::ReplicationEdit;
	Event.Instance = Entry.GetInstance();
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

	PostEvent(Event);
}


	/**------------------------------*/
	/*	 STORAGE API - ALL USERS   */
	/**------------------------------*/

void UFaerieItemStorage::MakeSaveData(FFaerieStorageExportData& OutItemData, const Container::FSaveParams Params) const
{
	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked();

	if (Params.ExportItemData)
	{
		for (const FFaerieStorageEntry& Entry : EntryMap)
		{
			FFaerieItemInstance Instance = Entry.GetInstance();
			FFaerieStorageEntryExportData& ExportEntry = OutItemData.Entries.AddDefaulted_GetRef();
			ExportEntry.ItemObject = Instance.GetItemPtr();
			ExportEntry.ExportData = ExportItemData(EntityManager, Instance);
			ExportEntry.Stacks = Entry.GetStacks();
		}
	}

	if (Params.ExportExtensionData)
	{
		RavelExtensionData(OutItemData.ExtensionData);
	}
}

void UFaerieItemStorage::LoadSaveData(const FFaerieStorageExportData& InItemData, const Container::FLoadParams Params)
{
	// Clear out state
	if (Params.ClearExtensionsBeforeImport)
	{
		ExtensionData.Reset();
	}

	if (Params.ClearItemDataBeforeImport)
	{
		Clear(Inventory::Tags::RemovalDeletion);
	}

	// See Footnote1
	//KeyGen.Reset();

	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked();

	// Load in save data
	for (const FFaerieStorageEntryExportData& ExportEntry : InItemData.Entries)
	{
		FFaerieItemInstance Instance = ImportItemData(EntityManager, ExportEntry.ItemObject, ExportEntry.ExportData);
		if (!Container::ValidateItemData(Instance))
		{
			continue;
		}

		const int32 StackLimit = Container::GetItemStackLimit(&EntityManager, Instance);
		const FFaerieStorageEntry Entry{ KeyGen.NextKey(), Instance, StackLimit, ExportEntry.Stacks };

		EntryMap.AppendUnsafe(Entry);
		// Normally, we would call Server_PostContentAdded after Append, but there should not be any proxies to update, and we don't generate event data here anyway.
	}

	// Rebuild extension state
	UnravelExtensionData(InItemData.ExtensionData);
	InitializeExtensions();
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
	return false;
}

FFaerieEntryKey UFaerieItemStorage::FindItem(const FFaerieItemProxy& Item) const
{
	if (const UFaerieItemStackProxy* StackProxy = Cast<UFaerieItemStackProxy>(Item.GetProxyObject()))
	{
		if (StackProxy->GetOuter() != this)
		{
			// This proxy does not belong to this storage.
			return FFaerieEntryKey();
		}

		const FFaerieEntryKey EntryKey = StackProxy->GetKey();
		if (ContainsKey(EntryKey))
		{
			return EntryKey;
		}
	}
	return FFaerieEntryKey();
}

FFaerieAddress UFaerieItemStorage::GetFirstAddress() const
{
	if (EntryMap.IsEmpty()) return FFaerieAddress();
	const FFaerieStorageEntry& FirstEntry = EntryMap.Entries[0];
	return Container::Address::Encode(FirstEntry.GetKey(), FirstEntry.GetStacks()[0].Key);
}

bool UFaerieItemStorage::CanAddStack(const FFaerieItemProxy& Proxy, const EFaerieStorageAddStackBehavior AddStackBehavior) const
{
	if (!Proxy.IsValid())
	{
		return false;
	}

	const TOptional<FFaerieItemInstance> Instance = Proxy.GetItemInstance();
	if (!Instance.IsSet())
	{
		return false;
	}

	// Prevent recursive storage: don't allow adding an item that contains ourself.
	const FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked();
	if (EntityManager.IsEntityValid(Instance.GetValue().GetMassEntityHandle()))
	{
		if (SubObject::HasContainerInInstanceRecursive_Storage(EntityManager, Instance.GetValue(), TNotNull<const ThisClass*>(this)))
		{
			return false;
		}
	}

	const FFaerieExtensionAllowsAdditionArgs CanAddStackArgs {
		.AddStackBehavior = AddStackBehavior
	};

	return AllowsAddition(MakeConstArrayView(&Proxy, 1), CanAddStackArgs, true);
}

bool UFaerieItemStorage::CanAddStacks(const TArray<FFaerieItemProxy>& Proxies, const FFaerieExtensionAllowsAdditionArgs Args) const
{
	const FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked();
	for (const FFaerieItemProxy& Proxy : Proxies)
	{
		if (!Proxy.IsValid())
		{
			return false;
		}

		const TOptional<FFaerieItemInstance> Instance = Proxy.GetItemInstance();
		if (!Instance.IsSet())
		{
			return false;
		}

		// Prevent recursive storage: don't allow adding an item that contains ourself.
		if (EntityManager.IsEntityValid(Instance.GetValue().GetMassEntityHandle()))
		{
			if (SubObject::HasContainerInInstanceRecursive_Storage(EntityManager, Instance.GetValue(), TNotNull<const ThisClass*>(this)))
			{
				return false;
			}
		}
	}

	return AllowsAddition(Proxies, Args, true);
}

bool UFaerieItemStorage::CanAddStacks(const Utils::TArrayAdapter<FFaerieItemProxy>& Stacks, const FFaerieExtensionAllowsAdditionArgs Args) const
{
	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked();
	for (int32 i = 0; i < Stacks.Num(); ++i)
	{
		const FFaerieItemProxy Proxy = Stacks[i];

		if (!Proxy.IsValid())
		{
			return false;
		}

		const TOptional<FFaerieItemInstance> Instance = Proxy.GetItemInstance();
		if (!Instance.IsSet())
		{
			return false;
		}

		// Prevent recursive storage: don't allow adding an item that contains ourself.
		if (EntityManager.IsEntityValid(Instance.GetValue().GetMassEntityHandle()))
		{
			// Prevent recursive storage for mutable items
			if (SubObject::HasContainerInInstanceRecursive_Storage(EntityManager, Instance.GetValue(), TNotNull<const ThisClass*>(this)))
			{
				return false;
			}
		}
	}

	return AllowsAddition(Stacks, Args, true);
}

bool UFaerieItemStorage::CanEditStack(const FFaerieAddress Address, const FFaerieInventoryTag EditTag) const
{
	FFaerieEntryKey Entry;
	FFaerieStackKey Stack;
	Container::Address::Decode(Address, Entry, Stack);
	if (const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Entry))
	{
		return CanEditStackImpl(*EntryPtr, Stack, EditTag);
	}
	return false;
}

bool UFaerieItemStorage::CanRemoveEntry(const FFaerieEntryKey Key, const FFaerieInventoryTag Reason) const
{
	if (const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Key))
	{
		return CanRemoveEntryImpl(*EntryPtr, Reason);
	}
	return false;
}

bool UFaerieItemStorage::CanRemoveStack(const FFaerieAddress Address, const FFaerieInventoryTag Reason) const
{
	// By default, some removal reasons are allowed, unless an extension explicitly disallows it.
	const bool Default = Inventory::Tags::RemovalTagsAllowedByDefault().Contains(Reason);

	FFaerieEntryKey Entry;
	FFaerieStackKey Stack;
	Container::Address::Decode(Address, Entry, Stack);
	if (const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Entry))
	{
		const FFaerieStorageEntry::FStackReadAccess ReadAccess = EntryPtr->GetStackReadAccess(EntryMap, Stack);

		if (!AllowsRemoval(&ReadAccess, Reason, Default))
		{
			return false;
		}
	}

	return true;
}


	/**---------------------------------*/
	/*	 STORAGE API - AUTHORITY ONLY   */
	/**---------------------------------*/

bool UFaerieItemStorage::AddItemStack(const FFaerieUnownedItemStack& Stack, const EFaerieStorageAddStackBehavior AddStackBehavior)
{
	if (!Stack.IsValid()) return false;

	const ItemData::FScopeProxy Proxy(Stack.Instance, Stack.Copies, nullptr);
	if (!CanAddStack(FFaerieItemProxy(FFaerieItemProxy::ESingleFrame, &Proxy), AddStackBehavior))
	{
		return false;
	}

	(void)AddStackImpl(Stack, Container::IfOnlyNewStacks(AddStackBehavior));
	return true;
}

void UFaerieItemStorage::AddItemStack(const TValid<const FFaerieUnownedItemStack&> Stack,
	const EFaerieStorageAddStackBehavior AddStackBehavior, TValueOrError<Container::FEvent, FText>& OutResult)
{
	const ItemData::FScopeProxy Proxy(ValidGet(Stack).Instance, ValidGet(Stack).Copies, nullptr);
	if (!CanAddStack(FFaerieItemProxy(FFaerieItemProxy::ESingleFrame, &Proxy), AddStackBehavior))
	{
		OutResult = MakeError(Container::AdditionFailure_FailedCanAddStack);
		return;
	}

	OutResult = MakeValue(AddStackImpl(Stack, Container::IfOnlyNewStacks(AddStackBehavior)));
}

bool UFaerieItemStorage::AddItemStacks(const Utils::TArrayAdapter<FFaerieUnownedItemStack>& Adapter, const EFaerieStorageAddStackBehavior AddStackBehavior, const bool StopAfterFailure)
{
	SCOPE_CYCLE_COUNTER(STAT_Storage_AddMulti);

	const bool ForceNewStack = Container::IfOnlyNewStacks(AddStackBehavior);

	TArray<Container::FEvent> Events;
	Events.Reserve(Adapter.Num());

	ON_SCOPE_EXIT
	{
		if (!Events.IsEmpty())
		{
			// Execute PostEventBatch on all extensions with the finished Event
			PostEventBatch(Events);
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

		const ItemData::FScopeProxy Proxy(Stack.Instance, Stack.Copies, nullptr);
		if (!CanAddStack(FFaerieItemProxy(FFaerieItemProxy::ESingleFrame, &Proxy), AddStackBehavior))
		{
			if (StopAfterFailure)
			{
				return false;
			}
			continue;
		}

		Events.Add(AddStackImplNoBroadcast(Stack, ForceNewStack));
	}

	return true;
}

void UFaerieItemStorage::AddItemStackBulk(const TArray<FFaerieUnownedItemStack>& Stacks, const EFaerieStorageAddStackBehavior AddStackBehavior, const bool StopAfterFailure)
{
	AddItemStacks(Stacks, AddStackBehavior, StopAfterFailure);
}

bool UFaerieItemStorage::AddItemStackWithLog(const FFaerieUnownedItemStack& Stack, const EFaerieStorageAddStackBehavior AddStackBehavior, FFaerieBlueprintInventoryEvent& Event)
{
	TValueOrError<Container::FEvent, FText> Result = MakeError(FText::GetEmpty());
	AddItemStack(Stack, AddStackBehavior, Result);
	if (Result.HasValue())
	{
		Event = FFaerieBlueprintInventoryEvent::FromNativeEvent(Result.GetValue());
	}
	return false;
}

bool UFaerieItemStorage::RemoveEntry(const FFaerieEntryKey Key, const FFaerieInventoryTag RemovalTag, const int32 Amount)
{
	if (Amount == 0 || Amount < ItemData::EntireStack) return false;
	if (!RemovalTag.IsValid()) return false;

	if (const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Key))
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

	FFaerieEntryKey Entry;
	FFaerieStackKey Stack;
	Container::Address::Decode(Address, Entry, Stack);
	if (const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Entry))
	{
		if (CanRemoveStackImpl(*EntryPtr, Stack, RemovalTag))
		{
			(void)RemoveFromStackImpl(*EntryPtr, Stack, Amount, RemovalTag);
			return true;
		}
	}
	return false;
}

bool UFaerieItemStorage::TakeEntry(const FFaerieEntryKey Key, FFaerieUnownedItemStack& OutStack,
								   const FFaerieInventoryTag RemovalTag, const int32 Amount)
{
	if (Amount == 0 || Amount < ItemData::EntireStack) return false;
	if (!RemovalTag.IsValid()) return false;

	if (const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Key))
	{
		if (CanRemoveEntryImpl(*EntryPtr, RemovalTag))
		{
			const Container::FEvent Event = RemoveFromEntryImpl(*EntryPtr, Amount, RemovalTag);
			OutStack = FFaerieUnownedItemStack(Event.Instance, Event.Copies);
            return true;
		}
	}

	return false;
}

bool UFaerieItemStorage::TakeStack(const FFaerieAddress Address, FFaerieUnownedItemStack& OutStack,
								   const FFaerieInventoryTag RemovalTag, const int32 Amount)
{
	if (Amount == 0 || Amount < ItemData::EntireStack) return false;
	if (!RemovalTag.IsValid()) return false;

	FFaerieEntryKey Entry;
	FFaerieStackKey Stack;
	Container::Address::Decode(Address, Entry, Stack);
	if (const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Entry))
	{
		if (CanRemoveStackImpl(*EntryPtr, Stack, RemovalTag))
		{
			const Container::FEvent Event = RemoveFromStackImpl(*EntryPtr, Stack, Amount, RemovalTag);
			OutStack = FFaerieUnownedItemStack(Event.Instance, Event.Copies);
			return true;
		}
	}

	return false;
}

void UFaerieItemStorage::Clear(FFaerieInventoryTag RemovalTag)
{
	if (!RemovalTag.IsValid() || !RemovalTag.MatchesTag(Inventory::Tags::RemovalBase))
	{
		RemovalTag = Inventory::Tags::RemovalDeletion;
	}

	TArray<FFaerieEntryKey> Entries;
	CopyEntryKeys(Entries);

	TArray<Container::FEvent> Events;
	Events.Reserve(Entries.Num());

	for (const FFaerieEntryKey EntryKey : Entries)
	{
		const FFaerieStorageEntry& Entry = EntryMap[EntryKey];

		if (!CanRemoveEntryImpl(Entry, Inventory::Tags::RemovalMoving))
		{
			continue;
		}

		// RemoveFromEntryImplNoBroadcast should not be called with unvalidated parameters.
		Events.Add(RemoveFromEntryImplNoBroadcast(Entry, ItemData::EntireStack, RemovalTag));
	}

	PostEventBatch(Events);

	if (RemovalTag == Inventory::Tags::RemovalDeletion)
	{
		FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked();
		for (Container::FEvent& Event : Events)
		{
			Event.Instance.DestroyMassEntity(EntityManager);
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

FFaerieEntryKey UFaerieItemStorage::MoveStack(UFaerieItemStorage* ToStorage, const FFaerieAddress Address, const int32 Amount, const EFaerieStorageAddStackBehavior AddStackBehavior)
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

	if (!CanRemoveStackImpl(*EntryPtr, Stack, Inventory::Tags::RemovalMoving))
	{
		return FFaerieEntryKey::InvalidKey;
	}

	const int32 StackValue = EntryPtr->GetStack(Stack);
	if (0 >= StackValue)
	{
		return FFaerieEntryKey::InvalidKey;
	}

	ItemData::FScopeProxy View(EntryPtr->GetInstance(), StackValue, this);
	if (Amount > 0)
	{
		View.SetCopies(FMath::Min(StackValue, Amount));
	}

	if (!ToStorage->CanAddStack(FFaerieItemProxy(FFaerieItemProxy::ESingleFrame, &View), AddStackBehavior))
	{
		return FFaerieEntryKey::InvalidKey;
	}

	const Container::FEvent Event = RemoveFromStackImpl(*EntryPtr, Stack, Amount, Inventory::Tags::RemovalMoving);

	const TValid<FFaerieUnownedItemStack> ItemStack(Event.Instance, Event.Copies);
	return ToStorage->AddStackImpl(ItemStack, Container::IfOnlyNewStacks(AddStackBehavior)).EntryTouched;
}

FFaerieEntryKey UFaerieItemStorage::MoveEntry(UFaerieItemStorage* ToStorage, const FFaerieEntryKey Key, const EFaerieStorageAddStackBehavior AddStackBehavior)
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

	if (!CanRemoveEntryImpl(*EntryPtr, Inventory::Tags::RemovalMoving))
	{
		return FFaerieEntryKey::InvalidKey;
	}

	const FFaerieStorageEntry::FReadAccess ReadAccess = EntryPtr->GetReadAccess(EntryMap);
	if (!ToStorage->CanAddStack(FFaerieItemProxy(FFaerieItemProxy::ESingleFrame, &ReadAccess), AddStackBehavior))
	{
		return FFaerieEntryKey::InvalidKey;
	}

	const Container::FEvent RemoveResult = RemoveFromEntryImpl(*EntryPtr,
		ItemData::EntireStack, Inventory::Tags::RemovalMoving);

	const TValid<FFaerieUnownedItemStack> ItemStack(RemoveResult.Instance, RemoveResult.Copies);
	return ToStorage->AddStackImpl(ItemStack, Container::IfOnlyNewStacks(AddStackBehavior)).EntryTouched;
}

bool UFaerieItemStorage::MergeStacks(const FFaerieEntryKey Entry, const FFaerieStackKey FromStack, const FFaerieStackKey ToStack, const int32 Amount)
{
	const FFaerieAddress FromAddress = Container::Address::Encode(Entry, FromStack);
	const FFaerieAddress ToAddress = Container::Address::Encode(Entry, ToStack);

	// Verify the stack exists.
	const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Entry);
	if (!EntryPtr)
	{
		return false;
	}

	if (!CanEditStackImpl(*EntryPtr, FromStack, Inventory::Tags::Merge) ||
		!CanEditStackImpl(*EntryPtr, ToStack, Inventory::Tags::Merge))
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

	Container::FEvent Event = Container::FEvent::MakeBlank(this, Entry);
	Event.Type = Inventory::Tags::Merge;
	Event.Copies = AmountB; // Initially store the amount in stack B here.
	Event.Instance = EntryPtr->GetInstance();
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

	PostEvent(Event);

	return true;
}

bool UFaerieItemStorage::SplitStack(const FFaerieAddress Address, const int32 Amount)
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
	if (!CanEditStackImpl(*EntryPtr, Stack, Inventory::Tags::Split))
	{
		return false;
	}

	// Validate that the requested amount is less than what's in the stack
	if (Amount >= EntryPtr->GetStack(Stack))
	{
		return false;
	}

	Container::FEvent Event = Container::FEvent::MakeBlank(this, Entry);
	Event.Type = Inventory::Tags::Split;
	Event.Instance = EntryPtr->GetInstance();
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

	PostEvent(Event);

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

	TArray<FFaerieEntryKey> Entries;
	CopyEntryKeys(Entries);

	TArray<Container::FEvent> Events;
	Events.Reserve(Entries.Num());

	TArray<FFaerieUnownedItemStack> Stacks;
	Stacks.Reserve(Entries.Num());

	for (const FFaerieEntryKey EntryKey : Entries)
	{
		const FFaerieStorageEntry& Entry = EntryMap[EntryKey];

		if (!CanRemoveEntryImpl(Entry, Inventory::Tags::RemovalMoving))
		{
			continue;
		}

		const FFaerieStorageEntry::FReadAccess ReadAccess = Entry.GetReadAccess(EntryMap);
		if (!ToStorage->CanAddStack(FFaerieItemProxy(FFaerieItemProxy::ESingleFrame, &ReadAccess), DumpBehavior))
		{
			continue;
		}

		Container::FEvent& Event = Events.Add_GetRef(RemoveFromEntryImplNoBroadcast(Entry, ItemData::EntireStack, Inventory::Tags::RemovalMoving));

		Stacks.Emplace(Event.Instance, Event.Copies);
	}

	PostEventBatch(Events);

	ToStorage->AddItemStacks(Stacks, DumpBehavior, false);
}

#undef LOCTEXT_NAMESPACE

/*
 * Footnote1: You might think that even at runtime we could reset the key during Clear, since all items are removed,
 * and therefor no entries exist, making 100 a valid starting point again, *except* that other entities might still
 * be holding onto FFaerieEntryKeys, which could be cached in at some point later when potentially the entry is once more
 * valid, but with a completely different item. So during runtime, the Key must always increment.
 */