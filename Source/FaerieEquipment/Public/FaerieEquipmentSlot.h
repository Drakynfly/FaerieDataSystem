// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieEquipmentSlotConfig.h"
#include "FaerieItemContainerBase.h"
#include "FaerieItemDataProxy.h"
#include "FaerieSlotTag.h"
#include "FlakesData.h"
#include "Actions/FaerieInventoryClient.h"
#include "FaerieEquipmentSlot.generated.h"

struct FFaerieAssetInfo;
class UFaerieEquipmentSlot;
class UFaerieEquipmentSlotDescription;
class UFaerieItem;

DECLARE_LOG_CATEGORY_EXTERN(LogFaerieEquipmentSlot, Log, All)

namespace Faerie::Equipment::Tags
{
	FAERIEEQUIPMENT_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, SlotSet)
	FAERIEEQUIPMENT_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, SlotTake)
}

USTRUCT()
struct FFaerieEquipmentSlotSaveData
{
	GENERATED_BODY()

	UPROPERTY()
	FFaerieEquipmentSlotConfig Config;

	UPROPERTY()
	FFlake ItemStack;

	UPROPERTY()
	FEntryKey StoredKey;
};


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
	virtual FFaerieContainerSaveData MakeSaveData() const override;
	virtual void LoadSaveData(const FFaerieContainerSaveData& SaveData) override;
	virtual bool IsValidKey(FEntryKey Key) const override;
	virtual void ForEachKey(const TFunctionRef<void(FEntryKey)>& Func) const override;

protected:
	virtual void OnItemMutated(const UFaerieItem* InItem, const UFaerieItemToken* Token, FGameplayTag EditTag) override;

private:
	virtual FFaerieItemStackView View(FEntryKey Key) const override;
	virtual FFaerieItemProxy Proxy(FEntryKey Key) const override;
	virtual FFaerieItemStack Release(FEntryKey Key, int32 Copies) override;
	virtual int32 GetStack(FEntryKey Key) const override;

public:
	FFaerieItemStackView View() const;
	FFaerieItemProxy Proxy() const;
	int32 GetStack() const;
	//~ UFaerieItemContainerBase

	//~ IFaerieItemDataProxy
	virtual const UFaerieItem* GetItemObject() const override;
	virtual int32 GetCopies() const override;
	virtual TScriptInterface<IFaerieItemOwnerInterface> GetOwner() const override;
	//~ IFaerieItemDataProxy

	//~ IFaerieItemOwnerInterface
	virtual FFaerieItemStack Release(FFaerieItemStackView Stack) override;
	virtual bool Possess(FFaerieItemStack Stack) override;
	//~ IFaerieItemOwnerInterface

	virtual bool CanClientRunActions(const UFaerieInventoryClient* Client);

protected:
	virtual void BroadcastChange();
	virtual void BroadcastDataChange();

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
	void SetItemInSlot(FFaerieItemStack Stack);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Faerie|EquipmentSlot")
	FFaerieItemStack TakeItemFromSlot(int32 Copies);

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