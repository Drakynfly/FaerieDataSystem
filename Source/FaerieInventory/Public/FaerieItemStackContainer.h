﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemContainerBase.h"
#include "FaerieItemStackContainer.generated.h"

class UFaerieItem;
class UFaerieItemStackContainer;

namespace Faerie
{
	using FItemStackContainerEvent = TMulticastDelegate<void(UFaerieItemStackContainer*, FFaerieInventoryTag)>;

	namespace Inventory::Tags
	{
		FAERIEINVENTORY_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, SlotSet)
		FAERIEINVENTORY_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, SlotTake)
		FAERIEINVENTORY_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, SlotItemMutated)
		FAERIEINVENTORY_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, SlotClientReplication)
	}
}

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEquipmentSlotEvent, UFaerieItemStackContainer*, Slot, FFaerieInventoryTag, Event);

/**
 * A basic item container that stores and replicates a single stack of an item.
 */
UCLASS()
class FAERIEINVENTORY_API UFaerieItemStackContainer : public UFaerieItemContainerBase, public IFaerieItemDataProxy
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//~ UFaerieItemContainerBase
	virtual FInstancedStruct MakeSaveData(TMap<FGuid, FInstancedStruct>& ExtensionData) const override;
	virtual void LoadSaveData(FConstStructView ItemData, UFaerieItemContainerExtensionData* ExtensionData) override;
	virtual bool Contains(FEntryKey Key) const override;

private:
	virtual FFaerieItemStackView View(FEntryKey Key) const override;
	virtual FFaerieItemStack Release(FEntryKey Key, int32 Copies) override;
	virtual int32 GetStack(FEntryKey Key) const override;

public:
	virtual bool Contains(FFaerieAddress Address) const override;

private:
	virtual int32 GetStack(FFaerieAddress Address) const override;
	virtual const UFaerieItem* ViewItem(FFaerieAddress Address) const override;
	virtual FFaerieItemStackView ViewStack(FFaerieAddress Address) const override;
	virtual FFaerieItemProxy Proxy(FFaerieAddress Address) const override;
	virtual FFaerieItemStack Release(FFaerieAddress Address, int32 Copies) override;
	virtual TUniquePtr<Faerie::Container::IIterator> CreateIterator(bool) const override;
	virtual TUniquePtr<Faerie::Container::IFilter> CreateFilter(bool) const override;

	virtual FEntryKey FILTER_GetBaseKey(FFaerieAddress Address) const override;
	virtual TArray<FFaerieAddress> FILTER_GetKeyAddresses(FEntryKey Key) const override;

public:
	FFaerieItemStackView View() const;
	FFaerieItemProxy Proxy() const;
	int32 GetStack() const;
	//~ UFaerieItemContainerBase

public:
	//~ IFaerieItemDataProxy
	virtual const UFaerieItem* GetItemObject() const override;
	virtual int32 GetCopies() const override;
	virtual TScriptInterface<IFaerieItemOwnerInterface> GetItemOwner() const override;
	//~ IFaerieItemDataProxy

	//~ IFaerieItemOwnerInterface
	virtual FFaerieItemStack Release(FFaerieItemStackView Stack) override;
	virtual bool Possess(FFaerieItemStack Stack) override;

protected:
	virtual void OnItemMutated(const UFaerieItem* Item, const UFaerieItemToken* Token, FGameplayTag EditTag) override;
	//~ IFaerieItemOwnerInterface

protected:
	virtual void BroadcastChange(FFaerieInventoryTag Event);

	void SetStoredItem_Impl(const FFaerieItemStack& Stack);

public:
	Faerie::FItemStackContainerEvent::RegistrationType& GetOnContainerEvent() { return OnItemChangedNative; }

	// This checks if the stack could ever be contained by this container, ignoring its current state.
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemStackContainer")
	virtual bool CouldSetInSlot(FFaerieItemStackView View) const;

	// This checks if the stack can be set to this container. This is always called during SetItemInSlot, so do not feel the
	// need to always call this first, unless to preemptively check for User-facing purposes.
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemStackContainer")
	virtual bool CanSetInSlot(FFaerieItemStackView View) const;

	// Use to check beforehand if a removal will go through.
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemStackContainer")
	bool CanTakeFromSlot(int32 Copies) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Faerie|ItemStackContainer")
	bool SetItemInSlot(FFaerieItemStack Stack);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Faerie|ItemStackContainer")
	FFaerieItemStack TakeItemFromSlot(int32 Copies);

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemStackContainer")
	FEntryKey GetCurrentKey() const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemStackContainer")
	FFaerieAddress GetCurrentAddress() const;

	// Is there currently an item in this container?
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemStackContainer")
	bool IsFilled() const;

protected:
	UFUNCTION(/* Replication */)
	void OnRep_ItemStack();

	// Broadcast when the item filling this container is removed, a new item is set, or the item had its data mutated.
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FEquipmentSlotEvent OnItemChanged;

	Faerie::FItemStackContainerEvent OnItemChangedNative;

	// The current item stack being stored in this container.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, ReplicatedUsing = "OnRep_ItemStack", Category = "State")
	FFaerieItemStack ItemStack;

	// Incremented each time a new item is stored in this stack. Not changed when stack Copies is edited.
	UPROPERTY(Replicated)
	FEntryKey StoredKey;
};
