// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieClientActionBase.h"
#include "Components/ActorComponent.h"
#include "FaerieItemContainerStructs.h"
#include "StructUtils/InstancedStruct.h"
#include "FaerieInventoryClient.generated.h"

class UFaerieInventoryClient;
class UFaerieItemContainerBase;

UENUM()
enum class EFaerieClientRequestBatchType : uint8
{
	// This batch is for sending multiple individual requests at once. Each one will be run, even if some fail.
	Individuals,

	// This batch is for sending a sequence of requests. If one fails, no more will run.
	Sequence
};

USTRUCT(BlueprintType)
struct FFaerieClientStackPromptResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "ClientStackPromptResult")
	TObjectPtr<UFaerieInventoryClient> Client;

	UPROPERTY(BlueprintReadWrite, Category = "ClientStackPromptResult")
	FFaerieAddressableHandle Handle;

	UPROPERTY(BlueprintReadWrite, Category = "ClientStackPromptResult")
	int32 Amount = 0;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FFaerieClientStackPromptCallback, const FFaerieClientStackPromptResult&, Result);

USTRUCT(BlueprintType)
struct FFaerieClientStackPromptArgs
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "ClientStackPromptArgs")
	FFaerieAddressableHandle Handle;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FFaerieClientStackPromptHandler, const FFaerieClientStackPromptArgs&, Args);


/**
 * A component to add to client owned actors, that grants access to inventory functionality.
 */
UCLASS(ClassGroup = ("Faerie"), meta = (BlueprintSpawnableComponent),
	HideCategories = (Collision, ComponentTick, Replication, ComponentReplication, Activation, Sockets, Navigation))
class FAERIEINVENTORY_API UFaerieInventoryClient : public UActorComponent
{
	GENERATED_BODY()

public:
	UFaerieInventoryClient();

	// Overrides for allowing a client to run a request on the server.
	virtual bool CanAccessContainer(const UFaerieItemContainerBase* Container, const UScriptStruct* RequestType) const;

	/**
	 * Sends a request to the server to perform an inventory related edit.
	 * To define custom actions, derive a struct from FFaerieClientActionBase, and override Server_Execute.
	 */
	void RequestExecuteAction(const FFaerieClientActionBase& Args);

	/**
	 * Sends requests to the server to perform a batch of inventory related edits.
	 * To define custom actions, derive a struct from FFaerieClientActionBase, and override Server_Execute.
	 */
	void RequestExecuteAction_Batch(const TArray<const FFaerieClientActionBase*>& Args, EFaerieClientRequestBatchType Type);

	/**
	 * A specialized movement request.
	 * To define custom actions, derive a struct from FFaerieClientAction_MoveHandlerBase, and override everything.
	 */
	void RequestMoveAction(const FFaerieClientAction_MoveHandlerBase& MoveFrom, const FFaerieClientAction_MoveHandlerBase& MoveTo);

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "Faerie|InventoryClient")
	bool PromptStackChoice(const FFaerieClientStackPromptArgs& Args, const FFaerieClientStackPromptCallback& Callback);

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "Faerie|InventoryClient")
	void RespondToStackPrompt(FFaerieAddressableHandle Handle, int32 Amount);

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "Faerie|InventoryClient")
	void SetStackChoicePromptHandler(const FFaerieClientStackPromptHandler& Handler);

protected:
	/**
	 * Sends a request to the server to perform an inventory related edit.
	 * Args must be an InstancedStruct deriving from FFaerieClientActionBase.
	 * This can be used by calling MakeInstancedStruct, and passing any struct into it that is named like "FFaerieClientAction_...".
	 */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Faerie|InventoryClient")
	void RequestExecuteAction(const TInstancedStruct<FFaerieClientActionBase>& Args);

	/**
	 * Sends requests to the server to perform a batch of inventory related edits.
	 * Each Args struct must be an InstancedStruct deriving from FFaerieClientActionBase.
	 * This can be used by calling MakeInstancedStruct, and passing any struct into it that is named like "FFaerieClientAction_...".
	 */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Faerie|InventoryClient")
	void RequestExecuteAction_Batch(const TArray<TInstancedStruct<FFaerieClientActionBase>>& Args, EFaerieClientRequestBatchType Type);

	/**
	 * A specialized movement request.
	 * Args must be an InstancedStruct deriving from FFaerieClientAction_MoveHandlerBase.
	 * This can be used by calling MakeInstancedStruct, and passing any struct into it that is named like "FFaerieClientAction_Move...".
	 */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Faerie|InventoryClient")
	void RequestMoveAction(const TInstancedStruct<FFaerieClientAction_MoveHandlerBase>& MoveFrom, const TInstancedStruct<FFaerieClientAction_MoveHandlerBase>& MoveTo);

private:
	void Server_RequestExecuteAction(const FFaerieClientActionBase& Args);
	void Server_RequestExecuteAction_Batch(const TArray<const FFaerieClientActionBase*>& Args, EFaerieClientRequestBatchType Type);
	void Server_RequestMoveAction(const FFaerieClientAction_MoveHandlerBase& MoveFrom, const FFaerieClientAction_MoveHandlerBase& MoveTo);

	FFaerieClientStackPromptHandler StackPromptHandler;
	FFaerieClientStackPromptCallback ActivePromptCallback;
};