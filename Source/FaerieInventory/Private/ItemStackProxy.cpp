// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "ItemStackProxy.h"
#include "DelegateCommon.h"
#include "FaerieItem.h"
#include "FaerieItemStorage.h"
#include "FaerieInventoryLog.h"
#include "Tokens/FaerieStackLimiterToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemStackProxy)

using namespace Faerie;

namespace Faerie::Inventory
{
	UE_DEFINE_GAMEPLAY_TAG_TYPED(FFaerieInventoryTag, ProxyCreated, "Fae.Inventory.ProxyCreated")
	UE_DEFINE_GAMEPLAY_TAG_TYPED(FFaerieInventoryTag, ProxyUpdated, "Fae.Inventory.ProxyUpdated")
	UE_DEFINE_GAMEPLAY_TAG_TYPED(FFaerieInventoryTag, ProxyRemoved, "Fae.Inventory.ProxyRemoved")
}

const UFaerieItem* UFaerieItemStackProxy::GetItemObject() const
{
	if (!VerifyStatus())
	{
		return nullptr;
	}

	const FFaerieItemStackView EntryView = GetStorage()->View(GetKey());
	return EntryView.Item.Get();
}

int32 UFaerieItemStackProxy::GetCopies() const
{
	if (!VerifyStatus())
	{
		return 0;
	}

	return ItemStorage->ViewStack(Address).Copies;
}

TScriptInterface<IFaerieItemOwnerInterface> UFaerieItemStackProxy::GetItemOwner() const
{
	return GetStorage();
}

FDelegateHandle UFaerieItemStackProxy::BindToItemDataChanged(const FFaerieItemProxyChangedEvent& Event) const
{
	return const_cast<ThisClass*>(this)->OnProxyEvent.AddLambda(DYNAMIC_TO_LAMBDA(Event));
}

void UFaerieItemStackProxy::UnbindFromItemDataChanged(const FDelegateHandle& Handle) const
{
	const_cast<ThisClass*>(this)->OnProxyEvent.Remove(Handle);
}

void UFaerieItemStackProxy::UnbindAllFromItemDataChanged(const UObject* Object) const
{
	const_cast<ThisClass*>(this)->OnProxyEvent.RemoveAll(Object);
}

FFaerieItemStack UFaerieItemStackProxy::Release(const int32 Copies) const
{
	return ItemStorage->Release(GetKey(), Copies);
}

FEntryKey UFaerieItemStackProxy::GetKey() const
{
	return UFaerieItemStorage::GetAddressEntry(Address);
}

FFaerieAddressableHandle UFaerieItemStackProxy::GetAddressable() const
{
	return { ItemStorage, Address };
}

int32 UFaerieItemStackProxy::GetStackLimit() const
{
	return UFaerieStackLimiterToken::GetItemStackLimit(GetItemObject());
}

void UFaerieItemStackProxy::NotifyCreation()
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

	OnProxyEvent.Broadcast(this, Inventory::ProxyCreated);
	OnCacheUpdated.Broadcast(this, Inventory::ProxyCreated);
}

void UFaerieItemStackProxy::NotifyUpdate()
{
	LocalItemVersion++;
	OnProxyEvent.Broadcast(this, Inventory::ProxyUpdated);
	OnCacheUpdated.Broadcast(this, Inventory::ProxyUpdated);
}

void UFaerieItemStackProxy::NotifyRemoval()
{
	LocalItemVersion = -1;
	OnProxyEvent.Broadcast(this, Inventory::ProxyRemoved);
	OnCacheRemoved.Broadcast(this, Inventory::ProxyRemoved);
}

bool UFaerieItemStackProxy::VerifyStatus() const
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
