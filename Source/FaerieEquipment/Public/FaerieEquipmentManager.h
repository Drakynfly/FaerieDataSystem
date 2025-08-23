// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieContainerExtensionInterface.h"
#include "FaerieEquipmentSlotStructs.h"
#include "FaerieSlotTag.h"
#include "FaerieStoragePath.h"
#include "LoopUtils.h"
#include "Components/ActorComponent.h"
#include "StructUtils/InstancedStruct.h"

#include "FaerieEquipmentManager.generated.h"

class UFaerieEquipmentSlot;
class UFaerieEquipmentSlotDescription;
class UFaerieInventoryClient;
class UFaerieItemContainerBase;
class UItemContainerExtensionBase;
class UItemContainerExtensionGroup;

USTRUCT()
struct FFaerieEquipmentDefaultSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "FaerieEquipmentDefaultSlot")
	FFaerieEquipmentSlotConfig SlotConfig;

	// Predefined extensions added to this slot.
	UPROPERTY(EditAnywhere, Instanced, NoClear, Category = "FaerieEquipmentDefaultSlot")
	TObjectPtr<UItemContainerExtensionGroup> ExtensionGroup;
};

USTRUCT()
struct FFaerieEquipmentSaveData
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FFaerieEquipmentSlotSaveData> PerSlotData;

	UPROPERTY()
	TMap<FGuid, FInstancedStruct> ExtensionData;

	UPROPERTY()
	FGameplayTagContainer RemovedDefaultSlots;
};

UENUM(BlueprintType)
enum class EFaerieEquipmentSlotChangeType : uint8
{
	// This slot was added
	Addition,

	// This slot is being removed
	Removal,

	// The item in this slot has changed
	ItemChange,

	// A token from an item in this slot was edited
	TokenEdit
};

namespace Faerie
{
	using FEquipmentSlotEvent = TMulticastDelegate<void(UFaerieEquipmentSlot*, EFaerieEquipmentSlotChangeType)>;
}
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEquipmentChangedEvent, UFaerieEquipmentSlot*, Slot, EFaerieEquipmentSlotChangeType, Type);

/*
 * An actor component that manages an array of Equipment Slots, which can each store a single item entry.
 * A group of extensions is shared with all slots. Extension Interface calls on this object only apply to extension shared
 * between all slots. Slot-specific extensions must be requested from the slot itself.
 */
UCLASS(Blueprintable, ClassGroup = ("Faerie"), meta = (BlueprintSpawnableComponent),
	HideCategories = (Collision, ComponentTick, Replication, ComponentReplication, Activation, Sockets, Navigation))
class FAERIEEQUIPMENT_API UFaerieEquipmentManager : public UActorComponent, public IFaerieContainerExtensionInterface
{
	GENERATED_BODY()

	friend UFaerieEquipmentSlot;

public:
	UFaerieEquipmentManager();

	//~ UObject
	virtual void PostInitProperties() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ UObject

	//~ UActorComponent
	virtual void InitializeComponent() override;
	virtual void OnComponentCreated() override;
	virtual void ReadyForReplication() override;
	//~ UActorComponent

	//~ IFaerieContainerExtensionInterface
	virtual UItemContainerExtensionGroup* GetExtensionGroup() const override final;
	virtual bool AddExtension(UItemContainerExtensionBase* Extension) override;
	virtual bool RemoveExtension(UItemContainerExtensionBase* Extension) override;
	//~ IFaerieContainerExtensionInterface

	// Can the client request to run arbitrary actions on this equipment manager.
	virtual bool CanClientRunActions(const UFaerieInventoryClient* Client) const;

private:
	void AddDefaultSlots();
	void AddSubobjectsForReplication();

protected:
	void OnSlotItemChanged(UFaerieEquipmentSlot* Slot, bool TokenEdit);

public:
	/**------------------------------*/
	/*		 SAVE DATA API			 */
	/**------------------------------*/

	virtual FFaerieEquipmentSaveData MakeSaveData() const;
	virtual void LoadSaveData(const FFaerieEquipmentSaveData& SaveData);


	/**------------------------------*/
	/*			SLOTS API			 */
	/**------------------------------*/

	Faerie::FEquipmentSlotEvent::RegistrationType& GetOnEquipmentSlotEvent() { return OnEquipmentSlotEventNative; }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Faerie|EquipmentManager")
	UFaerieEquipmentSlot* AddSlot(const FFaerieEquipmentSlotConfig& Config);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Faerie|EquipmentManager")
	bool RemoveSlot(UFaerieEquipmentSlot* Slot);

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentManager")
	TArray<UFaerieEquipmentSlot*> GetSlots() const { return Slots; }

	/**
	 * Find a slot contained in this manager. Enable recursive to check slots contained in other slots.
	 */
	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentManager")
	UFaerieEquipmentSlot* FindSlot(FFaerieSlotTag SlotID, bool Recursive = false) const;


	/**------------------------------*/
	/*		 EXTENSIONS SYSTEM		 */
	/**------------------------------*/

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentManager")
	UItemContainerExtensionGroup* GetExtensions() const { return ExtensionGroup; }

	// Add a new extension of the given class, and return the result. If an extension of this class already exists, it
	// will be returned instead. Adds only to the slot at the ID
	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentManager", BlueprintAuthorityOnly, meta = (DeterminesOutputType = "ExtensionClass"))
	UItemContainerExtensionBase* AddExtensionToSlot(FFaerieSlotTag SlotID, TSubclassOf<UItemContainerExtensionBase> ExtensionClass);

	// Removes any existing extension(s) of the given class.
	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentManager", BlueprintAuthorityOnly)
	bool RemoveExtensionFromSlot(FFaerieSlotTag SlotID, TSubclassOf<UItemContainerExtensionBase> ExtensionClass);


	/**------------------------------*/
	/*		 EXTRA UTILITIES		 */
	/**------------------------------*/

	void ForEachContainer(Faerie::TBreakableLoop<UFaerieItemContainerBase*> Iter, bool Recursive);
	void ForEachSlot(Faerie::TBreakableLoop<UFaerieEquipmentSlot*> Iter, bool Recursive);

	// Gets all Slots and Storage objects for this manager and all contained items.
	// For only top-level containers, use GetSlots instead.
	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentManager")
	TArray<FFaerieStoragePath> GetAllContainerPaths() const;

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Faerie|EquipmentManager", meta = (DevelopmentOnly))
	void PrintSlotDebugInfo() const;

	/**------------------------------*/
	/*		C++ AND NATIVE EVENTS	 */
	/**------------------------------*/

protected:
	// A generic event when any slot is added, removed, or changed, either by adding or removing the item, or the item itself is changed.
	UPROPERTY(BlueprintAssignable, Transient, Category = "Events")
	FEquipmentChangedEvent OnEquipmentChangedEvent;

private:
	Faerie::FEquipmentSlotEvent OnEquipmentSlotEventNative;

protected:
	UE_DEPRECATED(5.5, "Use InstanceDefaultSlots instead")
	UPROPERTY(VisibleAnywhere, Category = "Equipment")
	TMap<FFaerieSlotTag, TObjectPtr<UFaerieEquipmentSlotDescription>> DefaultSlots;

	// Slots and their extensions to add to this equipment manager by default.
	UPROPERTY(EditAnywhere, Category = "Equipment")
	TArray<FFaerieEquipmentDefaultSlot> InstanceDefaultSlots;

	// Predefined extensions added to all slots in this manager.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, NoClear, Category = "Extensions")
	TObjectPtr<UItemContainerExtensionGroup> ExtensionGroup;

private:
	UPROPERTY(Replicated)
	TArray<TObjectPtr<UFaerieEquipmentSlot>> Slots;

	// Track if any default slots have been removed for serialization.
	FGameplayTagContainer RemovedDefaultSlots;
};