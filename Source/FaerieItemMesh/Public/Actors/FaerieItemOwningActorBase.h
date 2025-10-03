// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemOwnerInterface.h"
#include "FaerieItemVisualBase.h"
#include "FaerieItemOwningActorBase.generated.h"

struct FFaerieInventoryTag;
class IFaerieItemSource;
class UFaerieItemStackContainer;

/*
 * Base class for Actors that can own a stack of items, e.g., pick-ups on the ground.
 */
UCLASS(Abstract)
class FAERIEITEMMESH_API AFaerieItemOwningActorBase : public AFaerieItemVisualBase, public IFaerieItemOwnerInterface
{
	GENERATED_BODY()

public:
	AFaerieItemOwningActorBase();

#if WITH_EDITOR
protected:
	void InitStackFromConfig(bool RegenerateDisplay);

public:
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif

	//~ AActor
	virtual void PostLoad() override;
	virtual void OnConstruction(const FTransform& Transform) override;
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

	UFUNCTION(BlueprintCallable, Category = "ItemOwningActor")
	void SetOwnedStack(const FFaerieItemStack& Stack);

protected:
	virtual void OnItemChanged(UFaerieItemStackContainer* FaerieEquipmentSlot, FFaerieInventoryTag Event);

protected:
	// The item stack replication wrapper object.
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "ItemOwningActor")
	TObjectPtr<UFaerieItemStackContainer> ItemStack;

#if WITH_EDITORONLY_DATA
public:
	// If set, fill the ItemStack with an instance from this source.
	UPROPERTY(EditInstanceOnly, Category = "Stack Editor")
	TScriptInterface<const IFaerieItemSource> ItemSourceAsset;

	UPROPERTY(EditInstanceOnly, Category = "Stack Editor")
	int32 StackCopies = 1;

	UPROPERTY(EditInstanceOnly, Category = "Stack Editor")
	bool RegenerateDisplayOnConstruction = true;
#endif
};
