// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Actions/FaerieInventoryClient.h"
#include "FaerieItemStorage.h"
#include "Logging.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieInventoryClient)

UFaerieInventoryClient::UFaerieInventoryClient()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UFaerieInventoryClient::CanAccessContainer(const UFaerieItemContainerBase* Container, const UScriptStruct* RequestType) const
{
	// @todo implement
	/*
	if (auto&& PermissionExtensions = Storage->GetExtension<UInventoryClientPermissionExtensions>())
	{
		if (!PermissionExtensions->AllowsClient(this))
		{
			return false;
		}
	}
	*/
	return true;
}

bool UFaerieInventoryClient::CanAccessStorage(const UFaerieItemStorage* Storage, const UScriptStruct* RequestType) const
{
	return CanAccessContainer(Storage, RequestType);
}

bool FFaerieClientAction_MoveFromStorage::IsValid(const UFaerieInventoryClient* Client) const
{
	if (!::IsValid(Storage) ||
		!Client->CanAccessStorage(Storage, FFaerieClientAction_MoveHandlerBase::StaticStruct()) ||
		!Storage->IsValidKey(Key) ||
		!Faerie::ItemData::IsValidStack(Amount) ||
		!Storage->CanRemoveStack(Key, Faerie::Inventory::Tags::RemovalMoving))
	{
		return false;
	}
	return true;
}

bool FFaerieClientAction_MoveFromStorage::View(FFaerieItemStackView& View) const
{
	View = Storage->GetStackView(Key);
	if (Amount > 0)
	{
		View.Copies = FMath::Min(View.Copies, Amount);
	}
	return View.Item.IsValid();
}

bool FFaerieClientAction_MoveFromStorage::CanMove(const FFaerieItemStackView& View) const
{
	// @todo we might need to parameterize the StackBehavior
	return Storage->CanAddStack(View, EFaerieStorageAddStackBehavior::AddToAnyStack);
}

bool FFaerieClientAction_MoveFromStorage::Release(FFaerieItemStack& Stack) const
{
	return Storage->TakeStack(Key, Stack, Faerie::Inventory::Tags::RemovalMoving, Amount);
}

bool FFaerieClientAction_MoveFromStorage::Possess(const FFaerieItemStack& Stack) const
{
	// @todo we might need to parameterize the StackBehavior
	return Storage->AddItemStack(Stack, EFaerieStorageAddStackBehavior::AddToAnyStack);
}

bool FFaerieClientAction_MoveToStorage::IsValid(const UFaerieInventoryClient* Client) const
{
	return ::IsValid(Storage) &&
		Client->CanAccessStorage(Storage, FFaerieClientAction_MoveHandlerBase::StaticStruct());
}

bool FFaerieClientAction_MoveToStorage::CanMove(const FFaerieItemStackView& View) const
{
	return Storage->CanAddStack(View, AddStackBehavior);
}

bool FFaerieClientAction_MoveToStorage::Possess(const FFaerieItemStack& Stack) const
{
	return Storage->AddItemStack(Stack, AddStackBehavior);
}

void UFaerieInventoryClient::RequestExecuteAction_Implementation(const TInstancedStruct<FFaerieClientActionBase>& Args)
{
	if (Args.IsValid())
	{
		(void)Args.Get().Server_Execute(this);
	}
}

void UFaerieInventoryClient::RequestExecuteAction_Batch_Implementation(
	const TArray<TInstancedStruct<FFaerieClientActionBase>>& Args, const EFaerieClientRequestBatchType Type)
{
	for (auto&& Element : Args)
	{
		bool Ran = false;
		if (Element.IsValid())
		{
			Ran = Element.Get().Server_Execute(this);
		}

		if (!Ran && Type == EFaerieClientRequestBatchType::Sequence)
		{
			// Sequence failed, exit.
			return;
		}
	}
}

void UFaerieInventoryClient::RequestMoveAction_Implementation(
	const TInstancedStruct<FFaerieClientAction_MoveHandlerBase>& MoveFrom,
	const TInstancedStruct<FFaerieClientAction_MoveHandlerBase>& MoveTo)
{
	// Ensure client provided two valid structs.
	if (!MoveFrom.IsValid() ||
		!MoveTo.IsValid())
	{
		UE_LOG(LogFaerieInventory, Warning, TEXT("Client sent bad Move Request!"))
		return;
	}

	// Dereference and validate the struct properties.
	const FFaerieClientAction_MoveHandlerBase& MoveFromAction = MoveFrom.Get();
	const FFaerieClientAction_MoveHandlerBase& MoveToAction = MoveTo.Get();

	return RequestMoveAction(MoveFromAction, MoveToAction);
}

void UFaerieInventoryClient::RequestMoveAction(const FFaerieClientAction_MoveHandlerBase& MoveFrom,
											   const FFaerieClientAction_MoveHandlerBase& MoveTo)
{
	if (!MoveFrom.IsValid(this) ||
		!MoveTo.IsValid(this))
	{
		return;
	}

	FFaerieItemStackView FromView;
	if (!MoveFrom.View(FromView))
	{
		return;
	}

	if (!MoveTo.CanMove(FromView))
	{
		return;
	}

	const bool IsSwap = MoveTo.IsSwap();

	if (IsSwap)
	{
		FFaerieItemStackView ToView;
		MoveTo.View(ToView);
		if (!MoveFrom.CanMove(ToView))
		{
			return;
		}
	}

	// Finished validations, initiate move:

	FFaerieItemStack FromStack;
	if (!MoveFrom.Release(FromStack))
	{
		UE_LOG(LogFaerieInventory, Error, TEXT("Releasing for move failed! Validation should catch this!"))
		return;
	}

	if (IsSwap)
	{
		FFaerieItemStack ToStack;
		if (!MoveTo.Release(ToStack))
		{
			// Abort! Releasing for swap failed!
			UE_LOG(LogFaerieInventory, Error, TEXT("Releasing for swap failed! Validation should catch this!"))

			// Returning stack we removed.
			if (!MoveFrom.Possess(FromStack))
			{
				UE_LOG(LogFaerieInventory, Error, TEXT("Re-possess failed! Unable to recover from failed swap!"))
			}
			return;
		}
		if (!MoveFrom.Possess(ToStack))
		{
			UE_LOG(LogFaerieInventory, Error, TEXT("Swap failed! Issue with possession!"))
		}
	}

	if (!MoveTo.Possess(FromStack))
	{
		UE_LOG(LogFaerieInventory, Error, TEXT("Move failed! Issue with possession!"))
	}
}

bool FFaerieClientAction_DeleteEntry::Server_Execute(const UFaerieInventoryClient* Client) const
{
	auto&& Storage = Handle.ItemStorage.Get();
	if (!IsValid(Storage)) return false;
	if (!Client->CanAccessStorage(Storage, StaticStruct())) return false;

	return Storage->RemoveStack(Handle.Key, Faerie::Inventory::Tags::RemovalDeletion, Amount);
}

bool FFaerieClientAction_RequestMoveEntry::Server_Execute(const UFaerieInventoryClient* Client) const
{
	auto&& Storage = Handle.ItemStorage.Get();
	if (!IsValid(Storage)) return false;
	if (!IsValid(ToStorage)) return false;
	if (!Client->CanAccessStorage(Storage, StaticStruct())) return false;

	return Storage->MoveStack(ToStorage, Handle.Key, Amount).IsValid();
}

bool FFaerieClientAction_MergeStacks::Server_Execute(const UFaerieInventoryClient* Client) const
{
	if (!IsValid(Storage)) return false;
	if (!Client->CanAccessStorage(Storage, StaticStruct())) return false;
	return Storage->MergeStacks(Entry, FromStack, ToStack, Amount);
}

bool FFaerieClientAction_SplitStack::Server_Execute(const UFaerieInventoryClient* Client) const
{
	if (!IsValid(Storage)) return false;
	if (!Client->CanAccessStorage(Storage, StaticStruct())) return false;
	return Storage->SplitStack(Key.EntryKey, Key.StackKey, Amount);
}