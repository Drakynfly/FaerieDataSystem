// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemStackView.h"
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
	virtual bool Server_Execute(const UFaerieInventoryClient* Client) const
		PURE_VIRTUAL(FFaerieClientActionBase::Server_Execute, return false; )
};

USTRUCT()
struct FAERIEINVENTORY_API FFaerieClientAction_MoveHandlerBase
{
	GENERATED_BODY()

	virtual ~FFaerieClientAction_MoveHandlerBase() = default;

	virtual bool IsValid(const UFaerieInventoryClient* Client) const
		PURE_VIRTUAL(FFaerieClientAction_MoveHandlerBase::IsValid, return false; )
	virtual bool View(FFaerieItemStackView& View) const
		PURE_VIRTUAL(FFaerieClientAction_MoveHandlerBase::View, return false; )
	virtual bool CanMove(const FFaerieItemStackView& View) const
		PURE_VIRTUAL(FFaerieClientAction_MoveHandlerBase::CanMove, return false; )
	virtual bool Possess(const FFaerieItemStack& Stack) const
		PURE_VIRTUAL(FFaerieClientAction_MoveHandlerBase::Possess, return false; )
	virtual bool Release(FFaerieItemStack& Stack) const
		PURE_VIRTUAL(FFaerieClientAction_MoveHandlerBase::Release, return false; )

	// Only needs to be implemented for Target handlers
	virtual bool IsSwap() const { return false; }
};