// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieInventoryTag.h"
#include "FaerieItem.h"
#include "FaerieItemContainerBase.h"
#include "FaerieItemProxyBase.h"
#include "TypedGameplayTags.h"
#include "FaerieItemStackContainer.generated.h"

class UFaerieItem;
class UFaerieItemStackContainer;

USTRUCT()
struct FFaerieSimpleItemStackSaveData
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<const UFaerieItem> ItemObject;

	UPROPERTY()
	int32 Copies = 0;

	// Additional data stored with this item instance.
	UPROPERTY()
	FFaerieItemExportData ExportData;
};

namespace Faerie::Inventory
{
	namespace Tags
	{
		FAERIEINVENTORY_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, SlotItemMutated)
		FAERIEINVENTORY_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, SlotClientReplication)
	}
}

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEquipmentSlotEvent, UFaerieItemStackContainer*, Slot, FFaerieInventoryTag, Event);

// A struct binding a FaerieInstance to a copies value for atomic replication.
USTRUCT()
struct FFaerieStackContainerContent
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "StackContainerContent")
	FFaerieItemInstance Instance;

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
	virtual FInstancedStruct MakeSaveData(FFaerieItemContainerExtensionData& ExtensionData) const override;
	virtual void LoadSaveData(FConstStructView ItemData, const TSharedStruct<FFaerieItemContainerExtensionData>& ExtensionData) override;
	virtual bool Contains(FFaerieAddress Address) const override;

private:
	// This block of functions are hidden from the API as they deal with owning multiple items. ItemStackContainer can
	// only hold a single stack so these only need to exist for interoperability with our parent class.

	// ReSharper disable CppOverrideWithDifferentVisibility
	virtual FFaerieItemInstance ViewInstance(FFaerieEntryKey Key) const override;
	virtual FFaerieItemInstance ViewInstance(FFaerieAddress Address) const override;
	virtual FFaerieItemDataView ViewEntry(FFaerieEntryKey Key) const override;
	virtual FFaerieItemDataView ViewAddress(FFaerieAddress Address) const override;
	virtual FFaerieItemProxy Proxy(FFaerieAddress Address) const override;
	virtual void DestroyStack(FFaerieEntryKey Key, int32 Copies) override;
	virtual void DestroyStack(FFaerieAddress Address, int32 Copies) override;
	virtual TOptional<FFaerieUnownedItemStack> Release(FFaerieEntryKey Key, int32 Copies) override;
	virtual TOptional<FFaerieUnownedItemStack> Release(FFaerieAddress Address, int32 Copies) override;
	virtual bool CanPossess(const FFaerieItemDataView& View) const override;
	virtual void GetAllAddresses(TArray<FFaerieAddress>& Addresses) const override;
	virtual TUniquePtr<Faerie::Container::IEntryIterator> CreateEntryIterator() const override;
	virtual TUniquePtr<Faerie::Container::IAddressIterator> CreateAddressIterator() const override;
	virtual TUniquePtr<Faerie::Container::IAddressIterator> CreateSingleEntryIterator(FFaerieEntryKey Key) const override;
	// ReSharper restore CppOverrideWithDifferentVisibility

	//~ UFaerieItemContainerBase

public:
	//~ IFaerieItemDataProxy
	virtual TOptional<FFaerieItemInstance> GetItemInstance() const override final;
	virtual int32 GetCopies() const override final;
	virtual IFaerieItemOwnerInterface* GetItemOwner() const override final;
	virtual Faerie::ItemData::FProxyChangeEvent::RegistrationType& GetOnProxyChangeEvent() override final { return OnItemChangedNative; }
	//~ IFaerieItemDataProxy

	//~ IFaerieContainerProxy
	virtual FFaerieAddress Proxy_GetAddress() const override;
	virtual FFaerieItemNetworkHandle Proxy_GetNetworkHandle() const override;
	//~ IFaerieContainerProxy

	//~ IFaerieItemOwnerInterface
	virtual bool Possess(const FFaerieUnownedItemStack& View) override;

protected:
	// ReSharper disable once CppOverrideWithDifferentVisibility
	virtual void DestroyStack(const FFaerieItemProxy& Proxy, int32 Copies = Faerie::ItemData::EntireStack) override;

public:
	virtual void OnItemDataChanged(const Faerie::ItemData::FMutableReference& Instance, TNotNull<const UScriptStruct*> Struct, FGameplayTag EditTag) override;
	//~ IFaerieItemOwnerInterface

	FFaerieAddress GetAddress() const;

	virtual void BroadcastChange(FFaerieInventoryTag Event);

	bool IsOurKey(FFaerieEntryKey Key) const;
	bool IsOurAddress(FFaerieAddress Address) const;

	void SetStoredItem_Impl(Faerie::ItemData::FValidatedDataView View);

public:
	Faerie::ItemData::FProxyChangeEvent::RegistrationType& GetOnContainerEvent() { return OnItemChangedNative; }

	FFaerieItemDataView GetView() const;

	// This checks if the stack could ever be contained by this container, ignoring its current state.
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemStackContainer")
	virtual bool CouldSetInSlot(const FFaerieItemDataView& View) const;

	// This checks if the stack can be set to this container. This is always called during SetItemInSlot, so do not feel the
	// need to always call this first, unless to preemptively check for User-facing purposes.
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemStackContainer")
	virtual bool CanSetInSlot(const FFaerieItemDataView& View) const;

	// Use to check beforehand if a removal will go through.
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemStackContainer")
	bool CanTakeFromSlot(int32 Copies, UPARAM(meta = (Categories = "Fae.Inventory.Removal")) FFaerieInventoryTag Reason) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemStackContainer")
	bool SetItemInSlot(const FFaerieUnownedItemStack& Stack);

#if WITH_EDITOR
	// Directly set the contained item from the editor
	void SetItemInSlot_Editor(const FFaerieUnownedItemStack& Stack);
#endif

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemStackContainer")
	FFaerieUnownedItemStack TakeItemFromSlot(int32 Copies, UPARAM(meta = (Categories = "Fae.Inventory.Removal")) FFaerieInventoryTag Reason);

#if WITH_EDITOR
	// Directly remove the contained item from the editor
	void ClearStackInSlot_Editor();
#endif

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

protected:
	UFUNCTION(/* Replication */)
	void OnRep_ItemStack();

	// Broadcast when the item filling this container is removed, a new item is set, or the item had its data mutated.
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FEquipmentSlotEvent OnItemChanged;

	Faerie::ItemData::FProxyChangeEvent OnItemChangedNative;

	// The current item stack being stored in this container.
	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing = "OnRep_ItemStack", Category = "State")
	FFaerieStackContainerContent ItemStack;

	// Incremented each time a new item is stored in this stack. Not changed when stack Copies is edited.
	UPROPERTY(Replicated)
	FFaerieEntryKey StoredKey;
};
