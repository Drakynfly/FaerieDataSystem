// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "ItemStackProxy.h"
#include "FaerieItem.h"
#include "FaerieItemStorage.h"
#include "FaerieInventoryLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemStackProxy)

using namespace Faerie;

FMassEntityHandle UFaerieItemStackProxy::GetEntityHandle() const
{
	if (!VerifyStatus())
	{
		return FMassEntityHandle();
	}

	return GetOuterUFaerieItemStorage()->GetItemHandle(Address);
}

int32 UFaerieItemStackProxy::GetCopies() const
{
	if (!VerifyStatus())
	{
		return 0;
	}

	return GetOuterUFaerieItemStorage()->GetStackCopies(Address);
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
	if (GetEntityHandle().IsSet())
	{
		LocalItemVersion = 0;
	}
	else
	{
		LocalItemVersion = -1;
	}

	CheckItemCount(GetOuterUFaerieItemStorage()->GetStackCopies(Address));
}

void UFaerieItemStackProxy::NotifyUpdate()
{
	LocalItemVersion++;
	CheckItemCount(GetOuterUFaerieItemStorage()->GetStackCopies(Address));
}

void UFaerieItemStackProxy::NotifyRemoval()
{
	LocalItemVersion = -1;
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
