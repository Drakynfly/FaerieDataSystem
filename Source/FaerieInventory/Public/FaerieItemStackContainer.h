// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieInventoryTag.h"
#include "FaerieMassFragmentAccessors.h"
#include "FaerieItemContainerBase.h"
#include "FaerieItemExportData.h"
#include "FaerieItemProxyBase.h"
#include "TypedGameplayTags.h"
#include "FaerieItemStackContainer.generated.h"

class UFaerieItemStackContainer;

USTRUCT()
struct FFaerieSimpleItemStackSaveData : public FFaerieItemFragmentSaveData
{
	GENERATED_BODY()

	FAERIEINVENTORY_API virtual const UScriptStruct* GetFragmentType() const override;

	UPROPERTY()
	int32 Copies = 0;

	UPROPERTY()
	FFaerieItemExportData ExportData;

	UPROPERTY()
	TArray<FInstancedStruct> ExtensionData;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEquipmentSlotEvent, UFaerieItemStackContainer*, Slot, FFaerieInventoryTag, Event);

// A struct containing the minimum atomically replicated state of a stack container.
USTRUCT()
struct FFaerieStackContainerContent
{
	GENERATED_BODY()

	// Incremented each time a new item is stored in this stack. Not changed when stack Copies is edited.
	UPROPERTY(VisibleAnywhere, Category = "StackContainerContent")
	FFaerieEntryKey StoredKey;

	UPROPERTY(VisibleAnywhere, Category = "StackContainerContent")
	FFaerieNetItemID NetID;

	UPROPERTY(VisibleAnywhere, Category = "StackContainerContent")
	int32 Copies = 0;
};

/**
 * A basic item container that stores and replicates a single stack of an item.
 */
UCLASS()
class FAERIEINVENTORY_API UFaerieItemStackContainer : public UFaerieItemContainerBase, public IFaerieContainerProxy
{
	GENERATED_BODY()

public:
	//~ UObject
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ UObject

	//~ UFaerieItemContainerBase
	virtual FInstancedStruct MakeSaveData(const FMassEntityManager& EntityManager, Faerie::Container::FSaveParams Params) const override;
	virtual bool LoadSaveData(FMassEntityManager& EntityManager, FConstStructView ItemData, Faerie::Container::FLoadParams Params) override;
	virtual bool Contains(FFaerieAddress Address) const override;

private:
	// This block of functions is hidden from the API as they deal with owning multiple items. ItemStackContainer can
	// only hold a single stack so these only need to exist for interoperability with our parent class.

	// ReSharper disable CppOverrideWithDifferentVisibility
	virtual FMassEntityHandle GetItemHandle(FFaerieEntryKey Key) const override;
	virtual FMassEntityHandle GetItemHandle(FFaerieAddress Address) const override;
	virtual Faerie::ItemData::FScopeProxy ViewEntry(FFaerieEntryKey Key) const override;
	virtual Faerie::ItemData::FScopeProxy ViewAddress(FFaerieAddress Address) const override;
	virtual FFaerieItemProxy Proxy(const FMassEntityManager& EntityManager, FMassEntityHandle Item) const override;
	virtual FFaerieItemProxy Proxy(const FMassEntityManager& EntityManager, FFaerieEntryKey Key) const override;
	virtual FFaerieItemProxy Proxy(const FMassEntityManager& EntityManager, FFaerieAddress Address) const override;
	virtual bool Possess(FMassEntityManager& EntityManager, const FFaerieUnownedItemStack& Stack) override;
	virtual TOptional<FFaerieUnownedItemStack> Release(FMassEntityManager& EntityManager, FFaerieEntryKey Key, int32 Copies, FFaerieInventoryTag Reason) override;
	virtual TOptional<FFaerieUnownedItemStack> Release(FMassEntityManager& EntityManager, FFaerieAddress Address, int32 Copies, FFaerieInventoryTag Reason) override;
	virtual TOptional<FFaerieUnownedItemStack> Release(FMassEntityManager& EntityManager, const FFaerieItemProxy& Proxy, int32 Copies, FFaerieInventoryTag Reason) override;
	virtual bool CanPossess(const FMassEntityManager& EntityManager, const FFaerieItemProxy& Proxy) const override;
	virtual bool CanRelease(const FMassEntityManager& EntityManager, const FFaerieItemProxy& Proxy, FFaerieInventoryTag Reason) const override;
	virtual void GetAllAddresses(TAdderReserverRef<FFaerieAddress> Addresses) const override;
	virtual bool AckItemCreatedByNetwork(FMassEntityManager& EntityManager, FMassEntityHandle Entity, FFaerieNetItemID NetID) override;
	virtual void OnItemDataChanged(FMassEntityManager& EntityManager, const FMassEntityHandle Item, FGameplayTag EditTag) override;
	virtual TUniquePtr<Faerie::Container::IEntryIterator> CreateEntryIterator() const override;
	virtual TUniquePtr<Faerie::Container::IAddressIterator> CreateAddressIterator() const override;
	virtual TUniquePtr<Faerie::Container::IAddressIterator> CreateSingleEntryIterator(FFaerieEntryKey Key) const override;
	// ReSharper restore CppOverrideWithDifferentVisibility

	//~ UFaerieItemContainerBase

public:
	//~ Faerie::ItemData::IViewBase
	virtual FMassEntityHandle GetEntityHandle() const override final;
	virtual int32 GetCopies() const override final;
	//~ Faerie::ItemData::IViewBase

	//~ IFaerieContainerProxy
	virtual FFaerieAddress Proxy_GetAddress() const override;
	virtual FFaerieItemNetworkHandle Proxy_GetNetworkHandle() const override;
	//~ IFaerieContainerProxy

	void MakeSaveData(const FMassEntityManager& EntityManager, FFaerieSimpleItemStackSaveData& SaveData, Faerie::Container::FSaveParams Params) const;
	void LoadSaveData(FMassEntityManager& EntityManager, const FFaerieSimpleItemStackSaveData& SaveData, Faerie::Container::FLoadParams Params);

	FFaerieAddress GetAddress() const;

	void BroadcastChange(FMassEntityManager& EntityManager, FFaerieInventoryTag Event);

	bool IsOurItem(FMassEntityHandle Item) const;
	bool IsOurKey(FFaerieEntryKey Key) const;
	bool IsOurAddress(FFaerieAddress Address) const;

	void SetStoredItem_Impl(FMassEntityManager& EntityManager, const FFaerieUnownedItemStack& ValidStack);

public:
#if WITH_EDITOR
	// Lets the editor clear data out of persistent containers.
	void Editor_ClearData();
#endif

	Faerie::ItemData::FProxyChangeEvent::RegistrationType& GetOnContainerEvent() { return OnItemChangedNative; }

	Faerie::ItemData::FScopeProxy GetView() const;

	// This checks if the stack could ever be contained by this container, ignoring its current state.
	bool CouldSetInSlot(const FMassEntityManager& EntityManager, const FFaerieItemProxy& Proxy) const;

	// This checks if the stack can be set to this container. This is always called during SetItemInSlot, so do not feel the
	// need to always call this first, unless to preemptively check for User-facing purposes.
	bool CanSetInSlot(const FMassEntityManager& EntityManager, const FFaerieItemProxy& Proxy) const;

	// Use to check beforehand if a removal will go through.
	bool CanTakeFromSlot(const FMassEntityManager& EntityManager, int32 Copies, FFaerieInventoryTag Reason) const;

	bool SetItemInSlot(FMassEntityManager& EntityManager, const FFaerieUnownedItemStack& Stack);

	FFaerieUnownedItemStack TakeItemFromSlot(FMassEntityManager& EntityManager, int32 Copies, FFaerieInventoryTag Reason);

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemStackContainer")
	int32 GetStackCopies() const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemStackContainer")
	FFaerieEntryKey GetCurrentKey() const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemStackContainer")
	FFaerieAddress GetCurrentAddress() const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemStackContainer")
	FFaerieItemNetworkHandle GetNetworkHandle() const;

	// Is there currently an item in this container?
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemStackContainer")
	bool IsFilled() const;


	/**------------------------------*/
	/*		 BLUEPRINT API			 */
	/**------------------------------*/

protected:
	// This checks if the stack could ever be contained by this container, ignoring its current state.
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemStackContainer")
	bool CouldSetInSlot(const FFaerieItemProxy& Proxy) const;

	// This checks if the stack can be set to this container. This is always called during SetItemInSlot, so do not feel the
	// need to always call this first, unless to preemptively check for User-facing purposes.
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemStackContainer")
	bool CanSetInSlot(const FFaerieItemProxy& Proxy) const;

	// Use to check beforehand if a removal will go through.
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemStackContainer")
	bool CanTakeFromSlot(int32 Copies, UPARAM(meta = (Categories = "Fae.Inventory.Removal")) FFaerieInventoryTag Reason) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemStackContainer")
	bool SetItemInSlot(const FFaerieUnownedItemStack& Stack);

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemStackContainer")
	FFaerieUnownedItemStack TakeItemFromSlot(int32 Copies, UPARAM(meta = (Categories = "Fae.Inventory.Removal")) FFaerieInventoryTag Reason);

protected:
	UFUNCTION(/* Replication */)
	void OnRep_ItemStack(const FFaerieStackContainerContent& OldValue);

	// Broadcast when the item filling this container is removed, a new item is set, or the item had its data mutated.
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FEquipmentSlotEvent OnItemChanged;

	Faerie::ItemData::FProxyChangeEvent OnItemChangedNative;

	// The current item stack being stored in this container.
	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing = "OnRep_ItemStack", Category = "State")
	FFaerieStackContainerContent ItemStack;

	// Locally assigned item entity handle.
	UPROPERTY()
	FMassEntityHandle ItemEntity;
};