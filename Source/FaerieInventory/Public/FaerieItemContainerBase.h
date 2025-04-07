// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "NetSupportedObject.h"
#include "FaerieContainerExtensionInterface.h"
#include "FaerieItemOwnerInterface.h"
#include "InventoryDataStructs.h"
#include "FaerieItemContainerBase.generated.h"

class UItemContainerExtensionBase;

/**
 * Base class for objects that store FaerieItems
 */
UCLASS(Abstract)
class FAERIEINVENTORY_API UFaerieItemContainerBase : public UNetSupportedObject, public IFaerieItemOwnerInterface, public IFaerieContainerExtensionInterface
{
	GENERATED_BODY()

public:
	UFaerieItemContainerBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void InitializeNetObject(AActor* Actor) override;

	//~ IFaerieItemOwnerInterface
	virtual FFaerieItemStack Release(FFaerieItemStackView Stack) override;
	virtual bool Possess(FFaerieItemStack Stack) override;
	//~ IFaerieItemOwnerInterface

	//~ IFaerieContainerExtensionInterface
	virtual UItemContainerExtensionGroup* GetExtensionGroup() const override final;
	virtual bool AddExtension(UItemContainerExtensionBase* Extension) override;
	//~ IFaerieContainerExtensionInterface


	/**------------------------------*/
	/*		 SAVE DATA API			 */
	/**------------------------------*/
public:
	virtual FFaerieContainerSaveData MakeSaveData() const PURE_VIRTUAL(UFaerieItemContainerBase::MakeSaveData, return {}; )
	virtual void LoadSaveData(const FFaerieContainerSaveData& SaveData) PURE_VIRTUAL(UFaerieItemContainerBase::SaveData, )

protected:
	void RavelExtensionData(TMap<FGuid, FInstancedStruct>& Data) const;
	void UnravelExtensionData(const TMap<FGuid, FInstancedStruct>& Data);

	void TryApplyUnclaimedSaveData(UItemContainerExtensionBase* Extension);


	/**------------------------------*/
	/*		 ITEM ENTRY API			 */
	/**------------------------------*/
public:
	virtual bool IsValidKey(FEntryKey Key) const PURE_VIRTUAL(UFaerieItemContainerBase::IsValidKey, return false; )

	// Get a view of an entry
	virtual FFaerieItemStackView View(FEntryKey Key) const PURE_VIRTUAL(UFaerieItemContainerBase::View, return FFaerieItemStackView(); )

	// Creates or retrieves a proxy for an entry
	virtual FFaerieItemProxy Proxy(FEntryKey Key) const PURE_VIRTUAL(UFaerieItemContainerBase::Proxy, return nullptr; )

	// Iterate over and perform a task for each key.
	virtual void ForEachKey(const TFunctionRef<void(FEntryKey)>& Func) const PURE_VIRTUAL(UFaerieItemContainerBase::ForEachKey, ; )

	// Get the stack for a key.
	virtual int32 GetStack(FEntryKey Key) const PURE_VIRTUAL(UFaerieItemContainerBase::GetStack, return 0; )

protected:
	virtual void OnItemMutated(const UFaerieItem* Item, const UFaerieItemToken* Token, FGameplayTag EditTag);

	// This function must be called by child classes when binding items to new keys.
	void ReleaseOwnership(UFaerieItem* Item);

	// This function must be called by child classes when releasing a key.
	void TakeOwnership(UFaerieItem* Item);


	/**------------------------------*/
	/*			 VARIABLES			 */
	/**------------------------------*/

protected:
	// Subobject responsible for adding to or customizing container behavior.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
	TObjectPtr<class UItemContainerExtensionGroup> Extensions;

	// Save data for extensions that did not exist on us during unraveling.
	UPROPERTY(Transient)
	TMap<FGuid, FInstancedStruct> UnclaimedExtensionData;

	Faerie::TKeyGen<FEntryKey> KeyGen;
};