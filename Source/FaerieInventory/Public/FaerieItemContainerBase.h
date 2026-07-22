// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "NetSupportedObject.h"
#include "FaerieItemContainerStructs.h"
#include "FaerieItemDataView.h"
#include "FaerieItemOwnerInterface.h"
#include "FaerieItemProxy.h"
#include "FaerieUnownedItemStack.h"

#include "StructUtils/InstancedStruct.h"
#include "StructUtils/StructView.h"
#include "FaerieItemContainerBase.generated.h"

namespace Faerie::Container
{
	/*
	 * Extend ViewBase to provide access to an EntryKey
	 */
	class IEntryView : public ItemData::IViewBase
	{
	public:
		virtual FFaerieEntryKey ResolveKey() const = 0;
	};

	/*
	 * Extend EntryView to provide access to an Address
	 */
	class IAddressView : public IEntryView
	{
	public:
		virtual FFaerieAddress ResolveAddress() const = 0;
	};

	class IEntryIterator;
	class IAddressIterator;

	namespace Private
	{
		class FIteratorAccess;
	}
}

class UItemContainerExtensionBase;
class UItemContainerExtensionGroup;

USTRUCT()
struct FFaerieItemContainerExtensionData
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<uint32, FInstancedStruct> Data;
};

/**
 * The base class for objects that store and replicate FaerieItems.
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class FAERIEINVENTORY_API UFaerieItemContainerBase : public UNetSupportedObject, public IFaerieItemOwnerInterface
{
	GENERATED_BODY()

	friend Faerie::Container::Private::FIteratorAccess;

public:
	UFaerieItemContainerBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//~ UNetSupportedObject
	virtual void InitializeNetObject(TNotNull<AActor*> Actor) override;
	virtual void DeinitializeNetObject(TNotNull<AActor*> Actor) override;
	//~ UNetSupportedObject

	//~ IFaerieItemOwnerInterface
	virtual void DestroyStack(const FFaerieItemProxy& Proxy, int32 Copies = Faerie::ItemData::EntireStack) override;

	virtual bool Possess(const FFaerieUnownedItemStack& Stack) override;

public:
	// Note: this should be protected, not public, but I don't have a workaround for this yet.
	// Override to add logic when an item mutates while owned by this container.
	virtual void OnItemDataChanged(const Faerie::ItemData::FMutableReference& Instance, TNotNull<const UScriptStruct*> Struct, FGameplayTag EditTag) override;
	//~ IFaerieItemOwnerInterface


	/**------------------------------*/
	/*		 SAVE DATA API			 */
	/**------------------------------*/
public:
	[[nodiscard]] FFaerieItemExportData ExportItemData(const Faerie::ItemData::FRequireEntityManager& EntityManager, const Faerie::ItemData::FReference& Item) const;
	[[nodiscard]] FFaerieItemInstance ImportItemData(const Faerie::ItemData::FRequireEntityManager& EntityManager, const UFaerieItem* Item, const FFaerieItemExportData& ExportData);

	virtual FInstancedStruct MakeSaveData(FFaerieItemContainerExtensionData& ExtensionData) const PURE_VIRTUAL(UFaerieItemContainerBase::MakeSaveData, return {}; )
	virtual void LoadSaveData(FConstStructView ItemData, const TSharedStruct<FFaerieItemContainerExtensionData>& ExtensionData) PURE_VIRTUAL(UFaerieItemContainerBase::SaveData, )

protected:
	void RavelExtensionData(FFaerieItemContainerExtensionData& ExtensionData) const;
	void UnravelExtensionData(const TSharedStruct<FFaerieItemContainerExtensionData>& ExtensionData);


	/**------------------------------*/
	/*		 ITEM DATA API			 */
	/**------------------------------*/
public:
	// Is this a valid address in this container?
	virtual bool Contains(FFaerieAddress Address) const PURE_VIRTUAL(UFaerieItemContainerBase::Contains, return false; )

	// Get the instance data of an item.
	virtual FFaerieItemInstance ViewInstance(FFaerieEntryKey Key) const PURE_VIRTUAL(UFaerieItemContainerBase::ViewInstance, return FFaerieItemInstance(); )
	virtual FFaerieItemInstance ViewInstance(FFaerieAddress Address) const PURE_VIRTUAL(UFaerieItemContainerBase::ViewInstance, return FFaerieItemInstance(); )

	// Get a view of a stack
	virtual FFaerieItemDataView ViewEntry(FFaerieEntryKey Key) const PURE_VIRTUAL(UFaerieItemContainerBase::ViewEntry, return FFaerieItemDataView(); )
	virtual FFaerieItemDataView ViewAddress(FFaerieAddress Address) const PURE_VIRTUAL(UFaerieItemContainerBase::ViewAddress, return FFaerieItemDataView(); )

	// Creates or retrieves a proxy for an entry
	[[nodiscard]] virtual FFaerieItemProxy Proxy(FFaerieAddress Address) const PURE_VIRTUAL(UFaerieItemContainerBase::Proxy, return FFaerieItemProxy(); )

	virtual void DestroyStack(FFaerieEntryKey Key, int32 Copies) PURE_VIRTUAL(UFaerieItemContainerBase::DestroyStack, ; )

	virtual void DestroyStack(FFaerieAddress Address, int32 Copies) PURE_VIRTUAL(UFaerieItemContainerBase::DestroyStack, ; )

	[[nodiscard]] virtual TOptional<FFaerieUnownedItemStack> Release(FFaerieEntryKey Key, int32 Copies) PURE_VIRTUAL(UFaerieItemContainerBase::Release, return NullOpt; )

	[[nodiscard]] virtual TOptional<FFaerieUnownedItemStack> Release(FFaerieAddress Address, int32 Copies) PURE_VIRTUAL(UFaerieItemContainerBase::Release, return NullOpt; )

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemContainer")
	virtual bool CanPossess(const FFaerieItemDataView& DataView) const PURE_VIRTUAL(UFaerieItemContainerBase::CanPossess, return false; )

	virtual void GetAllAddresses(TArray<FFaerieAddress>& Addresses) const PURE_VIRTUAL(UFaerieItemContainerBase::GetAllAddresses, ; )


	/**------------------------------*/
	/*		 EXTENSIONS API			 */
	/**------------------------------*/

	UE_REWRITE UItemContainerExtensionGroup* GetExtensions() const { return Extensions; }

	/*
	 * Get an extension of a certain class.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Faerie|Extensions", meta = (DeterminesOutputType = "ExtensionClass", DynamicOutputParam = "Extension", ExpandBoolAsExecs = "ReturnValue"))
	bool FindExtension(TSubclassOf<UItemContainerExtensionBase> ExtensionClass, UItemContainerExtensionBase*& Extension, bool RecursiveSearch = true) const;

protected:
	// Iterators

	// Create an iterator for the entries in this container.
	virtual TUniquePtr<Faerie::Container::IEntryIterator> CreateEntryIterator() const;

	// Create an iterator for the addresses of each entry in this container.
	virtual TUniquePtr<Faerie::Container::IAddressIterator> CreateAddressIterator() const;

	// Create an iterator for the addresses of a single entry in this container.
	virtual TUniquePtr<Faerie::Container::IAddressIterator> CreateSingleEntryIterator(FFaerieEntryKey Key) const;


	/**------------------------------*/
	/*			 VARIABLES			 */
	/**------------------------------*/

protected:
	// Subobject responsible for adding to or customizing container behavior.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Instanced, Replicated, NoClear, Category = "ItemContainer")
	TObjectPtr<UItemContainerExtensionGroup> Extensions;

	Faerie::Inventory::TKeyGen<FFaerieEntryKey> KeyGen;
};