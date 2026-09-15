// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "ArrayAdapter.h"
#include "LoopUtils.h"
#include "FaerieFastArraySerializer.h"
#include "FaerieInventoryTag.h"
#include "NetSupportedObject.h"
#include "FaerieItemContainerStructs.h"
#include "FaerieItemDataView.h"
#include "FaerieItemOwnerInterface.h"
#include "FaerieItemProxy.h"
#include "FaerieUnownedItemStack.h"
#include "ItemContainerExtensionBase.h"

#include "Containers/AdderRef.h"
#include "StructUtils/InstancedStruct.h"
#include "StructUtils/StructView.h"

#include "FaerieItemContainerBase.generated.h"

namespace Faerie::Container
{
	struct FEvent;
	class IEntryIterator;
	class IAddressIterator;
	class IAddressView;

	namespace Private
	{
		class FIteratorAccess;
	}

	// Data for nested containers that are owned by a live item instance.
	USTRUCT()
	struct FNestedContainer
	{
		GENERATED_BODY()

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

	friend UFaerieItemContainerBase;

private:
	UPROPERTY(EditAnywhere, Category = "ItemContainerExtensions")
	TArray<FFaerieItemContainerExtensionStorageElement> Items;

	UE_REWRITE TArray<FFaerieItemContainerExtensionStorageElement>& GetArray() { return Items; }

	/** Owning extension to send Fast Array callbacks to */
	// UPROPERTY() Fast Arrays cannot have additional properties with Iris
	// ReSharper disable once CppUE4ProbableMemoryIssuesWithUObject
	TObjectPtr<UFaerieItemContainerBase> ChangeListener;

	// Dual pointer to UObject and it's Extension data that we can use to access parent extension data.
	TPair<FWeakObjectPtr, FFaerieItemContainerExtensions*> ParentExtensions;

	void PreStackReplicatedRemove(const FFaerieItemContainerExtensionStorageElement& Element) const;
	void PostStackReplicatedAdd(const FFaerieItemContainerExtensionStorageElement& Element) const;
	void PostStackReplicatedChange(const FFaerieItemContainerExtensionStorageElement& Element) const;

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
	FInstancedStruct& AddOrGetRef(TNotNull<const UScriptStruct*> Type, EWriteContainerDataFlag* OutFlag = nullptr);

	void Remove(int32 Index);
	void Reset();

	void ForEach(const TFunctionRef<Faerie::Utils::EIteratorFunctorReturn(FConstStructView)>& Functor) const;
	void ForEachMutable(const TFunctionRef<Faerie::Utils::EIteratorFunctorReturn(FStructView)>& Functor);

	void ForEach_Recursive(const TFunctionRef<Faerie::Utils::EIteratorFunctorReturn(FConstStructView)>& Functor) const;
	void ForEachMutable_Recursive(const TFunctionRef<Faerie::Utils::EIteratorFunctorReturn(FStructView)>& Functor);
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

	//~ UObject
	virtual void PostInitProperties() override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	//~ UObject

	//~ UNetSupportedObject
	virtual void InitializeNetObject(TNotNull<AActor*> Actor) override;
	//virtual void DeinitializeNetObject(TNotNull<AActor*> Actor) override;
	//~ UNetSupportedObject

	//~ IFaerieItemOwnerInterface
	virtual void OnItemDataChanged(const FFaerieItemInstance& Instance, FGameplayTag EditTag) override;
	//~ IFaerieItemOwnerInterface


	/**------------------------------*/
	/*		 SAVE DATA API			 */
	/**------------------------------*/
public:
	[[nodiscard]] FFaerieItemExportData ExportItemData(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item) const;
	[[nodiscard]] FFaerieItemInstance ImportItemData(FMassEntityManager& EntityManager, const UFaerieItem* Item, const FFaerieItemExportData& ExportData);

	virtual FInstancedStruct MakeSaveData(Faerie::Container::FSaveParams Params) const PURE_VIRTUAL(UFaerieItemContainerBase::MakeSaveData, return {}; )
	virtual void LoadSaveData(FConstStructView ItemData, Faerie::Container::FLoadParams Params) PURE_VIRTUAL(UFaerieItemContainerBase::SaveData, )

protected:
	void RavelExtensionData(TAdderRef<FInstancedStruct> SaveData) const;
	void UnravelExtensionData(TConstArrayView<FInstancedStruct> SaveData);


	/**------------------------------*/
	/*		 ITEM DATA API			 */
	/**------------------------------*/
public:
	// Is this a valid address in this container?
	virtual bool Contains(FFaerieAddress Address) const PURE_VIRTUAL(UFaerieItemContainerBase::Contains, return false; )

	// Get the instance data of an item.
	virtual TOptional<FFaerieItemInstance> ViewInstance(FFaerieEntryKey Key) const PURE_VIRTUAL(UFaerieItemContainerBase::ViewInstance, return NullOpt; )
	virtual TOptional<FFaerieItemInstance> ViewInstance(FFaerieAddress Address) const PURE_VIRTUAL(UFaerieItemContainerBase::ViewInstance, return NullOpt; )

	// Get a view of a stack
	[[nodiscard]] virtual Faerie::ItemData::FScopeProxy ViewEntry(FFaerieEntryKey Key) const PURE_VIRTUAL(UFaerieItemContainerBase::ViewEntry, return nullptr; )
	[[nodiscard]] virtual Faerie::ItemData::FScopeProxy ViewAddress(FFaerieAddress Address) const PURE_VIRTUAL(UFaerieItemContainerBase::ViewAddress, return nullptr; )

	// Creates or retrieves a proxy for an entry
	[[nodiscard]] virtual FFaerieItemProxy Proxy(FFaerieEntryKey Key) const PURE_VIRTUAL(UFaerieItemContainerBase::Proxy, return FFaerieItemProxy(); )
	[[nodiscard]] virtual FFaerieItemProxy Proxy(FFaerieAddress Address) const PURE_VIRTUAL(UFaerieItemContainerBase::Proxy, return FFaerieItemProxy(); )

	// Call this function to grant ownership of a FFaerieItemInstance stack. Returns true if ownership was accepted.
	// It is implied, and is the responsibility of the implementing class, to either accept ownership of the whole stack,
	// or none. Partial possession is not allowed.
	[[nodiscard]] virtual bool Possess(const FFaerieUnownedItemStack& Stack) PURE_VIRTUAL(UFaerieItemContainerBase::Possess, return false; )

	// Destroy a number of items associated with an entry key.
	virtual void DestroyStack(FFaerieEntryKey Key, int32 Copies = Faerie::ItemData::EntireStack) PURE_VIRTUAL(UFaerieItemContainerBase::DestroyStack, ; )

	// Destroy a number of items associated with an address.
	virtual void DestroyStack(FFaerieAddress Address, int32 Copies = Faerie::ItemData::EntireStack) PURE_VIRTUAL(UFaerieItemContainerBase::DestroyStack, ; )

	// Destroy a number of items associated with a proxy.
	virtual void DestroyStack(const FFaerieItemProxy& Proxy, int32 Copies = Faerie::ItemData::EntireStack) PURE_VIRTUAL(UFaerieItemContainerBase::DestroyStack, ; )

	[[nodiscard]] virtual TOptional<FFaerieUnownedItemStack> Release(FFaerieEntryKey Key, int32 Copies, FFaerieInventoryTag Reason) PURE_VIRTUAL(UFaerieItemContainerBase::Release, return NullOpt; )

	[[nodiscard]] virtual TOptional<FFaerieUnownedItemStack> Release(FFaerieAddress Address, int32 Copies, FFaerieInventoryTag Reason) PURE_VIRTUAL(UFaerieItemContainerBase::Release, return NullOpt; )

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemContainer")
	virtual bool CanPossess(const FFaerieItemProxy& Proxy) const PURE_VIRTUAL(UFaerieItemContainerBase::CanPossess, return false; )

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemContainer")
	virtual bool CanRelease(const FFaerieItemProxy& Proxy, FFaerieInventoryTag Reason) const PURE_VIRTUAL(UFaerieItemContainerBase::CanRelease, return false; )

	virtual void GetAllAddresses(TAdderReserverRef<FFaerieAddress> Addresses) const PURE_VIRTUAL(UFaerieItemContainerBase::GetAllAddresses, ; )

protected:
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

	void WriteContainerData(TNotNull<const UScriptStruct*> Type, const TFunctionRef<void(FStructView, FFaerieItemContainerExtensions::EWriteContainerDataFlag)>& Functor, bool CreateIfMissing = true);
	void WriteContainerData(TNotNull<const UScriptStruct*> Type, const TFunctionRef<void(FStructView)>& Functor, bool CreateIfMissing = true);

	bool HasContainerData(TNotNull<const UScriptStruct*> Type, bool RecurseParents = false) const;

	template <typename T>
	bool HasContainerData(bool RecurseParents = false) const
	{
		return HasContainerData(T::StaticStruct(), RecurseParents);
	}

	FConstStructView ReadContainerData(TNotNull<const UScriptStruct*> Type, bool RecurseParents = false) const;

	template <typename T>
	const T* ReadContainerData(bool RecurseParents = false) const
	{
		return ReadContainerData(T::StaticStruct(), RecurseParents).template GetPtr<T>();
	}

	template <typename T>
	const T& ReadContainerDataChecked(bool RecurseParents = false) const
	{
		return ReadContainerData(T::StaticStruct(), RecurseParents).template Get<T>();
	}

	void SetParentExtensions(TNotNull<UObject*> Obj, FFaerieItemContainerExtensions& InExtensions);
	void ClearParentExtensions();

	void InitializeExtensions();

	[[nodiscard]] bool AllowsAddition(const Faerie::Utils::TArrayAdapter<FFaerieItemProxy>& Proxies, const FFaerieExtensionAllowsAdditionArgs Args, const bool DefaultResult) const;

	[[nodiscard]] bool AllowsRemoval(const TNotNull<const Faerie::Container::IAddressView*> DataView, const FFaerieInventoryTag Reason, const bool DefaultResult) const;

	[[nodiscard]] bool AllowsEdit(const TNotNull<const Faerie::Container::IAddressView*> DataView, const FFaerieInventoryTag EditTag, const bool DefaultResult) const;

	void PostEvent(const Faerie::Container::FEvent& Event);

	void PostEventBatch(TConstArrayView<Faerie::Container::FEvent> Events);


	/**------------------------------*/
	/*			 VARIABLES			 */
	/**------------------------------*/

protected:
	// Storage for extension data that configures this container's behavior and responds to events.
	UPROPERTY(EditAnywhere, Replicated, Category = "ItemContainer")
	FFaerieItemContainerExtensions ExtensionData;

	Faerie::Inventory::TKeyGen<FFaerieEntryKey> KeyGen;
};