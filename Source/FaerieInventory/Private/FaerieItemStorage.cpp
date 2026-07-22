// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemStorage.h"
#include "FaerieInventoryLog.h"
#include "FaerieItem.h"
#include "FaerieItemStorageIterators.h"
#include "FaerieItemOwnership.h"
#include "FaerieSubObjectFilter.h"
#include "ItemStackProxy.h"
#include "ItemContainerExtensionBase.h"
#include "EntityManagerHelpers.h"

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

namespace Faerie::Storage
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

void UFaerieItemStorage::InitializeNetObject(const TNotNull<AActor*> Actor)
{
	Super::InitializeNetObject(Actor);
	Actor->AddReplicatedSubObject(Extensions);
	Extensions->InitializeNetObject(Actor);
}

void UFaerieItemStorage::DeinitializeNetObject(const TNotNull<AActor*> Actor)
{
	Extensions->DeinitializeNetObject(Actor);
	Actor->RemoveReplicatedSubObject(Extensions);
	Super::DeinitializeNetObject(Actor);
}

FInstancedStruct UFaerieItemStorage::MakeSaveData(FFaerieItemContainerExtensionData& ExtensionData) const
{
	RavelExtensionData(ExtensionData);

	const ItemData::FRequireEntityManager EntityManager(this);

	FFaerieStorageExportData ExportData;
	for (const FFaerieStorageEntry& Entry : EntryMap)
	{
		auto Instance = Entry.GetInstance();
		FFaerieStorageEntryExportData& ExportEntry = ExportData.Entries.AddDefaulted_GetRef();
		ExportEntry.ItemObject = Instance.GetItemPtr();
		ExportEntry.ExportData = ExportItemData(EntityManager, Instance);
		ExportEntry.Stacks = Entry.GetStacks();
	}

	return FInstancedStruct::Make(ExportData);
}

void UFaerieItemStorage::LoadSaveData(const FConstStructView ItemData, const TSharedStruct<FFaerieItemContainerExtensionData>& ExtensionData)
{
	// Clear out state
	Clear(Inventory::Tags::RemovalDeletion);
	KeyGen.Reset();

	Extensions::FGroupAPI::DeinitializeExtension(Extensions, this);

	const ItemData::FRequireEntityManager EntityManager(this);

	// Load in save data
	auto& ExportData = ItemData.Get<const FFaerieStorageExportData>();
	for (auto&& ExportEntry : ExportData.Entries)
	{
		const FFaerieItemInstance Instance = ImportItemData(EntityManager, ExportEntry.ItemObject, ExportEntry.ExportData);
		if (!Container::ValidateItemData(Instance))
		{
			continue;
		}

		const int32 StackLimit = Container::GetItemStackLimit(EntityManager, Instance);
		const FFaerieStorageEntry Entry{ KeyGen.NextKey(), Instance, StackLimit, ExportEntry.Stacks };

		EntryMap.AppendUnsafe(Entry);
	}

	// Determine the next valid key to use.
	if (!EntryMap.IsEmpty())
	{
		KeyGen.SetPosition(EntryMap.GetKeyAt(EntryMap.Num()-1));
	}

	// Rebuild extension state

	//@todo broadcast full refresh event?

	Extensions::FGroupAPI::InitializeExtension(Extensions, this);

	if (ExtensionData.IsValid())
	{
		UnravelExtensionData(ExtensionData);
	}
}

bool UFaerieItemStorage::Contains(const FFaerieAddress Address) const
{
	return ContainsAddress(Address);
}

FFaerieItemInstance UFaerieItemStorage::ViewInstance(const FFaerieEntryKey Key) const
{
	if (const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Key))
	{
		return EntryPtr->GetInstance();
	}
	return FFaerieItemInstance();
}

FFaerieItemInstance UFaerieItemStorage::ViewInstance(const FFaerieAddress Address) const
{
	FFaerieEntryKey Entry;
	Storage::Address::Decode_Entry(Address, Entry);
	if (const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Entry))
	{
		return EntryPtr->GetInstance();
	}
	return FFaerieItemInstance();
}

FFaerieItemDataView UFaerieItemStorage::ViewEntry(const FFaerieEntryKey Key) const
{
	if (const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Key))
	{
		return FFaerieItemDataView(EntryPtr->GetInstance(), EntryPtr->StackSum(), this);
	}
	return FFaerieItemDataView();
}

FFaerieItemDataView UFaerieItemStorage::ViewAddress(const FFaerieAddress Address) const
{
	FFaerieEntryKey Entry;
	FFaerieStackKey Stack;
	Storage::Address::Decode(Address, Entry, Stack);
	if (const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Entry))
	{
		return FFaerieItemDataView(EntryPtr->GetInstance(), EntryPtr->GetStack(Stack), this);
	}
	return FFaerieItemDataView();
}

FFaerieItemProxy UFaerieItemStorage::Proxy(const FFaerieAddress Address) const
{
	return FFaerieItemProxy(GetStackProxyImpl(Address));
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

TOptional<FFaerieUnownedItemStack> UFaerieItemStorage::Release(const FFaerieEntryKey Key, const int32 Copies)
{
	if (FFaerieUnownedItemStack OutStack;
		TakeEntry(Key, OutStack, Inventory::Tags::RemovalMoving, Copies))
	{
		return OutStack;
	}
	return NullOpt;
}

TOptional<FFaerieUnownedItemStack> UFaerieItemStorage::Release(const FFaerieAddress Address, const int32 Copies)
{
	if (FFaerieUnownedItemStack OutStack;
		TakeStack(Address, OutStack, Inventory::Tags::RemovalMoving, Copies))
	{
		return OutStack;
	}
	return NullOpt;
}

void UFaerieItemStorage::GetAllAddresses(TArray<FFaerieAddress>& Addresses) const
{
	Addresses.Reset(Algo::TransformAccumulate(EntryMap, &FFaerieStorageEntry::NumStacks, 0));
	for (const FFaerieStorageEntry& Entry : EntryMap)
	{
		for (const FFaerieKeyedStack& Stack : Entry.GetStacks())
		{
			Addresses.Add(Storage::Address::Encode(Entry.GetKey(), Stack.Key));
		}
	}
}

TUniquePtr<Container::IEntryIterator> UFaerieItemStorage::CreateEntryIterator() const
{
	// Don't provide an iterator if we are empty...
	if (EntryMap.IsEmpty()) return nullptr;
	return MakeUnique<Storage::FIterator_AllEntries_WithInterface>(this);
}

TUniquePtr<Container::IAddressIterator> UFaerieItemStorage::CreateAddressIterator() const
{
	// Don't provide an iterator if we are empty...
	if (EntryMap.IsEmpty()) return nullptr;
	return MakeUnique<Storage::FIterator_AllAddresses_WithInterface>(this);
}

TUniquePtr<Container::IAddressIterator> UFaerieItemStorage::CreateSingleEntryIterator(const FFaerieEntryKey Key) const
{
	// Don't provide an iterator if the key is invalid...
	if (const FFaerieStorageEntry* Entry = GetEntrySafe(Key))
	{
		return MakeUnique<Storage::FIterator_SingleEntry_WithInterface>(this, *Entry);
	}
	return nullptr;
}

void UFaerieItemStorage::DestroyStack(const FFaerieItemProxy& Proxy, const int32 Copies)
{
	if (const UFaerieItemStackProxy* StackProxy = Cast<UFaerieItemStackProxy>(Proxy.GetProxyObject()))
	{
		if (StackProxy->GetOuter() != this)
		{
			UE_LOG(LogFaerieInventory, Error, TEXT("This isn't our proxy! We cannot release copies from it."))
			return;
		}

		FFaerieUnownedItemStack OutStack;
		TakeStack(StackProxy->Address, OutStack, Inventory::Tags::RemovalDeletion, Copies);
	}
}

bool UFaerieItemStorage::CanPossess(const FFaerieItemDataView& View) const
{
	return CanAddStack(View, EFaerieStorageAddStackBehavior::AddToAnyStack);
}

bool UFaerieItemStorage::Possess(const FFaerieUnownedItemStack& Stack)
{
	FFaerieItemDataView StackView(Stack.Instance, Stack.Copies, nullptr);
	if (!CanAddStack(StackView, EFaerieStorageAddStackBehavior::AddToAnyStack)) return false;

	(void)AddStackImpl(StackView, false);
	return true;
}

void UFaerieItemStorage::OnItemDataChanged(const ItemData::FMutableReference& Instance, const TNotNull<const UScriptStruct*> Struct, const FGameplayTag EditTag)
{
	Super::OnItemDataChanged(Instance, Struct, EditTag);

	if (const FFaerieStorageEntry* Entry = FindEntry(Instance))
	{
		// There should only be a single address for a mutable item, so only try to update one.
		const FFaerieAddress Address = Entry->FirstAddress();

		if (const TWeakObjectPtr<UFaerieItemStackProxy>* Proxy = LocalStackProxies.Find(Address);
			Proxy && Proxy->IsValid())
		{
			Proxy->Get()->NotifyItemDataChanged(EditTag);
		}
	}
}

void UFaerieItemStorage::Client_PostContentAdded(const FFaerieStorageEntry& Entry)
{
	if (!Entry.IsValid())
	{
		UE_LOG(LogFaerieInventory, Error, TEXT("Client_PostContentAdded: Received Invalid Entry"))
		return;
	}

	// Prepare event data.
	Inventory::FEventData Event(Entry.GetKey());
	Event.Instance = Entry.GetInstance();
	Event.Copies = Entry.StackSum();
	Event.AddressesTouched = CollateAddresses(Entry);

	// Proxies may already exist for keys on the client if they are replicated by extensions or other means, and
	// happened to arrive before we got them.
	for (FFaerieAddress Address : Event.AddressesTouched)
	{
		if (auto&& StackProxy = LocalStackProxies.Find(Address))
		{
			if (StackProxy->IsValid())
			{
				StackProxy->Get()->NotifyCreation();
			}
		}
	}

	if (!IsValid(Extensions))
	{
		UE_LOG(LogFaerieInventory, Error, TEXT("Client_PostContentAdded: Invalid Extensions object!"))
		return;
	}

	Extensions::FGroupAPI::PostEvent(Extensions, this, Event, Inventory::Tags::Addition);
}

void UFaerieItemStorage::Client_PreContentRemoved(const FFaerieStorageEntry& Entry)
{
	// We cannot check Entry.IsValid(), because the ItemObject may already be destroyed before this storage received this
	// removal notice.

	// Prepare event data. Item may not be valid.
	Inventory::FEventData Event(Entry.GetKey());
	Event.Instance = Entry.GetInstance();
	Event.Copies = Entry.StackSum();
	Event.AddressesTouched = CollateAddresses(Entry);

	// Cleanup local views.
	for (const FFaerieAddress Address : Event.AddressesTouched)
	{
		TWeakObjectPtr<UFaerieItemStackProxy> StackProxy;
		LocalStackProxies.RemoveAndCopyValue(Address, StackProxy);
		if (UFaerieItemStackProxy* ProxyObject = StackProxy.Get())
		{
			ProxyObject->NotifyRemoval();
		}
	}

	if (!IsValid(Extensions))
	{
		UE_LOG(LogFaerieInventory, Error, TEXT("Client_PreContentRemoved: Invalid Extensions object!"))
		return;
	}

	Extensions::FGroupAPI::PostEvent(Extensions, this, Event, Inventory::Tags::RemovalDeletion);
}

void UFaerieItemStorage::Client_PostContentChanged(const FFaerieStorageEntry& Entry)
{
	if (!ensure(Entry.IsValid()))
	{
		UE_LOG(LogFaerieInventory, Error, TEXT("Client_PostContentChanged: Received Invalid Entry"))
		return;
	}

	if (!ensure(ContainsKey(Entry.GetKey())))
	{
		// Do nothing, Client_PreContentRemoved should handle this ...
		return;
	}

	Inventory::FEventData Event(Entry.GetKey());
	Event.Instance = Entry.GetInstance();
	Event.AddressesTouched.Reserve(Entry.NumStacks());

	// Call updates on any stack proxies.
	// Client_PostContentChanged is called when stacks are removed as well, so let's do some cleanup here.
	// Start by getting all the Addresses that we could have proxies for.
	TSet<FFaerieAddress> Addresses;
	Addresses.Reserve(Entry.NumStacks());
	for (const FFaerieKeyedStack& Stack : Entry.GetStacks())
	{
		const FFaerieAddress Address = Storage::Address::Encode(Entry.GetKey(), Stack.Key);
		Addresses.Add(Address);
		Event.AddressesTouched.Add(Address);
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

	if (!IsValid(Extensions))
	{
		UE_LOG(LogFaerieInventory, Error, TEXT("Client_PostContentChanged: Invalid Extensions object!"))
		return;
	}

	Extensions::FGroupAPI::PostEvent(Extensions, this, Event, Inventory::Tags::ReplicationEdit);
}


	/**------------------------------*/
	/*	  INTERNAL IMPLEMENTATIONS	 */
	/**------------------------------*/

TArray<FFaerieEntryKey> UFaerieItemStorage::CopyEntryKeys() const
{
	TArray<FFaerieEntryKey> Keys;
	Keys.Reserve(EntryMap.Num());
	Algo::Transform(EntryMap, Keys, &FFaerieStorageEntry::GetKey);
	return Keys;
}

TArray<FFaerieAddress> UFaerieItemStorage::CollateAddresses(const FFaerieStorageEntry& Entry)
{
	TArray<FFaerieAddress> Addresses;
	Addresses.Reserve(Entry.NumStacks());
	for (const FFaerieKeyedStack& Stack : Entry.GetStacks())
	{
		Addresses.Emplace(Storage::Address::Encode(Entry.GetKey(), Stack.Key));
	}
	return Addresses;
}

const FFaerieStorageEntry* UFaerieItemStorage::GetEntrySafe(const FFaerieEntryKey Key) const
{
	return EntryMap.Find(Key);
}

const FFaerieStorageEntry* UFaerieItemStorage::FindEntry(const ItemData::FReference& Item) const
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
	Storage::Address::Decode(Address, Entry, Stack);
	const FName ProxyName = MakeUniqueObjectName(This, UFaerieItemStackProxy::StaticClass(),
												 *FString::Printf(TEXT("STACK_PROXY_%s_%s"),
												 *Entry.ToString(), *Stack.ToString()));
	const TNotNull<UFaerieItemStackProxy*> NewStackProxy =
		NewObject<UFaerieItemStackProxy>(This, UFaerieItemStackProxy::StaticClass(), ProxyName);

	NewStackProxy->ItemStorage = This;
	NewStackProxy->Address = Address;

	This->LocalStackProxies.Add(Address, NewStackProxy);

	if (const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Entry))
	{
		if (EntryPtr->Contains(Stack))
		{
			NewStackProxy->NotifyCreation();
		}
	}

	return NewStackProxy;
}

Inventory::FEventData UFaerieItemStorage::AddStackImplNoBroadcast(const ItemData::FValidatedDataView& View, const bool ForceNewStack)
{
	static auto FindExistingEntry = [](const TNotNull<const UFaerieItemStorage*> Storage, const ItemData::FReference& Reference) -> const FFaerieStorageEntry*
		{
			// Mutables cannot stack, due to, well, being mutable, meaning that each instance retains the ability to
			// uniquely differ from others.
			if (!Reference->IsMutable())
			{
				return Storage->FindEntry(Reference);
			}
			return nullptr;
		};

	const FFaerieItemInstance ItemInstance = View->GetInstance();

	if (const FFaerieStorageEntry* const ExistingEntry = FindExistingEntry(this, ItemInstance))
	{
		// Setup Log for this event
		Inventory::FEventData Event(ExistingEntry->GetKey());
		Event.Instance = ItemInstance;
		Event.Copies = View->GetCopies();

		// Try to fill up the stacks of existing entries first, before creating a new entry.
		FFaerieStorageEntry::FReadWriteAccess Entry = ExistingEntry->GetReadWriteAccess(EntryMap);
		if (ForceNewStack)
		{
			Entry.AddToNewStacks(View->GetCopies(), Event.AddressesTouched);
		}
		else
		{
			Entry.AddToAnyStack(View->GetCopies(), Event.AddressesTouched);
		}
		return Event;
	}

	// Setup Log for this event
	Inventory::FEventData Event(KeyGen.NextKey());
	Event.Instance = ItemInstance;
	Event.Copies = View->GetCopies();

	ItemData::FRequireEntityManager EntityManager(this);
	if (ItemInstance.IsMutable())
	{
		Container::TakeOwnership(EntityManager, this, ItemInstance);
	}

	const int32 StackLimit = Container::GetItemStackLimit(EntityManager, ItemInstance);
	const FFaerieStorageEntry NewEntry ( Event.EntryTouched, ItemInstance, StackLimit, View->GetCopies(), Event.AddressesTouched );

	// NextKey() is guaranteed to have a greater value than all currently existing keys, so simply appending is fine, and
	// will keep the EntryMap sorted.
	EntryMap.AppendUnsafe(NewEntry);
	return Event;
}

Inventory::FEventData UFaerieItemStorage::AddStackImpl(const ItemData::FValidatedDataView& View, const bool ForceNewStack)
{
	SCOPE_CYCLE_COUNTER(STAT_Storage_Add);

	// Execute PreAddition on all extensions

	Extensions::FGroupAPI::PreAddition(Extensions, this, View);

	Inventory::FEventData Event = AddStackImplNoBroadcast(View, ForceNewStack);

	// Execute PostEventBatch on all extensions with the finished Event
	Extensions::FGroupAPI::PostEvent(Extensions, this, Event, Inventory::Tags::Addition);

	return Event;
}

Inventory::FEventData UFaerieItemStorage::RemoveFromEntryImplNoBroadcast(const FFaerieStorageEntry& InEntry, const int32 Amount)
{
	// Log for this event
	Inventory::FEventData Event(InEntry.GetKey());

	bool RemoveEntry = false;

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
				Container::ReleaseOwnership(ItemData::FRequireEntityManager(this), this, Event.Instance);
			}
			RemoveEntry = true;
		}
		else // Remove part of the entry
		{
			Event.Copies = FMath::Clamp(Amount, 1, Sum-1);
			Entry.RemoveFromAnyStack(Event.Copies, Event.AddressesTouched);
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

Inventory::FEventData UFaerieItemStorage::RemoveFromStackImplNoBroadcast(const FFaerieStorageEntry& InEntry, const FFaerieStackKey Stack, const int32 Amount)
{
	// Log for this event
	Inventory::FEventData Event(InEntry.GetKey());
	Event.AddressesTouched.Add(Storage::Address::Encode(InEntry.GetKey(), Stack));

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
					Container::ReleaseOwnership(ItemData::FRequireEntityManager(this), this, Entry->GetInstance());
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
		UE_LOG(LogFaerieInventory, Verbose, TEXT("Removing entire stack at: '%s_%s'"), *InEntry.GetKey().ToString(), *Stack.ToString());
		EntryMap.Remove(InEntry.GetKey());
	}

	return Event;
}

Inventory::FEventData UFaerieItemStorage::RemoveFromEntryImpl(const FFaerieStorageEntry& Entry, const int32 Amount,
															  const FFaerieInventoryTag Reason)
{
	SCOPE_CYCLE_COUNTER(STAT_Storage_Remove);

	// RemoveEntryImpl should not be called with unvalidated parameters.
	checkSlow(Faerie::ItemData::IsValidStackAmount(Amount));
	checkSlow(Reason.MatchesTag(Faerie::Inventory::Tags::RemovalBase))

	const FFaerieStorageEntry::FReadAccess ReadAccess = Entry.GetReadAccess(EntryMap);
	Extensions::FGroupAPI::PreRemoval(Extensions, this, ReadAccess, Amount);

	Inventory::FEventData Event = RemoveFromEntryImplNoBroadcast(Entry, Amount);

	Extensions::FGroupAPI::PostEvent(Extensions, this, Event, Reason);

	// Destroy the mass entity, if this stack is being deleted.
	if (Reason == Inventory::Tags::RemovalDeletion && Event.EntryRemoved)
	{
		Event.Instance.DestroyMassEntity(ItemData::FRequireEntityManager(this));
	}

	return Event;
}

Inventory::FEventData UFaerieItemStorage::RemoveFromStackImpl(const FFaerieStorageEntry& InEntry, const FFaerieStackKey Stack,
															  const int32 Amount, const FFaerieInventoryTag Reason)
{
	SCOPE_CYCLE_COUNTER(STAT_Storage_Remove);

	// RemoveFromStackImpl should not be called with unvalidated parameters.
	checkSlow(Faerie::ItemData::IsValidStackAmount(Amount));
	checkSlow(Reason.MatchesTag(Faerie::Inventory::Tags::RemovalBase))

	const FFaerieStorageEntry::FStackReadAccess ReadAccess = InEntry.GetStackReadAccess(EntryMap, Stack);
	Extensions::FGroupAPI::PreRemoval(Extensions, this, ReadAccess, Amount);

	Inventory::FEventData Event = RemoveFromStackImplNoBroadcast(InEntry, Stack, Amount);

	Extensions::FGroupAPI::PostEvent(Extensions, this, Event, Reason);

	// Destroy the mass entity, if this stack is being deleted.
	if (Reason == Inventory::Tags::RemovalDeletion && Event.EntryRemoved)
	{
		Event.Instance.DestroyMassEntity(ItemData::FRequireEntityManager(this));
	}

	return Event;
}

bool UFaerieItemStorage::CanEditStackImpl(const FFaerieStorageEntry& Entry, const FFaerieStackKey Stack, const FFaerieInventoryTag EditTag) const
{
	// By default, some removal reasons are allowed, unless an extension explicitly disallows it.
	const bool DefaultAllowed = Inventory::Tags::EditTagsAllowedByDefault().Contains(EditTag);

	const FFaerieStorageEntry::FStackReadAccess ReadAccess = Entry.GetStackReadAccess(EntryMap, Stack);
	switch (Extensions::FGroupAPI::AllowsEdit(Extensions, this, ReadAccess, EditTag))
	{
	case EEventExtensionResponse::NoExplicitResponse:	return DefaultAllowed;
	case EEventExtensionResponse::Allowed:				return true;
	case EEventExtensionResponse::Disallowed:			return false;
	default: return false;
	}
}

bool UFaerieItemStorage::CanRemoveEntryImpl(const FFaerieStorageEntry& Entry, const FFaerieInventoryTag Reason) const
{
	// By default, some removal reasons are allowed, unless an extension explicitly disallows it.
	bool Allowed = Inventory::Tags::RemovalTagsAllowedByDefault().Contains(Reason);

	for (auto It = Storage::FIterator_SingleEntry_WithInterface(this, Entry); It.IsValid(); It.Advance())
	{
		switch (Extensions::FGroupAPI::AllowsRemoval(Extensions, this, It, Reason))
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

bool UFaerieItemStorage::CanRemoveStackImpl(const FFaerieStorageEntry& Entry, const FFaerieStackKey Stack, const FFaerieInventoryTag Reason) const
{
	const FFaerieStorageEntry::FStackReadAccess ReadAccess = Entry.GetStackReadAccess(EntryMap, Stack);

	// By default, some removal reasons are allowed, unless an extension explicitly disallows it.
	const bool DefaultAllowed = Inventory::Tags::RemovalTagsAllowedByDefault().Contains(Reason);

	switch (Extensions::FGroupAPI::AllowsRemoval(Extensions, this, ReadAccess, Reason))
	{
	case EEventExtensionResponse::NoExplicitResponse:	return DefaultAllowed;
	case EEventExtensionResponse::Allowed:				return true;
	case EEventExtensionResponse::Disallowed:			return false;
	default: return false;
	}
}


/**------------------------------*/
	/*	 STORAGE API - ALL USERS   */
	/**------------------------------*/

FFaerieAddress UFaerieItemStorage::MakeAddress(const FFaerieEntryKey Entry, const FFaerieStackKey Stack)
{
	return Storage::Address::Encode(Entry, Stack);
}

FFaerieEntryKey UFaerieItemStorage::GetAddressEntry(const FFaerieAddress Address)
{
	FFaerieEntryKey Key;
	Storage::Address::Decode_Entry(Address, Key);
	return Key;
}

FFaerieStackKey UFaerieItemStorage::GetAddressStack(const FFaerieAddress Address)
{
	FFaerieStackKey Key;
	Storage::Address::Decode_Stack(Address, Key);
	return Key;
}

TTuple<FFaerieEntryKey, FFaerieStackKey> UFaerieItemStorage::BreakAddress(const FFaerieAddress Address)
{
	FFaerieEntryKey Entry;
	FFaerieStackKey Stack;
	Storage::Address::Decode(Address, Entry, Stack);
	return MakeTuple(Entry, Stack);
}

int32 UFaerieItemStorage::GetStack(const FFaerieAddress Address) const
{
	FFaerieEntryKey Entry;
	FFaerieStackKey Stack;
	Storage::Address::Decode(Address, Entry, Stack);
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
	Storage::Address::Decode(Address, Entry, Stack);
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
			Out.Add(Storage::Address::Encode(Key, Stack.Key));
		}

		checkSlow(!Out.IsEmpty())
	}

	return Out;
}

void UFaerieItemStorage::GetAllKeys(TArray<FFaerieEntryKey>& Keys) const
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
	Storage::Address::Decode(Address, Entry, Stack);
	if (const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Entry))
	{
		return EntryPtr->Contains(Stack);
	}
	return false;
}

bool UFaerieItemStorage::ContainsItem(const FFaerieItemInstance& Item) const
{
	return !!FindEntry(Item);
}

FFaerieEntryKey UFaerieItemStorage::FindItem(const FFaerieItemInstance& Item) const
{
	if (!Item.IsValid())
	{
		return FFaerieEntryKey();
	}

	if (const FFaerieStorageEntry* Entry = FindEntry(Item))
	{
		return Entry->GetKey();
	}

	return FFaerieEntryKey();
}

FFaerieAddress UFaerieItemStorage::GetFirstAddress() const
{
	if (EntryMap.IsEmpty()) return FFaerieAddress();
	const FFaerieStorageEntry& FirstEntry = EntryMap.Entries[0];
	return Storage::Address::Encode(FirstEntry.GetKey(), FirstEntry.GetStacks()[0].Key);
}

FFaerieItemInstance UFaerieItemStorage::GetEntryItem(const FFaerieEntryKey Key) const
{
	if (const FFaerieStorageEntry* Entry = GetEntrySafe(Key))
	{
		return Entry->GetInstance();
	}
	return FFaerieItemInstance();
}

bool UFaerieItemStorage::CanAddStack(const FFaerieItemDataView& View, const EFaerieStorageAddStackBehavior AddStackBehavior) const
{
	if (!View.IsValid())
	{
		return false;
	}

	if (const FFaerieItemInstance Instance = View.GetInstance();
		Instance.IsMutable())
	{
		ItemData::FRequireEntityManager EntityManager(this);
		// Prevent recursive storage for mutable items
		// @todo replace with non-allocating solution that early outs
		TArray<TNotNull<UFaerieItemContainerBase*>> Containers;
		SubObject::GetContainersInInstanceRecursive(EntityManager, Instance, Containers);
		if (Containers.Contains(this))
		{
			return false;
		}
	}

	const FFaerieExtensionAllowsAdditionArgs CanAddStackArgs { AddStackBehavior };

	switch (Extensions::FGroupAPI::AllowsAddition(Extensions, this, MakeConstArrayView(&View, 1), CanAddStackArgs))
	{
	case EEventExtensionResponse::NoExplicitResponse:
	case EEventExtensionResponse::Allowed:				return true;
	case EEventExtensionResponse::Disallowed:			return false;
	default: return false;
	}
}

bool UFaerieItemStorage::CanAddStacks(const TArray<FFaerieItemDataView>& Views, const FFaerieExtensionAllowsAdditionArgs Args) const
{
	ItemData::FRequireEntityManager EntityManager(this);
	for (const FFaerieItemDataView& Stack : Views)
	{
		if (!Stack.IsValid())
		{
			return false;
		}

		if (const FFaerieItemInstance Instance = Stack.GetInstance();
			Instance.IsMutable())
		{
			// Prevent recursive storage for mutable items
			// @todo replace with non-allocating solution that early outs
			TArray<TNotNull<UFaerieItemContainerBase*>> Containers;
			SubObject::GetContainersInInstanceRecursive(EntityManager, Instance, Containers);
			if (Containers.Contains(this))
			{
				return false;
			}
		}
	}

	switch (Extensions::FGroupAPI::AllowsAddition(Extensions, this, Views, Args))
	{
	case EEventExtensionResponse::NoExplicitResponse:
	case EEventExtensionResponse::Allowed:				return true;
	case EEventExtensionResponse::Disallowed:			return false;
	default: return false;
	}
}

bool UFaerieItemStorage::CanEditStack(const FFaerieAddress Address, const FFaerieInventoryTag EditTag) const
{
	FFaerieEntryKey Entry;
	FFaerieStackKey Stack;
	Storage::Address::Decode(Address, Entry, Stack);
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
	FFaerieEntryKey Entry;
	FFaerieStackKey Stack;
	Storage::Address::Decode(Address, Entry, Stack);
	if (const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Entry))
	{
		const FFaerieStorageEntry::FStackReadAccess ReadAccess = EntryPtr->GetStackReadAccess(EntryMap, Stack);

		// By default, some removal reasons are allowed, unless an extension explicitly disallows it.
		const bool DefaultAllowed = Inventory::Tags::RemovalTagsAllowedByDefault().Contains(Reason);

		switch (Extensions::FGroupAPI::AllowsRemoval(Extensions, this, ReadAccess, Reason))
		{
		case EEventExtensionResponse::NoExplicitResponse:	return DefaultAllowed;
		case EEventExtensionResponse::Allowed:				return true;
		case EEventExtensionResponse::Disallowed:			return false;
		default: return false;
		}
	}
	return false;
}


	/**---------------------------------*/
	/*	 STORAGE API - AUTHORITY ONLY   */
	/**---------------------------------*/

bool UFaerieItemStorage::AddEntryFromInstance(const FFaerieItemInstance& Instance, const EFaerieStorageAddStackBehavior AddStackBehavior)
{
	const FFaerieItemDataView DataView(Instance, 1, nullptr);
	if (!CanAddStack(DataView, AddStackBehavior))
	{
		return false;
	}

	(void)AddStackImpl(DataView, Storage::IfOnlyNewStacks(AddStackBehavior));
	return true;
}

bool UFaerieItemStorage::AddItemStack(const FFaerieUnownedItemStack& Stack, const EFaerieStorageAddStackBehavior AddStackBehavior)
{
	const FFaerieItemDataView StackView(Stack.Instance, Stack.Copies, nullptr);
	if (!CanAddStack(StackView, AddStackBehavior))
	{
		return false;
	}

	(void)AddStackImpl(StackView, Storage::IfOnlyNewStacks(AddStackBehavior));
	return true;
}

void UFaerieItemStorage::AddItemStack(const FFaerieUnownedItemStack& Stack,
	const EFaerieStorageAddStackBehavior AddStackBehavior, TValueOrError<Inventory::FEventData, FText>& OutResult)
{
	const FFaerieItemDataView StackView(Stack.Instance, Stack.Copies, nullptr);
	if (!CanAddStack(StackView, AddStackBehavior))
	{
		OutResult = MakeError(Storage::AdditionFailure_FailedCanAddStack);
		return;
	}

	OutResult = MakeValue(AddStackImpl(StackView, Storage::IfOnlyNewStacks(AddStackBehavior)));
}

bool UFaerieItemStorage::AddItemStacks(const TConstArrayView<FFaerieUnownedItemStack> Stacks, const EFaerieStorageAddStackBehavior AddStackBehavior, const bool StopAfterFailure)
{
	SCOPE_CYCLE_COUNTER(STAT_Storage_AddMulti);

	const bool ForceNewStack = Storage::IfOnlyNewStacks(AddStackBehavior);

	TArray<Inventory::FEventData> Events;
	Events.Reserve(Stacks.Num());
	const Inventory::FEventLogBatch Batch(Events, Inventory::Tags::Addition);

	ON_SCOPE_EXIT
	{
		if (!Batch.Data.IsEmpty())
		{
			// Execute PostEventBatch on all extensions with the finished Event
			Extensions::FGroupAPI::PostEventBatch(Extensions, this, Batch);
		}
	};

	for (const FFaerieUnownedItemStack& Stack : Stacks)
	{
		if (!ensureAlwaysMsgf(
			Stack.IsValid(),
			TEXT("AddStackImpl was passed an invalid item view.")))
		{
			continue;
		}

		const FFaerieItemDataView StackView(Stack.Instance, Stack.Copies, nullptr);
		if (!CanAddStack(StackView, AddStackBehavior))
		{
			if (StopAfterFailure)
			{
				return false;
			}
			continue;
		}

		// Execute PreAddition on all extensions
		Extensions::FGroupAPI::PreAddition(Extensions, this, StackView);

		Events.Add(AddStackImplNoBroadcast(StackView, ForceNewStack));
	}

	return true;
}

void UFaerieItemStorage::AddItemStackBulk(const TArray<FFaerieUnownedItemStack>& Stacks, const EFaerieStorageAddStackBehavior AddStackBehavior, const bool StopAfterFailure)
{
	AddItemStacks(Stacks, AddStackBehavior, StopAfterFailure);
}

bool UFaerieItemStorage::AddItemStackWithLog(const FFaerieUnownedItemStack& Stack, const EFaerieStorageAddStackBehavior AddStackBehavior, FFaerieBlueprintInventoryEvent& Event)
{
	TValueOrError<Inventory::FEventData, FText> Result = MakeError(FText::GetEmpty());
	AddItemStack(Stack, AddStackBehavior, Result);
	if (Result.HasValue())
	{
		Event = FFaerieBlueprintInventoryEvent::FromNativeEvent(this, Inventory::Tags::Addition, Result.GetValue(), FDateTime::UtcNow());
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
	Storage::Address::Decode(Address, Entry, Stack);
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
			const Inventory::FEventData Event = RemoveFromEntryImpl(*EntryPtr, Amount, RemovalTag);
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
	Storage::Address::Decode(Address, Entry, Stack);
	if (const FFaerieStorageEntry* EntryPtr = GetEntrySafe(Entry))
	{
		if (CanRemoveStackImpl(*EntryPtr, Stack, RemovalTag))
		{
			const Inventory::FEventData Event = RemoveFromStackImpl(*EntryPtr, Stack, Amount, RemovalTag);
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

	const TArray<FFaerieEntryKey> Entries = CopyEntryKeys();

	TArray<Inventory::FEventData> Events;
	Events.Reserve(Entries.Num());
	const Inventory::FEventLogBatch Batch(Events, RemovalTag);

	for (const FFaerieEntryKey EntryKey : Entries)
	{
		const FFaerieStorageEntry& Entry = EntryMap[EntryKey];

		if (!CanRemoveEntryImpl(Entry, Inventory::Tags::RemovalMoving))
		{
			continue;
		}

		const FFaerieStorageEntry::FReadAccess ReadAccess = Entry.GetReadAccess(EntryMap);
		Extensions::FGroupAPI::PreRemoval(Extensions, this, ReadAccess, ItemData::EntireStack);

		// RemoveFromEntryImplNoBroadcast should not be called with unvalidated parameters.
		Events.Add(RemoveFromEntryImplNoBroadcast(Entry, ItemData::EntireStack));
	}

	Extensions::FGroupAPI::PostEventBatch(Extensions, this, Batch);

	if (RemovalTag == Inventory::Tags::RemovalDeletion)
	{
		ItemData::FRequireEntityManager EntityManager(this);
		for (Inventory::FEventData& Event : Events)
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
	Storage::Address::Decode(Address, Entry, Stack);
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

	FFaerieItemDataView View(EntryPtr->GetInstance(), StackValue, this);
	if (Amount > 0)
	{
		View.SetCopies(FMath::Min(StackValue, Amount));
	}

	if (!ToStorage->CanAddStack(View, AddStackBehavior))
	{
		return FFaerieEntryKey::InvalidKey;
	}

	const Inventory::FEventData Event = RemoveFromStackImpl(*EntryPtr, Stack, Amount, Inventory::Tags::RemovalMoving);

	const FFaerieItemDataView DataView(Event.Instance, Event.Copies, nullptr);
	return ToStorage->AddStackImpl(DataView, Storage::IfOnlyNewStacks(AddStackBehavior)).EntryTouched;
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
	if (!ToStorage->CanAddStack(FFaerieItemDataView(ReadAccess), AddStackBehavior))
	{
		return FFaerieEntryKey::InvalidKey;
	}

	const Inventory::FEventData RemoveResult = RemoveFromEntryImpl(*EntryPtr,
		ItemData::EntireStack, Inventory::Tags::RemovalMoving);

	const FFaerieItemDataView DataView(RemoveResult.Instance, RemoveResult.Copies, nullptr);
	return ToStorage->AddStackImpl(DataView, Storage::IfOnlyNewStacks(AddStackBehavior)).EntryTouched;
}

bool UFaerieItemStorage::MergeStacks(const FFaerieEntryKey Entry, const FFaerieStackKey FromStack, const FFaerieStackKey ToStack, const int32 Amount)
{
	const FFaerieAddress FromAddress = Storage::Address::Encode(Entry, FromStack);
	const FFaerieAddress ToAddress = Storage::Address::Encode(Entry, ToStack);

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

	Inventory::FEventData Event(Entry);
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
	}
	// Close Mutable scope

	Extensions::FGroupAPI::PostEvent(Extensions, this, Event, Inventory::Tags::Merge);

	return true;
}

bool UFaerieItemStorage::SplitStack(const FFaerieAddress Address, const int32 Amount)
{
	// Decode and verify the stack exists.
	FFaerieEntryKey Entry;
	FFaerieStackKey Stack;
	Storage::Address::Decode(Address, Entry, Stack);
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

	Inventory::FEventData Event(Entry);
	Event.Instance = EntryPtr->GetInstance();
	Event.Copies = Amount;
	Event.AddressesTouched.Add(Address);

	// Split the stack
	{
		FFaerieStorageEntry::FReadWriteAccess Handle = EntryPtr->GetReadWriteAccess(EntryMap);
		const FFaerieStackKey SplitStack = Handle.SplitStack(Stack, Amount);

		// Update event with final Address information
		Event.AddressesTouched.Add(Storage::Address::Encode(Entry, SplitStack));
	}

	Extensions::FGroupAPI::PostEvent(Extensions, this, Event, Inventory::Tags::Split);

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

	const TArray<FFaerieEntryKey> Entries = CopyEntryKeys();

	TArray<Inventory::FEventData> Events;
	Events.Reserve(Entries.Num());
	const Inventory::FEventLogBatch EventBatch(Events, Inventory::Tags::RemovalMoving);

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
		if (!ToStorage->CanAddStack(FFaerieItemDataView(ReadAccess), DumpBehavior))
		{
			continue;
		}

		Extensions::FGroupAPI::PreRemoval(Extensions, this, ReadAccess, ItemData::EntireStack);

		Inventory::FEventData& Event = Events.Add_GetRef(RemoveFromEntryImplNoBroadcast(Entry, ItemData::EntireStack));

		Stacks.Emplace(Event.Instance, Event.Copies);
	}

	Extensions::FGroupAPI::PostEventBatch(Extensions, this, EventBatch);

	ToStorage->AddItemStacks(Stacks, DumpBehavior, false);
}

#undef LOCTEXT_NAMESPACE

/*
 * Footnote1: You might think that even at runtime we could reset the key during Clear, since all items are removed,
 * and therefor no entries exist, making 100 a valid starting point again, *except* that other entities might still
 * be holding onto FFaerieEntryKeys, which could be cached in at some point later when potentially the entry is once more
 * valid, but with a completely different item. So during runtime, the Key must always increment.
 */