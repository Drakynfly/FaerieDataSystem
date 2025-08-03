// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieContainerExtensionInterface.h"
#include "InventoryDataStructs.h"
#include "Components/ActorComponent.h"

#include "FaerieInventoryComponent.generated.h"

enum class EFaerieAddressEventType : uint8;
class UItemContainerExtensionGroup;
class UItemContainerExtensionBase;
class UFaerieItemStorage;

DECLARE_LOG_CATEGORY_EXTERN(LogFaerieInventoryComponent, Log, All);

/**
 *	This is the core of the inventory system. The actual component added to actors to allow them to contain item data.
 *	It supports extension objects which customize and add to its functionality, eg: adding capacity limits, or crafting features.
 */
UCLASS(ClassGroup = ("Faerie"), meta = (BlueprintSpawnableComponent, ChildCannotTick),
	HideCategories = (Collision, ComponentTick, Replication, ComponentReplication, Activation, Sockets, Navigation))
class FAERIEINVENTORY_API UFaerieInventoryComponent : public UActorComponent, public IFaerieContainerExtensionInterface
{
	GENERATED_BODY()

public:
	UFaerieInventoryComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//~ UActorComponent
	virtual void PostInitProperties() override;
	virtual void ReadyForReplication() override;
	//~ UActorComponent

	//~ IFaerieContainerExtensionInterface
	virtual UItemContainerExtensionGroup* GetExtensionGroup() const override final;
	virtual bool AddExtension(UItemContainerExtensionBase* Extension) override;
	virtual bool RemoveExtension(UItemContainerExtensionBase* Extension) override;
	//~ IFaerieContainerExtensionInterface

protected:
	virtual void HandleAddressEvent(UFaerieItemStorage* Storage, EFaerieAddressEventType Type, FFaerieAddress Address);


	/**------------------------------*/
	/*	 INVENTORY API - ALL USERS   */
	/**------------------------------*/

public:
	UFUNCTION(BlueprintCallable, Category = "Faerie|InventoryComponent")
	UFaerieItemStorage* GetStorage() const { return ItemStorage; }


	/**-------------*/
	/*	 VARIABLES	*/
	/**-------------*/
protected:
	UPROPERTY(BlueprintReadOnly, Replicated, Instanced, Category = "ItemStorage")
	TObjectPtr<UFaerieItemStorage> ItemStorage;

	// Subobjects responsible for adding or customizing Inventory behavior.
	UPROPERTY(EditAnywhere, Instanced, NoClear, Category = "ItemStorage")
	TObjectPtr<UItemContainerExtensionGroup> Extensions;
};