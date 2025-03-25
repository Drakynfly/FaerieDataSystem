// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieDefinitions.h"
#include "FaerieFastArraySerializerHack.h"
#include "ItemContainerExtensionBase.h"
#include "StructUtils/InstancedStruct.h"
#include "StructUtils/StructView.h"
#include "InventoryReplicatedDataExtensionBase.generated.h"

struct FRepDataFastArray;

USTRUCT()
struct FRepDataPerEntryBase : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FRepDataPerEntryBase() = default;

	FRepDataPerEntryBase(const FEntryKey Key, const FInstancedStruct& Value)
	  : Key(Key),
		Value(Value) {}

	UPROPERTY(EditAnywhere, Category = "RepDataPerEntryBase")
	FEntryKey Key;

	UPROPERTY(EditAnywhere, Category = "RepDataPerEntryBase")
	FInstancedStruct Value;

	void PreReplicatedRemove(const FRepDataFastArray& InArraySerializer);
	void PostReplicatedAdd(const FRepDataFastArray& InArraySerializer);
	void PostReplicatedChange(const FRepDataFastArray& InArraySerializer);
};

class URepDataArrayWrapper;
class UInventoryReplicatedDataExtensionBase;

USTRUCT()
struct FRepDataFastArray : public FFaerieFastArraySerializer,
						   public TBinarySearchOptimizedArray<FRepDataFastArray, FRepDataPerEntryBase>
{
	GENERATED_BODY()

	friend TBinarySearchOptimizedArray;
	friend URepDataArrayWrapper;
	friend UInventoryReplicatedDataExtensionBase;

private:
	UPROPERTY()
	TArray<FRepDataPerEntryBase> Entries;

	// Enables TBinarySearchOptimizedArray
	TArray<FRepDataPerEntryBase>& GetArray() { return Entries; }

	/** Owning wrapper to send Fast Array callbacks to */
	UPROPERTY()
	TWeakObjectPtr<URepDataArrayWrapper> OwningWrapper;

public:
	TConstArrayView<FRepDataPerEntryBase> GetView() const { return Entries; }

	void RemoveDataForEntry(FEntryKey Key);
	void SetDataForEntry(FEntryKey Key, const FInstancedStruct& Data);

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return Faerie::Hacks::FastArrayDeltaSerialize<FRepDataPerEntryBase, FRepDataFastArray>(Entries, DeltaParms, *this);
	}

	/*
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize) const;
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize) const;
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize) const;
	*/

	void PreDataReplicatedRemove(const FRepDataPerEntryBase& Data) const;
	void PostDataReplicatedAdd(const FRepDataPerEntryBase& Data) const;
	void PostDataReplicatedChange(const FRepDataPerEntryBase& Data) const;

	// Only const iteration is allowed.
	using TRangedForConstIterator = TArray<FRepDataPerEntryBase>::RangedForConstIteratorType;
	FORCEINLINE TRangedForConstIterator begin() const { return TRangedForConstIterator(Entries.begin()); }
	FORCEINLINE TRangedForConstIterator end() const   { return TRangedForConstIterator(Entries.end());   }
};

template<>
struct TStructOpsTypeTraits<FRepDataFastArray> : public TStructOpsTypeTraitsBase2<FRepDataFastArray>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};

// A wrapper around a FRepDataFastArray allowing us to replicate it as a FastArray.
UCLASS(Within = InventoryReplicatedDataExtensionBase)
class URepDataArrayWrapper : public UNetSupportedObject
{
	GENERATED_BODY()

	friend FRepDataFastArray;
	friend UInventoryReplicatedDataExtensionBase;

public:
	virtual void PostInitProperties() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	void PreContentRemoved(const FRepDataPerEntryBase& Data);
	void PostContentAdded(const FRepDataPerEntryBase& Data);
	void PostContentChanged(const FRepDataPerEntryBase& Data);

private:
	UPROPERTY()
	TWeakObjectPtr<const UFaerieItemContainerBase> Container;

	UPROPERTY(Replicated)
	FRepDataFastArray DataArray;
};

/**
 * This is the base class for Inventory extensions that want to replicate addition data per Entry.
 * This is implemented by creating a FastArray wrapper object per bound container which efficiently replicates a custom
 * struct.
 */
UCLASS(Abstract)
class FAERIEINVENTORY_API UInventoryReplicatedDataExtensionBase : public UItemContainerExtensionBase
{
	GENERATED_BODY()

	friend URepDataArrayWrapper;

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//~ UItemContainerExtensionBase
	virtual FInstancedStruct MakeSaveData(const UFaerieItemContainerBase* Container) const override;
	virtual void LoadSaveData(const UFaerieItemContainerBase* Container, const FInstancedStruct& SaveData) override;
	virtual void InitializeExtension(const UFaerieItemContainerBase* Container) override;
	virtual void DeinitializeExtension(const UFaerieItemContainerBase* Container) override;
	virtual void PreRemoval(const UFaerieItemContainerBase* Container, FEntryKey Key, int32 Removal) override;
	//~ UItemContainerExtensionBase

	// Children must implement this. It gives the struct type instanced per item.
	virtual UScriptStruct* GetDataScriptStruct() const PURE_VIRTUAL(UInventoryReplicatedDataExtensionBase::GetDataScriptStruct, return nullptr; )
	virtual bool SaveRepDataArray() const { return false; }

private:
	virtual void PreEntryDataRemoved(const UFaerieItemContainerBase* Container, const FRepDataPerEntryBase& Data) {}
	virtual void PreEntryDataAdded(const UFaerieItemContainerBase* Container, const FRepDataPerEntryBase& Data) {}
	virtual void PreEntryDataChanged(const UFaerieItemContainerBase* Container, const FRepDataPerEntryBase& Data) {}

protected:
	FConstStructView GetDataForEntry(const UFaerieItemContainerBase* Container, const FEntryKey Key) const;

	bool EditDataForEntry(const UFaerieItemContainerBase* Container, const FEntryKey Key, const TFunctionRef<void(FStructView)>& Edit);

private:
	TStructView<FRepDataFastArray> FindFastArrayForContainer(const UFaerieItemContainerBase* Container);
	TConstStructView<FRepDataFastArray> FindFastArrayForContainer(const UFaerieItemContainerBase* Container) const;

private:
	UPROPERTY(Replicated)
	TArray<TObjectPtr<URepDataArrayWrapper>> PerContainerData;
};