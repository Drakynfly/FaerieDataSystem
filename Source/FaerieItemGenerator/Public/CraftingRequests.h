// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemProxy.h"
#include "GenerationAction.h"
#include "ItemSlotHandle.h"

#include "CraftingRequests.generated.h"

class IFaerieItemDataProxy;
class UItemGenerationConfig;
class UItemUpgradeConfig;
class UItemCraftingConfig;

// The client assembles these via UI and submits them to the server for validation when requesting an item generation.
USTRUCT(BlueprintType)
struct FAERIEITEMGENERATOR_API FGenerationRequest
{
	GENERATED_BODY()

	// The client must fill this with drivers that can have network ID mapping. This is automatic for serialized objects.
	// Runtime generated drivers must be created server-side and replicated for this to work.
	UPROPERTY(BlueprintReadWrite, Category = "Generation Request")
	TArray<TObjectPtr<UItemGenerationConfig>> Drivers;

	// The squirrel provided for deterministic generation (optional).
	UPROPERTY(BlueprintReadWrite, Category = "Crafting Request")
	TWeakObjectPtr<USquirrel> Squirrel;

	UPROPERTY(BlueprintReadWrite, Category = "Generation Request")
	FGenerationActionOnCompleteBinding OnComplete;
};

// Maps crafting slots to the item that the client wants to use.
USTRUCT(BlueprintType)
struct FAERIEITEMGENERATOR_API FRequestSlot
{
	GENERATED_BODY()

	// The resource slot this request slot is filling.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Request Slot")
	FFaerieItemSlotHandle SlotID;

	// The item that is being used to fill the slot.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Request Slot")
	FFaerieItemProxy ItemProxy;
};

// The client assembles these via UI and submits them to the server for validation when requesting an item craft.
USTRUCT(BlueprintType)
struct FAERIEITEMGENERATOR_API FCraftingRequest
{
	GENERATED_BODY()

	// These should be sourced from cooked assets, or spawned by the server, if needed at runtime. Clients cannot create them.
	UPROPERTY(BlueprintReadWrite, Category = "Crafting Request")
	TObjectPtr<UItemCraftingConfig> Config = nullptr;

	// @todo use array instead of single
	//TArray<TObjectPtr<UItemCraftingConfig> Configs;

	UPROPERTY(BlueprintReadWrite, Category = "Crafting Request")
	TArray<FRequestSlot> Slots;

	// The squirrel provided for deterministic generation (optional).
	UPROPERTY(BlueprintReadWrite, Category = "Crafting Request")
	TWeakObjectPtr<USquirrel> Squirrel;

	UPROPERTY(BlueprintReadWrite, Category = "Crafting Request")
	FGenerationActionOnCompleteBinding OnComplete;
};

// The client assembles these via UI and submits them to the server for validation when requesting an item upgrade.
USTRUCT(BlueprintType)
struct FAERIEITEMGENERATOR_API FUpgradeRequest
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Upgrade Request")
	TScriptInterface<IFaerieItemDataProxy> ItemProxy;

	// These should be safe to replicate, since they are (always?) sourced from cooked assets.
	UPROPERTY(BlueprintReadWrite, Category = "Upgrade Request")
	TObjectPtr<UItemUpgradeConfig> Config = nullptr;

	// @todo use array instead of single
	//TArray<TObjectPtr<UItemUpgradeConfig> Configs;

	// Note: Acts like map, but must be array for replication.
	UPROPERTY(BlueprintReadWrite, Category = "Upgrade Request")
	TArray<FRequestSlot> Slots;

	// The squirrel provided for deterministic generation (optional).
	UPROPERTY(BlueprintReadWrite, Category = "Crafting Request")
	TWeakObjectPtr<USquirrel> Squirrel;

	UPROPERTY(BlueprintReadWrite, Category = "Upgrade Request")
	FGenerationActionOnCompleteBinding OnComplete;
};