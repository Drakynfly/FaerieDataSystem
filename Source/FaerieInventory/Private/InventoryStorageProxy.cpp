// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "InventoryStorageProxy.h"
#include "FaerieItem.h"
#include "FaerieItemStorage.h"
#include "FaerieInventoryLog.h"
#include "Tokens/FaerieStackLimiterToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryStorageProxy)

TArray<FKeyedStack> UInventoryEntryProxyBase::GetAllStacks() const
{
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	if (const TConstStructView<FInventoryEntry>& Entry = GetInventoryEntry();
		ensure(Entry.IsValid()))
	{
		return Entry.Get().Stacks;
	}
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	return {};
}

int32 UInventoryEntryProxyBase::GetStackLimit() const
{
	return UFaerieStackLimiterToken::GetItemStackLimit(GetItemObject());
}

const UFaerieItem* UInventoryEntryStorageProxy::GetItemObject() const
{
	if (!VerifyStatus())
	{
		return nullptr;
	}

	const FFaerieItemStackView EntryView = GetStorage()->View(GetKey());
	return EntryView.Item.Get();
}

int32 UInventoryEntryStorageProxy::GetCopies() const
{
	if (!VerifyStatus())
	{
		return 0;
	}

	const FFaerieItemStackView EntryView = GetStorage()->View(GetKey());
	return EntryView.Copies;
}

TScriptInterface<IFaerieItemOwnerInterface> UInventoryEntryStorageProxy::GetItemOwner() const
{
	return GetStorage();
}

TConstStructView<FInventoryEntry> UInventoryEntryStorageProxy::GetInventoryEntry() const
{
	if (!VerifyStatus())
	{
		return TConstStructView<FInventoryEntry>();
	}

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	return GetStorage()->GetEntryView(GetKey());
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
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

	if (!IsValid(Storage) || !Storage->Contains(Key))
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

	return ItemStorage->ViewStack(Key.ToAddress()).Copies;
}

FEntryKey UInventoryStackProxy::GetKey() const
{
	return Key.EntryKey;
}

FInventoryKeyHandle UInventoryStackProxy::GetHandle() const
{
	return { ItemStorage, Key };
}

FFaerieAddressableHandle UInventoryStackProxy::GetAddressable() const
{
	return { ItemStorage, Key.ToAddress() };
}
