// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "ItemContainerExtensionBase.h"
#include "FaerieItemStack.h"
#include "TypedGameplayTags.h"
#include "Actions/FaerieInventoryClient.h"

#include "InventoryEjectionHandlerExtension.generated.h"

namespace Faerie::Inventory::Tags
{
	FAERIEINVENTORYCONTENT_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, RemovalEject)
}

class AItemRepresentationActor;

/**
 * An inventory extension that allows items to be removed from the inventory with the "Ejection" reason, and spawns
 * pickups for them.
 */
UCLASS()
class FAERIEINVENTORYCONTENT_API UInventoryEjectionHandlerExtension : public UItemContainerExtensionBase
{
	GENERATED_BODY()

	friend struct FFaerieClientAction_EjectViaRelease;

public:
	//~ UItemContainerExtensionBase
	virtual EEventExtensionResponse AllowsRemoval(const UFaerieItemContainerBase* Container, FEntryKey Key, FFaerieInventoryTag Reason) const override;
	virtual void PostRemoval(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event) override;
	//~ UItemContainerExtensionBase

private:
	void Enqueue(const FFaerieItemStack& Stack);

	void HandleNextInQueue();

	void PostLoadClassToSpawn(TSoftClassPtr<AItemRepresentationActor> ClassToSpawn);

protected:
	// Default visual actor when the item has no custom class.
	UPROPERTY(EditAnywhere, Category = "Config")
	TSoftClassPtr<AItemRepresentationActor> ExtensionDefaultClass;

	// Component to get a transform to spawn the actor with.
	UPROPERTY(BlueprintReadWrite, VisibleInstanceOnly, Category = "Config")
	TObjectPtr<USceneComponent> RelativeSpawningComponent;

	// Relative transform to spawn the actor with.
	UPROPERTY(EditAnywhere, Category = "Config")
	FTransform RelativeSpawningTransform;

	UPROPERTY()
	TArray<FFaerieItemStack> PendingEjectionQueue;

private:
	bool IsStreaming = false;
};

// Ejects an item in an Inventory Component
USTRUCT(BlueprintType)
struct FFaerieClientAction_EjectEntry final : public FFaerieClientActionBase
{
	GENERATED_BODY()

	virtual bool Server_Execute(const UFaerieInventoryClient* Client) const override;

	UPROPERTY(BlueprintReadWrite, Category = "RequestEjectEntry")
	FInventoryKeyHandle Handle;

	UPROPERTY(BlueprintReadWrite, Category = "RequestEjectEntry")
	int32 Amount = -1;
};

// Ejects an item generically by releasing the content.
USTRUCT(BlueprintType)
struct FFaerieClientAction_EjectViaRelease final : public FFaerieClientActionBase
{
	GENERATED_BODY()

	virtual bool Server_Execute(const UFaerieInventoryClient* Client) const override;

	UPROPERTY(BlueprintReadWrite, Category = "EjectViaRelease")
	TObjectPtr<UFaerieItemContainerBase> Container;

	UPROPERTY(BlueprintReadWrite, Category = "EjectViaRelease")
	FEntryKey Key;

	UPROPERTY(BlueprintReadWrite, Category = "EjectViaRelease")
	int32 Amount = -1;
};