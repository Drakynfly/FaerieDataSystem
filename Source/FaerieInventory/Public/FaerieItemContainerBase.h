// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "ArrayAdapter.h"
#include "LoopUtils.h"
#include "FaerieFastArraySerializer.h"
#include "FaerieInventoryTag.h"
#include "NetSupportedObject.h"
#include "FaerieItemContainerStructs.h"
#include "FaerieItemDataView.h"
#include "FaerieItemExportData.h"
#include "FaerieItemProxy.h"
#include "FaerieUnownedItemStack.h"
#include "ItemContainerExtensionBase.h"

#include "Containers/AdderRef.h"

#include "MassReplication/FaerieItemReplicationTypes.h"

#include "StructUtils/InstancedStruct.h"
#include "StructUtils/StructView.h"

#include "FaerieItemContainerBase.generated.h"

namespace Faerie::Container
{
	struct FContainerEventPayload;
	class IEntryIterator;
	class IAddressIterator;
	class IAddressView;

	namespace Private
	{
		class FIteratorAccess;
	}

	// @todo refactor this into a traits flag
	USTRUCT()
	struct FNoExport
	{
		GENERATED_BODY()
	};

	// Data for nested containers that are owned by a live item instance.
	USTRUCT()
	struct FNestedContainer : public FNoExport
	{
		GENERATED_BODY()

		// Handle of the item that owns the container. ItemHandle is not replicated to prevent server handles from reaching the client.
		UPROPERTY(NotReplicated)
		FMassEntityHandle ItemHandle;
	};
}

struct FFaerieExtensionAllowsAdditionArgs;
struct FFaerieInventoryTag;

USTRUCT()
struct FFaerieItemContainerExtensionStorageElement : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "ItemContainerExtensionStorageElement")
	FInstancedStruct ExtensionStruct;
};

USTRUCT()
struct FAERIEINVENTORY_API FFaerieItemContainerExtensions : public FFaerieFastArraySerializer
{
	GENERATED_BODY()

	friend class UFaerieItemContainerBase;

private:
	UPROPERTY(EditAnywhere, Category = "ItemContainerExtensions")
	TArray<FFaerieItemContainerExtensionStorageElement> Items;

	UE_REWRITE TArray<FFaerieItemContainerExtensionStorageElement>& GetArray() { return Items; }

	/** Owning extension to send Fast Array callbacks to */
	// UPROPERTY() Fast Arrays cannot have additional properties with Iris
	// ReSharper disable once CppUE4ProbableMemoryIssuesWithUObject
	TObjectPtr<UFaerieItemContainerBase> ChangeListener;

	void PreStackReplicatedRemove(const FFaerieItemContainerExtensionStorageElement& Element) const;
	void PostStackReplicatedAdd(const FFaerieItemContainerExtensionStorageElement& Element) const;
	void PostStackReplicatedChange(const FFaerieItemContainerExtensionStorageElement& Element) const;

	FFaerieItemContainerExtensions* GetParentExtensions() const;

public:
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FastArrayDeltaSerialize<FFaerieItemContainerExtensionStorageElement, FFaerieItemContainerExtensions>(Items, DeltaParms, *this);
	}

	FConstStructView Find(TNotNull<const UScriptStruct*> Type, bool RecurseParents) const;
	FStructView Find(TNotNull<const UScriptStruct*> Type, bool RecurseParents);

	enum EWriteContainerDataFlag
	{
		None,
		Created
	};
	FInstancedStruct& AddOrGetRef(FMassEntityManager& EntityManager, TNotNull<const UScriptStruct*> Type, EWriteContainerDataFlag* OutFlag = nullptr);

	void Remove(int32 Index);
	void Reset();

	void ForEach(const TFunctionRef<Faerie::Utils::EIteratorFunctorReturn(FConstStructView)>& Functor) const;
	void ForEachMutable(const TFunctionRef<Faerie::Utils::EIteratorFunctorReturn(FStructView)>& Functor);

	void ForEach_Recursive(const TFunctionRef<Faerie::Utils::EIteratorFunctorReturn(FConstStructView)>& Functor, bool& RecursingParents) const;
	void ForEachMutable_Recursive(const TFunctionRef<Faerie::Utils::EIteratorFunctorReturn(FStructView)>& Functor, bool& RecursingParents);
};

UINTERFACE()
class UFaerieTempInterfaceForGettingParentExtensions : public UInterface
{
	GENERATED_BODY()
};

class IFaerieTempInterfaceForGettingParentExtensions
{
	GENERATED_BODY()

public:
	virtual FFaerieItemContainerExtensions* GetExtensions() = 0;
};

/**
 * The base class for objects that store and replicate FaerieItems.
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class FAERIEINVENTORY_API UFaerieItemContainerBase : public UNetSupportedObject, public IFaerieTempInterfaceForGettingParentExtensions
{
	GENERATED_BODY()

	friend Faerie::Container::Private::FIteratorAccess;
	friend FFaerieItemContainerExtensions;
	friend class UFaerieItemOwnerNotifier;
	friend class AFaerieMassReplicationActor;

public:
	UFaerieItemContainerBase();

	//~ UObject
	virtual void PostInitProperties() override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	//~ UObject

	//~ IFaerieTempInterfaceForGettingParentExtensions
	virtual FFaerieItemContainerExtensions* GetExtensions() override { return &ExtensionData; }
	//~ IFaerieTempInterfaceForGettingParentExtensions


	/**------------------------------*/
	/*		LIFETIME API			 */
	/**------------------------------*/
public:
	virtual void InitializeContainer(FMassEntityManager& EntityManager);
	virtual void DeinitializeContainer(FMassEntityManager& EntityManager) {}


	/**------------------------------*/
	/*		 SAVE DATA API			 */
	/**------------------------------*/

	[[nodiscard]] FFaerieItemExportData ExportItemData(const FMassEntityManager& EntityManager, const FMassEntityHandle Item) const;
	[[nodiscard]] FMassEntityHandle ImportItemData(FMassEntityManager& EntityManager, const FFaerieItemExportData& ExportData);

	virtual FInstancedStruct MakeSaveData(const FMassEntityManager& EntityManager, Faerie::Container::FSaveParams Params) const PURE_VIRTUAL(UFaerieItemContainerBase::MakeSaveData, return {}; )
	virtual bool LoadSaveData(FMassEntityManager& EntityManager, FConstStructView ItemData, Faerie::Container::FLoadParams Params) PURE_VIRTUAL(UFaerieItemContainerBase::SaveData, return false; )

protected:
	void RavelExtensionData(const FMassEntityManager& EntityManager, TAdderRef<FInstancedStruct> SaveData) const;
	void UnravelExtensionData(FMassEntityManager& EntityManager, TConstArrayView<FInstancedStruct> SaveData);


	/**------------------------------*/
	/*		 ITEM DATA API			 */
	/**------------------------------*/
public:
	// Is this a valid address in this container?
	virtual bool Contains(FFaerieAddress Address) const PURE_VIRTUAL(UFaerieItemContainerBase::Contains, return false; )

	virtual FMassEntityHandle GetItemHandle(FFaerieEntryKey Key) const PURE_VIRTUAL(UFaerieItemContainerBase::GetItemHandle, return FMassEntityHandle(); )
	virtual FMassEntityHandle GetItemHandle(FFaerieAddress Key) const PURE_VIRTUAL(UFaerieItemContainerBase::GetItemHandle, return FMassEntityHandle(); )

	// Get a view of a stack
	[[nodiscard]] virtual Faerie::ItemData::FScopeProxy ViewEntry(FFaerieEntryKey Key) const PURE_VIRTUAL(UFaerieItemContainerBase::ViewEntry, return nullptr; )
	[[nodiscard]] virtual Faerie::ItemData::FScopeProxy ViewAddress(FFaerieAddress Address) const PURE_VIRTUAL(UFaerieItemContainerBase::ViewAddress, return nullptr; )

	// Creates or retrieves a proxy for an entry
	[[nodiscard]] virtual FFaerieItemProxy Proxy(const FMassEntityManager& EntityManager, FMassEntityHandle Item) const PURE_VIRTUAL(UFaerieItemContainerBase::Proxy, return FFaerieItemProxy(); )
	[[nodiscard]] virtual FFaerieItemProxy Proxy(const FMassEntityManager& EntityManager, FFaerieEntryKey Key) const PURE_VIRTUAL(UFaerieItemContainerBase::Proxy, return FFaerieItemProxy(); )
	[[nodiscard]] virtual FFaerieItemProxy Proxy(const FMassEntityManager& EntityManager, FFaerieAddress Address) const PURE_VIRTUAL(UFaerieItemContainerBase::Proxy, return FFaerieItemProxy(); )

	// Call this function to grant ownership of an item stack. Returns true if ownership was accepted.
	// It is implied, and is the responsibility of the implementing class, to either accept ownership of the whole stack,
	// or none. Partial possession is not allowed.
	[[nodiscard]] virtual bool Possess(FMassEntityManager& EntityManager, const FFaerieUnownedItemStack& Stack) PURE_VIRTUAL(UFaerieItemContainerBase::Possess, return false; )

	[[nodiscard]] virtual TOptional<FFaerieUnownedItemStack> Release(FMassEntityManager& EntityManager, FFaerieEntryKey Key, int32 Copies, FFaerieInventoryTag Reason) PURE_VIRTUAL(UFaerieItemContainerBase::Release, return NullOpt; )

	[[nodiscard]] virtual TOptional<FFaerieUnownedItemStack> Release(FMassEntityManager& EntityManager, FFaerieAddress Address, int32 Copies, FFaerieInventoryTag Reason) PURE_VIRTUAL(UFaerieItemContainerBase::Release, return NullOpt; )

	[[nodiscard]] virtual TOptional<FFaerieUnownedItemStack> Release(FMassEntityManager& EntityManager, const FFaerieItemProxy& Proxy, int32 Copies, FFaerieInventoryTag Reason) PURE_VIRTUAL(UFaerieItemContainerBase::Release, return NullOpt; )

	virtual bool CanPossess(const FMassEntityManager& EntityManager, const FFaerieItemProxy& Proxy) const PURE_VIRTUAL(UFaerieItemContainerBase::CanPossess, return false; )

	virtual bool CanRelease(const FMassEntityManager& EntityManager, const FFaerieItemProxy& Proxy, FFaerieInventoryTag Reason) const PURE_VIRTUAL(UFaerieItemContainerBase::CanRelease, return false; )

	virtual void GetAllAddresses(TAdderReserverRef<FFaerieAddress> Addresses) const PURE_VIRTUAL(UFaerieItemContainerBase::GetAllAddresses, ; )

protected:
	virtual bool AckItemCreatedByNetwork(FMassEntityManager& EntityManager, FMassEntityHandle Entity, FFaerieNetItemID NetID) PURE_VIRTUAL(UFaerieItemContainerBase::AckItemCreatedByNetwork, return false; )

	virtual void OnItemDataChanged(FMassEntityManager& EntityManager, const FMassEntityHandle Item, FGameplayTag EditTag);

	// Create an iterator for the entries in this container.
	virtual TUniquePtr<Faerie::Container::IEntryIterator> CreateEntryIterator() const;

	// Create an iterator for the addresses of each entry in this container.
	virtual TUniquePtr<Faerie::Container::IAddressIterator> CreateAddressIterator() const;

	// Create an iterator for the addresses of a single entry in this container.
	virtual TUniquePtr<Faerie::Container::IAddressIterator> CreateSingleEntryIterator(FFaerieEntryKey Key) const;


	/**------------------------------*/
	/*		 EXTENSIONS API			 */
	/**------------------------------*/
public:
	// @todo temp raw accessor
	UE_REWRITE FFaerieItemContainerExtensions& GetExtensionData() { return ExtensionData; }

	void WriteContainerData(FMassEntityManager& EntityManager, TNotNull<const UScriptStruct*> Type, const TFunctionRef<void(FStructView, FFaerieItemContainerExtensions::EWriteContainerDataFlag)>& Functor, bool CreateIfMissing);
	void WriteContainerData(FMassEntityManager& EntityManager, TNotNull<const UScriptStruct*> Type, const TFunctionRef<void(FStructView)>& Functor, bool CreateIfMissing);

	bool HasContainerData(const FMassEntityManager& EntityManager, TNotNull<const UScriptStruct*> Type, bool RecurseParents = false) const;

	template <typename T>
	bool HasContainerData(const FMassEntityManager& EntityManager, bool RecurseParents = false) const
	{
		return HasContainerData(EntityManager, T::StaticStruct(), RecurseParents);
	}

	FConstStructView ReadContainerData(const FMassEntityManager& EntityManager, TNotNull<const UScriptStruct*> Type, bool RecurseParents = false) const;

	template <typename T>
	const T* ReadContainerData(const FMassEntityManager& EntityManager, bool RecurseParents = false) const
	{
		return ReadContainerData(EntityManager, T::StaticStruct(), RecurseParents).template GetPtr<T>();
	}

	template <typename T>
	const T& ReadContainerDataChecked(const FMassEntityManager& EntityManager, bool RecurseParents = false) const
	{
		return ReadContainerData(EntityManager, T::StaticStruct(), RecurseParents).template Get<T>();
	}

	void SetParentExtensions(TNotNull<IFaerieTempInterfaceForGettingParentExtensions*> Obj);
	void ClearParentExtensions();

	void InitializeExtensions(FMassEntityManager& EntityManager);

	[[nodiscard]] bool AllowsAddition(const FMassEntityManager& EntityManager, const Faerie::Utils::TArrayAdapter<FFaerieItemProxy>& Proxies, const FFaerieExtensionAllowsAdditionArgs Args, const bool DefaultResult) const;

	[[nodiscard]] bool AllowsRemoval(const FMassEntityManager& EntityManager, const TNotNull<const Faerie::Container::IAddressView*> DataView, const FFaerieInventoryTag Reason, const bool DefaultResult) const;

	[[nodiscard]] bool AllowsEdit(const FMassEntityManager& EntityManager, const TNotNull<const Faerie::Container::IAddressView*> DataView, const FFaerieInventoryTag EditTag, const bool DefaultResult) const;

	void PostEvent(FMassEntityManager& EntityManager, const Faerie::Container::FContainerEventPayload& Event);
	void PostEventBatch(FMassEntityManager& EntityManager, TConstArrayView<Faerie::Container::FContainerEventPayload> Events);


	/**------------------------------*/
	/*		 BLUEPRINT API			 */
	/**------------------------------*/
protected:
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemContainer")
	bool CanPossess(const FFaerieItemProxy& Proxy) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemContainer")
	bool CanRelease(const FFaerieItemProxy& Proxy, FFaerieInventoryTag Reason) const;


	/**------------------------------*/
	/*			 VARIABLES			 */
	/**------------------------------*/

protected:
	// Storage for extension data that configures this container's behavior and responds to events.
	UPROPERTY(EditAnywhere, Replicated, Category = "ItemContainer")
	FFaerieItemContainerExtensions ExtensionData;

	// Temporary solution for replicating extension parent.
	UPROPERTY(Replicated, Transient)
	TObjectPtr<UObject> ParentThatImplementsExtensions;

	Faerie::Inventory::TKeyGen<FFaerieEntryKey> KeyGen;
};