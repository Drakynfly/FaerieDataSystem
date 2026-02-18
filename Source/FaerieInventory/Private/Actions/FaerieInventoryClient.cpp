// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Actions/FaerieInventoryClient.h"
#include "FaerieItemStorage.h"
#include "FaerieInventoryLog.h"
#include "Actions/FaerieClientActionBase.h"
#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieInventoryClient)

UFaerieInventoryClient::UFaerieInventoryClient()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

bool UFaerieInventoryClient::CanAccessContainer(const TNotNull<const UFaerieItemContainerBase*> Container, const UScriptStruct* RequestType) const
{
	// @todo implement
	/*
	if (auto&& PermissionExtensions = Extensions::Get<UInventoryClientPermissionExtensions>(Container))
	{
		if (!PermissionExtensions->AllowsClient(this))
		{
			return false;
		}
	}
	*/
	return true;
}

void UFaerieInventoryClient::RequestExecuteAction(const FFaerieClientActionBase& Args)
{
	if (GetOwner()->HasAuthority())
	{
		// If called on a server, run immediately.
		Server_RequestExecuteAction(Args);
	}
	else
	{
		// Otherwise, use the RPC version.
		TInstancedStruct<FFaerieClientActionBase> ArgsWrapper;
		ArgsWrapper.InitializeAs(Args);
		RequestExecuteAction(ArgsWrapper);
	}
}

void UFaerieInventoryClient::RequestExecuteAction_Batch(const TArray<const FFaerieClientActionBase*>& Args, const EFaerieClientRequestBatchType Type)
{
	if (GetOwner()->HasAuthority())
	{
		// If called on a server, run immediately.
		Server_RequestExecuteAction_Batch(Args, Type);
	}
	else
	{
		// Otherwise, use the RPC version.
		TArray<TInstancedStruct<FFaerieClientActionBase>> ArrayWrapper;
		ArrayWrapper.Reserve(Args.Num());
		for (const FFaerieClientActionBase* Element : Args)
		{
			TInstancedStruct<FFaerieClientActionBase>& ElementWrapper = ArrayWrapper.AddDefaulted_GetRef();
			ElementWrapper.InitializeAs(*Element);
		}
		RequestExecuteAction_Batch(ArrayWrapper, Type);
	}
}

void UFaerieInventoryClient::RequestMoveAction(const FFaerieClientAction_MoveHandlerBase& MoveFrom,
											   const FFaerieClientAction_MoveHandlerBase& MoveTo)
{
	if (GetOwner()->HasAuthority())
	{
		// If called on a server, run immediately.
		Server_RequestMoveAction(MoveFrom, MoveTo);
	}
	else
	{
		// Otherwise, use the RPC version.
		TInstancedStruct<FFaerieClientAction_MoveHandlerBase> MoveFromWrapper;
		TInstancedStruct<FFaerieClientAction_MoveHandlerBase> MoveToWrapper;
		MoveFromWrapper.InitializeAs(MoveFrom);
		MoveToWrapper.InitializeAs(MoveTo);
		RequestMoveAction(MoveFromWrapper, MoveToWrapper);
	}
}

void UFaerieInventoryClient::RequestExecuteAction_Implementation(const TInstancedStruct<FFaerieClientActionBase>& Args)
{
	if (Args.IsValid())
	{
		Server_RequestExecuteAction(Args.Get());
	}
}

void UFaerieInventoryClient::RequestExecuteAction_Batch_Implementation(
	const TArray<TInstancedStruct<FFaerieClientActionBase>>& Args, const EFaerieClientRequestBatchType Type)
{
	TArray<const FFaerieClientActionBase*> ExecuteArray;
	for (auto&& Element : Args)
	{
		if (!Element.IsValid())
		{
			return;
		}
		ExecuteArray.Add(Element.GetPtr());
	}

	Server_RequestExecuteAction_Batch(ExecuteArray, Type);
}

void UFaerieInventoryClient::RequestMoveAction_Implementation(
	const TInstancedStruct<FFaerieClientAction_MoveHandlerBase>& MoveFrom,
	const TInstancedStruct<FFaerieClientAction_MoveHandlerBase>& MoveTo)
{
	// Ensure the client provided two valid structs.
	if (!MoveFrom.IsValid() ||
		!MoveTo.IsValid())
	{
		UE_LOG(LogFaerieInventory, Warning, TEXT("Client sent bad Move Request!"))
		return;
	}

	return Server_RequestMoveAction(MoveFrom.Get(), MoveTo.Get());
}

bool UFaerieInventoryClient::PromptStackChoice(const FFaerieClientStackPromptArgs& Args, const FFaerieClientStackPromptCallback& Callback)
{
	if (StackPromptHandler.IsBound())
	{
		ActivePromptCallback = Callback;
		StackPromptHandler.Execute(Args);
		return true;
	}
	return false;
}

void UFaerieInventoryClient::RespondToStackPrompt(const FFaerieAddressableHandle Handle, const int32 Amount)
{
	if (ActivePromptCallback.IsBound())
	{
		FFaerieClientStackPromptResult Result;
		Result.Client = this;
		Result.Handle = Handle;
		Result.Amount = Amount;
		ActivePromptCallback.Execute(Result);
		ActivePromptCallback.Clear();
	}
}

void UFaerieInventoryClient::SetStackChoicePromptHandler(const FFaerieClientStackPromptHandler& Handler)
{
	StackPromptHandler = Handler;
}

void UFaerieInventoryClient::Server_RequestExecuteAction(const FFaerieClientActionBase& Args)
{
	(void)Args.Server_Execute(this);
}

void UFaerieInventoryClient::Server_RequestExecuteAction_Batch(const TArray<const FFaerieClientActionBase*>& Args,
	const EFaerieClientRequestBatchType Type)
{
	switch (Type)
	{
	case EFaerieClientRequestBatchType::Individuals:
		for (auto&& Element : Args)
		{
			(void)Element->Server_Execute(this);
		}
		break;
	case EFaerieClientRequestBatchType::Sequence:
		for (auto&& Element : Args)
		{
			if (!Element->Server_Execute(this))
			{
				// Sequence failed, exit.
				return;
			}
		}
		break;
	}
}

void UFaerieInventoryClient::Server_RequestMoveAction(const FFaerieClientAction_MoveHandlerBase& MoveFrom,
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
