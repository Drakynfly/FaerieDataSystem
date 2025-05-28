// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemOwnerInterface.h"
#include "Actors/ItemRepresentationActor.h"
#include "FaerieItemOwningActorBase.generated.h"

class UFaerieEquipmentSlot;

/*
 * Base class for Actors that can own a stack of items, e.g., pick-ups on the ground.
 */
UCLASS(Abstract)
class FAERIEINVENTORYCONTENT_API AFaerieItemOwningActorBase : public AItemRepresentationActor, public IFaerieItemOwnerInterface
{
	GENERATED_BODY()

public:
	AFaerieItemOwningActorBase();

	//~ AActor
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	//~ AActor

	//~ UFaerieItemDataProxy
	virtual const UFaerieItem* GetItemObject() const override;
	virtual int32 GetCopies() const override;
	virtual TScriptInterface<IFaerieItemOwnerInterface> GetItemOwner() const override;
	//~ UFaerieItemDataProxy

	//~ IFaerieItemOwnerInterface
	virtual FFaerieItemStack Release(FFaerieItemStackView Stack) override;
	virtual bool Possess(FFaerieItemStack Stack) override;
	//~ IFaerieItemOwnerInterface

protected:
	virtual void OnItemChanged(UFaerieEquipmentSlot* FaerieEquipmentSlot);

protected:
	// Stored stack is wrapped in an Equipment slot for ease of replication.
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "ItemOwningActor")
	TObjectPtr<UFaerieEquipmentSlot> ItemStack;
};
