// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "InventoryStorageProxy.h"
#include "FaerieItem.h"
#include "FaerieItemStorage.h"
#include "FaerieInventoryLog.h"
#include "Tokens/FaerieStackLimiterToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryStorageProxy)

using namespace Faerie;

const UFaerieItem* UInventoryStackProxy::GetItemObject() const
{
	if (!VerifyStatus())
	{
		return nullptr;
	}

	const FFaerieItemStackView EntryView = GetStorage()->View(GetKey());
	return EntryView.Item.Get();
}

int32 UInventoryStackProxy::GetCopies() const
{
	if (!VerifyStatus())
	{
		return 0;
	}

	return ItemStorage->ViewStack(Address).Copies;
}

TScriptInterface<IFaerieItemOwnerInterface> UInventoryStackProxy::GetItemOwner() const
{
	return GetStorage();
}

FFaerieItemStack UInventoryStackProxy::Release(const int32 Copies) const
{
	return ItemStorage->Release(GetKey(), Copies);
}

FEntryKey UInventoryStackProxy::GetKey() const
{
	return UFaerieItemStorage::GetAddressEntry(Address);
}

FFaerieAddressableHandle UInventoryStackProxy::GetAddressable() const
{
	return { ItemStorage, Address };
}

int32 UInventoryStackProxy::GetStackLimit() const
{
	return UFaerieStackLimiterToken::GetItemStackLimit(GetItemObject());
}

void UInventoryStackProxy::NotifyCreation()
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

	OnCacheUpdated.Broadcast(this);
}

void UInventoryStackProxy::NotifyUpdate()
{
	LocalItemVersion++;
	OnProxyEvent.Broadcast(this, Inventory::EStackProxyEventType::Updated);
	OnCacheUpdated.Broadcast(this);
}

void UInventoryStackProxy::NotifyRemoval()
{
	LocalItemVersion = -1;
	OnProxyEvent.Broadcast(this, Inventory::EStackProxyEventType::Removed);
	OnCacheRemoved.Broadcast(this);
}

bool UInventoryStackProxy::VerifyStatus() const
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
