// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemProxy.h"
#include "FaerieUnownedItemStack.h"
#include "ValidParameter.h"

#include "FaerieClientActionBase.generated.h"

class UFaerieInventoryClient;

USTRUCT()
struct FAERIEINVENTORY_API FFaerieClientActionBase
{
	GENERATED_BODY()

	virtual ~FFaerieClientActionBase() = default;

	/*
	 * Runs on the server when called by UFaerieInventoryClient::RequestExecuteAction.
	 * Use this to implement Client-to-Server edits to item storage.
	 */
	virtual bool Server_Execute(TNotNull<const UFaerieInventoryClient*> Client) const
		PURE_VIRTUAL(FFaerieClientActionBase::Server_Execute, return false; )
};

USTRUCT()
struct FAERIEINVENTORY_API FFaerieClientAction_MoveHandlerBase
{
	GENERATED_BODY()

	virtual ~FFaerieClientAction_MoveHandlerBase() = default;

	// Called on any Move before attempting.
	virtual bool IsValid(TNotNull<const UFaerieInventoryClient*> Client) const
		PURE_VIRTUAL(FFaerieClientAction_MoveHandlerBase::IsValid, return false; )

	// Called on any MoveFrom. Called on MoveTo only when attempting a Swap.
	virtual bool View(Faerie::ItemData::FScopeProxy& Proxy) const
		PURE_VIRTUAL(FFaerieClientAction_MoveHandlerBase::View, return false; )

	// Called on any MoveTo. Called on MoveFrom only when attempting a Swap.
	virtual bool CanMove(Faerie::TValid<const FFaerieItemProxy&> Proxy) const
		PURE_VIRTUAL(FFaerieClientAction_MoveHandlerBase::CanMove, return false; )

	// Called on any MoveTo. Called on MoveFrom only when attempting a Swap.
	virtual bool Possess(Faerie::TValid<const FFaerieUnownedItemStack&> Stack) const
		PURE_VIRTUAL(FFaerieClientAction_MoveHandlerBase::Possess, return false; )

	// Called on any MoveFrom. Called on MoveTo only when attempting a Swap.
	virtual bool Release(FFaerieUnownedItemStack& Stack) const
		PURE_VIRTUAL(FFaerieClientAction_MoveHandlerBase::Release, return false; )

	// Only needs to be implemented for MoveTo handlers. Requires all functions to be implemented.
	virtual bool IsSwap() const { return false; }
};