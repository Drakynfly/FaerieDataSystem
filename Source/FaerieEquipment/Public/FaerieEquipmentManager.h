// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieEquipmentSlotStructs.h"
#include "FaerieInventoryTag.h"
#include "FaerieItemContainerBase.h"
#include "FaerieSlotTag.h"
#include "FaerieItemContainerPath.h"
#include "FaerieItemStackContainer.h"
#include "FaerieSubObjectFilter.h"

#include "Components/ActorComponent.h"

#include "FaerieEquipmentManager.generated.h"

USTRUCT()
struct FFaerieEquipmentDefaultSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "EquipmentDefaultSlot")
	FFaerieEquipmentSlotConfig SlotConfig;
};

USTRUCT()
struct FFaerieEquipmentSaveData
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FFaerieSimpleItemStackSaveData> PerSlotData;

	// @Todo unused currently...
	UPROPERTY()
	FFaerieItemContainerExtensionSaveData ExtensionData;

	UPROPERTY()
	FGameplayTagContainer RemovedDefaultSlots;
};

namespace Faerie::Equipment
{
	namespace Tags
	{
		FAERIEEQUIPMENT_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, SlotCreated)
		FAERIEEQUIPMENT_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, SlotDeleted)
	}

	using FSlotEvent = TMulticastDelegate<void(TNotNull<UFaerieItemStackContainer*>, FFaerieInventoryTag)>;

	static inline const auto SlotFilter = SubObject::Filter().ByClass<UFaerieItemStackContainer>();
	static inline const auto RecursiveSlotFilter = SubObject::Filter().Recursive().ByClass<UFaerieItemStackContainer>();
}

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEquipmentChangedEvent, UFaerieItemStackContainer*, Slot, FFaerieInventoryTag, Event);

/*
 * An actor component that manages an array of Equipment Slots, which can each store a single item entry.
 * A group of extensions is shared with all slots. Extension Interface calls on this object only apply to extension shared
 * between all slots. Slot-specific extensions must be requested from the slot itself.
 */
UCLASS(Blueprintable, ClassGroup = ("Faerie"), meta = (BlueprintSpawnableComponent),
	HideCategories = (Collision, ComponentTick, Replication, ComponentReplication, Activation, Sockets, Navigation))
class FAERIEEQUIPMENT_API UFaerieEquipmentManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UFaerieEquipmentManager();

	//~ UObject
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ UObject

	//~ UActorComponent
	virtual void InitializeComponent() override;
	virtual void OnComponentCreated() override;
	virtual void ReadyForReplication() override;
	//~ UActorComponent

private:
	void AddDefaultSlots();
	void AddSubobjectsForReplication();

protected:
	void OnDataChangeEvent(const FFaerieItemProxy& Proxy, FGameplayTag Tag);
	void BroadcastSlotEvent(TNotNull<UFaerieItemStackContainer*> Slot, FFaerieInventoryTag Event);

public:
	/**------------------------------*/
	/*		 SAVE DATA API			 */
	/**------------------------------*/

	FFaerieEquipmentSaveData MakeSaveData(Faerie::Container::FSaveParams Params) const;
	void LoadSaveData(const FFaerieEquipmentSaveData& SaveData, Faerie::Container::FLoadParams Params);


	/**------------------------------*/
	/*			SLOTS API			 */
	/**------------------------------*/

	Faerie::Equipment::FSlotEvent::RegistrationType& GetOnEquipmentSlotEvent() { return OnEquipmentSlotEventNative; }

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentManager")
	UFaerieItemStackContainer* AddSlot(const FFaerieEquipmentSlotConfig& Config);

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentManager")
	bool RemoveSlot(UFaerieItemStackContainer* Slot);

	// Switches the content of two slots, as long as the content of each can fit in the other.
	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentManager")
	bool TrySwapSlots(UFaerieItemStackContainer* SlotA, UFaerieItemStackContainer* SlotB);

protected:
	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentManager", meta = (DisplayName = "Get Slots"))
	TArray<UFaerieItemStackContainer*> BP_GetSlots() const { return Slots; }

	static const UFaerieItemStackContainer* FindSlotImpl(const UFaerieItemStackContainer* ParentSlot, const FMassEntityManager& EntityManager, FFaerieSlotTag SlotTag, bool bRecursive);

public:
	TConstArrayView<UFaerieItemStackContainer*> GetSlots() const { return Slots; }

	/**
	 * Find a slot contained in this manager. Enable recursive to check slots contained in other slots.
	 */
	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentManager")
	const UFaerieItemStackContainer* FindSlot(FFaerieSlotTag SlotID, bool Recursive = false) const;
		  UFaerieItemStackContainer* FindSlot(FFaerieSlotTag SlotID, bool Recursive = false);


	/**------------------------------*/
	/*		 EXTENSIONS SYSTEM		 */
	/**------------------------------*/

	FFaerieItemContainerExtensions& GetExtensionData() { return ExtensionData; }

	void WriteContainerData(TNotNull<const UScriptStruct*> Type, const TFunctionRef<void(FStructView)>& Functor);


	/**------------------------------*/
	/*		 EXTRA UTILITIES		 */
	/**------------------------------*/

	// Gets all Slots and Storage objects for this manager and all contained items.
	// For only top-level containers, use GetSlots instead.
	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentManager")
	TArray<FFaerieItemContainerPath> GetAllContainerPaths() const;

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
	Faerie::Equipment::FSlotEvent OnEquipmentSlotEventNative;

protected:
	// Slots and their extensions to add to this equipment manager by default.
	UPROPERTY(EditAnywhere, Category = "Equipment")
	TArray<FFaerieEquipmentDefaultSlot> InstanceDefaultSlots;

	// @todo not correctly implemented as we are not a ContainerBase... but works for now
	UPROPERTY(EditAnywhere, Category = "Extensions")
	FFaerieItemContainerExtensions ExtensionData;

private:
	UPROPERTY(Replicated)
	TArray<TObjectPtr<UFaerieItemStackContainer>> Slots;

	// Track if any default slots have been removed for serialization.
	FGameplayTagContainer RemovedDefaultSlots;
};