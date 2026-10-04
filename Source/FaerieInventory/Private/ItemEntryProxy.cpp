// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "ItemEntryProxy.h"
#include "FaerieItem.h"
#include "FaerieItemStorage.h"
#include "FaerieInventoryLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemEntryProxy)

using namespace Faerie;

TOptional<FFaerieItemInstance> UFaerieItemEntryProxy::GetItemInstance() const
{
	if (!VerifyStatus())
	{
		return NullOpt;
	}

	return GetOuterUFaerieItemStorage()->ViewInstance(Key);
}

int32 UFaerieItemEntryProxy::GetCopies() const
{
	if (!VerifyStatus())
	{
		return 0;
	}

	return GetOuterUFaerieItemStorage()->GetEntryCopies(Key);
}

IFaerieItemOwnerInterface* UFaerieItemEntryProxy::GetItemOwner() const
{
	return GetOuterUFaerieItemStorage();
}

UFaerieItemStorage* UFaerieItemEntryProxy::GetItemStorage() const
{
	return GetOuterUFaerieItemStorage();
}

void UFaerieItemEntryProxy::NotifyLocalCreation()
{
	// If we are created on the server, or on a client for a pre-existing item, set Version to 0.
	LocalItemVersion = 0;
}

void UFaerieItemEntryProxy::NotifyDelayedCreation()
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

	CheckItemCount(GetCopies());
}

void UFaerieItemEntryProxy::NotifyUpdate()
{
	LocalItemVersion++;
	CheckItemCount(GetCopies());
}

void UFaerieItemEntryProxy::NotifyRemoval()
{
	LocalItemVersion = -1;
}

bool UFaerieItemEntryProxy::VerifyStatus() const
{
	const UFaerieItemStorage* Storage = GetOuterUFaerieItemStorage();

	if (!IsValid(Storage) || !Storage->ViewEntry(Key).IsValid())
	{
		UE_LOGF(LogFaerieInventory, Warning, "FaerieItemEntryProxy is invalid! Debug State will follow:")
		UE_LOGF(LogFaerieInventory, Warning, "     Entry Proxy: %ls", *GetName());
		UE_LOGF(LogFaerieInventory, Warning, "     Owning Storage: %ls", IsValid(Storage) ? *Storage->GetName() : TEXT("Invalid"));
		UE_LOGF(LogFaerieInventory, Warning, "     Key: %ls", *Key.ToString());
		UE_LOGF(LogFaerieInventory, Warning, "     Item Version : %i", LocalItemVersion);
		return false;
	}

	return true;
}
