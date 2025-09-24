// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "BinarySearchOptimizedArray.h"
#include "FaerieDefinitions.h"
#include "FaerieFastArraySerializerHack.h"
#include "ItemContainerExtensionBase.h"
#include "StructUtils/InstancedStruct.h"
#include "StructUtils/StructView.h"
#include "InventoryReplicatedDataExtensionBase.generated.h"

struct FFaerieReplicatedSimMap;

USTRUCT()
struct FFaerieReplicatedValue : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FFaerieReplicatedValue() = default;

	FFaerieReplicatedValue(const FFaerieAddress Key, const FInstancedStruct& Value)
	  : Key(Key),
		Value(Value) {}

	UPROPERTY(EditAnywhere, Category = "RepDataPerEntryBase")
	FFaerieAddress Key;

	UPROPERTY(EditAnywhere, Category = "RepDataPerEntryBase")
	FInstancedStruct Value;

	void PreReplicatedRemove(const FFaerieReplicatedSimMap& InArraySerializer);
	void PostReplicatedAdd(const FFaerieReplicatedSimMap& InArraySerializer);
	void PostReplicatedChange(const FFaerieReplicatedSimMap& InArraySerializer);
};

class URepDataArrayWrapper;
class UInventoryReplicatedDataExtensionBase;

/*
 * A replicated array of key/value pairs to emulate Map behavior over the network.
 * Implementation is accelerated by using a fast array for replication, and binary search for access.
 */
USTRUCT()
struct FFaerieReplicatedSimMap : public FFaerieFastArraySerializer,
								 public TBinarySearchOptimizedArray<FFaerieReplicatedSimMap, FFaerieReplicatedValue>
{
	GENERATED_BODY()

	friend TBinarySearchOptimizedArray;
	friend URepDataArrayWrapper;
	friend UInventoryReplicatedDataExtensionBase;

private:
	UPROPERTY()
	TArray<FFaerieReplicatedValue> Entries;

	// Enables TBinarySearchOptimizedArray
	TArray<FFaerieReplicatedValue>& GetArray() { return Entries; }

	/** Owning wrapper to send Fast Array callbacks to */
	// UPROPERTY() Fast Arrays cannot have additional properties with Iris
	// ReSharper disable once CppUE4ProbableMemoryIssuesWithUObject
	TObjectPtr<URepDataArrayWrapper> OwningWrapper;

	// Is writing to Entries locked? Enabled while ItemHandles are active.
	mutable uint32 WriteLock = 0;

public:
	TConstArrayView<FFaerieReplicatedValue> GetView() const { return Entries; }

	void RemoveValue(FFaerieAddress Address);
	FInstancedStruct& GetOrCreateValue(FFaerieAddress Address);
	void SetValue(FFaerieAddress Address, const FInstancedStruct& Data);

	struct FValueWriteScope : FNoncopyable
	{
		FValueWriteScope(const FFaerieAddress Address, FFaerieReplicatedSimMap& Source);
		~FValueWriteScope();

	protected:
		FFaerieReplicatedValue& Handle;

	private:
		FFaerieReplicatedSimMap& Source;

	public:
		FInstancedStruct* operator->() const { return &Handle.Value; }
		FStructView Get() const { return Handle.Value; }
	};

	FValueWriteScope GetWriteScope(const FFaerieAddress Address)
	{
		return FValueWriteScope(Address, *this);
	}

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return Faerie::Hacks::FastArrayDeltaSerialize<FFaerieReplicatedValue, FFaerieReplicatedSimMap>(Entries, DeltaParms, *this);
	}

	/*
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize) const;
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize) const;
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize) const;
	*/

	void PreDataReplicatedRemove(const FFaerieReplicatedValue& Data) const;
	void PostDataReplicatedAdd(const FFaerieReplicatedValue& Data) const;
	void PostDataReplicatedChange(const FFaerieReplicatedValue& Data) const;

	// Only const iteration is allowed.
	using TRangedForConstIterator = TArray<FFaerieReplicatedValue>::RangedForConstIteratorType;
	TRangedForConstIterator begin() const;
	TRangedForConstIterator end() const;
};

template<>
struct TStructOpsTypeTraits<FFaerieReplicatedSimMap> : public TStructOpsTypeTraitsBase2<FFaerieReplicatedSimMap>
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

	friend FFaerieReplicatedSimMap;
	friend UInventoryReplicatedDataExtensionBase;

public:
	virtual void PostInitProperties() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	void Server_PreContentRemoved(const FFaerieReplicatedValue& Data);
	void Server_PostContentAdded(const FFaerieReplicatedValue& Data);
	void Server_PostContentChanged(const FFaerieReplicatedValue& Data);

	void Client_PreContentRemoved(const FFaerieReplicatedValue& Data);
	void Client_PostContentAdded(const FFaerieReplicatedValue& Data);
	void Client_PostContentChanged(const FFaerieReplicatedValue& Data);

private:
	UPROPERTY(Replicated)
	TWeakObjectPtr<const UFaerieItemContainerBase> Container;

	UPROPERTY(Replicated)
	FFaerieReplicatedSimMap DataArray;
};

/**
 * This is the base class for Inventory extensions that want to replicate addition data per Address.
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
	//virtual void PreRemoval(const UFaerieItemContainerBase* Container, FEntryKey Key, int32 Removal) override;
	virtual void PostRemoval(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event) override;
	//~ UItemContainerExtensionBase

	// Children must implement this. It gives the struct type instanced per item.
	virtual UScriptStruct* GetDataScriptStruct() const PURE_VIRTUAL(UInventoryReplicatedDataExtensionBase::GetDataScriptStruct, return nullptr; )
	virtual bool SaveRepDataArray() const { return false; }

private:
	virtual void PreEntryDataRemoved(const UFaerieItemContainerBase* Container, const FFaerieReplicatedValue& Data) {}
	virtual void PreEntryDataAdded(const UFaerieItemContainerBase* Container, const FFaerieReplicatedValue& Data) {}
	virtual void PreEntryDataChanged(const UFaerieItemContainerBase* Container, const FFaerieReplicatedValue& Data) {}

protected:
	FConstStructView GetDataForHandle(FFaerieAddressableHandle Handle) const;

	bool EditDataForHandle(FFaerieAddressableHandle Handle, const TFunctionRef<void(FStructView)>& Edit);

private:
	TStructView<FFaerieReplicatedSimMap> FindFastArrayForContainer(const UFaerieItemContainerBase* Container);
	TConstStructView<FFaerieReplicatedSimMap> FindFastArrayForContainer(const UFaerieItemContainerBase* Container) const;

#if WITH_EDITOR
	void PrintPerContainerDataDebug() const;
#endif

private:
	UPROPERTY(Replicated)
	TArray<TObjectPtr<URepDataArrayWrapper>> PerContainerData;
};