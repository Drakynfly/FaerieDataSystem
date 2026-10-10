// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemVisualBase.h"

#include "Extensions/ContainerEventSubscription.h"

#include "FaerieItemOwningActorBase.generated.h"

struct FFaerieInventoryTag;
class UFaerieItemAsset;
class UFaerieItemStackContainer;

/*
 * Base class for Actors that can own a stack of items, e.g., pick-ups on the ground.
 */
UCLASS(Abstract)
class FAERIEITEMMESH_API AFaerieItemOwningActorBase : public AFaerieItemVisualBase, public IFaerieContainerEventSubscriber
{
	GENERATED_BODY()

	friend class UActorFactoryFaerieItem;

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
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ AActor

public:
	//~ Faerie::ItemData::IViewBase
	virtual FMassEntityHandle GetEntityHandle() const override;
	virtual TOptional<FFaerieItemInstance> GetItemInstance() const override;
	virtual int32 GetCopies() const override;
	virtual const UObject* GetItemOwner() const override;
	//~ Faerie::ItemData::IViewBase

protected:
	//~ IFaerieContainerEventSubscriber
	virtual void OnContainerEventBatch(TNotNull<UFaerieItemContainerBase*> Container, TConstArrayView<const Faerie::Container::FContainerEventPayload*> Events) override;
	//~ IFaerieContainerEventSubscriber

public:
	UFaerieItemStackContainer* GetContainer() const { return ItemStack; }

	void SetOwnedStack(FMassEntityManager& EntityManager, const FFaerieUnownedItemStack& Stack);

protected:
	UFUNCTION(BlueprintCallable, Category = "ItemOwningActor")
	void SetOwnedStack(const FFaerieUnownedItemStack& Stack);

#if WITH_EDITOR
	UFUNCTION(Category = "Stack Editor", meta = (CallInEditor = "true"))
	void ViewItemObject();

	UFUNCTION(Category = "Stack Editor", meta = (CallInEditor = "true"))
	void RegenerateStack();
#endif

protected:
	// The item stack replication wrapper object.
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "ItemOwningActor")
	TObjectPtr<UFaerieItemStackContainer> ItemStack;

#if WITH_EDITORONLY_DATA
private:
	// If set, fill the ItemStack with an instance from this source.
	UPROPERTY()
	TScriptInterface<const class IFaerieItemSource> ItemSourceAsset;

public:
	UPROPERTY(EditInstanceOnly, Category = "Stack Editor", meta = (NoResetToDefault))
	TObjectPtr<const UFaerieItemAsset> SourceAsset;

	UPROPERTY(EditInstanceOnly, Category = "Stack Editor")
	int32 StackCopies = 1;

	UPROPERTY(EditInstanceOnly, Category = "Stack Editor")
	bool RegenerateDisplayOnConstruction = true;
#endif
};
