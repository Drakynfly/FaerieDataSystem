// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "InventoryStorageProxy.h"
#include "FaerieItem.h"
#include "FaerieItemStorage.h"
#include "Logging.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryStorageProxy)

TArray<FKeyedStack> UInventoryEntryProxyBase::GetAllStacks() const
{
	if (const FInventoryEntryView& Entry = GetInventoryEntry();
		ensure(Entry.IsValid()))
	{
		return Entry.Get().Stacks;
	}
	return {};
}

int32 UInventoryEntryProxyBase::GetStackLimit() const
{
	if (const FInventoryEntryView& Entry = GetInventoryEntry();
		ensure(Entry.IsValid()))
	{
		return Entry.Get().Limit;
	}
	return 0;
}

const UFaerieItem* UInventoryEntryStorageProxy::GetItemObject() const
{
	if (!VerifyStatus())
	{
		return nullptr;
	}

	const FInventoryEntryView EntryView = GetStorage()->GetEntryView(GetKey());
	if (!ensure(EntryView.IsValid()))
	{
		return nullptr;
	}

	return EntryView.Get().ItemObject;
}

int32 UInventoryEntryStorageProxy::GetCopies() const
{
	if (!VerifyStatus())
	{
		return 0;
	}

	const FInventoryEntryView EntryView = GetStorage()->GetEntryView(GetKey());
	if (!ensure(EntryView.IsValid()))
	{
		return 0;
	}

	return EntryView.Get().StackSum();
}

TScriptInterface<IFaerieItemOwnerInterface> UInventoryEntryStorageProxy::GetOwner() const
{
	return GetStorage();
}

FInventoryEntryView UInventoryEntryStorageProxy::GetInventoryEntry() const
{
	if (!VerifyStatus())
	{
		return FInventoryEntryView();
	}

	return GetStorage()->GetEntryView(GetKey());
}

void UInventoryEntryStorageProxy::NotifyCreation()
{
	// If we are created on the server, or on a client for a pre-existing item, set Version to 0.
	// For clients that do not have the item replicated yet, -1 denotes awaiting initial replication.
	if (IsValid(GetItemObject()))
	{
		LocalItemVersion = 0;
	}
	else
	{
		LocalItemVersion = -1;
	}

	OnCacheUpdatedNative.Broadcast(this);
	OnCacheUpdated.Broadcast(this);
}

void UInventoryEntryStorageProxy::NotifyUpdate()
{
	LocalItemVersion++;
	OnCacheUpdatedNative.Broadcast(this);
	OnCacheUpdated.Broadcast(this);
}

void UInventoryEntryStorageProxy::NotifyRemoval()
{
	LocalItemVersion = -1;
	OnCacheRemovedNative.Broadcast(this);
	OnCacheRemoved.Broadcast(this);
}

bool UInventoryEntryStorageProxy::VerifyStatus() const
{
	auto&& Storage = GetStorage();
	auto&& Key = GetKey();

	if (!IsValid(Storage) || !Storage->IsValidKey(Key))
	{
		UE_LOG(LogFaerieInventory, Warning, TEXT("InventoryEntryProxy is invalid! Debug State will follow:"))\
		UE_LOG(LogFaerieInventory, Warning, TEXT("     Entry Cache: %s"), *GetName());
		UE_LOG(LogFaerieInventory, Warning, TEXT("     OwningInventory: %s"), IsValid(Storage) ? *Storage->GetName() : TEXT("Invalid"));
		UE_LOG(LogFaerieInventory, Warning, TEXT("     Key: %s"), *Key.ToString());
		UE_LOG(LogFaerieInventory, Warning, TEXT("     Item Version : %i"), LocalItemVersion);
		return false;
	}

	return true;
}

FEntryKey UInventoryEntryProxy::GetKey() const
{
	return Key;
}

int32 UInventoryStackProxy::GetCopies() const
{
	if (!VerifyStatus())
	{
		return 0;
	}

	const FInventoryEntryView EntryView = ItemStorage->GetEntryView(Key.EntryKey);
	if (!ensure(EntryView.IsValid()))
	{
		return 0;
	}

	return EntryView.Get().GetStack(Key.StackKey);
}

FEntryKey UInventoryStackProxy::GetKey() const
{
	return Key.EntryKey;
}

FInventoryKeyHandle UInventoryStackProxy::GetHandle() const
{
	return { ItemStorage, Key };
}