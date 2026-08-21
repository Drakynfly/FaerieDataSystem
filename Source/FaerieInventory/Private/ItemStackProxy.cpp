// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "ItemStackProxy.h"
#include "FaerieItem.h"
#include "FaerieItemStorage.h"
#include "FaerieInventoryLog.h"

#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemStackProxy)

using namespace Faerie;

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

	return GetOuterUFaerieItemStorage()->ViewInstance(Address);
}

int32 UFaerieItemStackProxy::GetCopies() const
{
	if (!VerifyStatus())
	{
		return 0;
	}

	return GetOuterUFaerieItemStorage()->GetStackCopies(Address);
}

IFaerieItemOwnerInterface* UFaerieItemStackProxy::GetItemOwner() const
{
	return GetOuterUFaerieItemStorage();
}

FFaerieEntryKey UFaerieItemStackProxy::GetKey() const
{
	return UFaerieItemStorage::GetAddressEntry(Address);
}

FFaerieItemNetworkHandle UFaerieItemStackProxy::GetNetworkHandle() const
{
	return FFaerieItemNetworkHandle(GetOuterUFaerieItemStorage(), Address);
}

UFaerieItemStorage* UFaerieItemStackProxy::GetItemStorage() const
{
	return GetOuterUFaerieItemStorage();
}

void UFaerieItemStackProxy::NotifyLocalCreation()
{
	// If we are created on the server, or on a client for a pre-existing item, set Version to 0.
	LocalItemVersion = 0;
}

void UFaerieItemStackProxy::NotifyDelayedCreation()
{
	// If we are created on a client for a pre-existing item, set Version to 0.
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
	const UFaerieItemStorage* Storage = GetOuterUFaerieItemStorage();
	const FFaerieEntryKey Key = GetKey();

	if (!IsValid(Storage) || !Storage->ViewEntry(Key).IsValid())
	{
		UE_LOGF(LogFaerieInventory, Warning, "FaerieItemStackProxy is invalid! Debug State will follow:")
		UE_LOGF(LogFaerieInventory, Warning, "     Stack Proxy: %ls", *GetName());
		UE_LOGF(LogFaerieInventory, Warning, "     Owning Storage: %ls", IsValid(Storage) ? *Storage->GetName() : TEXT("Invalid"));
		UE_LOGF(LogFaerieInventory, Warning, "     Entry: %ls", *Key.ToString());
		UE_LOGF(LogFaerieInventory, Warning, "     Item Version : %i", LocalItemVersion);
		return false;
	}

	return true;
}
