// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemContainerStructs.h"
#include "ItemContainerExtensionBase.h"
#include "MassProcessor.h"
#include "TypedGameplayTags.h"
#include "Actions/FaerieClientActionBase.h"

#include "InventoryEjectionHandlerExtension.generated.h"

namespace Faerie::Inventory::Tags
{
	FAERIEINVENTORYCONTENT_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, RemovalEject)
}

namespace Faerie::Content
{
	struct FEjectionData
	{
		TWeakObjectPtr<AActor> Owner;
		FFaerieUnownedItemStack Stack;
	};
}

class AFaerieItemOwningActorBase;

/**
 * A container extension that allows items to be removed with the "Ejection" tag, and spawns pickups for them.
 */
USTRUCT()
struct FFaerieItemContainerEjectionConfig : public FFaerieItemContainerData
{
	GENERATED_BODY()

	void HandleNextInQueue(const Faerie::Content::FEjectionData& Ejection) const;
	void PostLoadClassToSpawn(TSharedPtr<struct FStreamableHandle> Handle, const Faerie::Content::FEjectionData Ejection) const;
	void SpawnVisualizer(const TSubclassOf<AFaerieItemOwningActorBase>& Class, const Faerie::Content::FEjectionData& Ejection) const;

	// Default visual actor when the item has no custom class.
	UPROPERTY(EditAnywhere, Category = "Config")
	TSoftClassPtr<AFaerieItemOwningActorBase> ExtensionDefaultClass;

	// Component to get a transform to spawn the actor with.
	UPROPERTY(VisibleInstanceOnly, Category = "Config")
	TObjectPtr<USceneComponent> RelativeSpawningComponent;

	// Relative transform to spawn the actor with.
	UPROPERTY(EditAnywhere, Category = "Config")
	FTransform RelativeSpawningTransform;
};

UCLASS()
class UFaerieContainerEjectionHandler : public UMassProcessor
{
	GENERATED_BODY()

public:
	UFaerieContainerEjectionHandler();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};

/*
 * Ejects a stack of items from a container, dropping it on the ground as a pickup. Requires the container to be
 * configured with a FFaerieItemContainerEjectionConfig
 */
USTRUCT(BlueprintType)
struct FFaerieClientAction_EjectEntry final : public FFaerieClientActionBase
{
	GENERATED_BODY()

	virtual bool Server_Execute(TNotNull<const UFaerieInventoryClient*> Client) const override;

	UPROPERTY(BlueprintReadWrite, Category = "EjectViaRelease")
	FFaerieItemNetworkHandle Handle;

	UPROPERTY(BlueprintReadWrite, Category = "EjectEntry")
	int32 Amount = -1;
};