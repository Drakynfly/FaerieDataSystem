// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "ItemStackProxy.h"
#include "FaerieItem.h"
#include "FaerieItemStorage.h"
#include "FaerieInventoryLog.h"

#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemStackProxy)

using namespace Faerie;

namespace Faerie::Inventory
{
	UE_DEFINE_GAMEPLAY_TAG_TYPED(FFaerieInventoryTag, ProxyCreated, "Fae.Inventory.ProxyCreated")
	UE_DEFINE_GAMEPLAY_TAG_TYPED(FFaerieInventoryTag, ProxyUpdated, "Fae.Inventory.ProxyUpdated")
	UE_DEFINE_GAMEPLAY_TAG_TYPED(FFaerieInventoryTag, ProxyRemoved, "Fae.Inventory.ProxyRemoved")
}

UWorld* UFaerieItemStackProxy::GetWorld() const
{
	// If GetWorld is called on a StackProxy, it must be able to find it from its parent UFaerieItemStorage.
	UWorld* WorldFromSuperChain = GetTypedOuter<UWorld>();
	check(WorldFromSuperChain);
	return WorldFromSuperChain;
}

TOptional<FFaerieItemInstance> UFaerieItemStackProxy::GetItemInstance() const
{
	if (!VerifyStatus())
	{
		return NullOpt;
	}

	return ItemStorage->ViewInstance(Address);
}

int32 UFaerieItemStackProxy::GetCopies() const
{
	if (!VerifyStatus())
	{
		return 0;
	}

	return ItemStorage->GetStack(Address);
}

IFaerieItemOwnerInterface* UFaerieItemStackProxy::GetItemOwner() const
{
	return GetStorage();
}

FFaerieEntryKey UFaerieItemStackProxy::GetKey() const
{
	return UFaerieItemStorage::GetAddressEntry(Address);
}

FFaerieItemNetworkHandle UFaerieItemStackProxy::GetNetworkHandle() const
{
	return FFaerieItemNetworkHandle(ItemStorage, Address);
}

void UFaerieItemStackProxy::NotifyCreation()
{
	// If we are created on the server, or on a client for a pre-existing item, set Version to 0.
	// For clients that do not have the item replicated yet, -1 denotes awaiting initial replication.
	if (GetItemInstance().IsSet())
	{
		LocalItemVersion = 0;
	}
	else
	{
		LocalItemVersion = -1;
	}

	OnProxyEvent.Broadcast(FFaerieItemProxy(this), Inventory::ProxyCreated);
}

void UFaerieItemStackProxy::NotifyUpdate()
{
	LocalItemVersion++;
	OnProxyEvent.Broadcast(FFaerieItemProxy(this), Inventory::ProxyUpdated);
}

void UFaerieItemStackProxy::NotifyRemoval()
{
	LocalItemVersion = -1;
	OnProxyEvent.Broadcast(FFaerieItemProxy(this), Inventory::ProxyRemoved);
}

void UFaerieItemStackProxy::NotifyItemDataChanged(const FGameplayTag EditTag)
{
	OnProxyEvent.Broadcast(FFaerieItemProxy(this), EditTag);
}

bool UFaerieItemStackProxy::VerifyStatus() const
{
	UFaerieItemStorage* Storage = GetStorage();
	const FFaerieEntryKey Key = GetKey();

	if (!IsValid(Storage) || !Storage->ViewEntry(Key).IsValid())
	{
		UE_LOG(LogFaerieInventory, Warning, TEXT("InventoryStackProxy is invalid! Debug State will follow:"))\
		UE_LOG(LogFaerieInventory, Warning, TEXT("     Stack Proxy: %s"), *GetName());
		UE_LOG(LogFaerieInventory, Warning, TEXT("     Owning Storage: %s"), IsValid(Storage) ? *Storage->GetName() : TEXT("Invalid"));
		UE_LOG(LogFaerieInventory, Warning, TEXT("     Entry: %s"), *Key.ToString());
		UE_LOG(LogFaerieInventory, Warning, TEXT("     Item Version : %i"), LocalItemVersion);
		return false;
	}

	return true;
}
