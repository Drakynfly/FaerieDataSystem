// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieEquipmentSlotStructs.h"
#include "FaerieInventoryTag.h"
#include "FaerieItemContainerBase.h"
#include "FaerieSlotTag.h"
#include "Actions/FaerieInventoryClient.h"
#include "FaerieEquipmentSlot.generated.h"

struct FFaerieAssetInfo;
class UFaerieEquipmentSlot;
class UFaerieItem;

namespace Faerie::Equipment::Tags
{
	FAERIEEQUIPMENT_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, SlotSet)
	FAERIEEQUIPMENT_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, SlotTake)
}

using FEquipmentSlotEventNative = TMulticastDelegate<void(UFaerieEquipmentSlot*)>;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEquipmentSlotEvent, UFaerieEquipmentSlot*, Slot);

/**
 * An equipment slot that holds and replicates a single Stack of Items.
 */
UCLASS(BlueprintType)
class FAERIEEQUIPMENT_API UFaerieEquipmentSlot : public UFaerieItemContainerBase, public IFaerieItemDataProxy
{
	GENERATED_BODY()

	// We friend the only classes allowed to set our Config
	friend class UFaerieEquipmentManager;
	friend class UFaerieChildSlotToken;

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

public:
	virtual TUniquePtr<Faerie::IContainerIterator> CreateIterator() const override;
	virtual TUniquePtr<Faerie::IContainerFilter> CreateFilter(bool FilterByAddresses) const override;

public:
	FFaerieItemStackView View() const;
	FFaerieItemProxy Proxy() const;
	int32 GetStack() const;

protected:
	FFaerieEquipmentSlotSaveData MakeSlotData(TMap<FGuid, FInstancedStruct>& ExtensionData) const;
	void LoadSlotData(const FFaerieEquipmentSlotSaveData& SlotData, UFaerieItemContainerExtensionData* ExtensionData);
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
	virtual void BroadcastChange();
	virtual void BroadcastDataChange();

	void SetItemInSlot_Impl(const FFaerieItemStack& Stack);

public:
	FFaerieSlotTag GetSlotID() const { return Config.SlotID; }

	FEquipmentSlotEventNative::RegistrationType& GetOnItemChanged() { return OnItemChangedNative; }
	FEquipmentSlotEventNative::RegistrationType& GetOnItemDataChanged() { return OnItemDataChangedNative; }

	// This checks if the stack could ever be contained by this slot, ignoring its current state.
	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentSlot")
	bool CouldSetInSlot(FFaerieItemStackView View) const;

	// This checks if the stack can be set to this slot. This is always called during SetItemInSlot, so do not feel the
	// need to always call this first, unless to preemptively check for User-facing purposes.
	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentSlot")
	bool CanSetInSlot(FFaerieItemStackView View) const;

	// Use to check beforehand if a removal will go through.
	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentSlot")
	bool CanTakeFromSlot(int32 Copies) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Faerie|EquipmentSlot")
	bool SetItemInSlot(FFaerieItemStack Stack);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Faerie|EquipmentSlot")
	FFaerieItemStack TakeItemFromSlot(int32 Copies);

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentSlot")
	FEntryKey GetCurrentKey() const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentSlot")
	FFaerieAddress GetCurrentAddress() const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentSlot")
	FFaerieAssetInfo GetSlotInfo() const;

	// Is there currently an item in this slot?
	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentSlot")
	bool IsFilled() const;

	UFaerieEquipmentSlot* FindSlot(FFaerieSlotTag SlotTag, bool bRecursive) const;

protected:
	UFUNCTION(/* Replication */)
	void OnRep_Item();

	// Broadcast when the item filling this slot is removed, or a new item is set.
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FEquipmentSlotEvent OnItemChanged;

	// Broadcast when the Item filling this slot has its data mutated. Usually by a sub-item being added/removed.
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FEquipmentSlotEvent OnItemDataChanged;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Replicated, Category = "Config", meta = (ExposeOnSpawn = true))
	FFaerieEquipmentSlotConfig Config;

	// Current item stack being contained in this slot.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, ReplicatedUsing = "OnRep_Item", Category = "State")
	FFaerieItemStack ItemStack;

private:
	// Incremented each time a new item is stored in this stack. Not changed when stack Copies is edited.
	UPROPERTY(Replicated)
	FEntryKey StoredKey;

	FEquipmentSlotEventNative OnItemChangedNative;
	FEquipmentSlotEventNative OnItemDataChangedNative;
};