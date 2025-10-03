// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "NetSupportedObject.h"
#include "FaerieContainerExtensionInterface.h"
#include "FaerieItemContainerStructs.h"
#include "FaerieItemOwnerInterface.h"
#include "FaerieItemProxy.h"
#include "ItemContainerEvent.h"
#include "StructUtils/StructView.h"
#include "FaerieItemContainerBase.generated.h"

namespace Faerie
{
	class IContainerIterator;
	class IContainerFilter;
}

class UFaerieItemContainerBase;
class UItemContainerExtensionBase;

UCLASS()
class FAERIEINVENTORY_API UFaerieItemContainerExtensionData : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TMap<FGuid, FInstancedStruct> Data;
};

/**
 * The base class for objects that store and replicate FaerieItems.
 */
UCLASS(Abstract, Blueprintable)
class FAERIEINVENTORY_API UFaerieItemContainerBase : public UNetSupportedObject, public IFaerieItemOwnerInterface, public IFaerieContainerExtensionInterface
{
	GENERATED_BODY()

public:
	UFaerieItemContainerBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//~ UNetSupportedObject
	virtual void InitializeNetObject(AActor* Actor) override;
	virtual void DeinitializeNetObject(AActor* Actor) override;
	//~ UNetSupportedObject

	//~ IFaerieItemOwnerInterface
	virtual FFaerieItemStack Release(FFaerieItemStackView Stack) override;
	virtual bool Possess(FFaerieItemStack Stack) override;

protected:
	virtual void OnItemMutated(const UFaerieItem* Item, const UFaerieItemToken* Token, FGameplayTag EditTag) override;
	//~ IFaerieItemOwnerInterface

public:
	//~ IFaerieContainerExtensionInterface
	virtual UItemContainerExtensionGroup* GetExtensionGroup() const override final;
	virtual bool AddExtension(UItemContainerExtensionBase* Extension) override;
	//~ IFaerieContainerExtensionInterface


	/**------------------------------*/
	/*		 SAVE DATA API			 */
	/**------------------------------*/
public:
	virtual FInstancedStruct MakeSaveData(TMap<FGuid, FInstancedStruct>& ExtensionData) const PURE_VIRTUAL(UFaerieItemContainerBase::MakeSaveData, return {}; )
	virtual void LoadSaveData(FConstStructView ItemData, UFaerieItemContainerExtensionData* ExtensionData) PURE_VIRTUAL(UFaerieItemContainerBase::SaveData, )

protected:
	void RavelExtensionData(TMap<FGuid, FInstancedStruct>& ExtensionData) const;
	void UnravelExtensionData(UFaerieItemContainerExtensionData* ExtensionData);

	void TryApplyUnclaimedSaveData(UItemContainerExtensionBase* Extension);


	/**------------------------------*/
	/*		 ITEM ENTRY API (OLD)	 */
	/**------------------------------*/
public:
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemContainer", DisplayName = "Contains (deprecated)")
	virtual bool Contains(FEntryKey Key) const PURE_VIRTUAL(UFaerieItemContainerBase::Contains, return false; )

	// Get a view of an entry
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemContainer", DisplayName = "View (deprecated)")
	virtual FFaerieItemStackView View(FEntryKey Key) const PURE_VIRTUAL(UFaerieItemContainerBase::View, return FFaerieItemStackView(); )

	// A more efficient overload of Release if we already know the Key.
	virtual FFaerieItemStack Release(FEntryKey Key, int32 Copies) PURE_VIRTUAL(UFaerieItemContainerBase::Release, return FFaerieItemStack(); )

	// Get the stack for a key.
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemContainer", DisplayName = "Get Stack (deprecated)")
	virtual int32 GetStack(FEntryKey Key) const PURE_VIRTUAL(UFaerieItemContainerBase::GetStack, return 0; )


	/**------------------------------*/
	/*		 ITEM ENTRY API (NEW)	 */
	/**------------------------------*/

	// Note: This is the new api. FFaerieAddress will eventually replace public usage of FEntryKey.

public:
	// Is this a valid address in this container?
	virtual bool Contains(FFaerieAddress Address) const PURE_VIRTUAL(UFaerieItemContainerBase::Contains, return false; )

	// Get the total number of copies keyed to an address.
	virtual int32 GetStack(FFaerieAddress Address) const PURE_VIRTUAL(UFaerieItemContainerBase::GetStack, return 0; )

	// Get a view of an item or stack
	virtual const UFaerieItem* ViewItem(FFaerieAddress Address) const PURE_VIRTUAL(UFaerieItemContainerBase::ViewItem, return nullptr; )
	virtual FFaerieItemStackView ViewStack(FFaerieAddress Address) const PURE_VIRTUAL(UFaerieItemContainerBase::ViewStack, return FFaerieItemStackView(); )

	// Creates or retrieves a proxy for an entry
	virtual FFaerieItemProxy Proxy(FFaerieAddress Address) const PURE_VIRTUAL(UFaerieItemContainerBase::Proxy, return FFaerieItemProxy(); )

	// A more efficient overload of Release if we already know the address.
	virtual FFaerieItemStack Release(FFaerieAddress Address, int32 Copies) PURE_VIRTUAL(UFaerieItemContainerBase::Release, return FFaerieItemStack(); )

	// Create an iterator for the specific implementation of this container.
	virtual TUniquePtr<Faerie::IContainerIterator> CreateIterator() const;
	virtual TUniquePtr<Faerie::IContainerFilter> CreateFilter(bool FilterByAddresses) const;

protected:
	// Blueprint versions (temp, until Old Blueprint versions are removed)
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemContainer", DisplayName = "Contains")
	bool Contains_Address(const FFaerieAddress Address) const { return Contains(Address); }

	// Get a view of an entry
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemContainer", DisplayName = "View")
	FFaerieItemStackView View_Address(const FFaerieAddress Address) const { return ViewStack(Address); }

	// Creates or retrieves a proxy for an entry
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemContainer", DisplayName = "Proxy")
	FFaerieItemProxy Proxy_Address(const FFaerieAddress Address) const { return Proxy(Address); }

	// Get the stack for a key.
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemContainer", DisplayName = "Get Stack")
	int32 GetStack_Address(const FFaerieAddress Address) const { return GetStack(Address); }


	/**------------------------------*/
	/*			 VARIABLES			 */
	/**------------------------------*/

protected:
	// Subobject responsible for adding to or customizing container behavior.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "ItemContainer")
	TObjectPtr<UItemContainerExtensionGroup> Extensions;

	// Save data for extensions that did not exist on us during unraveling.
	UPROPERTY(Transient)
	TObjectPtr<UFaerieItemContainerExtensionData> UnclaimedExtensionData;

	Faerie::TKeyGen<FEntryKey> KeyGen;
};