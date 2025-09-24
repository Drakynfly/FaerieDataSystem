// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "ItemContainerExtensionBase.h"
#include "FaerieItemStack.h"
#include "TypedGameplayTags.h"
#include "Actions/FaerieClientActionBase.h"

#include "InventoryEjectionHandlerExtension.generated.h"

class UFaerieItemStorage;

namespace Faerie::Inventory::Tags
{
	FAERIEINVENTORYCONTENT_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, RemovalEject)
}

class AFaerieItemOwningActorBase;

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
	virtual EEventExtensionResponse AllowsRemoval(const UFaerieItemContainerBase* Container, FFaerieAddress Address, FFaerieInventoryTag Reason) const override;
	virtual void PostRemoval(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event) override;
	//~ UItemContainerExtensionBase

private:
	void Enqueue(const FFaerieItemStack& Stack);

	void HandleNextInQueue();

	void PostLoadClassToSpawn(TSharedPtr<struct FStreamableHandle> Handle);
	void SpawnVisualizer(const TSubclassOf<AFaerieItemOwningActorBase>& Class);

protected:
	// Default visual actor when the item has no custom class.
	UPROPERTY(EditAnywhere, Category = "Config")
	TSoftClassPtr<AFaerieItemOwningActorBase> ExtensionDefaultClass;

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

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "EjectEntry")
	TWeakObjectPtr<UFaerieItemStorage> ItemStorage;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "EjectEntry")
	FFaerieAddress Address;

	UPROPERTY(BlueprintReadWrite, Category = "EjectEntry")
	int32 Amount = -1;
};

// Ejects an item generically by releasing the content.
USTRUCT(BlueprintType)
struct FFaerieClientAction_EjectViaRelease final : public FFaerieClientActionBase
{
	GENERATED_BODY()

	virtual bool Server_Execute(const UFaerieInventoryClient* Client) const override;

	UPROPERTY(BlueprintReadWrite, Category = "EjectViaRelease")
	FFaerieAddressableHandle Handle;

	UPROPERTY(BlueprintReadWrite, Category = "EjectViaRelease")
	int32 Amount = -1;
};