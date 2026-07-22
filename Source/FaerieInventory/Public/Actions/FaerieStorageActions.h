// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieClientActionBase.h"
#include "FaerieStorageEnums.h"
#include "FaerieStorageStructs.h"
#include "FaerieStorageActions.generated.h"

class UFaerieItemStorage;

USTRUCT(BlueprintType)
struct FFaerieClientAction_MoveFromStorage final : public FFaerieClientAction_MoveHandlerBase
{
	GENERATED_BODY()

	virtual bool IsValid(TNotNull<const UFaerieInventoryClient*> Client) const override;
	virtual bool View(FFaerieItemDataView& View) const override;
	virtual bool CanMove(const FFaerieItemDataView& View) const override;
	virtual bool Release(FFaerieUnownedItemStack& Stack) const override;
	virtual bool Possess(const FFaerieUnownedItemStack& Stack) const override;

	UPROPERTY(BlueprintReadWrite, Category = "MoveFromStorage")
	TObjectPtr<UFaerieItemStorage> Storage = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "MoveFromStorage")
	FFaerieAddress Address;

	UPROPERTY(BlueprintReadWrite, Category = "MoveFromStorage")
	int32 Amount = -1;
};

USTRUCT(BlueprintType)
struct FFaerieClientAction_MoveToStorage final : public FFaerieClientAction_MoveHandlerBase
{
	GENERATED_BODY()

	virtual bool IsValid(TNotNull<const UFaerieInventoryClient*> Client) const override;
	virtual bool CanMove(const FFaerieItemDataView& View) const override;
	virtual bool Possess(const FFaerieUnownedItemStack& Stack) const override;

	// MoveToStorage doesn't support swaps.
	virtual bool View(FFaerieItemDataView& View) const override { return false; }
	virtual bool Release(FFaerieUnownedItemStack& Stack) const override { return false; }

	UPROPERTY(BlueprintReadWrite, Category = "MoveToStorage")
	TObjectPtr<UFaerieItemStorage> Storage = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "MoveToStorage")
	EFaerieStorageAddStackBehavior AddStackBehavior = EFaerieStorageAddStackBehavior::AddToAnyStack;
};

USTRUCT(BlueprintType)
struct FFaerieClientAction_DeleteEntry final : public FFaerieClientActionBase
{
	GENERATED_BODY()

	virtual bool Server_Execute(TNotNull<const UFaerieInventoryClient*> Client) const override;

	UPROPERTY(BlueprintReadWrite, Category = "DeleteEntry")
	TObjectPtr<UFaerieItemStorage> Storage = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "DeleteEntry")
	FFaerieAddress Address;

	UPROPERTY(BlueprintReadWrite, Category = "DeleteEntry")
	int32 Amount = -1;
};

USTRUCT(BlueprintType)
struct FFaerieClientAction_RequestMoveEntry final : public FFaerieClientActionBase
{
	GENERATED_BODY()

	virtual bool Server_Execute(TNotNull<const UFaerieInventoryClient*> Client) const override;

	UPROPERTY(BlueprintReadWrite, Category = "MoveEntry")
	TObjectPtr<UFaerieItemStorage> Storage = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "MoveEntry")
	FFaerieAddress Address;

	UPROPERTY(BlueprintReadWrite, Category = "MoveEntry")
	int32 Amount = -1;

	UPROPERTY(BlueprintReadWrite, Category = "MoveEntry")
	TObjectPtr<UFaerieItemStorage> ToStorage = nullptr;
};

USTRUCT(BlueprintType)
struct FFaerieClientAction_MergeStacks final : public FFaerieClientActionBase
{
	GENERATED_BODY()

	virtual bool Server_Execute(TNotNull<const UFaerieInventoryClient*> Client) const override;

	UPROPERTY(BlueprintReadWrite, Category = "MergeStacks")
	TObjectPtr<UFaerieItemStorage> Storage = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "MergeStacks")
	FFaerieEntryKey Entry;

	UPROPERTY(BlueprintReadWrite, Category = "MergeStacks")
	FFaerieStackKey FromStack;

	UPROPERTY(BlueprintReadWrite, Category = "MergeStacks")
	FFaerieStackKey ToStack;

	// Amount to move from A to B. If equal to -1, the entire stack will attempt to merge.
	UPROPERTY(BlueprintReadWrite, Category = "MergeStacks")
	int32 Amount = -1;
};

USTRUCT(BlueprintType)
struct FFaerieClientAction_SplitStack final : public FFaerieClientActionBase
{
	GENERATED_BODY()

	virtual bool Server_Execute(TNotNull<const UFaerieInventoryClient*> Client) const override;

	UPROPERTY(BlueprintReadWrite, Category = "SplitStack")
	TObjectPtr<UFaerieItemStorage> Storage = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "SplitStack")
	FFaerieAddress Address;

	UPROPERTY(BlueprintReadWrite, Category = "SplitStack")
	int32 Amount = 1;
};