// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"

#include "FaerieInventoryComponent.generated.h"

class UFaerieItemStorage;

/**
 *	This is the core of the inventory system. The actual component added to actors to allow them to contain item data.
 *	It supports extension objects which customize and add to its functionality, eg: adding capacity limits, or crafting features.
 */
UCLASS(ClassGroup = ("Faerie"), meta = (BlueprintSpawnableComponent, ChildCannotTick),
	HideCategories = (Collision, ComponentTick, Replication, ComponentReplication, Activation, Sockets, Navigation))
class FAERIEINVENTORY_API UFaerieInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFaerieInventoryComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//~ UActorComponent
	virtual void ReadyForReplication() override;
	//~ UActorComponent


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
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Replicated, Instanced, Category = "ItemStorage", meta = (ShowInnerProperties))
	TObjectPtr<UFaerieItemStorage> ItemStorage;
};